# Etat du noyau `LibreHU-main` sur l'UJC201

Premier test sur l'unite le 20/10/2026 (CI run 53, `7013a1d67`, partition recovery) : voir *Demarrage* et *Audio*. Colonne « reverse-engineering » : etat atteint par le noyau reconstruit
(son `README.md`), pour comparaison.

| Fonction | `LibreHU-main` (Autochips) | `reverse-engineering` | Notes pour `LibreHU-main` |
|---|---|---|---|
| Compilation CI | **OK** (run 40, `710d83310`) | OK | `Image.gz-dtb` 9,7 Mo (stock 10,3 Mo : pilotes Jancar absents) ; mises au point de l'environnement : [BUILD.md](BUILD.md) |
| Demarrage jusqu'a l'interface Android | **init Android atteint** (6,5 s : KernelSU, `wmt_loader`), puis panique audio (TDA7729, corrigee) ; a retester | OK | DT stock de l'unite (`mediatek/ac8257_ujc201`), comme `reverse-engineering` |
| eMMC, `/data` (options `autoformat`...) | non teste | OK | options natives ici (`fs/ext4/super.c`) |
| GPU PowerVR GE8300 | non teste | OK (tampons sous 4 Go) | pilote `m1.10ED5130912` natif, meme DDK que le stock |
| Affichage, HWC, ecran externe | non teste | OK (ABI reconstruite) | `video/ac8257` natif ; `CONFIG_LCM_WIDTH/HEIGHT` 1024 x 600 comme le stock, ecran reel 1280 x 720 (`wm size` du stock) decrit par la metazone (`lcm_driver_common`) |
| Retro-eclairage | non teste | OK la plupart du temps | |
| Tactile (dalle Goodix GT911) | non teste | OK (goodix.c adapte) | pilote MediaTek `GT928` (`mtk-tpd`, RST 45 / INT 29) comme le stock ; inversion de X du paquet desactivee (le stock ne l'a pas) |
| Tactiles d'autres dalles (FocalTech, ILITEK, Himax, Hynitron) | non teste | absent | pilotes GPL publics, apres le GT928 : [TOUCH.md](TOUCH.md) |
| `/dev/gpios_ioctl` (Jancar) | non teste | present | reimplementation de `reverse-engineering` (`drivers/misc/jancar`), noeud du DT stock |
| Metazone `/dev/mtz` | non teste | OK (lecture) | pilote Autochips natif (lecture et ecriture : ne rien ecrire sans sauvegarde) |
| Camera de recul, AV-in, AVM (TVD, DI, NR, WCH, backcar) | non teste | **ne marche pas** | pilotes Autochips presents ; decodeur `tp9963` (camera AHD) reconstruit depuis le stock : [VIDEO.md](VIDEO.md) |
| ARM2 (`dualarm`, affichage rapide) | non teste | **ne marche pas** | pilote Autochips present |
| Telecommande IR (`gpio-ir-receiver`, `rc-jancar`) | non teste | inconnu | table `rc-jancar` relue dans le noyau stock (identite NEC 0x001-0x2FF), absente du paquet |
| Audio | panique au demarrage (run 53), corrigee | non teste | `SND_SOC_ATC` desactive comme le stock. Le paquet Autochips ajoute les controles ALSA du processeur audio TDA7729 pour tout projet sauf `ac8257_evb` / `ac8257_jac` ; l'UJC201 n'en a pas (rien dans le noyau stock) : la HAL audio ecrit un controle, `tda7729_write()` lit un client I2C jamais sonde (NULL). Plus de TDA7729 avec `CONFIG_JANCAR_SOLUTION` |
| Wi-Fi, BT, GPS, FM (modules stock) | non teste | OK (CRC du stock exportes) | essayer clang (CRC natifs) avant de reprendre l'export de CRC |
| USB peripherique, adb | non teste | OK | |
| MCU / CAN (`ttyS1`) | non teste | OK | |
| spidev, UART2/3 | non teste | OK | `spi-ac82xx-dev` natif |
| KernelSU Next, overclock, gouverneurs | non teste | OK (CPU 2,3 GHz, GPU 730 MHz) | repris ; CPU jusqu'a 2,6 GHz (2,3 au demarrage), tensions reglables : [TUNING.md](TUNING.md) |
| Ethernet USB (RTL8152/8153, RNDIS, ipheth, AX88179...) | non teste | modules (`tools/ac8257/usbnet`) | integres au noyau |
| Mise en veille / reprise | non teste | inconnu | |
