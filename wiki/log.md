# Wiki Log

> Chronological record of all wiki actions. Append-only.
> Actions: ingest, update, query, lint, create, archive, delete

## [2026-05-05] create | DeepSeek → Claude Code 直连配置
- **Created:** concepts/deepseek-claude-code.md — DeepSeek Anthropic API 兼容层配置，V4-Pro/V4-Flash 模型策略，验证方法
- **Updated:** index.md (pages 10→11)

## [2026-05-05] merge | 双 wiki 合并：/mnt/CharlieII/wiki → /mnt/CharlieII/claude/wiki
- **Source (retired):** `/mnt/CharlieII/wiki`（11 页 + raw/）
- **Target (canonical):** `/mnt/CharlieII/claude/wiki`（保留 2 个独有页面 ncs-app-versioning, memfault-version-requirements）
- **方法:** `rsync -av` 覆盖骨架，独有文件手动保留
- **更新:** index.md 合并为 13 页，`.env` 更新指向
- **清理:** 源目录 `/mnt/CharlieII/wiki/` 暂未删除

## [2026-04-30] create | WiFi Knowledge Wiki initialized
- Domain: Wi-Fi / Wireless Networking
- Structure created with SCHEMA.md, index.md, log.md
- Wiki location: /workspace/wiki/

## [2026-04-30] create | Core WiFi wiki pages
- Created: concepts/wifi-standards-evolution.md — Full standards history Wi-Fi 1-8
- Created: concepts/wifi-6-6e-7.md — Wi-Fi 6/6E/7 technical deep dive (OFDMA, MLO, 4096-QAM, 320 MHz)
- Created: concepts/wifi-security.md — WPA2, WPA3, SAE, OWE, PMF, KRACK, WPS
- Created: entities/wifi-hardware-ecosystem.md — Chipsets, routers, mesh, antennas, deployment
- Created: comparisons/wpa2-vs-wpa3.md — Side-by-side comparison
- Updated: index.md — Added all 5 pages
- Sources: Wi-Fi Alliance, IEEE 802.11 standards, Qualcomm, Broadcom, MediaTek product docs

## [2026-05-01] merge | Wireless + Homelab wikis merged
- **Action:** Merged `/opt/data/home/wiki/` → `/opt/data/workspace/wiki/`
- **Domain expanded:** Wi-Fi + Homelab, networking, self-hosted infrastructure
- **New page:** concepts/wireguard-comprehensive-guide.md (moved from home wiki)
- **Duplicate handled:** concepts/wireguard-openwrt-china-tunnel.md — kept workspace version (includes multi-device expansion section)
- **SCHEMA.md updated:** Combined tag taxonomy (Standard, Protocol, Hardware, Security, Deployment, Networking, Homelab, Technology, Meta)
- **index.md updated:** Total pages: 7
- **Originating session:** WireGuard research (protocol design, networking topologies, embedded Zephyr/nRF/IoT)
- Conflicts: none

## [2026-05-01] delete | WiFi test content removed
- **Action:** User confirmed WiFi research was test data — deleted all 5 WiFi pages
- **Deleted:** concepts/wifi-standards-evolution.md, concepts/wifi-6-6e-7.md, concepts/wifi-security.md, entities/wifi-hardware-ecosystem.md, comparisons/wpa2-vs-wpa3.md
- **Cleaned:** index.md (pages 7→2, removed Entities/Comparisons/Queries sections), SCHEMA.md (domain narrowed to Homelab/networking/WireGuard/OpenWrt)
- **Removed empty dirs:** comparisons/, entities/
- **Remaining pages:** wireguard-comprehensive-guide, wireguard-openwrt-china-tunnel

## [2026-05-01] create | Hermes architecture wiki page
- **Created:** concepts/hermes-architecture.md — Full codebase architecture analysis (v0.11.0)
- **Content:** Agent loop, tool system, gateway, skills, memory, context compression, cron, transports, deployment modes, key statistics, design principles
- **Reading time:** ~10 minutes
- **Sources:** Source code analysis of /opt/hermes/ (~625K lines, 1303 source files)
- **Cron job:** Scheduled to monitor GitHub releases and auto-update this page

## [2026-05-02] ingest + create | AI SVG & PPTX generation
- **Prompt:** "请生成一篇wiki，以这个为例介绍svg和pptx的生成管理和过程"
- **Case study:** 有轨电车运行管理系统 PPT（11 页, 3 张 SVG, 310 KB）
- **RAW saved:** 
  - raw/articles/ppt-gen-tram-ops-script.md — Complete 630-line PptxGenJS script
  - raw/articles/ai-svg-pptx-generation-process.md — Full generation process narrative
- **Created:** concepts/svg-pptx-agent-generation.md — SVG（纯XML手写）+ PPTX（PptxGenJS JS API）生成工作流指南
  - Covers: SVG element mapping, color palette management, dark/light slide strategy, PptxGenJS pitfalls, full workflow diagram, SVG vs PPTX decision matrix
- **Updated:** index.md (pages 3→4), concepts/hermes-architecture.md (added cross-reference)
- **Cross-references:** svg-pptx-agent-generation ⇄ hermes-architecture

## [2026-05-03] create + cleanup | NAS Ubuntu VM wiki page + Hermes dir cleanup
- **Rewrote as NAS NFS architecture page:** entities/hermes-setup-on-linux-with-nas-nfs-backup.md — NAS is single source of truth, new machines recover by mounting NFS
  - Covers: NFS mount details (192.168.75.10:/volume1/CharlieII, NFSv3, 3.6TB), what lives on NAS (hermes/, webui/, wiki/, backup/), zero-reconfig recovery, 5 pitfalls
- Also fixed: Telegram gateway (systemd 203/EXEC, stale paths → corrected), WebUI workspace → /mnt/CharlieII
- **Updated:** index.md (pages 4→5, added Entities section), concepts/hermes-architecture.md (added cross-ref to nas-ubuntu24-vm)
- **Cleanup /mnt/CharlieII/hermes/:** Removed Docker-era exploration leftovers
  - Deleted: webui/ (old Docker WebUI state), src/ (empty), sandboxes/singularity/, bin/tirith (12MB), images/ (empty), tram_ops_research.md, response_store.db*, models_dev_cache.json
- **Hermes WebUI setup:** Installed nesquena/hermes-webui at /mnt/CharlieII/hermes-webui/, state at webui-state/, bound 0.0.0.0:8787
  - Fixed tab completion: eval "$(hermes completion bash)" in ~/.bashrc
  - CLI session bridge: requires show_cli_sessions toggle in WebUI Settings
- **Auto-start:** systemd services for gateway + WebUI with linger → survive reboots
  - WebUI: manual systemd unit created (~/.config/systemd/user/hermes-webui.service)
- **Wiki relocated:** /mnt/CharlieII/hermes/workspace/wiki/ → /mnt/CharlieII/wiki/
  - WIKI_PATH updated in .env to /mnt/CharlieII/wiki

## [2026-05-03] create | DNS-over-HTTPS (DoH) wiki page
- **Created:** concepts/dns-over-https-doh.md
- **Content:** How DoH encrypts DNS, office threat model (what IT can/can't see), DoH vs DoT comparison, browser + systemd setup via cloudflared, limitations (SNI/ECH, managed devices, VPN as stronger alternative)
- **Updated:** index.md (pages 5→6)

## [2026-05-03] ingest | Docker permissions + Hermes docker-compose
- **Raw sources saved:**
  - raw/articles/linux-docker-permissions-source.md ← /mnt/CharlieII/Permissions.txt
  - raw/articles/hermes-docker-compose-source.md ← /mnt/CharlieII/hermes_backup/docker-compose.yaml
- **Created:** concepts/linux-vm-docker-permission.md — Linux 权限模型、UID 对齐（含实战：Synology NAS uid=1002 + Ubuntu VM uid=1000 对齐全过程）、Docker 容器权限（init container / user 指令 / userns-remap）、chmod/chown 速查
- **Created:** concepts/hermes-docker-compose-deployment.md — 三服务编排（init→agent+webui）、named volume 共享、NAS 路径映射、部署步骤与排错
- **Updated:** index.md (pages 6→8)
- **Note:** Claude chat source (ca49b286...) was private/unreachable; wiki pages synthesized from the two referenced files alone.

## [2026-05-03] create | ttyd Web 终端设置
- **Created:** concepts/ttyd-web-terminal-setup.md — ttyd 安装与使用、`-W` 可写模式、systemd 开机自启、常见问题（oh-my-bash 时钟前缀 `THEME_SHOW_CLOCK=false`、非登录 shell PATH 修复、hermes completion 条件加载）、与 noVNC/SSH 对比
- **Updated:** index.md (pages 8→9)
- **Context:** 绿联 UGOS Pro noVNC 不支持剪贴板通道（SPICE/QEMU Agent 安装后仍不可用），ttyd 作为浏览器端原生终端替代方案
- **Learned:** ttyd 非 login shell，只读 `.bashrc` 不读 `.profile`；`~/.local/bin` 需在 `.bashrc` 里显式加到 PATH；oh-my-bash font 主题时钟需在加载前用 `export THEME_SHOW_CLOCK=false` 关闭

## [2026-05-05] create + update | Hermes file map wiki page + state dir fix
- **Created:** concepts/hermes-file-map-customization.md — Physical file layout: where repos live, which files to customize (config.yaml, .env, SOUL.md, webui .env, systemd units) vs auto-managed
- **Updated:** entities/hermes-setup-on-linux-with-nas-nfs-backup.md — Fixed stale `webui-state/` → `webui/` references (3 locations: table, .env example, text)
- **Updated:** index.md (pages 9→10)

## [2026-05-07] create | concepts/memfault-version-requirements.md
- Sourced from: Memfault docs (https://docs.memfault.com/docs/platform/software-version-hardware-version)
- Covers: allowed characters, natsort ordering, v-prefix caveat, OTA ordering across NCS bumps

## [2026-05-07] create | concepts/embedded-system-general-debugging.md
- Sourced from: sQSPI/MSPI driver debugging session (NCS v3.3.0, nRF54LM20DK + nRF7002 EB-II)
- Covers: baseline testing, variable isolation, multi-device comparison, barrier patterns, loop testing, nordicsemi_uart_monitor.py best practice

## [2026-05-07] create | concepts/mcp-nrflow-tools.md
- Sourced from: mcp_nrflow_nordicsemi_workflow_ncs resource content
- Covers: tool inventory, resource descriptions, comparison vs manual scripting, multi-device best practices

## [2026-05-07] update | index.md + concepts/*.md + SCHEMA.md
- Fixed [[wikilinks]] to GitHub-compatible markdown links in all concept pages and index (15 total pages)
- Fixed VCOM0/VCOM1 mapping: UART30 = VCOM0 on nRF54LM20DK (not VCOM1)
- Updated embedded-system-general-debugging Lesson 10: prefer nordicsemi_uart_monitor.py over manual Python serial
- Updated SCHEMA.md: document markdown link convention instead of wikilinks
- Added wiki/raw/ to .gitignore (51MB hardware PDFs/Altium — not for git)
