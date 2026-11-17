# UJC201 - noyau LibreHU 4.19 ThinLTO, tout integre (Wi-Fi, BT, GPS, FM dans le noyau, aucun module a installer) :
# zip TWRP librehu_kernel32_twrp_<v>.zip. Image de 32 Mio : partition boot agrandie au prealable par
# librehu_boot32_twrp.zip (refuse sur la table d'origine, boot 10 Mio). Verifie avant d'ecrire : empreinte, taille de
# la partition, version d'Android (a9 / a10), memoire interne montee (sauvegarde boot-<date>.img).

ui_print "UJC201 - noyau LibreHU ThinLTO (boot 32 Mio, tout integre)"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
F=${UJC_TMP:-/tmp}/librehu_kernel32; rm -rf $F; mkdir -p $F
unzip -o -q "$ZIPFILE" 'files/*' -d $F || abort "extraction du zip impossible"
F=$F/files
VARIANT=$(cat $F/android)
case "$(cat $F/branch)" in
experimental-*)
	ui_print "Noyau EXPERIMENTAL : a n'installer qu'apres un essai reussi"
	ui_print "avec recovery_test_ac8257_$VARIANT-lto32.img (docs/K419.md du depot du noyau).";;
esac
[ "$(sha256sum $F/boot.img | cut -d' ' -f1)" = "$(cut -d' ' -f1 $F/boot.img.sha256)" ] || abort "image de boot abimee (empreinte)"
_bp=${UJC_PART_boot:-}
[ -n "$_bp" ] || for _p in /dev/block/by-name/boot $BYNAME/boot /dev/block/platform/*/*/by-name/boot; do
	[ -e "$_p" ] && { _bp=$_p; break; }
done
[ -n "$_bp" ] || abort "partition boot introuvable"
_bsz=$(blockdev --getsize64 "$_bp" 2>/dev/null)
[ -z "$_bsz" ] && [ -f "$_bp" ] && _bsz=$(stat -c %s "$_bp")
if [ -z "$_bsz" ] || [ "$_bsz" -lt $(stat -c %s $F/boot.img) ]; then
	ui_print "Partition boot de $(( ${_bsz:-0} / 1048576 )) Mio : ce noyau demande 32 Mio."
	ui_print "Flasher d'abord librehu_boot32_twrp.zip, redemarrer dans TWRP, puis ce zip."
	abort "partition boot trop petite : rien n'a ete modifie"
fi
mount_rw
android_check
[ "$VARIANT" = a9 ] && require_no_verity vendor
[ -d $SDCARD ] || abort "memoire interne non montee : sauvegardes impossibles, rien n'a ete modifie"

flash_image boot.img boot
[ "$VARIANT" = a9 ] && vintf_accept_419
rm -rf ${F%/files}

ui_print " "
ui_print "Termine. Redemarrer (menu de demarrage TWRP / Android / Fastboot, Android au bout de 3 s)."
ui_print "Wi-Fi / BT / GPS / FM sont dans le noyau : le noyau refuse les modules de /vendor/lib/modules"
ui_print "(normal). Le module KernelSU librehu_connectivity n'est plus utile avec ce noyau : le retirer."
ui_print "Retour arriere : boot-<date>.img ci-dessus (TWRP : Install, Install Image, Boot)"
ui_print "ou un noyau 10 Mio (librehu_kernel_twrp_<v>.zip, il marche aussi sur la partition 32 Mio)."
cleanup
exit 0
