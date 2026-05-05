# 热门语音模型研究报告 (May 2026)

> 研究时间: 2026-05-03
> 目的: 比较当前热门语音模型，评估接入 Hermes Agent 的可行性

---

## 一、模型总览

### 1. VibeVoice (Microsoft) ⭐ 46,365 ⭐

| 维度 | 详情 |
|------|------|
| **类型** | TTS + ASR 模型家族 |
| **开源协议** | MIT |
| **GitHub** | https://github.com/microsoft/VibeVoice |
| **HuggingFace** | https://huggingface.co/collections/microsoft/vibevoice |
| **创建时间** | 2025-08-25 |
| **论文** | ICLR 2026 Oral (TTS), arXiv:2601.18184 (ASR) |

#### 模型家族

| 模型 | 参数量 | 用途 | 下载量 | 模型大小 |
|------|--------|------|--------|----------|
| VibeVoice-TTS-1.5B | 1.5B | 长文本多说话人 TTS (最长90分钟，4说话人) | 261K | ~3GB |
| VibeVoice-Realtime-0.5B | 0.5B | 实时流式 TTS (单说话人) | 1.15M | 1.02GB |
| VibeVoice-ASR-7B | 7B | 语音识别 (60分钟单次处理，50+语言，说话人分离) | 662K | ~14GB |
| VibeVoice-ASR-HF | 7B | Transformers 集成版 ASR | 305K | ~14GB |

#### 核心创新
- **7.5Hz 超低帧率连续语音分词器** (Acoustic + Semantic)
- **Next-token diffusion 框架**: LLM backbone + Diffusion head
- 实时变体采用 interleaved windowed 设计：增量编码文本 + 并行扩散生成

#### 硬件要求 (估算)
- Realtime-0.5B: 最低 4GB VRAM (0.5B 参数，1GB 权重)
- TTS-1.5B: 8-12GB VRAM
- ASR-7B: 16-24GB VRAM (可用 vLLM 加速)

#### 限制
- TTS 代码因滥用问题已于 2025-09 移除 (模型权重仍在)
- 实时版仅支持英文 (实验性支持 9 种其他语言)
- 仅限研究用途

---

### 2. Sesame CSM (Conversational Speech Model) ⭐ 14,605 ⭐

| 维度 | 详情 |
|------|------|
| **类型** | 对话语音生成模型 (Text+Audio → Speech) |
| **开源协议** | Apache 2.0 |
| **GitHub** | https://github.com/SesameAILabs/csm |
| **HuggingFace** | https://huggingface.co/sesame/csm-1b |
| **创建时间** | 2025-02-26 |

#### 技术架构
- **Llama 3.2-1B** 作为 backbone
- **Mimi** (Kyutai) 音频编解码器
- 输入: 文本 + 说话人上下文音频 → 输出: RVQ 音频码
- HuggingFace Transformers 4.52.1+ 原生支持

#### 硬件要求
- CUDA GPU 必须
- 1B 参数 (Llama + 轻量 decoder)，预估 4-6GB VRAM
- 测试于 CUDA 12.4/12.6

#### 特点
- 上下文感知：提供前几轮对话音频作为 context，生成极其自然的对话语音
- 可以 fine-tune (社区已有中文微调项目)
- 实时速度：推理速度较快 (~1x 实时)
- Sesame 官方的交互 demo 效果极好 ("跨越恐怖谷")

---

### 3. OmniVoice (k2-fsa) ⭐ 4,618 ⭐

| 维度 | 详情 |
|------|------|
| **类型** | 多语言零样本 TTS + 声音克隆 |
| **开源协议** | Apache 2.0 |
| **GitHub** | https://github.com/k2-fsa/OmniVoice |
| **HuggingFace** | https://huggingface.co/k2-fsa/OmniVoice |
| **论文** | arXiv:2604.00688 |
| **创建时间** | 2026-03-31 (非常新!) |
| **下载量** | 2M+ |

#### 核心特性
- **600+ 语言**支持
- **语音克隆**: 零样本，1句参考音频即可
- **语音设计**: 控制性别、年龄、音调、方言、耳语等属性
- **极快推理**: RTF 低至 0.025 (40倍实时速度)
- **细粒度控制**: 非语言符号 ([laughter])、发音纠正 (拼音/音素)

#### 硬件要求
- NVIDIA GPU (CUDA) 或 Apple Silicon (MPS)
- 模型通过 safetensors 分发，体积适中
- 预估 VRAM: 4-8GB (实际以模型大小为准)

#### 生态
- `pip install omnivoice` 一键安装
- 内置 Web UI: `omnivoice-demo --ip 0.0.0.0 --port 8001`
- 社区 OpenAI 兼容服务器: https://github.com/maemreyo/omnivoice-server
- ComfyUI 集成

---

### 4. Orpheus TTS (CanopyAI) ⭐ 6,123 ⭐

| 维度 | 详情 |
|------|------|
| **类型** | LLM-based TTS |
| **开源协议** | Apache 2.0 |
| **GitHub** | https://github.com/canopyai/Orpheus-TTS |
| **创建时间** | 2025-03-08 |

#### 技术架构
- 基于 **Llama-3B** backbone 微调
- 将语音 token 化并通过 LLM 生成
- 使用 unsloth 进行微调

#### 硬件要求
- GPU: 6-8GB VRAM (3B 模型)
- 无 GPU: 支持 Llama.cpp 推理 (CPU 运行)
- 未来计划: 1B, 400M, 150M 小模型

#### 生态
- LM Studio 本地运行
- **OpenAI 兼容 FastAPI**: https://github.com/Lex-au/Orpheus-FastAPI
- 社区活跃

---

### 5. Moshi (Kyutai) — 语音对话

| 维度 | 详情 |
|------|------|
| **类型** | 端到端语音对话 (全双工) |
| **开源协议** | Apache 2.0 |
| **延迟** | ~200ms 真实延迟 |
| **架构** | Mimi codec + 7B Helium LLM |
| **硬件** | 单 GPU (量化版可在 MacBook M2 运行) |

#### 特点
- 真正的 speech-to-speech，无需 ASR→LLM→TTS 流水线
- 支持打断、情感表达、唱歌、笑声
- 开源但使用复杂（需了解其音频编解码架构）

---

### 6. Ultravox (Fixie AI) ⭐ 4,412 ⭐

| 维度 | 详情 |
|------|------|
| **类型** | 多模态实时语音 LLM |
| **开源协议** | MIT |
| **GitHub** | https://github.com/fixie-ai/ultravox |
| **默认模型** | Llama 3.3 70B + 8B 变体 |

#### 特点
- 音频输入 → 流式文本输出
- 在音频编码器和 LLM 之间训练投射层 (adapter)
- 7-8 小时训练 (8xH100)
- 提供托管 API (ultravox.ai)
- 适合构建实时语音 agent

---

### 7. GLM-4-Voice (智谱 AI / THUDM)

| 维度 | 详情 |
|------|------|
| **类型** | 端到端语音对话 (中英双语) |
| **开源协议** | Apache 2.0 |
| **参数** | 9B |
| **硬件** | A100 40GB (量化后可 24GB) |

---

### 8. 其他值得关注的模型

| 模型 | 类型 | 亮点 | 开源 |
|------|------|------|------|
| **GPT-4o Voice** (OpenAI) | 语音对话 | 最佳质量，全双工 | ❌ |
| **MiniMax Audio** | TTS API | 极高自然度，流式 | ❌ |
| **CosyVoice** (阿里) | 流式 TTS | 中文最佳，声音克隆 | Apache 2.0 |
| **F5-TTS** | Flow-matching TTS | MIT，社区流式分支 | MIT |
| **ChatTTS** | 对话 TTS | 自然填充词/笑声 | Apache 2.0 |
| **FishSpeech** (FishAudio) | 流式 TTS | 声音克隆，低延迟 | MIT |
| **SparkTTS** (讯飞) | 流式 TTS | 声音克隆，可控 | Apache 2.0 |

---

## 二、对比总表

| 模型 | 类型 | Stars | 协议 | 最低VRAM | 实时 | 中文 | 声音克隆 | OpenAI API |
|------|------|-------|------|----------|------|------|----------|------------|
| **VibeVoice 0.5B** | 流式TTS | 46K | MIT | 4GB | ✅ | ❌ | ❌ | ❌ |
| **VibeVoice 1.5B** | TTS | 46K | MIT | 8-12GB | ❌ | ❌ | ❌ | ❌ |
| **Sesame CSM** | 对话语音 | 14.6K | Apache2 | 4-6GB | ⚠️~1x | ❌ | ✅(上下文) | ❌ |
| **OmniVoice** | TTS+克隆 | 4.6K | Apache2 | 4-8GB | ⚠️40xRTF | ✅ | ✅ | ✅(社区) |
| **Orpheus TTS** | TTS | 6.1K | Apache2 | 6-8GB | ❌ | ❌ | ❌ | ✅(社区) |
| **Moshi** | 语音对话 | - | Apache2 | 6-16GB | ✅200ms | ❌ | N/A | ❌ |
| **Ultravox** | 语音LLM | 4.4K | MIT | 16GB+(8B) | ✅ | ❌ | N/A | ✅(托管) |
| **CosyVoice** | TTS | - | Apache2 | 6GB | ✅流式 | ✅ | ✅ | ❌ |
| **GPT-4o Voice** | 语音对话 | - | 闭源 | N/A | ✅ | ✅ | N/A | ✅ |

---

## 三、接入 Hermes 的可行性分析

### Hermes 当前语音架构

Hermes 已有的 TTS 提供商：
- Edge TTS (免费，默认)
- ElevenLabs
- OpenAI TTS
- MiniMax TTS
- Mistral Voxtral
- NeuTTS (本地)

集成路径：
```
用户语音 → STT (faster-whisper等) → 文本 → LLM推理 → 文本 → TTS → 语音输出
```

### 各模型接入方案

#### 方案 A: VibeVoice-Realtime-0.5B → Hermes TTS Provider

**可行性**: ⭐⭐⭐ (中等)

- **路径**: 编写 HTTP wrapper，暴露 OpenAI-compatible `/v1/audio/speech` 端点
- **优点**: 模型小(0.5B)，VRAM 需求低(4GB)，MIT 协议，流式输出
- **难点**: 
  - 需要自己搭建 API 服务器 (目前没有现成的 OpenAI 兼容层)
  - 仅英文，中文不可用
  - TTS 代码被移除，需要依赖社区 fork (vibevoice-community/VibeVoice)
- **推荐度**: 如果你只需要英文 TTS 且愿意搭服务，可行

#### 方案 B: OmniVoice → Hermes TTS Provider ⭐ 推荐

**可行性**: ⭐⭐⭐⭐⭐ (最高)

- **路径**: 使用 `omnivoice-server` (OpenAI 兼容 HTTP 服务器)
- **优点**: 
  - 已有 OpenAI 兼容服务器 (maemreyo/omnivoice-server)
  - 600+ 语言，中文支持好
  - 声音克隆 + 声音设计
  - Apache 2.0，极快推理 (40x RTF)
  - `pip install omnivoice` 一键安装
- **实施步骤**:
  1. 部署 omnivoice-server 到本地 GPU 机器
  2. Hermes 配置自定义 TTS endpoint:
     ```yaml
     tts:
       provider: openai  # 或其他兼容提供者
       openai:
         base_url: http://localhost:8001/v1
         api_key: not-needed
     ```
  3. 或者通过 Hermes 的 custom provider 机制配置
- **推荐度**: 🏆 **最佳选择** — 综合开源协议、功能、易用性、中文支持

#### 方案 C: Sesame CSM → Hermes 对话语音

**可行性**: ⭐⭐ (较低)

- **路径**: 需要大幅改造 Hermes 的语音交互流程
- **原因**: CSM 是 Text+Audio→Speech 模型，不是纯 TTS。它需要对话上下文音频才能发挥最佳效果
- **可能方式**: 
  - 写自定义 Hermes 工具，将 LLM 输出 + 历史音频 → CSM → 播放
  - 但这意味着每次对话都要缓存音频上下文
- **推荐度**: 不推荐作为 Hermes TTS provider；更适合独立 demo

#### 方案 D: Orpheus TTS → Hermes TTS Provider

**可行性**: ⭐⭐⭐⭐ (较高)

- **路径**: Orpheus-FastAPI (OpenAI 兼容)
- **优点**: 已有 OpenAI 兼容 API，社区活跃，支持 CPU 推理
- **缺点**: 仅英文，不是实时流式，模型较大 (3B)
- **推荐度**: 仅适合英文场景

#### 方案 E: Moshi → 真正的语音对话

**可行性**: ⭐ (非常低，但理想)

- 如果 Hermes 能接入 Moshi，就能实现真正的语音对话 (而非 ASR→LLM→TTS 流水线)
- 但这需要 Hermes 从根本上改变语音处理架构
- 目前 Hermes 的架构是文本中心的，集成 Moshi 需要大量底层改造
- **更现实的路径**: Mikupad 或类似项目将 Moshi 作为独立服务，与 Hermes 通过 API 交互

### 最终推荐

| 场景 | 推荐模型 | 理由 |
|------|----------|------|
| **快速集成，中文好用** | OmniVoice | OpenAI 兼容服务器，600+语言，声音克隆 |
| **最小 VRAM，英文** | VibeVoice 0.5B | 仅 1GB 权重，MIT 协议 |
| **英语最佳 TTS** | Orpheus TTS | 成熟生态，OpenAI 兼容 |
| **未来：真正的语音对话** | Moshi | 全双工，200ms延迟，但需大改 Hermes |

---

## 四、实施路线图建议

### 第一阶段：OmniVoice 集成 (1-2 天)

```bash
# 1. 在 GPU 机器上安装
pip install omnivoice

# 2. 部署 OpenAI 兼容服务器
git clone https://github.com/maemreyo/omnivoice-server
cd omnivoice-server
python server.py --host 0.0.0.0 --port 8001

# 3. 在 Hermes 中配置
hermes config set tts.provider openai
hermes config set tts.openai.base_url "http://192.168.75.XX:8001/v1"
hermes config set tts.openai.api_key "not-needed"
```

### 第二阶段：体验优化

- 测试不同声音克隆效果
- 调整 latency 和 chunk size
- 添加中文声音预设

### 第三阶段（可选）：探索 Moshi 实时对话

- 将 Moshi 作为独立语音接口
- Hermes 作为"大脑"，Moshi 作为"嘴巴+耳朵"
- 需要自定义中间件将两者桥接

---

## 五、关键注意事项

1. **Hermes 的 TTS 目前走 OpenAI 兼容接口** — 任何能暴露 `/v1/audio/speech` 端点的服务理论上都可以接入
2. **STT 和 TTS 是分离的** — 语音→文本 走 STT 管道，文本→语音走 TTS 管道
3. **vLLM 加速** — VibeVoice-ASR 已支持 vLLM，OmniVoice 推理本身已很快
4. **NAS 环境约束** — 你的 UGREEN DX4600 NAS 没有 GPU，需要在有 GPU 的机器上运行模型服务器，Hermes 通过网络 API 调用
5. **网络延迟** — 局域网内 HTTP API 延迟 <10ms，加上模型推理 50-200ms，总延迟可接受
