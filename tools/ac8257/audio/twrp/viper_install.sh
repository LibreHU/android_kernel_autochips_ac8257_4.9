# UJC201 - pilote ViPER4Android RE sans Magisk (zip TWRP)
# Installe libv4a_re.so dans /vendor/lib[64]/soundfx et declare l'effet dans les audio_effects.xml / .conf
# de vendor, ou de system s'il n'y en a pas dans vendor (librairie "v4a_re", effet "v4a_standard_re", UUID 90380da3-...) : ce que fait le module Magisk ViPERFX_RE.
# Sauvegarde : <fichier>.v4a-orig a cote + /sdcard/UJC201_viper_backup/<date>/.
# dm-verity : si /vendor (ou /system quand sa configuration est modifiee) est verifie (fs_mgr "avb"/"verify") et que
# vbmeta ne l'a pas desactive (adb disable-verity), s'arrete sans rien modifier.
# Retour : zip ujc201_viper_restore.
# Avec KernelSU (Next), le module d'origine s'installe sans toucher aux partitions.

UUID=90380da3-8536-4744-a6a3-5731970e640f
ui_print "UJC201 - ViPER4Android RE (pilote $V4A_VERSION) sans Magisk"
ui_print "-----------------------------------------------------------"
mount_rw
[ -d $SDCARD ] || abort "memoire interne non montee (/data dechiffre ?) : pas de copie de sauvegarde possible"
TMPD=${UJC_TMP:-/tmp}/ujc201_v4a; rm -rf $TMPD; mkdir -p $TMPD
unzip -o -q "$ZIPFILE" 'files/*' -d $TMPD || abort "extraction du zip impossible"

SYS=$(sysdir)
SDK=$(grep -m1 '^ro.build.version.sdk=' $SYS/build.prop 2>/dev/null | cut -d= -f2)
ui_print "Android SDK ${SDK:-?}"
case "$SDK" in 3[4-9]|[4-9][0-9]) abort "Android 14+ : il faut la variante AIDL du pilote";; esac

CFGS=$(effect_configs)
[ -n "$CFGS" ] || abort "aucun audio_effects.xml / .conf dans vendor ni system"

# --- dm-verity : vendor (pilote), et system si sa configuration est modifiee
case "$CFGS" in $(sysdir)/*) require_no_verity vendor system;; *) require_no_verity vendor;; esac

STAMP=$(date +%Y%m%d-%H%M%S)
BKDIR=$SDCARD/UJC201_viper_backup/$STAMP
mkdir -p $BKDIR || abort "dossier $BKDIR impossible"

# --- pilote
LIBS=0
for arch in lib lib64; do
	d=$VENMNT/$arch/soundfx
	[ -d $d ] || continue
	so=$TMPD/files/libv4a_re_armeabi-v7a.so; [ $arch = lib64 ] && so=$TMPD/files/libv4a_re_arm64-v8a.so
	[ -f $so ] || abort "$so absent du zip"
	cp $so $d/libv4a_re.so || abort "copie dans $d impossible (vendor plein ?)"
	chmod 0644 $d/libv4a_re.so; chown 0:0 $d/libv4a_re.so 2>/dev/null
	[ -z "$UJC_TEST" ] && chcon u:object_r:vendor_file:s0 $d/libv4a_re.so
	ui_print "$d/libv4a_re.so"
	LIBS=$((LIBS + 1))
done
[ $LIBS -gt 0 ] || abort "$VENMNT/lib/soundfx introuvable"

# --- configuration des effets
TMP=$TMPD/cfg
for f in $CFGS; do
	ui_print " "
	ui_print "$f"
	# version d'origine : la sauvegarde si deja installe (reinstallation propre)
	SRC=$f; [ -f "$f.v4a-orig" ] && SRC=$f.v4a-orig
	case $f in
	*.xml)
		grep -q '<libraries>' $SRC && grep -q '<effects>' $SRC || { ui_print "  pas de <libraries> / <effects> : ignore"; continue; }
		sed -e '/v4a_standard_re/d' -e '/"v4a_re"/d' \
		    -e "s|<libraries>|<libraries>\n        <library name=\"v4a_re\" path=\"libv4a_re.so\"/>|" \
		    -e "s|<effects>|<effects>\n        <effect name=\"v4a_standard_re\" library=\"v4a_re\" uuid=\"$UUID\"/>|" \
		    $SRC > $TMP || abort "sed"
		;;
	*.conf)
		grep -q '^libraries {' $SRC && grep -q '^effects {' $SRC || { ui_print "  pas de libraries / effects : ignore"; continue; }
		sed -e '/v4a_standard_re {/,/}/d' -e '/v4a_re {/,/}/d' \
		    -e "s|^libraries {|libraries {\n  v4a_re {\n    path /vendor/lib/soundfx/libv4a_re.so\n  }|" \
		    -e "s|^effects {|effects {\n  v4a_standard_re {\n    library v4a_re\n    uuid $UUID\n  }|" \
		    $SRC > $TMP || abort "sed"
		;;
	esac
	grep -q "$UUID" $TMP || abort "modification de $f echouee"
	if [ ! -f "$f.v4a-orig" ]; then
		cp -p "$f" "$f.v4a-orig" || abort "sauvegarde $f.v4a-orig impossible"
		ui_print "  sauvegarde : $f.v4a-orig"
	fi
	mkdir -p "$BKDIR$(dirname "$f")" && cp -p "$SRC" "$BKDIR$f"
	# reecriture sur place : garde l'inode, les droits et le contexte SELinux
	cat $TMP > "$f" || abort "ecriture de $f impossible (restaurer $f.v4a-orig)"
	cmp -s $TMP "$f" || abort "verification de $f echouee"
	ui_print "  effet v4a_standard_re declare"
done
rm -rf $TMPD

ui_print " "
ui_print "Termine. Sauvegarde : $BKDIR"
ui_print "Redemarrer, puis ouvrir ViPER4Android : le pilote doit etre \"charge\"."
ui_print "Retour arriere : flasher ujc201_viper_restore.zip"
cleanup
exit 0
