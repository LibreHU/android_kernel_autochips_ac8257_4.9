#!/usr/bin/env python3
"""tools/stable/compiled.py . arch/arm64/configs/ac8257_ujc201_defconfig tools/stable/compiled.json
Approximate set of source files compiled with our defconfig: walks Makefiles from the top-level dirs, following
obj-y / obj-$(CONFIG_X)=y entries (files and sub-directories), composite objects (foo-y, foo-objs, foo-$(CONFIG_X))."""
import os, re, sys, json
K = sys.argv[1]; CFG = sys.argv[2]
conf = {}
for l in open(CFG):
    m = re.match(r"(CONFIG_\w+)=(.*)", l)
    if m: conf[m.group(1)] = m.group(2).strip().strip('"')
def on(tok):
    tok = tok.strip()
    if tok in ("y", ""): return True
    m = re.fullmatch(r"\$\((CONFIG_\w+)\)", tok)
    return bool(m) and conf.get(m.group(1)) in ("y", "m")
compiled = set(); dirs_done = set()
def parse(d):
    if d in dirs_done: return
    dirs_done.add(d)
    mk = None
    for n in ("Kbuild", "Makefile"):
        if os.path.isfile(os.path.join(K, d, n)): mk = os.path.join(K, d, n); break
    if not mk: return
    txt = open(mk, errors="replace").read().replace("\\\n", " ")
    comp = {}
    lines = txt.splitlines()
    var = {}
    for l in lines:
        m = re.match(r"\s*([A-Za-z_][\w\-]*?)(-y|-\$\((CONFIG_\w+)\))?\s*(:=|\+=|=)\s*(.*)", l)
        if not m: continue
        name = m.group(1)
        if m.group(2):
            if not on(m.group(2)[1:]): continue
            name += "-y"
        if m.group(4) == "+=":
            var.setdefault(name, []).extend(m.group(5).split())
        else:
            var[name] = m.group(5).split()
    def expand(toks, depth=0):
        out = []
        for t in toks:
            mm = re.fullmatch(r"\$\(([\w\-]+)\)", t)
            if mm and depth < 5:
                out.extend(expand(var.get(mm.group(1), []), depth + 1))
            else:
                out.append(t)
        return out
    for l in lines:
        m = re.match(r"\s*([\w\-]+)-(y|objs|\$\(CONFIG_\w+\))\s*[:+]?=\s*(.*)", l)
        if m and m.group(1) not in ("obj", "lib", "ccflags", "asflags", "subdir", "always", "extra", "targets", "hostprogs", "head", "core", "libs", "drivers", "init", "net", "virt", "clean"):
            if on(m.group(2)) or m.group(2) == "objs":
                comp.setdefault(m.group(1), []).extend(m.group(3).split())
    for l in lines:
        m = re.match(r"\s*(obj|lib|core|drivers|net|libs|init|virt|head)-(y|\$\(CONFIG_\w+\))\s*[:+]?=\s*(.*)", l)
        if not m or not on(m.group(2)): continue
        for t in expand(m.group(3).split()):
            if t.startswith("$(") : continue
            if t.endswith("/"):
                parse(os.path.normpath(os.path.join(d, t)))
            elif t.endswith(".o") or t.endswith(".a"):
                base = t[:-2]
                def add(b, depth=0):
                    if b in comp and depth < 3:
                        for s in comp[b]:
                            if s.endswith(".o"): add(s[:-2], depth + 1)
                    for ext in (".c", ".S"):
                        p = os.path.normpath(os.path.join(d, b + ext))
                        if os.path.isfile(os.path.join(K, p)): compiled.add(p)
                add(base)
for top in ("init", "kernel", "mm", "fs", "ipc", "security", "crypto", "block", "drivers", "sound", "net", "lib", "arch/arm64/kernel", "arch/arm64/mm", "arch/arm64/lib", "arch/arm64/crypto", "arch/arm64/kvm", "arch/arm64/net", "virt", "certs", "usr"):
    parse(top)
json.dump(sorted(compiled), open(sys.argv[3], "w"))
print(len(compiled), "compiled sources;", len(dirs_done), "dirs")
