# Copie de common/twrp-common.sh de LibreHU-flashablesZips (memes fonctions que ses zips TWRP).
# Fonctions communes aux zips TWRP audio (incluses dans update-binary par build.sh)
ui_print() { if [ -n "$OUTFD" ] && [ -e /proc/self/fd/$OUTFD ] && [ -z "$UJC_TEST" ]; then echo "ui_print $1" > /proc/self/fd/$OUTFD; echo "ui_print" > /proc/self/fd/$OUTFD; else echo "$1"; fi; }
abort() { ui_print "ERREUR : $1"; cleanup; exit 1; }

BYNAME=/dev/block/platform/bootdevice/by-name
SYSMNT=${UJC_SYSMNT:-/system_root}
VENMNT=${UJC_VENMNT:-/vendor}
SDCARD=${UJC_SDCARD:-/data/media/0}
VBMETA=${UJC_VBMETA:-$BYNAME/vbmeta}
MOUNTED_SYS=0; MOUNTED_VEN=0

cleanup() {
	[ -n "$UJC_TEST" ] && return
	sync
	[ $MOUNTED_VEN = 1 ] && umount $VENMNT 2>/dev/null
	[ $MOUNTED_SYS = 1 ] && umount $SYSMNT 2>/dev/null
}

# system et vendor en lecture-ecriture
mount_rw() {
	[ -n "$UJC_TEST" ] && return
	for _pm in system:$SYSMNT vendor:$VENMNT; do
		part=${_pm%%:*}; mnt=${_pm#*:}
		umount $mnt 2>/dev/null
		[ $part = system ] && umount /system 2>/dev/null
		mkdir -p $mnt
		blockdev --setrw $BYNAME/$part 2>/dev/null
		mount -t ext4 -o rw $BYNAME/$part $mnt || abort "montage de $part en ecriture impossible"
		[ $part = system ] && MOUNTED_SYS=1 || MOUNTED_VEN=1
	done
}

# Dossier "system" de la partition system (system-as-root : /system_root/system)
sysdir() { [ -d $SYSMNT/system/etc ] && echo $SYSMNT/system || echo $SYSMNT; }

# Fichiers de configuration des effets audio : ceux de vendor (Android 9 les lit en priorite), sinon ceux de system
effect_configs() {
	for _d in $VENMNT/etc $(sysdir)/etc; do
		_found=""
		for _c in $_d/audio_effects*.xml $_d/audio_effects*.conf; do
			[ -f "$_c" ] && [ ! -L "$_c" ] && echo "$_c" && _found=1
		done
		[ -n "$_found" ] && return
	done
}

# --- dm-verity de /vendor (AVB) : modifier vendor sous verity actif = Android ne demarre plus
# Drapeaux fs_mgr d'une partition : fstab du device tree (montage anticipe d'Android 9) ou fstab de vendor.
# $1 = vendor ou system
fsmgr_flags() {
	_dt=${UJC_DT:-/proc/device-tree}/firmware/android/fstab/$1/fsmgr_flags
	[ -f $_dt ] && tr '\0' ' ' < $_dt
	for _fs in $VENMNT/etc/fstab.*; do
		[ -f "$_fs" ] && grep -E "^[^#]*[[:space:]]/$1[[:space:]]" "$_fs" | awk '{print $NF}'
	done
}
# 1 octet (decimal) de vbmeta a l'offset $1
vbmeta_byte() { dd if=$VBMETA bs=1 skip=$1 count=1 2>/dev/null | od -An -tu1 | tr -d ' '; }
vbmeta_ok() { [ "$(dd if=$VBMETA bs=1 count=4 2>/dev/null)" = AVB0 ]; }
# AvbVBMetaImageHeader.flags (offset 120, big-endian) : bit 0 = HASHTREE_DISABLED (adb disable-verity)
hashtree_disabled() { v=$(vbmeta_byte 123); [ -n "$v" ] && [ $((v & 1)) = 1 ]; }

# audio_policy_configuration.xml utilise : meme ordre de recherche que l'AudioPolicyManager d'Android 9
policy_file() {
	for _pf in $VENMNT/etc/audio/audio_policy_configuration.xml $VENMNT/etc/audio_policy_configuration.xml \
		$(sysdir)/etc/audio_policy_configuration.xml; do
		[ -f $_pf ] && echo $_pf && return
	done
}

# Arrete sans rien modifier si une des partitions ($@ : vendor, system) est sous dm-verity actif
require_no_verity() {
	_all=""
	for _p in "$@"; do
		_fl=$(fsmgr_flags $_p | tr '\n' ' ')
		ui_print "fs_mgr /$_p : ${_fl:-(introuvable)}"
		_all="$_all $_fl"
	done
	if ! echo "$_all" | grep -qE 'avb|verify'; then
		ui_print "partitions non verifiees (pas de avb / verify) : OK"
	elif vbmeta_ok && hashtree_disabled; then
		ui_print "dm-verity deja desactive dans vbmeta : OK"
	else
		ui_print " "
		ui_print "dm-verity est actif : modifier la partition empecherait Android de demarrer."
		ui_print "Rien n'a ete modifie. Pour le desactiver vous-meme : sauvegarder VBMeta dans TWRP,"
		ui_print "puis depuis un PC en fastboot : fastboot --disable-verity flash vbmeta vbmeta.img"
		ui_print "(avec la sauvegarde renommee), demarrer Android une fois, puis reflasher ce zip."
		abort "dm-verity actif"
	fi
}

# Partition d'un fichier : vendor ou system
part_of() { case "$1" in $VENMNT/*) echo vendor;; *) echo system;; esac; }

# --- Image de partition du zip (files/<image> + files/<image>.sha256) : $1 = image, $2 = partition (boot...)
# Verifie l'empreinte et la taille, sauvegarde la partition actuelle dans $SDCARD/LibreHU/backup, ecrit, relit.
flash_image() {
	_img=$1; _pn=$2
	eval _part=\${UJC_PART_$_pn:-}  # essais hors autoradio : UJC_PART_boot=<fichier>
	[ -n "$_part" ] || for _p in /dev/block/by-name/$_pn /dev/block/platform/*/by-name/$_pn /dev/block/platform/*/*/by-name/$_pn; do
		[ -e "$_p" ] && { _part=$_p; break; }
	done
	[ -n "$_part" ] || abort "partition $_pn introuvable"
	_t=${UJC_TMP:-/tmp}/librehu_img_$_pn; rm -rf $_t; mkdir -p $_t
	unzip -o -q "$ZIPFILE" "files/$_img" "files/$_img.sha256" -d $_t || abort "extraction du zip impossible"
	_e=$(cut -d' ' -f1 $_t/files/$_img.sha256)
	[ "$(sha256sum $_t/files/$_img | cut -d' ' -f1)" = "$_e" ] || abort "image abimee (empreinte)"
	_isz=$(stat -c %s $_t/files/$_img)
	_psz=$(blockdev --getsize64 "$_part" 2>/dev/null)
	[ -z "$_psz" ] && [ -f "$_part" ] && _psz=$(stat -c %s "$_part")
	[ -n "$_psz" ] && [ "$_isz" -le "$_psz" ] || abort "image ($_isz octets) plus grande que la partition ($_psz)"
	if [ -d $SDCARD ]; then
		mkdir -p $SDCARD/LibreHU/backup
		_b=$SDCARD/LibreHU/backup/$_pn-$(date +%Y%m%d-%H%M%S).img
		dd if="$_part" of="$_b" bs=1048576 2>/dev/null || abort "sauvegarde de $_pn impossible (memoire pleine ?)"
		ui_print "Sauvegarde : $_b"
	else
		ui_print "Memoire interne non montee : pas de sauvegarde de $_pn"
	fi
	blockdev --setrw "$_part" 2>/dev/null
	dd if=$_t/files/$_img of="$_part" bs=1048576 2>/dev/null || abort "ecriture de $_part impossible"
	sync
	[ "$(head -c $_isz "$_part" | sha256sum | cut -d' ' -f1)" = "$_e" ] || abort "verification apres ecriture echouee : reflasher la sauvegarde"
	rm -rf $_t
	ui_print "$_img ecrit dans $_part"
}
