# Entrees et sorties video de l'UJC201

Ce que montrent le noyau stock (`4.9.117+ #25` / `#72`), ses journaux et ceux de la HAL camera stock
(`camhal3lite`) sur l'unite.

| Prise / fonction | Puce | Chemin | Pilote noyau |
|---|---|---|---|
| **Camera AHD** (recul, 720p / 1080p, 25 / 30 i/s) | **Techpoint TP9963** (sur le PCB) | MIPI CSI-2 → ISP de l'AC8257 ; capteur `MAIN2`, I2C `0x8a` (7 bits `0x45`) | `tp9963_yuv` (reconstruit, voir plus bas) ; recul rapide par l'**ARM2** au demarrage |
| **CVBS IN 1 / CVBS IN 2** (RCA, video seule) et camera de recul CVBS | decodeur CVBS **integre a l'AC8257** (TVD) | commutateur video analogique (GPIO 167 / 165 du noeud `gpios_ioctl`), puis TVD | `drivers/misc/autochips` (`vss_tvd`, memoire `cvbs-ion`) |
| **VIDEO OUT** (RCA) | puce d'affichage USB MacroSilicon | copie d'ecran envoyee en USB par l'appli Jancar `com.jancar.usbdisplay` | aucun pilote dedie (USB) |
| Camera USB (webcam) | — | USB, UVC | `CONFIG_USB_VIDEO_CLASS=y` (comme le stock) |

## TP9963
- **Present sur l'unite** : la HAL stock le trouve au demarrage, `[searchSensor]sensorIdx 2 found
  <0x2863/tp9963_yuv/SENSOR_DRVNAME_TP9963_YUV>`, une fois l'ARM2 sorti (`arm2 is exited, to search sensor!`).
- Decodeur 2 canaux HD-TVI / AHD / CVI / CVBS vers MIPI CSI-2, meme famille que les TP2850 / TP2854 dont les pilotes
  sont dans le paquet, mais **un pilote a lui** dans le stock : identifiant `0x2863`, autres sequences de registres,
  sortie MIPI 2 canaux. Les pilotes TP2850 / TP2854 ne le reconnaissent pas (ils attendent `0x2850` / `0x2854`).
- Le stock declare `tp2850_yuv tp2854_yuv tp9963_yuv` ; la HAL ne trouve que le TP9963 sur l'unite : TP2850 et
  TP2854 sont prevus pour d'autres cartes (un seul canal pour le TP2850, quatre pour le TP2854 en vue 360) et ne
  repondent pas.
- **Format** : la metazone `0x101e2` (`ADDR_AHD_SENSOR_VALUE` de Jancar, ecrite par ses reglages et par
  LibreHU-service : 0 = 720p25, 1 = 1080p25, 2 = 720p30, 3 = 1080p30) decide, apres la detection automatique ; la
  metazone `0x10045` (format force d'ATC Settings) evite la detection. Le pilote ecrit `0x1019d` = 1 quand la puce
  repond, comme le stock.
- La HAL recoit une fenetre fixe de 1280 x 720 (comme le stock), l'etat du signal des canaux 0 et 1, et les
  reglages luminosite / contraste / saturation / teinte (registres `0x10` a `0x13` du canal).

### Reconstruction
Le paquet GPL n'a pas la source du pilote. Il a ete reconstruit depuis l'image du noyau stock `#25` :
desassemblage des fonctions `TP9963*` (symboles du noyau stock), suivi des registres et de la pile pour retrouver
chaque ecriture I2C (dont les lectures-modifications de `0x42` / `0x43`), tables de saut des formats et des
commandes de la HAL, table des capteurs (`kdSensorList`), sequence d'alimentation et configuration materielle
(lues dans les donnees du noyau, avec la table de relocations).

Verifications :
- les 6 tables de formats et le bloc MIPI sont compares automatiquement aux ecritures du binaire stock ;
- `GetInfo` : chaque champ ecrit par le stock correspond a la structure du paquet et aux valeurs du TP2850 ;
- recoupement avec deux pilotes GPL publics du meme TP9963 : [OpenIPC (Ingenic T41)](https://github.com/OpenIPC/openingenic/blob/f203d6f2e69a4ac187191873364973dca1633624/kernel/sensors/t41/tp9963/tp9963.c)
  et [Allwinner Tina V821](https://github.com/sam-yangjj/tina-v821-v1.3-bsp/blob/4f82c3bed72342d306e1b14755f9618a6ca7a504/drivers/vin/modules/sensor/tp9963.c) :
  meme identifiant (`0xfe` / `0xff` = `0x28` / `0x63`), meme adresse (`0x88` / `0x8a` selon la broche SAD),
  sequences 1080p identiques a la reference Techpoint (`STD_HDA`, sans son `0x18 = 0x2a`), sortie MIPI identique a
  `MIPI_2CH2LANE_594M` (2 canaux, 2 voies), `0x40 = 0x04` = les deux canaux (`CH_ALL`).

Ces pilotes publics n'ont pas servi de source : leur sortie MIPI et leur mode de detection sont propres a leur
plateforme (Ingenic, Allwinner), ils ne connaissent pas l'interface `imgsensor` de MediaTek / Autochips ni les
metazones de Jancar. Ils confirment les valeurs.

Non teste sur l'unite. A verifier au premier demarrage : `dmesg | grep TP9963` (identifiant `0x2863`, format), puis
l'image de la camera AHD dans la vue de recul.

## ARM2 (recul rapide)
Le coprocesseur ARM9 (« ARM2 ») de l'AC8257 affiche la camera de recul des la mise sous tension, avant Android
(AutoChips annonce 2 s en CVBS, 3 s en AHD), avec la memoire reservee `arm2-backcar-*`. Il rend ensuite la main a
Linux, qui reprend le TP9963 ou le TVD. Ses reglages (source CVBS / AHD / 360, mode, miroir, recadrage) sont dans la
metazone (`docs/atc.md` de LibreHU-service).
