---
title: Linux VM / Docker UID Permission Alignment
created: 2026-05-03
updated: 2026-05-03
type: concept
tags: [linux, docker, vm, uid, permissions, self-hosted, troubleshooting]
sources: [raw/articles/linux-docker-permissions-source.md]
confidence: high
---

# Linux VM / Docker UID 权限对齐

> **5 分钟阅读** · 跨宿主机/VM/容器的 UID 权限问题根治方案

## 1. Linux 权限模型（30 秒复习）

每个文件有三层身份 + 三种权限：

| 身份 | 权限 | 数值 |
|------|------|------|
| Owner (user) | read | 4 |
| Group | write | 2 |
| Everyone else | execute | 1 |

`chmod 755` = owner 全权 (7) + group/others 只读执行 (5)

Linux 内核只认 **UID/GID 数字**，不管用户名。跨系统共享文件时 UID 必须一致，否则权限错乱。

## 2. 核心问题

**UID 不匹配**。当宿主机/VM/容器使用不同 UID 时，共享文件系统上会出现：

- 宿主机创建的文件在 VM 里显示为数字 UID（如 `1002 uucp`）
- VM 进程无法读写宿主机 UID 拥有的文件（如 `drwx------` 目录）
- `Permission denied` 即使 `ls` 看起来正常

典型场景：

| 场景 | 宿主 UID | VM/容器 UID | 表现 |
|------|----------|------------|------|
| Ugreen NAS + Ubuntu VM | 1002 (Charlie) | 1000 (charlie) | `hermes/` drwx------ → 容器无法访问 |
| Docker bind mount | 1026 (nasuser) | 1000 (container) | Volume 写失败: `EACCES` |
| NFS 共享 | 1000 (host) | 1001 (VM) | 文件 owner 显示为数字 |

## 3. 解决方案

### 方案 A：对齐 UID（根治疗法，推荐） ✅

**适用范围：** 你能控制 VM 或容器内的用户 UID。

**原理：** 让 VM 内用户的 UID = 共享宿主机的 UID。所有权限问题自然消失。

**实战案例：Synology NAS (uid=1002) + Ubuntu VM (uid=1000)**

```bash
# 预备：安装 SSH server（避免操作中被踢出）
sudo apt-get install -y openssh-server
sudo systemctl enable --now ssh

# 从另一台机器 SSH 进入（不要在被改用户会话中执行）
ssh charlie@192.168.75.30
sudo -i

# 关键：先后台 disown usermod，再杀进程
# 这样 usermod 不会因为连接断开而丢失
bash -c '(sleep 2; usermod -u 1002 charlie) & disown'
pkill -9 -u charlie

# 重新连接
ssh charlie@192.168.75.30
id
# uid=1002(charlie) gid=1000(charlie) groups=1000(charlie),...
```

**为什么 `(sleep 2; usermod) & disown` 是必需的：**
- `usermod` 不能改正在使用的用户的 UID
- `pkill -9 -u charlie` 会杀掉所有 charlie 进程（包括你自己的 SSH！）
- 如果不 detach，`usermod` 也一起被杀，永远执行不到
- `& disown` 把命令从当前会话剥离，让它独立存活

**验证：**

```bash
ls -la /mnt/CharlieII/
# 之前：drwx------ 26 1002 uucp  ...
# 之后：drwx------ 26 charlie charlie ...
```

### 方案 B：宽松权限（权宜之计）

**适用范围：** 无法改 UID（如共享 VM、CI 环境）。

```bash
# 宿主机侧
chmod 755 /path/to/shared/dir
chmod -R a+r /path/to/shared/dir
```

缺点：安全性降低，每次新建文件都要重新设置。

### 方案 C：Docker Init Container

**适用范围：** Docker Compose 多服务部署。

```yaml
hermes-init:
  image: busybox
  user: root
  restart: "no"
  command: >
    sh -c "
      chmod 755 /opt/data &&
      chown -R 10000:10000 /opt/data
    "
  volumes:
    - /host/path:/opt/data
```

应用服务 `depends_on` init 容器，确保权限就绪。

### 方案 D：Docker `user:` 指令

```yaml
services:
  app:
    image: myapp
    user: "1000:1000"
    volumes:
      - /host/data:/data
```

适合知道确切宿主机 UID 的场景。失去跨环境可移植性。

### 方案 E：User Namespace Remapping

在 `/etc/docker/daemon.json` 中启用：

```json
{ "userns-remap": "default" }
```

Docker 自动映射容器内 UID。最安全，但性能开销 ~5%，部分 volume 驱动不兼容。

## 4. 速查：chmod / chown / id 常用命令

| 命令 | 效果 |
|------|------|
| `id` | 查看当前 UID/GID |
| `ls -la /path` | 查看文件/目录权限和 owner |
| `whoami` | 当前用户名 |
| `chmod 755 /path` | 目录：owner 全权，其他人可读可进入 |
| `chmod 644 /path/file` | 文件：owner 读写，其他人只读 |
| `chmod 600 /path/secret` | 只有 owner 能读写 |
| `chown -R 1000:1000 /path` | 递归改 UID:GID |
| `sudo chown -R 1002:1002 /home/charlie` | UID 修改后修复 home 目录所有权 |

## 5. 排错清单

1. `id` — 查看当前 UID/GID
2. `ls -la /shared/path` — 文件的实际 owner UID 是什么？
3. 宿主机 vs VM 的 UID 一致吗？（`id` 两侧对比）
4. 如果是 `0000` 权限（全部不可读） → `chmod 644` 修复
5. Docker: `docker exec <container> id` — 确认容器内 UID
6. Docker: `docker-compose.yml` 里检查 `user:` 字段
7. NFS: 如果 owner 显示为数字 → UID 在本地不存在

## 关联页面

- [[hermes-docker-compose-deployment]] — 完整 Hermes docker-compose 部署方案
- [[hermes-architecture]] — Hermes Agent 架构全景
