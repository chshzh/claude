---
title: ttyd Web Terminal Setup
created: 2026-05-03
updated: 2026-05-03
type: concept
tags: [self-hosted, homelab, guide, troubleshooting]
sources: []
confidence: high
---

# ttyd — 轻量 Web 终端

> **2 分钟阅读** · 在浏览器里获得完整终端体验，绕过 noVNC 剪贴板限制

## 1. 是什么

[ttyd](https://github.com/tsl0922/ttyd) 是一个轻量级工具，把命令行终端共享到网页上。浏览器访问 → 获得一个完整的 bash shell，支持原生复制粘贴（`Ctrl+Shift+V` / `Cmd+V`），无需 SSH 客户端。

**核心价值：** 绕过虚拟机管理平台 noVNC 的剪贴板限制（如绿联 UGOS Pro KVM），在浏览器里直接操作终端。

## 2. 安装

```bash
sudo apt-get install -y ttyd
```

## 3. 启动与使用

```bash
# 默认只读模式
ttyd -p 8989 bash

# 可写模式（-W 允许输入）
ttyd -W -p 8989 bash
```

浏览器访问 `http://<VM_IP>:8989`。

**重要：** 不加 `-W` 时终端是只读的，无法输入任何内容。这是最常见的问题。

## 4. 开机自启（systemd）

```bash
sudo tee /etc/systemd/system/ttyd.service <<'EOF'
[Unit]
Description=ttyd web terminal
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=charlie
ExecStart=/usr/bin/ttyd -W -p 8989 bash
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable --now ttyd
```

验证：`sudo systemctl status ttyd --no-pager`

## 5. 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 能看到终端但不能输入 | 缺少 `-W` 参数 | `ttyd -W -p 8989 bash` |
| `command not found`（如 `hermes`） | ttyd 是非登录 shell，没读 `.profile`，PATH 缺少 `~/.local/bin` | 在 `~/.bashrc` 开头加 `PATH="$HOME/.local/bin:$PATH"` |
| 提示符带有 `08:54:38` 时间戳 | oh-my-bash font 主题自带时钟 | `export THEME_SHOW_CLOCK=false` 加在 `.bashrc` 里 oh-my-bash 加载之前 |
| 端口被占用 | 端口已在使用 | 换端口：`ttyd -W -p 8081 bash` |

### 修复 .bashrc 完整示例

以下是在 `.bashrc` 中解决 ttyd 常见问题的完整配置：

```bash
# 关掉 oh-my-bash 时钟前缀（必须在 oh-my-bash 加载前设置）
export THEME_SHOW_CLOCK=false

# ... oh-my-bash 加载 ...

# 确保 ~/.local/bin 在 PATH（非登录 shell 如 ttyd 不读 .profile）
if [ -d "$HOME/.local/bin" ]; then
    PATH="$HOME/.local/bin:$PATH"
fi

# hermes 自动补全（只在可用时加载）
if command -v hermes >/dev/null 2>&1; then
    eval "$(hermes completion bash)"
fi
```

## 6. noVNC vs ttyd vs SSH 对比

| | noVNC | ttyd | SSH |
|---|---|---|---|
| 图形桌面 | ✅ | ❌ | ❌ |
| 剪贴板 | ❌ (绿联 UGOS Pro) | ✅ | ✅ |
| 需要客户端 | 浏览器 | 浏览器 | Terminal.app |
| 终端操作 | 可用但不便 | ✅ 原生体验 | ✅ |
| 设置难度 | 已有 | 一行命令 | 一行命令 |

**推荐组合：** noVNC 作为"显示器"看 VM 桌面状态 + ttyd 或 SSH 做实际终端交互。

## 关联页面

- [[linux-vm-docker-permission]] — UID 权限对齐：避免容器/VM 间的文件权限问题
- [[hermes-setup-on-linux-with-nas-nfs-backup]] — NAS NFS 架构：Hermes 数据持久化
