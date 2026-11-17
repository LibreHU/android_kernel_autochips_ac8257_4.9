# UJC201 - noyau LibreHU 4.19 ThinLTO, tout integre (Wi-Fi, BT, GPS, FM dans le noyau, aucun module a installer) :
# zip TWRP librehu_kernel32_twrp_<v>.zip. Image de 32 Mio : partition boot agrandie au prealable par
# librehu_boot32_twrp.zip (refuse sur la table d'origine, boot 10 Mio). Verifie avant d'ecrire : empreinte, taille de
# la partition, version d'Android (a9 / a10), memoire interne montee (sauvegarde boot-<date>.img).

ui_print "UJC201 - LibreHU ThinLTO kernel (32 MiB boot, everything built in)"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
F=${UJC_TMP:-/tmp}/librehu_kernel32; rm -rf $F; mkdir -p $F
unzip -o -q "$ZIPFILE" 'files/*' -d $F || abort "cannot extract the zip"
F=$F/files
VARIANT=$(cat $F/android)
case "$(cat $F/branch)" in
experimental-*)
	ui_print "EXPERIMENTAL kernel: install it only after a successful try"
	ui_print "with recovery_test_ac8257_$VARIANT-lto32.img (docs/K419.md of the kernel repository).";;
esac
[ "$(sha256sum $F/boot.img | cut -d' ' -f1)" = "$(cut -d' ' -f1 $F/boot.img.sha256)" ] || abort "damaged boot image (checksum)"
_bp=${UJC_PART_boot:-}
[ -n "$_bp" ] || for _p in /dev/block/by-name/boot $BYNAME/boot /dev/block/platform/*/*/by-name/boot; do
	[ -e "$_p" ] && { _bp=$_p; break; }
done
[ -n "$_bp" ] || abort "boot partition not found"
_bsz=$(blockdev --getsize64 "$_bp" 2>/dev/null)
[ -z "$_bsz" ] && [ -f "$_bp" ] && _bsz=$(stat -c %s "$_bp")
if [ -z "$_bsz" ] || [ "$_bsz" -lt $(stat -c %s $F/boot.img) ]; then
	ui_print "Boot partition of $(( ${_bsz:-0} / 1048576 )) MiB: this kernel needs 32 MiB."
	ui_print "Flash librehu_boot32_twrp.zip first, reboot into TWRP, then this zip."
	abort "boot partition too small: nothing was changed"
fi
mount_rw
android_check
[ "$VARIANT" = a9 ] && require_no_verity vendor
[ -d $SDCARD ] || abort "internal storage not mounted: no backup possible, nothing was changed"

flash_image boot.img boot
[ "$VARIANT" = a9 ] && vintf_accept_419
rm -rf ${F%/files}

ui_print " "
ui_print "Done. Reboot (boot menu TWRP / Android / Fastboot, Android after 3 s)."
ui_print "Wi-Fi / BT / GPS / FM are built into the kernel: it refuses the modules of /vendor/lib/modules"
ui_print "(expected). The KernelSU module librehu_connectivity is useless with this kernel: remove it."
ui_print "To go back: boot-<date>.img above (TWRP: Install, Install Image, Boot)"
ui_print "or a 10 MiB kernel (librehu_kernel_twrp_<v>.zip, it also works on the 32 MiB partition)."
cleanup
exit 0
