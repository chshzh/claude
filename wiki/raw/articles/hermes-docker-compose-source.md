---
source_url: file:///mnt/CharlieII/hermes_backup/docker-compose.yaml
ingested: 2026-05-03
sha256: a50e779ec45db6f38b73bb7b83745b0053c52d69fca5285b03d964cc36c68f2b
---

services:
  hermes-init:
    image: busybox
    container_name: hermes-init
    restart: "no"
    user: root
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

  hermes-agent:
    image: nousresearch/hermes-agent:latest
    container_name: hermes-agent
    restart: unless-stopped
    depends_on:
      hermes-init:
        condition: service_completed_successfully
    command: sh -c "hermes gateway run"
    ports:
      - "8642:8642"
    environment:
      - HOME=/opt/data
      - PATH=/opt/hermes/.venv/bin:/usr/local/bin:/usr/bin:/bin
    volumes:
      - /volume1/CharlieII/hermes/data:/opt/data
      - hermes-agent-src:/opt/hermes
      - /volume1/CharlieII:/home

  hermes-webui:
    image: ghcr.io/nesquena/hermes-webui:latest
    container_name: hermes-webui
    restart: unless-stopped
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

volumes:
  hermes-agent-src:
