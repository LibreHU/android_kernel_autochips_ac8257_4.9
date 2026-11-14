#!/bin/bash
# LibreHU: boots the QEMU variant in one scenario and checks its console (tools/qemu/README.md).
#   tools/qemu/run.sh <Image> <work dir> <scenario>
# The work dir holds init.cpio (build_init.sh), disk.img, disk_bad.img and disk.env (make_disk.sh); the console goes
# to <work dir>/<scenario>.log. Prints "<scenario>: PASS" (exit 0), FAIL or TIMEOUT (exit 1).
#   initramfs          initramfs with the test init (rdinit=/init): userspace reached
#   dm-verity          as the LK with a verifying vbmeta: skip_initramfs dm="... verity ... PARTUUID=..." root=/dev/dm-0
#   dm-verity-corrupt  same on disk_bad.img: verity refuses the changed block and restarts
#   partuuid           as the LK with a vbmeta in --disable-verity(-verification) mode: skip_initramfs root=PARTUUID=...
#   partuuid-missing   root=PARTUUID= of no partition: the kernel gives up after its 10 s wait
# QEMU "virt" machine with a GICv2 (the GICv3 init of this kernel calls the MediaTek GIC extension, mt_gic_ext_init(),
# which needs the mcucfg registers), the initcalls of initcall_blacklist.txt skipped. TIMEOUT (seconds) defaults to 120.
set -eu
IMG=${1:?usage: tools/qemu/run.sh <Image> <work dir> <scenario>}
W=${2:?usage: tools/qemu/run.sh <Image> <work dir> <scenario>}
SC=${3:?usage: tools/qemu/run.sh <Image> <work dir> <scenario>}
HERE=$(cd "$(dirname "$0")" && pwd)
QEMU=${QEMU:-qemu-system-aarch64}

BL=$(sed 's/#.*//' "$HERE/initcall_blacklist.txt" | tr -s ' \t' '\n' | grep -v '^$' | paste -sd, -)
BASE="console=ttyAMA0 earlycon=pl011,0x9000000 panic=-1 loglevel=8 initcall_blacklist=$BL"
DONE='LIBREHU QEMU INIT: done'
EXTRA=()
if [ "$SC" != initramfs ]; then
	PARTUUID=; DATA_BLOCKS=; ROOT_HASH=; SALT=
	. "$W/disk.env"
	DM="1 vroot none ro 1,0 $((DATA_BLOCKS * 8)) verity 1 PARTUUID=$PARTUUID PARTUUID=$PARTUUID 4096 4096"
	DM="$DM $DATA_BLOCKS $DATA_BLOCKS sha256 $ROOT_HASH $SALT 2 restart_on_corruption ignore_zero_blocks"
fi
disk() { EXTRA=(-drive "file=$W/$1,format=raw,if=none,id=d0,readonly=on" -device virtio-blk-device,drive=d0); }
case $SC in
initramfs)
	APPEND="$BASE rdinit=/init"; EXTRA=(-initrd "$W/init.cpio"); PASS=$DONE;;
dm-verity)
	APPEND="$BASE skip_initramfs init=/init root=/dev/dm-0 dm=\"$DM\""; disk disk.img; PASS=$DONE;;
dm-verity-corrupt)
	APPEND="$BASE skip_initramfs init=/init root=/dev/dm-0 dm=\"$DM\""; disk disk_bad.img
	PASS="Restarting system with command 'dm-verity device corrupted'";;
partuuid)
	APPEND="$BASE skip_initramfs init=/init ro root=PARTUUID=$PARTUUID"; disk disk.img; PASS=$DONE;;
partuuid-missing)
	APPEND="$BASE skip_initramfs init=/init ro root=PARTUUID=00000000-0000-0000-0000-000000000001"; disk disk.img
	PASS='still missing after 10000 ms';;
*)
	echo "unknown scenario $SC" >&2; exit 2;;
esac

LOG="$W/$SC.log"
"$QEMU" -M virt,gic-version=2 -cpu cortex-a53 -smp 4 -m 2048 -nographic -no-reboot -nic none \
	-kernel "$IMG" "${EXTRA[@]}" -append "$APPEND" < /dev/null > "$LOG" 2>&1 &
PID=$!
RES=TIMEOUT
for _ in $(seq "${TIMEOUT:-120}"); do
	if grep -qF "$PASS" "$LOG"; then RES=PASS; break; fi
	if grep -q 'Kernel panic' "$LOG"; then RES=FAIL; break; fi
	kill -0 "$PID" 2>/dev/null || { RES=FAIL; break; }
	sleep 1
done
# the power off path of the MediaTek drivers then oopses in QEMU (no RGU / PMIC): stop QEMU there
kill "$PID" 2>/dev/null || true
wait "$PID" 2>/dev/null || true
echo "$SC: $RES"
[ "$RES" = PASS ]
