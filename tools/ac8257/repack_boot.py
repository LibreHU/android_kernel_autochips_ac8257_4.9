#!/usr/bin/env python3
"""
Puts a kernel built from this tree into the stock UJC201 boot.img, keeping everything else (ramdisk,
cmdline, load addresses, header v1 recovery DTBO, OS version) byte for byte.

    tools/ac8257/repack_boot.py <stock boot.img> <out/arch/arm64/boot/Image.gz-dtb> <new boot.img>
                                [--vbmeta <vbmeta blob>] [--size <partition bytes>] [--cmdline-append <args>]

Android boot image header v0/v1 (system/tools/mkbootimg). The id field is the SHA-1 of the sections,
recomputed as mkbootimg does.

AVB footer: the stock boot partition ends with an AVB footer ("AVBf", last 64 bytes) pointing to its
own vbmeta (boot is a chained partition of the stock vbmeta, Jancar key). Without it the LK refuses
the image, unlocked or not, and the unit reboots at the logo with no kernel log. The stock vbmeta is
put back after the new image (aligned to 4 KiB) and the footer rewritten with the new image size, as
tools/bootmenu/mkboot.py of the device tree does: right key, only the hash differs, which an
unlocked LK and Android's fs_mgr tolerate. The output fills the whole partition (10 MiB for boot).

To test in the recovery partition instead (32 MiB, so that a crash ends with a normal boot rather
than a boot loop), give the stock recovery vbmeta (prebuilt/avb/recovery_stock_vbmeta_250718.bin of
the device tree) and --size 33554432. In recovery mode the LK does not add "init=/init" (it does for a
normal boot, with skip_initramfs): without a ramdisk the kernel then mounts system as root but finds no
init (/init is not in the kernel's default list); --cmdline-append "init=/init" starts Android's init.
"""
import argparse
import hashlib
import struct
import sys

MAGIC = b"ANDROID!"


def pad(data, page):
    return data + b"\0" * ((page - len(data) % page) % page)


def avb_footer(img):
    """(original image size, vbmeta blob) of an AVB footer, or None."""
    foot = img[-64:]
    if foot[:4] != b"AVBf":
        return None
    _, _, _, orig, voff, vsz = struct.unpack(">4sIIQQQ", foot[:36])
    return orig, img[voff:voff + vsz]


def main():
    ap = argparse.ArgumentParser(usage=__doc__)
    ap.add_argument("stock")
    ap.add_argument("kernel")
    ap.add_argument("out")
    ap.add_argument("--vbmeta", help="vbmeta blob for the AVB footer (default: the stock image's own)")
    ap.add_argument("--size", type=int, help="partition size (default: size of the stock image)")
    ap.add_argument("--cmdline-append", default="", help="appended to the header command line")
    a = ap.parse_args()
    stock = open(a.stock, "rb").read()
    kernel = open(a.kernel, "rb").read()
    footer = avb_footer(stock)
    part = a.size or len(stock)
    vbmeta = open(a.vbmeta, "rb").read() if a.vbmeta else (footer[1] if footer else None)
    if footer:
        stock = stock[:footer[0]]
    if stock[:8] != MAGIC:
        sys.exit("not an Android boot image")
    (k_size, k_addr, r_size, r_addr, s_size, s_addr, tags, page, version, os_version) = struct.unpack_from("<10I", stock, 8)
    if version > 1:
        sys.exit("header v%d not handled (stock UJC201 uses v1)" % version)
    name = stock[48:64]
    cmdline = stock[64:576]
    if a.cmdline_append:
        text = cmdline.rstrip(b"\0") + b" " + a.cmdline_append.encode()
        if len(text) >= 512:
            sys.exit("command line too long for the header (512 bytes)")
        cmdline = text.ljust(512, b"\0")
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
    size = len(out)
    if vbmeta:
        if vbmeta[:4] != b"AVB0":
            sys.exit("not a vbmeta blob")
        voff = (size + 4095) // 4096 * 4096
        if voff + len(vbmeta) + 64 > part:
            sys.exit("image %d bytes + vbmeta do not fit in the partition (%d bytes)" % (size, part))
        img = bytearray(part)
        img[:size] = out
        img[voff:voff + len(vbmeta)] = vbmeta
        img[-64:] = b"AVBf" + struct.pack(">IIQQQ", 1, 0, size, voff, len(vbmeta)) + b"\0" * 28
        out = bytes(img)
    elif size > part:
        sys.exit("boot image %d bytes > partition %d bytes" % (size, part))
    else:
        print("warning: no AVB footer (the stock UJC201 LK refuses such a boot image)")
    open(a.out, "wb").write(out)
    print("kernel %d -> %d bytes, ramdisk %d bytes, cmdline %r, image %d bytes, AVB footer: %s, output %d bytes"
          % (k_size, len(kernel), len(ramdisk), cmdline.rstrip(b"\0").decode(errors="replace"), size,
             "yes" if vbmeta else "no", len(out)))


if __name__ == "__main__":
    main()
