#!/usr/bin/env python3
"""Scan git staged files, all tracked files, a directory tree, or a single file.
Sanitize mode strips BLOCK findings in-place, preserving surrounding structure.

Usage:
  python3 scan.py staged             # scan staged changes (default)
  python3 scan.py tracked            # scan all tracked files in cwd repo
  python3 scan.py dir <path>         # scan all files under a directory tree
  python3 scan.py <filepath>         # scan a specific file
  python3 scan.py sanitize <filepath> # redact BLOCK findings in-place

Exit codes (scan modes):
  0 = CLEAN
  1 = BLOCK (hard credentials — do not commit)
  2 = WARN  (review required, may be intentional)
"""

import re
import sys
import subprocess
from pathlib import Path

# Hard credentials — exit 1, do not commit
BLOCK_PATTERNS = [
    (r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY', 'private key'),
    (r'github_pat_[A-Za-z0-9_]{36,}', 'GitHub fine-grained PAT'),
    (r'ghp_[A-Za-z0-9]{36,}', 'GitHub classic PAT'),
    (r'ghs_[A-Za-z0-9]{36,}', 'GitHub Actions token'),
    (r'glpat-[A-Za-z0-9_-]{20,}', 'GitLab token'),
    (r'AKIA[0-9A-Z]{16}', 'AWS access key ID'),
    (r'npm_[A-Za-z0-9]{36,}', 'npm token'),
    # JSON-format: "KEY_WITH_SECRET/TOKEN/PASSWORD": "hexvalue"
    (r'(?i)["\'][^"\']*(?:secret|token|password|api[_-]?key)[^"\']*["\']:\s*["\'][A-Fa-f0-9]{40,}["\']', 'JSON hex credential'),
    # Assignment format: key = value or key: value (non-JSON)
    (r'(?i)(?:secret|token|password|api.key)\s*[=:]\s*["\']?[A-Fa-f0-9]{40,}["\']?', 'long hex credential'),
    # Non-placeholder quoted password value
    (r'(?i)password\s*=\s*["\'][^<>${{}}\'"\s]{10,}["\']', 'hardcoded password'),
]

# Sanitize substitutions — parallel to BLOCK_PATTERNS, preserve surrounding structure
# Each is (detect_pattern, replacement_with_backrefs).
# Order matters: more specific patterns first.
SANITIZE_PATTERNS = [
    (r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY.*', '-----BEGIN <REDACTED PRIVATE KEY>'),
    (r'(github_pat_)[A-Za-z0-9_]{36,}', r'\1<REDACTED>'),
    (r'(ghp_)[A-Za-z0-9]{36,}', r'\1<REDACTED>'),
    (r'(ghs_)[A-Za-z0-9]{36,}', r'\1<REDACTED>'),
    (r'(glpat-)[A-Za-z0-9_-]{20,}', r'\1<REDACTED>'),
    (r'(AKIA)[0-9A-Z]{16}', r'\1<REDACTED>'),
    (r'(npm_)[A-Za-z0-9]{36,}', r'\1<REDACTED>'),
    # JSON-format: preserve key name and quotes, replace only the hex value
    (r'(?i)(["\'][^"\']*(?:secret|token|password|api[_-]?key)[^"\']*["\']:\s*["\'])[A-Fa-f0-9]{40,}(["\'])', r'\1<REDACTED>\2'),
    # Assignment format: preserve key and separator
    (r'(?i)((?:secret|token|password|api.key)\s*[=:]\s*["\']?)[A-Fa-f0-9]{40,}(["\']?)', r'\1<REDACTED>\2'),
    # Quoted password: preserve key and quotes
    (r'(?i)(password\s*=\s*["\'])[^<>${{}}\'"\s]{10,}(["\'])', r'\1<REDACTED>\2'),
]

# Needs review — exit 2
WARN_PATTERNS = [
    (r'192\.168\.\d{1,3}\.\d{1,3}', 'private IP (192.168.x.x)'),
    (r'10\.\d{1,3}\.\d{1,3}\.\d{1,3}', 'private IP (10.x.x.x)'),
    (r'172\.(?:1[6-9]|2[0-9]|3[01])\.\d{1,3}\.\d{1,3}', 'private IP (172.16-31.x.x)'),
    (r'(?i)Authorization:\s*["\']?(?:Bearer|Basic|Token) ', 'auth header value'),
    (r'Bearer [A-Za-z0-9._\-]{20,}', 'Bearer token'),
]

# Line-level suppressors — skip line if any matches
OK_LINE_PATTERNS = [
    r'<[A-Za-z][A-Za-z0-9_-]*>',           # <placeholder>
    r'\$[A-Z_][A-Z0-9_]{2,}',              # $ENV_VAR
    r'\{\{.*?\}\}',                         # {{template}}
    r'your.(?:password|token|key|secret)',  # obvious placeholder text
    r'example\.(com|org|net)',              # example domain
]

# Paths where private IPs are known-intentional (lab topology docs)
LAB_IP_PATHS = ('skills/', 'wiki/', 'agents/', 'SKILL.md', 'README.md')


def is_ok_line(line: str, filepath: str = '') -> bool:
    if any(re.search(p, line) for p in OK_LINE_PATTERNS):
        return True
    return False


def suppress_warn(kind: str, filepath: str) -> bool:
    if 'private IP' in kind and any(seg in filepath for seg in LAB_IP_PATHS):
        return True
    return False


def scan_lines(lines: list[str], filepath: str) -> list[dict]:
    findings = []
    for n, line in enumerate(lines, 1):
        if is_ok_line(line, filepath):
            continue
        for pat, kind in BLOCK_PATTERNS:
            if re.search(pat, line):
                findings.append({
                    'sev': 'BLOCK', 'file': filepath, 'line': n,
                    'kind': kind, 'text': line.strip()[:120],
                })
                break  # one BLOCK per line is enough
        for pat, kind in WARN_PATTERNS:
            if re.search(pat, line) and not suppress_warn(kind, filepath):
                findings.append({
                    'sev': 'WARN', 'file': filepath, 'line': n,
                    'kind': kind, 'text': line.strip()[:120],
                })
    return findings


def read_staged(path: str) -> list[str]:
    r = subprocess.run(['git', 'show', f':{path}'], capture_output=True)
    if r.returncode != 0:
        return []  # deleted or unreadable
    try:
        return r.stdout.decode('utf-8').splitlines()
    except UnicodeDecodeError:
        return []  # binary


def read_disk(path: str) -> list[str]:
    try:
        return open(path, encoding='utf-8', errors='ignore').read().splitlines()
    except OSError:
        return []


def git_staged_files() -> list[str]:
    r = subprocess.run(['git', 'diff', '--cached', '--name-only'], capture_output=True, text=True)
    return [f for f in r.stdout.strip().splitlines() if f]


def git_tracked_files() -> list[str]:
    r = subprocess.run(['git', 'ls-files'], capture_output=True, text=True)
    return [f for f in r.stdout.strip().splitlines() if f]


def sanitize_lines(lines: list[str]) -> tuple[list[str], int]:
    """Apply SANITIZE_PATTERNS in-place. Returns (sanitized_lines, redaction_count)."""
    total = 0
    result = []
    for line in lines:
        new_line = line
        for pat, replacement in SANITIZE_PATTERNS:
            new_line, n = re.subn(pat, replacement, new_line)
            total += n
        result.append(new_line)
    return result, total


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else 'staged'

    if mode == 'sanitize':
        if len(sys.argv) < 3:
            print('Usage: scan.py sanitize <filepath>', file=sys.stderr)
            sys.exit(1)
        filepath = sys.argv[2]
        lines = read_disk(filepath)
        sanitized, count = sanitize_lines(lines)
        if count == 0:
            print(f'✓ Nothing to sanitize in {filepath}')
            sys.exit(0)
        Path(filepath).write_text('\n'.join(sanitized) + '\n', encoding='utf-8')
        print(f'✓ Sanitized {count} value(s) in {filepath} — replaced with <REDACTED>')
        remaining = [x for x in scan_lines(read_disk(filepath), filepath) if x['sev'] == 'BLOCK']
        if remaining:
            print(f'⚠  {len(remaining)} BLOCK finding(s) remain — manual review needed:')
            for f in remaining:
                print(f"  line {f['line']}: [{f['kind']}] {f['text']}")
            sys.exit(1)
        print('✓ Post-sanitize scan: CLEAN')
        sys.exit(0)

    if mode == 'staged':
        files = git_staged_files()
        get_lines = read_staged
        print(f"Scanning {len(files)} staged file(s)...")
    elif mode == 'tracked':
        files = git_tracked_files()
        get_lines = read_disk
        print(f"Scanning {len(files)} tracked file(s)...")
    elif mode == 'dir':
        root = sys.argv[2] if len(sys.argv) > 2 else '.'
        files = [str(p) for p in Path(root).rglob('*') if p.is_file()]
        get_lines = read_disk
        print(f"Scanning {len(files)} file(s) under {root}...")
    else:
        files = [mode]
        get_lines = read_disk
        print(f"Scanning {mode}...")

    all_findings: list[dict] = []
    for f in files:
        all_findings.extend(scan_lines(get_lines(f), f))

    blocks = [x for x in all_findings if x['sev'] == 'BLOCK']
    warns = [x for x in all_findings if x['sev'] == 'WARN']

    if not all_findings:
        print('✓ CLEAN — no sensitive data detected')
        sys.exit(0)

    if blocks:
        print(f'\n❌ BLOCK ({len(blocks)} finding(s)) — DO NOT COMMIT:')
        for f in blocks:
            print(f"  {f['file']}:{f['line']}  [{f['kind']}]")
            print(f"    {f['text']}")

    if warns:
        print(f'\n⚠  WARN ({len(warns)} finding(s)) — review before committing:')
        for f in warns:
            print(f"  {f['file']}:{f['line']}  [{f['kind']}]")
            print(f"    {f['text']}")

    sys.exit(1 if blocks else 2)


if __name__ == '__main__':
    main()
