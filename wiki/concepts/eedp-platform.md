---
title: EEDP — Embodied Embedded Development Platform
created: 2026-05-10
updated: 2026-05-10
type: concept
tags: [embedded, ncs, nrf54, wifi, test-automation, ai-agent, hardware, gpio]
confidence: medium
sources: []
---

# EEDP — Embodied Embedded Development Platform

## 概述

**EEDP** = **Embodied Embedded Development Platform**（具身嵌入式开发平台）。

一个 AI 可操控的嵌入式硬件开发平台。AI Agent（Claude Code / Cursor / Cline）通过 REST API 和 MCP 协议，从纯代码操作延伸到物理操作——真实的按按钮、读 LED、分析协议、控制网络环境。

"Embodied" 意味着 AI 不仅存在于数字世界（写代码、读 log），而是通过物理接口（GPIO/JLink/Saleae）与真实硬件直接交互——像人一样动手操作开发板。

## 架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        AI Agent                                 │
│          Claude Code · Cursor · Cline · Hermes Agent            │
└─────────────────────────┬───────────────────────────────────────┘
                          │ REST API · MCP 协议
┌─────────────────────────┴───────────────────────────────────────┐
│                 EEDP Controller (PC Host)                        │
│                                                                  │
│  ┌─────────────────┬─────────────┬─────────────┬──────────┬────┐│
│  │ Hardw Ctrl&Sense│ Debug/JLink │ LA / Saleae │  Router  │ Web││
│  │   GPIO button   │ flash/reset │ capture/    │  Control │    ││
│  │   LED read      │ addr2line   │ decode      │  SSH     │ API││
│  └────────┬────────┴──────┬──────┴──────┬──────┴────┬─────┴──┬─┘│
└───────────┼───────────────┼─────────────┼───────────┼────────┼──┘
            │               │             │           │        │
   ┌────────┴──┐     ┌──────┴──────┐ ┌────┴─────┐ ┌──┴──┐ ┌───┴───┐
   │nRF7002DK  │     │3× Target   │ │ Saleae   │ │Router│ │Memfault│
   │GPIO Shell │     │Board       │ │ Logic 2  │ │BE92U │ │Cloud   │
   └───────────┘     └─────────────┘ └──────────┘ └─────┘ └───────┘
```

![EEDP Architecture v0.3](../raw/assets/eedp-architecture-v0.3.svg)

## EEDP Controller — 五大模块

### 1. Hardware Control & Sense（硬件感知与控制）

**接口：** nRF7002DK 的 Zephyr GPIO Shell。通过 UART 发送 `gpio set/get/clear` 命令。

| 功能 | 命令 | 说明 |
|------|------|------|
| 按按钮 | `gpio set P1.00` | 控制三极管模拟按键按下 |
| 松按钮 | `gpio clear P1.00` | 释放 |
| 读 LED | `gpio get P0.00` | 返回 0 或 1 |

**接线：** 每块目标板 8 根线（4 按键 OUT + 4 LED IN），3 块板共 24 根。nRF5340 GPIO 充足（~40）。3.3V 逻辑，共地连接。

**可替换性：** 任何支持 `CONFIG_GPIO_SHELL=y` 的 Zephyr 板都可以替代 nRF7002DK。

**关键优势：** 零固件开发——Zephyr 内置 GPIO shell 已提供全部能力。按键和 LED 已由 nRF54LM20DK board configuration 引出到 Pin Header。

### 2. Debugger / JLink

**MCP 工具：** `jlink_mcp` (cyj0920/jlink_mcp ★7) 或 `dbgprobe-mcp-server` ★4。

| 功能 | MCP 工具 | 说明 |
|------|----------|------|
| Flash | `mcp_jlink_flash` | 烧录 .hex |
| Reset | `mcp_jlink_reset` | 复位目标板 |
| Debug | GDB attach | 通过 `west debug` 或 JLink 直连 |
| Coredump | addr2line | 解析 crash address |

**硬件：** 每块 nRF54LM20DK 自带 JLink OB（板载调试器），通过 USB 直连 PC。3 块板 = 3 个 USB 口。

### 3. Logic Analyzer / Saleae

**MCP 工具：** `logic-analyzer-ai-mcp` (wegitor ★9)。

| 功能 | MCP 工具 | 说明 |
|------|----------|------|
| 捕获 | `mcp_saleae_capture` | 多通道同时采样 |
| 解码 | `mcp_saleae_decode` | SPI / QSPI / MSPI / UART |
| 触发 | `mcp_saleae_trigger` | 条件触发 |
| 导出 | `mcp_saleae_export` | 输出 CSV / 波形图 |

**硬件：** Saleae Logic 2，8 通道，USB 连接 PC。Probe wires 分配到各块目标板测量 sQSPI (MSPI) 总线或其他关键信号。

### 4. Router Control

**接口：** SSH (paramiko) 连接 ASUS BE92U + Merlin 固件。

| 功能 | 命令 | 说明 |
|------|------|------|
| 重启 WiFi | `service restart_wireless` | 模拟 Wi-Fi 中断/恢复 |
| 阻断设备 | `iptables -A FORWARD -p tcp -m mac --mac-source XX:XX -j DROP` | 模拟网络异常 |
| 查询 ARP | `arp -a` | 查询目标板是否在线 |
| DHCP 控制 | `dnsmasq restart` | DHCP 服务控制 |

**可替换性：** 也可以使用 OpenWRT 路由器（`ubus` / `uci` 命令更全面）。

### 5. Web

**接口：** HTTPS REST API 和 subprocess 调用。

| 功能 | 方式 | 说明 |
|------|------|------|
| Memfault API | `requests.get/post` | 查看崩溃报告、OTA 状态、设备列表 |
| nrfutil | `subprocess.run` | `nrfutil device reset --sn X` |
| west flash | `subprocess.run` | `west flash -d build --recover --dev-id X` |

**Memfault:** 无现成 MCP — 通过 REST API 直接调用 (`api.memfault.com`)。API 文档在 `api-docs.memfault.com`。

## 目标硬件

| 组件 | 型号 | 数量 | 用途 |
|------|------|:---:|------|
| GPIO 控制器 | nRF7002DK (nRF5340) | 1 | Zephyr GPIO Shell 控制按键/LED |
| 目标板 | nRF54LM20DK + nRF7002EB2 | 3 | Wi-Fi 开发测试目标 |
| 逻辑分析仪 | Saleae Logic 2 | 1 | 协议分析 |
| 路由器 | ASUS BE92U + Merlin | 1 | Wi-Fi 网络环境控制 |

## 实施策略 — 以 Cursor 为例的分层方案

EEDP 的知识和工具按**持久性**分三层存储，避免每次会话加载冗余信息：

| 类别 | 内容 | 载入策略 |
|------|------|----------|
| 平台知识 | 硬件拓扑、API 端点、GPIO 映射 | CLAUDE.md，随项目加载 |
| 核心能力 | 按键/LED 控制、UART 读写、自检脚本 | scripts/，每次都跑 |
| 按需工具 | Saleae 分析、JLink 深度调试、Router 控制、Memfault 查阅 | MCP 或脚本，需要时启用 |

### 文件结构

```
eedp-project/
├── CLAUDE.md              ← 平台知识 + 入口索引（~300行）
├── eedp-config.yaml       ← GPIO 映射、串口列表
├── scripts/
│   ├── eedp-init.py       ← 自检：每次会话先跑
│   ├── eedp-controller.py ← FastAPI 服务（需要时启动）
│   ├── eedp-flash.py      ← 编译+烧录
│   └── eedp-test.py       ← 测试序列
└── .env                   ← API Key 等敏感配置
```

### 各文件职责

**CLAUDE.md** — 平台知识 + 入口索引

```markdown
# EEDP — Embodied Embedded Development Platform

## 硬件拓扑
- nRF7002DK（GPIO Shell, UART /dev/ttyUSB0）
- Target #1: nRF54LM20DK+nRF7002EB2（UART /dev/ttyUSB1, JLink SN=xxx）
- Target #2: ...

## 启动自检
每次会话开始，先运行：
$ python scripts/eedp-init.py

## 核心 API（本机 HTTP 服务）
EEDP Controller → http://localhost:8080
POST /press    {"board":1, "button":2}
GET  /led      {"board":1, "led":3}  → {"level":1}

## 常用工作流
- 开发(dev) — 改代码 → flash → 读 log
- 调试(debug) — 同上 + 激活 jlink_mcp
- 验证(validate) — python scripts/eedp-test.py <test_name>

## MCP 工具索引
| 工具 | 何时启用 |
| jlink_mcp | 需要烧录/调试时 |
| logic-analyzer-ai-mcp | 需要分析协议时 |
```

**scripts/eedp-init.py** — 启动自检

每条新会话第一阶段执行。验证所有硬件就绪：

```python
def self_test():
    ports = scan_serial_ports()
    assert ports['nRF7002DK'].gpio_shell_alive()
    for board in [1,2,3]:
        assert ports[f'Target{board}'].uart_alive()
    print("✅ EEDP Ready: 3/3 boards online")
```

**scripts/eedp-controller.py** — 常驻 FastAPI 服务

核心编排层，暴露 REST API 供 AI 通过 curl 调用。五个模块合一：

```python
controller.start()  # → localhost:8080
# HardwareControl — nRF7002DK GPIO shell 代理
# DebugManager    — JLink 操作 (按需加载 jlink_mcp)
# SaleaeManager   — Saleae MCP 代理 (按需启动)
# RouterManager   — SSH 路由器控制
# WebClient       — Memfault / nrfutil
```

**eedp-config.yaml** — 硬件配置

```yaml
nrf7002dk:
  port: /dev/ttyUSB0
  baud: 115200
targets:
  - name: board-1
    uart: /dev/ttyUSB1
    jlink_sn: "1051869687"
    buttons: {BTN1: P0.11, BTN2: P0.12, BTN3: P0.24, BTN4: P0.25}
    leds:   {LED1: P0.28, LED2: P0.29, LED3: P0.30, LED4: P0.31}
```

### 典型会话启动流程

```
用户：开始开发 nRF54LM20DK 的 WiFi 驱动

AI 行为：
1. 读取 CLAUDE.md → 了解硬件拓扑和 API
2. 执行自检 → $ python scripts/eedp-init.py
   ✅ EEDP Ready: 3/3 boards · GPIO Shell OK · Router reachable
3. 启动Controller → $ python scripts/eedp-controller.py start
   ✅ Controller on http://localhost:8080
4. 根据用户指令进入 dev / debug / validate 模式
```

### 按需加载对照表

| 资源 | 加载时机 | 方式 |
|------|----------|------|
| CLAUDE.md | 会话开始 | Cursor 自动加载 |
| eedp-config.yaml | 读取 CLAUDE.md 后 | AI 读取 |
| eedp-init.py | 每次第一件事 | AI 执行 |
| eedp-controller.py | 需要操作硬件时 | AI 启动 |
| jlink_mcp MCP | 需要烧录/调试 | AI 或用户启用 |
| logic-analyzer-ai-mcp MCP | 需要协议分析 | AI 或用户启用 |
| eedp-test.py | 需要跑测试序列 | AI 执行 |

## 软件开发闭环

```
edit (Claude Code) → build (west) → flash (mcp_jlink) → observe (gpio/uart/saleae) → analyze (memfault/ai) → edit → ...
```

一个完整的嵌入式开发循环，AI 全程自主执行。

## 现有 MCP / 工具生态

| 工具 | 项目 | ★ | 状态 |
|------|------|:---:|:---:|
| JLink | `jlink_mcp` / `dbgprobe-mcp-server` | 7/4 | 可用 |
| Saleae | `logic-analyzer-ai-mcp` | 9 | 可用 |
| UART Debug | `chsh-sk-ncs-debug` (skill) | — | 已实现 |
| GPIO Shell | Zephyr 内置 | — | 零固件开发 |
| Memfault | REST API | — | 无 MCP，直接 HTTP |

## 与现有技能的整合

- [chsh-sk-ncs-debug](concepts/chsh-sk-ncs-debug.md) — UART log capture, loop test, crash analysis 已覆盖
- [github-actions-ncs-ci](concepts/github-actions-ncs-ci.md) — CI/CD 集成
- [embedded-system-general-debugging](concepts/embedded-system-general-debugging.md) — 调试方法论

## 未解决的问题

- Router 是否从 Merlin 换到 OpenWRT（ubus 命令更丰富）
- eedp-controller.py 的具体 FastAPI 接口设计
- 启动自检脚本的深度（GPIO 回路测试 u 需要时短接按键和LED）
- MCP 工具在 Cursor 中的自动激活/停用流程
- 多板并发测试的竞态处理

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v0.1 | 2026-05-10 | 初始概念，三块目标板 + nRF7002DK GPIO 方案 |
| v0.2 | 2026-05-10 | 加入物理方案调研（电磁铁/舵机/Ingun探针），后改用 GPIO shell |
| v0.3 | 2026-05-10 | EEDP Controller 重构为 5 模块分类，详细架构图 |
| v0.4 | 2026-05-10 | 实施策略：三层分层方案（CLAUDE.md + scripts/ + MCP 按需加载），以 Cursor 为例的启动流程和自检规范 |
