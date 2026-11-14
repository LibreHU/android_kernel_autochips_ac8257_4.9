#!/bin/bash
# LibreHU: builds the test /init (tools/qemu/init.c, static aarch64) and an initramfs holding it.
#   tools/qemu/build_init.sh <work dir> <kernel build dir>
# The initramfs is made with usr/gen_init_cpio of the kernel build. CLANG defaults to clang-14, the linker is the
# GNU ld of binutils-aarch64-linux-gnu.
set -eu
W=${1:?usage: tools/qemu/build_init.sh <work dir> <kernel build dir>}
KO=${2:?usage: tools/qemu/build_init.sh <work dir> <kernel build dir>}
CLANG=${CLANG:-clang-14}

mkdir -p "$W"
$CLANG --target=aarch64-linux-gnu -O2 -ffreestanding -fno-stack-protector -fno-builtin -nostdlib -static \
	-fuse-ld=bfd -B /usr/bin/aarch64-linux-gnu- -Wl,-e,_start -o "$W/init" tools/qemu/init.c
printf 'dir /dev 0755 0 0\nnod /dev/console 0600 0 0 c 5 1\nfile /init %s 0755 0 0\n' "$W/init" > "$W/initramfs.list"
"$KO/usr/gen_init_cpio" "$W/initramfs.list" > "$W/init.cpio"
echo "built $W/init and $W/init.cpio"
