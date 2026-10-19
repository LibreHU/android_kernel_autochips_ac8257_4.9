#!/bin/sh
# LibreHU: USB network modules for the stock AC8257 kernel (Linux 4.9.117+, Jancar #25 and #72: same exports,
# same CRCs). Built out of tree against the reconstructed kernel source configured with the stock configuration,
# with clang; modpost takes the stock CRCs (reference/stock-Module.symvers), and check_ko.py verifies vermagic and
# that every import exists in the stock kernel with that CRC.
#
#   tools/usbnet/build.sh <android_kernel_autochips_ac8257_4.9> <out dir>
#
# Needs: clang (any recent one), the AOSP GCC 4.9 aarch64 binutils in PATH (aarch64-linux-android-), llvm-strip.
set -e
TREE=$(cd "$1" && pwd); OUT=$(mkdir -p "$2" && cd "$2" && pwd)
HERE=$(cd "$(dirname "$0")" && pwd)
KO=$OUT/build/kernel; SRC=$OUT/build/src
rm -rf "$OUT/build"; mkdir -p "$KO" "$SRC"

MAKE="make -s -C $TREE O=$KO ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- CC=clang CLANG_TRIPLE=aarch64-linux-gnu- HOSTCC=gcc"
cp "$TREE/reference/stock-ac8257_demo.config" "$KO/.config"
# Real genksyms CRCs from this tree are not used: the stock ones come from stock-Module.symvers.
"$TREE/scripts/config" --file "$KO/.config" -d AC8257_STOCK_CRCS
$MAKE olddefconfig
$MAKE modules_prepare
[ "$(cat "$KO/include/config/kernel.release")" = "4.9.117+" ] || { echo "kernel.release differs from the stock 4.9.117+"; exit 1; }

for f in r8152.c ipheth.c dm9601.c sr9700.c sr9700.h sr9800.c sr9800.h smsc95xx.c smsc95xx.h smsc75xx.c smsc75xx.h \
	mcs7830.c ch9200.c lan78xx.c lan78xx.h rtl8150.c pegasus.c pegasus.h cdc_eem.c; do
	cp "$TREE/drivers/net/usb/$f" "$SRC/"
done
python3 "$HERE/make_rndis.py" "$TREE/drivers/net/usb" "$SRC"
cp "$HERE/Kbuild" "$SRC/"
cp "$TREE/reference/stock-Module.symvers" "$SRC/stock.symvers"
$MAKE M="$SRC" KBUILD_EXTRA_SYMBOLS="$SRC/stock.symvers" -j"$(nproc)" modules

mkdir -p "$OUT/modules"
for k in "$SRC"/*.ko; do llvm-strip --strip-debug -o "$OUT/modules/$(basename "$k")" "$k" 2>/dev/null ||
	llvm-strip-18 --strip-debug -o "$OUT/modules/$(basename "$k")" "$k"; done
python3 "$HERE/check_ko.py" "$SRC/stock.symvers" "$OUT"/modules/*.ko | tee "$OUT/modules/CHECK.txt"
! grep -vq "crc_bad=0 unresolved=\[\] vermagic_ok=True" "$OUT/modules/CHECK.txt"
(cd "$OUT/modules" && sha256sum *.ko > SHA256SUMS)
echo "modules in $OUT/modules"
