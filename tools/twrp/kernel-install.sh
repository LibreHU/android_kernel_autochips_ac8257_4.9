# UJC201 - noyau LibreHU + ses modules Wi-Fi / BT / GPS / FM (zip TWRP, sans KernelSU), d'apres le zip kernel-stable
# de LibreHU-flashablesZips. Tout est verifie avant d'ecrire : empreintes, noyau de l'image = celui des modules,
# dm-verity coupe sur /vendor, memoire interne montee (sauvegardes). Puis boot (sauvegarde boot-<date>.img), puis les
# modules (conn-modules.sh). Pour la branche experimental-4.19.325 : seulement apres un essai recovery reussi.

ui_print "UJC201 - LibreHU kernel + Wi-Fi / BT / GPS / FM modules"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
conn_extract
VARIANT=$(cat $F/android)	# lu avant conn_install, qui efface $F
case "$(cat $F/branch)" in
experimental-*)
	ui_print "EXPERIMENTAL kernel: install it only after a successful try"
	ui_print "with recovery_test_ac8257_$(cat $F/android).img (docs/K419.md of the kernel repository).";;
esac
[ "$(sha256sum $F/boot.img | cut -d' ' -f1)" = "$(cut -d' ' -f1 $F/boot.img.sha256)" ] || abort "damaged boot image (checksum)"
grep -q "^$(kernel_sum $F/boot.img) " $F/kernels.sha256 || abort "the kernel of the image is not the one of the modules"
mount_rw
android_check
require_no_verity vendor
[ -d $SDCARD ] || abort "internal storage not mounted: no backup possible, nothing was changed"

flash_image boot.img boot
conn_check_boot
conn_install
[ "$VARIANT" = a9 ] && vintf_accept_419

ui_print " "
ui_print "Done. Reboot (boot menu TWRP / Android / Fastboot, Android after 3 s)."
ui_print "To go back: boot-<date>.img above (TWRP: Install, Install Image, Boot)"
ui_print "and the conn-modules-stable-restore zip (LibreHU-flashablesZips) for the original modules."
cleanup
exit 0
