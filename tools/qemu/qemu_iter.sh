#!/bin/bash
# LibreHU: finds the initcalls to add to initcall_blacklist.txt after a kernel change (tools/qemu/README.md). Boots the
# initramfs scenario with initcall_debug; while the kernel oopses inside an initcall, adds the last initcall started
# to a local list and boots again. Stops when userspace is reached, or on an oops outside an initcall (timer, work):
# then look at the call trace and add the initcall that started that timer or work by hand.
#   tools/qemu/qemu_iter.sh <Image> <work dir> [max boots]
# initcall_blacklist.txt is not changed: the names found are printed, the console is <work dir>/iter.log.
set -eu
IMG=${1:?usage: tools/qemu/qemu_iter.sh <Image> <work dir> [max boots]}
W=${2:?usage: tools/qemu/qemu_iter.sh <Image> <work dir> [max boots]}
MAX=${3:-20}
HERE=$(cd "$(dirname "$0")" && pwd)
QEMU=${QEMU:-qemu-system-aarch64}
BL=$(sed 's/#.*//' "$HERE/initcall_blacklist.txt" | tr -s ' \t' '\n' | grep -v '^$' | paste -sd, -)
L="$W/iter.log"
NEW=
for it in $(seq "$MAX"); do
	"$QEMU" -M virt,gic-version=2 -cpu cortex-a53 -smp 4 -m 2048 -nographic -no-reboot -nic none \
		-kernel "$IMG" -initrd "$W/init.cpio" \
		-append "console=ttyAMA0 earlycon=pl011,0x9000000 rdinit=/init panic=-1 loglevel=8 initcall_debug initcall_blacklist=$BL" \
		< /dev/null > "$L" 2>&1 &
	PID=$!
	for _ in $(seq 60); do
		grep -q 'LIBREHU QEMU INIT: done\|Kernel panic' "$L" && break
		sleep 1
	done
	kill "$PID" 2>/dev/null || true
	wait "$PID" 2>/dev/null || true
	if grep -q 'LIBREHU QEMU INIT: done' "$L"; then
		echo "userspace reached (boot $it); names to add to initcall_blacklist.txt:${NEW:- none}"
		exit 0
	fi
	if ! grep -A25 'Call trace' "$L" | grep -q 'do_one_initcall'; then
		echo "boot $it: oops outside an initcall, see $L:"
		grep -A12 'Call trace' "$L" | head -13
		echo "names found so far:${NEW:- none}"
		exit 1
	fi
	F=$(grep -o 'calling  [A-Za-z0-9_.]*+' "$L" | tail -1 | sed 's/calling  //; s/+$//')
	[ -n "$F" ] || { echo "boot $it: no initcall name in $L" >&2; exit 1; }
	echo "boot $it: oops in $F"
	BL="$BL,$F"
	NEW="$NEW $F"
done
echo "still oopsing after $MAX boots; names found:$NEW"
exit 1
