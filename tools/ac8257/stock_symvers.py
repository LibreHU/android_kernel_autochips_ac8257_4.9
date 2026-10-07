#!/usr/bin/env python3
"""
Rebuilds the Module.symvers of the stock AC8257 kernel (exported symbols and their CONFIG_MODVERSIONS
CRCs), to build modules that load on the stock kernel, and to check this tree against it.

    tools/ac8257/stock_symvers.py <stock Image.gz-dtb or Image> <out Module.symvers> [tree Module.symvers]

How: the stock kernel is relocatable (CONFIG_RELOCATABLE, KASLR), so __ksymtab and __kcrctab are zero
in the image and filled at boot from the R_AARCH64_RELATIVE table. That table is found in the image,
__kcrctab is the run of relocations whose addends are 32-bit values (the CRCs), and __ksymtab is the run
of 16-byte {value, name} entries just before it, in the same order. The export type (EXPORT_SYMBOL or
_GPL) is not visible in the image: it is taken from this tree's Module.symvers when given, else
EXPORT_SYMBOL.

With a tree Module.symvers, also prints how many CRCs match: a matching CRC means the symbol's prototype
and the types it uses are the same as in the stock kernel.
"""
import struct
import sys
import zlib

BASE = 0xFFFFFF8008080000  # KIMAGE_VADDR + TEXT_OFFSET (arm64, 39-bit VA)
R_AARCH64_RELATIVE = 0x403


def load_image(path):
    raw = open(path, "rb").read()
    if raw[:2] == b"\x1f\x8b":
        return zlib.decompressobj(16 + zlib.MAX_WBITS).decompress(raw)
    return raw


def find_rela(img):
    """Longest run of 24-byte RELATIVE relocations targeting kernel addresses."""
    best = (0, 0)
    pos = 0
    n = len(img) - 24
    while pos < n:
        off, info, _ = struct.unpack_from("<QQQ", img, pos)
        if info == R_AARCH64_RELATIVE and off >> 32 == 0xFFFFFF80:
            start = pos
            while pos < n:
                off, info, _ = struct.unpack_from("<QQQ", img, pos)
                if info != R_AARCH64_RELATIVE and info != 0:
                    break
                pos += 24
            if pos - start > best[1] - best[0]:
                best = (start, pos)
        else:
            pos += 8
    return best


def cstring(img, va):
    o = va - BASE
    if not 0 <= o < len(img):
        return None
    e = img.find(b"\0", o, o + 256)
    return img[o:e].decode("ascii", "replace") if e > o else None


def main():
    if len(sys.argv) not in (3, 4):
        sys.exit(__doc__)
    img = load_image(sys.argv[1])
    a, b = find_rela(img)
    rel = {}
    for p in range(a, b, 24):
        off, info, add = struct.unpack_from("<QQQ", img, p)
        if info == R_AARCH64_RELATIVE:
            rel[off] = add
    # __kcrctab: longest run of 8-byte slots whose addend fits in 32 bits.
    small = sorted(o for o, v in rel.items() if v < 2 ** 32)
    best, start, prev = (0, 0), small[0], small[0]
    for o in small[1:] + [None]:
        if o is not None and o - prev == 8:
            prev = o
            continue
        if (prev - start) // 8 + 1 > best[1]:
            best = (start, (prev - start) // 8 + 1)
        if o is not None:
            start = prev = o
    crc_start, count = best
    ksym_start = crc_start - 16 * count
    tree = {}
    if len(sys.argv) == 4:
        for line in open(sys.argv[3]):
            f = line.split("\t")
            if len(f) >= 4:
                tree[f[1]] = (f[0], f[3].strip())
    out = []
    missing_names = 0
    for i in range(count):
        name = cstring(img, rel.get(ksym_start + 16 * i + 8, 0))
        crc = rel.get(crc_start + 8 * i, 0) & 0xFFFFFFFF
        if not name:
            missing_names += 1
            continue
        out.append((name, crc))
    with open(sys.argv[2], "w") as f:
        for name, crc in out:
            kind = tree.get(name, (None, "EXPORT_SYMBOL"))[1]
            f.write("0x%08x\t%s\tvmlinux\t%s\n" % (crc, name, kind))
    print("stock exports: %d (unnamed: %d)" % (len(out), missing_names))
    if tree:
        same = sum(1 for n, c in out if n in tree and int(tree[n][0], 16) == c)
        differ = sorted(n for n, c in out if n in tree and int(tree[n][0], 16) != c)
        absent = sorted(n for n, c in out if n not in tree)
        print("same CRC: %d, different CRC: %d, not exported by this tree: %d" % (same, len(differ), len(absent)))
        with open(sys.argv[2] + ".diff", "w") as f:
            f.write("# different CRC (prototype or types differ from the stock kernel)\n")
            f.writelines(n + "\n" for n in differ)
            f.write("# exported by the stock kernel only\n")
            f.writelines(n + "\n" for n in absent)


if __name__ == "__main__":
    main()
