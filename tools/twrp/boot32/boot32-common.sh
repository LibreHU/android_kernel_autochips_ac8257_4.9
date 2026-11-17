# --- Partition boot de 32 Mio (zips librehu_boot32_twrp.zip et librehu_boot32_restore_twrp.zip), apres
# twrp-common.sh. Table d'origine de l'UJC201 et nouvelle table (seules les entrees 26 a 30 changent) :
#   boot   @498 Mio  10 -> 32 Mio (debut inchange)
#   dtbo   @508 -> @530 Mio, 8 -> 1 Mio (contenu ~52 Ko)
#   tee1   @516 -> @531 Mio, 5 Mio        tee2 @521 -> @536 Mio, 7 Mio
#   vendor @528 -> @543 Mio, 2048 -> 2033 Mio (meme fin ; systeme de fichiers reduit puis recopie)
# La GPT est calculee sur l'unite par files/gpt32 (tools/twrp/boot32/gpt32.c) a partir de sa propre GPT
# principale : les GUID et la taille du disque restent ceux de l'unite. Le zip ne contient aucune table.
# Verified Boot : le vbmeta stock porte un hashtree pour vendor, que la reduction de vendor casse ; les deux zips
# exigent un vbmeta deja en VERIFICATION_DISABLED (celui que LineageOS demande) et n'ecrivent jamais vbmeta.
# Pied AVB de boot : libavb le lit a la fin de la partition ; il est recopie a la nouvelle fin a chaque changement.

B32_DISK=${UJC_DISK:-/dev/block/mmcblk0}
B32_VEND=${UJC_VENDOR:-$BYNAME/vendor}
B32_DIR=$SDCARD/LibreHU/boot32
B32_TMP=${UJC_TMP:-/tmp}/librehu_boot32
MiB=1048576
OLD_BOOT=498; OLD_DTBO=508; OLD_TEE1=516; OLD_TEE2=521; OLD_VEND=528; OLD_VEND_MIB=2048; BOOT_OLD_MIB=10
NEW_DTBO=530; NEW_TEE1=531; NEW_TEE2=536; NEW_VEND=543; NEW_VEND_MIB=2033; BOOT_NEW_MIB=32

# Grands nombres : le shell de TWRP (/sbin/sh = mksh) calcule sur 32 bits signes, $(( )) et [ -le ] debordent a
# 2 Gio ($((2048 * 1048576)) = -2147483648 : r184 refusait sa propre sauvegarde complete de vendor, « memoire
# pleine ? »). Tout ce qui peut atteindre 2 Gio passe par awk (flottants, exacts jusqu'a 2^53).
b32_calc() { awk "BEGIN { printf \"%.0f\\n\", $1 }"; }
b32_test() { awk -v a="$1" -v b="$3" "BEGIN { exit !(a $2 b) }"; }
b32_size() { wc -c < "$1" | tr -d ' '; }
OLD_VEND_BYTES=$(b32_calc "$OLD_VEND_MIB * $MiB"); NEW_VEND_BYTES=$(b32_calc "$NEW_VEND_MIB * $MiB")
b32_sha() { sha256sum "$1" | cut -d' ' -f1; }
# Essais hors autoradio seulement (UJC_TEST) : arret net a une etape, comme une coupure de courant
b32_cut() { [ -n "$UJC_TEST" ] && [ "$UJC_FAIL_AT" = "$1" ] && { echo "(test) simulated power cut: $1"; exit 3; }; return 0; }
# zone du disque (debut en octets, longueur en octets) -> sortie standard
b32_rd() { dd if=$B32_DISK bs=512 skip=$(( $1 / 512 )) count=$(( ($1 % 512 + $2 + 511) / 512 )) 2>/dev/null \
	| tail -c +$(( $1 % 512 + 1 )) | head -c $2; }
b32_regsha() { dd if=$B32_DISK bs=$MiB skip=$1 count=$(b32_calc "int(($2 + $MiB - 1) / $MiB)") 2>/dev/null | head -c $2 | sha256sum | cut -d' ' -f1; }
b32_hex() { od -A n -t x1 | tr -d ' \n'; }
b32_be() { od -A n -t u1 | tr -s ' \n' '  ' | awk '{ v = 0; for (i = 1; i <= NF; i++) v = v * 256 + $i; printf "%.0f\n", v }'; }
b32_fsval() { tune2fs -l "$1" 2>/dev/null | awk -F: -v k="$2" '$1==k{gsub(/ /,"",$2);print $2}'; }
# fichier -> disque a $2 Mio, puis relecture
b32_put() {
	dd if="$1" of=$B32_DISK bs=$MiB seek=$2 conv=notrunc,fsync 2>/dev/null || abort "cannot write $(basename $1) at $2 MiB"
	[ "$(b32_regsha $2 $(wc -c < "$1"))" = "$(b32_sha "$1")" ] || abort "$(basename $1) read back different at $2 MiB"
	ui_print "  $(basename $1) written at $2 MiB, read back identical"
}
# 64 octets (fichier $1) a la fin d'une partition boot de $2 Mio, puis relecture
b32_footer() {
	_o=$(( (OLD_BOOT + $2) * MiB - 64 ))
	dd if="$1" of=$B32_DISK bs=64 seek=$(( _o / 64 )) conv=notrunc,fsync 2>/dev/null || abort "cannot write the AVB footer"
	[ "$(b32_rd $_o 64 | sha256sum | cut -d' ' -f1)" = "$(b32_sha "$1")" ] || abort "AVB footer read back different"
	ui_print "  boot AVB footer written at the end of the $2 MiB partition"
}

# Outils : gpt32 du zip, e2fsprogs de TWRP
b32_tools() {
	for _t in dd sha256sum head tail wc od awk tr cut grep df cp mv unzip e2fsck resize2fs tune2fs; do
		command -v $_t >/dev/null 2>&1 || abort "tool $_t missing from TWRP: nothing was changed"
	done
	rm -rf $B32_TMP; mkdir -p $B32_TMP
	if [ -n "$UJC_GPT32" ]; then GPT32=$UJC_GPT32; else
		unzip -o -q "$ZIPFILE" files/gpt32 -d $B32_TMP || abort "cannot extract gpt32"
		GPT32=$B32_TMP/files/gpt32; chmod 0755 $GPT32
	fi
}
# Etat de la table : B32_STATE = orig / new / other, B32_BACKUP = ok / bad, B32_SGPT = LBA de la GPT de secours
b32_state() {
	_s=$($GPT32 state $B32_DISK 2>$B32_TMP/gpt32.err)
	_rc=$?
	[ $_rc -le 1 ] || abort "unreadable GPT: $(cat $B32_TMP/gpt32.err)"
	B32_STATE=$(echo "$_s" | sed -n 's/.*state=\([a-z]*\).*/\1/p')
	B32_BACKUP=$(echo "$_s" | sed -n 's/.*backup=\([a-z]*\).*/\1/p')
	B32_SGPT=$(echo "$_s" | sed -n 's/.*sgpt_lba=\([0-9]*\).*/\1/p')
	[ -n "$B32_STATE" ] && [ -n "$B32_SGPT" ] || abort "gpt32: unexpected answer ($_s)"
}
# Nouvelle GPT ($1 = grow ou shrink), ecrite principale puis secours, relue, etat attendu $2
b32_write_gpt() {
	$GPT32 make $B32_DISK $1 $B32_TMP/pgpt.bin $B32_TMP/sgpt.bin >$B32_TMP/gpt32.out 2>$B32_TMP/gpt32.err \
		|| abort "cannot compute the GPT: $(cat $B32_TMP/gpt32.err)"
	_lba=$(sed -n 's/.*sgpt_lba=\([0-9]*\).*/\1/p' $B32_TMP/gpt32.out)
	[ "$_lba" = "$B32_SGPT" ] || abort "inconsistent backup GPT position"
	b32_cut before_gpt
	dd if=$B32_TMP/pgpt.bin of=$B32_DISK bs=512 seek=0 conv=notrunc,fsync 2>/dev/null || abort "cannot write the primary GPT"
	b32_cut between_gpt
	dd if=$B32_TMP/sgpt.bin of=$B32_DISK bs=512 seek=$_lba conv=notrunc,fsync 2>/dev/null || abort "cannot write the backup GPT"
	sync
	[ "$(dd if=$B32_DISK bs=512 count=34 2>/dev/null | sha256sum | cut -d' ' -f1)" = "$(b32_sha $B32_TMP/pgpt.bin)" ] \
		|| abort "primary GPT read back different"
	b32_state
	[ "$B32_STATE" = $2 ] && [ "$B32_BACKUP" = ok ] || abort "GPT read back: state $B32_STATE, backup $B32_BACKUP"
	ui_print "  primary and backup GPT written and read back ($2)"
}
# GPT de secours perdue (coupure entre les deux ecritures) : recalculee depuis la principale, table inchangee
b32_fix_backup() {
	[ "$B32_BACKUP" = ok ] && return
	_want=$B32_STATE
	$GPT32 make $B32_DISK same $B32_TMP/pgpt.bin $B32_TMP/sgpt.bin >$B32_TMP/gpt32.out 2>$B32_TMP/gpt32.err \
		|| abort "cannot compute the backup GPT: $(cat $B32_TMP/gpt32.err)"
	dd if=$B32_TMP/sgpt.bin of=$B32_DISK bs=512 seek=$B32_SGPT conv=notrunc,fsync 2>/dev/null || abort "cannot write the backup GPT"
	sync
	b32_state
	[ "$B32_STATE" = $_want ] && [ "$B32_BACKUP" = ok ] || abort "backup GPT read back: state $B32_STATE, backup $B32_BACKUP"
	ui_print "  backup GPT recomputed and read back"
}
# vendor et system demontes (TWRP les monte parfois)
b32_unmount() {
	[ -n "$UJC_TEST" ] && return
	for _m in $VENMNT /vendor $SYSMNT /system_root /system; do umount $_m 2>/dev/null; done
	grep -qE ' (/vendor|/system_root|/system) ' /proc/mounts && abort "vendor or system still mounted (TWRP > Mount: untick everything)"
	ui_print "vendor and system unmounted"
}
# vbmeta en VERIFICATION_DISABLED (octet 123 = poids faible des flags, bit 1)
b32_require_vbmeta() {
	vbmeta_ok || abort "unreadable vbmeta ($VBMETA)"
	_v=$(vbmeta_byte 123)
	if [ -z "$_v" ] || [ $((_v & 2)) != 2 ]; then
		ui_print "vbmeta still verifies the partitions (flags=$_v): the vendor hashtree would be broken."
		ui_print "This zip never writes vbmeta. First flash vbmeta with both options (as for LineageOS):"
		ui_print "fastboot --disable-verity --disable-verification flash vbmeta vbmeta.img"
		abort "vbmeta: VERIFICATION_DISABLED required, nothing was changed"
	fi
	ui_print "vbmeta: VERIFICATION_DISABLED (flags=$_v)"
}
b32_free_kb() { df -k "$1" 2>/dev/null | tail -n1 | awk '{print $(NF-2)}'; }
