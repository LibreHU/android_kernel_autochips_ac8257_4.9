# UJC201 - partition boot 10 -> 32 Mio (zip TWRP librehu_boot32_twrp.zip). Prerequis : sauvegarde SP Flash Tool
# complete (Readback), vbmeta en VERIFICATION_DISABLED, memoire interne montee avec 4,4 Go libres.
# Ordre : verifications en lecture seule, sauvegardes dans /sdcard/LibreHU/boot32 (GPT, boot, dtbo, tee1, tee2,
# vendor), ecritures seulement dans l'ancienne zone de vendor (dtbo, tee1, tee2, vendor reduit, pied AVB de boot),
# GPT principale et de secours en dernier. Coupure avant la GPT : librehu_boot32_restore_twrp.zip remet vendor.

ui_print "UJC201 - partition boot 32 Mio"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
b32_tools
b32_state
case $B32_STATE in
new)
	if [ -e $B32_DIR/IN_PROGRESS ] || [ "$B32_BACKUP" != ok ]; then
		ui_print "Migration interrompue juste apres la GPT principale : fin de la migration"
		b32_fix_backup
		rm -f $B32_DIR/vendor_new.img $B32_DIR/IN_PROGRESS
		echo "migration $VERSION $(date) (reprise)" > $B32_DIR/MIGRATED
		ui_print "Termine. Redemarrer dans TWRP, puis flasher librehu_kernel32_twrp_a9.zip ou _a10.zip."
	else
		ui_print "La partition boot fait deja 32 Mio : rien a faire."
	fi
	rm -rf $B32_TMP; cleanup; exit 0;;
orig) ui_print "Table d'origine reconnue (secours : $B32_BACKUP)";;
*) abort "table de partitions inconnue (pas celle de l'UJC201 d'origine) : rien n'a ete modifie";;
esac
[ -d $SDCARD ] || abort "memoire interne non montee : sauvegardes impossibles, rien n'a ete modifie"
if [ -e $B32_DIR/IN_PROGRESS ] || [ -e $B32_DIR/SHA256SUMS ]; then
	ui_print "Sauvegardes d'une migration precedente dans $B32_DIR."
	ui_print "Migration interrompue : flasher librehu_boot32_restore_twrp.zip (il remet vendor)."
	ui_print "Sinon (ancienne sauvegarde inutile) : supprimer ce dossier, puis reflasher ce zip."
	abort "sauvegardes existantes : rien n'a ete modifie"
fi
b32_unmount
b32_require_vbmeta

ui_print "Verifications (lecture seule)"
[ "$(b32_rd $(( OLD_DTBO * MiB )) 4 | b32_hex)" = d7b7ab1e ] || abort "dtbo : en-tete inattendu"
dsz=$(b32_rd $(( OLD_DTBO * MiB + 4 )) 4 | b32_be)
[ "$dsz" -gt 0 ] && [ "$dsz" -le $MiB ] || abort "dtbo : $dsz octets, ne tient pas dans 1 Mio"
ui_print "  dtbo : $dsz octets (tient dans 1 Mio)"
[ "$(b32_rd $(( (OLD_BOOT + BOOT_OLD_MIB) * MiB - 64 )) 4)" = AVBf ] || abort "boot : pas de pied AVB en fin de partition"
ui_print "  boot : pied AVB present"
for x in tee1:$OLD_TEE1 tee2:$OLD_TEE2; do
	[ "$(b32_rd $(( ${x#*:} * MiB )) 4 | b32_hex)" = 88168858 ] || abort "${x%%:*} : en-tete MTK absent"
done
ui_print "  tee1 / tee2 : en-tetes MTK presents"
[ -e "$B32_VEND" ] || abort "vendor introuvable ($B32_VEND)"
e2fsck -n -f $B32_VEND >/dev/null 2>&1 || abort "e2fsck -n vendor signale des erreurs : ne pas continuer"
tune2fs -l $B32_VEND 2>/dev/null | grep -q '^Filesystem features:.*shared_blocks' && abort "vendor en shared_blocks : reduction impossible"
bs=$(b32_fsval $B32_VEND "Block size"); bc=$(b32_fsval $B32_VEND "Block count"); fr=$(b32_fsval $B32_VEND "Free blocks")
[ -n "$bs" ] && [ -n "$bc" ] && [ -n "$fr" ] || abort "tune2fs : vendor illisible"
max=$(( NEW_VEND_MIB * MiB / bs )); used=$(( bc - fr ))
ui_print "  vendor : $bc blocs de $bs octets, $used utilises, maximum apres reduction $max"
[ $(( used + used / 10 + 8192 )) -lt $max ] || abort "vendor trop plein pour 2033 Mio"
[ $(( bc * bs )) -le $(( OLD_VEND_MIB * MiB )) ] || abort "vendor : systeme de fichiers plus grand que la partition"
mkdir -p $B32_DIR || abort "dossier $B32_DIR impossible"
free=$(b32_free_kb $B32_DIR)
[ -n "$free" ] && [ "$free" -gt 4400000 ] || abort "il faut 4,4 Go libres dans la memoire interne ($free Ko)"
ui_print "  memoire interne : $free Ko libres"

ui_print "1/4 Sauvegardes dans $B32_DIR"
echo "migration $VERSION $(date)" > $B32_DIR/IN_PROGRESS
dd if=$B32_DISK of=$B32_DIR/pgpt_orig.bin bs=512 count=34 2>/dev/null || abort "sauvegarde GPT"
dd if=$B32_DISK of=$B32_DIR/sgpt_orig.bin bs=512 skip=$B32_SGPT count=33 2>/dev/null || abort "sauvegarde GPT de secours"
dd if=$B32_DISK of=$B32_DIR/boot_orig.img bs=$MiB skip=$OLD_BOOT count=$BOOT_OLD_MIB 2>/dev/null || abort "sauvegarde boot"
dd if=$B32_DISK of=$B32_DIR/dtbo_orig.img bs=$MiB skip=$OLD_DTBO count=8 2>/dev/null || abort "sauvegarde dtbo"
dd if=$B32_DISK of=$B32_DIR/tee1_orig.img bs=$MiB skip=$OLD_TEE1 count=5 2>/dev/null || abort "sauvegarde tee1"
dd if=$B32_DISK of=$B32_DIR/tee2_orig.img bs=$MiB skip=$OLD_TEE2 count=7 2>/dev/null || abort "sauvegarde tee2"
dd if=$B32_DISK of=$B32_DIR/vendor_orig.img bs=$MiB skip=$OLD_VEND count=$OLD_VEND_MIB 2>/dev/null || abort "sauvegarde vendor"
[ "$(wc -c < $B32_DIR/vendor_orig.img)" = $(( OLD_VEND_MIB * MiB )) ] || abort "sauvegarde vendor incomplete (memoire pleine ?)"
( cd $B32_DIR && sha256sum pgpt_orig.bin sgpt_orig.bin boot_orig.img dtbo_orig.img tee1_orig.img tee2_orig.img \
	vendor_orig.img > SHA256SUMS ) || abort "empreintes des sauvegardes"
[ "$(b32_regsha $OLD_VEND $(( OLD_VEND_MIB * MiB )))" = "$(b32_sha $B32_DIR/vendor_orig.img)" ] || abort "sauvegarde vendor differente de la partition"
ui_print "  sauvegardes faites et relues (SHA256SUMS)"

ui_print "2/4 Preparation"
head -c $MiB $B32_DIR/dtbo_orig.img > $B32_TMP/dtbo_new.img
tail -c 64 $B32_DIR/boot_orig.img > $B32_TMP/boot_footer.bin
[ "$(head -c 4 $B32_TMP/boot_footer.bin)" = AVBf ] || abort "boot : pied AVB absent de la sauvegarde"
cp $B32_DIR/vendor_orig.img $B32_DIR/vendor_new.img || abort "copie de vendor (memoire pleine ?)"
e2fsck -f -y $B32_DIR/vendor_new.img >/dev/null 2>&1; [ $? -le 1 ] || abort "e2fsck de la copie de vendor"
if [ "$bc" -gt "$max" ]; then
	resize2fs $B32_DIR/vendor_new.img $max >/dev/null 2>&1 || abort "resize2fs vendor"
	e2fsck -f -y $B32_DIR/vendor_new.img >/dev/null 2>&1; [ $? -le 1 ] || abort "e2fsck vendor apres reduction"
fi
nbc=$(b32_fsval $B32_DIR/vendor_new.img "Block count")
[ -n "$nbc" ] && [ $(( nbc * bs )) -le $(( NEW_VEND_MIB * MiB )) ] || abort "vendor reduit trop grand pour 2033 Mio"
head -c $(( nbc * bs )) $B32_DIR/vendor_new.img > $B32_DIR/vendor_new.fs && mv $B32_DIR/vendor_new.fs $B32_DIR/vendor_new.img \
	|| abort "vendor reduit"
e2fsck -n -f $B32_DIR/vendor_new.img >/dev/null 2>&1 || abort "vendor reduit : e2fsck -n signale des erreurs"
ui_print "  vendor reduit a $nbc blocs, controle"

ui_print "3/4 Ecriture dans l'ancienne zone de vendor"
b32_put $B32_TMP/dtbo_new.img $NEW_DTBO
b32_put $B32_DIR/tee1_orig.img $NEW_TEE1
b32_put $B32_DIR/tee2_orig.img $NEW_TEE2
b32_cut before_vendor
b32_put $B32_DIR/vendor_new.img $NEW_VEND
b32_footer $B32_TMP/boot_footer.bin $BOOT_NEW_MIB

ui_print "4/4 Table de partitions"
b32_write_gpt grow new
rm -f $B32_DIR/vendor_new.img $B32_DIR/IN_PROGRESS
echo "migration $VERSION $(date)" > $B32_DIR/MIGRATED
rm -rf $B32_TMP

ui_print " "
ui_print "Termine : boot 32 Mio, dtbo 1 Mio, tee1 / tee2 deplaces, vendor 2033 Mio."
ui_print "1. Redemarrer dans TWRP (Redemarrer > Recovery) : le noyau de TWRP relit la table."
ui_print "2. Flasher librehu_kernel32_twrp_a9.zip ou _a10.zip (noyau ThinLTO 32 Mio)."
ui_print "Le noyau actuel (10 Mio) reste en place et demarre comme avant."
ui_print "Sauvegardes : $B32_DIR (vendor_orig.img = 2 Go, supprimable apres essai)."
ui_print "Retour a 10 Mio : librehu_boot32_restore_twrp.zip."
ui_print "Apres la migration, flasher les images boot de 10 Mio seulement par les zips TWRP LibreHU 4.19"
ui_print "(ils recopient le pied AVB en fin de partition) : pas de fastboot, pas de TWRP Install Image."
cleanup
exit 0
