#!/usr/bin/env python3
# Copyright (C) 2026 LibreHU
# SPDX-License-Identifier: GPL-2.0-only
"""Rename one NEEDED library of an ELF shared object in place, to a name of the same length.

    tools/gpu/rename_needed.py <elf> <old name> <new name>

Only the bytes of that string in .dynstr change: no section, segment, offset or hash table moves. Refused unless:
the names have the same length; the file is a little-endian ELF32 / ELF64 shared object; one DT_NEEDED entry of
.dynamic names <old>; that string occurs once in .dynstr; no other dynamic entry, dynamic symbol, version need or
version definition refers to a string inside it (a suffix shared by the string table). Prints the offset changed.
"""
import struct
import sys


def fail(msg):
    sys.exit(f"rename_needed: {msg}")


def main():
    path, old, new = sys.argv[1], sys.argv[2].encode(), sys.argv[3].encode()
    if len(old) != len(new) or not old or old == new:
        fail("the names must differ and have the same length")
    data = bytearray(open(path, "rb").read())
    if data[:4] != b"\x7fELF" or data[5] != 1:
        fail("not a little-endian ELF file")
    is64 = data[4] == 2
    if struct.unpack_from("<H", data, 16)[0] != 3:
        fail("not a shared object (ET_DYN)")
    if is64:
        shoff, = struct.unpack_from("<Q", data, 0x28)
        shentsize, shnum = struct.unpack_from("<HH", data, 0x3A)
    else:
        shoff, = struct.unpack_from("<I", data, 0x20)
        shentsize, shnum = struct.unpack_from("<HH", data, 0x2E)
    sections = []
    for i in range(shnum):
        o = shoff + i * shentsize
        if is64:
            name, typ, _, _, off, size, link, info, _, entsize = struct.unpack_from("<IIQQQQIIQQ", data, o)
        else:
            name, typ, _, _, off, size, link, info, _, entsize = struct.unpack_from("<IIIIIIIIII", data, o)
        sections.append(dict(type=typ, off=off, size=size, link=link, info=info, entsize=entsize))
    dyn = [s for s in sections if s["type"] == 6]  # SHT_DYNAMIC
    if len(dyn) != 1:
        fail("no single .dynamic section")
    dyn = dyn[0]
    dynstr = sections[dyn["link"]]
    if dynstr["type"] != 3:  # SHT_STRTAB
        fail(".dynamic does not link to a string table")
    s0, s1 = dynstr["off"], dynstr["off"] + dynstr["size"]
    strtab = bytes(data[s0:s1])

    refs = []  # string offsets referenced from anywhere in the dynamic linking data
    needed = []
    esize = 16 if is64 else 8
    string_tags = {1, 14, 15, 29}  # DT_NEEDED, DT_SONAME, DT_RPATH, DT_RUNPATH
    for o in range(dyn["off"], dyn["off"] + dyn["size"], esize):
        tag, val = struct.unpack_from("<qQ" if is64 else "<iI", data, o)
        if tag == 0:
            break
        if tag in string_tags:
            refs.append(val)
            if tag == 1 and strtab[val:val + len(old) + 1] == old + b"\0":
                needed.append(val)
    if len(needed) != 1:
        fail(f"{len(needed)} DT_NEEDED entries name {old.decode()} (one expected)")
    target = needed[0]
    if strtab.count(old + b"\0") != 1:
        fail(f"{old.decode()} occurs more than once in .dynstr")
    for s in sections:
        if s["type"] == 11 and s["link"] == dyn["link"]:  # SHT_DYNSYM
            es = 24 if is64 else 16
            for o in range(s["off"], s["off"] + s["size"], es):
                refs.append(struct.unpack_from("<I", data, o)[0])
        elif s["type"] == 0x6ffffffe and s["link"] == dyn["link"]:  # SHT_GNU_verneed
            o = s["off"]
            for _ in range(s["info"]):
                vn_version, vn_cnt, vn_file, vn_aux, vn_next = struct.unpack_from("<HHIII", data, o)
                refs.append(vn_file)
                a = o + vn_aux
                for _ in range(vn_cnt):
                    _, _, _, vna_name, vna_next = struct.unpack_from("<IHHII", data, a)
                    refs.append(vna_name)
                    a += vna_next
                if not vn_next:
                    break
                o += vn_next
        elif s["type"] == 0x6ffffffd and s["link"] == dyn["link"]:  # SHT_GNU_verdef
            o = s["off"]
            for _ in range(s["info"]):
                _, _, _, vd_cnt, _, vd_aux, vd_next = struct.unpack_from("<HHHHIII", data, o)
                a = o + vd_aux
                for _ in range(vd_cnt):
                    vda_name, vda_next = struct.unpack_from("<II", data, a)
                    refs.append(vda_name)
                    a += vda_next
                if not vd_next:
                    break
                o += vd_next
    inside = [r for r in refs if target < r < target + len(old)]
    if inside:
        fail(f"other strings share the bytes of {old.decode()} (offsets {inside})")
    data[s0 + target:s0 + target + len(new)] = new
    open(path, "wb").write(data)
    print(f"{path}: NEEDED {old.decode()} -> {new.decode()} (.dynstr+{target:#x})")


if __name__ == "__main__":
    main()
