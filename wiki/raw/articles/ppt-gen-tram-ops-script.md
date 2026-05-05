---
source_url: "local: /tmp/create_tram_ppt.js (Hermes agent-generated script)"
ingested: 2026-05-02
sha256: e0b493ed12a95ae34857ccac770200cd8dd96bf392d13080c21c1d650f1070f3
---

# PPTX Generation Script — Tram Operations Management

This is the complete PptxGenJS script that generates the 11-slide presentation
"现代有轨电车运行管理系统" (Modern Tram Operations Management System).

The script was written by Hermes Agent (deepseek-v4-pro) in a single pass during
a Telegram conversation with Charlie Shao on 2026-05-02. The prompt was:
"帮我解释下电车tram的运行管理，如果有图片就更好了" followed by "好的" (when
offered to generate a PPT).

Key characteristics of the generated script:
- 11 slides: title, agenda, architecture (SVG), OCC, signaling/CBTC, TSP (SVG),
  power supply (SVG), depot+maintenance, vendor ecosystem, trends, thank-you
- ~630 lines of JavaScript
- Uses PptxGenJS library (installed via npm)
- Embeds 3 pre-generated SVG diagrams as images
- Dark/light slide alternating scheme
- Color palette: slate-900 background, blue/green/amber/purple/red/cyan accents
- Production time: ~2 minutes from prompt to final .pptx

## Script Content

```javascript
     1|const pptxgen = require("/tmp/node_modules/pptxgenjs");
     2|const fs = require("fs");
     3|
     4|// ── Colour Palette ──
     5|const C = {
     6|  darkBg:   "0F1729",
     7|  lightBg:  "F8FAFC",
     8|  blue:     "3B82F6",
     9|  blueLt:   "DBEAFE",
    10|  blueDk:   "1E40AF",
    11|  green:    "10B981",
    12|  greenLt:  "D1FAE5",
    13|  amber:    "F59E0B",
    14|  amberLt:  "FEF3C7",
    15|  purple:   "8B5CF6",
    16|  purpleLt: "EDE9FE",
    17|  red:      "EF4444",
    18|  redLt:    "FEE2E2",
    19|  cyan:     "06B6D4",
    20|  grey100:  "F1F5F9",
    21|  grey200:  "E2E8F0",
    22|  grey400:  "94A3B8",
    23|  grey600:  "475569",
    24|  grey800:  "1E293B",
    25|  white:    "FFFFFF",
    26|  textDark: "1E293B",
    27|  textLight:"E2E8F0",
    28|};
    29|
    30|let pres = new pptxgen();
    31|pres.layout = "LAYOUT_16x9"; // 10" x 5.625"
    32|pres.author = "Charlie Shao";
    33|pres.title = "现代有轨电车运行管理系统";
    34|
    35|// ═══════════════════════════════════════════
    36|// SLIDE 1 — TITLE (dark)
    37|// ═══════════════════════════════════════════
    38|{
    39|  let s = pres.addSlide();
    40|  s.background = { color: C.darkBg };
    41|
    42|  // Accent bar top
    43|  s.addShape(pres.shapes.RECTANGLE, { x: 0, y: 0, w: 10, h: 0.06, fill: { color: C.blue } });
    44|
    45|  // Main title
    46|  s.addText("🚃 现代有轨电车运行管理系统", {
    47|    x: 0.8, y: 1.2, w: 8.4, h: 1.2,
    48|    fontSize: 40, fontFace: "Arial Black", color: C.textLight,
    49|    bold: true, align: "left", margin: 0,
    50|  });
    51|
    52|  // Subtitle
    53|  s.addText("Tram Operations Management System", {
    54|    x: 0.8, y: 2.4, w: 8.4, h: 0.6,
    55|    fontSize: 18, fontFace: "Calibri", color: C.grey400,
    56|    italic: true, align: "left", margin: 0,
    57|  });
    58|
    59|  // Bottom info bar
    60|  s.addShape(pres.shapes.RECTANGLE, { x: 0, y: 4.8, w: 10, h: 0.825, fill: { color: C.grey800 } });
    61|  s.addText("OCC · ATS · CBTC · TSP · SCADA · 牵引供电 · 预测维护   |   May 2026", {
    62|    x: 0.8, y: 4.8, w: 8.4, h: 0.825,
    63|    fontSize: 11, fontFace: "Calibri", color: C.grey400, align: "left", valign: "middle", margin: 0,
    64|  });
    65|}
    66|
    67|// ═══════════════════════════════════════════
    68|// SLIDE 2 — Agenda (light)
    69|// ═══════════════════════════════════════════
    70|{
    71|  let s = pres.addSlide();
    72|  s.background = { color: C.lightBg };
    73|
    74|  s.addText("目录 Agenda", {
    75|    x: 0.6, y: 0.3, w: 8.8, h: 0.7,
    76|    fontSize: 32, fontFace: "Arial Black", color: C.textDark, bold: true, margin: 0,
    77|  });
    78|
    79|  const items = [
    80|    { n: "01", t: "运行控制中心 (OCC)",       c: C.blue },
    81|    { n: "02", t: "信号与列车控制 (CBTC/GoA)", c: C.purple },
    82|    { n: "03", t: "路口信号优先 (TSP)",        c: C.amber },
    83|    { n: "04", t: "牵引供电系统",               c: C.red },
    84|    { n: "05", t: "车辆段管理 & 预测维护",      c: C.green },
    85|    { n: "06", t: "供应商生态 & 未来趋势",       c: C.cyan },
    86|  ];
    87|
    88|  items.forEach((it, i) => {
    89|    let y = 1.3 + i * 0.68;
    90|    // Number circle
    91|    s.addShape(pres.shapes.OVAL, {
    92|      x: 0.8, y: y, w: 0.45, h: 0.45, fill: { color: it.c },
    93|    });
    94|    s.addText(it.n, {
    95|      x: 0.8, y: y, w: 0.45, h: 0.45,
    96|      fontSize: 14, fontFace: "Arial Black", color: C.white,
    97|      align: "center", valign: "middle", margin: 0,
    98|    });
    99|    // Text
   100|    s.addText(it.t, {
   101|      x: 1.5, y: y, w: 7, h: 0.45,
   102|      fontSize: 18, fontFace: "Calibri", color: C.textDark,
   103|      align: "left", valign: "middle", margin: 0,
   104|    });
   105|    // Divider line
   106|    if (i < items.length - 1) {
   107|      s.addShape(pres.shapes.LINE, {
   108|        x: 1.5, y: y + 0.52, w: 7, h: 0,
   109|        line: { color: C.grey200, width: 0.75 },
   110|      });
   111|    }
   112|  });
   113|}
   114|
   115|// ═══════════════════════════════════════════
   116|// SLIDE 3 — System Architecture (light, SVG)
   117|// ═══════════════════════════════════════════
   118|{
   119|  let s = pres.addSlide();
   120|  s.background = { color: C.lightBg };
   121|
   122|  s.addText("系统架构总览", {
   123|    x: 0.6, y: 0.2, w: 8.8, h: 0.55,
   124|    fontSize: 28, fontFace: "Arial Black", color: C.textDark, bold: true, margin: 0,
   125|  });
   126|  s.addText("五大核心子系统由 OCC 统一调度", {
   127|    x: 0.6, y: 0.75, w: 8.8, h: 0.35,
   128|    fontSize: 14, fontFace: "Calibri", color: C.grey600, italic: true, margin: 0,
   129|  });
   130|
   131|  // SVG image — scale to fit
   132|  let svgW = 9.2, svgH = 5.85; // original 1100x700 ratio → ca 9.2 x 5.85 → scale to fit
   133|  let availH = 4.2;
   134|  let scale = availH / svgH;
   135|  let dispW = svgW * scale;
   136|  let dispH = svgH * scale;
   137|  let leftX = (10 - dispW) / 2;
   138|
   139|  try {
   140|    s.addImage({ path: "/opt/data/workspace/tram_ops_overview.svg", x: leftX, y: 1.25, w: dispW, h: dispH });
   141|  } catch (e) {
   142|    s.addText("(SVG 架构图)", { x: 2, y: 2, w: 6, h: 1, fontSize: 20, color: C.grey400, align: "center" });
   143|  }
   144|}
   145|
   146|// ═══════════════════════════════════════════
   147|// SLIDE 4 — OCC Deep Dive (light)
   148|// ═══════════════════════════════════════════
   149|{
   150|  let s = pres.addSlide();
   151|  s.background = { color: C.lightBg };
   152|
   153|  s.addText("运行控制中心 (OCC)", {
   154|    x: 0.6, y: 0.3, w: 8.8, h: 0.6,
   155|    fontSize: 28, fontFace: "Arial Black", color: C.textDark, bold: true, margin: 0,
   156|  });
   157|
   158|  const cards = [
   159|    { icon: "🎛️", title: "ATS 列车自动监督", desc: "监控所有列车位置/速度/准点率\n自动路由设定, 冲突检测\n时刻表实时调节", color: C.blue, bg: C.blueLt },
   160|    { icon: "⚡", title: "SCADA 设备监控", desc: "牵引供电:电压/电流/温度\n断路器远程分合闸\n隧道通风/消防/照明", color: C.purple, bg: C.purpleLt },
   161|    { icon: "📍", title: "AVLS 车辆定位", desc: "GPS + 里程计 + 地面信标\n精度 ±1~5 米\n实时位置上报 ATS/PIS", color: C.green, bg: C.greenLt },
   162|    { icon: "📢", title: "PIS + CCTV", desc: "站台实时到站预报\n车载广播 & 信息屏\n全线 IP 摄像头网络", color: C.amber, bg: C.amberLt },
   163|  ];
   164|
   165|  cards.forEach((c, i) => {
   166|    let col = i % 2, row = Math.floor(i / 2);
   167|    let x = 0.6 + col * 4.5;
   168|    let y = 1.15 + row * 2.1;
   169|
   170|    // Card bg
   171|    s.addShape(pres.shapes.RECTANGLE, {
   172|      x, y, w: 4.2, h: 1.85,
   173|      fill: { color: C.white },
   174|      shadow: { type: "outer", blur: 4, offset: 1, angle: 135, color: "000000", opacity: 0.08 },
   175|    });
   176|
   177|    // Left accent
   178|    s.addShape(pres.shapes.RECTANGLE, { x, y, w: 0.07, h: 1.85, fill: { color: c.color } });
   179|
   180|    // Icon + Title
   181|    s.addText(c.icon + "  " + c.title, {
   182|      x: x + 0.3, y: y + 0.15, w: 3.7, h: 0.45,
   183|      fontSize: 15, fontFace: "Calibri", color: c.color, bold: true, margin: 0,
   184|    });
   185|
   186|    // Description
   187|    s.addText(c.desc, {
   188|      x: x + 0.3, y: y + 0.65, w: 3.7, h: 1.05,
   189|      fontSize: 11, fontFace: "Calibri", color: C.grey600, align: "left", valign: "top", margin: 0,
   190|      lineSpacingMultiple: 1.3,
   191|    });
   192|  });
   193|
   194|  // Physical layout note
   195|  s.addShape(pres.shapes.RECTANGLE, { x: 0.6, y: 5.0, w: 8.8, h: 0.4, fill: { color: C.grey100 } });
   196|  s.addText("OCC 物理布局: 巨型视频墙 + 调度员工作站 + 冗余热备服务器 + UPS 不间断电源", {
   197|    x: 0.8, y: 5.0, w: 8.4, h: 0.4,
   198|    fontSize: 11, fontFace: "Calibri", color: C.grey600, align: "left", valign: "middle", margin: 0,
   199|  });
   200|}
   201|
   202|// ═══════════════════════════════════════════
   203|// SLIDE 5 — Signaling & Train Control (light)
   204|// ═══════════════════════════════════════════
   205|{
   206|  let s = pres.addSlide();
   207|  s.background = { color: C.lightBg };
   208|
   209|  s.addText("信号与列车控制", {
   210|    x: 0.6, y: 0.3, w: 8.8, h: 0.6,
   211|    fontSize: 28, fontFace: "Arial Black", color: C.textDark, bold: true, margin: 0,
   212|  });
   213|
   214|  // Left: Line-of-Sight vs CBTC
   215|  s.addShape(pres.shapes.RECTANGLE, { x: 0.6, y: 1.1, w: 4.2, h: 2.0, fill: { color: C.white }, shadow: { type: "outer", blur: 4, offset: 1, angle: 135, color: "000000", opacity: 0.08 } });
   216|  s.addShape(pres.shapes.RECTANGLE, { x: 0.6, y: 1.1, w: 0.07, h: 2.0, fill: { color: C.purple } });
   217|
   218|  s.addText("👁️ 传统: 目视行车 (LoS)", {
   219|    x: 0.9, y: 1.2, w: 3.7, h: 0.4,
   220|    fontSize: 15, fontFace: "Calibri", color: C.purple, bold: true, margin: 0,
   221|  });
   222|  s.addText([
   223|    { text: "• 司机目视控制速度和制动", options: { bullet: false, breakLine: true } },
   224|    { text: "• 遵循道路交通规则", options: { bullet: false, breakLine: true } },
   225|    { text: "• 仅道岔/单线/隧道设信号", options: { bullet: false, breakLine: true } },
   226|    { text: "• 适用: 路面混合路权路段", options: { bullet: false } },
   227|  ], {
   228|    x: 0.9, y: 1.65, w: 3.7, h: 1.3,
   229|    fontSize: 11, fontFace: "Calibri", color: C.grey600, lineSpacingMultiple: 1.3, margin: 0,
   230|  });
   231|
   232|  // Right: CBTC
   233|  s.addShape(pres.shapes.RECTANGLE, { x: 5.2, y: 1.1, w: 4.2, h: 2.0, fill: { color: C.white }, shadow: { type: "outer", blur: 4, offset: 1, angle: 135, color: "000000", opacity: 0.08 } });
   234|  s.addShape(pres.shapes.RECTANGLE, { x: 5.2, y: 1.1, w: 0.07, h: 2.0, fill: { color: C.blue } });
   235|
   236|  s.addText("📡 现代: CBTC (IEEE 1474)", {
   237|    x: 5.5, y: 1.2, w: 3.7, h: 0.4,
   238|    fontSize: 15, fontFace: "Calibri", color: C.blue, bold: true, margin: 0,
   239|  });
   240|  s.addText([
   241|    { text: "ATP — 自动防护: 超速强制制动", options: { bullet: false, breakLine: true } },
   242|    { text: "ATO — 自动运行: 速度调节, 精确停车", options: { bullet: false, breakLine: true } },
   243|    { text: "ATS — 自动监督: 路由设定, 时刻表调节", options: { bullet: false, breakLine: true } },
   244|    { text: "适用: 独立路权, 高密度线路", options: { bullet: false } },
   245|  ], {
   246|    x: 5.5, y: 1.65, w: 3.7, h: 1.3,
   247|    fontSize: 11, fontFace: "Calibri", color: C.grey600, lineSpacingMultiple: 1.3, margin: 0,
   248|  });
   249|
   250|  // GoA levels — horizontal bar graph
   251|  s.addText("自动化等级 (GoA)", {
   252|    x: 0.6, y: 3.4, w: 8.8, h: 0.4,
   253|    fontSize: 16, fontFace: "Calibri", color: C.textDark, bold: true, margin: 0,
   254|  });
   255|
   256|  const goa = [
   257|    { l: "GoA 0", d: "全手动", col: C.grey400, w: 1.5 },
   258|    { l: "GoA 1", d: "手动+ATP防护", col: C.blue, w: 1.8 },
   259|    { l: "GoA 2", d: "半自动 (ATO+司机)", col: C.green, w: 2.2 },
   260|    { l: "GoA 3", d: "无人驾驶 (有乘务员)", col: C.amber, w: 2.2 },
   261|    { l: "GoA 4", d: "全自动无人值守", col: C.purple, w: 1.5 },
   262|  ];
   263|
   264|  let gx = 0.6;
   265|  goa.forEach(g => {
   266|    s.addShape(pres.shapes.RECTANGLE, {
   267|      x: gx, y: 3.9, w: g.w - 0.08, h: 0.7, fill: { color: g.col },
   268|    });
   269|    s.addText(g.l, {
   270|      x: gx, y: 3.9, w: g.w - 0.08, h: 0.4,
   271|      fontSize: 12, fontFace: "Arial Black", color: C.white, align: "center", valign: "bottom", margin: 0,
   272|    });
   273|    s.addText(g.d, {
   274|      x: gx, y: 4.25, w: g.w - 0.08, h: 0.3,
   275|      fontSize: 8, fontFace: "Calibri", color: C.white, align: "center", valign: "top", margin: 0,
   276|    });
   277|    gx += g.w;
   278|  });
   279|
   280|  // Highlight: Siemens driverless tram
   281|  s.addShape(pres.shapes.RECTANGLE, { x: 0.6, y: 4.9, w: 8.8, h: 0.5, fill: { color: C.purpleLt } });
   282|  s.addText("🔬 Siemens 已在德国波茨坦成功测试 GoA 4 无人驾驶电车 (2023) — 全程 6km 无司机运行", {
   283|    x: 0.8, y: 4.9, w: 8.4, h: 0.5,
   284|    fontSize: 11, fontFace: "Calibri", color: C.purple, align: "left", valign: "middle", margin: 0,
   285|  });
   286|}
   287|
   288|// ═══════════════════════════════════════════
   289|// SLIDE 6 — TSP (light, SVG)
   290|// ═══════════════════════════════════════════
   291|{
   292|  let s = pres.addSlide();
   293|  s.background = { color: C.lightBg };
   294|
   295|  s.addText("路口信号优先 (TSP)", {
   296|    x: 0.6, y: 0.2, w: 8.8, h: 0.55,
   297|    fontSize: 28, fontFace: "Arial Black", color: C.textDark, bold: true, margin: 0,
   298|  });
   299|  s.addText("Transit Signal Priority — 电车如何\"一路绿灯\"", {
   300|    x: 0.6, y: 0.75, w: 8.8, h: 0.3,
   301|    fontSize: 13, fontFace: "Calibri", color: C.grey600, italic: true, margin: 0,
   302|  });
   303|
   304|  try {
   305|    s.addImage({ path: "/opt/data/workspace/tram_tsp_diagram.svg", x: 0.3, y: 1.2, w: 9.4, h: 4.2 });
   306|  } catch (e) {
   307|    s.addText("(TSP 信号优先图)", { x: 2, y: 2, w: 6, h: 1, fontSize: 20, color: C.grey400, align: "center" });
   308|  }
   309|}
   310|
   311|// ═══════════════════════════════════════════
   312|// SLIDE 7 — Traction Power (light, SVG)
   313|// ═══════════════════════════════════════════
   314|{
   315|  let s = pres.addSlide();
   316|  s.background = { color: C.lightBg };
   317|
   318|  s.addText("牵引供电系统", {
   319|    x: 0.6, y: 0.2, w: 8.8, h: 0.55,
   320|    fontSize: 28, fontFace: "Arial Black", color: C.textDark, bold: true, margin: 0,
   321|  });
   322|  s.addText("750V DC — 从城市电网到接触网", {
   323|    x: 0.6, y: 0.75, w: 8.8, h: 0.3,
   324|    fontSize: 13, fontFace: "Calibri", color: C.grey600, italic: true, margin: 0,
   325|  });
   326|
   327|  try {
   328|    s.addImage({ path: "/opt/data/workspace/tram_power_system.svg", x: 0.3, y: 1.2, w: 9.4, h: 4.2 });
   329|  } catch (e) {
   330|    s.addText("(供电系统图)", { x: 2, y: 2, w: 6, h: 1, fontSize: 20, color: C.grey400, align: "center" });
   331|  }
   332|}
   333|
   334|// ═══════════════════════════════════════════
   335|// SLIDE 8 — Depot + Predictive Maintenance (light)
   336|// ═══════════════════════════════════════════
   337|{
   338|  let s = pres.addSlide();
   339|  s.background = { color: C.lightBg };
   340|
   341|  s.addText("车辆段管理 & 预测维护", {
   342|    x: 0.6, y: 0.3, w: 8.8, h: 0.6,
   343|    fontSize: 28, fontFace: "Arial Black", color: C.textDark, bold: true, margin: 0,
   344|  });
   345|
   346|  // Left column: Depot
   347|  s.addShape(pres.shapes.RECTANGLE, { x: 0.6, y: 1.1, w: 4.2, h: 4.2, fill: { color: C.white }, shadow: { type: "outer", blur: 4, offset: 1, angle: 135, color: "000000", opacity: 0.08 } });
   348|  s.addShape(pres.shapes.RECTANGLE, { x: 0.6, y: 1.1, w: 0.07, h: 4.2, fill: { color: C.green } });
   349|
   350|  s.addText("🏭 车辆段功能", {
   351|    x: 0.9, y: 1.2, w: 3.7, h: 0.4,
   352|    fontSize: 16, fontFace: "Calibri", color: C.green, bold: true, margin: 0,
   353|  });
   354|
   355|  const depotItems = [
   356|    "停车调度 — 轨道分配, 过夜停放",
   357|    "车辆清洗 — 自动洗车线",
   358|    "耗材补充 — 撒砂, 雨刮液等",
   359|    "段内信号 — 联锁系统 (CBI)",
   360|    "与车队管理 / ERP 集成",
   361|  ];
   362|  depotItems.forEach((t, i) => {
   363|    s.addShape(pres.shapes.OVAL, {
   364|      x: 0.9, y: 1.85 + i * 0.45, w: 0.22, h: 0.22, fill: { color: C.green },
   365|    });
   366|    s.addText(String(i + 1), {
   367|      x: 0.9, y: 1.85 + i * 0.45, w: 0.22, h: 0.22,
   368|      fontSize: 10, fontFace: "Calibri", color: C.white, align: "center", valign: "middle", margin: 0,
   369|    });
   370|    s.addText(t, {
   371|      x: 1.25, y: 1.8 + i * 0.45, w: 3.3, h: 0.35,
   372|      fontSize: 11, fontFace: "Calibri", color: C.grey600, valign: "middle", margin: 0,
   373|    });
   374|  });
   375|
   376|  // Maintenance levels
   377|  s.addText("维护等级", {
   378|    x: 0.9, y: 4.1, w: 3.7, h: 0.3,
   379|    fontSize: 13, fontFace: "Calibri", color: C.textDark, bold: true, margin: 0,
   380|  });
   381|  const mLevels = [
   382|    { l: "日检", d: "目视+清洁", c: C.green },
   383|    { l: "定检", d: "闸瓦/轮对/碳条", c: C.blue },
   384|    { l: "架修", d: "转向架/电机", c: C.amber },
   385|    { l: "大修", d: "10-15年全拆解", c: C.red },
   386|  ];
   387|  let mlx = 0.9;
   388|  mLevels.forEach(ml => {
   389|    s.addShape(pres.shapes.RECTANGLE, { x: mlx, y: 4.5, w: 0.95, h: 0.65, fill: { color: ml.c } });
   390|    s.addText(ml.l, {
   391|      x: mlx, y: 4.5, w: 0.95, h: 0.35,
   392|      fontSize: 11, fontFace: "Arial Black", color: C.white, align: "center", valign: "bottom", margin: 0,
   393|    });
   394|    s.addText(ml.d, {
   395|      x: mlx, y: 4.78, w: 0.95, h: 0.3,
   396|      fontSize: 7, fontFace: "Calibri", color: C.white, align: "center", valign: "top", margin: 0,
   397|    });
   398|    mlx += 1.0;
   399|  });
   400|
   401|  // Right column: Predictive Maintenance
   402|  s.addShape(pres.shapes.RECTANGLE, { x: 5.2, y: 1.1, w: 4.2, h: 4.2, fill: { color: C.white }, shadow: { type: "outer", blur: 4, offset: 1, angle: 135, color: "000000", opacity: 0.08 } });
   403|  s.addShape(pres.shapes.RECTANGLE, { x: 5.2, y: 1.1, w: 0.07, h: 4.2, fill: { color: C.cyan } });
   404|
   405|  s.addText("📊 预测维护体系", {
   406|    x: 5.5, y: 1.2, w: 3.7, h: 0.4,
   407|    fontSize: 16, fontFace: "Calibri", color: C.cyan, bold: true, margin: 0,
   408|  });
   409|
   410|  const pmItems = [
   411|    { t: "车载 TCMS", d: "实时采集牵引/制动/车门/空调数据" },
   412|    { t: "事件记录仪", d: "防撞\"黑匣子\", 数据无线卸载(Wi-Fi/LTE)" },
   413|    { t: "轮对冲击检测", d: "WILD — 检测车轮扁疤和轨道冲击力" },
   414|    { t: "受电弓监测", d: "摄像头/激光检测碳条磨损和接触网状态" },
   415|    { t: "AI 故障预测", d: "ML 建模 → 提前预测故障 → 自动生成工单" },
   416|  ];
   417|  pmItems.forEach((pm, i) => {
   418|    let py = 1.85 + i * 0.62;
   419|    s.addShape(pres.shapes.RECTANGLE, { x: 5.5, y: py, w: 3.7, h: 0.5, fill: { color: C.grey100 } });
   420|    s.addText(pm.t, {
   421|      x: 5.6, y: py, w: 3.5, h: 0.25,
   422|      fontSize: 11, fontFace: "Calibri", color: C.textDark, bold: true, margin: 0,
   423|    });
   424|    s.addText(pm.d, {
   425|      x: 5.6, y: py + 0.23, w: 3.5, h: 0.25,
   426|      fontSize: 9, fontFace: "Calibri", color: C.grey600, margin: 0,
   427|    });
   428|  });
   429|}
   430|
   431|// ═══════════════════════════════════════════
   432|// SLIDE 9 — Vendor Ecosystem (light)
   433|// ═══════════════════════════════════════════
   434|{
   435|  let s = pres.addSlide();
   436|  s.background = { color: C.lightBg };
   437|
   438|  s.addText("全球供应商生态", {
   439|    x: 0.6, y: 0.3, w: 8.8, h: 0.6,
   440|    fontSize: 28, fontFace: "Arial Black", color: C.textDark, bold: true, margin: 0,
   441|  });
   442|
   443|  const headerOpts = { fill: { color: C.grey800 }, color: C.white, bold: true, fontSize: 11, fontFace: "Calibri", align: "center", valign: "middle", margin: 0 };
   444|  const cellOpts = { fill: { color: C.white }, color: C.textDark, fontSize: 10, fontFace: "Calibri", align: "left", valign: "middle", margin: [2, 6, 2, 6] };
   445|  const altOpts = { fill: { color: C.grey100 }, color: C.textDark, fontSize: 10, fontFace: "Calibri", align: "left", valign: "middle", margin: [2, 6, 2, 6] };
   446|
   447|  let rows = [
   448|    [
   449|      { text: "供应商", options: headerOpts },
   450|      { text: "核心产品", options: headerOpts },
   451|      { text: "优势领域", options: headerOpts },
   452|    ],
   453|    [
   454|      { text: "Siemens Mobility", options: { ...cellOpts, bold: true } },
   455|      { text: "Trainguard MT · Vicos ATS\nAvenio 电车 · Sitras 供电", options: cellOpts },
   456|      { text: "端到端集成\n无人驾驶电车研发", options: cellOpts },
   457|    ],
   458|    [
   459|      { text: "Alstom", options: { ...altOpts, bold: true } },
   460|      { text: "Iconis ATS · Smartlock CBI\nCitadis 电车 · APS 地面供电", options: altOpts },
   461|      { text: "无触网供电 (APS)\n交钥匙电车系统", options: altOpts },
   462|    ],
   463|    [
   464|      { text: "CRRC 中国中车", options: { ...cellOpts, bold: true } },
   465|      { text: "国产 CBTC · TST 电车信号\n大连/株洲电车车辆", options: cellOpts },
   466|      { text: "中国市场主导\n成本优势, 交钥匙", options: cellOpts },
   467|    ],
   468|    [
   469|      { text: "Thales", options: { ...altOpts, bold: true } },
   470|      { text: "SelTrac CBTC\nTMS 电车管理系统", options: altOpts },
   471|      { text: "曼彻斯特 Metrolink\n深厚信号技术积累", options: altOpts },
   472|    ],
   473|    [
   474|      { text: "Hitachi Rail", options: { ...cellOpts, bold: true } },
   475|      { text: "ATS · CBTC 解决方案", options: cellOpts },
   476|      { text: "并购 Thales/Ansaldo\n信号组合丰富", options: cellOpts },
   477|    ],
   478|  ];
   479|
   480|  s.addTable(rows, {
   481|    x: 0.6, y: 1.15, w: 8.8,
   482|    colW: [2.0, 3.5, 3.3],
   483|    border: { pt: 0.5, color: C.grey200 },
   484|    rowH: [0.4, 0.65, 0.65, 0.65, 0.65, 0.65],
   485|  });
   486|
   487|  // CAF note
   488|  s.addShape(pres.shapes.RECTANGLE, { x: 0.6, y: 4.95, w: 8.8, h: 0.4, fill: { color: C.grey100 } });
   489|  s.addText("另有: CAF (西班牙) — Urbos 电车平台, 接触网/电池混合供电方案", {
   490|    x: 0.8, y: 4.95, w: 8.4, h: 0.4,
   491|    fontSize: 10, fontFace: "Calibri", color: C.grey600, align: "left", valign: "middle", margin: 0,
   492|  });
   493|}
   494|
   495|// ═══════════════════════════════════════════
   496|// SLIDE 10 — Future Trends (dark)
   497|// ═══════════════════════════════════════════
   498|{
   499|  let s = pres.addSlide();
   500|  s.background = { color: C.darkBg };
   501|
   502|  s.addText("未来趋势", {
   503|    x: 0.6, y: 0.3, w: 8.8, h: 0.6,
   504|    fontSize: 28, fontFace: "Arial Black", color: C.textLight, bold: true, margin: 0,
   505|  });
   506|
   507|  const trends = [
   508|    { icon: "🔋", title: "无触网供电", desc: "APS / 超级电容 / 电池 / 氢燃料\n消除架空线视觉污染", col: C.green },
   509|    { icon: "☁️",  title: "云化 OCC", desc: "从本地服务器迁移至云端\n降低成本, 弹性扩展, 灾备", col: C.blue },
   510|    { icon: "🤖", title: "自动驾驶", desc: "GoA 4 无人驾驶\nSiemens 波茨坦已示范", col: C.purple },
   511|    { icon: "🪞", title: "数字孪生", desc: "全网络虚拟镜像\nWhat-if 仿真, 优化, 培训", col: C.cyan },
   512|    { icon: "📡", title: "C-V2X", desc: "5G 车路协同\n电车⇔信号灯⇔其他车辆直接通信", col: C.amber },
   513|    { icon: "⚡", title: "智能电网融合", desc: "再生能量回馈电网\n电车作为移动储能单元", col: C.red },
   514|  ];
   515|
   516|  trends.forEach((t, i) => {
   517|    let col = i % 3, row = Math.floor(i / 3);
   518|    let x = 0.6 + col * 3.05;
   519|    let y = 1.15 + row * 2.15;
   520|
   521|    s.addShape(pres.shapes.RECTANGLE, {
   522|      x, y, w: 2.85, h: 1.9,
   523|      fill: { color: C.grey800 },
   524|    });
   525|
   526|    // Top accent
   527|    s.addShape(pres.shapes.RECTANGLE, { x, y, w: 2.85, h: 0.05, fill: { color: t.col } });
   528|
   529|    // Icon
   530|    s.addText(t.icon, {
   531|      x, y: y + 0.15, w: 2.85, h: 0.55,
   532|      fontSize: 28, align: "center", valign: "middle", margin: 0,
   533|    });
   534|
   535|    // Title
   536|    s.addText(t.title, {
   537|      x: x + 0.2, y: y + 0.75, w: 2.45, h: 0.35,
   538|      fontSize: 14, fontFace: "Calibri", color: t.col, bold: true, align: "center", margin: 0,
   539|    });
   540|
   541|    // Description
   542|    s.addText(t.desc, {
   543|      x: x + 0.2, y: y + 1.15, w: 2.45, h: 0.6,
   544|      fontSize: 9, fontFace: "Calibri", color: C.grey400, align: "center", lineSpacingMultiple: 1.3, margin: 0,
   545|    });
   546|  });
   547|}
   548|
   549|// ═══════════════════════════════════════════
   550|// SLIDE 11 — Thank You / Q&A (dark)
   551|// ═══════════════════════════════════════════
   552|{
   553|  let s = pres.addSlide();
   554|  s.background = { color: C.darkBg };
   555|
   556|  // Accent bar top
   557|  s.addShape(pres.shapes.RECTANGLE, { x: 0, y: 0, w: 10, h: 0.06, fill: { color: C.blue } });
   558|
   559|  s.addText("谢谢！", {
   560|    x: 0.8, y: 1.5, w: 8.4, h: 1.0,
   561|    fontSize: 48, fontFace: "Arial Black", color: C.textLight,
   562|    bold: true, align: "center", margin: 0,
   563|  });
   564|
   565|  s.addText("Questions & Discussion", {
   566|    x: 0.8, y: 2.5, w: 8.4, h: 0.6,
   567|    fontSize: 20, fontFace: "Calibri", color: C.grey400,
   568|    italic: true, align: "center", margin: 0,
   569|  });
   570|
   571|  // Divider
   572|  s.addShape(pres.shapes.LINE, {
   573|    x: 3.5, y: 3.4, w: 3, h: 0,
   574|    line: { color: C.blue, width: 2 },
   575|  });
   576|
   577|  // Key takeaways
   578|  s.addText([
   579|    { text: "运行管理 = OCC 统一调度 + 5大子系统协同", options: { breakLine: true, bullet: false } },
   580|    { text: "TSP 路口优先是电车准时运行的核心技术", options: { breakLine: true, bullet: false } },
   581|    { text: "供电走向无触网 (APS/超级电容/氢能)", options: { breakLine: true, bullet: false } },
   582|    { text: "AI 预测维护 + 云化 OCC + 自动驾驶 = 未来方向", options: { bullet: false } },
   583|  ], {
   584|    x: 1.5, y: 3.7, w: 7, h: 1.5,
   585|    fontSize: 13, fontFace: "Calibri", color: C.grey400,
   586|    align: "center", lineSpacingMultiple: 1.4, margin: 0,
   587|  });
   588|
   589|  s.addText("Generated by Hermes Agent · May 2026", {
   590|    x: 0.8, y: 5.2, w: 8.4, h: 0.3,
   591|    fontSize: 9, fontFace: "Calibri", color: C.grey600,
   592|    align: "center", margin: 0,
   593|  });
   594|}
   595|
   596|// ── Save ──
   597|const outPath = "/opt/data/workspace/电车运行管理系统.pptx";
   598|pres.writeFile({ fileName: outPath }).then(() => {
   599|  console.log("✅ PPT saved to " + outPath);
   600|}).catch(err => {
   601|  console.error("❌ Error:", err.message);
   602|});
   603|
```
