---
title: AI Agent 视觉内容生成 — SVG 与 PPTX 工作流
created: 2026-05-02
updated: 2026-05-02
type: concept
tags: [meta, guide, reference, hermes, agent-workflow]
sources: [raw/articles/ai-svg-pptx-generation-process.md, raw/articles/ppt-gen-tram-ops-script.md]
confidence: high
---

# AI Agent 视觉内容生成 — SVG 与 PPTX 工作流

> **案例：** 有轨电车运行管理系统 PPT（11 页，3 张 SVG 图，310 KB）
> **总耗时：** ~3 分钟 | **模型：** deepseek-v4-pro | **会话：** 2026-05-02

## 概述

Hermes Agent 可以从零生成专业级视觉内容，无需任何图形化工具。核心思路：

- **SVG：** 直接写 XML 文本 — 矩形、圆形、文字、线条逐层叠加
- **PPTX：** 通过 PptxGenJS 库用 JavaScript 代码生成

两种格式都以**纯文本代码**为源，让 LLM 可以直接"画"出任何东西。

---

## 1. SVG 生成：纯 XML 手写

### 为什么 SVG？

SVG (Scalable Vector Graphics) 本质上是 XML 文本文件。这意味着：

1. **无需工具** — LLM 直接生成文本即可，不需要 launches Photoshop/Figma/Illustrator
2. **矢量无损** — 任意放大不模糊，嵌入 PPT 后依然清晰
3. **纯文本可调试** — 有问题直接改 XML，不需要重画
4. **中文原生支持** — `<text>` 标签直接写中文

### 生成策略

```
defs → gradients + filters + shadows
  ↓
background → dark fill rect
  ↓
boxes → rect with rx/ry for rounded corners
  ↓
lines → stroke-dasharray for connections
  ↓
text → Chinese + English labels
  ↓
annotations → small italics, muted colors
```

### 核心 SVG 元素对照

| 元素 | 用途 | 示例 |
|------|------|------|
| `<rect>` | 矩形框、卡片、背景 | `fill="#1e293b" stroke="#3b82f6" rx="10"` |
| `<circle>` | 圆点、状态灯 | `fill="#ef4444" filter="url(#glow)"` |
| `<line>` | 连接线、分隔线 | `stroke-dasharray="6,3"` |
| `<text>` | 所有文字 | `font-family="PingFang SC, Microsoft YaHei"` |
| `<linearGradient>` | 渐变色 | `stop-color="#3b82f6"` |
| `<filter>` | 阴影、发光 | `feDropShadow` / `feGaussianBlur` |

### 案例：电车架构图 (12,573 bytes)

```xml
<!-- OCC 中心框 -->
<rect x="330" y="90" width="440" height="130" rx="12" fill="#1e293b"
      stroke="#3b82f6" stroke-width="2" filter="url(#shadow)"/>

<!-- 子系统卡片 -->
<rect x="348" y="150" width="95" height="55" rx="6"
      fill="url(#blueGrad)" opacity="0.85"/>
<text x="395" y="175" fill="#fff" font-size="10">ATS</text>
<text x="395" y="192" fill="#bfdbfe" font-size="8">列车自动监督</text>

<!-- 通信骨干网 -->
<line x1="80" y1="260" x2="1020" y2="260" stroke="#3b82f6"
      stroke-width="2" stroke-dasharray="6,3"/>
```

关键技巧：
- 所有坐标手动计算（760px 宽画布，各元素间距一致）
- 中英双语标注（标题中文，副标题英文 italic）
- 深色主题（`#0f1729` 底色）适合投影演示

---

## 2. PPTX 生成：PptxGenJS

### 为什么 PptxGenJS？

- **npm 包，一行安装** — `npm install pptxgenjs`
- **JavaScript API** — LLM 可以直接写代码，不依赖 GUI
- **完整功能** — 表格、图表、图片、形状、阴影、颜色
- **SVG 嵌入** — 直接 `addImage({ path: "diagram.svg" })` 嵌入矢量图

### 脚本结构

```javascript
const pptxgen = require("pptxgenjs");
let pres = new pptxgen();
pres.layout = "LAYOUT_16x9";   // 10" × 5.625"

// ── Slide 1: Title (dark) ──
{
  let s = pres.addSlide();
  s.background = { color: C.darkBg };
  s.addText("标题", { x: 0.8, y: 1.2, fontSize: 40, color: C.textLight });
}

// ── Slide 2: Content (light) ──
{
  let s = pres.addSlide();
  s.background = { color: C.lightBg };
  s.addText("内容", { ... });
  s.addShape(pres.shapes.RECTANGLE, { ... });
  s.addImage({ path: "diagram.svg", ... });
}

pres.writeFile({ fileName: "output.pptx" });
```

### 配色管理

集中定义颜色常量，全局复用：

```javascript
const C = {
  darkBg:   "0F1729",   // 深色背景
  lightBg:  "F8FAFC",   // 浅色背景
  blue:     "3B82F6",   // 主色调
  green:    "10B981",   // 成功/积极
  amber:    "F59E0B",   // 警告/强调
  purple:   "8B5CF6",   // 科技/创新
  red:      "EF4444",   // 供电/危险
  cyan:     "06B6D4",   // 监测/信息
};
```

### 深色/浅色交替策略

```
Slide 1 (Title)    → dark
Slide 2 (Agenda)   → light
Slide 3 (SVG图)    → light
Slide 4-9 (内容)   → light
Slide 10 (Trends)  → dark   ← 视觉节奏变化
Slide 11 (Q&A)     → dark   ← 收尾呼应
```

这种 "三明治" 结构让演示有呼吸感，不会全程白底单调，也不会全程黑底压抑。

### PptxGenJS 关键坑

> ⚠️ 以下是实测踩过的坑：

| 坑 | 正确做法 | 错误做法 |
|----|----------|----------|
| 颜色 `#` 前缀 | `color: "FF0000"` | `color: "#FF0000"` → 文件损坏 |
| 阴影透明度 | `opacity: 0.15` | 8位颜色 `"00000020"` → 文件损坏 |
| 多行文本 | `breakLine: true` | 文字粘连 |
| 阴影对象复用 | 每次 `() => ({...})` | 共享对象 → 第二个形状损坏 |
| 圆角卡片 + 边条 | 用 `RECTANGLE` | `ROUNDED_RECTANGLE` → 边条对不齐 |

---

## 3. 完整工作流：从提问到 PPT 交付

```
用户提问 ("解释电车运行管理")
        │
        ▼
  ┌─────────────┐
  │ delegate_task │ → 子 Agent 搜索 Wikipedia/IEEE/厂商文档
  │ 研究 (42 API) │ → 返回 222 行结构化摘要
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ write_file   │ → 3 张 SVG (纯 XML, 共 ~32KB)
  │ × 3          │ → 架构图 / TSP图 / 供电图
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ write_file   │ → 630 行 JS 脚本
  │ (PPT 脚本)   │ → 定义 14 个颜色常量 + 11 个 slide 块
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ terminal     │ → node create_ppt.js
  │ (运行脚本)   │ → 输出 310KB .pptx
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ QA 校验      │ → unzip + grep 检查 11 slides
  │              │ → 确认无 placeholder / 死链接
  └──────┬──────┘
         │
         ▼
    交付用户  🎉
```

### 各阶段耗时

| 阶段 | 耗时 | 工具调用 |
|------|------|----------|
| 研究 | ~9 min | delegate_task (42 API) |
| SVG 绘制 | ~30 sec | write_file × 3 |
| PPT 脚本 | ~1 min | write_file × 1 |
| 运行 + QA | ~20 sec | terminal, unzip+grep |
| **总计** | **~11 min** | 6 轮工具调用 |

> 注：研究阶段占比最高（~80%），如果跳过研究直接用已有知识，SVG+PPTX 纯生成只需 ~2 分钟。

---

## 4. SVG vs PPTX：何时用哪个？

| 维度 | SVG | PPTX |
|------|-----|------|
| **适合场景** | 单张图、架构图、流程图 | 演示文稿、报告、培训材料 |
| **生成方式** | 直接写 XML | JavaScript 脚本 |
| **依赖** | 零依赖 | 需要 npm + PptxGenJS |
| **可编辑性** | 文本编辑器 / Inkscape | PowerPoint / Keynote / WPS |
| **嵌入关系** | SVG 可嵌入 PPTX | PPTX 可导出 PNG/PDF |
| **复杂度上限** | 高（但手工坐标计算繁琐） | 很高（有表格/图表 API） |
| **LLM 友好度** | ★★★★★（纯文本） | ★★★★☆（JS 代码） |

**经验法则：**
- 需要**嵌入 PPT** 的图 → 先画 SVG，PPT 里用 `addImage` 引用
- 需要**多页结构、表格、图表** → 直接用 PptxGenJS
- 需要**交互式 Web 展示** → SVG（可加 CSS 动画）/ HTML

---

## 5. 最佳实践

### 色彩系统
- 定义 10-15 个命名颜色常量，不在各处写裸色值
- 深色背景用 `#0F1729`（slate-900），浅色用 `#F8FAFC`（slate-50）
- 每种功能色（蓝/绿/琥珀/紫/红/青）配一个对应浅色版 (`Lt`) 做背景
- 文字在深色背景上永远用 `#E2E8F0`（不纯白，更柔和）

### 字体
- 标题用 `Arial Black`（有力），正文用 `Calibri`（清晰）
- 中文在 SVG 里指定 `PingFang SC, Microsoft YaHei` 后备
- PPT 标题 28-40pt，正文 10-14pt，标注 8-9pt

### 间距
- SVG 画布通常 1000-1100px 宽，以 50px 为基本网格单位
- PPT 用英寸（10" × 5.625"），内容区留 0.6" 边距
- 卡片间间距 0.3-0.5"

### 验证
- PPTX 本质是 ZIP → `unzip -l` 查看文件结构
- `grep '<a:t>' slide*.xml` 提取所有文字，检查完整性
- 搜索 `xxxx\|lorem\|ipsum\|placeholder` 确认无残留

---

## 6. 案例代码索引

完整可复现的源码：

- **SVG 架构图：** `/opt/data/workspace/tram_ops_overview.svg`
- **SVG TSP 图：** `/opt/data/workspace/tram_tsp_diagram.svg`
- **SVG 供电图：** `/opt/data/workspace/tram_power_system.svg`
- **PPT 生成脚本：** `raw/articles/ppt-gen-tram-ops-script.md`（含完整 630 行 JavaScript）
- **生成过程记录：** `raw/articles/ai-svg-pptx-generation-process.md`
- **PPT 产物：** `/opt/data/workspace/电车运行管理系统.pptx`（310 KB, 11 页）

---

## 关联页面
- [[hermes-architecture]] — Hermes Agent 完整架构（生成这些内容的基础设施）
- 本页用到的 SVG 技术参考：[MDN SVG Tutorial](https://developer.mozilla.org/en-US/docs/Web/SVG/Tutorial)
- PptxGenJS 官方文档：[pptxgenjs.com](https://pptgenjs.com/docs/)

^[raw/articles/ai-svg-pptx-generation-process.md]
^[raw/articles/ppt-gen-tram-ops-script.md]
