# Compiler et tester le noyau `LibreHU-main` (base Autochips)

## CI
`.github/workflows/build.yml` compile chaque push sur `LibreHU-main` et `connectivity-builtin` (lancement manuel possible)
et publie en artefacts :
- `Image.gz-dtb`, `config` ;
- `recovery_test_ac8257.img` : 32 Mio, vbmeta du recovery stock, pour le test depuis la partition recovery ;
- `boot_ac8257.img` : 10 Mio, vbmeta du boot (partition boot normale) ;
- `boot_ac8257_bootmenu.img` : le meme noyau derriere le **menu de demarrage** (TWRP / Android / Fastboot a chaque
  demarrage, Android au bout de 3 s ; toucher l'ecran met en pause) de l'arbre device (`tools/ac8257/bootmenu`, copie
  de `tools/bootmenu`) : ramdisk dont `/init` est le menu, noyau patche `skip_initramfs` -> `want_initramfs` (sinon le
  LK lui fait ignorer le ramdisk), `tools/ac8257/bootmenu_image.py` ; environ 290 Ko de marge dans les 10 Mio ;
- `BUILD_INFO.txt` (commit, ligne de commande ajoutee, SHA-256).

Les memes fichiers vont aussi dans la release **`latest-<branche>`** du depot ([latest-LibreHU-main](https://github.com/LibreHU/android_kernel_autochips_ac8257_4.9/releases/tag/latest-LibreHU-main),
[latest-connectivity-builtin](https://github.com/LibreHU/android_kernel_autochips_ac8257_4.9/releases/tag/latest-connectivity-builtin), en pre-release), remplacee a chaque build,
avec `release.txt` (`version=r<build>-<commit>`, branche, commit, date). Pas de release pour un lancement manuel avec
une autre ligne de commande. [LibreHU-flashablesZips](https://github.com/LibreHU/LibreHU-flashablesZips) en fait les
zips TWRP de la categorie **Noyau** de LibreHU Apps (onglet Zips : flash par TWRP, ou ecriture directe de l'image en
root) ; ils ecrivent seulement la partition boot, apres sauvegarde de l'actuelle dans `/sdcard/LibreHU/backup`.

## Particularites de l'arbre Autochips
| Point | Pourquoi | Ce que fait la CI |
|---|---|---|
| **Python 2** | `tools/dct/DrvGen.py` genere `cust.dtsi` a partir du `.dws` ; script Python 2 lance par `#! /usr/bin/python` | runner `ubuntu-22.04`, paquet `python2`, lien `/usr/bin/python`. Ubuntu 23.04 et plus n'ont plus de paquet `python2` : Python 2.7.18 compile dans `/opt/python2.7` (avec `CFLAGS=-std=gnu11` sous GCC 15, Ubuntu 26.04 : en C23, `false` / `true` sont des mots-cles et `Include/asdl.h` ne compile plus), `python` en tete du `PATH`, et `make ... python=python` (la variable `$(python)` de `scripts/drvgen/drvgen.mk`, vide par defaut, lance `DrvGen.py` par son `#!`). Le wrapper `bin/clang` du clang AOSP (`#!/usr/bin/env python`) veut aussi Python 2 |
| **`dtc` et GCC hote >= 10** | `scripts/dtc` definit `yylloc` deux fois (lexer et parser) ; GCC 10+ (`-fno-common` par defaut) refuse l'edition de liens | `HOSTCC="gcc -fcommon"` (la correction amont « Remove redundant YYLOC global declaration » est dans `reverse-engineering`, pas ici) |
| **Variables de `kenv.mk`** | l'environnement Android d'Autochips passe des variables a `make`, lues par Kconfig (`option env=`) : elles ne sont pas dans la defconfig et les options qui en dependent disparaissent sans elles | valeurs deduites de la config stock : `ATC_DISABLE_BATTERY_CHARGER=yes` (`ATC_DISABLE_BAT_CHAGE=y`, pas de batterie) et `ATC_FASTDISP_VERSION=2.0` (`ATC_FASTDISP_VERSION=y`, affichage rapide 2.0 ; sans elle `primary_display.c` compile un bloc fastdisp 1.0 qui ne compile pas) ; `ENABLE_AB_UPDATE`, `ATC_MACRO_VTS`, `ATC_MULTI_DISP_SUPPORT_ENABLE` restent vides (leurs options sont absentes du stock) |
| **`atc_storage_partition.h`** | table des partitions eMMC generee par le build Android (sortie passee par `ATC_LINUXINCLUDE`), absente du paquet ; lue par `sd.c` pour `CONFIG_MMC_ATC_SW_WP` (protection logicielle des partitions « RO », active seulement par debugfs) | `drivers/mmc/host/mediatek/ComboA/ac8257/atc_storage_partition.h` = table compilee dans le noyau stock #72 (41 partitions, nom, debut, taille, RO/RW) |
| **Compilateur** | `kenv.mk` prevoit GCC 4.9 AOSP, clang en option ; le noyau stock #72 a ete compile par **clang 6.0.2 (r316199, `clang-4691093`)** avec les binutils GCC 4.9. Avec GCC 4.9 seul, le build s'arrete sur des avertissements que clang ne donne pas (`-Werror` : `maybe-uninitialized` dans `kernel/compat.c`, format dans `avin_mod.c`) | AOSP `clang-4691093` (prebuilts Android 9) + binutils GCC 4.9 : `CC=clang CLANG_TRIPLE=aarch64-linux-gnu-` ; donne aussi des CRC de symboles comparables a ceux du stock (modules vendor) |
| **Liens symboliques** | le paquet a ete fait sous Windows : les 27 liens de l'arbre (dont `arch/arm64/boot/dts/include/dt-bindings`) y sont des petits fichiers texte contenant leur cible | remis en vrais liens dans `LibreHU-main` (`autochips-gpl` garde le paquet tel quel) |
| **Bits d'execution** | perdus pour les 3 binaires precompiles `scripts/dtc/dtc_overlay`, `scripts/dtc/ufdt_apply_overlay`, `scripts/mkimage` (« Permission denied ») | remis (`100755`) dans `LibreHU-main` |
| **`ac8257_reserved_memory.dtsi`** | `ac8257.dts` inclut `<generated/ac8257_reserved_memory.dtsi>`, produit par le build Android d'Autochips (`device/autochips/common/ParseReservedMemory/generate_rsv.py`, absent du paquet) a partir de `ac8257_reserved_memory.xls` ; la table de Jancar differe de ce xls (mdp 15 Mo au lieu de 8, arm2-backcar-ui 5 Mo au lieu de 4) | `arch/arm64/boot/dts/generated/ac8257_reserved_memory.dtsi` = noeud `/reserved-memory` des noyaux stock #25 et #72 (identiques), adresses utilisees par le LK et l'ARM2 ; verifie : le DTB obtenu a le meme noeud que le stock |
| **`libtinfo5`** | le clang AOSP `clang-4691093` est lie a `libtinfo.so.5`, absente d'Ubuntu 22.04 par defaut | paquet `libtinfo5`. Absent d'Ubuntu 24.04 et 26.04 (`libtinfo.so.6` seulement) : lien `libtinfo.so.5` -> `libtinfo.so.6` dans un dossier a part, mis dans `LD_LIBRARY_PATH` (verifie : build complet sous 24.04 et 26.04) |
| **defconfig** | `ac8257_demo_defconfig` d'Autochips vise une carte de demonstration (ecran 720 x 1280, capteur imx135, audio ATC, `ATC_ARM2_LOG_BUFFER` actif) et ne donne pas la config de l'unite | `ac8257_ujc201_defconfig` = **config complete du noyau stock #72** (IKCONFIG), ajustee seulement la ou des sources manquent : overlay `ac8257_demo` au lieu de `UJC201_64`, sans `TOUCHSCREEN_FTS*`, `TOUCHSCREEN_SYNA_TCM2*` ; `JANCAR_GPIOS` et `tp9963_yuv` sont les reimplementations LibreHU |

Le device tree ajoute au noyau est `mediatek/ac8257` (source Autochips) ; l'overlay `UJC201_64` du stock manque,
l'unite garde sa partition `dtbo` stock. A comparer avec le DTB stock (`reverse-engineering` en a une version
decompilee, identique octet pour octet au stock).

## En local
Pas a pas complet (paquets, chaine GCC 4.9 + clang `4691093`, build, images) pour **Ubuntu 22.04, 24.04 et 26.04** :
section [Compiler from scratch](../README.md#compiler-from-scratch) du README (24.04 et 26.04 verifies de bout en bout
dans des conteneurs propres le 21/10/2026). Les commandes `make` doivent inclure
`CC=clang CLANG_TRIPLE=aarch64-linux-gnu-` (GCC 4.9 seul s'arrete sur des avertissements) et les variables
`ATC_DISABLE_BATTERY_CHARGER=yes ATC_FASTDISP_VERSION=2.0`.

## Tester sur l'unite
Meme methode que `reverse-engineering` (`docs/BUILD.md` de cette branche, details et pieges) :
1. Garder une copie du `boot.img` stock et un retour possible : SP Flash Tool avec le scatter stock, ou TWRP.
   Sauvegarder aussi `para` (`dd if=/dev/block/by-name/para of=...`).
2. **Partition recovery d'abord** : `fastboot flash recovery recovery_test_ac8257.img`, puis
   `adb reboot recovery`. `fastboot boot` ne marche pas sur cette unite. `init=/init` est ajoute a la ligne de
   commande (le LK ne le met pas en mode recovery). Un plantage finit sur le boot normal stock, journal garde
   (`/proc/last_kmsg`, `/sys/fs/pstore/console-ramoops*`). Remettre TWRP dans recovery ensuite.
3. Boot normal (`boot_ac8257.img` dans `boot`) seulement quand le test recovery est bon : un noyau qui boucle
   depuis `boot` ne se remplace qu'avec SP Flash Tool, qui efface la RAM et le journal.
4. Regarder : `adb logcat`, `adb shell dmesg`, `/proc/last_kmsg`, console UART (`ttyS0`, 921600 8N1).

Outils de mise au point de `reverse-engineering` (`drivers/misc/librehu`) : la console pstore precoce reste ; la
panique minutee, l'interception des redemarrages, le one-shot et le retour en recovery
(`CONFIG_AC8257_BRINGUP_PANIC`) sont desactives depuis le 21/10/2026 (noyau utilise au quotidien) : a remettre a `y`
dans le defconfig pour un noyau de test qui risque de bloquer.
Avec ces outils actifs (noyaux d'avant le 21/10/2026), `adb reboot recovery` et `adb reboot bootloader` deviennent
une panique et l'unite redemarre sur Android. Pour redemarrer normalement sur un tel noyau :
`adb shell su -c "echo 0 > /sys/module/ac8257_bringup/parameters/intercept_reboot"`, puis la commande `adb reboot`.

## Auteur de la compilation

La CI fixe `KBUILD_BUILD_USER=dokt` et `KBUILD_BUILD_HOST=librehu` (`env:` de `.github/workflows/build.yml`) :
`/proc/version` et la premiere ligne du journal du noyau affichent `Linux version 4.9.x+ (dokt@librehu) ...`. En local, exporter les memes
variables avant `make` pour obtenir la meme chaine (sans effet sur les modules : le vermagic n'en depend pas).
