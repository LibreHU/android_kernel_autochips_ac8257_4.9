#!/bin/sh
# KernelSU module that loads the connectivity modules built with CONFIG_LIBREHU_CONN_MODULES (docs/STABLE.md).
#   tools/stable/conn_ksu_module.sh <kernel out dir> <dest zip> <version name>
# The kernel of a linux-stable branch no longer has the symbol CRCs of the stock kernel, so the vendor modules of
# /vendor/lib/modules (wmt_drv, wmt_chrdev_wifi, wlan_drv_gen4m, bt_drv, gps_drv, fmradio_drv) are refused.
# This module loads the same drivers, built from the GPL sources with this kernel, at post-fs-data (before
# wmt_loader starts), only when /proc/version is the one of the kernel they were built with: with any other
# kernel it does nothing and the vendor modules are used as before. /vendor is not touched.
set -e
OUT=$1 ZIP=$(realpath -m "$2") VER=$3
STRIP=${STRIP:-aarch64-linux-android-strip}
CONN=$OUT/drivers/misc/mediatek/connectivity
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
mkdir -p "$W/modules"
for m in common/wmt_drv wlan/adaptor/wmt_chrdev_wifi bt/mt66xx/wmt/bt_drv gps/gps_drv fmradio/fmradio_drv \
	wlan/core/gen4m/wlan_drv_gen4m; do
	"$STRIP" --strip-debug -o "$W/modules/$(basename $m).ko" "$CONN/$m.ko"
done
# /proc/version = linux_banner of this vmlinux, without the final newline
strings "$OUT/vmlinux" | grep -m1 '^Linux version ' > "$W/kernel_version"

cat > "$W/module.prop" <<EOF
id=librehu_connectivity
name=LibreHU connectivity modules
version=$VER
versionCode=$(date -u +%Y%m%d%H)
author=LibreHU
description=Wi-Fi, Bluetooth, GPS and FM radio drivers built with the LibreHU kernel $VER, for kernels whose symbol CRCs no longer match the vendor modules. Does nothing with another kernel.
EOF

cat > "$W/post-fs-data.sh" <<'EOF'
#!/system/bin/sh
# LibreHU: load the connectivity modules built with this kernel (tools/stable/conn_ksu_module.sh)
MODDIR=${0%/*}
LOG=$MODDIR/last.log
{
	echo "$(date) $(cat /proc/version)"
	if [ "$(cat /proc/version)" != "$(cat "$MODDIR/kernel_version")" ]; then
		echo "other kernel: nothing loaded, the vendor modules are used"
		exit 0
	fi
	for m in wmt_drv wmt_chrdev_wifi bt_drv gps_drv fmradio_drv wlan_drv_gen4m; do
		if grep -q "^$m " /proc/modules; then
			echo "$m: already loaded"
		elif insmod "$MODDIR/modules/$m.ko"; then
			echo "$m: loaded"
		else
			echo "$m: insmod failed ($?)"
		fi
	done
} > "$LOG" 2>&1
EOF

cat > "$W/customize.sh" <<'EOF'
# KernelSU install script: only on the head unit kernel this module was built for
if [ "$(cat /proc/version)" != "$(cat "$MODPATH/kernel_version")" ]; then
	ui_print "! Built for another kernel:"
	ui_print "  $(cat "$MODPATH/kernel_version")"
	ui_print "  It will do nothing until that kernel runs."
fi
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
EOF

rm -f "$ZIP"
(cd "$W" && zip -q -r -9 "$ZIP" module.prop customize.sh post-fs-data.sh kernel_version modules)
echo "$ZIP: $(cat "$W/kernel_version")"
