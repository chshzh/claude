#!/bin/bash
# Weekly release update — pins agent and webui to latest release tags
# Deliver to Telegram via cron job every Friday 17:00
set -e

TIMESTAMP=$(date '+%Y-%m-%d %H:%M')
REPORT="📅 Release Update Report — $TIMESTAMP

"

# ── Hermes Agent ──
AGENT_DIR="/home/charlie/hermes-agent"
if [ -d "$AGENT_DIR" ]; then
  cd "$AGENT_DIR"
  BEFORE=$(git describe --tags --always 2>/dev/null || git rev-parse --short HEAD)
  git fetch --tags --quiet 2>/dev/null || true
  LATEST_TAG=$(git tag --sort=-v:refname | head -1)
  if [ -z "$LATEST_TAG" ]; then
    REPORT+="🤖 Agent: no tags found\n"
  elif git merge-base --is-ancestor "$LATEST_TAG" HEAD 2>/dev/null && [ "$BEFORE" = "$LATEST_TAG" ]; then
    REPORT+="🤖 Agent: already at latest $LATEST_TAG ✅\n"
  else
    if git checkout --quiet "$LATEST_TAG" 2>/dev/null; then
      AFTER=$(git describe --tags --always)
      REPORT+="🤖 Agent: $BEFORE → $AFTER ✅\n"
    else
      REPORT+="🤖 Agent: FAILED to checkout $LATEST_TAG ❌\n"
    fi
  fi
else
  REPORT+="🤖 Agent: repo not found ❌\n"
fi

# ── Hermes WebUI ──
WEBUI_DIR="/home/charlie/hermes-webui"
NEEDS_RESTART=false
if [ -d "$WEBUI_DIR" ]; then
  cd "$WEBUI_DIR"
  BEFORE=$(git describe --tags --always 2>/dev/null || git rev-parse --short HEAD)
  git fetch --tags --quiet 2>/dev/null || true
  LATEST_TAG=$(git tag --sort=-v:refname | head -1)
  if [ -z "$LATEST_TAG" ]; then
    REPORT+="🌐 WebUI: no tags found\n"
  elif git merge-base --is-ancestor "$LATEST_TAG" HEAD 2>/dev/null && [ "$BEFORE" = "$LATEST_TAG" ]; then
    REPORT+="🌐 WebUI: already at latest $LATEST_TAG ✅\n"
  else
    if git checkout --quiet "$LATEST_TAG" 2>/dev/null; then
      AFTER=$(git describe --tags --always)
      REPORT+="🌐 WebUI: $BEFORE → $AFTER ✅"
      NEEDS_RESTART=true
    else
      REPORT+="🌐 WebUI: FAILED to checkout $LATEST_TAG ❌\n"
    fi
  fi

  if [ "$NEEDS_RESTART" = true ]; then
    systemctl --user restart hermes-webui 2>/dev/null
    if [ $? -eq 0 ]; then
      REPORT+=" (service restarted)\n"
    else
      REPORT+=" (service restart FAILED ❌)\n"
    fi
  fi
else
  REPORT+="🌐 WebUI: repo not found ❌\n"
fi

echo -e "$REPORT"
