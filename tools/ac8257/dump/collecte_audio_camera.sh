#!/bin/bash
# Collecte, dans un dump de la ROM stock (system et vendor extraits), ce qu'il faut pour etudier l'audio,
# la camera de recul, la radio FM et les services Jancar. Produit ~/ujc201_collecte.tar.gz (a envoyer).
#
# usage (WSL / Linux) :  bash collecte_audio_camera.sh <dossier system> <dossier vendor>
#   ex. : bash collecte_audio_camera.sh /mnt/c/dump/system /mnt/c/dump/vendor
set -u
set -f   # les motifs (*audio*...) vont a find, pas au shell
SYS=${1:?usage: $0 <dossier system> <dossier vendor>}; VEN=${2:?usage: $0 <dossier system> <dossier vendor>}
# system-as-root : le "vrai" system est souvent dans system/system
[ -f "$SYS/system/build.prop" ] && SYS="$SYS/system"
[ -f "$SYS/build.prop" ] || echo "ATTENTION : pas de build.prop dans $SYS (bon dossier ?)"
[ -f "$VEN/build.prop" ] || echo "ATTENTION : pas de build.prop dans $VEN (bon dossier ?)"
OUT=$HOME/ujc201_collecte
rm -rf "$OUT"; mkdir -p "$OUT/system" "$OUT/vendor"

# 1. Listes completes (droits, taille, chemin ; cibles des liens)
for p in system:$SYS vendor:$VEN; do
	n=${p%%:*}; d=${p#*:}
	(cd "$d" && find . -printf '%M %10s %p%l\n' 2>/dev/null | sort -k3) > "$OUT/liste_$n.txt"
done

# copie <racine> <dest> <expression find...> : garde l'arborescence
copy() {
	local root=$1 dest=$2; shift 2
	(cd "$root" && find . -type f \( "$@" \) -print0 2>/dev/null | xargs -0 -r cp --parents -t "$dest" 2>/dev/null)
}
K='-iname *audio* -o -iname *mixer* -o -iname *effect* -o -iname *aurisys* -o -iname *smartpa* -o -iname *sound*'
CAM='-ipath *backcar* -o -ipath *camera* -o -ipath *avm* -o -ipath *dvr* -o -ipath *ccd* -o -ipath *reverse* -o -ipath *hd2cam*'
FM='-ipath *fmradio* -o -ipath *radio* -o -iname *fm*'

# 2. vendor : configuration
copy "$VEN" "$OUT/vendor" -path './etc/*' \( $K -o -iname '*camera*' -o -iname '*avm*' -o -iname '*fm*' \)
copy "$VEN" "$OUT/vendor" -path './etc/audio_param/*' -o -path './etc/aurisys_param/*' -o -path './etc/init/*' \
	-o -path './etc/vintf/*' -o -name manifest.xml -o -path './etc/selinux/*' -o -path './avm/*' \
	-o -name build.prop -o -name default.prop -o -path './etc/fstab*' -o -name 'ueventd*.rc'
# vendor : HAL audio et librairies
copy "$VEN" "$OUT/vendor" -path './lib*/hw/audio*' -o -path './lib*/hw/android.hardware.audio*' \
	-o -path './bin/hw/*audio*' -o -path './lib*/libaudio*' -o -path './lib*/libtinyalsa*' \
	-o -path './lib*/libaurisys*' -o -path './lib*/libfm*' -o -path './lib*/*fmr*' -o -path './lib*/libspeech*' \
	-o -path './lib/modules/*fm*' -o -path './lib/modules/*audio*' -o -path './lib/modules/*snd*'
# vendor : applis Jancar / camera / radio (APK + odex)
copy "$VEN" "$OUT/vendor" -path './app/ivi-services/*' -o \( \( -path './app/*' -o -path './priv-app/*' \) \( $CAM -o $FM \) \)

# 3. system
copy "$SYS" "$OUT/system" -name build.prop -o -path './etc/init/*' -o -path './etc/permissions/*' \
	-o \( -path './etc/*' \( $K -o -iname '*camera*' \) \) \
	-o \( -path './framework/*' \( -iname '*jancar*' -o -iname '*autochips*' -o -iname '*atc*' \) \) \
	-o \( \( -path './app/*' -o -path './priv-app/*' \) \( $CAM -o $FM -o -ipath '*jancar*' -o -ipath '*autochips*' -o -ipath '*viper*' -o -ipath '*v4a*' \) \)
# soundfx : la liste suffit
for d in "$VEN/lib/soundfx" "$VEN/lib64/soundfx" "$SYS/lib/soundfx" "$SYS/lib64/soundfx"; do
	[ -d "$d" ] && { echo "== $d"; ls -l "$d"; }
done > "$OUT/soundfx.txt"

# 4. Reperage par nom (ce que les motifs ci-dessus auraient rate)
for p in system:$SYS vendor:$VEN; do
	n=${p%%:*}; d=${p#*:}
	(cd "$d" && find . -type f 2>/dev/null | grep -i -E 'audio|mixer|alsa|tinymix|snd|sound|backcar|camera|avm|dvr|ccd|reverse|fmradio|radio|jancar|autochips|viper|v4a' | sed "s|^\.|$n|")
done | sort > "$OUT/reperage.txt"

# 5. Archive
cd "$HOME" && tar czf ujc201_collecte.tar.gz ujc201_collecte
echo
echo "Archive : $HOME/ujc201_collecte.tar.gz ($(du -h ujc201_collecte.tar.gz | cut -f1))"
echo "Fichiers copies : $(find ujc201_collecte -type f | wc -l)"
echo "Plus gros fichiers :"; find ujc201_collecte -type f -printf '%s %p\n' | sort -rn | head -8 | awk '{printf "  %6.1f Mo  %s\n", $1/1048576, $2}'
echo "Pour la copier sur le bureau Windows :  cp ~/ujc201_collecte.tar.gz /mnt/c/Users/\$USER_WINDOWS/Desktop/"
