# UJC201 - retour de la partition boot 32 -> 10 Mio (zip TWRP librehu_boot32_restore_twrp.zip).
# - Table 32 Mio : le contenu ACTUEL de dtbo, tee1, tee2 et vendor (modifications faites depuis la migration
#   comprises) est copie dans /sdcard/LibreHU/boot32/restore, vendor regrandi a 2048 Mio, puis tout est ecrit aux
#   emplacements d'origine et la GPT d'origine est recalculee (meme GUID). Le noyau en place doit tenir dans 10 Mio
#   (sinon : flasher d'abord un noyau 10 Mio, zip librehu_kernel_twrp_a9 / _a10, qui marche aussi sur 32 Mio).
#   Coupure en cours de route : reflasher ce zip, il reprend les copies verifiees.
# - Table d'origine avec une migration interrompue (IN_PROGRESS) : vendor_orig.img est recopie a sa place.

ui_print "UJC201 - retour de la partition boot a 10 Mio"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
b32_tools
b32_state
R=$B32_DIR/restore
[ -d $SDCARD ] || abort "memoire interne non montee : rien n'a ete modifie"

if [ "$B32_STATE" = orig ]; then
	if [ -e $B32_DIR/IN_PROGRESS ]; then
		ui_print "Migration interrompue : remise de vendor depuis $B32_DIR/vendor_orig.img"
		[ -f $B32_DIR/vendor_orig.img ] && [ -f $B32_DIR/SHA256SUMS ] || abort "vendor_orig.img absent : restaurer avec SP Flash Tool"
		( cd $B32_DIR && sha256sum -c SHA256SUMS >/dev/null 2>&1 ) || abort "sauvegardes abimees : restaurer avec SP Flash Tool"
		[ "$(dd if=$B32_DISK bs=512 count=34 2>/dev/null | sha256sum | cut -d' ' -f1)" = "$(b32_sha $B32_DIR/pgpt_orig.bin)" ] \
			|| abort "la GPT n'est pas celle de la sauvegarde : rien n'a ete modifie"
		b32_unmount
		b32_put $B32_DIR/vendor_orig.img $OLD_VEND
		mv $B32_DIR $B32_DIR-repare-$(date +%Y%m%d-%H%M%S)
		ui_print "Vendor d'origine remis. Redemarrer dans TWRP. Les sauvegardes sont gardees a cote."
	elif [ -e $R/IN_PROGRESS ] || [ "$B32_BACKUP" != ok ]; then
		ui_print "Retour interrompu juste apres la GPT principale : fin du retour"
		b32_fix_backup
		rm -f $R/*.img $R/IN_PROGRESS $R/SHA256SUMS $R/boot_footer.bin $B32_DIR/vendor_orig.img $B32_DIR/vendor_new.img
		rmdir $R 2>/dev/null
		[ -d $B32_DIR ] && mv $B32_DIR $B32_DIR-retour-$(date +%Y%m%d-%H%M%S)
		ui_print "Termine. Redemarrer dans TWRP."
	else
		ui_print "La partition boot fait deja 10 Mio (table d'origine) : rien a faire."
	fi
	rm -rf $B32_TMP; cleanup; exit 0
fi
[ "$B32_STATE" = new ] || abort "table de partitions inconnue : rien n'a ete modifie"
ui_print "Table 32 Mio reconnue (secours : $B32_BACKUP)"
b32_unmount
b32_require_vbmeta

# Noyau en place : image + vbmeta du pied AVB dans les 10 premiers Mio (reprise : pied de la copie verifiee, la fin
# de la partition 32 Mio est deja dans le vendor recopie)
foot=$(( (OLD_BOOT + BOOT_NEW_MIB) * MiB - 64 ))
mkdir -p $R || abort "dossier $R impossible"
RESUME=0
[ -f $R/SHA256SUMS ] && ( cd $R && sha256sum -c SHA256SUMS >/dev/null 2>&1 ) && RESUME=1
if [ $RESUME = 1 ]; then cp $R/boot_footer.bin $B32_TMP/foot.bin; else b32_rd $foot 64 > $B32_TMP/foot.bin; fi
[ "$(head -c 4 $B32_TMP/foot.bin)" = AVBf ] || abort "pas de pied AVB en fin de boot : flasher d'abord un noyau 10 Mio LibreHU"
fv() { dd if=$B32_TMP/foot.bin bs=1 skip=$1 count=8 2>/dev/null | b32_be; }
isz=$(fv 12); voff=$(fv 20); vsz=$(fv 28)
lim=$(( BOOT_OLD_MIB * MiB - 64 ))
# comparaison dans awk (nombres flottants) : un pied abime donne des valeurs hors de l'arithmetique du shell
if ! awk -v a="$isz" -v o="$voff" -v s="$vsz" -v l=$lim 'BEGIN { exit !(a > 0 && a <= l && o + s <= l && o >= a) }'; then
	ui_print "Le noyau en place ($isz octets) ne tient pas dans 10 Mio."
	abort "flasher d'abord un noyau 10 Mio (librehu_kernel_twrp_a9 / _a10), puis ce zip"
fi
ui_print "Noyau en place : $isz octets, tient dans 10 Mio"

if [ $RESUME = 1 ]; then
	ui_print "Reprise : copies verifiees de $R"
else
	free=$(b32_free_kb $R)
	[ -n "$free" ] && [ "$free" -gt 2300000 ] || abort "il faut 2,3 Go libres dans la memoire interne ($free Ko)"
	ui_print "1/4 Copies dans $R"
	rm -f $R/*
	[ "$(b32_rd $(( NEW_DTBO * MiB )) 4 | b32_hex)" = d7b7ab1e ] || abort "dtbo : en-tete inattendu"
	for x in tee1:$NEW_TEE1 tee2:$NEW_TEE2; do
		[ "$(b32_rd $(( ${x#*:} * MiB )) 4 | b32_hex)" = 88168858 ] || abort "${x%%:*} : en-tete MTK absent"
	done
	{ dd if=$B32_DISK bs=$MiB skip=$NEW_DTBO count=1 2>/dev/null; dd if=/dev/zero bs=$MiB count=7 2>/dev/null; } > $R/dtbo.img
	dd if=$B32_DISK of=$R/tee1.img bs=$MiB skip=$NEW_TEE1 count=5 2>/dev/null || abort "copie de tee1"
	dd if=$B32_DISK of=$R/tee2.img bs=$MiB skip=$NEW_TEE2 count=7 2>/dev/null || abort "copie de tee2"
	cp $B32_TMP/foot.bin $R/boot_footer.bin
	dd if=$B32_DISK of=$R/vendor.img bs=$MiB skip=$NEW_VEND count=$NEW_VEND_MIB 2>/dev/null || abort "copie de vendor"
	[ "$(wc -c < $R/vendor.img)" = $(( NEW_VEND_MIB * MiB )) ] || abort "copie de vendor incomplete (memoire pleine ?)"
	[ "$(wc -c < $R/dtbo.img)" = $(( 8 * MiB )) ] || abort "copie de dtbo incomplete"
	e2fsck -f -y $R/vendor.img >/dev/null 2>&1; [ $? -le 1 ] || abort "e2fsck de la copie de vendor"
	bs=$(b32_fsval $R/vendor.img "Block size")
	[ -n "$bs" ] || abort "copie de vendor illisible"
	resize2fs $R/vendor.img $(( OLD_VEND_MIB * MiB / bs )) >/dev/null 2>&1 || abort "resize2fs vendor (2048 Mio)"
	e2fsck -f -y $R/vendor.img >/dev/null 2>&1; [ $? -le 1 ] || abort "e2fsck vendor apres agrandissement"
	[ "$(wc -c < $R/vendor.img)" = $(( OLD_VEND_MIB * MiB )) ] || abort "vendor agrandi : taille inattendue"
	e2fsck -n -f $R/vendor.img >/dev/null 2>&1 || abort "vendor agrandi : e2fsck -n signale des erreurs"
	( cd $R && sha256sum dtbo.img tee1.img tee2.img boot_footer.bin vendor.img > SHA256SUMS ) || abort "empreintes des copies"
	ui_print "  copies faites et controlees (SHA256SUMS)"
fi

ui_print "2/4 Ecriture aux emplacements d'origine"
echo "retour $VERSION $(date)" > $R/IN_PROGRESS
b32_put $R/dtbo.img $OLD_DTBO
b32_put $R/tee1.img $OLD_TEE1
b32_put $R/tee2.img $OLD_TEE2
b32_footer $R/boot_footer.bin $BOOT_OLD_MIB
b32_cut before_vendor
b32_put $R/vendor.img $OLD_VEND

ui_print "3/4 Table de partitions d'origine"
b32_write_gpt shrink orig

ui_print "4/4 Nettoyage"
rm -f $R/*.img $R/IN_PROGRESS $R/SHA256SUMS $R/boot_footer.bin $B32_DIR/vendor_orig.img $B32_DIR/vendor_new.img
rmdir $R 2>/dev/null
[ -d $B32_DIR ] && mv $B32_DIR $B32_DIR-retour-$(date +%Y%m%d-%H%M%S)
rm -rf $B32_TMP
ui_print "  copies de vendor supprimees ; petites sauvegardes gardees dans $B32_DIR-retour-*"

ui_print " "
ui_print "Termine : boot 10 Mio, dtbo 8 Mio, tee1 / tee2 et vendor (2048 Mio) a leur place d'origine."
ui_print "Redemarrer dans TWRP (Redemarrer > Recovery), puis demarrer Android."
ui_print "Noyaux a utiliser : variantes 10 Mio (librehu_kernel_twrp_a9 / _a10)."
cleanup
exit 0
