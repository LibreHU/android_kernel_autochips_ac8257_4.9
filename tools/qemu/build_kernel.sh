#!/bin/bash
# LibreHU: builds the QEMU variant of the AC8257 4.19 kernel (tools/qemu/README.md): ac8257_ujc201_defconfig plus
# tools/qemu/qemu.config (PL011 console, virtio disk), Image only. Run from the top of the kernel tree.
#   tools/qemu/build_kernel.sh <build dir>
# KMAKE and ATC_MAKE default to the values of .github/workflows/build.yml (clang 14, GNU binutils, Python 2 as
# "python" for the AutoChips DTS generator); both can be overridden from the environment.
set -eu
O=${1:?usage: tools/qemu/build_kernel.sh <build dir>}
KMAKE=${KMAKE:-"ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- CC=clang-14 CLANG_TRIPLE=aarch64-linux-gnu- HOSTCC=gcc python=python"}
ATC_MAKE=${ATC_MAKE:-"ATC_DISABLE_BATTERY_CHARGER=yes ATC_FASTDISP_VERSION=2.0"}

make O="$O" $KMAKE $ATC_MAKE ac8257_ujc201_defconfig
# merge only (-m), then olddefconfig resolves the dependencies
scripts/kconfig/merge_config.sh -m -O "$O" "$O/.config" tools/qemu/qemu.config
make O="$O" $KMAKE $ATC_MAKE olddefconfig
for c in SERIAL_AMBA_PL011_CONSOLE VIRTIO_MMIO VIRTIO_BLK; do
	grep -q "^CONFIG_$c=y" "$O/.config" || { echo "CONFIG_$c is not set in $O/.config" >&2; exit 1; }
done
make O="$O" $KMAKE $ATC_MAKE -j"$(nproc)" Image
