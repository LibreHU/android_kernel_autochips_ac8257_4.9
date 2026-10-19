# Noyau Autochips AC8257 : paquet GPL officiel (branche `autochips-gpl`)

Cette branche contient, **tel quel**, le paquet de sources du noyau que le distributeur SIXWIN (Jancar UJC201) a
envoye le 19/10/2026 en reponse a la demande de conformite GPLv2 (section 3). Aucun fichier n'est modifie ; seul ce
`README_LIBREHU.md` est ajoute. Historique sans lien avec `master` (branche orpheline).

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
methode : `docs/ujc201/kernel_gpl.md` sur la branche `master`.

## Utilisation
- Branche de reference, a ne pas modifier : si Jancar envoie les parties manquantes, elles arrivent ici dans un
  nouveau commit (paquet tel que recu, avec sa provenance), puis sont fusionnees dans `ac8257-integration`.
- Travail : branche `ac8257-integration` (reconstruction de `master` + ces sources).
- Le paquet n'a pas ete compile tel quel : la defconfig du produit et les ajouts Jancar manquent.
