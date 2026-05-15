#!/usr/bin/env python3
"""
Check all SKILL.md files for dead relative links.

Usage:
    python3 scripts/check-deadlinks.py [--root ~/.claude/skills]

Scans every SKILL.md in the skills tree, extracts relative markdown links,
resolves them against the SKILL.md's directory, and reports any that don't
resolve to an existing file.

Skips external URLs (http*), anchors (#), mailto:, and links inside code
blocks (fenced or inline).
"""

import re
import sys
from pathlib import Path

LINK_RE = re.compile(r'\[([^\]]*)\]\(([^)]*)\)')
CODE_FENCE_RE = re.compile(r'^```')

def extract_links(text: str) -> list[tuple[str, str, bool]]:
    """Extract (link_text, url, in_code_block) tuples, filtering code blocks."""
    lines = text.split('\n')
    in_fence = False
    results = []
    
    for i, line in enumerate(lines):
        if CODE_FENCE_RE.match(line.strip()):
            in_fence = not in_fence
            continue
        
        # Skip lines with inline code containing links (they're illustrative)
        if '`[' in line and '](' in line:
            continue
        
        for text, url in LINK_RE.findall(line):
            results.append((text, url, in_fence))
    
    return results


def check_skills(root: Path) -> list[tuple[Path, str, str, str, str]]:
    """
    Scan all SKILL.md files and return broken link reports.
    
    Returns list of (skill_rel_path, link_text, url, resolved_path, in_code_block)
    """
    broken = []
    
    for sf in sorted(root.rglob("SKILL.md")):
        try:
            text_content = sf.read_text(encoding='utf-8')
        except Exception as e:
            print(f"  SKIP {sf.relative_to(root)} — {e}", file=sys.stderr)
            continue
        
        skill_dir = sf.parent
        links = extract_links(text_content)
        
        for link_text, url, in_code in links:
            # Skip non-relative links
            if url.startswith(('http:', 'https:', '#', 'mailto:')):
                continue
            
            target = (skill_dir / url).resolve()
            
            if not target.exists():
                try:
                    rel = sf.relative_to(root)
                except ValueError:
                    rel = sf
                broken.append((
                    str(rel),
                    link_text,
                    url,
                    str(target),
                    in_code
                ))
    
    return broken


def main():
    root = Path("~/.claude/skills").expanduser()
    
    # Allow override via --root
    if len(sys.argv) > 2 and sys.argv[1] == '--root':
        root = Path(sys.argv[2]).expanduser()
    
    if not root.exists():
        print(f"Skills root not found: {root}", file=sys.stderr)
        sys.exit(1)
    
    print(f"Scanning {root}...")
    broken = check_skills(root)
    
    if not broken:
        print("\n✅ No dead links found.")
        return
    
    print(f"\n❌ {len(broken)} dead link(s) found:\n")
    
    real_broken = []
    code_block = []
    
    for rel, text, url, target, in_code in broken:
        if in_code:
            code_block.append((rel, text, url, target))
        else:
            real_broken.append((rel, text, url, target))
    
    if real_broken:
        print("=== ACTUAL BROKEN LINKS (outside code blocks) ===")
        for rel, text, url, target in real_broken:
            print(f"  ■ {rel}")
            print(f"    '{text}' → {url}")
            print(f"    Resolves to: {target}")
            print()
    
    if code_block:
        print("=== IN CODE BLOCKS (likely intentional examples) ===")
        for rel, text, url, target in code_block:
            print(f"  ■ {rel}  —  '{text}' → {url}")
    
    if real_broken:
        sys.exit(2)


if __name__ == "__main__":
    main()
