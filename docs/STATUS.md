# Etat du noyau `main` sur l'UJC201

Aucun test sur l'unite pour l'instant. Colonne « reverse-engineering » : etat atteint par le noyau reconstruit
(son `README.md`), pour comparaison.

| Fonction | `main` (Autochips) | `reverse-engineering` | Notes pour `main` |
|---|---|---|---|
| Compilation CI | en cours de mise au point | OK | Python 2, `-fcommon`, clang `4691093`, liens, bits d'execution, memoire reservee : voir [BUILD.md](BUILD.md) |
| Demarrage jusqu'a l'interface Android | non teste | OK | DT `ac8257` d'Autochips au lieu du DT stock decompile |
| eMMC, `/data` (options `autoformat`...) | non teste | OK | options natives ici (`fs/ext4/super.c`) |
| GPU PowerVR GE8300 | non teste | OK (tampons sous 4 Go) | pilote `m1.10ED5130912` natif, meme DDK que le stock |
| Affichage, HWC, ecran externe | non teste | OK (ABI reconstruite) | `video/ac8257` natif, ecran 1024 x 600 dans la defconfig |
| Retro-eclairage | non teste | OK la plupart du temps | |
| Tactile GT928 | non teste | OK (goodix.c adapte) | pilote GT928 Autochips natif |
| `/dev/gpios_ioctl` (Jancar) | **absent** | present | a reprendre (`CONFIG_JANCAR_GPIOS` manque aux sources) |
| Metazone `/dev/mtz` | non teste | OK (lecture) | pilote Autochips natif (lecture et ecriture : ne rien ecrire sans sauvegarde) |
| Camera de recul, AV-in, AVM (TVD, DI, NR, WCH, backcar) | non teste | **ne marche pas** | pilotes Autochips presents ; decodeur `tp9963` absent |
| ARM2 (`dualarm`, affichage rapide) | non teste | **ne marche pas** | pilote Autochips present |
| Audio | non teste | non teste | `SND_SOC_ATC` desactive comme le stock |
| Wi-Fi, BT, GPS, FM (modules stock) | non teste | OK (CRC du stock exportes) | essayer clang (CRC natifs) avant de reprendre l'export de CRC |
| USB peripherique, adb | non teste | OK | |
| MCU / CAN (`ttyS1`) | non teste | OK | |
| spidev, UART2/3 | non teste | OK | `spi-ac82xx-dev` natif |
| KernelSU Next, overclock, gouverneurs | absent | OK | a reprendre |
| Mise en veille / reprise | non teste | inconnu | |
