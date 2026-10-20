# Noyau LibreHU pour l'UJC201 (Jancar, Autochips AC8257)

Noyau Linux 4.9.117 de l'autoradio Jancar UJC201 (SoC Autochips AC8257), construit sur les **sources GPL officielles
d'Autochips** recues de SIXWIN / Jancar le 19/10/2026, auxquelles on ajoute les modifications LibreHU.

**Etat** : compile en CI (`Image.gz-dtb`, images boot et recovery de test). **Pas encore teste sur l'unite.**

> Pour l'instant cette branche est le noyau Autochips « nu » avec la config du noyau stock #72 et seulement ce qu'il
> faut pour qu'il compile : aucune modification LibreHU n'y est encore reprise (voir [Modifications a reprendre](#modifications-a-reprendre)).

## Les branches

| Branche | Contenu | Regle |
|---|---|---|
| **`LibreHU-main`** (par defaut) | `autochips-gpl` + nos modifications | **branche de travail** : tout nouveau travail sur le noyau arrive ici |
| `autochips-gpl` | paquet GPL Autochips **tel que recu** (`kernel-4.9.tgz`, SHA-256 `f0303951…d83c577`) | jamais modifiee ; un nouvel envoi de Jancar y est ajoute tel quel |
| `reverse-engineering` | noyau reconstruit sans les sources Autochips (base publique MediaTek MT6761 + reverse engineering) ; demarre la ROM stock | reference : on y reprend les modifications ; garde aussi les notes de l'appareil (`docs/ujc201/`) et les outils `tools/ac8257/` (usbnet, audio, dump) |

```
autochips-gpl ──(git merge autochips-gpl)──▶ LibreHU-main ◀──(reprise commit par commit)── reverse-engineering
```

- Nouvel envoi de Jancar (pilotes manquants) : ajoute sur `autochips-gpl`, puis `git merge autochips-gpl` dans
  `LibreHU-main`.
- Modification de `reverse-engineering` : adaptee aux chemins Autochips (`ac8257` au lieu de `mt6765` / `mt6761`), un
  groupe par commit, CI verte, puis test sur l'unite. Suivi dans [docs/PORTING.md](docs/PORTING.md).

## Ce que `LibreHU-main` ajoute deja au paquet Autochips

Uniquement de quoi compiler le noyau de l'unite (detail : [docs/BUILD.md](docs/BUILD.md)) :
- **CI** (`.github/workflows/build.yml`) avec le compilateur du stock (AOSP clang 6.0.2 `clang-4691093` + binutils
  GCC 4.9) et les variables de `kenv.mk` du stock (`ATC_DISABLE_BATTERY_CHARGER=yes`, `ATC_FASTDISP_VERSION=2.0`) ;
  artefacts : `Image.gz-dtb`, `config`, `recovery_test_ac8257.img`, `boot_ac8257.img`.
- **defconfig de l'unite** `ac8257_ujc201_defconfig` = config complete du noyau stock #72, sans les options dont les
  sources manquent (`JANCAR_*`, capteur `tp9963`, overlay `UJC201_64`, tactiles d'autres ecrans).
- **Reparations du paquet** (fait sous Windows) : 27 liens symboliques, 3 bits d'execution.
- **Fichiers generes par le build Android d'Autochips**, absents du paquet, repris du noyau stock : memoire reservee
  (`arch/arm64/boot/dts/generated/ac8257_reserved_memory.dtsi`) et table des partitions eMMC
  (`drivers/mmc/host/mediatek/ComboA/ac8257/atc_storage_partition.h`).
- **Outils d'image** : `tools/ac8257/repack_boot.py` + gabarits stock (boot, vbmeta du recovery).

## Modifications a reprendre

Depuis `reverse-engineering` (commits et etat : [docs/PORTING.md](docs/PORTING.md)). Beaucoup de corrections de la
reconstruction (device tree, frame buffer, HWC, ext4 `autoformat`, metazone, spidev...) sont **deja natives** dans
le code Autochips et n'ont pas a etre reprises.

| Modification | Pourquoi | Etat |
|---|---|---|
| Jancar `/dev/gpios_ioctl` | utilise par `com.jancar.services` ; `CONFIG_JANCAR_GPIOS` absent du paquet | a reprendre |
| Outils de mise au point (console pstore precoce, panique minutee, retour en recovery, one-shot) | methode de test depuis la partition recovery | a reprendre |
| KernelSU Next (v3.4.0-legacy, hooks manuels) | root, fonctionne avec le gestionnaire KernelSU Next v3.4.0 | a reprendre |
| Gouverneurs CPU (schedutil, conservative...), tables d'overclock CPU / GPU plafonnees aux maxima stock | reglables depuis Kernel Adiutor / SmartPack et l'onglet Tweaks de LibreHU-service | a reprendre |
| Boost tactile, zram lz4, ordonnanceur I/O deadline | fluidite | a reprendre |
| Export des CRC de symboles du noyau stock | seulement si les modules vendor stock (Wi-Fi, BT, GPS, FM) refusent de se charger ; avec le meme compilateur que le stock, les CRC natifs devraient correspondre | a verifier d'abord |
| Tactile GT928 (inversion des axes), USB (connexion au premier pull-up), IRQ d'affichage | corrections faites sur la reconstruction | a verifier sur l'unite, reprendre si besoin |

## Roadmap

1. **Compilation** — ✅ fait (CI run 40, 20/10/2026).
2. **Premier demarrage** de ce noyau « presque stock » depuis la partition recovery : affichage, tactile, eMMC,
   `/data`, adb ; comparer la config produite (artefact `config`) avec celle du stock et le DT `ac8257` avec le DT
   stock.
3. **Parite avec le stock** : modules vendor (Wi-Fi, BT, GPS, FM), metazone, audio, touches, IR, MCU / CAN ; puis ce
   que la reconstruction n'a jamais fait marcher et dont les pilotes Autochips sont ici : **camera de recul, AV-in,
   AVM** (TVD, DI, NR, WCH, backcar) et **ARM2** (`dualarm`, affichage rapide), sauf le decodeur `tp9963` qui manque.
4. **Ajouts LibreHU** : `/dev/gpios_ioctl`, KernelSU Next, gouverneurs et overclock, boost tactile, zram, I/O.
5. **Usage quotidien** : installation dans la partition `boot` (et plus seulement le test recovery), retrait des
   options de mise au point, images publiees en release.

## Futurs ajouts (pistes)

- **Sources manquantes de Jancar**, redemandees le 19/10/2026 : pilotes `JANCAR_SOLUTION` / `JANCAR_GPIOS`, pilote
  LCM `jac_*`, `lowmemorykiller` (`jancar.services`), telecommande `rc-jancar`, overlay `UJC201_64`, capteur
  `tp9963`, tactiles d'autres ecrans. A leur arrivee : `autochips-gpl` puis fusion ici.
- **Modules hors arbre recompiles depuis les sources** (`wmt_drv`, `bt_drv`, `gps_drv`, `fmradio_drv`,
  `wmt_chrdev_wifi`, `wlan_drv_gen4m`, `met`, `fpsgo`) : a demander a Jancar, ou a adapter depuis l'arbre GPL
  Samsung MT6765 (meme famille de puces, connectivite MT6631) ; d'ici la, les modules stock.
- **Adaptateurs Ethernet USB** : modules (dont `r8152`) construits avec ce noyau, comme `tools/ac8257/usbnet` le fait
  pour le noyau stock.
- **Device tree propre a l'UJC201** au lieu de l'overlay de demonstration Autochips, et pilote d'ecran tire de la
  description de la metazone.
- **Veille / reprise**, et camera de recul rapide (ARM2) des le demarrage normal.

## Documentation

| Document | Contenu |
|---|---|
| [docs/README.md](docs/README.md) | index |
| [docs/BUILD.md](docs/BUILD.md) | compiler (CI, en local), particularites de l'arbre Autochips, tester sur l'unite |
| [docs/STATUS.md](docs/STATUS.md) | etat par fonction sur l'unite, compare a `reverse-engineering` |
| [docs/PORTING.md](docs/PORTING.md) | modifications de `reverse-engineering` : natives, a verifier, a reprendre ; suivi |
| [README_LIBREHU.md](README_LIBREHU.md) | provenance du paquet Autochips et ce qui y manque |
| `README` | README d'origine de Linux |

## Regles

- Rien n'est ecrit sous dm-verity et vbmeta n'est jamais desactive automatiquement ; si besoin, a la main :
  `fastboot --disable-verity flash vbmeta vbmeta.img`.
- Tester d'abord depuis la partition recovery, jamais directement dans `boot` : un noyau qui boucle depuis `boot` ne
  se remplace qu'avec SP Flash Tool.
- Ne pas publier ses propres partitions `nvram`, `nvdata`, `proinfo`, `persist`, `metazone`.

Licence : GPL-2.0 (noyau Linux, voir `COPYING`).
