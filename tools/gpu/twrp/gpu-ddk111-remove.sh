# UJC201 - retrait du paquet Vulkan LibreHU (librehu_gpu_ddk111_remove_twrp.zip) : enleve les fichiers que
# librehu_gpu_ddk111_twrp.zip a ajoutes (/vendor/librehu/ddk111/MANIFEST). Refuse tant que la partition boot contient
# un noyau avec le pilote DDK 1.11 : sans ce paquet, ce pilote n'aurait plus d'espace utilisateur (pas d'affichage).

ui_print "UJC201 - LibreHU Vulkan package: removal"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
if boot_has_ddk111; then
	ui_print "The kernel in the boot partition has the DDK 1.11 driver: without this package"
	ui_print "Android would have no display. Flash another kernel first (ROM update,"
	ui_print "librehu_kernel32_twrp_<v>.zip of rc-4.19.325, stable-4.9.337), then this zip."
	abort "DDK 1.11 kernel in boot: nothing was changed"
fi
mount_rw
require_vendor_unverified
[ -f $GPU_DIR/MANIFEST ] || abort "the package is not installed (no /vendor/librehu/ddk111/MANIFEST)"
manifest_paths $GPU_DIR/MANIFEST | while read _f; do rm -f "$_f"; done
if [ -f $GPU_DIR/created_odm_permissions ]; then
	rmdir $VENMNT/odm/etc/permissions 2>/dev/null || ui_print "/vendor/odm/etc/permissions not empty: kept"
fi
rm -f $GPU_DIR/MANIFEST $GPU_DIR/version $GPU_DIR/created_odm_permissions
# dossiers du paquet, vides, du plus profond au moins profond
find $VENMNT/librehu -depth -type d -exec rmdir {} \; 2>/dev/null
[ -e $VENMNT/etc/init/librehu_gpu_ddk111.rc ] && abort "librehu_gpu_ddk111.rc still there"
ui_print " "
ui_print "Done: the vendor is back to its stock GPU stack (DDK 1.10, no Vulkan)."
cleanup
exit 0
