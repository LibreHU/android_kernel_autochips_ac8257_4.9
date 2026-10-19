# UJC201 - retire la sortie audio "deep_buffer" (zip TWRP), comme tools/audio/no_deep_buffer.sh mais sans adb root
# Sur cette unite la sortie deep_buffer (PCM 3, MultiMedia2) reste muette : toute la musique passe alors par
# "primary" (celle des sonneries, qui marche). Voir docs/audio_deep_buffer.md.
# Fichier : le premier audio_policy_configuration.xml dans l'ordre de recherche d'Android 9.
# Sauvegarde : <fichier>.nodeep-orig a cote + /sdcard/UJC201_audio_backup/<date>/. Retour : ujc201_nodeepbuffer_restore.

ui_print "UJC201 - sortie audio deep_buffer retiree"
ui_print "------------------------------------------"
mount_rw
[ -d $SDCARD ] || abort "memoire interne non montee (/data dechiffre ?) : pas de copie de sauvegarde possible"
f=$(policy_file)
[ -n "$f" ] || abort "aucun audio_policy_configuration.xml"
ui_print "Fichier utilise : $f"
SRC=$f; [ -f "$f.nodeep-orig" ] && SRC=$f.nodeep-orig
if ! grep -q deep_buffer "$SRC"; then
	ui_print "deep_buffer absent : rien a faire."
	cleanup; exit 0
fi
require_no_verity $(part_of $f)

TMP=${UJC_TMP:-/tmp}/ujc201_policy.xml
# Bloc <mixPort name="deep_buffer"> ... </mixPort> supprime, et deep_buffer retire des sources des routes.
sed -e '/<mixPort name="deep_buffer"/,/<\/mixPort>/d' -e 's/,deep_buffer//g' -e 's/deep_buffer,//g' "$SRC" > $TMP || abort "sed"
grep -q deep_buffer $TMP && abort "deep_buffer encore present (route qui n'utilise que lui ?) : rien n'a ete change"
grep -q 'sources=""' $TMP && abort "route sans source : rien n'a ete change"
grep -q 'mixPort name="primary output"' $TMP || abort "sortie primary absente : rien n'a ete change"
grep -q '</audioPolicyConfiguration>' $TMP || abort "fichier modifie tronque : rien n'a ete change"

BKDIR=$SDCARD/UJC201_audio_backup/$(date +%Y%m%d-%H%M%S)
if [ ! -f "$f.nodeep-orig" ]; then
	cp -p "$f" "$f.nodeep-orig" || abort "sauvegarde $f.nodeep-orig impossible"
	ui_print "Sauvegarde : $f.nodeep-orig"
fi
mkdir -p "$BKDIR$(dirname "$f")" && cp -p "$SRC" "$BKDIR$f" && ui_print "Copie : $BKDIR$f"
# reecriture sur place : garde l'inode, les droits et le contexte SELinux
cat $TMP > "$f" || abort "ecriture de $f impossible (restaurer $f.nodeep-orig)"
cmp -s $TMP "$f" || abort "verification de $f echouee"
rm -f $TMP
ui_print " "
ui_print "OK : deep_buffer retire, la musique passe par primary. Redemarrer."
ui_print "Retour arriere : flasher ujc201_nodeepbuffer_restore.zip"
cleanup
exit 0
