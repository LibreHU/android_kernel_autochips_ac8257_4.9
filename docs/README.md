# Noyau AC8257 LibreHU (branche `LibreHU-main`, base Autochips)

Documentation du noyau de travail de l'UJC201 (Jancar, Autochips AC8257) construit sur les **sources GPL
officielles d'Autochips** (branche `autochips-gpl`), auxquelles on rapporte les modifications utiles du noyau
reconstruit (branche `reverse-engineering`).

| Document | Contenu |
|---|---|
| [BUILD.md](BUILD.md) | compiler (CI, en local), particularites de l'arbre Autochips, tester sur l'unite |
| [STATUS.md](STATUS.md) | etat par fonction sur l'unite (rien n'est encore teste avec ce noyau) |
| [PORTING.md](PORTING.md) | modifications de `reverse-engineering` : deja natives chez Autochips, a verifier, a reprendre ; suivi |
| [../README_LIBREHU.md](../README_LIBREHU.md) | les trois branches, provenance du paquet Autochips et ce qui y manque |

## Les trois branches
| Branche | Role |
|---|---|
| `autochips-gpl` | paquet GPL Autochips tel que recu (SIXWIN / Jancar, 19/10/2026), jamais modifie |
| `reverse-engineering` | noyau reconstruit (base publique MediaTek MT6761 + reverse engineering) ; reference |
| `LibreHU-main` | **branche de travail** : `autochips-gpl` + nos modifications |

Un nouvel envoi de Jancar (pilotes manquants) est ajoute tel quel sur `autochips-gpl`, puis
`git merge autochips-gpl` sur `LibreHU-main`.

## Ce qui manque encore aux sources Autochips
Pilotes et modifications Jancar (`CONFIG_JANCAR_SOLUTION`, `CONFIG_JANCAR_GPIOS`, pilote LCM `jac_*`,
`lowmemorykiller` `jancar.services`, telecommande `rc-jancar`), overlay `UJC201_64`, capteur `tp9963`, tactiles
d'autres ecrans, modules hors arbre (`wmt_drv`, `bt_drv`, `gps_drv`, `fmradio_drv`, `wmt_chrdev_wifi`,
`wlan_drv_gen4m`, `met`, `fpsgo`) : redemandes a Jancar le 19/10/2026. Detail dans `README_LIBREHU.md` ;
analyse complete et sources de remplacement : `docs/ujc201/kernel_gpl.md` de `reverse-engineering`.

## Regles
- Rien n'est ecrit sous dm-verity et vbmeta n'est jamais desactive automatiquement ; si besoin, a la main :
  `fastboot --disable-verity flash vbmeta vbmeta.img`.
- Ne pas publier ses propres partitions `nvram`, `nvdata`, `proinfo`, `persist`, `metazone`.
- Tester d'abord depuis la partition recovery (voir [BUILD.md](BUILD.md)), jamais directement le boot.
