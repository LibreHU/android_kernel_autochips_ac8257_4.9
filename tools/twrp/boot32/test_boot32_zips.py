#!/usr/bin/env python3
# Copyright (C) 2026 LibreHU
# SPDX-License-Identifier: GPL-2.0
"""Simulation of librehu_boot32_twrp.zip and librehu_boot32_restore_twrp.zip on a synthetic eMMC image (GPT of
test_gpt32.py, fake boot / dtbo / tee / vendor ext4 contents, no data of any unit), with power cuts.

    tools/twrp/boot32/test_boot32_zips.py <dist with the two zips> <gpt32 built for this machine>

Needs e2fsprogs (mkfs.ext4, e2fsck, resize2fs, tune2fs, debugfs) and about 8 GB of free space (sparse files).
Scenarios: refusal without VERIFICATION_DISABLED; cut before vendor / before the GPT / between the two GPT writes
during the migration, then repair or end; refusal of a kernel over 10 MiB; cut before vendor and between the GPT
writes during the return; final state = original GPT byte for byte, contents equal, vendor clean with its file.
"""
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_gpt32 as G  # noqa: E402

MIB = 1 << 20
TWRP = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


def sh(*a, **k):
    return subprocess.run(list(a), check=True, capture_output=True, text=True, **k).stdout


def boot_image(size):
    """Fake boot image of <size> bytes inside a 10 MiB image ending with an AVB footer (as repack_boot.py)."""
    img = bytearray(os.urandom(size)) + bytearray(10 * MIB - size)
    voff = (size + 4095) // 4096 * 4096
    img[voff:voff + 4] = b"AVB0"
    img[-64:] = b"AVBf" + struct.pack(">IIQQQ", 1, 0, size, voff, 1024) + bytes(28)
    return bytes(img)


class Sim:
    def __init__(self, d, dist, gpt32):
        self.d, self.dist, self.gpt32 = d, dist, gpt32
        self.disk = os.path.join(d, "disk.img")
        self.sd = os.path.join(d, "sd")
        self.bin = os.path.join(d, "bin")
        os.makedirs(self.bin)
        with open(os.path.join(self.bin, "getprop"), "w") as f:
            f.write("#!/bin/sh\necho ac8257_demo\n")
        os.chmod(os.path.join(self.bin, "getprop"), 0o755)
        for n, flags in (("vbmeta_stock", 0), ("vbmeta_off", 3)):
            v = bytearray(4096); v[:4] = b"AVB0"; struct.pack_into(">I", v, 120, flags)
            open(os.path.join(d, n), "wb").write(v)
        self.ub = {}
        for z in ("librehu_boot32_twrp.zip", "librehu_boot32_restore_twrp.zip"):
            self.ub[z] = os.path.join(d, z + ".sh")
            open(self.ub[z], "w").write(sh("unzip", "-p", os.path.join(dist, z), "META-INF/com/google/android/update-binary"))

    def put(self, mib, data):
        with open(self.disk, "r+b") as f:
            f.seek(mib * MIB); f.write(data)

    def get(self, mib, n):
        with open(self.disk, "rb") as f:
            f.seek(mib * MIB); return f.read(n)

    def setup(self):
        self.ent = G.entries(G.ORIG)
        G.make_disk(self.disk, self.ent)
        self.boot = boot_image(9 * MIB)
        self.dtbo = bytearray(os.urandom(52599)) + bytearray(8 * MIB - 52599)
        self.dtbo[:8] = bytes.fromhex("d7b7ab1e") + struct.pack(">I", 52599)
        self.tee1 = b"\x88\x16\x88\x58" + os.urandom(5 * MIB - 4)
        self.tee2 = b"\x88\x16\x88\x58" + os.urandom(7 * MIB - 4)
        root = os.path.join(self.d, "vroot"); os.makedirs(os.path.join(root, "lib"))
        self.blob = os.urandom(20 * MIB)
        open(os.path.join(root, "lib", "blob.ko"), "wb").write(self.blob)
        v = os.path.join(self.d, "vendor.ext4")
        with open(v, "wb") as f:
            f.truncate(2048 * MIB)
        sh("mkfs.ext4", "-q", "-F", "-b", "4096", "-L", "vendor", "-d", root, v)
        for mib, data in ((498, self.boot), (508, bytes(self.dtbo)), (516, self.tee1), (521, self.tee2)):
            self.put(mib, data)
        sh("dd", f"if={v}", f"of={self.disk}", "bs=1M", "seek=528", "conv=notrunc,sparse", "status=none")
        os.remove(v)
        self.gpt0 = (G.read(self.disk, 0, 34), G.read(self.disk, G.SECTORS - 33, 33))
        self.h0 = self.digest()

    def digest(self):
        h = hashlib.sha256()
        with open(self.disk, "rb") as f:
            for _ in range(0, 2580):               # first 2580 MiB: GPT, boot..vendor
                h.update(f.read(MIB))
        h.update(G.read(self.disk, G.SECTORS - 33, 33))
        return h.hexdigest()

    def vendor_dev(self, mib, size):
        """by-name/vendor of the running kernel: a copy of the region (old table until TWRP restarts)."""
        p = os.path.join(self.d, "vendor_dev.img")
        sh("dd", f"if={self.disk}", f"of={p}", "bs=1M", f"skip={mib}", f"count={size}", "conv=sparse", "status=none")
        return p

    def run(self, zipname, vbmeta="vbmeta_off", cut=None):
        st = self.state()
        old = st.startswith("state=orig")
        env = dict(os.environ, PATH=self.bin + ":" + os.environ["PATH"], UJC_TEST="1", UJC_DISK=self.disk,
                   UJC_VENDOR=self.vendor_dev(528 if old else 543, 2048 if old else 2033),
                   UJC_VBMETA=os.path.join(self.d, vbmeta), UJC_SDCARD=self.sd, UJC_TMP=os.path.join(self.d, "tmp"),
                   UJC_GPT32=self.gpt32, UJC_FAIL_AT=cut or "")
        os.makedirs(self.sd, exist_ok=True)
        r = subprocess.run(["sh", self.ub[zipname], "3", "", os.path.join(self.dist, zipname)], env=env,
                           capture_output=True, text=True)
        os.remove(env["UJC_VENDOR"])
        return r.returncode, r.stdout + r.stderr

    def state(self):
        return subprocess.run([self.gpt32, "state", self.disk], capture_output=True, text=True).stdout

    def check_new(self, boot):
        assert self.state().startswith("state=new backup=ok"), self.state()
        assert self.get(530, MIB) == bytes(self.dtbo[:MIB])
        assert self.get(531, 5 * MIB) == self.tee1 and self.get(536, 7 * MIB) == self.tee2
        assert self.get(498, 10 * MIB - 64) == boot[:-64]
        assert self.get(529, MIB)[-64:] == boot[-64:], "AVB footer not at the end of the 32 MiB boot"
        self.check_vendor(543, 2033)

    def check_vendor(self, mib, size):
        v = self.vendor_dev(mib, size)
        assert subprocess.run(["e2fsck", "-fn", v], capture_output=True).returncode == 0, "vendor e2fsck"
        out = os.path.join(self.d, "blob.out")
        sh("debugfs", "-R", f"dump lib/blob.ko {out}", v)
        assert open(out, "rb").read() == self.blob, "vendor file changed"
        bc = [l for l in sh("tune2fs", "-l", v).splitlines() if l.startswith("Block count")][0]
        os.remove(v); os.remove(out)
        return int(bc.split(":")[1])

    def check_orig(self, boot):
        assert self.state().startswith("state=orig backup=ok"), self.state()
        assert (G.read(self.disk, 0, 34), G.read(self.disk, G.SECTORS - 33, 33)) == self.gpt0, "GPT != original"
        assert self.get(508, 8 * MIB) == bytes(self.dtbo)
        assert self.get(516, 5 * MIB) == self.tee1 and self.get(521, 7 * MIB) == self.tee2
        assert self.get(498, 10 * MIB) == boot
        assert self.check_vendor(528, 2048) == 2048 * MIB // 4096

    def flash10(self, img):
        """10 MiB kernel written on the 32 MiB boot partition by flash_image (zips TWRP 4.19)."""
        part = os.path.join(self.d, "boot.part")
        open(part, "wb").write(self.get(498, 32 * MIB))
        z = os.path.join(self.d, "k10.zip"); fd = os.path.join(self.d, "k10")
        os.makedirs(os.path.join(fd, "files"), exist_ok=True)
        open(os.path.join(fd, "files", "boot.img"), "wb").write(img)
        open(os.path.join(fd, "files", "boot.img.sha256"), "w").write(hashlib.sha256(img).hexdigest() + "  boot.img\n")
        if os.path.exists(z):
            os.remove(z)
        sh("zip", "-q", "-r", z, "files", cwd=fd)
        script = os.path.join(self.d, "flash.sh")
        open(script, "w").write('. "$1"\nflash_image boot.img boot\n')
        env = dict(os.environ, UJC_TEST="1", ZIPFILE=z, UJC_PART_boot=part, UJC_TMP=os.path.join(self.d, "tmp"),
                   UJC_SDCARD=os.path.join(self.d, "nosd"))
        sh("sh", script, os.path.join(TWRP, "twrp-common.sh"), env=env)
        self.put(498, open(part, "rb").read())


def expect(rc_out, rc, *texts):
    r, out = rc_out
    assert r == rc, f"rc {r} != {rc}\n{out}"
    for t in texts:
        assert t in out, f"{t!r} missing\n{out}"


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    dist, gpt32 = (os.path.abspath(a) for a in sys.argv[1:])
    I, R = "librehu_boot32_twrp.zip", "librehu_boot32_restore_twrp.zip"
    with tempfile.TemporaryDirectory() as d:
        s = Sim(d, dist, gpt32)
        s.setup()
        expect(s.run(I, vbmeta="vbmeta_stock"), 1, "VERIFICATION_DISABLED requis")
        assert s.digest() == s.h0, "refusal wrote to the disk"
        expect(s.run(R), 0, "rien a faire")
        for cut in ("before_vendor", "before_gpt"):
            expect(s.run(I, cut=cut), 3, "coupure simulee : " + cut)
            assert s.state().startswith("state=orig")
            expect(s.run(I), 1, "sauvegardes existantes")
            expect(s.run(R), 0, "Vendor d'origine remis")
            assert s.digest() == s.h0, "repair after a cut %s: disk differs" % cut
            shutil.rmtree(s.sd)
        print("migration cut + repair: OK")
        expect(s.run(I, cut="between_gpt"), 3, "between_gpt")
        assert s.state().startswith("state=new backup=bad")
        expect(s.run(I), 0, "fin de la migration", "GPT de secours recalculee")
        s.check_new(s.boot)
        expect(s.run(I), 0, "deja 32 Mio")
        print("migration cut between the GPT writes + end: OK")
        # kernel over 10 MiB in place: refused; then a 10 MiB kernel flashed by a 4.19 TWRP zip
        big = bytearray(32 * MIB); big[:11 * MIB] = os.urandom(11 * MIB)
        big[-64:] = b"AVBf" + struct.pack(">IIQQQ", 1, 0, 11 * MIB, 11 * MIB, 1024) + bytes(28)
        s.put(498, bytes(big))
        expect(s.run(R), 1, "ne tient pas dans 10 Mio")
        k10 = boot_image(8 * MIB)
        s.flash10(k10)
        expect(s.run(R, cut="before_vendor"), 3, "before_vendor")
        expect(s.run(R), 0, "Reprise : copies verifiees", "(orig)")
        s.check_orig(k10)
        print("return cut before vendor + resume: OK")
        shutil.rmtree(s.sd)
        expect(s.run(I), 0, "(new)")
        s.check_new(k10)
        expect(s.run(R, cut="between_gpt"), 3, "between_gpt")
        assert s.state().startswith("state=orig backup=bad")
        expect(s.run(R), 0, "fin du retour")
        s.check_orig(k10)
        assert not os.path.exists(os.path.join(s.sd, "LibreHU", "boot32")), "boot32 folder left"
        print("return cut between the GPT writes + end: OK")
    print("boot32 zips: all scenarios OK")


if __name__ == "__main__":
    main()
