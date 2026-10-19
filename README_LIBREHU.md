# Noyau AC8257 LibreHU : branche de travail `main`

## Les trois branches
| Branche | Contenu | Regle |
|---|---|---|
| `autochips-gpl` | paquet GPL Autochips **tel quel** (SIXWIN / Jancar, 19/10/2026) + ce README | on n'y touche pas ; un nouvel envoi de Jancar y arrive tel quel, puis est fusionne dans `main` |
| `reverse-engineering` | noyau reconstruit (base publique MediaTek MT6761 + reverse engineering), demarre la ROM stock | reference de ce qu'on a fait ; on y reprend les modifications |
| **`main`** | **`autochips-gpl` + nos modifications reprises de `reverse-engineering`** | **branche de travail principale** |

`main` part de `autochips-gpl` (meme historique : `git merge autochips-gpl` suffit apres un nouvel envoi). Ajoute pour
l'instant :
- CI (`.github/workflows/build.yml`, artefacts seulement) et outils d'image (`tools/ac8257/repack_boot.py`, gabarit de
  boot et vbmeta du recovery stock), repris de `reverse-engineering` ; compilateur AOSP GCC 4.9 (prevu par `kenv.mk`,
  le stock a ete compile par clang 6.0.2) et `ATC_DISABLE_BATTERY_CHARGER=yes`, seule variable de `kenv.mk` lue par
  l'arbre ;
- `arch/arm64/configs/ac8257_ujc201_defconfig` = `ac8257_demo_defconfig` d'Autochips + les reglages produit du noyau
  stock #72 : ecran 1024 x 600, capteurs `tp2850_yuv tp2854_yuv` (`tp9963_yuv` absent du paquet), `SND_SOC_ATC` non.
  L'overlay reste `mediatek/ac8257_demo` (`UJC201_64` absent) : l'unite garde sa partition `dtbo` stock.

## Modifications de `reverse-engineering` a reprendre
48 commits de code sur `reverse-engineering` (`git log reverse-engineering -- . ':!docs' ':!*.md' ':!reference'`).
Une partie contournait l'absence des sources Autochips et existe deja ici (verifie dans le paquet) :

| Groupe | Commits | Dans le paquet Autochips | A faire |
|---|---|---|---|
| Compatibles renommes du device tree stock | `179bd19dc` | oui (arbre `ac8257` natif) | rien, a verifier au demarrage |
| Frame buffer `autochips,framebuffer`, ABI HWC (ecran externe, affichage rapide) | `a9ea3a022`, `dfcd7fa01` | oui (`video/ac8257/videox`) | rien |
| Options ext4 `autoformat` / `autorestore` / `permissioncheck` | `abdcad3a5` | oui (`fs/ext4/super.c`) | rien |
| Tactile GT928 (adresse, polling, EINT 42, axes) | `e74ff9a2c`, `a26190740`, `474264ca4`, `bf5607775`, `b6651b70d`, `c4e33f390` | pilote GT928 Autochips (`touchscreen/mediatek/GT928`) | tester ; reprendre l'inversion des axes si besoin |
| Tampons sous 4 Go (Trusty, GPU, ION) | `6a44c9b82`, `4fa7c72d4`, `35b0f246e` | `GFP_DMA` dans ION et Trusty | verifier le GPU et ION avec 6 Go de RAM |
| metazone (`/dev/mtz`, API noyau) | `3c421dd1b` (stubs), `7b96133c9` | oui (`drivers/misc/autochips/metazone`) | rien |
| spidev, UART2/3, horloges | `1e95903d7`, `010e78855`, `dafab7e4c` | `spi-ac82xx-dev` | verifier le majeur 163 pour `wmt_chrdev_wifi` |
| Memoire SPM / SCP reservee | `de3d52f70`, `d4b3d57aa` | DT `ac8257` | verifier (boot recovery et normal) |
| IRQ d'affichage sans horloge | `c8cda0229`, `18f94cea6` (garde IRQ) | a verifier | tester |
| **Jancar `/dev/gpios_ioctl`** | `ef2ff5ff3` | **non** (`CONFIG_JANCAR_GPIOS` absent) | **reprendre** |
| **Export des CRC du noyau stock** (modules vendor wifi/bt/gps/fm) | `3c421dd1b` (+ `tools/ac8257/stock/Module.symvers`) | non | reprendre si les modules stock refusent de se charger |
| USB : connexion au premier pull-up | `9f32427be` | a verifier | tester l'adb |
| Mise au point (console pstore precoce, panique minutee, retour en recovery, one-shot) | `5d50c2353`, `e833e4408`, `66b3140c6`, `d415557dc`, `27cc3b086`, `33bf59608`, `912ebc302`, `8ef6d980f`, `144a90eb6` | non | reprendre pour la methode de test recovery |
| Outils d'image (`repack_boot`) et CI | `d0c82e5d5`, `cd7e19590`, `dc209546f` | — | **fait** |
| Overclock CPU / GPU, gouverneurs, reglages | `eb3b1ba14`, `59871d544`, `ca26db327`, `3e3d3eda1`, `53bd202ce`, `365db127c`, `0da8dc694` | non | reprendre |
| KernelSU Next | `54f36f715` | non | reprendre |

Methode : `git show <commit>` sur `reverse-engineering`, adapter aux chemins Autochips (`mt6765` / `mt6761` cote
reconstruction, `ac8257` ici), un groupe par commit, CI verte puis test sur l'unite par la methode recovery
(`docs/BUILD.md` de `reverse-engineering`). Ne jamais desactiver dm-verity ni ecrire vbmeta automatiquement.

---

## Le paquet GPL Autochips (base de cette branche)

La branche `autochips-gpl` contient, **tel quel**, le paquet de sources du noyau que le distributeur SIXWIN (Jancar UJC201) a
envoye le 19/10/2026 en reponse a la demande de conformite GPLv2 (section 3). Aucun fichier n'y est modifie ; seul un
`README_LIBREHU.md` est ajoute. Historique sans lien avec `reverse-engineering` (branche orpheline).

## Provenance
- Expediteur : SIXWIN EU Direct (distributeur des autoradios UJC201), courriel du 19/10/2026 : « We have obtained the
  corresponding GPL source package from our upstream/platform team ».
- Fichier recu (lien OneDrive) : `kernel-4.9.tgz`, 156 599 414 octets, SHA-256
  `f0303951aed4ed65eef9da7fdaf0718c50d019f5c956429dc4cdb5cb7d83c577`.
- Contenu : dossier `kernel-4.9/` (60 984 fichiers, 840 Mo), dont le contenu est a la racine de cette branche.
- Nature : noyau de reference Autochips de la plateforme AC8257 (Linux 4.9.117), configs
  `arch/arm64/configs/ac8257_{car,demo,evb,jac}[_debug]_defconfig`, device trees `ac8257*.dts`, pilotes Autochips
  `drivers/misc/autochips/` (metazone, tvd, avin, backcar, di, nr, wch, dualarm, arm2_log, n4_video_decoder...),
  affichage avec ecran externe (`ext_disp`), GPU PowerVR `m1.10ED5130912`, tactile GT928.

## Comparaison avec le noyau de l'unite
Noyau de reference : celui de la mise a jour officielle Jancar de janvier 2026, `4.9.117+ #72` (22/01/2026,
`jac0416@jacx`), config integree (`IKCONFIG`), 1 137 chemins de sources graves dans l'image, 9 674 symboles exportes.
- `ac8257_demo_defconfig` : 428 des 435 options identiques au stock ; les 7 autres sont les reglages du produit
  (overlay `mediatek/UJC201_64`, capteurs `tp2850_yuv tp2854_yuv tp9963_yuv`, ecran 1024 x 600, `SND_SOC_ATC` non).
- Les 1 137 sources du stock sont toutes ici, sauf 14 fichiers tactiles ; le pilote GPU correspond au DDK du stock
  (`1.10@5130912`).

## Ce qui manque (demande envoyee le 19/10/2026)
1. **Pilotes et configuration Jancar** : `CONFIG_JANCAR_SOLUTION`, `CONFIG_JANCAR_GPIOS` (peripherique
   `gpios_ioctl`, exports `gpio_get_mode` / `gpio_set_mode`, `show_jancar_board_id`), overlay de device tree
   `mediatek/UJC201_64`, defconfig exacte du #72 (`ARCH_MTK_PROJECT="ac8257_demo"`).
2. **Modifications Jancar de fichiers existants** (ici en version Autochips d'origine) : pilote LCM
   (`jac_lcm_get_mipi_params_form_MetaZone`, `jac_detect_format_from_metazone`, `jac_push_table`,
   `jac_set_lcd_power_kthread`, `jac_set_params`), `drivers/staging/android/lowmemorykiller.c` (`jancar.services`,
   `/jancar/config/ivi-priapps.ini`), table de telecommande IR `rc-jancar`.
3. **Pilotes actives mais absents** : capteur `tp9963_yuv` ; tactiles `CONFIG_TOUCHSCREEN_FTS`,
   `CONFIG_TOUCHSCREEN_SYNA_TCM2` / `_I2C`, `ILITEK_V3`, `hxchipset` (Himax), `synaptics_dsx`, Sitronix (ecrans
   d'autres variantes ; celui de l'unite, GT928, est present).
4. **Modules hors arbre de `/vendor/lib/modules`** (`vendor/mediatek/kernel_modules/`) : connectivite `wmt_drv`,
   `bt_drv`, `gps_drv`, `fmradio_drv`, `wmt_chrdev_wifi`, `wlan_drv_gen4m` ; `met`, `fpsgo`. Seul l'adaptateur
   integre `connadp` est ici.

Sources publiques de remplacement (connectivite, met, fpsgo, tactiles : arbre GPL Samsung MT6765) et detail de la
methode : `docs/ujc201/kernel_gpl.md` sur la branche `reverse-engineering`.

