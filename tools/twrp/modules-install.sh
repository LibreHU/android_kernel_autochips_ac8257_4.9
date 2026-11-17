# UJC201 - modules Wi-Fi / BT / GPS / FM recompiles pour un noyau LibreHU (zip TWRP, sans KernelSU), d'apres le zip
# conn-modules-stable de LibreHU-flashablesZips. Modules seuls : la partition boot doit deja contenir ce noyau
# (librehu_kernel_twrp.zip ou une image boot de la meme release). dm-verity : s'arrete sans rien modifier si /vendor
# est verifie.

ui_print "UJC201 - Wi-Fi / BT / GPS / FM modules of the LibreHU kernel"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
conn_extract
conn_check_boot
mount_rw
android_check
require_no_verity vendor
conn_install

ui_print " "
ui_print "Done. Reboot: the vendor scripts load these modules as before."
ui_print "Back to the original modules (before any other kernel): conn-modules-stable-restore zip (LibreHU-flashablesZips)."
cleanup
exit 0
