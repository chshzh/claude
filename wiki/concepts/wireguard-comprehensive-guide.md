---
title: WireGuard 全面指南
created: 2026-05-01
updated: 2026-05-01
type: concept
tags: [networking, vpn, wireguard, openwrt, embedded, guide]
sources: [wireguard.com]
---

# WireGuard 全面指南

## 概述

WireGuard 是一个极度简约但极具安全性的 VPN 协议与实现，由 Jason A. Donenfeld 于 2015 年开始开发，2020 年 3 月并入 Linux 5.6 主线内核。它只有一个目标：比 OpenVPN/IPsec 更快、更简单、更安全、更小——并且做到了。

| 核心指标 | 数值 |
|---|---|
| 内核模块代码量 | ~4,000 行 C |
| 协议框架 | Noise IKpsk2 (1-RTT 握手) |
| 加密原语 | Curve25519 + ChaCha20Poly1305 + BLAKE2s |
| 传输层 | UDP 单端口（默认 51820） |
| 调度方式 | 无守护进程，以内核网络设备形式运行 |

---

## 技术架构

### 协议设计

WireGuard 基于 **Noise Protocol Framework** 的 `Noise_IKpsk2` 模式：
- **IK** = 发起方预先知道接收方的静态公钥
- **psk** = 可选预共享密钥（提供抗量子安全层）
- 一次握手同时完成身份验证和会话密钥派生

**握手流程（4条消息）：**
1. 发起方发送：静态公钥 + 临时公钥 + 时间戳 + MAC（通过静态-静态 DH + 临时-静态 DH 加密）
2. 响应方回复：临时公钥 + 加密的静态公钥 + 加密的首包数据 + MAC（双向密钥建立）
3. 发起方发送最终确认（负载高时可能是 cookie 响应）
4. 传输密钥由 BLAKE2s 派生，64 位计数器防重放

**Keepalive 机制：**
- 闲置 10 秒后发送 32 字节空包，维持 NAT 绑定
- `PersistentKeepalive` 可配置间隔（NAT 后推荐 25 秒）

### 加密原语

| 功能 | 原语 | 作用 |
|---|---|---|
| 密钥交换 | Curve25519 (X25519) | 静态 + 临时 DH |
| 对称加密+认证 | ChaCha20Poly1305 (IETF 变体) | 载荷加密 |
| 哈希/KDF | BLAKE2s (256-bit) | 密钥派生 |
| 公钥指纹 | BLAKE2s(公钥) | 节点身份标识 |
| 可选 | 预共享密钥 (PSK) | 多层对称加密 |

所有原语均经过持续审计（Cure53、OSTIF），无协商环节——一套固定的原语，硬编码，彻底杜绝降级攻击。

### 内核集成方式

- Linux 5.6 正式合入，作为纯内核网络设备驱动（`/drivers/net/wireguard/`）
- 通过 **Netlink** 接口管理：`wg` 命令通过 `rtnetlink` 控制
- 零拷贝发送路径、批量加密（SSE/AVX/AES-NI 卸载）、中等硬件可跑满 >1 Gbps
- 密钥常驻内核内存，永不交换至磁盘

---

## 快速上手

### 密钥生成

```bash
# 生成公钥/私钥对
wg genkey | tee privatekey | wg pubkey > publickey
# 或带 PSK
wg genpsk > presharedkey
```

### 最简单的点对点配置

**服务端 (10.0.0.1/24, 公网 1.2.3.4):**
```ini
[Interface]
PrivateKey = <server-private>
Address = 10.0.0.1/24
ListenPort = 51820

[Peer]
PublicKey = <client-public>
AllowedIPs = 10.0.0.2/32
```

**客户端 (10.0.0.2/24, NAT 后):**
```ini
[Interface]
PrivateKey = <client-private>
Address = 10.0.0.2/24

[Peer]
PublicKey = <server-public>
Endpoint = 1.2.3.4:51820
AllowedIPs = 0.0.0.0/0, ::/0   # 全流量隧道
PersistentKeepalive = 25
```

### wg-quick

`wg-quick` 是官方推荐的便捷脚本，自动完成：
- 创建 wg 接口
- 分配 IP 地址
- 基于 `AllowedIPs` 自动添加路由
- 配置 iptables/nftables MASQUERADE（如需要）
- 设置 `fwmark` 防止路由环路

启用：`wg-quick up wg0` | 停用：`wg-quick down wg0`

---

## 组网应用

### 关键概念：AllowedIPs

WireGuard 最独特的设计：**AllowedIPs 同时完成路由和访问控制**。它既是"这个对端允许发送哪些 IP 的流量"，也是"通往这些 IP 的路由指向谁"。没有传统的路由表配置。

> `AllowedIPs = 0.0.0.0/0, ::/0` = 全流量隧道（VPN 网关模式）
> `AllowedIPs = 10.0.0.0/24, 192.168.1.0/24` = 特定子网互访

### 常见拓扑

#### 1. Hub-and-Spoke（中心辐射型）

最常用：一台服务器做中心节点，多台客户端通过它互访。

```
客户端A ──→ 服务器 ──→ 客户端B
```

**服务器配置：** 每个客户端作为一个 Peer，`AllowedIPs` 为客户端自己的隧道 IP
**客户端配置：** 全流量或目标子网指向服务器

**优点：** 配置简单，集中管理
**缺点：** 所有流量经服务器转发，带宽瓶颈，延迟增加一跳

#### 2. Site-to-Site（站点互联）

两个子网通过 WireGuard 隧道互联，类似传统 IPSec 站点。

```
办公室 (192.168.1.0/24) ──── 隧道 ──── 数据中心 (10.0.0.0/8)
```

双方均需开启 `net.ipv4.ip_forward=1`，添加双方子网的 `AllowedIPs`，并配置 iptables/nftables 转发规则。

#### 3. Mesh 全网状

每个节点与所有其他节点直接互联。

```
  节点A ──── 节点B
    ╲       ╱
     ╲     ╱
      节点C
```

每台机器上配置所有对端。使用 [Netmaker](https://netmaker.io) 或 [Tailscale](https://tailscale.com) 自动管理密钥分发和路由。

### NAT 穿越与 Roaming

- **NAT 穿越：** WireGuard 本身不做 UPnP/NAT-PMP，依靠 `PersistentKeepalive` 保持 NAT 映射。对端通过 UDP 套接字向公网发心跳，NAT 路由器维持映射。
- **Roaming（漫游）：** 当收到来自新源 IP/端口的合法数据包时，WireGuard **自动更新对端 endpoint**，无需重新握手。这对移动设备极其重要——从 4G 切换到 WiFi，隧道自动保持。

### MTU 考量

WireGuard 封装开销 = 60 字节（IPv4 + UDP + WireGuard 头部）：

| 物理链路 MTU | 推荐 wg MTU |
|---|---|
| 以太网 1500 | 1420 |
| PPPoE 1492 | 1360 |
| 蜂窝 1500 | 1420（注意某些运营商更小） |

### 防火墙集成

```bash
# iptables
iptables -A FORWARD -i wg0 -j ACCEPT
iptables -A FORWARD -o wg0 -j ACCEPT
iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

# nftables
table inet filter {
  chain forward {
    type filter hook forward priority 0
    iifname "wg0" accept
    oifname "wg0" accept
  }
}
```

`wg-quick` 会在 `up` 时自动添加上述 `MASQUERADE` 规则（`Table = auto` 时）。

### fwmark 与策略路由

WireGuard 使用 `fwmark = 0xca6c` 标记自身发出的 UDP 包，配合策略路由防止路由环路：
- 业务流量走 wg 接口
- WireGuard 的 UDP 传输流量走物理接口
- `wg-quick` 自动创建独立路由表 + ip rule

---

## 嵌入式应用

WireGuard 的极简设计使其在嵌入式系统中极具优势——4,000 行内核代码意味着更小的 flash 占用、更少的攻击面、更低的 CPU 需求。

### OpenWrt

自 OpenWrt 21.02 起，WireGuard 是默认 VPN。在路由器上性能远优于 OpenVPN。

| 路由器 CPU | WireGuard 吞吐量 | OpenVPN 吞吐量（对比） |
|---|---|---|
| MIPS 24Kc @ 580 MHz | ~60 Mbps | ~15 Mbps |
| MIPS 74Kc @ 720 MHz | ~90 Mbps | ~20 Mbps |
| ARM Cortex-A9 @ 720 MHz | ~130 Mbps | ~30 Mbps |

**安装：**
```bash
opkg update
opkg install wireguard-tools luci-app-wireguard
```

**LuCI 注意：** 可能存在填写 Peer 公钥时误填成客户端私钥的 UI bug——务必在终端用 `wg pubkey < privatekey` 推导公钥后再填入。

> 🔗 详见已有页面：[[wireguard-openwrt-china-tunnel]]

### Buildroot

```bash
# 启用 WireGuard 内核支持
BR2_PACKAGE_WIREGUARD_LINUX_COMPAT=y   # < 5.6 内核
BR2_PACKAGE_WIREGUARD_TOOLS=y

# 内核配置 (≥ 5.6 自动使用主线)
CONFIG_WIREGUARD=y
```

总占用约 1.3 MB（工具 + 模块），适合 8 MB flash 设备。

### Yocto

```
# meta-openembedded/meta-networking 层
IMAGE_INSTALL:append = " wireguard-tools"

# 内核配置片段
CONFIG_WIREGUARD=y
```

跨编译对 ARM、MIPS、RISC-V 均开箱支持。

### Zephyr RTOS / nRF Connect SDK

Zephyr 自 2.6.0 开始原生支持 WireGuard（`CONFIG_NET_WIREGUARD`）。这对 Nordic Semiconductor 芯片尤其重要：

**nRF9160 (Cortex-M33 + LTE-M/NB-IoT)：**
- WireGuard 跑在应用核心上
- 使用 Arm CryptoCell CC310 硬件加速 ChaCha20/Poly1305 和 Curve25519
- 带硬件卸载：~2.5 Mbps | 不带卸载：~0.5 Mbps
- 内存：~40 KB RAM + ~70 KB Flash
- 示例：`samples/net/wireguard` (Zephyr) / `nrf/samples/nrf9160/wireguard` (NCS)

**nRF5340 (双核 Cortex-M33)：**
- 应用核心跑 WireGuard，网络核心处理 802.15.4/蓝牙
- 同样可调用 CryptoCell 310

**限制：** 单 WireGuard 接口、有限对端数（~8-16，受 RAM 约束）

**真实案例：** 资产追踪器、智能电表、环境传感器——通过 WireGuard 将 nRF9160 数据安全传输到 AWS/Azure IoT Hub。

### 轻量级 Userspace 实现

**wireguard-Go**（官方 Go 实现）：
- 全用户态，无需内核模块
- 二进制 ~5 MB，运行时 ~5-10 MB RAM
- 性能约为内核模块的 80%
- 适合无法加载内核模块的嵌入式 Linux（busybox、容器）

**boringtun**（Cloudflare Rust 实现）：
- 二进制仅 ~100 KB，RAM ~1 MB
- 专为嵌入式/移动端设计
- 可编译至 ARM、MIPS、RISC-V
- Cloudflare Zero Trust 客户端内置使用

> 注意：无 MMU 的 MCU（Cortex-M 系列）无法运行 Go/Rust 运行时，应使用 Zephyr 原生 WireGuard。

### 性能优化建议

| 场景 | 建议 |
|---|---|
| MCU 无硬件加密 | ＜1 Mbps，考虑限制对端数（≤4）|
| MCU 有硬件卸载（CC310） | 2-5 Mbps，够用于遥测/FOTA |
| 嵌入式 Linux @ 500 MHz | ~30 Mbps，可跑轻量级网关 |
| 路由器 MIPS @ 580 MHz | ~60 Mbps，适合家庭/小企业 |

**最佳实践：**
- 优先使用硬件加密卸载（CC310、ARM Crypto Extensions）
- 限制对端数量（嵌入式设备建议 ≤16）
- 合理安排 `PersistentKeepalive` 间隔以减少电池消耗
- 结合 Nordic KMU 安全存储 WireGuard 私钥

---

## 与主流 VPN 方案对比

| 维度 | WireGuard | OpenVPN | IPsec/IKEv2 |
|---|---|---|---|
| 代码量 | ~4,000 行 | O(100k) 行 | 数十万行 |
| 加密协商 | 无协商，一套原语硬编码 | 多选项可降级 | 复杂算法协商 |
| 握手延迟 | 1-RTT (3-4 条消息) | 2-6+ RTT (TLS) | 4-8+ 消息 (IKEv2) |
| 连接模型 | 无状态 UDP | 有状态 TCP/UDP | 有状态 |
| NAT 穿越 | 内置 Keepalive | 需额外配置 | 需 NAT-T |
| 漫游 | 自动——新 IP 立即可用 | 需重新连接 | 需 MOBIKE |
| 配置复杂度 | 1 公钥 + 1 IP 列表/对端 | 证书/配置文件 | 复杂策略配置 |
| 审计覆盖 | 全面 (Cure53, OSTIF) | 有，但攻击面大 | 有，但复杂 |

---

## 常见坑点

1. **公钥私钥混淆**：`AllowedIPs` 配的是**对端**的隧道 IP，不是自己的。
2. **防火墙未放行 UDP**：WireGuard 使用 UDP。未放行端口时表现为主机不可达。
3. **AllowedIPs = 0.0.0.0/0 导致 SSH 断开**：如果通过 SSH 配置远程服务器，全流量隧道会切断 SSH 会话。预先准备两条连接，或先写路由排除 SSHD 的 IP。
4. **双端 NAT**：两端都在 NAT 后无法直接互通（需要打洞服务器或者 DERP 中继）。
5. **内核版本 < 5.6**：需自行编译 `wireguard-linux-compat` 内核模块。
6. **MTU 未调整**：默认 1420 在 PPPoE 链路上导致分片。以 MTU 1492 - 60 = 1432，实际环境酌情更小。
7. **wg-quick 的 MASQUERADE 冲突**：如有自己的 iptables 规则，需注意 `wg-quick` 自动添加的 POSTROUTING 规则可能与你已有的规则重叠。

---

## 推荐工具 & 生态

| 工具 | 用途 |
|---|---|
| [wg](https://www.wireguard.com/quickstart/) | 官方命令行管理工具 |
| [wg-quick](https://www.wireguard.com/quickstart/) | 便捷配置脚本 |
| [Netmaker](https://netmaker.io) | WireGuard 全自动网格 |
| [Tailscale](https://tailscale.com) | 基于 WireGuard 的身份 + 网格方案 |
| [Headscale](https://github.com/juanfont/headscale) | Tailscale 开源控制面 |
| [Subspace](https://subspace.com) | WireGuard 集中管理面板 |
| [wg-easy](https://github.com/wg-easy/wg-easy) | Docker 一键部署 WireGuard |
| [Algo VPN](https://github.com/trailofbits/algo) | WireGuard + IPsec 自动部署脚本 |

---

## 快速参考

```bash
# 常用命令
wg show                          # 查看所有接口状态
wg show wg0                      # 查看指定接口
wg showconf wg0                  # 导出配置
wg set wg0 peer <pubkey> remove  # 移除对端
wg set wg0 peer <pubkey> endpoint <ip:port>  # 更新端点
wg-quick up wg0                  # 启动接口
wg-quick down wg0                # 停止接口
systemctl enable wg-quick@wg0    # 开机自启

# 调试
ping 10.0.0.1                    # 测试隧道连通性
tcpdump -i wg0                   # 查看隧道内流量
tcpdump -i eth0 udp port 51820   # 查看 WireGuard 封装后的流量
```
