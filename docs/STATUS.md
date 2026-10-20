# Etat du noyau `LibreHU-main` sur l'UJC201

Aucun test sur l'unite pour l'instant. Colonne « reverse-engineering » : etat atteint par le noyau reconstruit
(son `README.md`), pour comparaison.

| Fonction | `LibreHU-main` (Autochips) | `reverse-engineering` | Notes pour `LibreHU-main` |
|---|---|---|---|
| Compilation CI | **OK** (run 40, `710d83310`) | OK | `Image.gz-dtb` 9,7 Mo (stock 10,3 Mo : pilotes Jancar absents) ; mises au point de l'environnement : [BUILD.md](BUILD.md) |
| Demarrage jusqu'a l'interface Android | non teste | OK | DT stock de l'unite (`mediatek/ac8257_ujc201`), comme `reverse-engineering` |
| eMMC, `/data` (options `autoformat`...) | non teste | OK | options natives ici (`fs/ext4/super.c`) |
| GPU PowerVR GE8300 | non teste | OK (tampons sous 4 Go) | pilote `m1.10ED5130912` natif, meme DDK que le stock |
| Affichage, HWC, ecran externe | non teste | OK (ABI reconstruite) | `video/ac8257` natif ; `CONFIG_LCM_WIDTH/HEIGHT` 1024 x 600 comme le stock, ecran reel 1280 x 720 (`wm size` du stock) decrit par la metazone (`lcm_driver_common`) |
| Retro-eclairage | non teste | OK la plupart du temps | |
| Tactile GT928 | non teste | OK (goodix.c adapte) | `goodix.c` + `atc_customize.c` d'Autochips, comme le stock |
| `/dev/gpios_ioctl` (Jancar) | non teste | present | reimplementation de `reverse-engineering` (`drivers/misc/jancar`), noeud du DT stock |
| Metazone `/dev/mtz` | non teste | OK (lecture) | pilote Autochips natif (lecture et ecriture : ne rien ecrire sans sauvegarde) |
| Camera de recul, AV-in, AVM (TVD, DI, NR, WCH, backcar) | non teste | **ne marche pas** | pilotes Autochips presents ; decodeur `tp9963` absent |
| ARM2 (`dualarm`, affichage rapide) | non teste | **ne marche pas** | pilote Autochips present |
| Audio | non teste | non teste | `SND_SOC_ATC` desactive comme le stock |
| Wi-Fi, BT, GPS, FM (modules stock) | non teste | OK (CRC du stock exportes) | essayer clang (CRC natifs) avant de reprendre l'export de CRC |
| USB peripherique, adb | non teste | OK | |
| MCU / CAN (`ttyS1`) | non teste | OK | |
| spidev, UART2/3 | non teste | OK | `spi-ac82xx-dev` natif |
| KernelSU Next, overclock, gouverneurs | non teste | OK (CPU 2,3 GHz, GPU 730 MHz) | repris ; CPU jusqu'a 2,6 GHz (2,2 au demarrage), tensions reglables : [TUNING.md](TUNING.md) |
| Ethernet USB (RTL8152/8153, RNDIS, ipheth, AX88179...) | non teste | modules (`tools/ac8257/usbnet`) | integres au noyau |
| Mise en veille / reprise | non teste | inconnu | |
