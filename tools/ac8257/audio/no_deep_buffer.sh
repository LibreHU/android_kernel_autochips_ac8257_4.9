#!/usr/bin/env bash
# Retire la sortie "deep_buffer" de la politique audio Android de l'UJC201 (AC8257, Android 9) :
# toute la musique passe alors par la sortie "primary" (celle des sonneries, qui fonctionne).
#
# Pourquoi : sur cette unite, la sortie deep_buffer (PCM 3 "MultiMedia2") reste muette alors que
# primary (PCM 0 "MultiMedia1") marche. deep_buffer ne sert qu'a economiser la batterie (gros blocs).
# Voir docs/audio_deep_buffer.md.
#
# Usage (PC avec adb, autoradio root et /vendor inscriptible) :
#   tools/audio/no_deep_buffer.sh apply     sauvegarde, modifie, verifie, propose le redemarrage
#   tools/audio/no_deep_buffer.sh restore   remet le fichier d'origine
#   tools/audio/no_deep_buffer.sh status    fichier utilise, deep_buffer present ou non, sorties actives
set -euo pipefail

BACKUP_DIR=/sdcard/librehu-audio
BACKUP=$BACKUP_DIR/audio_policy_configuration.xml.orig
TMP=/data/local/tmp/audio_policy_configuration.xml.new

sh_() { adb shell "$@" | tr -d '\r'; }

die() {
    echo "ERREUR : $*" >&2
    exit 1
}

# Meme ordre de recherche que l'AudioPolicyManager d'Android 9 : le premier fichier trouve est utilise.
policy_file() {
    for f in /odm/etc/audio_policy_configuration.xml \
        /vendor/etc/audio/audio_policy_configuration.xml \
        /vendor/etc/audio_policy_configuration.xml \
        /system/etc/audio_policy_configuration.xml; do
        if [ "$(sh_ "[ -f $f ] && echo y")" = y ]; then
            echo "$f"
            return
        fi
    done
    die "aucun audio_policy_configuration.xml trouve"
}

prepare() {
    adb wait-for-device
    adb root >/dev/null
    adb wait-for-device
    adb remount >/dev/null 2>&1 || true
}

writable_check() {
    local f=$1
    sh_ "touch $f 2>/dev/null && echo ok" | grep -q ok ||
        die "$f n'est pas inscriptible (adb disable-verity puis redemarrer, puis adb remount)"
}

apply() {
    prepare
    local f
    f=$(policy_file)
    echo "Fichier utilise : $f"
    writable_check "$f"

    if [ "$(sh_ "grep -c deep_buffer $f" || true)" = 0 ]; then
        echo "deep_buffer deja retire, rien a faire."
        return
    fi

    sh_ "mkdir -p $BACKUP_DIR"
    if [ "$(sh_ "[ -f $BACKUP ] && echo y")" != y ]; then
        sh_ "cp $f $BACKUP"
        echo "Sauvegarde : $BACKUP"
    else
        echo "Sauvegarde deja presente : $BACKUP (conservee)"
    fi
    adb pull "$f" ./audio_policy_configuration.xml.orig >/dev/null && echo "Copie locale : ./audio_policy_configuration.xml.orig"

    # Bloc <mixPort name="deep_buffer"> ... </mixPort> supprime, et deep_buffer retire des routes.
    sh_ "sed -e '/<mixPort name=\"deep_buffer\"/,/<\\/mixPort>/d' -e 's/primary output,deep_buffer/primary output/' $f > $TMP"

    [ "$(sh_ "grep -c deep_buffer $TMP" || true)" = 0 ] || die "deep_buffer encore present dans le fichier modifie, rien n'a ete change"
    sh_ "grep -q 'mixPort name=\"primary output\"' $TMP" || die "sortie primary absente du fichier modifie, rien n'a ete change"
    sh_ "grep -q '</audioPolicyConfiguration>' $TMP" || die "fichier modifie tronque, rien n'a ete change"

    # Ecriture sur place : garde le proprietaire, les droits et le label SELinux du fichier d'origine.
    sh_ "cat $TMP > $f && rm $TMP && sync"
    [ "$(sh_ "grep -c deep_buffer $f" || true)" = 0 ] || die "ecriture echouee, lancer : $0 restore"
    sh_ "ls -lZ $f"
    echo
    echo "OK : deep_buffer retire. Redemarrer pour l'appliquer :  adb reboot"
    echo "Puis verifier :  $0 status"
}

restore() {
    prepare
    local f
    f=$(policy_file)
    [ "$(sh_ "[ -f $BACKUP ] && echo y")" = y ] || die "pas de sauvegarde $BACKUP (copie locale possible : adb push audio_policy_configuration.xml.orig $f)"
    writable_check "$f"
    sh_ "cat $BACKUP > $f && sync"
    echo "Fichier d'origine remis dans $f. Redemarrer :  adb reboot"
}

status() {
    adb wait-for-device
    local f
    f=$(policy_file)
    echo "Fichier utilise : $f"
    echo "Occurrences de deep_buffer : $(sh_ "grep -c deep_buffer $f" || true)"
    echo "Sorties audio ouvertes par Android :"
    sh_ "dumpsys media.audio_flinger | grep -E 'Output thread|AudioStreamOut|ViPERDSP'" || true
}

case "${1:-}" in
apply) apply ;;
restore) restore ;;
status) status ;;
*)
    echo "Usage : $0 apply | restore | status" >&2
    exit 2
    ;;
esac
