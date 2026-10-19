# UJC201 - remet la sortie audio deep_buffer (fichier d'origine sauvegarde par ujc201_nodeepbuffer_install)

ui_print "UJC201 - sortie audio deep_buffer remise"
ui_print "-----------------------------------------"
mount_rw
f=$(policy_file)
[ -n "$f" ] && [ -f "$f.nodeep-orig" ] || { ui_print "Pas de sauvegarde .nodeep-orig : rien a remettre."; cleanup; exit 0; }
cat "$f.nodeep-orig" > "$f" || abort "ecriture de $f impossible"
cmp -s "$f.nodeep-orig" "$f" || abort "verification de $f echouee"
rm -f "$f.nodeep-orig"
ui_print "$f : remis d'origine. Redemarrer."
cleanup
exit 0
