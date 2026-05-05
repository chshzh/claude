---
title: Hermes Docker Compose 部署
created: 2026-05-03
updated: 2026-05-03
type: concept
tags: [docker, self-hosted, nas, reference]
sources: [raw/articles/hermes-docker-compose-source.md]
confidence: high
---

# Hermes Agent + WebUI Docker Compose 部署

> **5 分钟阅读** · 在 NAS 上用 Docker Compose 一键部署 Hermes Agent 和 WebUI

## 架构一览

```
docker-compose up
    │
    ├─ ① hermes-init (busybox, run once)
    │     └─ 统一修正 /opt/data 和 /opt/webui 的权限
    │         chmod 755 + chown 10000:10000
    │
    ├─ ② hermes-agent (when init done ✓)
    │     └─ hermes gateway run → :8642
    │
    └─ ③ hermes-webui (when init done ✓)
          └─ Node server → :8787
```

三个服务，一条依赖链：**init → agent + webui 并行启动**

## 服务拆解

### 1. hermes-init — 权限修正

```yaml
hermes-init:
  image: busybox
  user: root                       # 必须 root 才能 chown
  restart: "no"                    # 一次性任务
  command: >
    sh -c "
      chmod 755 /opt/data &&
      chown -R 10000:10000 /opt/data &&
      mkdir -p /opt/webui &&
      chmod 755 /opt/webui &&
      chown -R 10000:10000 /opt/webui
    "
  volumes:
    - /volume1/CharlieII/hermes/data:/opt/data
    - /volume1/CharlieII/hermes/webui:/opt/webui
```

**为什么需要它？** NAS 上的文件可能属于任意 UID。agent 和 webui 容器内部用户是 UID 10000，如果宿主机文件 UID 不匹配，容器进程无法写入。init 容器以 root 跑，一次性修正权限，然后退出。^[raw/articles/linux-docker-permissions-source.md]

### 2. hermes-agent — 核心引擎

```yaml
hermes-agent:
  image: nousresearch/hermes-agent:latest
  depends_on:
    hermes-init:
      condition: service_completed_successfully   # init 必须成功
  command: sh -c "hermes gateway run"
  ports:
    - "8642:8642"                  # gateway 端口
  environment:
    - HOME=/opt/data               # 持久化数据目录
    - PATH=/opt/hermes/.venv/bin:/usr/local/bin:/usr/bin:/bin
  volumes:
    - /volume1/CharlieII/hermes/data:/opt/data    # 配置、记忆、会话
    - hermes-agent-src:/opt/hermes                # agent 源码（named volume）
    - /volume1/CharlieII:/home                    # 整个工作区
```

**关键设计：**
- `HOME=/opt/data` — 所有持久化数据（config.yaml, memory, sessions）都在 NAS 上
- `/volume1/CharlieII:/home` — 映射整个工作区，agent 可以访问所有项目文件
- `hermes-agent-src` 是 named volume，与 webui 共享 agent 源码

### 3. hermes-webui — 前端界面

```yaml
hermes-webui:
  image: ghcr.io/nesquena/hermes-webui:latest
  depends_on:
    hermes-init:
      condition: service_completed_successfully
  ports:
    - "8787:8787"
  volumes:
    - /volume1/CharlieII/hermes/data:/home/hermeswebui/.hermes
    - /volume1/CharlieII/hermes/webui:/home/hermeswebui/.hermes/webui
    - /volume1/CharlieII/hermes/data/workspace:/workspace
    - hermes-agent-src:/home/hermeswebui/.hermes/hermes-agent
  environment:
    - HERMES_WEBUI_HOST=0.0.0.0
    - HERMES_WEBUI_PORT=8787
    - HERMES_WEBUI_STATE_DIR=/home/hermeswebui/.hermes/webui
```

**关键设计：**
- WebUI 和 agent 共享同一份 `hermes/data`（通过不同的容器内路径映射）
- 共享 `hermes-agent-src` named volume — webui 需要 agent 源码来显示技能列表等
- `workspace` 独立映射为 `/workspace`，方便 webui 访问工作文件

## 部署步骤

```bash
# 1. 创建宿主机目录
mkdir -p /volume1/CharlieII/hermes/{data,webui}

# 2. 启动
cd /mnt/CharlieII/hermes_backup
docker compose up -d

# 3. 验证
docker compose ps                    # 三个容器都应 Up
curl http://localhost:8642/health    # agent gateway
curl http://localhost:8787           # webui

# 4. 查看日志
docker compose logs hermes-agent
docker compose logs hermes-webui
```

## 设计要点总结

| 要素 | 做法 | 原因 |
|------|------|------|
| 权限 | init 容器 `chown 10000:10000` | 消除 UID 不匹配 |
| 持久化 | bind mount 到 NAS 路径 | 容器重建数据不丢失 |
| 源码共享 | named volume `hermes-agent-src` | agent 和 webui 需要同一份代码 |
| 启动顺序 | `depends_on` + `condition: service_completed_successfully` | 权限就位后再启动应用 |
| 网络 | host 端口 8642 + 8787 | 局域网直接访问 |

## 排错

| 症状 | 检查 |
|------|------|
| agent 起不来 | `docker compose logs hermes-init` — init 是否成功？ |
| webui 连不上 agent | webui 通过 gateway 端口通信，确认 `HERMES_WEBUI_HOST` 配置 |
| 权限报错 | `docker exec hermes-agent ls -la /opt/data` 检查 UID |
| 配置不生效 | 确认 `HOME=/opt/data`，config.yaml 是否在正确位置 |

## 关联页面

- [[linux-vm-docker-permission]] — Linux Docker/VM UID 权限对齐详解
- [[hermes-architecture]] — Hermes Agent 架构全景
- [[hermes-setup-on-linux-with-nas-nfs-backup]] — NAS NFS 备份架构
