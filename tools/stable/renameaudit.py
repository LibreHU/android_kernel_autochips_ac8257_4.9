#!/usr/bin/env python3
"""tools/stable/renameaudit.py <base> : applied stable commits that changed files the original commit does not touch
(git am -3 rename detection can move a fix for an absent driver onto a vendor one)."""
import subprocess, re, sys
def g(*a): return subprocess.run(["git", *a], capture_output=True, text=True, errors="replace").stdout
out = g("log", "--format=%H%x00%B%x01", f"{sys.argv[1]}..HEAD")
for rec in out.split("\x01"):
    if "\x00" not in rec: continue
    h, body = rec.strip().split("\x00", 1)
    m = re.search(r"linux-stable commit ([0-9a-f]{40})", body)
    if not m: continue
    mine = set(g("diff-tree", "--no-commit-id", "--name-only", "-r", "--no-renames", h).split())
    orig = set(g("diff-tree", "--no-commit-id", "--name-only", "-r", "--no-renames", m.group(1)).split())
    extra = mine - orig
    if extra:
        print(h[:10], m.group(1)[:12], sorted(extra), g("log", "-1", "--format=%s", h).strip())
