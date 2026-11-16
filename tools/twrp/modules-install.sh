# UJC201 - modules Wi-Fi / BT / GPS / FM recompiles pour un noyau LibreHU (zip TWRP, sans KernelSU), d'apres le zip
# conn-modules-stable de LibreHU-flashablesZips. Modules seuls : la partition boot doit deja contenir ce noyau
# (librehu_kernel_twrp.zip ou une image boot de la meme release). dm-verity : s'arrete sans rien modifier si /vendor
# est verifie.

ui_print "UJC201 - modules Wi-Fi / BT / GPS / FM du noyau LibreHU"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
conn_extract
conn_check_boot
mount_rw
android_check
require_no_verity vendor
conn_install

ui_print " "
ui_print "Termine. Redemarrer : les scripts du vendor chargent ces modules comme avant."
ui_print "Retour aux modules d'origine (avant tout autre noyau) : zip conn-modules-stable-restore (LibreHU-flashablesZips)."
cleanup
exit 0
