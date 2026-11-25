#!/bin/bash
# Copyright (C) 2026 LibreHU
# SPDX-License-Identifier: GPL-2.0-only
# Essai des zips TWRP du paquet Vulkan hors autoradio (CI de la branche -vulkan), sur un faux vendor stock :
#   tools/gpu/test_twrp_zips.sh <dist>      (dist : librehu_gpu_ddk111_twrp.zip, librehu_gpu_ddk111_remove_twrp.zip)
# Les fichiers de reference du vendor (libs MTK, hwcomposer) sont des temoins : une copie d'essai du zip
# d'installation recoit leurs empreintes a la place de celles du vendor stock (files/unit_vendor.sha256), le reste du
# zip est celui de la release. Scenarios : installation, reinstallation, refus sans rien ecrire (lib de reference
# differente, ro.hardware.vulkan deja pose, pilote Vulkan deja la, dm-verity actif), retrait refuse sous un noyau
# DDK 1.11, retrait sous un autre noyau (vendor rendu identique a l'original).
set -e
DIST=$(realpath "$1")
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
V=$T/vendor
mkdir -p $T/bin $V $T/system/system/etc $T/sdcard $T/dt/firmware/android/fstab/vendor $T/tmp $T/zip
printf '#!/bin/sh\n[ "$1" = ro.product.device ] && echo ac8257_demo\n' > $T/bin/getprop; chmod +x $T/bin/getprop
echo 'ro.board.platform=ac8257' > $V/build.prop
echo 'ro.build.version.release=10' > $T/system/system/build.prop
printf 'wait' > $T/dt/firmware/android/fstab/vendor/fsmgr_flags
cd $T/zip && unzip -q "$DIST/librehu_gpu_ddk111_twrp.zip"
# fichiers du vendor stock que le paquet recouvre (temoins) et fichiers de reference (temoins, empreintes reecrites)
while read -r src dst; do mkdir -p "$(dirname "$V${dst#/vendor}")"; echo "stock $dst" > "$V${dst#/vendor}"; done < files/bound
: > files/unit_vendor.sha256.new
while read -r sum path; do
	f=$V${path#/vendor}; mkdir -p "$(dirname "$f")"; echo "reference $path" > "$f"
	echo "$(sha256sum "$f" | cut -d' ' -f1)  $path" >> files/unit_vendor.sha256.new
done < files/unit_vendor.sha256
mv files/unit_vendor.sha256.new files/unit_vendor.sha256
mkdir -p $V/firmware $V/etc/init $V/etc/permissions $V/odm/etc/vintf
echo fw > $V/firmware/rgx.fw.22.68.54.30
echo rc > $V/etc/init/android.hardware.graphics.allocator@2.0-service.rc
echo xml > $V/etc/permissions/handheld_core_hardware.xml
zip -q -r -X $T/install.zip META-INF files
(cd $V && find . | sort) > $T/vendor.before
head -c 1048576 /dev/zero > $T/boot_other.img
{ head -c 1048576 /dev/zero; printf 'librehu_gpu\0'; } > $T/boot_ddk111.img
unzip -q -o "$DIST/librehu_gpu_ddk111_remove_twrp.zip" META-INF/com/google/android/update-binary -d $T/rm
export PATH=$T/bin:$PATH UJC_TEST=1 UJC_VENMNT=$V UJC_SYSMNT=$T/system UJC_SDCARD=$T/sdcard UJC_DT=$T/dt \
	UJC_TMP=$T/tmp UJC_VBMETA=$T/none UJC_PART_boot=$T/boot_other.img
INSTALL="sh $T/zip/META-INF/com/google/android/update-binary 3 1 $T/install.zip"
REMOVE="sh $T/rm/META-INF/com/google/android/update-binary 3 1 $DIST/librehu_gpu_ddk111_remove_twrp.zip"
ok() { echo "OK   $1"; }
ko() { echo "FAIL $1"; exit 1; }
refused() {  # $1 = libelle, la commande ensuite : doit echouer et ne rien ecrire
	_l=$1; shift
	if "$@" > $T/out 2>&1; then cat $T/out; ko "$_l: accepted"; fi
	(cd $V && find . | sort) | cmp -s - $T/vendor.before || ko "$_l: the vendor changed"
	ok "$_l: refused, nothing written ($(grep ERROR $T/out | head -1))"
}

refused "other reference library" sh -c "echo x >> $V/lib64/libged.so; $INSTALL; r=\$?; sed -i '\$d' $V/lib64/libged.so; exit \$r"
refused "ro.hardware.vulkan already set" sh -c "echo ro.hardware.vulkan=x >> $T/system/system/build.prop; $INSTALL; r=\$?; sed -i '/ro.hardware.vulkan/d' $T/system/system/build.prop; exit \$r"
refused "Vulkan driver already there" sh -c "touch $V/lib64/hw/vulkan.ac8257.so; $INSTALL; r=\$?; rm $V/lib64/hw/vulkan.ac8257.so; exit \$r"
refused "dm-verity active" sh -c "printf wait,avb > $T/dt/firmware/android/fstab/vendor/fsmgr_flags; $INSTALL; r=\$?; printf wait > $T/dt/firmware/android/fstab/vendor/fsmgr_flags; exit \$r"

$INSTALL > $T/out 2>&1 || { cat $T/out; ko "install"; }
n=$(wc -l < $T/zip/files/MANIFEST)
[ "$(wc -l < $V/librehu/ddk111/MANIFEST)" = "$n" ] || ko "install: MANIFEST"
while read -r sum path; do [ "$(sha256sum "$V${path#/vendor}" | cut -d' ' -f1)" = "$sum" ] || ko "install: $path"; done < $T/zip/files/MANIFEST
while read -r src dst; do grep -qx "stock $dst" "$V${dst#/vendor}" || ko "install: stock $dst changed"; done < $T/zip/files/bound
grep -q '^on early-init && property:ro.boot.librehu_gpu=1.11$' $V/etc/init/librehu_gpu_ddk111.rc || ko "install: rc trigger"
[ "$(grep -c '^    mount none ' $V/etc/init/librehu_gpu_ddk111.rc)" = $(( $(wc -l < $T/zip/files/bound) + 1 )) ] || ko "install: rc mounts"
while read -r src dst; do grep -q "^    mount none $src $dst bind$" $V/etc/init/librehu_gpu_ddk111.rc || ko "install: rc $dst"; [ -f "$V${src#/vendor}" ] || ko "install: $src"; done < $T/zip/files/bound
[ -d $V/odm/etc/permissions ] || ko "install: /vendor/odm/etc/permissions"
ok "install: $n files, stock files unchanged, $(( $(wc -l < $T/zip/files/bound) + 1 )) bind mounts"
$INSTALL > $T/out 2>&1 && grep -q 'Previous installation' $T/out || { cat $T/out; ko "reinstall"; }
ok "reinstall"

UJC_PART_boot=$T/boot_ddk111.img $REMOVE > $T/out 2>&1 && { cat $T/out; ko "remove under a DDK 1.11 kernel: accepted"; }
[ -f $V/librehu/ddk111/MANIFEST ] || ko "remove under a DDK 1.11 kernel: files removed"
ok "remove refused under a DDK 1.11 kernel ($(grep ERROR $T/out))"
$REMOVE > $T/out 2>&1 || { cat $T/out; ko "remove"; }
(cd $V && find . | sort) | cmp -s - $T/vendor.before || { (cd $V && find . | sort) | diff $T/vendor.before - ; ko "remove: vendor not back to the original"; }
ok "remove: vendor identical to the original"
echo "all tests passed"
