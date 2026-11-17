# UJC201 - retour de la partition boot 32 -> 10 Mio (zip TWRP librehu_boot32_restore_twrp.zip).
# - Table 32 Mio : le contenu ACTUEL de dtbo, tee1, tee2 et vendor (modifications faites depuis la migration
#   comprises) est copie dans /sdcard/LibreHU/boot32/restore, vendor regrandi a 2048 Mio, puis tout est ecrit aux
#   emplacements d'origine et la GPT d'origine est recalculee (meme GUID). Le noyau en place doit tenir dans 10 Mio
#   (sinon : flasher d'abord un noyau 10 Mio, zip librehu_kernel_twrp_a9 / _a10, qui marche aussi sur 32 Mio).
#   Coupure en cours de route : reflasher ce zip, il reprend les copies verifiees.
# - Table d'origine avec une migration interrompue (IN_PROGRESS) : vendor_orig.img est recopie a sa place.

ui_print "UJC201 - boot partition back to 10 MiB"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
b32_tools
b32_state
R=$B32_DIR/restore
[ -d $SDCARD ] || abort "internal storage not mounted: nothing was changed"

if [ "$B32_STATE" = orig ]; then
	if [ -e $B32_DIR/IN_PROGRESS ]; then
		# Arret pendant les sauvegardes (etape 1 de la migration) : SHA256SUMS ne s'ecrit qu'a la fin de l'etape 1,
		# avant toute ecriture sur le disque (le nouveau dtbo a 530 Mio en premier) : rien a remettre.
		if [ ! -e $B32_DIR/SHA256SUMS ] && [ "$(b32_rd $(( NEW_DTBO * MiB )) 4 | b32_hex)" != d7b7ab1e ]; then
			ui_print "Previous try stopped during the backups: nothing had been written to the disk."
			rm -f $B32_DIR/*.img $B32_DIR/*.bin $B32_DIR/IN_PROGRESS
			rmdir $B32_DIR 2>/dev/null
			ui_print "Incomplete copies deleted: nothing to put back."
			rm -rf $B32_TMP; cleanup; exit 0
		fi
		ui_print "Interrupted migration: putting vendor back from $B32_DIR/vendor_orig.img"
		[ -f $B32_DIR/vendor_orig.img ] && [ -f $B32_DIR/SHA256SUMS ] || abort "incomplete backups and disk already changed: restore with SP Flash Tool"
		( cd $B32_DIR && sha256sum -c SHA256SUMS >/dev/null 2>&1 ) || abort "damaged backups: restore with SP Flash Tool"
		# en-tete + entrees seulement : l'octet 432 du MBR (code d'amorcage, inutilise par la GPT) change sur l'UJC201
		# (0x40 dans une sauvegarde SP Flash Tool, 0x33 plus tard sur la meme unite)
		[ "$(dd if=$B32_DISK bs=512 skip=1 count=33 2>/dev/null | sha256sum | cut -d' ' -f1)" = \
			"$(tail -c +513 $B32_DIR/pgpt_orig.bin | sha256sum | cut -d' ' -f1)" ] \
			|| abort "the GPT is not the one of the backup: nothing was changed"
		b32_unmount
		b32_put $B32_DIR/vendor_orig.img $OLD_VEND
		mv $B32_DIR $B32_DIR-repare-$(date +%Y%m%d-%H%M%S)
		ui_print "Original vendor put back. Reboot into TWRP. The backups are kept next to it."
	elif [ -e $R/IN_PROGRESS ] || [ "$B32_BACKUP" != ok ]; then
		ui_print "Return stopped right after the primary GPT: finishing the return"
		b32_fix_backup
		rm -f $R/*.img $R/IN_PROGRESS $R/SHA256SUMS $R/boot_footer.bin $B32_DIR/vendor_orig.img $B32_DIR/vendor_new.img
		rmdir $R 2>/dev/null
		[ -d $B32_DIR ] && mv $B32_DIR $B32_DIR-retour-$(date +%Y%m%d-%H%M%S)
		ui_print "Done. Reboot into TWRP."
	else
		ui_print "The boot partition is already 10 MiB (original table): nothing to do."
	fi
	rm -rf $B32_TMP; cleanup; exit 0
fi
[ "$B32_STATE" = new ] || abort "unknown partition table: nothing was changed"
ui_print "32 MiB table found (backup GPT: $B32_BACKUP)"
b32_unmount
b32_require_vbmeta

# Noyau en place : image + vbmeta du pied AVB dans les 10 premiers Mio (reprise : pied de la copie verifiee, la fin
# de la partition 32 Mio est deja dans le vendor recopie)
foot=$(( (OLD_BOOT + BOOT_NEW_MIB) * MiB - 64 ))
mkdir -p $R || abort "cannot create folder $R"
RESUME=0
[ -f $R/SHA256SUMS ] && ( cd $R && sha256sum -c SHA256SUMS >/dev/null 2>&1 ) && RESUME=1
if [ $RESUME = 1 ]; then cp $R/boot_footer.bin $B32_TMP/foot.bin; else b32_rd $foot 64 > $B32_TMP/foot.bin; fi
[ "$(head -c 4 $B32_TMP/foot.bin)" = AVBf ] || abort "no AVB footer at the end of boot: flash a 10 MiB LibreHU kernel first"
fv() { dd if=$B32_TMP/foot.bin bs=1 skip=$1 count=8 2>/dev/null | b32_be; }
isz=$(fv 12); voff=$(fv 20); vsz=$(fv 28)
lim=$(( BOOT_OLD_MIB * MiB - 64 ))
# comparaison dans awk (nombres flottants) : un pied abime donne des valeurs hors de l'arithmetique du shell
if ! awk -v a="$isz" -v o="$voff" -v s="$vsz" -v l=$lim 'BEGIN { exit !(a > 0 && a <= l && o + s <= l && o >= a) }'; then
	ui_print "The kernel in place ($isz bytes) does not fit in 10 MiB."
	abort "flash a 10 MiB kernel first (librehu_kernel_twrp_a9 / _a10), then this zip"
fi
ui_print "Kernel in place: $isz bytes, fits in 10 MiB"

if [ $RESUME = 1 ]; then
	ui_print "Resuming: verified copies of $R"
else
	free=$(b32_free_kb $R)
	[ -n "$free" ] && [ "$free" -gt 2300000 ] || abort "2.3 GB free needed in the internal storage ($free KB)"
	ui_print "1/4 Copies in $R"
	rm -f $R/*
	[ "$(b32_rd $(( NEW_DTBO * MiB )) 4 | b32_hex)" = d7b7ab1e ] || abort "dtbo: unexpected header"
	for x in tee1:$NEW_TEE1 tee2:$NEW_TEE2; do
		[ "$(b32_rd $(( ${x#*:} * MiB )) 4 | b32_hex)" = 88168858 ] || abort "${x%%:*}: MTK header missing"
	done
	{ dd if=$B32_DISK bs=$MiB skip=$NEW_DTBO count=1 2>/dev/null; dd if=/dev/zero bs=$MiB count=7 2>/dev/null; } > $R/dtbo.img
	dd if=$B32_DISK of=$R/tee1.img bs=$MiB skip=$NEW_TEE1 count=5 2>/dev/null || abort "tee1 copy"
	dd if=$B32_DISK of=$R/tee2.img bs=$MiB skip=$NEW_TEE2 count=7 2>/dev/null || abort "tee2 copy"
	cp $B32_TMP/foot.bin $R/boot_footer.bin
	dd if=$B32_DISK of=$R/vendor.img bs=$MiB skip=$NEW_VEND count=$NEW_VEND_MIB 2>/dev/null || abort "vendor copy"
	b32_test "$(b32_size $R/vendor.img)" "==" "$NEW_VEND_BYTES" || abort "incomplete vendor copy (storage full?)"
	[ "$(b32_size $R/dtbo.img)" = $(( 8 * MiB )) ] || abort "incomplete dtbo copy"
	e2fsck -f -y $R/vendor.img >/dev/null 2>&1; [ $? -le 1 ] || abort "e2fsck of the vendor copy"
	bs=$(b32_fsval $R/vendor.img "Block size")
	[ -n "$bs" ] || abort "unreadable vendor copy"
	resize2fs $R/vendor.img $(b32_calc "int($OLD_VEND_BYTES / $bs)") >/dev/null 2>&1 || abort "resize2fs vendor (2048 MiB)"
	e2fsck -f -y $R/vendor.img >/dev/null 2>&1; [ $? -le 1 ] || abort "e2fsck of vendor after growing"
	b32_test "$(b32_size $R/vendor.img)" "==" "$OLD_VEND_BYTES" || abort "grown vendor: unexpected size"
	e2fsck -n -f $R/vendor.img >/dev/null 2>&1 || abort "grown vendor: e2fsck -n reports errors"
	( cd $R && sha256sum dtbo.img tee1.img tee2.img boot_footer.bin vendor.img > SHA256SUMS ) || abort "checksums of the copies"
	ui_print "  copies made and checked (SHA256SUMS)"
fi

ui_print "2/4 Writing at the original places"
echo "return $VERSION $(date)" > $R/IN_PROGRESS
b32_put $R/dtbo.img $OLD_DTBO
b32_put $R/tee1.img $OLD_TEE1
b32_put $R/tee2.img $OLD_TEE2
b32_footer $R/boot_footer.bin $BOOT_OLD_MIB
b32_cut before_vendor
b32_put $R/vendor.img $OLD_VEND

ui_print "3/4 Original partition table"
b32_write_gpt shrink orig

ui_print "4/4 Cleaning up"
rm -f $R/*.img $R/IN_PROGRESS $R/SHA256SUMS $R/boot_footer.bin $B32_DIR/vendor_orig.img $B32_DIR/vendor_new.img
rmdir $R 2>/dev/null
[ -d $B32_DIR ] && mv $B32_DIR $B32_DIR-retour-$(date +%Y%m%d-%H%M%S)
rm -rf $B32_TMP
ui_print "  vendor copies deleted; small backups kept in $B32_DIR-retour-*"

ui_print " "
ui_print "Done: boot 10 MiB, dtbo 8 MiB, tee1 / tee2 and vendor (2048 MiB) at their original places."
ui_print "Reboot into TWRP (Reboot > Recovery), then boot Android."
ui_print "Kernels to use: 10 MiB variants (librehu_kernel_twrp_a9 / _a10)."
cleanup
exit 0
