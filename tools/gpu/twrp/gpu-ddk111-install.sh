# UJC201 - paquet Vulkan LibreHU : espace utilisateur PowerVR DDK 1.11 (Vulkan 1.1) pour le vendor stock, zip TWRP
# librehu_gpu_ddk111_twrp.zip (tools/gpu/make_vulkan_package.py). Ne remplace AUCUN fichier du vendor stock : ajoute
# /vendor/librehu/ddk111 (copies DDK 1.11 montees par-dessus les fichiers DDK 1.10), le pilote Vulkan, libgex.so, le
# firmware rgx_ddk111.fw.22.68.54.30 et /vendor/etc/init/librehu_gpu_ddk111.rc, qui n'agit que sous un noyau avec le
# pilote DDK 1.11 (ro.boot.librehu_gpu=1.11). Sous tout autre noyau, rien ne change. Verifie tout avant d'ecrire.

ui_print "UJC201 - LibreHU Vulkan package (PowerVR DDK 1.11 user space)"
ui_print "version $VERSION"
ui_print "-----------------------------------------------------------"
T=${UJC_TMP:-/tmp}/librehu_gpu_ddk111; rm -rf $T; mkdir -p $T
unzip -o -q "$ZIPFILE" 'files/*' -d $T || abort "cannot extract the zip"
F=$T/files
cd $F/payload || abort "payload missing from the zip"
while read _sum _path; do
	[ "$(sha256sum ".${_path}" | cut -d' ' -f1)" = "$_sum" ] || abort "damaged file in the zip: $_path"
done < $F/MANIFEST
cd /

mount_rw
require_vendor_unverified

# --- Le vendor est-il bien celui pour lequel ce paquet a ete verifie ?
grep -q '^ro.board.platform=ac8257$' $VENMNT/build.prop || abort "vendor without ro.board.platform=ac8257: not the UJC201 stock vendor"
while read _sum _path; do
	_f=$VENMNT${_path#/vendor}
	[ -f "$_f" ] || abort "$_path missing from the vendor"
	if [ "$(sha256sum "$_f" | cut -d' ' -f1)" != "$_sum" ]; then
		ui_print "$_path differs from the stock UJC201 vendor this package was checked against."
		abort "unknown vendor build: nothing was changed"
	fi
done < $F/unit_vendor.sha256
ui_print "Stock libraries used by the package (libged, libgralloc_extra, libgpu_aux...): OK"
while read _src _dst; do
	[ -f "$VENMNT${_dst#/vendor}" ] || abort "$_dst missing: the package mounts its DDK 1.11 copy over it"
done < $F/bound
for _v in $VENMNT/lib/hw/vulkan.*.so $VENMNT/lib64/hw/vulkan.*.so; do
	case "$_v" in *'*'*|*/vulkan.librehu.so) ;; *) abort "this vendor already has a Vulkan driver ($_v): nothing was changed";; esac
done
for _p in $VENMNT/build.prop $VENMNT/default.prop $(sysdir)/build.prop; do
	[ -f "$_p" ] && grep -q '^ro.hardware.vulkan=' "$_p" && abort "ro.hardware.vulkan is already set in $_p: nothing was changed"
done
ui_print "No Vulkan driver nor ro.hardware.vulkan on this vendor: OK"

# --- Place libre (paquet + 8 Mio)
_need=$(( $(du -sk $F/payload | cut -f1) + 8192 ))
_free=$(df -k $VENMNT | awk 'NR==2 {print $4}')
[ -n "$_free" ] && [ "$_free" -gt "$_need" ] || abort "not enough room on /vendor (${_free:-?} KiB free, $_need KiB needed)"

# --- Mise a jour : les fichiers d'une installation precedente partent d'abord
if [ -f $GPU_DIR/MANIFEST ]; then
	ui_print "Previous installation $(cat $GPU_DIR/version 2>/dev/null): replaced"
	manifest_paths $GPU_DIR/MANIFEST | while read _f; do rm -f "$_f"; done
fi

# --- Copie
cd $F/payload
find vendor -type f | while read _rel; do
	_dst=$VENMNT/${_rel#vendor/}
	mkdir -p "$(dirname "$_dst")"
	cp -f "$_rel" "$_dst" || abort "cannot write $_dst (storage full?)"
	chown 0:0 "$_dst"; chmod 0644 "$_dst"
done
cd /
if [ ! -d $VENMNT/odm/etc/permissions ]; then
	mkdir -p $VENMNT/odm/etc/permissions || abort "cannot create /vendor/odm/etc/permissions"
	touch $GPU_DIR/created_odm_permissions
fi
chown 0:0 $VENMNT/odm/etc/permissions; chmod 0755 $VENMNT/odm/etc/permissions
find $VENMNT/librehu -type d -exec chown 0:0 {} \; -exec chmod 0755 {} \;

# --- Contextes SELinux : ceux des fichiers stock equivalents
while read _src _dst; do
	set_ctx "$VENMNT${_src#/vendor}" "$VENMNT${_dst#/vendor}"
done < $F/bound
for _a in lib lib64; do
	set_ctx $VENMNT/$_a/libgex.so $VENMNT/$_a/libgpu_aux.so
	set_ctx $VENMNT/$_a/hw/vulkan.librehu.so $VENMNT/$_a/egl/libGLESv2_mtk.so
	set_ctx $GPU_DIR/$_a $VENMNT/$_a
	set_ctx $GPU_DIR/$_a/egl $VENMNT/$_a/egl
	set_ctx $GPU_DIR/$_a/hw $VENMNT/$_a/hw
done
set_ctx $VENMNT/librehu $VENMNT/lib64
set_ctx $GPU_DIR $VENMNT/lib64
set_ctx $VENMNT/firmware/rgx_ddk111.fw.22.68.54.30 $VENMNT/firmware/rgx.fw.22.68.54.30
set_ctx $VENMNT/etc/init/librehu_gpu_ddk111.rc $VENMNT/etc/init/android.hardware.graphics.allocator@2.0-service.rc
set_ctx $GPU_DIR/permissions $VENMNT/etc/permissions
for _x in $GPU_DIR/permissions/*.xml; do set_ctx "$_x" $VENMNT/etc/permissions/handheld_core_hardware.xml; done
set_ctx $VENMNT/odm/etc/permissions $VENMNT/odm/etc

# --- Relecture
while read _sum _path; do
	[ "$(sha256sum "$VENMNT${_path#/vendor}" | cut -d' ' -f1)" = "$_sum" ] || abort "check after writing failed: $_path"
done < $F/MANIFEST
cp -f $F/MANIFEST $F/version $GPU_DIR/
chown 0:0 $GPU_DIR/MANIFEST $GPU_DIR/version; chmod 0644 $GPU_DIR/MANIFEST $GPU_DIR/version
set_ctx $GPU_DIR/MANIFEST $VENMNT/etc/permissions/handheld_core_hardware.xml
set_ctx $GPU_DIR/version $VENMNT/etc/permissions/handheld_core_hardware.xml
rm -rf $T

ui_print " "
ui_print "Done: $(wc -l < $GPU_DIR/MANIFEST) files on /vendor, the stock ones unchanged."
ui_print "Nothing changes until a kernel with the DDK 1.11 driver boots:"
ui_print "flash librehu_kernel32_twrp_<v>.zip of the rc-4.19.325-vulkan release now."
ui_print "Any other kernel (ROM update, stable, rc-4.19.325) keeps the stock GPU stack."
ui_print "To remove: librehu_gpu_ddk111_remove_twrp.zip, under another kernel."
cleanup
exit 0
