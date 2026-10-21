#!/usr/bin/env python3
"""
Boot image with the boot menu of the device tree (tools/ac8257/bootmenu: TWRP / Android / Fastboot at each start,
Android after 3 s) in front of this kernel, as tools/bootmenu/mkboot.py of the device tree does with the stock kernel.

    tools/ac8257/bootmenu_image.py <out/arch/arm64/boot/Image> <dtb> <stock boot template> <boot_bootmenu.img>
                                   [--init tools/ac8257/bootmenu/bootmenu] [--size 10485760] [--zopfli]

1. kernel: "skip_initramfs" -> "want_initramfs" (for a normal boot the LK passes skip_initramfs, which makes the
   kernel ignore the boot ramdisk), compressed again (gzip -9, or zopfli with pigz -11 for --zopfli), DTB appended;
2. ramdisk (cpio newc, gzip -9): /init = bootmenu, /dev/console, /proc, /sys, /dev, /system_root;
3. boot image and AVB footer: tools/ac8257/repack_boot.py --ramdisk.
"""
import argparse
import gzip
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def newc(entries):
    """cpio newc archive of (inode, name, mode, data, rdev major, rdev minor)."""
    out = bytearray()
    for ino, name, mode, data, rmaj, rmin in entries + [(0, b"TRAILER!!!", 0, b"", 0, 0)]:
        nlink = 2 if mode & 0o040000 else 1
        fields = (ino, mode, 0, 0, nlink, 0, len(data), 0, 0, rmaj, rmin, len(name) + 1, 0)
        out += b"070701" + b"".join(b"%08X" % f for f in fields) + name + b"\0"
        out += b"\0" * (-len(out) % 4)
        out += data
        out += b"\0" * (-len(out) % 4)
    return bytes(out)


def main():
    ap = argparse.ArgumentParser(usage=__doc__)
    ap.add_argument("image")
    ap.add_argument("dtb")
    ap.add_argument("template")
    ap.add_argument("out")
    ap.add_argument("--init", default=os.path.join(HERE, "bootmenu", "bootmenu"))
    ap.add_argument("--size", type=int, default=10485760)
    ap.add_argument("--zopfli", action="store_true", help="compress the kernel with pigz -11 (zopfli)")
    a = ap.parse_args()

    raw = open(a.image, "rb").read()
    n = raw.count(b"skip_initramfs\0")
    if n != 1:
        sys.exit("skip_initramfs found %d times (1 expected)" % n)
    raw = raw.replace(b"skip_initramfs\0", b"want_initramfs\0")
    if a.zopfli:
        kgz = subprocess.run(["pigz", "-11", "-n", "-c"], input=raw, stdout=subprocess.PIPE, check=True).stdout
    else:
        kgz = gzip.compress(raw, 9, mtime=0)
    kernel = kgz + open(a.dtb, "rb").read()

    init = open(a.init, "rb").read()
    if init[:4] != b"\x7fELF" or init[18] != 0xB7:
        sys.exit("the boot menu must be an aarch64 ELF")
    ramdisk = gzip.compress(
        newc(
            [
                (300001, b"dev", 0o040755, b"", 0, 0),
                (300002, b"dev/console", 0o020600, b"", 5, 1),
                (300003, b"proc", 0o040755, b"", 0, 0),
                (300004, b"sys", 0o040755, b"", 0, 0),
                (300005, b"system_root", 0o040755, b"", 0, 0),
                (300006, b"init", 0o100750, init, 0, 0),
            ]
        ),
        9,
        mtime=0,
    )
    print("kernel (want_initramfs) %d bytes, ramdisk %d bytes (boot menu %d)" % (len(kernel), len(ramdisk), len(init)))
    with tempfile.TemporaryDirectory() as d:
        k = os.path.join(d, "Image.gz-dtb")
        r = os.path.join(d, "ramdisk.cpio.gz")
        open(k, "wb").write(kernel)
        open(r, "wb").write(ramdisk)
        subprocess.run(
            [sys.executable, os.path.join(HERE, "repack_boot.py"), a.template, k, a.out, "--size", str(a.size), "--ramdisk", r],
            check=True,
        )


if __name__ == "__main__":
    main()
