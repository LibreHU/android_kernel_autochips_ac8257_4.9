#!/usr/bin/env python3
"""res.py <both|ours|theirs> <file> [hunk numbers 1-based, default all] : resolves conflict markers in a file."""
import sys, re
mode, path = sys.argv[1], sys.argv[2]
only = {int(x) for x in sys.argv[3:]}
lines = open(path, encoding="utf-8", errors="surrogateescape").read().split("\n")
out = []; i = 0; k = 0
while i < len(lines):
    if lines[i].startswith("<<<<<<< "):
        k += 1
        j = i + 1; ours = []
        while not lines[j].startswith("======="): ours.append(lines[j]); j += 1
        j += 1; theirs = []
        while not lines[j].startswith(">>>>>>> "): theirs.append(lines[j]); j += 1
        if only and k not in only:
            out += [lines[i]] + ours + ["======="] + theirs + [lines[j]]
        else:
            out += {"both": ours + theirs, "ours": ours, "theirs": theirs}[mode]
        i = j + 1
    else:
        out.append(lines[i]); i += 1
open(path, "w", encoding="utf-8", errors="surrogateescape").write("\n".join(out))
print(path, k, "hunks")
