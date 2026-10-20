# Dalles tactiles de l'UJC201

Jancar monte plusieurs dalles selon les series ; son noyau stock embarque le pilote de chacune et garde celui qui
trouve une puce au demarrage. Inventaire fait sur le noyau stock (`4.9.117+ #25` / `#72` : chemins de sources,
symboles, chaines, `IKCONFIG`), l'overlay `dtbo` stock de l'unite et ses journaux.

## Comment le noyau choisit le pilote
- L'overlay stock (`UJC201_64`) declare la dalle sur le bus I2C 3 : `cap_touch@5d` (`mediatek,cap_touch`) pour le
  cadre tactile MediaTek (TPD), plus `ctp@01` (`goodix,gt928`, liaison TI) pour `goodix.c`. `tpd-resolution`
  720 x 1280, 5 points.
- Le cadre MediaTek (`mtk_tpd.c`) essaie les pilotes TPD **dans l'ordre d'edition de liens** et s'arrete au premier
  qui trouve une puce. Le GT928 est lie en premier, comme dans le stock : sur une unite a dalle GT911 les autres
  pilotes ne sont jamais essayes.
- `goodix.c` sonde d'abord `ctp@01` et abandonne (« Ti link support but noack »), comme dans le stock.

## Dalles connues et pilotes

| Puce (dalle) | Pilote du stock | Dans `LibreHU-main` | Source |
|---|---|---|---|
| **Goodix GT911** (ton unite : ID 911, firmware 1060, 1080 x 600 annonces) | `GT928` MediaTek TPD (`mtk-tpd`, RST GPIO 45, INT GPIO 29) ; `goodix.c` (`gt911`, `gt9110`, `gt912`, `gt927`, `gt9271`, `gt928`, `gt967`) | **oui** (paquet Autochips) ; inversion de X du paquet desactivee, absente du stock | paquet GPL Autochips |
| FocalTech **FT8201** (TDDI) | FocalTech V3.4 20211214 (`fts_ts`, `CONFIG_TOUCHSCREEN_FTS`) | oui : V3.0 20190102 configuree FT8201, `CONFIG_TOUCHSCREEN_FTS` | [MediaTek-Pie/kernel-4.9](https://github.com/MediaTek-Pie/kernel-4.9) `focaltech_hd_touch` |
| ILITEK TDDI (puce non nommee dans les chaines) | ILITEK V3 3.0.12.0.241001 (`ILITEK_TDDI`, `ILITEK_V3/ilitek_v3.c`) | oui : V3 3.0.3.0.200610, I2C | [CartelProject/MT6765-4.9](https://github.com/CartelProject/MT6765-4.9) `ili9881x` |
| Himax **HX83102D / E / J** (TDDI, zero-flash) | Himax 2.1.0.12 (`himax,hxcommon`, `Himax_tp`) | oui : 2.0.0.68, HX83102 A / B / D / **E** (pas **J**) | [gta7lite/android_kernel_samsung_mt6765](https://github.com/gta7lite/android_kernel_samsung_mt6765) `hxchipset_8789p1_8185p3` |
| Hynitron **CST340**, **CJT540** | `Hyn_device` (`mediatek,cap_touch_cst`, firmwares `HYN_CST3_1.bin`, `HYN_CST5_1.bin`) | oui : pilote Linux officiel `hynitron_cstxxx` (CST3xx) retroporte, aussi sur `mediatek,cap_touch_cst` | [torvalds/linux](https://github.com/torvalds/linux/blob/master/drivers/input/touchscreen/hynitron_cstxxx.c) |
| Synaptics TouchComm (TCM2) | `synaptics_tcm_i2c`, `CONFIG_TOUCHSCREEN_SYNA_TCM2` | **non** | seule source publique : celle de Samsung, liee au format de rapport des firmwares Samsung |
| Synaptics RMI4 (DSX) | `synaptics_dsx` (`synaptics_dsx_i2c.c`, `rmi_dev`, `fw_update`, `test_reporting`) | **non** | pilotes generiques Synaptics publics ([synaptics-dsx-v2.1](https://github.com/synaptics-touch/synaptics-dsx-v2.1)), mais pas la variante MediaTek du stock ni son noeud de device tree |
| Sitronix **ST7123** (TDDI) | `sitronix_ts_i2c` / `sitronix_ts_spi` (« MTK_TPD ») | **non** | aucune source publique de la variante MediaTek trouvee |
| Jadard **JD9366T** (TDDI) | `jadard_tp` (`module_jd9366t_flash`) | **non** | aucune source publique trouvee |

TDDI : la meme puce pilote l'ecran et le tactile. Le cote ecran passe par `lcm_driver_common`, configure par la
metazone ; les ajouts Jancar de ce pilote (`jac_*`) manquent au paquet (voir `README_LIBREHU.md`).

## Firmware des dalles
Aucun pilote ajoute n'ecrit dans la puce au demarrage :
- FocalTech : pas de firmware integre (`FTS_AUTO_UPGRADE_EN 0`) ; les modules de mise a jour qui demandent un
  « pramboot » (FT6216, FT7251, FT8006M/P/U, FT8607, FT8613S, FT8716, FT8719, FT8736, FT8739) sont retires, leurs
  fichiers ne sont pas dans les arbres publics ;
- ILITEK : le firmware d'une autre tablette present dans la source n'est pas repris, `BOOT_FW_UPDATE` reste coupe ;
- Himax zero-flash : la puce n'a pas de flash, le pilote charge son firmware **en RAM** a chaque demarrage depuis
  `/vendor/firmware/Himax_firmware.bin` (fichier de la ROM stock pour ces dalles) ; rien n'est ecrit dans la puce.

## Reconnaitre sa dalle
- `dmesg | grep -iE "GTP|FTS_TS|ILITEK|HXTP|HYN|Himax"` : le pilote qui a trouve la puce le dit.
- `getevent -p` : nom du peripherique (`mtk-tpd` pour les pilotes TPD).
- Sur le stock : `cat /proc/bus/input/devices`.

Non teste sur une unite a dalle autre que GT911 : ces pilotes sont la au cas ou, ils compilent en CI et ne
changent rien sur une unite a GT911.
