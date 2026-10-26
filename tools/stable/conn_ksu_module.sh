#!/bin/sh
# KernelSU module that loads the connectivity modules built with CONFIG_LIBREHU_CONN_MODULES (docs/STABLE.md).
#   tools/stable/conn_ksu_module.sh <kernel out dir> <dest zip> <version name>
# The kernel of a linux-stable branch no longer has the symbol CRCs of the stock kernel, so the vendor modules of
# /vendor/lib/modules (wmt_drv, wmt_chrdev_wifi, wlan_drv_gen4m, bt_drv, gps_drv, fmradio_drv) are refused.
# This module loads the same drivers, built from the GPL sources with this kernel, in the vendor order (wmt_drv at
# post-fs-data, before wmt_loader starts; the others once vendor.connsys.driver.ready=yes), only when /proc/version
# is the one of the kernel they were built with: with any other kernel it does nothing and the vendor modules are
# used as before. /vendor is not touched.
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

# Same order as the vendor init scripts (init.wmt_drv.rc, init.wlan_drv.rc, init.bt_drv.rc, init.gps_drv.rc,
# init.fmradio_drv.rc): wmt_drv first; wmt_loader (class core) then initialises it and sets
# vendor.connsys.driver.ready=yes, and only then the other modules are loaded.
cat > "$W/post-fs-data.sh" <<'EOF'
#!/system/bin/sh
# LibreHU: load wmt_drv built with this kernel (tools/stable/conn_ksu_module.sh)
MODDIR=${0%/*}
LOG=$MODDIR/last.log
{
	echo "$(date) $(cat /proc/version)"
	if [ "$(cat /proc/version)" != "$(cat "$MODDIR/kernel_version")" ]; then
		echo "other kernel: nothing loaded, the vendor modules are used"
		exit 0
	fi
	if grep -q "^wmt_drv " /proc/modules; then
		echo "wmt_drv: already loaded"
	elif insmod "$MODDIR/modules/wmt_drv.ko"; then
		echo "wmt_drv: loaded"
	else
		echo "wmt_drv: insmod failed ($?)"
	fi
} > "$LOG" 2>&1
EOF

cat > "$W/service.sh" <<'EOF'
#!/system/bin/sh
# LibreHU: once wmt_loader has initialised wmt_drv (vendor.connsys.driver.ready=yes), load the other modules
MODDIR=${0%/*}
LOG=$MODDIR/last.log
[ "$(cat /proc/version)" = "$(cat "$MODDIR/kernel_version")" ] || exit 0
grep -q "^wmt_drv " /proc/modules || exit 0
i=0
while [ "$(getprop vendor.connsys.driver.ready)" != "yes" ] && [ $i -lt 120 ]; do
	sleep 1
	i=$((i + 1))
done
{
	echo "$(date) vendor.connsys.driver.ready=$(getprop vendor.connsys.driver.ready) after ${i}s"
	[ "$(getprop vendor.connsys.driver.ready)" = "yes" ] || exit 0
	for m in wmt_chrdev_wifi wlan_drv_gen4m bt_drv gps_drv fmradio_drv; do
		if grep -q "^$m " /proc/modules; then
			echo "$m: already loaded"
		elif insmod "$MODDIR/modules/$m.ko"; then
			echo "$m: loaded"
		else
			echo "$m: insmod failed ($?)"
		fi
	done
} >> "$LOG" 2>&1
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
set_perm "$MODPATH/service.sh" 0 0 0755
EOF

rm -f "$ZIP"
(cd "$W" && zip -q -r -9 "$ZIP" module.prop customize.sh post-fs-data.sh service.sh kernel_version modules)
echo "$ZIP: $(cat "$W/kernel_version")"
