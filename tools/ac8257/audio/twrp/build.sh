#!/bin/sh
# Construit les zips TWRP audio de l'UJC201 (dans out/) :
#   ujc201_nodeepbuffer_install.zip / _restore.zip : sortie deep_buffer retiree (musique muette)
#   ujc201_viper_install.zip / _restore.zip         : pilote ViPER4Android RE sans Magisk, si le module est donne
# usage : tools/audio/twrp/build.sh [ViPER4Android-RE-vX.Y.Z.zip]   (module non-AIDL de ViPERFX_RE)
set -e
MOD=""; [ -n "$1" ] && { MOD=$(readlink -f "$1"); [ -f "$MOD" ] || { echo "$1 introuvable" >&2; exit 1; }; }
cd "$(dirname "$0")"
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
mkdir -p out
VER=""
if [ -n "$MOD" ]; then
	unzip -q -o "$MOD" module.prop LICENSE 'common/files/*' -d "$W/mod"
	VER=$(sed -n 's/^version=//p' "$W/mod/module.prop")
fi
# $1 = nom du zip, $2 = script
mkzip() {
	d=$W/$1; mkdir -p $d/META-INF/com/google/android
	{ echo '#!/sbin/sh'; echo "OUTFD=\$2; ZIPFILE=\$3; V4A_VERSION=$VER"; cat common.sh; cat $2; } \
		> $d/META-INF/com/google/android/update-binary
	echo "# dummy - voir update-binary" > $d/META-INF/com/google/android/updater-script
	rm -f out/$1.zip
	(cd $d && zip -q -X -r "$OLDPWD/out/$1.zip" .)
	echo "out/$1.zip"
}
mkzip ujc201_nodeepbuffer_install nodeep_install.sh
mkzip ujc201_nodeepbuffer_restore nodeep_restore.sh
if [ -n "$MOD" ]; then
	mkdir -p $W/ujc201_viper_install/files
	cp "$W"/mod/common/files/libv4a_re_*.so "$W/mod/LICENSE" $W/ujc201_viper_install/files/
	mkzip ujc201_viper_install viper_install.sh
	mkzip ujc201_viper_restore viper_restore.sh
	echo "(ViPER $VER)"
fi
