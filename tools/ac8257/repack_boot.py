#!/usr/bin/env python3
"""
Puts a kernel built from this tree into the stock UJC201 boot.img, keeping everything else (ramdisk,
cmdline, load addresses, header v1 recovery DTBO, OS version) byte for byte.

    tools/ac8257/repack_boot.py <stock boot.img> <out/arch/arm64/boot/Image.gz-dtb> <new boot.img>

Android boot image header v0/v1 (system/tools/mkbootimg). The id field is the SHA-1 of the sections,
recomputed as mkbootimg does. The result must not be larger than the boot partition (10 MiB on the
UJC201): the script refuses otherwise.
"""
import hashlib
import struct
import sys

MAGIC = b"ANDROID!"
BOOT_PARTITION = 10 * 1024 * 1024


def pad(data, page):
    return data + b"\0" * ((page - len(data) % page) % page)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    stock = open(sys.argv[1], "rb").read()
    kernel = open(sys.argv[2], "rb").read()
    if stock[:8] != MAGIC:
        sys.exit("not an Android boot image")
    (k_size, k_addr, r_size, r_addr, s_size, s_addr, tags, page, version, os_version) = struct.unpack_from("<10I", stock, 8)
    if version > 1:
        sys.exit("header v%d not handled (stock UJC201 uses v1)" % version)
    name = stock[48:64]
    cmdline = stock[64:576]
    extra = stock[608:1632]
    o = page
    sections = []
    for size in (k_size, r_size, s_size):
        sections.append(stock[o:o + size])
        o += (size + page - 1) // page * page
    recovery_dtbo = b""
    rd_offset = 0
    if version == 1:
        rd_size, rd_offset, hdr_size = struct.unpack_from("<IQI", stock, 1632)
        recovery_dtbo = stock[o:o + rd_size]
    _, ramdisk, second = sections

    sha = hashlib.sha1()
    for blob in (kernel, ramdisk, second) + ((recovery_dtbo,) if version == 1 else ()):
        sha.update(blob)
        sha.update(struct.pack("<I", len(blob)))
    digest = sha.digest() + b"\0" * 12

    header = MAGIC + struct.pack("<10I", len(kernel), k_addr, len(ramdisk), r_addr, len(second), s_addr, tags, page, version, os_version)
    header += name + cmdline + digest + extra
    if version == 1:
        header += struct.pack("<IQI", len(recovery_dtbo), rd_offset, 1648)
    out = pad(header, page) + pad(kernel, page) + pad(ramdisk, page) + (pad(second, page) if second else b"")
    if version == 1 and recovery_dtbo:
        out += pad(recovery_dtbo, page)
    if len(out) > BOOT_PARTITION:
        sys.exit("boot image %d bytes > boot partition %d bytes" % (len(out), BOOT_PARTITION))
    open(sys.argv[3], "wb").write(out)
    print("kernel %d -> %d bytes, ramdisk %d bytes, cmdline %r, image %d bytes"
          % (k_size, len(kernel), len(ramdisk), cmdline.rstrip(b"\0").decode(errors="replace"), len(out)))


if __name__ == "__main__":
    main()
