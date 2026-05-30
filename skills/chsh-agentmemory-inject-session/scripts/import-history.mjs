#!/usr/bin/env node
/**
 * Import Cursor + Claude + GitHub Copilot history into a remote AgentMemory instance.
 *
 * Sources (in order):
 *   1. ~/.claude/projects  — Claude Code JSONL transcripts
 *   2. ~/.cursor/projects  — Cursor agent-transcript JSONL
 *   3. state.vscdb (cursorDiskKV) — Cursor sidebar bubbles (composerData)
 *   4. ~/Library/…/workspaceStorage  — VS Code GitHub Copilot sessions
 *        4a. <hash>/GitHub.copilot-chat/transcripts/*.jsonl  (new format, Apr 2026+)
 *        4b. <hash>/chatSessions/*.json                       (old format, pre-Apr 2026)
 *
 * Dedup: server session IDs are fetched first; already-known sessions
 * are skipped locally and never uploaded.
 *
 * Usage:
 *   AGENTMEMORY_URL=http://... AGENTMEMORY_SECRET=... node import-history.mjs
 * Or rely on mcp.json auto-detection (strips // comments before parse).
 */
import { execFileSync } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import { readFileSync, readdirSync, existsSync } from 'node:fs';
import { homedir } from 'node:os';
import { join, basename } from 'node:path';

const VERSION = '0.9.24';
const GLOBAL_DB = join(
  homedir(),
  'Library/Application Support/Cursor/User/globalStorage/state.vscdb',
);

// ─── Config ──────────────────────────────────────────────────────────────────

function loadConfig() {
  let url = process.env.AGENTMEMORY_URL;
  let secret = process.env.AGENTMEMORY_SECRET;
  if (!url || !secret) {
    try {
      const raw = readFileSync(join(homedir(), '.cursor/mcp.json'), 'utf8');
      const stripped = raw.replace(/^\s*\/\/.*$/gm, ''); // strip comment-only lines, not URLs
      const mcp = JSON.parse(stripped);
      const env = mcp?.mcpServers?.agentmemory?.env ?? {};
      if (!url) url = env.AGENTMEMORY_URL;
      if (!secret) secret = env.AGENTMEMORY_SECRET;
    } catch {}
  }
  if (!secret) throw new Error('Set AGENTMEMORY_SECRET or configure ~/.cursor/mcp.json');
  return { url: (url || 'http://localhost:3111').replace(/\/+$/, ''), secret };
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

function generateId(prefix) {
  return `${prefix}_${randomUUID().replace(/-/g, '').slice(0, 12)}`;
}

function deriveProject(cwd) {
  if (!cwd || cwd === '/') return 'unknown';
  const parts = cwd.split('/').filter(Boolean);
  return parts[parts.length - 1] || 'unknown';
}

function toText(content) {
  if (typeof content === 'string') return content;
  if (content == null) return '';
  if (Array.isArray(content)) {
    return content
      .map((c) => {
        if (typeof c === 'string') return c;
        if (c?.type === 'text' && typeof c.text === 'string') return c.text;
        if (typeof c?.text === 'string') return c.text;
        return '';
      })
      .filter(Boolean)
      .join('\n');
  }
  if (typeof content === 'object' && typeof content.text === 'string') return content.text;
  return '';
}

function extractToolUses(content) {
  const out = [];
  if (!Array.isArray(content)) return out;
  const seen = new Set();
  for (const c of content) {
    if (c?.type === 'tool_use' && c.name && !seen.has(c.id)) {
      seen.add(c.id);
      out.push({ name: c.name, input: c.input || {}, id: c.id });
    }
  }
  return out;
}

// ─── JSONL parser ─────────────────────────────────────────────────────────────

function parseJsonlText(text, fallbackSessionId, sourceTag) {
  const entries = [];
  for (const line of text.split('\n')) {
    const t = line.trim();
    if (!t) continue;
    try {
      const p = JSON.parse(t);
      if (p && typeof p === 'object') entries.push(p);
    } catch {}
  }

  let sessionId = '';
  let cwd = '';
  let firstTs = '';
  let lastTs = '';
  const observations = [];

  for (const entry of entries) {
    if (entry.sessionId && !sessionId) sessionId = entry.sessionId;
    if (entry.cwd && !cwd) cwd = entry.cwd;
    const ts = entry.timestamp || entry.createdAt || new Date().toISOString();
    if (!firstTs) firstTs = ts;
    lastTs = ts;

    const role = entry.message?.role || entry.role;
    const content = entry.message?.content;
    const entryType = entry.type || '';

    if ((entryType === 'user' || role === 'user') && role === 'user') {
      const text = toText(content);
      if (text.trim()) {
        observations.push({
          id: generateId('obs'),
          sessionId: 'imported',
          timestamp: ts,
          hookType: 'prompt_submit',
          userPrompt: text,
          source: sourceTag,
        });
      }
    } else if ((entryType === 'assistant' || role === 'assistant') && role === 'assistant') {
      const text = toText(content);
      const tools = extractToolUses(content);
      if (text.trim()) {
        observations.push({
          id: generateId('obs'),
          sessionId: 'imported',
          timestamp: ts,
          hookType: 'stop',
          assistantResponse: text,
          source: sourceTag,
        });
      }
      for (const tool of tools) {
        observations.push({
          id: generateId('obs'),
          sessionId: 'imported',
          timestamp: ts,
          hookType: 'pre_tool_use',
          toolName: tool.name,
          toolInput: tool.input,
          source: sourceTag,
        });
      }
    }
  }

  const effectiveId = sessionId || fallbackSessionId || generateId('sess');
  for (const obs of observations) {
    if (obs.sessionId === 'imported') obs.sessionId = effectiveId;
  }

  const now = new Date().toISOString();
  return {
    sessionId: effectiveId,
    project: deriveProject(cwd),
    cwd: cwd || process.cwd(),
    startedAt: firstTs || now,
    endedAt: lastTs || now,
    observations,
    tags: [sourceTag],
  };
}

// ─── File scanning ────────────────────────────────────────────────────────────

function findJsonlFiles(root, maxFiles = 1000) {
  const files = [];
  const stack = [root];
  while (stack.length && files.length < maxFiles) {
    const dir = stack.pop();
    let entries;
    try { entries = readdirSync(dir, { withFileTypes: true }); } catch { continue; }
    for (const ent of entries) {
      if (files.length >= maxFiles) break;
      const full = join(dir, ent.name);
      if (ent.isDirectory()) stack.push(full);
      else if (ent.isFile() && ent.name.endsWith('.jsonl')) files.push(full);
    }
  }
  return files;
}

function importJsonlDir(dir, sourceTag, skipIds) {
  const files = findJsonlFiles(dir, 1000);
  const sessions = [];
  for (const file of files) {
    let text;
    try { text = readFileSync(file, 'utf8'); } catch { continue; }
    const parsed = parseJsonlText(text, basename(file, '.jsonl'), sourceTag);
    if (!parsed.observations.length) continue;
    if (skipIds.has(parsed.sessionId)) continue;
    skipIds.add(parsed.sessionId);
    sessions.push(parsed);
  }
  return { files: files.length, sessions };
}

// ─── Cursor sidebar (state.vscdb) ─────────────────────────────────────────────

function extractLexicalText(richText) {
  if (!richText) return '';
  try {
    const root = typeof richText === 'string' ? JSON.parse(richText).root : richText.root;
    const parts = [];
    const walk = (n) => {
      if (!n || typeof n !== 'object') return;
      if (n.type === 'text' && n.text) parts.push(n.text);
      for (const c of n.children || []) walk(c);
    };
    walk(root || {});
    return parts.join('');
  } catch {
    return typeof richText === 'string' ? richText : '';
  }
}

function extractBubbleText(bubble) {
  const r = extractLexicalText(bubble.richText);
  if (r.trim()) return r.trim();
  if (typeof bubble.text === 'string' && bubble.text.trim()) return bubble.text.trim();
  if (bubble.thinking?.text?.trim()) return bubble.thinking.text.trim();
  return '';
}

function loadSidebarSessions(skipIds) {
  if (!existsSync(GLOBAL_DB)) {
    console.warn('  state.vscdb not found, skipping sidebar step');
    return [];
  }

  const count = Number.parseInt(
    execFileSync('sqlite3', [GLOBAL_DB, "SELECT count(*) FROM cursorDiskKV WHERE key LIKE 'bubbleId:%';"],
      { encoding: 'utf8' }).trim(), 10,
  );
  const batchSize = 400;
  const byComposer = new Map();

  for (let offset = 0; offset < count; offset += batchSize) {
    const raw = execFileSync('sqlite3', [
      GLOBAL_DB,
      `SELECT key, hex(value) FROM cursorDiskKV WHERE key LIKE 'bubbleId:%' LIMIT ${batchSize} OFFSET ${offset};`,
    ], { encoding: 'utf8', maxBuffer: 128 * 1024 * 1024 });

    for (const line of raw.split('\n')) {
      if (!line.trim()) continue;
      const sep = line.indexOf('|');
      if (sep < 0) continue;
      const key = line.slice(0, sep);
      const hex = line.slice(sep + 1);
      const m = key.match(/^bubbleId:([^:]+):/);
      if (!m) continue;
      const composerId = m[1];
      if (skipIds.has(composerId)) continue; // already on server
      let bubble;
      try { bubble = JSON.parse(Buffer.from(hex, 'hex').toString('utf8')); } catch { continue; }
      const text = extractBubbleText(bubble);
      if (!text.trim()) continue;
      if (!byComposer.has(composerId)) byComposer.set(composerId, []);
      byComposer.get(composerId).push({
        type: bubble.type,
        text,
        createdAt: bubble.createdAt || new Date().toISOString(),
      });
    }
    if ((offset / batchSize) % 25 === 0 && offset > 0) {
      process.stdout.write(`    …${offset}/${count} bubbles scanned\r`);
    }
  }
  if (count > 0) process.stdout.write(`    …${count}/${count} bubbles scanned\n`);

  const sessions = [];
  for (const [composerId, bubbles] of byComposer) {
    if (skipIds.has(composerId)) continue;
    bubbles.sort((a, b) => String(a.createdAt).localeCompare(String(b.createdAt)));
    const observations = bubbles.map((b) => ({
      id: generateId('obs'),
      sessionId: composerId,
      timestamp: b.createdAt,
      hookType: b.type === 1 ? 'prompt_submit' : 'stop',
      ...(b.type === 1 ? { userPrompt: b.text } : { assistantResponse: b.text }),
      source: 'cursor-sidebar',
    }));
    skipIds.add(composerId);
    sessions.push({
      sessionId: composerId,
      project: 'cursor-sidebar',
      cwd: homedir(),
      startedAt: observations[0].timestamp,
      endedAt: observations[observations.length - 1].timestamp,
      observations,
      tags: ['cursor-sidebar-import'],
    });
  }
  return sessions;
}

// ─── VS Code Copilot sessions ─────────────────────────────────────────────────

const VSCODE_WS_BASE = join(
  homedir(),
  'Library/Application Support/Code/User/workspaceStorage',
);

/**
 * Parse a VS Code Copilot transcript JSONL (new format, Apr 2026+).
 * Events: session.start, user.message, assistant.turn_start, assistant.response
 */
function parseCopilotTranscriptJsonl(text, fallbackId) {
  const observations = [];
  let sessionId = fallbackId;
  let cwd = '';
  let firstTs = '';
  let lastTs = '';

  for (const line of text.split('\n')) {
    const t = line.trim();
    if (!t) continue;
    let entry;
    try { entry = JSON.parse(t); } catch { continue; }
    if (!entry || typeof entry !== 'object') continue;

    const { type, data = {} } = entry;
    const ts = data.timestamp || data.startTime || new Date().toISOString();
    if (!firstTs) firstTs = ts;
    lastTs = ts;

    if (type === 'session.start') {
      if (data.sessionId) sessionId = data.sessionId;
      if (data.cwd) cwd = data.cwd;
    } else if (type === 'user.message') {
      const text = typeof data.content === 'string' ? data.content.trim()
        : toText(data.content);
      if (text) {
        observations.push({
          id: generateId('obs'),
          sessionId: 'imported',
          timestamp: ts,
          hookType: 'prompt_submit',
          userPrompt: text,
          source: 'copilot-transcript',
        });
      }
    } else if (type === 'assistant.response' || type === 'assistant.turn') {
      const text = typeof data.content === 'string' ? data.content.trim()
        : toText(data.content);
      if (text) {
        observations.push({
          id: generateId('obs'),
          sessionId: 'imported',
          timestamp: ts,
          hookType: 'stop',
          assistantResponse: text,
          source: 'copilot-transcript',
        });
      }
    }
  }

  for (const obs of observations) {
    if (obs.sessionId === 'imported') obs.sessionId = sessionId;
  }
  const now = new Date().toISOString();
  return { sessionId, cwd: cwd || VSCODE_WS_BASE, startedAt: firstTs || now, endedAt: lastTs || now, observations };
}

/**
 * Parse a VS Code Copilot chatSession JSON (old format, pre-Apr 2026).
 * Fields: sessionId, creationDate (ms), lastMessageDate (ms), requests[].message.parts[].text
 */
function parseCopilotChatSessionJson(text, fallbackId) {
  let data;
  try { data = JSON.parse(text); } catch { return null; }
  if (!data || typeof data !== 'object') return null;

  const sessionId = data.sessionId || fallbackId;
  const startedAt = data.creationDate
    ? new Date(data.creationDate).toISOString() : new Date().toISOString();
  const endedAt = data.lastMessageDate
    ? new Date(data.lastMessageDate).toISOString() : startedAt;

  const observations = [];
  for (const req of (data.requests || [])) {
    // Extract user message text from parts array
    const parts = req?.message?.parts || [];
    let userText = '';
    for (const p of parts) {
      if (typeof p?.text === 'string' && p.text.trim()) {
        userText += p.text;
      }
    }
    if (userText.trim()) {
      observations.push({
        id: generateId('obs'),
        sessionId,
        timestamp: startedAt,
        hookType: 'prompt_submit',
        userPrompt: userText.trim(),
        source: 'copilot-chatsession',
      });
    }
    // Responses are not stored in old format (only user messages)
  }

  return { sessionId, cwd: VSCODE_WS_BASE, startedAt, endedAt, observations };
}

/** Load the workspace name from workspace.json alongside a workspaceStorage hash dir */
function wsName(wsDir) {
  try {
    const raw = readFileSync(join(wsDir, 'workspace.json'), 'utf8');
    const ws = JSON.parse(raw).workspace || '';
    return ws.replace(/file:\/\/\/opt\/nordic\/ncs\//g, '')
      .replace(/file:\/\/\//g, '')
      .replace(/\.code-workspace$/, '');
  } catch {
    return '';
  }
}

/** Scan all VS Code workspaceStorage dirs and collect Copilot sessions */
function loadCopilotSessions(skipIds) {
  if (!existsSync(VSCODE_WS_BASE)) {
    console.warn('  VS Code workspaceStorage not found, skipping Copilot step');
    return { newTranscripts: 0, newChatSessions: 0, sessions: [] };
  }

  let wsDirs;
  try { wsDirs = readdirSync(VSCODE_WS_BASE, { withFileTypes: true }); } catch { return { newTranscripts: 0, newChatSessions: 0, sessions: [] }; }

  const sessions = [];
  let transcriptFiles = 0;
  let chatSessionFiles = 0;

  for (const ent of wsDirs) {
    if (!ent.isDirectory()) continue;
    const wsDir = join(VSCODE_WS_BASE, ent.name);
    const project = wsName(wsDir) || ent.name.slice(0, 8);

    // 4a: new JSONL transcripts
    const transcriptsDir = join(wsDir, 'GitHub.copilot-chat', 'transcripts');
    if (existsSync(transcriptsDir)) {
      let files;
      try { files = readdirSync(transcriptsDir); } catch { files = []; }
      for (const fname of files) {
        if (!fname.endsWith('.jsonl')) continue;
        transcriptFiles++;
        const fallbackId = fname.replace('.jsonl', '');
        if (skipIds.has(fallbackId)) continue;
        let text;
        try { text = readFileSync(join(transcriptsDir, fname), 'utf8'); } catch { continue; }
        const parsed = parseCopilotTranscriptJsonl(text, fallbackId);
        if (!parsed.observations.length) continue;
        if (skipIds.has(parsed.sessionId)) continue;
        skipIds.add(parsed.sessionId);
        sessions.push({ ...parsed, project, tags: ['copilot-transcript-import'] });
      }
    }

    // 4b: old chatSessions JSON
    const chatSessionsDir = join(wsDir, 'chatSessions');
    if (existsSync(chatSessionsDir)) {
      let files;
      try { files = readdirSync(chatSessionsDir); } catch { files = []; }
      for (const fname of files) {
        if (!fname.endsWith('.json')) continue;
        chatSessionFiles++;
        const fallbackId = fname.replace('.json', '');
        if (skipIds.has(fallbackId)) continue;
        let text;
        try { text = readFileSync(join(chatSessionsDir, fname), 'utf8'); } catch { continue; }
        const parsed = parseCopilotChatSessionJson(text, fallbackId);
        if (!parsed || !parsed.observations.length) continue;
        if (skipIds.has(parsed.sessionId)) continue;
        skipIds.add(parsed.sessionId);
        sessions.push({ ...parsed, project, tags: ['copilot-chatsession-import'] });
      }
    }
  }

  return { newTranscripts: transcriptFiles, newChatSessions: chatSessionFiles, sessions };
}

// ─── AgentMemory REST ─────────────────────────────────────────────────────────

async function fetchServerSessionIds(cfg) {
  try {
    const res = await fetch(`${cfg.url}/agentmemory/sessions`, {
      headers: { authorization: `Bearer ${cfg.secret}` },
      signal: AbortSignal.timeout(15_000),
    });
    if (!res.ok) return new Set();
    const data = await res.json();
    return new Set((data.sessions || []).map((s) => s.id));
  } catch {
    return new Set();
  }
}

async function postImport(cfg, exportData) {
  const res = await fetch(`${cfg.url}/agentmemory/import`, {
    method: 'POST',
    headers: {
      'content-type': 'application/json',
      authorization: `Bearer ${cfg.secret}`,
    },
    body: JSON.stringify({ exportData, strategy: 'skip' }), // skip = safe dedup on server too
    signal: AbortSignal.timeout(300_000),
  });
  const text = await res.text();
  let json = {};
  try { json = text ? JSON.parse(text) : {}; } catch {
    throw new Error(`Non-JSON response (${res.status}): ${text.slice(0, 300)}`);
  }
  if (!res.ok || json.success === false) {
    throw new Error(json.error || `Import failed HTTP ${res.status}: ${text.slice(0, 300)}`);
  }
  return json;
}

function buildExportData(sessions) {
  const observations = {};
  const sessionRecords = sessions.map((s) => {
    observations[s.sessionId] = s.observations;
    return {
      id: s.sessionId,
      project: s.project,
      cwd: s.cwd,
      startedAt: s.startedAt,
      endedAt: s.endedAt,
      status: 'completed',
      observationCount: s.observations.length,
      tags: s.tags || [],
      firstPrompt: s.observations.find((o) => o.userPrompt)?.userPrompt?.slice(0, 200),
    };
  });
  return { version: VERSION, exportedAt: new Date().toISOString(), sessions: sessionRecords, observations, memories: [], summaries: [] };
}

function chunkSessions(all, size = 30) {
  const chunks = [];
  for (let i = 0; i < all.length; i += size) chunks.push(all.slice(i, i + size));
  return chunks;
}

// ─── Main ─────────────────────────────────────────────────────────────────────

async function main() {
  const cfg = loadConfig();
  console.log(`Target: ${cfg.url}`);

  const health = await fetch(`${cfg.url}/agentmemory/health`, {
    headers: { authorization: `Bearer ${cfg.secret}` },
  }).then((r) => r.json());
  console.log(`Server: ${health.status} (${health.version || 'v?'}, ${health.health?.workers?.[0]?.version || ''})`);

  // Pre-fetch server's known session IDs to skip on re-run (dedup)
  process.stdout.write('Fetching server session list for dedup… ');
  const skipIds = await fetchServerSessionIds(cfg);
  console.log(`${skipIds.size} sessions already on server (will be skipped)`);

  const all = [];

  // Step 1: Claude JSONL
  const claudeDir = join(homedir(), '.claude/projects');
  if (existsSync(claudeDir)) {
    const { files, sessions } = importJsonlDir(claudeDir, 'claude-jsonl-import', skipIds);
    all.push(...sessions);
    console.log(`Step 1 (Claude JSONL):  ${files} files scanned → ${sessions.length} new sessions`);
  } else {
    console.log('Step 1 (Claude JSONL):  ~/.claude/projects not found, skipped');
  }

  // Step 2: Cursor agent-transcript JSONL
  const cursorDir = join(homedir(), '.cursor/projects');
  if (existsSync(cursorDir)) {
    const { files, sessions } = importJsonlDir(cursorDir, 'cursor-jsonl-import', skipIds);
    all.push(...sessions);
    console.log(`Step 2 (Cursor JSONL):  ${files} files scanned → ${sessions.length} new sessions`);
  } else {
    console.log('Step 2 (Cursor JSONL):  ~/.cursor/projects not found, skipped');
  }

  // Step 3: Cursor sidebar bubbles from state.vscdb
  console.log('Step 3 (Cursor sidebar): extracting bubbles from state.vscdb…');
  const sidebar = loadSidebarSessions(skipIds);
  all.push(...sidebar);
  const sidebarObs = sidebar.reduce((n, s) => n + s.observations.length, 0);
  console.log(`Step 3 (Cursor sidebar): ${sidebar.length} new sessions (${sidebarObs} observations)`);

  // Step 4: VS Code GitHub Copilot sessions (new JSONL transcripts + old chatSessions JSON)
  const { newTranscripts, newChatSessions, sessions: copilotSessions } = loadCopilotSessions(skipIds);
  all.push(...copilotSessions);
  const copilotTranscripts = copilotSessions.filter((s) => s.tags?.includes('copilot-transcript-import'));
  const copilotChatSess = copilotSessions.filter((s) => s.tags?.includes('copilot-chatsession-import'));
  const copilotObs = copilotSessions.reduce((n, s) => n + s.observations.length, 0);
  console.log(`Step 4 (Copilot):        ${newTranscripts} transcript files, ${newChatSessions} chatSession files scanned → ${copilotTranscripts.length} new transcript sessions + ${copilotChatSess.length} new chatSession sessions (${copilotObs} observations)`);

  if (!all.length) {
    console.log('\nNothing new to import — all sessions already on server.');
    return;
  }

  const totalObs = all.reduce((n, s) => n + s.observations.length, 0);
  console.log(`\nImporting ${all.length} sessions, ${totalObs} observations in batches…`);

  let importedSessions = 0;
  let importedObs = 0;
  for (const [i, chunk] of chunkSessions(all, 30).entries()) {
    const chunkObs = chunk.reduce((n, s) => n + s.observations.length, 0);
    process.stdout.write(`  batch ${i + 1}: ${chunk.length} sessions, ${chunkObs} obs… `);
    const result = await postImport(cfg, buildExportData(chunk));
    importedSessions += result.stats?.sessions ?? chunk.length;
    importedObs += result.stats?.observations ?? chunkObs;
    console.log('ok');
  }

  const final = await fetchServerSessionIds(cfg);
  console.log('\nDone.');
  console.log(`  Imported this run:   ~${importedSessions} sessions, ~${importedObs} observations`);
  console.log(`  Server total:        ${final.size} sessions`);
  console.log(`  Viewer:              ${cfg.url.replace(':3111', ':3113')} → Replay tab`);
}

main().catch((err) => {
  console.error('Import failed:', err.message || err);
  process.exit(1);
});
