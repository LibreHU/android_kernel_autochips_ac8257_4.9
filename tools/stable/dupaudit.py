#!/usr/bin/env python3
"""tools/stable/dupaudit.py [base] : flags applied stable commits whose added blocks (>=2 meaningful lines) were already in the file before them."""
import subprocess, re, sys, json
K = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True).stdout.strip(); BASE = sys.argv[1] if len(sys.argv) > 1 else "origin/LibreHU-main"
def g(*a): return subprocess.run(["git", *a], cwd=K, capture_output=True, text=True, errors="replace").stdout
TRIV = re.compile(r"^[\s{}();,]*$|^\s*(return\s*\w*;|break;|else|#endif.*|\*/|/\*|\*)\s*$")
commits = g("rev-list", "--reverse", f"{BASE}..HEAD").split()
flag = []
for c in commits:
    body = g("log", "-1", "--format=%B", c)
    if "linux-stable commit" not in body: continue
    diff = g("diff", "-U0", f"{c}^", c)
    cur = None; blocks = {}
    for line in diff.splitlines():
        if line.startswith("+++ "): cur = line[6:] if line.startswith("+++ b/") else None; continue
        if line.startswith("--- ") or line.startswith("@@"):
            if cur: blocks.setdefault(cur, []).append([])
            continue
        if cur and line.startswith("+"):
            if not blocks.get(cur): blocks.setdefault(cur, []).append([])
            blocks[cur][-1].append(line[1:])
    for f, bl in blocks.items():
        if not f.endswith((".c", ".h", ".S", "Makefile", "Kconfig")): continue
        pre = None
        for b in bl:
            meaningful = [l.strip() for l in b if l.strip() and not TRIV.match(l)]
            if len(meaningful) < 1: continue
            if pre is None:
                pre = g("show", f"{c}^:{f}")
                prenorm = "\n".join(l.strip() for l in pre.splitlines() if l.strip())
            seg = "\n".join(l.strip() for l in b if l.strip())
            if not (seg and seg in prenorm): continue
            m = re.search(r"linux-stable commit ([0-9a-f]{40})", body)
            up = g("show", f"{m.group(1)}:{f}") if m else ""
            upn = "\n".join(l.strip() for l in up.splitlines() if l.strip())
            ours = "\n".join(l.strip() for l in g("show", f"{c}:{f}").splitlines() if l.strip())
            if up and ours.count(seg) > upn.count(seg):
                flag.append((c[:10], f, len(meaningful), g("log", "-1", "--format=%s", c).strip(), meaningful[0][:60]))
for x in flag: print("\t".join(map(str, x)))
print(len(flag), "flagged of", len(commits), file=sys.stderr)
