#!/usr/bin/env python3
"""
Extracts the references used for the AC8257 reconstruction from the stock UJC201 images.

    tools/ac8257/extract_stock.py <Image.gz-dtb (boot.img kernel)> [dtbo.img] <out dir>

Writes into <out dir>:
  stock.config         kernel configuration (CONFIG_IKCONFIG, same as /proc/config.gz)
  stock.dtb / .dts     device tree appended to the kernel (needs dtc for the .dts)
  dtboN.dtb / .dts     overlays of dtbo.img (Android DTBO table)
  source-files.txt     kernel source files named in the stock kernel (__FILE__ of WARN/BUG and
                       friends), each tagged against this tree:
                         OK    same path here
                         REN   here under mt6761 / mt6765 / ... instead of ac8257
                         MISS  not in this tree
  version.txt          "Linux version" banner (compiler, builder, date)

Only the Python standard library is needed (dtc is optional).
"""
import gzip
import os
import re
import shutil
import struct
import subprocess
import sys
import zlib

TREE = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
FDT_MAGIC = b"\xd0\x0d\xfe\xed"
DTBO_MAGIC = 0xD7B7AB1E
IKCFG_START = b"IKCFG_ST"
IKCFG_END = b"IKCFG_ED"
SOURCE_PREFIX = re.compile(rb"kernel-4\.9/([A-Za-z0-9_./+-]+\.[chS])")
RENAMES = ["mt6761", "mt6765", "mt6762", "mt6763", "mt6739"]


def split_kernel(raw):
    """Returns (Image, appended dtb or None) from Image.gz-dtb or a plain Image."""
    if raw[:2] != b"\x1f\x8b":
        return raw, None
    d = zlib.decompressobj(16 + zlib.MAX_WBITS)
    image = d.decompress(raw)
    rest = d.unused_data
    dtb = None
    i = rest.find(FDT_MAGIC)
    if i >= 0:
        size = struct.unpack(">I", rest[i + 4:i + 8])[0]
        dtb = rest[i:i + size]
    return image, dtb


def ikconfig(image):
    s = image.find(IKCFG_START)
    e = image.find(IKCFG_END, s)
    if s < 0 or e < 0:
        return None
    return gzip.decompress(image[s + len(IKCFG_START):e]).decode()


def dtbo_entries(data):
    magic, _total, _hdr, esz, count, off = struct.unpack(">6I", data[:24])
    if magic != DTBO_MAGIC:
        raise ValueError("not an Android DTBO image")
    for i in range(count):
        size, offset = struct.unpack(">2I", data[off + i * esz:off + i * esz + 8])
        yield data[offset:offset + size]


def decompile(dtb_path):
    if shutil.which("dtc"):
        subprocess.run(["dtc", "-q", "-I", "dtb", "-O", "dts", "-o", dtb_path[:-4] + ".dts", dtb_path], check=False)


def tag(path):
    if os.path.exists(os.path.join(TREE, path)):
        return "OK  ", path
    for p in RENAMES:
        alt = path.replace("ac8257", p).replace("AC8257", p.upper())
        if alt != path and os.path.exists(os.path.join(TREE, alt)):
            return "REN ", "%s -> %s" % (path, alt)
    return "MISS", path


def main():
    args = sys.argv[1:]
    if len(args) not in (2, 3):
        sys.exit(__doc__)
    out = args[-1]
    os.makedirs(out, exist_ok=True)
    image, dtb = split_kernel(open(args[0], "rb").read())

    cfg = ikconfig(image)
    if cfg:
        open(os.path.join(out, "stock.config"), "w").write(cfg)
    m = re.search(rb"Linux version [^\n\x00]+", image)
    if m:
        open(os.path.join(out, "version.txt"), "w").write(m.group(0).decode(errors="replace") + "\n")
    if dtb:
        p = os.path.join(out, "stock.dtb")
        open(p, "wb").write(dtb)
        decompile(p)
    if len(args) == 3:
        for i, entry in enumerate(dtbo_entries(open(args[1], "rb").read())):
            p = os.path.join(out, "dtbo%d.dtb" % i)
            open(p, "wb").write(entry)
            decompile(p)

    files = sorted({f.decode() for f in SOURCE_PREFIX.findall(image)})
    tagged = [tag(f) for f in files]
    with open(os.path.join(out, "source-files.txt"), "w") as f:
        for t, line in tagged:
            f.write("%s %s\n" % (t.strip(), line))
    counts = {}
    for t, _ in tagged:
        counts[t.strip()] = counts.get(t.strip(), 0) + 1
    print("config: %s, dtb: %s, source files: %d %s" % (bool(cfg), bool(dtb), len(files), counts))


if __name__ == "__main__":
    main()
