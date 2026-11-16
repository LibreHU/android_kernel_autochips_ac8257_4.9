#!/usr/bin/env python3
# Copyright (C) 2026 LibreHU
# SPDX-License-Identifier: GPL-2.0
"""Zips TWRP de la release d'un noyau LibreHU, a cote du module KernelSU de connectivite (CI du noyau).

    tools/twrp/make_twrp_zips.py <dist> <branche> <version>

Lit dans <dist> boot_ac8257.img, boot_ac8257_bootmenu.img et librehu_connectivity_ksu.zip (les memes six modules que
le module KernelSU, et le /proc/version de leur noyau) et ecrit :
  - librehu_kernel_twrp.zip : noyau (boot_ac8257_bootmenu.img, menu de demarrage) + modules, pour la partition boot
    et /vendor/lib/modules ;
  - librehu_conn_modules_twrp.zip : modules seuls, quand la partition boot contient deja ce noyau.
Meme contenu et memes verifications que les zips kernel-stable / conn-modules-stable de LibreHU-flashablesZips
(update-binary = twrp-common.sh + conn-modules.sh + corps du zip) ; archives reproductibles (dates fixes).
"""
import hashlib
import os
import struct
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
MODULES = ["wmt_drv", "wmt_chrdev_wifi", "bt_drv", "gps_drv", "fmradio_drv", "wlan_drv_gen4m"]
DEVICES = "ac8257_demo"
DATE = (2020, 1, 1, 0, 0, 0)


def kernel_sum(data, name):
    """Empreinte de la section noyau d'une image de boot Android v0/v1 (apres l'en-tete, kernel_size octets)."""
    if data[:8] != b"ANDROID!":
        sys.exit(f"{name} : pas une image de boot Android")
    ksize, page = struct.unpack_from("<I", data, 8)[0], struct.unpack_from("<I", data, 36)[0]
    return hashlib.sha256(data[page:page + ksize]).hexdigest()


# Version d'Android de /system (apres mount_rw) contre la variante du noyau (fichier files/android du zip)
ANDROID_CHECK = r'''
android_check() {
	_want=$(cat $F/android)
	_prop=$(sysdir)/build.prop
	[ -f "$_prop" ] || abort "build.prop de /system introuvable : version d'Android inconnue, rien n'a ete modifie"
	_rel=$(grep -m1 '^ro.build.version.release=' "$_prop" | cut -d= -f2)
	_maj=${_rel%%.*}
	case "$_want" in
	a9) [ "$_maj" = 9 ] || abort "noyau Android 9 (a9) mais /system = Android $_rel : prendre la variante a10";;
	a10) [ -n "$_maj" ] && [ "$_maj" -ge 10 ] 2>/dev/null || abort "noyau Android 10+ (a10) mais /system = Android $_rel : prendre la variante a9";;
	*) abort "variante inconnue : $_want";;
	esac
	ui_print "Android $_rel : variante $_want, OK"
}
'''


def update_binary(zid, version, body):
    common = open(os.path.join(HERE, "twrp-common.sh")).read() + "\n" + open(os.path.join(HERE, "conn-modules.sh")).read() \
        + ANDROID_CHECK
    check = ('DEV=$(getprop ro.product.device)\n'
             f'case " {DEVICES} " in *" $DEV "*) ;; *) abort "appareil \\"$DEV\\" non pris en charge ({DEVICES})";; esac\n')
    head = f'#!/sbin/sh\nOUTFD=$2; ZIPFILE=$3; ZIP_ID={zid}; VERSION="{version}"\n'
    return head + common + "\n" + check + open(os.path.join(HERE, body)).read()


def write_zip(path, zid, version, body, files):
    with zipfile.ZipFile(path, "w") as z:
        for name, data, mode in [("META-INF/com/google/android/update-binary", update_binary(zid, version, body), 0o755),
                                 ("META-INF/com/google/android/updater-script", "# voir update-binary\n", 0o755)] + \
                                [("files/" + n, d, 0o644) for n, d in sorted(files.items())]:
            info = zipfile.ZipInfo(name, date_time=DATE)
            info.external_attr = mode << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, data)


def main():
    if len(sys.argv) != 5 or sys.argv[4] not in ("a9", "a10"):
        sys.exit(__doc__)
    dist, branch, version, variant = sys.argv[1:]
    images = {n: open(os.path.join(dist, n), "rb").read()
              for n in (f"boot_ac8257_{variant}.img", f"boot_ac8257_bootmenu_{variant}.img")}
    ksu = zipfile.ZipFile(os.path.join(dist, f"librehu_connectivity_ksu_{variant}.zip"))
    files, sums = {}, []
    for m in MODULES:
        data = ksu.read(f"modules/{m}.ko")
        if data[:4] != b"\x7fELF":
            sys.exit(f"{m}.ko : pas un module ELF")
        files[f"modules/{m}.ko"] = data
        sums.append(f"{hashlib.sha256(data).hexdigest()}  {m}.ko")
    files["modules/SHA256SUMS"] = "\n".join(sums) + "\n"
    kv = ksu.read("kernel_version").decode().strip()
    if not kv.startswith("Linux version "):
        sys.exit("kernel_version absent du module KernelSU")
    files["kernel_version"] = kv + "\n"
    files["kernels.sha256"] = "".join(f"{kernel_sum(d, n)}  {n}\n" for n, d in images.items())
    files["branch"] = branch + "\n"
    files["android"] = variant + "\n"
    write_zip(os.path.join(dist, f"librehu_conn_modules_twrp_{variant}.zip"), "librehu_conn_modules", version,
              "modules-install.sh", files)
    boot = images[f"boot_ac8257_bootmenu_{variant}.img"]
    files["boot.img"] = boot
    files["boot.img.sha256"] = f"{hashlib.sha256(boot).hexdigest()}  boot.img\n"
    write_zip(os.path.join(dist, f"librehu_kernel_twrp_{variant}.zip"), "librehu_kernel", version, "kernel-install.sh",
              files)


if __name__ == "__main__":
    main()
