# Etat du noyau `LibreHU-main` sur l'UJC201

Tests sur l'unite le 20/10/2026 depuis la partition recovery : run 53 (`7013a1d67`) panique audio a 6,5 s ; run 57
(`aeefa2c28`) jusqu'au launcher, panique camera a 26 s et `/system` protege en ecriture. Voir *Demarrage*, *Audio*,
*eMMC*, *Camera*. Le 21/10/2026, run 64 (`f678cd166`) : utilisable au quotidien, 40 min sans plantage, Geekbench 5
a 2,4 GHz. Colonne « reverse-engineering » : etat atteint par le noyau reconstruit
(son `README.md`), pour comparaison.

| Fonction | `LibreHU-main` (Autochips) | `reverse-engineering` | Notes pour `LibreHU-main` |
|---|---|---|---|
| Mises a jour linux-stable (`stable-4.9.150` ... `stable-4.9.337`, 4.9.118 -> 4.9.337) | **4.9.337 OK sur l'unite (27/10)** : demarrage 14 s, USB, adb, MCU, Bluetooth, Wi-Fi, point d'acces, root, FM, GPIO, CVBS 1 / 2, camera ; 4.9.150 valide, 4.9.200 demarre ; 225 a 319 non testes | - | [STABLE.md](STABLE.md) : 7 lots, decisions par commit, audit ; restent GPS avec antenne, veille, `rndis`, appel BT, temperatures |
| Compilation CI | **OK** (run 40, `710d83310`) | OK | `Image.gz-dtb` 9,7 Mo (stock 10,3 Mo : pilotes Jancar absents) ; mises au point de l'environnement : [BUILD.md](BUILD.md) |
| Demarrage jusqu'a l'interface Android | **OK** (run 64), depuis la partition recovery **et depuis `boot`** (sans artefact ni glitch) ; run 57 : panique camera a 26 s, corrigee | OK | DT stock de l'unite (`mediatek/ac8257_ujc201`), comme `reverse-engineering` |
| eMMC, `/data` (options `autoformat`...) | **OK** ; ecritures sur `/system` refusees (run 57), corrige | OK | options natives ici (`fs/ext4/super.c`) ; protection logicielle (`CONFIG_MMC_ATC_SW_WP`) : le `init.ac8257.rc` du vendor ecrit 1 dans `mmc0/sw_wp_en`, fichier absent du noyau stock (l'ecriture y echoue), present ici : `/system` et les autres partitions « RO » refusaient l'ecriture (EXT4 I/O error). Fichier plus cree avec `CONFIG_JANCAR_SOLUTION` ; `adb remount` affiche alors « set_sw_wp_state open sw_wp_en fail! No such file or directory » puis « remount succeeded » : sans effet, le fichier manque aussi au stock |
| GPU PowerVR GE8300 | **OK** (`GLES: PowerVR Rogue GE8300, OpenGL ES 3.2 build 1.10@5130912`, Geekbench 5) | OK (tampons sous 4 Go) | pilote `m1.10ED5130912` natif, meme DDK que le stock |
| Affichage, HWC, ecran externe | non teste | OK (ABI reconstruite) | `video/ac8257` natif ; `CONFIG_LCM_WIDTH/HEIGHT` 1024 x 600 comme le stock, ecran reel 1280 x 720 (`wm size` du stock) decrit par la metazone (`lcm_driver_common`) |
| Retro-eclairage | **OK**, ecran et touches tactiles (luminosite maximale plus haute que le stock) | OK la plupart du temps | |
| Tactile (dalle Goodix GT911) | **OK** (run 64, `f678cd166`) ; touches virtuelles hors ecran justes apres une calibration LibreHU-service ; avant : hors cadre, matrice restee `1 0 0 0 1 0 1` (identifiant de dalle absent de `/proc/gt9xx_config`) | OK (goodix.c adapte) | pilote MediaTek `GT928` (`mtk-tpd`, RST 45 / INT 29) comme le stock ; inversion de X du paquet desactivee (le stock ne l'a pas) ; calibration Jancar `/sys/devices/platform/touch/gt9xx_props` reprise du stock ([TOUCH.md](TOUCH.md#calibration-gt9xx_props)) |
| Tactiles d'autres dalles (FocalTech, ILITEK, Himax, Hynitron) | non teste | absent | pilotes GPL publics, apres le GT928 : [TOUCH.md](TOUCH.md) |
| `/dev/gpios_ioctl` (Jancar) | present (`10, 45`) ; voir *Camera de recul* (GPIO deja pris) | present | reimplementation de `reverse-engineering` (`drivers/misc/jancar`), noeud du DT stock |
| Metazone `/dev/mtz` | present (`10, 62`), lue (rotation, decodeur camera) | OK (lecture) | pilote Autochips natif (lecture et ecriture : ne rien ecrire sans sauvegarde) |
| Camera de recul, AV-in, AVM (TVD, DI, NR, WCH, backcar) | run 64 : plus de panique, **TP9963 detecte** (ID 0x2863, metazone 3), image non verifiee ; `backcar` : `gpio_request` des GPIO SDBC (331) et HDAVM (333) refuse, **comme le DT stock le prevoit** : broche 2 (SDBC) aussi dans le noeud `gpios_ioctl` (lue par jancar.services), broche 4 (HDAVM) aussi celle du recepteur IR (`gpio-ir-recv`), GPIO MCU commentee (`#backcar_mcu_notify_gpio`) dans l'overlay stock ; numeros globaux = broche + 329 (165 → `gpio-494`, 167 → `gpio-496`) ; commutateur CVBS (165 / 167) ecrit par LibreHU-service : **CVBS IN 1 (position AV 0/0) et CVBS IN 2 (1/0) OK** avec un lecteur DVD (decodeur TVD, 720 x 576) ; **camera de recul CVBS OK** (vue de recul LibreHU-service, camera 3, 720 x 576) ; AHD (TP9963) : puce detectee, image non testee (le pilote stock ne traite que l'AHD) | **ne marche pas** | pilotes Autochips presents ; decodeur `tp9963` (camera AHD) reconstruit depuis le stock : [VIDEO.md](VIDEO.md). Panique : sequences d'alimentation du paquet pour TP2850 / TP2854 (AVDD, DOVDD, DVDD, AFVDD) ; sur MAIN2, DVDD est `vcamd_main2`, LDO `vcamd` du PMIC qui ne s'enregistre pas (stock aussi), `regulator_get` rend `-EPROBE_DEFER` et ce pointeur d'erreur etait utilise. Avec `CONFIG_JANCAR_SOLUTION` : sequences du noyau stock relues dans son image (GPIO PDN / RST seulement, plus MCLK pour le TP2854), pointeurs d'erreur traites comme absents |
| ARM2 (`dualarm`, affichage rapide) | non teste | **ne marche pas** | pilote Autochips present |
| Telecommande IR (`gpio-ir-receiver`, `rc-jancar`) | non teste | inconnu | table `rc-jancar` relue dans le noyau stock (identite NEC 0x001-0x2FF), absente du paquet |
| Audio | **OK** (sortie RCA) ; quelques « data underflow » sous charge (Geekbench) ; avertissement `atc_alsa` en double au 2e essai de sonde de la carte | non teste | `SND_SOC_ATC` desactive comme le stock. Le paquet Autochips ajoute les controles ALSA du processeur audio TDA7729 pour tout projet sauf `ac8257_evb` / `ac8257_jac` ; l'UJC201 n'en a pas (rien dans le noyau stock) : la HAL audio ecrit un controle, `tda7729_write()` lit un client I2C jamais sonde (NULL). Plus de TDA7729 avec `CONFIG_JANCAR_SOLUTION` |
| Wi-Fi, BT, GPS, FM (modules stock) | **Wi-Fi client (RSSI -59 dBm, 65 Mbit/s en 2,4 GHz) et Wi-Fi Direct / point d'acces OK, BT OK (recherche lente au premier test : connecteur camera / antennes BT-Wi-Fi mal branche ; rebranche, detection a distance OK), FM OK (RDS)** ; **GPS OK** (position a ±4 m, 21 satellites utilises sur 36 visibles : GPS, GLONASS, Galileo, BeiDou, SBAS) | OK (CRC du stock exportes) | essayer clang (CRC natifs) avant de reprendre l'export de CRC |
| USB peripherique, adb | **OK** (adb root, remount) ; USB 1 hote OK, USB 2 adb / hote OK | OK | |
| MCU / CAN (`ttyS1`) | **MCU detecte** | OK | |
| spidev, UART2/3 | non teste | OK | `spi-ac82xx-dev` natif |
| KernelSU Next, overclock, gouverneurs | **OK** : root (`su -c id`), 2,4 GHz stable (Geekbench 5) | OK (CPU 2,3 GHz, GPU 730 MHz) | repris ; CPU jusqu'a 2,6 GHz (2,3 au demarrage), tensions reglables : [TUNING.md](TUNING.md) |
| Ethernet USB (RTL8152/8153, RNDIS, ipheth, AX88179...) | **OK** (RTL8152) ; quelques `Rx status -71` ; les `insmod` de `init.librehu_usbnet.rc` (modules du noyau stock) echouent, sans effet : pilotes integres | modules (`tools/ac8257/usbnet`) | integres au noyau |
| Lecteur CD / DVD USB | lecteur detecte, CD audio lu par LibreHU Disc, puis **panique du noyau** pendant la lecture (21/10/2026, r71) : `Internal error: Accessing user space memory outside uaccess.h routines`, `PC is at mmc_ioctl_cdrom_read_audio` (`CDROMREADAUDIO`, 12 trames). Bug du 4.9 : apres un `READ CD` refuse par le lecteur, `cdrom_read_cdda_bpc` lit le code d'erreur dans `rq->sense`, que `blk_execute_rq` a remis a NULL (son tampon est sur sa pile) ; acces a l'adresse 2, vu comme un acces a la memoire utilisateur. Corrige : tampon de sense propre a `cdrom_read_cdda_bpc` (les noyaux recents l'allouent avec la requete). A retester | absent | `CONFIG_BLK_DEV_SR`, `CONFIG_CHR_DEV_SG`, `CONFIG_UDF_FS` ajoutes (ISO 9660 / Joliet deja la) ; vold d'Android ne monte pas `/dev/block/sr0` : montage root |
| Mise en veille / reprise | non teste | inconnu | |
| Sondes de temperature | **OK** sauf batterie a 0 °C (pas de batterie, chargeur desactive `ATC_DISABLE_BATTERY_CHARGER` : a comparer au stock) | | |
| Systeme de fichiers `/system` (`mmcblk0p31`) | compteur ext4 : 244 erreurs depuis le dernier fsck (premiere le 13/10, `__ext4_new_inode`, derniere `ext4_mb_generate_buddy`) : erreurs enregistrees, pas de nouvelle erreur d'E/S ; `e2fsck -fn` en lecture seule depuis TWRP pour voir l'etat | | |

## Broches et signaux du vehicule

Source des branchements : ivi-services de Jancar (`Platform_AutoChips_8257_Base`, doc de LibreHU-service
`docs/ivi-services/02-hardware.md`), DT stock. Numero global Linux = broche + 329 (`/sys/kernel/debug/gpio`).
✅ OK · ❌ NOK · ⬜ non teste · — sans objet sur cette unite.

| Signal | Relie a | Broche / trame | Sens | Etat | Note |
|---|---|---|---|---|---|
| Marche arriere | SoC | GPIO 2 (`gpio-331`) | entree, actif bas | ✅ | lue via `/dev/gpios_ioctl` ; aussi `backcar_sdbc_gpio` du DT (d'ou le refus `bc_failed`) |
| Marche arriere par la box CAN | MCU / CAN | broadcast `action_backcar_notification` | — | — | box Hiworld non compatible avec la Clio 3 de test ; le fil (GPIO 2) suffit |
| ACC | MCU | trame `00` | — | ✅ | la GPIO 134 existe mais Jancar prend l'ACC sur le MCU |
| Frein a main | MCU | trame `04` | — | ✅ | sens inverse si metazone 65952 = `0x5A5A5A51` |
| Feux (ILL) | MCU | trame `0B` | — | ✅ | |
| Clignotant gauche | SoC (selon ivi-services) | GPIO 7 (`gpio-336`) ; faisceau B1 « L-TURN » | entree, actif bas | — | lue `lo` au repos, inchangee avec B1 a la masse ou a +12 V (via 1 kΩ) : non cable sur cette unite |
| Clignotant droit | SoC (selon ivi-services) | GPIO 6 (`gpio-335`) ; faisceau B2 « R-TURN » | entree, actif bas | — | lue `hi` au repos, inchangee avec B2 a la masse ou a +12 V : non cable sur cette unite |
| Ecran allume / eteint | SoC | GPIO 5 (`gpio-334`) | sortie, 1 = allume | ✅ | `out hi` ; la luminosite est le PWM `lcd-backlight` |
| Mute ampli | SoC | GPIO 166 (`gpio-495`) | sortie, 1 = muet | ✅ | `out lo`, son OK |
| Commutateur video SW1 / SW2 | SoC | GPIO 167 / 165 (`gpio-496` / `gpio-494`) | sorties | ✅ | CVBS IN 1 (0/0), IN 2 (1/0), recul CVBS (1/1) OK |
| Alimentation antenne radio | SoC + MCU | GPIO 110, commande MCU `43` | sortie | ✅ | FM recue avec RDS |
| Tuner I2C (reset) | SoC | GPIO 112 | sortie | — | pas de tuner I2C (`jancar.radio.id = -1`), FM interne MT6631 |
| Reset hub USB | SoC | GPIO 174 | sortie, impulsion 0 | ⬜ | a la coupure de l'ACC |
| Inconnues | SoC | GPIO 164, 97 | sorties, 1 au demarrage | ⬜ | |
| Liaison MCU | SoC | `/dev/ttyS1`, 115200 bauds | — | ✅ | MCU detecte, commandes executees (AMP CONT) |
| AMP CONT (allumage ampli externe / caisson) | MCU | commande `44` (`44 01` / `44 00`) | sortie | ✅ | coupee a l'ACC off par ivi-services / LibreHU-service |
| Tactile GT911 | SoC | RST GPIO 45, INT GPIO 29, I2C 3 `0x5d` | — | ✅ | |
| Recepteur IR | SoC | broche 4 (`gpio-333`) | entree, IRQ | ⬜ | aussi `backcar_hdavm_gpio` du DT |
| Detection recul HD (`backcar`) | SoC | broche 1 (`gpio-330`) | entree | ⬜ | prise par le pilote `backcar` |
| Decodeur camera TP9963 | SoC | I2C `0x45`, PDN / RST | — | ✅ | repond (ID `0x2863`) |
| Processeur audio BD37534 | SoC | I2C 6 `0x40` | — | ✅ | pilote par Android, son OK |
