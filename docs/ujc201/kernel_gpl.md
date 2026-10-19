# Sources GPL du noyau (reponse Jancar / SIXWIN du 19/10/2026)

Suite a la demande formelle GPLv2 (section 3 : « source complet correspondant »), le distributeur SIXWIN a envoye
le 19/10/2026 un lien OneDrive vers les sources du noyau de l'UJC201 (Linux 4.9.117+).

## Le paquet
- Fichier : `kernel-4.9.tgz`, 156 599 414 octets, SHA-256
  `f0303951aed4ed65eef9da7fdaf0718c50d019f5c956429dc4cdb5cb7d83c577`.
- Contenu : un dossier `kernel-4.9/` (65 641 entrees, 840 Mo decompresse), `Makefile` 4.9.117, arbre Android
  (`Android.mk`, `kenv.mk`, `build.config.*`), `verity_dev_keys.x509` (cle publique de test AOSP).
- Configs AC8257 : `arch/arm64/configs/ac8257_{car,demo,evb,jac}[_debug]_defconfig` et
  `arch/arm/configs/ac8257_demo_1g_32[_debug]_defconfig` ; device trees `arch/arm64/boot/dts/mediatek/ac8257*.dts`.
- Plateforme Autochips presente : `drivers/misc/autochips/` (`metazone`, `tvd`, `avin`, `backcar`,
  `n4_video_decoder`, `dualarm`, `arm2_log`, `nr`, `wch`, `di`, `rsv_cpu`, `boot_state`), affichage
  `drivers/misc/mediatek/video/ac8257/` (dont l'ecran externe `ext_disp` / multidisp), capteurs `tp2850`, `tp2854`,
  `tp2855`, `n4`.

## Comparaison avec le noyau stock (#72)
Methode : config integree au noyau stock (`IKCONFIG`, extraite de `prebuilt/kernel`) comparee en texte aux
defconfigs et aux symboles des `Kconfig` du paquet (rien n'a ete compile ni execute depuis le paquet).

- Bannieres : stock `4.9.117+ (jac0416@jacx) ... clang 6.0.2 (r316199) #72 SMP PREEMPT Thu Jan 22 2026`.
- `ac8257_demo_defconfig` est la base : 428 des 435 options identiques. Les 7 ecarts sont les reglages du produit
  (le stock vaut) : `CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE_NAMES` (`mediatek/UJC201_64`), capteurs
  (`tp2850_yuv tp2854_yuv tp9963_yuv`), ecran `CONFIG_LCM_WIDTH/HEIGHT` (1024 x 600), `CONFIG_SND_SOC_ATC` (non).
  `ac8257_jac_defconfig` donne 427 / 435.
- **Absent du paquet** (actif dans le noyau stock, sans `Kconfig` ni source dans le paquet) :
  - `CONFIG_JANCAR_SOLUTION`, `CONFIG_JANCAR_GPIOS` : les pilotes propres a Jancar ; le mot « jancar »
    n'apparait dans aucun fichier source du paquet ;
  - `CONFIG_TOUCHSCREEN_FTS` (FocalTech), `CONFIG_TOUCHSCREEN_SYNA_TCM2`, `CONFIG_TOUCHSCREEN_SYNA_TCM2_I2C`
    (Synaptics) : pilotes tactiles d'autres ecrans (celui de l'unite, GT928, est present) ;
  - overlay de device tree `mediatek/UJC201_64` ;
  - pilote du capteur `tp9963_yuv` (decodeur video AHD / CVBS) ;
  - pilotes tactiles listes par le noyau stock (chemins `__FILE__`) : `ILITEK_V3`, `hxchipset` (Himax),
    `synaptics_dsx`, `synaptics_tcm2` (14 fichiers).
- **Absent aussi : les modules de connectivite et de performance**, compiles hors de l'arbre du noyau chez MediaTek
  (`drivers/misc/mediatek/connectivity/Makefile` : « move to standalone repo »,
  `vendor/mediatek/kernel_modules/connectivity/...`), charges depuis `/vendor/lib/modules` (lsmod du stock) :
  `wmt_drv` (CONSYS / WMT, socle commun), `bt_drv` (Bluetooth), `gps_drv` (GPS), `fmradio_drv` (FM MT6631),
  `wmt_chrdev_wifi` et `wlan_drv_gen4m` (Wi-Fi), `fpsgo`. Seul l'adaptateur integre (`connadp` :
  `connectivity_build_in_adapter.c`, `wmt_build_in_adapter.c`) est dans le paquet. Ces modules se chargent dans le
  noyau et MediaTek les publie sous GPLv2 ailleurs (ex. arbres de Google Nest) : leurs sources sont dues aussi. A
  verifier sur l'unite : `strings /vendor/lib/modules/wmt_drv.ko | grep license=` (idem pour les autres).
- **Absent enfin : les modifications Jancar de fichiers existants.** Chaines du noyau stock introuvables dans les
  sources du paquet (le fichier y est, mais en version Autochips d'origine) :
  - ecran (pilote LCM) : `jac_lcm_get_mipi_params_form_MetaZone`, `jac_detect_format_from_metazone`,
    `jac_push_table`, `jac_set_lcd_power_kthread`, `jac_set_params`, `jac_lcm_tpye`, etat pinctrl `jac_rst_output0` ;
  - tueur de processus memoire : `mtk_lowmemorykiller jump kill 'jancar.services'`, lecture de
    `/jancar/config/ivi-priapps.ini` (`drivers/staging/android/lowmemorykiller.c` du paquet ne les a pas) ;
  - telecommande IR : table `rc-jancar` ; carte : `show_jancar_board_id` ; GPIO : peripherique `gpios_ioctl`
    (exports `gpio_get_mode` / `gpio_set_mode`) ;
  - tactile Sitronix (`sitronix_*`, `st_dev_*`), en plus des Himax / ILITEK / Synaptics.
- Verifie complet par ailleurs : les 1 137 chemins de fichiers source graves dans le noyau stock
  (`/media/disk1/jenkins_workspace/code/ae_android_project/ac8257/code/kernel-4.9/...`) sont tous dans le paquet sauf
  les 14 fichiers tactiles ; 9 674 symboles exportes du stock compares aux `EXPORT_SYMBOL` du paquet : seuls
  manquent ceux des pilotes ci-dessus. Le pilote GPU (PowerVR `m1.10ED5130912`, DDK `1.10@5130912` comme le stock),
  le codec audio `mt6357`, le capteur `tp2850` / `tp2854`, le device tree de base `mediatek/ac8257` et le tactile
  GT928 sont presents. Le module `met` (profilage MediaTek, importe par les modules vendor) est hors arbre comme la
  connectivite.
- **Hors noyau, pas du ressort de cette demande** : preloader et LK (chargeurs MediaTek, pas sous GPL), firmware
  ARM2, firmware MCU, blobs vendor. Les logiciels GPL de `/system` (ex. `e2fsprogs`, `iproute2`, `iptables`) sont
  ceux d'AOSP, deja publics, sauf modification par Jancar (non verifie : il faudrait la liste de `/system/bin`).
- **RIL** : rien a reclamer cote noyau. Le noyau stock n'a pas de pilote de modem USB (`USB_SERIAL_OPTION`,
  `USB_NET_QMI_WWAN`, `USB_ACM`, `USB_WDM` non actives) ; `MTK_ECCCI` (modem integre MediaTek) est actif par la base
  `ac8257_demo` et present dans le paquet. Le RIL lui-meme (`rild`, bibliotheques vendor) est du code utilisateur,
  hors GPL du noyau.

Conclusion : c'est le noyau Autochips de reference de la plateforme, presque identique en configuration, **sans les
ajouts Jancar**. Le tactile de l'unite (Goodix GT928, `CONFIG_TOUCHSCREEN_MTK_GT928`) est dans le paquet
(`drivers/input/touchscreen/mediatek/GT928`) ; les pilotes tactiles absents servent aux autres ecrans de la gamme.
Un noyau compile depuis ce paquet seul n'aurait ni les GPIO Jancar, ni la camera `tp9963`, ni la description
materielle de l'UJC201 : pas de noyau de remplacement direct, mais de quoi completer le noyau reconstruit.

## Apport pour le noyau reconstruit
[android_kernel_autochips_ac8257_4.9](https://github.com/LibreHU/android_kernel_autochips_ac8257_4.9) (base publique
MediaTek MT6761 + reconstruction, demarre la ROM stock) liste les sources du noyau stock qu'il n'a pas
(`reference/stock-source-files.txt`) et les options sans `Kconfig` (`reference/stock-config-missing-symbols.txt`) :
- 107 des 121 fichiers source manquants ou renommes sont dans le paquet ; il ne manque que les 14 fichiers tactiles
  ci-dessus ;
- 36 des 42 options sans `Kconfig` sont dans le paquet (`ATC_AVIN`, `ATC_BACKCAR`, `ATC_BOOT_STATE`...) ; restent
  `JANCAR_GPIOS`, `JANCAR_SOLUTION`, `TOUCHSCREEN_FTS`, `TOUCHSCREEN_FTS_DIRECTORY`, `TOUCHSCREEN_SYNA_TCM2`,
  `TOUCHSCREEN_SYNA_TCM2_I2C`.
Les pilotes Autochips qui manquaient a la reconstruction (camera de recul / AV-in / AVM : `tvd`, `di`, `nr`, `wch`,
`backcar`, `avin` ; ARM2 : `dualarm`) sont donc disponibles. Le tactile GT928 de l'unite marche deja dans la
reconstruction ; le Wi-Fi, le BT, le GPS et la FM y marchent avec les modules stock (CRC du stock exportes).

## Ce que le paquet apporte deja
- Les pilotes de plateforme a lire au lieu de desassembler : metazone (`metazone_drv.c`, format de la partition),
  entree video (`tvd`, `avin`), recul (`backcar`), ecran externe (`mtk_disp_mgr.c`, `g_open_ext_disp`), ARM2.
- Des en-tetes et une config exacts pour compiler des modules (`tools/ac8257/usbnet/` compile aujourd'hui contre un 4.9.117
  standard puis verifie les CRC du stock).

## Sources publiques de remplacement
- **Connectivite (WMT, BT, GPS, FM, Wi-Fi gen4m), `met`, `fpsgo`** : disponibles dans les sources GPL Samsung du
  Galaxy Tab A7 Lite (MT6765, Linux 4.19,
  [gta7lite/android_kernel_samsung_mt6765](https://github.com/gta7lite/android_kernel_samsung_mt6765)), compilees
  dans l'arbre : `drivers/misc/mediatek/connectivity/{common,bt,gps,fmradio,wlan/core/gen4m}` (924 fichiers), avec
  la plateforme WMT `mt6761.c` (l'AC8257 derive du MT6761) et la puce FM `mt6631` ; `met_drv`, `performance/fpsgo_v3`.
  A porter vers 4.9 (API du noyau) et a accorder aux firmwares et au userspace du stock (`/vendor/firmware`,
  `wmt_loader`, `wmt_launcher`) : versions differentes de celles du #72, adaptations Autochips (`CONSYS_8257`)
  possibles. Les modules stock marchent deja dans le noyau reconstruit : c'est une solution de secours, pas un
  besoin immediat.
- **Tactiles** Himax `hxchipset`, ILITEK v3, Synaptics TCM2 : meme arbre Samsung. Inutiles pour l'unite (GT928,
  present dans le paquet).
- **Capteur `tp9963`** (decodeur AHD Techpoint) : aucune source publique trouvee. Les pilotes Techpoint sont fournis
  sous accord aux fabricants ; `tp2850` / `tp2854` sont dans le paquet. Quel decodeur porte l'unite :
  `dmesg | grep -iE "tp28|tp99"` sur le noyau stock.
- **Code Jancar** (`JANCAR_SOLUTION`, `gpios_ioctl`, LCM `jac_*`, `lowmemorykiller`, `rc-jancar`, identifiant de
  carte) : n'existe nulle part ailleurs ; seul Jancar peut le fournir. Le noyau reconstruit en reimplemente une
  partie (`/dev/gpios_ioctl`, lecture de la metazone).

## Suite
- Redemander a SIXWIN / Jancar les parties manquantes, nommees ci-dessus (la GPLv2 couvre tout le code compile dans
  le noyau distribue, pilotes Jancar inclus) : sources de `CONFIG_JANCAR_SOLUTION` et `CONFIG_JANCAR_GPIOS`, des
  pilotes tactiles FTS / Synaptics TCM2 (et ILITEK, Himax, Synaptics DSX), du capteur `tp9963`, du device tree
  `UJC201_64`, des modules `/vendor/lib/modules` (connectivite `wmt_drv`, `bt_drv`, `gps_drv`, `fmradio_drv`,
  `wmt_chrdev_wifi`, `wlan_drv_gen4m` ; `fpsgo`, `met`), les versions modifiees par Jancar des fichiers existants
  (pilote LCM `jac_*`, `lowmemorykiller.c`, table IR `rc-jancar`, `gpios_ioctl`, tactile Sitronix), et la defconfig
  utilisee pour le #72.
- Garder une copie du paquet (les liens OneDrive partages expirent) ; la GPL permet de le redistribuer.
- Aucun flash de noyau n'est automatise : il demande vbmeta sans verification, fait a la main
  (`fastboot --disable-verity flash vbmeta vbmeta.img`).
