#!/bin/bash
# LibreHU: test disk of the QEMU runs (tools/qemu/README.md). GPT disk with one "system" partition of fixed PARTUUID:
# ext4 root holding the test init, then the dm-verity hash tree right after the file system, without verity
# superblock (the layout avbtool gives system). Also disk_bad.img, the same with one byte of /init changed.
#   tools/qemu/make_disk.sh <work dir> <test init>
# Writes <work dir>/disk.img, disk_bad.img and disk.env (PARTUUID, DATA_BLOCKS, ROOT_HASH, SALT for run.sh).
# Needs mke2fs and debugfs (e2fsprogs), veritysetup (cryptsetup-bin), sgdisk (gdisk). No root needed.
set -eu
W=${1:?usage: tools/qemu/make_disk.sh <work dir> <test init>}
INIT=${2:?usage: tools/qemu/make_disk.sh <work dir> <test init>}
PARTUUID=1f2e3d4c-5b6a-4978-8695-a4b3c2d1e0f9
DATA_BLOCKS=4096	# 16 MiB of ext4 in 4 KiB blocks; the partition (20 MiB) also holds the hash tree

mkdir -p "$W/root/dev" "$W/root/proc" "$W/root/sys"
install -m 0755 "$INIT" "$W/root/init"
# the kernel already has a console from its own rootfs; the node in the image is only a convenience (needs root)
[ -e "$W/root/dev/console" ] || mknod "$W/root/dev/console" c 5 1 2>/dev/null || true

truncate -s 0 "$W/part.img"
truncate -s $((DATA_BLOCKS * 4096)) "$W/part.img"
mke2fs -q -F -t ext4 -b 4096 -O ^has_journal -d "$W/root" "$W/part.img" $DATA_BLOCKS
veritysetup format --no-superblock --data-block-size=4096 --hash-block-size=4096 --data-blocks=$DATA_BLOCKS \
	--hash-offset=$((DATA_BLOCKS * 4096)) "$W/part.img" "$W/part.img" > "$W/verity.txt"
ROOT_HASH=$(awk '/^Root hash:/ { print $3 }' "$W/verity.txt")
SALT=$(awk '/^Salt:/ { print $2 }' "$W/verity.txt")
[ -n "$ROOT_HASH" ] && [ -n "$SALT" ] || { echo "no root hash / salt in $W/verity.txt" >&2; exit 1; }

truncate -s 0 "$W/disk.img"
truncate -s 64M "$W/disk.img"
sgdisk -o -n 1:2048:+20M -t 1:8300 -u 1:$PARTUUID -c 1:system "$W/disk.img" > /dev/null
dd if="$W/part.img" of="$W/disk.img" bs=512 seek=2048 conv=notrunc status=none

# corrupted copy: one byte in the first data block of /init
BLK=$(debugfs -R "bmap /init 0" "$W/part.img" 2>/dev/null | tail -1)
case $BLK in ''|*[!0-9]*) echo "no block for /init in $W/part.img" >&2; exit 1;; esac
cp "$W/disk.img" "$W/disk_bad.img"
printf '\377' | dd of="$W/disk_bad.img" bs=1 seek=$((2048 * 512 + BLK * 4096 + 100)) conv=notrunc status=none

printf 'PARTUUID=%s\nDATA_BLOCKS=%s\nROOT_HASH=%s\nSALT=%s\n' "$PARTUUID" "$DATA_BLOCKS" "$ROOT_HASH" "$SALT" > "$W/disk.env"
echo "built $W/disk.img and $W/disk_bad.img (root hash $ROOT_HASH)"
