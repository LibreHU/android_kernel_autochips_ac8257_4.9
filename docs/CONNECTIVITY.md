# Wi-Fi, Bluetooth, GPS et FM integres au noyau (branche `connectivity-builtin`)

Sur la ROM stock, ces fonctions passent par six modules du vendor (`/vendor/lib/modules`) : `wmt_drv.ko`,
`wmt_chrdev_wifi.ko`, `wlan_drv_gen4m.ko`, `bt_drv.ko`, `gps_drv.ko`, `fmradio_drv.ko`. Ils sont compiles pour le
noyau Jancar (`4.9.117+`, CRC des symboles du stock) et le paquet GPL Autochips n'en a pas les sources.
Cette branche les remplace par des pilotes integres au noyau (`CONFIG_MTK_COMBO_DRV_BUILD_IN=y`). Elle est
separee de `LibreHU-main` tant que ce n'est pas teste sur l'unite.

## Source
Pilotes MediaTek `vendor/mediatek/kernel_modules/connectivity` publies par Samsung (SM-A055F, MT6765 / MT6768,
Linux 4.19), via [rsuntk-oss/mtk_connectivity_module](https://github.com/rsuntk-oss/mtk_connectivity_module)
(`3d48d43`). Provenance et liste des modifications : `drivers/misc/mediatek/connectivity/README.librehu`.

## Adaptation a l'AC8257
| Point | Choix | Pourquoi |
|---|---|---|
| Plateforme WMT | code **MT6761** (`platform/mt6761.c`) lie a `mediatek,ac8257-consys` | 95 des 98 messages `WMT-CONSYS-HW` du `wmt_drv.ko` stock sont dans `mt6761.c` (77 dans `mt6765.c`) ; identifiant de version consys `0x10020300` dans le journal stock, la valeur attendue par `mt6761.c` |
| Identifiant SoC | **0x8257**, comme le stock | `wmt_loader` stock et la commande `WMT_SET_CHIP_ID` envoyee au firmware recoivent la meme valeur qu'avant ; 0x8257 est traite comme 0x6761 dans les tables du code |
| GPS | reglages MT6761 (zone EMI a `0x281000`) | meme decalage dans le `gps_drv.ko` stock |
| Wi-Fi | gen4m CONNAC 1.0 (`soc1_0`), dossier plateforme `mt6765` | pas de dossier `mt6761` ; meme generation, meme firmware `WIFI_RAM_CODE_soc1_0` |
| Memoire DMA Wi-Fi | zone `mediatek,wifi-reserve-memory` du device tree (3 Mo) | comme le `wlan_drv_gen4m.ko` stock |
| FM | puce MT6631 | `chipid:0x6631` dans le journal stock |
| Firmwares | inchanges, lus dans `/vendor/firmware` | rien a ajouter a la ROM |

## Demarrage
1. Au boot, le noyau cree `/dev/wmtdetect` (aucun module a charger).
2. `wmt_loader` (vendor, inchange) envoie l'identifiant de la puce puis demande l'initialisation : le noyau lance
   WMT, Bluetooth, GPS, FM et Wi-Fi d'un coup (« built-in mode » au lieu de « kernel object mode »).
3. Les scripts init du vendor (`init.wmt_drv.rc`, `init.wlan_drv.rc`, `init.bt_drv.rc`, `init.gps_drv.rc`,
   `init.fmradio_drv.rc`) essaient toujours d'`insmod` les six modules : le noyau les refuse
   (`<module>: built into the kernel, module from vendor refused`, erreur `EEXIST`). Rien a modifier dans
   `/vendor`. Sans ce refus, les memes peripheriques seraient enregistres deux fois.

## WPA2 / WPA3
- WPA2 : gere comme avant, par `wpa_supplicant` et le firmware.
- WPA3 (SAE) : toujours **non**. Le pilote sait le faire, mais il lui faut `NL80211_CMD_EXTERNAL_AUTH`, arrive
  dans cfg80211 avec Linux 4.17 ; il est desactive automatiquement sur ce noyau 4.9. Il faudrait aussi
  Android 10 cote framework ([source.android.com : WPA3 et OWE](https://source.android.com/docs/core/connect/wifi-wpa3-owe)).

## Etat
- Compile (clang 6.0.2, CI de la branche).
- **Non teste sur l'unite.** A verifier au premier demarrage : `dmesg | grep -E "WMT|wlan|built into"`,
  `ls /dev/stpwmt /dev/stpbt /dev/stpgps /dev/fm /dev/wmtWifi`, puis Wi-Fi, Bluetooth (appairage, A2DP, appel),
  GPS (YGPS), radio FM.
- En cas de probleme : flasher le `boot` de `LibreHU-main` (modules vendor), rien d'autre n'est touche.
