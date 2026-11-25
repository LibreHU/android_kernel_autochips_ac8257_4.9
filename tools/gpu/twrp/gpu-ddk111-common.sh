
# --- Paquet Vulkan (PowerVR DDK 1.11), en plus de tools/twrp/twrp-common.sh
GPU_DIR=$VENMNT/librehu/ddk111

# Partition boot (essais hors autoradio : UJC_PART_boot=<fichier>)
boot_part() {
	_bp=${UJC_PART_boot:-}
	[ -n "$_bp" ] || for _p in /dev/block/by-name/boot $BYNAME/boot /dev/block/platform/*/*/by-name/boot; do
		[ -e "$_p" ] && { _bp=$_p; break; }
	done
	[ -n "$_bp" ] || abort "boot partition not found"
	echo "$_bp"
}

# Le noyau de la partition boot a-t-il le pilote DDK 1.11 ? Son device tree (non compresse, a la suite de
# Image.gz-dtb comme dans l'image du menu de demarrage) porte alors la propriete /firmware/android/librehu_gpu.
boot_has_ddk111() { grep -q -a librehu_gpu "$(boot_part)"; }

# Contexte SELinux d'un fichier ou dossier existant (toybox : "contexte chemin")
ctx_of() { ls -dZ "$1" 2>/dev/null | awk '{print $1}'; }
# $1 recoit le contexte de $2 (fichier de reference du vendor stock)
set_ctx() {
	[ -n "$UJC_TEST" ] && return
	_c=$(ctx_of "$2")
	case "$_c" in u:object_r:*) ;; *) abort "cannot read the SELinux context of $2";; esac
	chcon "$_c" "$1" || abort "cannot label $1"
}

# Fichiers d'un MANIFEST (lignes "<sha256>  /vendor/...") : chemin sur la partition vendor montee
manifest_paths() { awk '{print $2}' "$1" | sed "s#^/vendor#$VENMNT#"; }
