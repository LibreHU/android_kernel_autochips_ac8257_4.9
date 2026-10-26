#!/usr/bin/env python3
"""Applies linux-stable commits in order onto the current branch with git am -3 (docs/STABLE.md).
  tools/stable/apply.py <last stable commit (Linux 4.9.N)> <log.tsv>
Needs the linux-stable commits in this repository (git fetch stable ...) and tools/stable/compiled.json
(tools/stable/compiled.py).
Resumable: commits already carrying 'linux-stable commit <hash>' in the branch are skipped.
Stops (exit 2) on a conflict in a file compiled with our defconfig; non-compiled conflicts keep our version."""
import json, os, re, subprocess, sys
K = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True).stdout.strip()
END, LOG = sys.argv[1], sys.argv[2]
compiled = set(json.load(open(os.path.join(K, "tools/stable/compiled.json"))))
cdirs = {os.path.dirname(f) for f in compiled}
DENY = ("include/xen", "include/linux/ceph", "include/net/bluetooth", "include/linux/mlx", "include/rdma", "include/linux/kvm", "include/kvm", "include/net/sctp", "include/drm", "include/linux/sunrpc", "include/linux/nfs")
def rel(f):
    if f in compiled: return True
    if f.endswith(".h") and not f.startswith(DENY):
        d = os.path.dirname(f)
        return d in cdirs or d in ("include/linux", "include/uapi/linux", "include/net", "include/asm-generic", "include/uapi/asm-generic") or f.startswith("arch/arm64/include")
    return f in ("Makefile",)
def g(*a, inp=None, check=False):
    r = subprocess.run(["git", *a], cwd=K, capture_output=True, text=True, input=inp)
    if check and r.returncode: sys.exit(f"git {' '.join(a)}: {r.stderr}")
    return r
done = set(re.findall(r"linux-stable commit ([0-9a-f]{40})", g("log", "--format=%B", "--grep=linux-stable commit").stdout))
todo = g("rev-list", "--reverse", END).stdout.split()
# Decisions already logged in this batch or in an earlier one (docs/stable/*.tsv): "already present", "ignored"
# and "not taken" commits leave no commit in the branch
logged = set()
for t in {LOG} | {os.path.join(K, "docs/stable", f) for f in os.listdir(os.path.join(K, "docs/stable")) if f.endswith(".tsv")}:
    if os.path.exists(t):
        logged |= {l.split("\t")[0] for l in open(t)}
log = open(LOG, "a")
def note(h, what, detail=""):
    s = g("log", "-1", "--format=%s", h).stdout.strip()
    log.write(f"{h[:12]}\t{what}\t{s}\t{detail}\n"); log.flush()
n = 0
for h in todo:
    if h in done or h[:12] in logged: continue
    parents = g("log", "-1", "--format=%P", h).stdout.split()
    if not parents:  # shallow boundary: its parent is v4.9.117
        diff = g("diff", "--binary", "stable/v4.9.117", h).stdout
        r = g("apply", "--3way", "--index", "-", inp=diff)
        if r.returncode: sys.exit(f"boundary {h}: {r.stderr}")
        msg = g("log", "-1", "--format=%B", h).stdout.rstrip() + f"\n\n(linux-stable commit {h})\n"
        env = dict(os.environ)
        for k, f in (("GIT_AUTHOR_NAME", "%an"), ("GIT_AUTHOR_EMAIL", "%ae"), ("GIT_AUTHOR_DATE", "%aD")):
            env[k] = g("log", "-1", f"--format={f}", h).stdout.strip()
        subprocess.run(["git", "commit", "-q", "-F", "-"], cwd=K, input=msg, text=True, env=env, check=True)
        note(h, "appliqué"); n += 1; continue
    patch = g("format-patch", "-1", "--stdout", "--no-signature", "-k", h).stdout
    patch = re.sub(r"\n---\n", f"\n\n(linux-stable commit {h})\n---\n", patch, count=1)
    before = g("rev-parse", "HEAD").stdout.strip()
    # no rename detection: it moved fixes for drivers absent here (spi-mt65xx.c) onto vendor ones (spi-ac82xx.c)
    r = g("-c", "merge.renames=false", "am", "-3", "-k", "--keep-cr", inp=patch)
    if r.returncode == 0:
        if g("rev-parse", "HEAD").stdout.strip() == before:
            note(h, "déjà présent (identique)", "git am : patch déjà appliqué"); continue
        other = set(g("diff-tree", "--no-commit-id", "--name-only", "-r", "--no-renames", "HEAD").stdout.split()) - \
            set(g("diff-tree", "--no-commit-id", "--name-only", "-r", "--no-renames", h).stdout.split())
        if other:
            note(h, "appliqué"); print(f"STOP {h[:12]} applied to files the original does not touch: {sorted(other)}"); sys.exit(3)
        note(h, "appliqué"); n += 1; continue
    out = r.stdout + r.stderr
    st = g("diff", "--name-only", "--diff-filter=U").stdout.split()
    if not st and ("No changes" in out or "already applied" in out or "patch does not apply" not in out and not g("diff", "--cached", "--name-only").stdout.strip()):
        g("am", "--skip"); note(h, "déjà présent (identique)", "git am : aucun changement"); continue
    if st and all(rel(f) for f in st) is not None:
        auto = []
        for f in st:
            path = os.path.join(K, f)
            try: txt = open(path, encoding="utf-8", errors="surrogateescape").read()
            except FileNotFoundError: auto = None; break
            hunks = re.findall(r"<<<<<<< [^\n]*\n(.*?)=======\n(.*?)>>>>>>> [^\n]*\n", txt, re.S)
            outside = re.sub(r"<<<<<<< [^\n]*\n.*?>>>>>>> [^\n]*\n", "", txt, flags=re.S)
            have = {l.strip() for l in outside.splitlines() if l.strip()} | {l.strip() for o, t in hunks for l in o.splitlines() if l.strip()}
            if not hunks or not all(all(l.strip() in have for l in t.splitlines() if l.strip()) for o, t in hunks):
                auto = None; break
            auto.append((path, re.sub(r"<<<<<<< [^\n]*\n(.*?)=======\n.*?>>>>>>> [^\n]*\n", lambda m: m.group(1), txt, flags=re.S)))
        if auto:
            for path, txt in auto: open(path, "w", encoding="utf-8", errors="surrogateescape").write(txt)
            g("add", "--", *st)
            if not g("diff", "--cached", "--name-only").stdout.strip():
                g("am", "--skip"); note(h, "déjà présent (forme Android/MediaTek)", "auto : lignes amont déjà dans " + " ".join(st)); continue
            r2 = g("am", "--continue")
            if r2.returncode == 0:
                note(h, "appliqué (conflit auto : lignes amont déjà présentes)", " ".join(st)); n += 1; continue
    if st and not any(rel(f) for f in st):
        for f in st:
            if g("cat-file", "-e", f"HEAD:{f}").returncode == 0:
                g("checkout", "HEAD", "--", f)
            else:
                g("rm", "-q", "--cached", f); 
                if os.path.exists(os.path.join(K, f)): os.remove(os.path.join(K, f))
        if not g("diff", "--cached", "--name-only").stdout.strip() and not g("diff", "HEAD", "--name-only", "--", *st).stdout.strip():
            g("am", "--skip"); note(h, "ignoré (conflit hors code compilé)", " ".join(st)); continue
        g("add", "-A", "--", *st)
        r2 = g("am", "--continue")
        if r2.returncode == 0:
            note(h, "appliqué en partie (conflit hors code compilé, notre version gardée)", " ".join(st)); n += 1; continue
    print(f"STOP {h[:12]} conflicts={st}\n{out[-1500:]}")
    sys.exit(2)
print("done, applied", n)
