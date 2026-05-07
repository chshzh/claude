---
title: DeepSeek → Claude Code 直连配置
created: 2026-05-05
updated: 2026-05-07
type: concept
tags: [ai, llm, guide, reference]
sources: []
confidence: medium
---

# DeepSeek → Claude Code 直连配置

DeepSeek 提供了完整的 Anthropic API 兼容层，使 Claude Code 可以直接将 DeepSeek 模型作为后端使用，无需任何代理或中转服务。

## 原理

Claude Code 原生通过 Anthropic Messages API 与模型通信。DeepSeek 在 `https://api.deepseek.com/anthropic` 暴露了完全兼容的端点，Claude Code 只需修改 `ANTHROPIC_BASE_URL` 即可指向 DeepSeek，无需修改任何代码。

## 核心配置

```yaml
# Claude Code 配置文件 (如 ~/.claude/settings.json 或项目级 .claude/settings.local.json)
{
  "env": {
    "ANTHROPIC_BASE_URL": "https://api.deepseek.com/anthropic",
    "ANTHROPIC_AUTH_TOKEN": "${DEEPSEEK_API_KEY}",
    "API_TIMEOUT_MS": "3000000",
    "ANTHROPIC_MODEL": "deepseek-v4-pro",
    "ANTHROPIC_SMALL_FAST_MODEL": "deepseek-v4-flash",
    "ANTHROPIC_DEFAULT_SONNET_MODEL": "deepseek-v4-pro",
    "ANTHROPIC_DEFAULT_OPUS_MODEL": "deepseek-v4-pro",
    "ANTHROPIC_DEFAULT_HAIKU_MODEL": "deepseek-v4-flash",
    "CLAUDE_CODE_SUBAGENT_MODEL": "deepseek-v4-pro",
    "CLAUDE_CODE_EFFORT_LEVEL": "max"
  },
  "model": "deepseek-v4-pro"
}
```

### 参数说明

| 变量 | 值 | 作用 |
|------|-----|------|
| `ANTHROPIC_BASE_URL` | `https://api.deepseek.com/anthropic` | 将 Anthropic API 请求重定向到 DeepSeek |
| `ANTHROPIC_AUTH_TOKEN` | `$DEEPSEEK_API_KEY` | DeepSeek API Key（从环境变量读取） |
| `API_TIMEOUT_MS` | `3000000` | 超时时间（毫秒），处理长上下文推理时需放大 |
| `CLAUDE_CODE_EFFORT_LEVEL` | `max` | 让 DeepSeek 在复杂任务上投入更多推理计算 |

## 模型策略

Claude Code 中 `ANTHROPIC_MODEL` 设定默认模型，其余变量控制不同场景下的模型选择：

- **主模型** (`ANTHROPIC_MODEL`) → `deepseek-v4-pro` — 日常交互、复杂推理
- **快速模型** (`SMALL_FAST`) → `deepseek-v4-flash` — 简单查询、快速响应
- **Sonnet 对标** → `deepseek-v4-pro` — 重量级任务
- **Opus 对标** → `deepseek-v4-pro` — 最复杂任务
- **Haiku 对标** → `deepseek-v4-flash` — 轻量任务
- **子代理模型** (`SUBAGENT`) → `deepseek-v4-pro` — Claude Code 子任务

逻辑清晰：**重量级任务走 V4-Pro，轻量任务走 V4-Flash**。

## 验证

启动后，Claude Code 界面上直接显示当前模型名（如 `deepseek-v4-pro`）。可通过对话确认：

```
> 你是什么模型？
I'm DeepSeek V4-Pro.
```

## 注意事项

- DeepSeek API Key 通过环境变量 `DEEPSEEK_API_KEY` 传入，不要在配置文件中明文硬编码
- 超时时间建议设置 3 分钟以上（`3000000ms`），DeepSeek 深度推理模式下首次响应可能较慢
- `CLAUDE_CODE_EFFORT_LEVEL` 设为 `max` 会显著增加延迟，但推理质量更高
- DeepSeek Anthropic 兼容层**不支持流式 SSE 以外的高级 Anthropic 功能**（如 Tool Use Beta），Claude Code 核心对话流程完全正常
- 配置更改后重启 Claude Code 即可生效
