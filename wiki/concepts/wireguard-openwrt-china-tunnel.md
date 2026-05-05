---
title: OpenWrt WireGuard 翻墙隧道
created: 2026-04-30
updated: 2026-04-30
type: guide
tags: [wireguard, openwrt, vpn, networking, troubleshooting, china, multi-device]
---

# OpenWrt WireGuard 翻墙隧道

> 经实测验证的完整配置。挪威→中国，OpenWrt 作服务器，一人用。

## 架构

```
朋友设备 (中国)                我的网络 (挪威)
WireGuard Client               ╷
10.0.0.2/24                    │ UDP:8443
DNS: 1.1.1.1, 8.8.8.8         │
AllowedIPs: 0.0.0.0/0          ▼
                    公网 IP:81.191.174.122:8443
                               │
                    主路由 (Platinum-6840)
                    端口转发 UDP 8443 → 192.168.1.75
                               │
                    Zyxel EX5700 OpenWrt
                    wg0: 10.0.0.1/24
                    NAT: wg0 → lan → wan
```

---

## 配置清单

### 1. OpenWrt 安装

```bash
apk add wireguard-tools luci-app-wireguard
```

> OpenWrt 25.x 内核已含 wireguard 模块，不需 kmod。

### 2. 密钥生成

```bash
cd /etc/wireguard
umask 077
wg genkey | tee server_private.key | wg pubkey > server_public.key
wg genkey | tee client_private.key | wg pubkey > client_public.key
wg genpsk    # 可选，增强安全性
```

### 3. 服务端配置 (`/etc/config/network`)

```
config interface 'wg0'
    option proto 'wireguard'
    option private_key '<server_private.key>'
    option listen_port '8443'
    list addresses '10.0.0.1/24'

config wireguard_wg0
    option public_key '<client_public.key>'    ← 公钥！不是私钥！
    option preshared_key '<PSK>'               ← 可选
    list allowed_ips '10.0.0.2/32'
    option persistent_keepalive '25'
```

### 4. 客户端配置

```ini
[Interface]
PrivateKey = <client_private.key>
Address = 10.0.0.2/24
DNS = 1.1.1.1, 8.8.8.8

[Peer]
PublicKey = <server_public.key>
AllowedIPs = 0.0.0.0/0
Endpoint = <公网IP>:8443           ← 公网 IP，不是内网！
PersistentKeepalive = 25
```

---

## 防火墙配置

### 关键事实：OpenWrt 的 WAN 口可能叫 `lan1`

物理接口 `lan1` 映射到 WAN zone（input 默认 REJECT）。WireGuard 包从铂金路由转发过来走的是 WAN 口，必须显式放行。

### 正确做法

```bash
# 1. 为 wg0 创建独立 zone（非 WAN zone）
uci add firewall zone
uci set firewall.@zone[-1].name='wg'
uci set firewall.@zone[-1].network='wg0'
uci set firewall.@zone[-1].input='ACCEPT'
uci set firewall.@zone[-1].output='ACCEPT'
uci set firewall.@zone[-1].forward='ACCEPT'
uci set firewall.@zone[-1].masq='1'

# 2. 从 wan zone 移除 wg0
uci del_list firewall.@zone[1].network='wg0'

# 3. 允许 wan → 8443
uci add firewall rule
uci set firewall.@rule[-1].name='Allow-WireGuard'
uci set firewall.@rule[-1].src='wan'
uci set firewall.@rule[-1].dest_port='8443'
uci set firewall.@rule[-1].proto='udp'
uci set firewall.@rule[-1].target='ACCEPT'

# 4. 转发
uci add firewall forwarding
uci set firewall.@forwarding[-1].src='wg'
uci set firewall.@forwarding[-1].dest='lan'
uci add firewall forwarding
uci set firewall.@forwarding[-1].src='wg'
uci set firewall.@forwarding[-1].dest='wan'

uci commit firewall && /etc/init.d/firewall restart
```

---

## 主路由端口转发 (Platinum-6840)

在 `http://192.168.1.254` → NAT / Port Forwarding：

| 协议 | WAN 端口 | LAN IP | LAN 端口 |
|------|----------|--------|----------|
| UDP  | 8443     | 192.168.1.75 | 8443 |

---

## 🐛 排坑实录

### 坑 1：OpenWrt LuCI 把客户端私钥当公钥填入

**症状**：tcpdump 看到握手包到达，但 `wg show` 永远无 `latest handshake`。

**判断**：
```bash
tcpdump -i any -n udp port 8443    # 有 UDP length 148 的包 → 包到达了
wg show                              # 无 handshake → 密钥错误
```

**修复**：用 `wg pubkey` 从私钥推导出真正的公钥。
```bash
echo "<private_key>" | wg pubkey
# 输出才是正确的 PublicKey
```

### 坑 2：wg0 被误放入 WAN zone

**症状**：密钥修了仍然无 handshake。

**判断**：
```bash
uci show firewall | grep wg
# firewall.@zone[1].network='wan' 'wan6' 'wg0'
# WAN zone input=REJECT → 包被丢弃
```

**修复**：创建独立 wg zone (input=ACCEPT)，从 wan zone 移除 wg0。

### 坑 3：防火墙规则方向写反

**症状**：`src='lan'` 规则无效。

**发现**：`network.wan.device='lan1'` — 流量从 WAN 口进入，不是 LAN。
**修复**：规则写 `src='wan'`，不是 `src='lan'`。

### 坑 4：Endpoint 写了内网 IP

**症状**：本地测试能通，蜂窝不通。

**修复**：客户端 `Endpoint` 必须用公网 IP（`81.191.174.122`），不能用 `192.168.1.75`。

### 坑 5：第一个 peer 的 AllowedIPs 是 /24，新设备无法握手

**症状**：加了新 peer，客户端配置正确，永远无 handshake。

**发现**：`wg show` 显示 `peer: xxx  allowed ips: 10.0.0.0/24` — 整个子网都被第一个 peer 占了。

**修复**：`uci set network.@wireguard_wg0[0].allowed_ips='10.0.0.2/32'`

---

## 多设备扩展

> **核心规则：一把密钥 = 一个设备，不可共用。**

| 设备 | 私钥 | 公钥 | 服务端 peer IP |
|------|------|------|---------------|
| 朋友手机 | `client1_private` | `client1_pub` | `10.0.0.2/32` |
| 朋友电脑 | `client2_private` | `client2_pub` | `10.0.0.3/32` |
| 备用设备 N | `clientN_private` | `clientN_pub` | `10.0.0.N/32` |

### 添加新设备（LuCI 网页版）

1. LuCI → Network → Interfaces → wg0 → Peers → **Add**
2. 在新设备上生成密钥，填入 Public Key
3. Allowed IPs: `10.0.0.N/32`（注意是 `/32`）
4. 可选 Route Allowed IPs、Persistent Keepalive = 25
5. Save & Apply

### 添加新设备（命令行）

```bash
# 1. 为新设备生成密钥（在 Zyxel 上代劳，或让设备自行生成）
wg genkey | tee /tmp/peer_private.key | wg pubkey > /tmp/peer_public.key

# 2. 添加 peer
uci add network wireguard_wg0
uci set network.@wireguard_wg0[-1].description='Friend_Laptop'
uci set network.@wireguard_wg0[-1].public_key='<peer_public.key 的内容>'
uci add_list network.@wireguard_wg0[-1].allowed_ips='10.0.0.3/32'

uci commit network
ifdown wg0 && ifup wg0
```

#### 实际案例：添加 Device3（公钥 `RebN2R...wTk=`，IP `10.0.0.3/32`）

```bash
uci add network wireguard_wg0
uci set network.@wireguard_wg0[-1].description='Device3'
uci set network.@wireguard_wg0[-1].public_key='RebN2RPA+nMPcYOwI/d3Sf1+aH3Csm940eNgO/wwsTk='
uci add_list network.@wireguard_wg0[-1].allowed_ips='10.0.0.3/32'
uci commit network
ifdown wg0 && ifup wg0
```

### ⚠️ 关键：第一个 peer 的 AllowedIPs 必须改成 /32

如果之前用 LuCI 创建 peer 时填了 `10.0.0.0/24`，这个 peer 会"霸占"整个子网——新 peer 永远抢不到握手。

```bash
uci set network.@wireguard_wg0[0].allowed_ips='10.0.0.2/32'
```

### 客户端配置模板（每设备一样的部分）

```ini
[Peer]
PublicKey = <server_public.key>
Endpoint = 81.191.174.122:8443
AllowedIPs = 0.0.0.0/0
PersistentKeepalive = 25
```

只有 `[Interface]` 里的 `PrivateKey` 和 `Address` 每台设备不同。

---

## 验证清单

```bash
# 服务端
wg show                          # 有 latest handshake
tcpdump -i any -n udp port 8443 # 有 UDP length 148 入站包

# 客户端
curl -s ifconfig.me              # 返回公网 IP (81.191.174.122)
nslookup google.com              # 能解析（不被 DNS 污染）
curl -s -o /dev/null -w '%{http_code}' https://www.youtube.com  # 200
```

---

## 故障速查

| 症状 | 查 |
|------|----|
| wg show 无 handshake | ① 密钥是否对 (wg pubkey 验) ② 主路由 UDP 端口转发 ③ 防火墙 wan src ACCEPT |
| 有 handshake 不能上网 | ① wg zone 有 masq ② forwarding wg→wan |
| 能上网但部分网站不行 | MTU 调低到 1280 |
| 几分钟就断 | PersistentKeepalive=25 |

---

## 本文工作配置

| 参数 | 值 |
|------|-----|
| 服务端 IP | 10.0.0.1/24 |
| 客户端 IP | 10.0.0.2/24 |
| 端口 | UDP 8443 |
| 公网入口 | 81.191.174.122 |
| 服务端公钥 | `5Y1wV+7amnbrFxrh+S+sKKZ2k3tSS6eCt9GpS5x/6zE=` |
| 客户端公钥 | `kCatu1kH0QuL8pNzosZZLC5R7wfehFRkAL3kK4QScWM=` |
| DNS | 1.1.1.1, 8.8.8.8 |
| MTU | 1420 |
