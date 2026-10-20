# Etat du noyau `LibreHU-main` sur l'UJC201

Tests sur l'unite le 20/10/2026 depuis la partition recovery : run 53 (`7013a1d67`) panique audio a 6,5 s ; run 57
(`aeefa2c28`) jusqu'au launcher, panique camera a 26 s et `/system` protege en ecriture. Voir *Demarrage*, *Audio*,
*eMMC*, *Camera*. Le 21/10/2026, run 64 (`f678cd166`) : utilisable au quotidien, 40 min sans plantage, Geekbench 5
a 2,4 GHz. Colonne « reverse-engineering » : etat atteint par le noyau reconstruit
(son `README.md`), pour comparaison.

| Fonction | `LibreHU-main` (Autochips) | `reverse-engineering` | Notes pour `LibreHU-main` |
|---|---|---|---|
| Compilation CI | **OK** (run 40, `710d83310`) | OK | `Image.gz-dtb` 9,7 Mo (stock 10,3 Mo : pilotes Jancar absents) ; mises au point de l'environnement : [BUILD.md](BUILD.md) |
| Demarrage jusqu'a l'interface Android | **OK** (run 64) ; run 57 : panique camera a 26 s, corrigee | OK | DT stock de l'unite (`mediatek/ac8257_ujc201`), comme `reverse-engineering` |
| eMMC, `/data` (options `autoformat`...) | **OK** ; ecritures sur `/system` refusees (run 57), corrige | OK | options natives ici (`fs/ext4/super.c`) ; protection logicielle (`CONFIG_MMC_ATC_SW_WP`) : le `init.ac8257.rc` du vendor ecrit 1 dans `mmc0/sw_wp_en`, fichier absent du noyau stock (l'ecriture y echoue), present ici : `/system` et les autres partitions « RO » refusaient l'ecriture (EXT4 I/O error). Fichier plus cree avec `CONFIG_JANCAR_SOLUTION` ; `adb remount` affiche alors « set_sw_wp_state open sw_wp_en fail! No such file or directory » puis « remount succeeded » : sans effet, le fichier manque aussi au stock |
| GPU PowerVR GE8300 | **OK** (`GLES: PowerVR Rogue GE8300, OpenGL ES 3.2 build 1.10@5130912`, Geekbench 5) | OK (tampons sous 4 Go) | pilote `m1.10ED5130912` natif, meme DDK que le stock |
| Affichage, HWC, ecran externe | non teste | OK (ABI reconstruite) | `video/ac8257` natif ; `CONFIG_LCM_WIDTH/HEIGHT` 1024 x 600 comme le stock, ecran reel 1280 x 720 (`wm size` du stock) decrit par la metazone (`lcm_driver_common`) |
| Retro-eclairage | **OK**, ecran et touches tactiles (luminosite maximale plus haute que le stock) | OK la plupart du temps | |
| Tactile (dalle Goodix GT911) | **OK** (run 64, `f678cd166`) ; touches virtuelles hors ecran justes apres une calibration LibreHU-service ; avant : hors cadre, matrice restee `1 0 0 0 1 0 1` (identifiant de dalle absent de `/proc/gt9xx_config`) | OK (goodix.c adapte) | pilote MediaTek `GT928` (`mtk-tpd`, RST 45 / INT 29) comme le stock ; inversion de X du paquet desactivee (le stock ne l'a pas) ; calibration Jancar `/sys/devices/platform/touch/gt9xx_props` reprise du stock ([TOUCH.md](TOUCH.md#calibration-gt9xx_props)) |
| Tactiles d'autres dalles (FocalTech, ILITEK, Himax, Hynitron) | non teste | absent | pilotes GPL publics, apres le GT928 : [TOUCH.md](TOUCH.md) |
| `/dev/gpios_ioctl` (Jancar) | present (`10, 45`) ; voir *Camera de recul* (GPIO deja pris) | present | reimplementation de `reverse-engineering` (`drivers/misc/jancar`), noeud du DT stock |
| Metazone `/dev/mtz` | present (`10, 62`), lue (rotation, decodeur camera) | OK (lecture) | pilote Autochips natif (lecture et ecriture : ne rien ecrire sans sauvegarde) |
| Camera de recul, AV-in, AVM (TVD, DI, NR, WCH, backcar) | run 64 : plus de panique, **TP9963 detecte** (ID 0x2863, metazone 3), image non verifiee ; `backcar` : `gpio_request` des GPIO SDBC (331) et HDAVM (333) refuse (deja pris ?), GPIO MCU absente du DT | **ne marche pas** | pilotes Autochips presents ; decodeur `tp9963` (camera AHD) reconstruit depuis le stock : [VIDEO.md](VIDEO.md). Panique : sequences d'alimentation du paquet pour TP2850 / TP2854 (AVDD, DOVDD, DVDD, AFVDD) ; sur MAIN2, DVDD est `vcamd_main2`, LDO `vcamd` du PMIC qui ne s'enregistre pas (stock aussi), `regulator_get` rend `-EPROBE_DEFER` et ce pointeur d'erreur etait utilise. Avec `CONFIG_JANCAR_SOLUTION` : sequences du noyau stock relues dans son image (GPIO PDN / RST seulement, plus MCLK pour le TP2854), pointeurs d'erreur traites comme absents |
| ARM2 (`dualarm`, affichage rapide) | non teste | **ne marche pas** | pilote Autochips present |
| Telecommande IR (`gpio-ir-receiver`, `rc-jancar`) | non teste | inconnu | table `rc-jancar` relue dans le noyau stock (identite NEC 0x001-0x2FF), absente du paquet |
| Audio | **OK** (sortie RCA) ; quelques « data underflow » sous charge (Geekbench) ; avertissement `atc_alsa` en double au 2e essai de sonde de la carte | non teste | `SND_SOC_ATC` desactive comme le stock. Le paquet Autochips ajoute les controles ALSA du processeur audio TDA7729 pour tout projet sauf `ac8257_evb` / `ac8257_jac` ; l'UJC201 n'en a pas (rien dans le noyau stock) : la HAL audio ecrit un controle, `tda7729_write()` lit un client I2C jamais sonde (NULL). Plus de TDA7729 avec `CONFIG_JANCAR_SOLUTION` |
| Wi-Fi, BT, GPS, FM (modules stock) | **Wi-Fi client et Wi-Fi Direct / point d'acces OK, BT OK (recherche et appairage lents), FM OK (RDS)** ; GPS : moteur demarre, pas de position (a tester dehors) | OK (CRC du stock exportes) | essayer clang (CRC natifs) avant de reprendre l'export de CRC |
| USB peripherique, adb | **OK** (adb root, remount) ; USB 1 hote OK, USB 2 adb / hote OK | OK | |
| MCU / CAN (`ttyS1`) | **MCU detecte** | OK | |
| spidev, UART2/3 | non teste | OK | `spi-ac82xx-dev` natif |
| KernelSU Next, overclock, gouverneurs | **OK** : root (`su -c id`), 2,4 GHz stable (Geekbench 5) | OK (CPU 2,3 GHz, GPU 730 MHz) | repris ; CPU jusqu'a 2,6 GHz (2,3 au demarrage), tensions reglables : [TUNING.md](TUNING.md) |
| Ethernet USB (RTL8152/8153, RNDIS, ipheth, AX88179...) | **OK** (RTL8152) ; quelques `Rx status -71` ; les `insmod` de `init.librehu_usbnet.rc` (modules du noyau stock) echouent, sans effet : pilotes integres | modules (`tools/ac8257/usbnet`) | integres au noyau |
| Mise en veille / reprise | non teste | inconnu | |
| Sondes de temperature | **OK** sauf batterie a 0 °C (pas de batterie, chargeur desactive `ATC_DISABLE_BATTERY_CHARGER` : a comparer au stock) | | |
| Systeme de fichiers `/system` (`mmcblk0p31`) | compteur ext4 : 244 erreurs depuis le dernier fsck (premiere le 13/10, `__ext4_new_inode`, derniere `ext4_mb_generate_buddy`) : erreurs enregistrees, pas de nouvelle erreur d'E/S ; `e2fsck -fn` en lecture seule depuis TWRP pour voir l'etat | | |
