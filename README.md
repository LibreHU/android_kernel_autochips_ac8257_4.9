# Noyau LibreHU pour l'UJC201 (Jancar, Autochips AC8257)

Noyau Linux 4.9.117 de l'autoradio Jancar UJC201 (SoC Autochips AC8257), construit sur les **sources GPL officielles
d'Autochips** recues de SIXWIN / Jancar le 19/10/2026, auxquelles on ajoute les modifications LibreHU.

**Etat** : compile en CI (`Image.gz-dtb`, images boot et recovery de test). **Pas encore teste sur l'unite.**

> Les modifications de `reverse-engineering` sont reprises (voir [Modifications LibreHU](#modifications-librehu)),
> plus l'overclock jusqu'a 2,6 GHz avec tensions reglables, l'Ethernet USB integre et le pilote du decodeur camera
> TP9963, absent du paquet, reconstruit depuis le noyau stock.

## Le SoC AC8257

![AC8257 : caracteristiques (AutoChips)](docs/img/ac8257_autochips_features.webp)

Page « AC8257产品特征 » (caracteristiques de l'AC8257) de la
[brochure AutoChips 2020](https://img.sae-china.org/resources/platform/uploadid_1612260475_QARR.pdf), © AutoChips.

Ce qu'on en verifie sur l'unite :

| Annonce | Sur l'UJC201 |
|---|---|
| 4 coeurs Cortex-A53 jusqu'a 2,0 GHz, GPU GE8300 | **oui** : 4 coeurs au demarrage (`SMP: Total of 4 processors activated`, MIDR `410fd034` = Cortex-A53), table stock a 2001 MHz maximum, PowerVR GE8300. L'overclock LibreHU (2,3 GHz au demarrage, 2,6 GHz possibles) depasse donc la fiche : voir [docs/TUNING.md](docs/TUNING.md) |
| ARM9 dedie au recul rapide (CVBS en 2 s, AHD 1080p / 720p en 3 s) | **oui** : c'est l'« ARM2 » (`dualarm`, memoire `arm2-backcar-*`) ; il rend la camera a Linux apres le demarrage |
| AHD par MIPI CSI-2 | **oui** : decodeur Techpoint TP9963 sur MIPI ; CVBS par le decodeur integre (TVD) ; voir [docs/VIDEO.md](docs/VIDEO.md) |
| LPDDR3 / LPDDR4(X), eMMC 5.1, USB 3.1 Gen1 + USB 2.0 OTG, modem LTE Cat 7 integre, ecran jusqu'a 1920 x 1080, decodage 1080p30 H.265 / H.264 | non verifie un par un ; l'ecran de l'unite est en 1280 x 720 |
| Gravure 14 nm | non verifiable ici (aucune autre source trouvee) |

C'est une fiche commerciale : elle donne les limites de la puce, pas celles de la carte Jancar (alimentation,
refroidissement, puces presentes).

## Les branches

| Branche | Contenu | Regle |
|---|---|---|
| **`LibreHU-main`** (par defaut) | `autochips-gpl` + nos modifications | **branche de travail** : tout nouveau travail sur le noyau arrive ici |
| `autochips-gpl` | paquet GPL Autochips **tel que recu** (`kernel-4.9.tgz`, SHA-256 `f0303951…d83c577`) | jamais modifiee ; un nouvel envoi de Jancar y est ajoute tel quel |
| `reverse-engineering` | noyau reconstruit sans les sources Autochips (base publique MediaTek MT6761 + reverse engineering) ; demarre la ROM stock | reference : on y reprend les modifications ; garde aussi les notes de l'appareil (`docs/ujc201/`) et les outils `tools/ac8257/` (usbnet, audio, dump) |

```
autochips-gpl ──(git merge autochips-gpl)──▶ LibreHU-main ◀──(reprise commit par commit)── reverse-engineering
```

- Nouvel envoi de Jancar (pilotes manquants) : ajoute sur `autochips-gpl`, puis `git merge autochips-gpl` dans
  `LibreHU-main`.
- Modification de `reverse-engineering` : adaptee aux chemins Autochips (`ac8257` au lieu de `mt6765` / `mt6761`), un
  groupe par commit, CI verte, puis test sur l'unite. Suivi dans [docs/PORTING.md](docs/PORTING.md).

## Ce que `LibreHU-main` ajoute au paquet Autochips

### Pour compiler et demarrer l'unite (detail : [docs/BUILD.md](docs/BUILD.md))
- **CI** (`.github/workflows/build.yml`) avec le compilateur du stock (AOSP clang 6.0.2 `clang-4691093` + binutils
  GCC 4.9) et les variables de `kenv.mk` du stock (`ATC_DISABLE_BATTERY_CHARGER=yes`, `ATC_FASTDISP_VERSION=2.0`) ;
  artefacts : `Image.gz-dtb`, `config`, `recovery_test_ac8257.img`, `boot_ac8257.img`.
- **defconfig de l'unite** `ac8257_ujc201_defconfig` = config complete du noyau stock #72, sans les options dont les
  sources manquent (overlay `UJC201_64`, une partie des tactiles d'autres ecrans), plus les ajouts ci-dessous.
- **Device tree de l'unite** `mediatek/ac8257_ujc201` = le DT du noyau stock #72, decompile (recompile, il donne le
  DTB stock octet pour octet). Le `ac8257.dts` generique d'Autochips n'a pas le noeud GPIO Jancar, le recepteur IR,
  ni `firmware/android/vbmeta`, et monte `system` alors que l'unite est en system-as-root.
- **Reparations du paquet** (fait sous Windows) : 27 liens symboliques, 3 bits d'execution.
- **Fichiers generes par le build Android d'Autochips**, absents du paquet, repris du noyau stock : memoire reservee
  et table des partitions eMMC.
- **Outils d'image** : `tools/ac8257/repack_boot.py` + gabarits stock (boot, vbmeta du recovery).

## Modifications LibreHU

Reprises de `reverse-engineering` (commits d'origine et suivi : [docs/PORTING.md](docs/PORTING.md)), ou nouvelles.
Code LibreHU dans `drivers/misc/librehu` et `drivers/misc/jancar` ; le code Autochips n'est touche que par les
points d'accroche.

| Modification | Detail | CI |
|---|---|---|
| Jancar `/dev/gpios_ioctl` | `CONFIG_JANCAR_GPIOS` (noms d'options du stock), reimplementation : la source Jancar manque au paquet | OK |
| KernelSU Next v3.4.0-legacy | hooks manuels, gestionnaire KernelSU Next v3.4.0 | OK |
| Boost tactile, zram lz4, I/O deadline, gouverneurs conservative et schedutil | `interactive` reste le gouverneur par defaut, comme le stock | OK |
| Outils de mise au point | console pstore precoce, panique minutee, interception des redemarrages, one-shot, retour en recovery, memoire SPM / SCP hors de Linux en boot recovery. **A desactiver pour l'usage quotidien** | OK |
| **Overclock CPU jusqu'a 2,6 GHz**, maximum au demarrage **2,3 GHz** | 16 OPP ; 2,1-2,3 GHz a 1,025 V (stables sur `reverse-engineering`), 2,4-2,6 GHz a 1,05-1,10 V (non testes) ; [docs/TUNING.md](docs/TUNING.md) | OK (run 48) |
| **Tensions CPU reglables** | `ac8257_cpufreq.volt_uv` / `volt_offset_uv`, 0,65-1,10 V, envoyees a la SSPM | OK (run 48) |
| **GPU 730 MHz** et OPP intermediaires, **tensions VCORE reglables** | le GPU tourne sur VCORE (partagee avec le SoC) : `ac8257_gpufreq.vcore_uv`, 0,60-0,90 V | OK (run 48) |
| **Decodeur camera TP9963** (camera AHD) | pilote `tp9963_yuv` absent du paquet, **reconstruit depuis le noyau stock** (registres, metazones, reponses a la HAL camera) et recoupe avec les pilotes GPL OpenIPC et Allwinner du meme TP9963 ; [docs/VIDEO.md](docs/VIDEO.md) | OK (run 49) |
| **Tactiles des autres dalles** | FocalTech FT8201, ILITEK TDDI, Himax HX83102, Hynitron CST340 : pilotes GPL publics les plus proches du stock, essayes apres le GT928 (rien ne change sur une unite a GT911) ; references des dalles et ce qui manque (Synaptics, Sitronix, Jadard) : [docs/TOUCH.md](docs/TOUCH.md) | OK (run 51) |
| Tactile : pas d'inversion de l'axe X | le pilote GT928 du paquet inverse X pour une dalle annoncant X_MAX = 1080 (celle de l'unite) ; le stock ne le fait pas : desactive pour `CONFIG_JANCAR_SOLUTION` | OK (run 49) |
| **Ethernet USB integre** | RTL8152/8153, RNDIS (partage de connexion Android), iPhone, LAN78xx, SMSC, DM9601, SR9700/9800, MCS7830, CH9200, RTL8150, Pegasus, CDC EEM (en plus d'AX88179, CDC Ether, CDC NCM du stock) | OK (run 48) |

Pas a reprendre :
- **tactile** : la reconstruction adaptait le `goodix.c` generique ; ici, comme le stock, `goodix.c` d'Autochips
  sonde d'abord puis abandonne, et c'est le pilote MediaTek `GT928` (`gt9xx_driver.c`, peripherique `mtk-tpd`,
  RST GPIO 45, INT GPIO 29) qui pilote la dalle Goodix **GT911** de l'unite (firmware 1060, 1080 x 600) ;
- **USB** (connexion au premier pull-up) et **IRQ d'affichage** : deja dans les pilotes Autochips ;
- **export des CRC du noyau stock** : seulement si les modules vendor (Wi-Fi, BT, GPS, FM) refusent de se charger ;
  avec le meme compilateur que le stock, les CRC natifs devraient correspondre.

## Roadmap

1. **Compilation** — ✅ fait (CI run 40, 20/10/2026).
2. **Premier demarrage** de ce noyau « presque stock » depuis la partition recovery : affichage, tactile, eMMC,
   `/data`, adb ; comparer la config produite (artefact `config`) avec celle du stock et le DT `ac8257` avec le DT
   stock.
3. **Parite avec le stock** : modules vendor (Wi-Fi, BT, GPS, FM), metazone, audio, touches, IR, MCU / CAN ; puis ce
   que la reconstruction n'a jamais fait marcher et dont les pilotes Autochips sont ici : **camera de recul, AV-in,
   AVM** (TVD, DI, NR, WCH, backcar), **ARM2** (`dualarm`, affichage rapide) et le decodeur `tp9963` reconstruit.
4. **Ajouts LibreHU** — repris en CI (20/10/2026) : `/dev/gpios_ioctl`, KernelSU Next, gouverneurs, overclock et
   tensions, boost tactile, zram, I/O, Ethernet USB ; a tester sur l'unite.
5. **Usage quotidien** : installation dans la partition `boot` (et plus seulement le test recovery), retrait des
   options de mise au point, images publiees en release.

## Futurs ajouts (pistes)

- **Sources manquantes de Jancar**, redemandees le 19/10/2026 : pilotes `JANCAR_SOLUTION` / `JANCAR_GPIOS`, pilote
  LCM `jac_*`, `lowmemorykiller` (`jancar.services`), telecommande `rc-jancar`, overlay `UJC201_64`, capteur
  `tp9963` (pour comparer a la reconstruction), tactiles d'autres ecrans. A leur arrivee : `autochips-gpl` puis
  fusion ici.
- **Modules hors arbre recompiles depuis les sources** (`wmt_drv`, `bt_drv`, `gps_drv`, `fmradio_drv`,
  `wmt_chrdev_wifi`, `wlan_drv_gen4m`, `met`, `fpsgo`) : a demander a Jancar, ou a adapter depuis l'arbre GPL
  Samsung MT6765 (meme famille de puces, connectivite MT6631) ; d'ici la, les modules stock.
- **Device tree propre a l'UJC201** au lieu de l'overlay de demonstration Autochips, et pilote d'ecran tire de la
  description de la metazone.
- **Veille / reprise**, et camera de recul rapide (ARM2) des le demarrage normal.

## Compiler from scratch

Sur un PC Linux x86-64 (Ubuntu 22.04 conseille : dernier Ubuntu avec un paquet `python2`), environ 10 Go libres.
Memes etapes que la CI (`.github/workflows/build.yml`) ; details et pieges : [docs/BUILD.md](docs/BUILD.md).

```bash
# 1. Paquets (python2 : generateur DTS Autochips tools/dct/DrvGen.py ; libtinfo5 : clang AOSP)
sudo apt install bc bison flex libssl-dev libelf-dev cpio git curl python3 python2 libtinfo5 build-essential
sudo ln -sf /usr/bin/python2 /usr/bin/python      # DrvGen.py est lance par "#! /usr/bin/python"

# 2. Sources
git clone -b LibreHU-main https://github.com/LibreHU/android_kernel_autochips_ac8257_4.9 kernel
cd kernel

# 3. Chaine de compilation du noyau stock : binutils AOSP GCC 4.9 + AOSP clang 6.0.2 (clang-4691093)
git clone --depth 1 -b lineage-17.1 https://github.com/LineageOS/android_prebuilts_gcc_linux-x86_aarch64_aarch64-linux-android-4.9 ../gcc49  # = a61b4b9, comme la CI
mkdir -p ../clang && curl -fL https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+archive/refs/tags/android-9.0.0_r61/clang-4691093.tar.gz | tar xz -C ../clang
export PATH="$PWD/../clang/bin:$PWD/../gcc49/bin:$PATH"
clang --version | grep 4691093                      # doit afficher clang-4691093

# 4. Configuration et build
M='O=out ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- CC=clang CLANG_TRIPLE=aarch64-linux-gnu-'
M="$M ATC_DISABLE_BATTERY_CHARGER=yes ATC_FASTDISP_VERSION=2.0"
make $M HOSTCC="gcc -fcommon" ac8257_ujc201_defconfig
make $M HOSTCC="gcc -fcommon" -j"$(nproc)" Image.gz-dtb

# 5. Images pour l'unite (gabarits et vbmeta du stock)
python3 tools/ac8257/repack_boot.py tools/ac8257/stock/boot_template_250718.img out/arch/arm64/boot/Image.gz-dtb \
    recovery_test_ac8257.img --vbmeta tools/ac8257/stock/recovery_stock_vbmeta_250718.bin --size 33554432 \
    --cmdline-append "init=/init log_buf_len=8M"
python3 tools/ac8257/repack_boot.py tools/ac8257/stock/boot_template_250718.img out/arch/arm64/boot/Image.gz-dtb \
    boot_ac8257.img --size 10485760
```
Resultat : `out/arch/arm64/boot/Image.gz-dtb`, `recovery_test_ac8257.img` (test depuis la partition recovery) et
`boot_ac8257.img`. Tester d'abord depuis recovery ([docs/BUILD.md](docs/BUILD.md), *Tester sur l'unite*).

## Documentation

| Document | Contenu |
|---|---|
| [docs/README.md](docs/README.md) | index |
| [docs/BUILD.md](docs/BUILD.md) | compiler (CI, en local), particularites de l'arbre Autochips, tester sur l'unite |
| [docs/STATUS.md](docs/STATUS.md) | etat par fonction sur l'unite, compare a `reverse-engineering` |
| [docs/TUNING.md](docs/TUNING.md) | frequences et tensions CPU / GPU : tables, parametres, prudence |
| [docs/TOUCH.md](docs/TOUCH.md) | dalles tactiles de l'UJC201 : references, pilotes du stock et de LibreHU, sources |
| [docs/VIDEO.md](docs/VIDEO.md) | camera AHD (TP9963), CVBS IN, VIDEO OUT, ARM2 ; reconstruction du pilote TP9963 |
| [docs/PORTING.md](docs/PORTING.md) | modifications de `reverse-engineering` : natives, a verifier, a reprendre ; suivi |
| [README_LIBREHU.md](README_LIBREHU.md) | provenance du paquet Autochips et ce qui y manque |
| `README` | README d'origine de Linux |

## Regles

- Rien n'est ecrit sous dm-verity et vbmeta n'est jamais desactive automatiquement ; si besoin, a la main :
  `fastboot --disable-verity flash vbmeta vbmeta.img`.
- Tester d'abord depuis la partition recovery, jamais directement dans `boot` : un noyau qui boucle depuis `boot` ne
  se remplace qu'avec SP Flash Tool.
- Ne pas publier ses propres partitions `nvram`, `nvdata`, `proinfo`, `persist`, `metazone`.

Licence : GPL-2.0 (noyau Linux, voir `COPYING`).
