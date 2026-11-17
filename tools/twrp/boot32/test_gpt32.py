#!/usr/bin/env python3
# Copyright (C) 2026 LibreHU
# SPDX-License-Identifier: GPL-2.0
"""Test of gpt32 on a synthetic disk (no data of any unit): GPT with the UJC201 layout around boot, random GUIDs.

    tools/twrp/boot32/test_gpt32.py <gpt32 binary> [runner...]     e.g. runner = qemu-aarch64-static

Checks: state orig; make grow -> only the 5 entries change, CRCs right, state new; make shrink -> the original
tables byte for byte; refusals (wrong direction, bad CRC, other partition in the moving area, bad backup reported).
"""
import os
import struct
import subprocess
import sys
import tempfile
import zlib

SECTORS = 244252672          # 128 GB eMMC of the UJC201 (any size works)
MIB = 2048
N, ES = 128, 128
ORIG = [("boot", 498, 10), ("dtbo", 508, 8), ("tee1", 516, 5), ("tee2", 521, 7), ("vendor", 528, 2048)]
NEW = [("boot", 498, 32), ("dtbo", 530, 1), ("tee1", 531, 5), ("tee2", 536, 7), ("vendor", 543, 2033)]
TYPE = bytes.fromhex("a2a0d0ebe5b9334487c068b6b72699c7")


def entry(name, start, last):
    return TYPE + os.urandom(16) + struct.pack("<QQQ", start, last, 0) + name.encode("utf-16le").ljust(72, b"\0")


def entries(layout):
    e = [entry("p%d" % i, (8 + 16 * i) * MIB, (16 + 16 * i) * MIB - 1) for i in range(25)]
    e += [entry(n, s * MIB, (s + l) * MIB - 1) for n, s, l in layout]
    e.append(entry("system", 2576 * MIB, (2576 + 3072) * MIB - 1))
    e += [entry("p%d" % i, (5648 + 64 * i) * MIB, (5680 + 64 * i) * MIB - 1) for i in range(31, 41)]
    return b"".join(e).ljust(N * ES, b"\0")


def header(cur, alt, ent_lba, ent):
    h = bytearray(struct.pack("<8sIIIIQQQQ16sQIII", b"EFI PART", 0x10000, 92, 0, 0, cur, alt, 34, SECTORS - 34,
                              os.urandom(16) if cur == 1 else bytes(16), ent_lba, N, ES, zlib.crc32(ent)))
    return h


def finish(h, guid=None):
    if guid:
        h[56:72] = guid
    struct.pack_into("<I", h, 16, 0)
    struct.pack_into("<I", h, 16, zlib.crc32(bytes(h[:92])))
    return bytes(h).ljust(512, b"\0")


def make_disk(path, ent):
    mbr = bytearray(512)
    mbr[446:462] = bytes([0, 0, 2, 0, 0xEE, 0xFF, 0xFF, 0xFF]) + struct.pack("<II", 1, 0xFFFFFFFF)
    mbr[510:512] = b"\x55\xaa"
    p = header(1, SECTORS - 1, 2, ent)
    pf = finish(p)
    b = bytearray(pf[:92])
    struct.pack_into("<QQ", b, 24, SECTORS - 1, 1)
    struct.pack_into("<Q", b, 72, SECTORS - 33)
    bf = finish(b)
    with open(path, "wb") as f:
        f.truncate(SECTORS * 512)
        f.write(bytes(mbr) + pf + ent)
        f.seek((SECTORS - 33) * 512)
        f.write(ent + bf)


def read(path, lba, n):
    with open(path, "rb") as f:
        f.seek(lba * 512)
        return f.read(n * 512)


def write(path, lba, data):
    with open(path, "r+b") as f:
        f.seek(lba * 512)
        f.write(data)


def check_tables(p, s, layout):
    h, e = p[512:604], p[1024:1024 + N * ES]
    hh = bytearray(h); struct.pack_into("<I", hh, 16, 0)
    assert zlib.crc32(bytes(hh)) == struct.unpack_from("<I", h, 16)[0], "primary header CRC"
    assert zlib.crc32(e) == struct.unpack_from("<I", h, 88)[0], "primary entries CRC"
    assert s[:N * ES] == e, "backup entries differ"
    b = s[N * ES:N * ES + 92]
    bb = bytearray(b); struct.pack_into("<I", bb, 16, 0)
    assert zlib.crc32(bytes(bb)) == struct.unpack_from("<I", b, 16)[0], "backup header CRC"
    assert struct.unpack_from("<QQ", b, 24) == (SECTORS - 1, 1) and struct.unpack_from("<Q", b, 72)[0] == SECTORS - 33
    for i, (n, st, l) in enumerate(layout):
        fs, ls = struct.unpack_from("<QQ", e, (25 + i) * ES + 32)
        assert (fs, ls) == (st * MIB, (st + l) * MIB - 1), n
    return e


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    tool = sys.argv[2:] + [sys.argv[1]]
    run = lambda *a: subprocess.run(tool + list(a), capture_output=True, text=True)
    with tempfile.TemporaryDirectory() as d:
        disk = os.path.join(d, "disk.img")
        ent = entries(ORIG)
        make_disk(disk, ent)
        p0, s0 = read(disk, 0, 34), read(disk, SECTORS - 33, 33)
        r = run("state", disk)
        assert r.returncode == 0 and r.stdout.startswith("state=orig backup=ok"), r.stdout + r.stderr
        r = run("make", disk, "shrink", d + "/x", d + "/y")
        assert r.returncode == 2 and "not the 32 MiB layout" in r.stderr, r.stderr
        r = run("make", disk, "grow", d + "/p", d + "/s")
        assert r.returncode == 0 and "sgpt_lba=%d pgpt_sectors=34 sgpt_sectors=33" % (SECTORS - 33) in r.stdout, r
        p, s = open(d + "/p", "rb").read(), open(d + "/s", "rb").read()
        assert len(p) == 34 * 512 and len(s) == 33 * 512 and p[:512] == p0[:512]
        e = check_tables(p, s, NEW)
        for i in list(range(25)) + list(range(30, N)):
            assert e[i * ES:(i + 1) * ES] == ent[i * ES:(i + 1) * ES], "entry %d changed" % i
        for i in range(25, 30):
            assert e[i * ES:i * ES + 32] == ent[i * ES:i * ES + 32] and e[i * ES + 48:(i + 1) * ES] == ent[i * ES + 48:(i + 1) * ES]
        write(disk, 0, p); write(disk, SECTORS - 33, s)
        r = run("state", disk)
        assert r.returncode == 0 and r.stdout.startswith("state=new backup=ok"), r.stdout
        r = run("make", disk, "shrink", d + "/p2", d + "/s2")
        assert r.returncode == 0, r.stderr
        assert open(d + "/p2", "rb").read() == p0 and open(d + "/s2", "rb").read() == s0, "shrink != original tables"
        # bad backup header is reported, not fatal; "same" rebuilds the backup of the current layout
        write(disk, SECTORS - 1, b"X")
        r = run("state", disk)
        assert r.returncode == 0 and "backup=bad" in r.stdout, r.stdout
        r = run("make", disk, "same", d + "/p3", d + "/s3")
        assert r.returncode == 0, r.stderr
        assert open(d + "/p3", "rb").read() == read(disk, 0, 34) and open(d + "/s3", "rb").read() == s
        write(disk, SECTORS - 33, s)
        assert run("state", disk).stdout.startswith("state=new backup=ok")
        write(disk, SECTORS - 1, b"X")
        # bad entries CRC
        write(disk, 2, bytes([ent[0] ^ 1]))
        r = run("state", disk)
        assert r.returncode == 2 and "CRC" in r.stderr, r.stderr
        # another partition inside 498..2576 MiB
        bad = bytearray(entries(ORIG)); struct.pack_into("<QQ", bad, 24 * ES + 32, 600 * MIB, 601 * MIB)
        make_disk(disk, bytes(bad))
        r = run("state", disk)
        assert r.returncode == 2 and "another partition" in r.stderr, r.stderr
        # unknown layout
        bad = bytearray(entries(ORIG)); struct.pack_into("<Q", bad, 26 * ES + 40, 515 * MIB)
        make_disk(disk, bytes(bad))
        r = run("state", disk)
        assert r.returncode == 1 and r.stdout.startswith("state=other"), r.stdout
    print("gpt32 tests OK (%s)" % " ".join(tool))


if __name__ == "__main__":
    main()
