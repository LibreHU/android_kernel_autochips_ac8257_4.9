# Compiler et tester le noyau `main` (base Autochips)

## CI
`.github/workflows/build.yml` compile chaque push sur `main` (lancement manuel possible) et publie en artefacts
(pas de release) :
- `Image.gz-dtb`, `config` ;
- `recovery_test_ac8257.img` : 32 Mio, vbmeta du recovery stock, pour le test depuis la partition recovery ;
- `boot_ac8257.img` : 10 Mio, vbmeta du boot (partition boot normale) ;
- `BUILD_INFO.txt` (commit, ligne de commande ajoutee, SHA-256).

## Particularites de l'arbre Autochips
| Point | Pourquoi | Ce que fait la CI |
|---|---|---|
| **Python 2** | `tools/dct/DrvGen.py` genere `cust.dtsi` a partir du `.dws` ; script Python 2 lance par `#! /usr/bin/python` | runner `ubuntu-22.04`, paquet `python2`, lien `/usr/bin/python` |
| **`dtc` et GCC hote >= 10** | `scripts/dtc` definit `yylloc` deux fois (lexer et parser) ; GCC 10+ (`-fno-common` par defaut) refuse l'edition de liens | `HOSTCC="gcc -fcommon"` (la correction amont « Remove redundant YYLOC global declaration » est dans `reverse-engineering`, pas ici) |
| **Variables de `kenv.mk`** | l'environnement Android d'Autochips passe `ATC_*` a `make` ; seule `ATC_DISABLE_BATTERY_CHARGER` est lue par l'arbre (refroidissement batterie) | `ATC_DISABLE_BATTERY_CHARGER=yes` (l'unite n'a pas de batterie) ; les autres donnent des avertissements Kconfig sans effet |
| **Compilateur** | `kenv.mk` prevoit GCC 4.9 AOSP, clang en option ; le noyau stock #72 a ete compile par clang 6.0.2 (r316199) | GCC 4.9 AOSP (`aarch64-linux-android-4.9`, miroir LineageOS) |
| **defconfig** | `ac8257_demo_defconfig` d'Autochips vise une carte de demonstration (ecran 720 x 1280, capteur imx135, audio ATC) | `ac8257_ujc201_defconfig` : la meme + reglages produit du stock #72 (ecran 1024 x 600, capteurs `tp2850` / `tp2854`, pas de `SND_SOC_ATC`) |

Le device tree ajoute au noyau est `mediatek/ac8257` (source Autochips) ; l'overlay `UJC201_64` du stock manque,
l'unite garde sa partition `dtbo` stock. A comparer avec le DTB stock (`reverse-engineering` en a une version
decompilee, identique octet pour octet au stock).

## En local
```
git clone --depth 1 -b lineage-17.1 https://github.com/LineageOS/android_prebuilts_gcc_linux-x86_aarch64_aarch64-linux-android-4.9 gcc49
export PATH=$PWD/gcc49/bin:$PATH
# python2 joignable en /usr/bin/python (DrvGen.py)
M="O=out ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- ATC_DISABLE_BATTERY_CHARGER=yes"
make $M HOSTCC="gcc -fcommon" ac8257_ujc201_defconfig
make $M HOSTCC="gcc -fcommon" -j$(nproc) Image.gz-dtb
```
Avec clang (comme le stock, a essayer pour que les modules vendor du stock se chargent sans export de CRC) :
ajouter `CC=clang CLANG_TRIPLE=aarch64-linux-gnu-`.

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

Les outils de mise au point de `reverse-engineering` (console pstore precoce, panique minutee, retour en
recovery) ne sont pas encore repris ici : voir [PORTING.md](PORTING.md).
