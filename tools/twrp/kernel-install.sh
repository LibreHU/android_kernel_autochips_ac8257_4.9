# UJC201 - noyau LibreHU + ses modules Wi-Fi / BT / GPS / FM (zip TWRP, sans KernelSU), d'apres le zip kernel-stable
# de LibreHU-flashablesZips. Tout est verifie avant d'ecrire : empreintes, noyau de l'image = celui des modules,
# dm-verity coupe sur /vendor, memoire interne montee (sauvegardes). Puis boot (sauvegarde boot-<date>.img), puis les
# modules (conn-modules.sh). Pour la branche experimental-4.19.325 : seulement apres un essai recovery reussi.

ui_print "UJC201 - noyau LibreHU + modules Wi-Fi / BT / GPS / FM"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
conn_extract
case "$(cat $F/branch)" in
experimental-*)
	ui_print "Noyau EXPERIMENTAL : a n'installer qu'apres un essai reussi"
	ui_print "avec recovery_test_ac8257.img (docs/K419.md du depot du noyau).";;
esac
[ "$(sha256sum $F/boot.img | cut -d' ' -f1)" = "$(cut -d' ' -f1 $F/boot.img.sha256)" ] || abort "image de boot abimee (empreinte)"
grep -q "^$(kernel_sum $F/boot.img) " $F/kernels.sha256 || abort "le noyau de l'image n'est pas celui des modules"
mount_rw
require_no_verity vendor
[ -d $SDCARD ] || abort "memoire interne non montee : sauvegardes impossibles, rien n'a ete modifie"

flash_image boot.img boot
conn_check_boot
conn_install

ui_print " "
ui_print "Termine. Redemarrer (menu de demarrage TWRP / Android / Fastboot, Android au bout de 3 s)."
ui_print "Retour arriere : boot-<date>.img ci-dessus (TWRP : Install, Install Image, Boot)"
ui_print "et zip conn-modules-stable-restore (LibreHU-flashablesZips) pour les modules d'origine."
cleanup
exit 0
