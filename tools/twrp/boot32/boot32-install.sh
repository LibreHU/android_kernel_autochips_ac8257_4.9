# UJC201 - partition boot 10 -> 32 Mio (zip TWRP librehu_boot32_twrp.zip). Prerequis : sauvegarde SP Flash Tool
# complete (Readback), vbmeta en VERIFICATION_DISABLED, memoire interne montee avec 4,4 Go libres.
# Ordre : verifications en lecture seule, sauvegardes dans /sdcard/LibreHU/boot32 (GPT, boot, dtbo, tee1, tee2,
# vendor), ecritures seulement dans l'ancienne zone de vendor (dtbo, tee1, tee2, vendor reduit, pied AVB de boot),
# GPT principale et de secours en dernier. Coupure avant la GPT : librehu_boot32_restore_twrp.zip remet vendor.

ui_print "UJC201 - 32 MiB boot partition"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
b32_tools
b32_state
case $B32_STATE in
new)
	if [ -e $B32_DIR/IN_PROGRESS ] || [ "$B32_BACKUP" != ok ]; then
		ui_print "Migration stopped right after the primary GPT: finishing the migration"
		b32_fix_backup
		rm -f $B32_DIR/vendor_new.img $B32_DIR/IN_PROGRESS
		echo "migration $VERSION $(date) (resumed)" > $B32_DIR/MIGRATED
		ui_print "Done. Reboot into TWRP, then flash librehu_kernel32_twrp_a9.zip or _a10.zip."
	else
		ui_print "The boot partition is already 32 MiB: nothing to do."
	fi
	rm -rf $B32_TMP; cleanup; exit 0;;
orig) ui_print "Original table found (backup GPT: $B32_BACKUP)";;
*) abort "unknown partition table (not the original UJC201 one): nothing was changed";;
esac
[ -d $SDCARD ] || abort "internal storage not mounted: no backup possible, nothing was changed"
# Essai arrete pendant les sauvegardes (etape 1) : SHA256SUMS ne s'ecrit qu'a la fin de l'etape 1, avant toute
# ecriture sur le disque (etape 3, qui commence par le nouveau dtbo a 530 Mio) : rien a remettre, copies jetees.
if [ -e $B32_DIR/IN_PROGRESS ] && [ ! -e $B32_DIR/SHA256SUMS ] && \
	[ "$(b32_rd $(( NEW_DTBO * MiB )) 4 | b32_hex)" != d7b7ab1e ]; then
	ui_print "Previous try stopped during the backups: nothing had been written to the disk."
	rm -f $B32_DIR/*.img $B32_DIR/*.bin $B32_DIR/IN_PROGRESS
	ui_print "  incomplete copies deleted, starting again"
fi
if [ -e $B32_DIR/IN_PROGRESS ] || [ -e $B32_DIR/SHA256SUMS ]; then
	ui_print "Backups of a previous migration in $B32_DIR."
	ui_print "Interrupted migration: flash librehu_boot32_restore_twrp.zip (it puts vendor back)."
	ui_print "Otherwise (old useless backup): delete this folder, then flash this zip again."
	abort "existing backups: nothing was changed"
fi
b32_unmount
b32_require_vbmeta

ui_print "Checks (read only)"
[ "$(b32_rd $(( OLD_DTBO * MiB )) 4 | b32_hex)" = d7b7ab1e ] || abort "dtbo: unexpected header"
dsz=$(b32_rd $(( OLD_DTBO * MiB + 4 )) 4 | b32_be)
[ "$dsz" -gt 0 ] && [ "$dsz" -le $MiB ] || abort "dtbo: $dsz bytes, does not fit in 1 MiB"
ui_print "  dtbo: $dsz bytes (fits in 1 MiB)"
[ "$(b32_rd $(( (OLD_BOOT + BOOT_OLD_MIB) * MiB - 64 )) 4)" = AVBf ] || abort "boot: no AVB footer at the end of the partition"
ui_print "  boot: AVB footer present"
for x in tee1:$OLD_TEE1 tee2:$OLD_TEE2; do
	[ "$(b32_rd $(( ${x#*:} * MiB )) 4 | b32_hex)" = 88168858 ] || abort "${x%%:*}: MTK header missing"
done
ui_print "  tee1 / tee2: MTK headers present"
[ -e "$B32_VEND" ] || abort "vendor not found ($B32_VEND)"
e2fsck -n -f $B32_VEND >/dev/null 2>&1 || abort "e2fsck -n reports errors on vendor: do not go on"
tune2fs -l $B32_VEND 2>/dev/null | grep -q '^Filesystem features:.*shared_blocks' && abort "vendor uses shared_blocks: cannot shrink it"
bs=$(b32_fsval $B32_VEND "Block size"); bc=$(b32_fsval $B32_VEND "Block count"); fr=$(b32_fsval $B32_VEND "Free blocks")
[ -n "$bs" ] && [ -n "$bc" ] && [ -n "$fr" ] || abort "tune2fs: unreadable vendor"
max=$(b32_calc "int($NEW_VEND_BYTES / $bs)"); used=$(( bc - fr ))
ui_print "  vendor: $bc blocks of $bs bytes, $used used, maximum after shrinking $max"
[ $(( used + used / 10 + 8192 )) -lt $max ] || abort "vendor too full for 2033 MiB"
b32_test "$(b32_calc "$bc * $bs")" "<=" "$OLD_VEND_BYTES" || abort "vendor: file system larger than the partition"
mkdir -p $B32_DIR || abort "cannot create folder $B32_DIR"
free=$(b32_free_kb $B32_DIR)
[ -n "$free" ] && [ "$free" -gt 4400000 ] || abort "4.4 GB free needed in the internal storage ($free KB)"
ui_print "  internal storage: $free KB free"

ui_print "1/4 Backups in $B32_DIR"
echo "migration $VERSION $(date)" > $B32_DIR/IN_PROGRESS
dd if=$B32_DISK of=$B32_DIR/pgpt_orig.bin bs=512 count=34 2>/dev/null || abort "GPT backup"
dd if=$B32_DISK of=$B32_DIR/sgpt_orig.bin bs=512 skip=$B32_SGPT count=33 2>/dev/null || abort "backup GPT backup"
dd if=$B32_DISK of=$B32_DIR/boot_orig.img bs=$MiB skip=$OLD_BOOT count=$BOOT_OLD_MIB 2>/dev/null || abort "boot backup"
dd if=$B32_DISK of=$B32_DIR/dtbo_orig.img bs=$MiB skip=$OLD_DTBO count=8 2>/dev/null || abort "dtbo backup"
dd if=$B32_DISK of=$B32_DIR/tee1_orig.img bs=$MiB skip=$OLD_TEE1 count=5 2>/dev/null || abort "tee1 backup"
dd if=$B32_DISK of=$B32_DIR/tee2_orig.img bs=$MiB skip=$OLD_TEE2 count=7 2>/dev/null || abort "tee2 backup"
dd if=$B32_DISK of=$B32_DIR/vendor_orig.img bs=$MiB skip=$OLD_VEND count=$OLD_VEND_MIB 2>/dev/null || abort "vendor backup"
b32_cut in_backups
b32_test "$(b32_size $B32_DIR/vendor_orig.img)" "==" "$OLD_VEND_BYTES" || abort "incomplete vendor backup (storage full?)"
( cd $B32_DIR && sha256sum pgpt_orig.bin sgpt_orig.bin boot_orig.img dtbo_orig.img tee1_orig.img tee2_orig.img \
	vendor_orig.img > SHA256SUMS ) || abort "checksums of the backups"
[ "$(b32_regsha $OLD_VEND $OLD_VEND_BYTES)" = "$(b32_sha $B32_DIR/vendor_orig.img)" ] || abort "vendor backup different from the partition"
ui_print "  backups made and read back (SHA256SUMS)"

ui_print "2/4 Preparation"
head -c $MiB $B32_DIR/dtbo_orig.img > $B32_TMP/dtbo_new.img
tail -c 64 $B32_DIR/boot_orig.img > $B32_TMP/boot_footer.bin
[ "$(head -c 4 $B32_TMP/boot_footer.bin)" = AVBf ] || abort "boot: AVB footer missing from the backup"
cp $B32_DIR/vendor_orig.img $B32_DIR/vendor_new.img || abort "vendor copy (storage full?)"
e2fsck -f -y $B32_DIR/vendor_new.img >/dev/null 2>&1; [ $? -le 1 ] || abort "e2fsck of the vendor copy"
if [ "$bc" -gt "$max" ]; then
	resize2fs $B32_DIR/vendor_new.img $max >/dev/null 2>&1 || abort "resize2fs vendor"
	e2fsck -f -y $B32_DIR/vendor_new.img >/dev/null 2>&1; [ $? -le 1 ] || abort "e2fsck of vendor after shrinking"
fi
nbc=$(b32_fsval $B32_DIR/vendor_new.img "Block count")
nfs=$(b32_calc "${nbc:-0} * $bs")
[ -n "$nbc" ] && b32_test "$nfs" "<=" "$NEW_VEND_BYTES" || abort "shrunk vendor too large for 2033 MiB"
head -c $nfs $B32_DIR/vendor_new.img > $B32_DIR/vendor_new.fs && mv $B32_DIR/vendor_new.fs $B32_DIR/vendor_new.img \
	|| abort "shrunk vendor"
e2fsck -n -f $B32_DIR/vendor_new.img >/dev/null 2>&1 || abort "shrunk vendor: e2fsck -n reports errors"
ui_print "  vendor shrunk to $nbc blocks, checked"

ui_print "3/4 Writing into the old vendor area"
b32_put $B32_TMP/dtbo_new.img $NEW_DTBO
b32_put $B32_DIR/tee1_orig.img $NEW_TEE1
b32_put $B32_DIR/tee2_orig.img $NEW_TEE2
b32_cut before_vendor
b32_put $B32_DIR/vendor_new.img $NEW_VEND
b32_footer $B32_TMP/boot_footer.bin $BOOT_NEW_MIB

ui_print "4/4 Partition table"
b32_write_gpt grow new
rm -f $B32_DIR/vendor_new.img $B32_DIR/IN_PROGRESS
echo "migration $VERSION $(date)" > $B32_DIR/MIGRATED
rm -rf $B32_TMP

ui_print " "
ui_print "Done: boot 32 MiB, dtbo 1 MiB, tee1 / tee2 moved, vendor 2033 MiB."
ui_print "1. Reboot into TWRP (Reboot > Recovery): the TWRP kernel reads the new table."
ui_print "2. Flash librehu_kernel32_twrp_a9.zip or _a10.zip (32 MiB ThinLTO kernel)."
ui_print "The current kernel (10 MiB) stays in place and boots as before."
ui_print "Backups: $B32_DIR (vendor_orig.img = 2 GB, can be deleted after a try)."
ui_print "Back to 10 MiB: librehu_boot32_restore_twrp.zip."
ui_print "After the migration, flash 10 MiB boot images only with the LibreHU 4.19 TWRP zips"
ui_print "(they copy the AVB footer to the end of the partition): no fastboot, no TWRP Install Image."
cleanup
exit 0
