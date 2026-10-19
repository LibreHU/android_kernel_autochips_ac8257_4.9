# Tester un noyau sur l'UJC201 : partition boot, misc, RAM

<!-- Deplace depuis le README du depot TWRP (android_device_alps_ac8257_demo), 19/10/2026. -->
- La partition `boot` stock se termine par un **footer AVB** (son propre vbmeta, cle Jancar) : sans lui le LK
  refuse l'image (logo puis reboot, aucun journal noyau). Le garder en reconditionnant (`mkboot.py` du depot TWRP [android_device_alps_ac8257_demo](https://github.com/LibreHU/android_device_alps_ac8257_demo),
  `tools/ac8257/repack_boot.py`).
- `fastboot boot` ne demarre pas l'image (le LK redemarre) : tester un noyau depuis la partition **recovery**
  (`fastboot oem reboot-recovery`), un crash finit alors sur le boot normal.
- Le « misc » Android de cette unite est la partition **`para`** (message bootloader, commande `boot-recovery`
  de Rescue Party) ; la partition nommee `misc` sert a autre chose. En sauvegarder une copie avant tout test.
- SP Flash Tool reinitialise la RAM : pstore / `last_kmsg` sont alors perdus.
- 6 Go de RAM mais Trusty 32 bits et GPU : le noyau stock alloue leurs tampons sous 4 Go (`GFP_DMA`).
- Le GPU (base MT6761) n'adresse que 32 bits : les tampons ION qu'il recoit (gralloc, composition) doivent
  aussi etre sous 4 Go, sinon la memoire au-dela se corrompt (artefacts, plantages partout dans Android).
  Test decisif : `mem=3G` sur la ligne de commande (toute la RAM sous 4 Go physiques, la RAM commence a 1 Go).
- Un essai rate qui finit en coupure a froid (`[pmic_check_rst] Cold Reset`, `kedump: last is full pmic reset`)
  ne laisse aucun journal : le preloader teste la RAM au demarrage suivant.
- Forcer le drapeau RTC « recovery » a chaque boot pour relancer directement le noyau de test fait boucler
  l'unite si ce noyau ne demarre qu'apres un boot stock ; sortie : SP Flash Tool, partition recovery seule.
- Le LK compte les demarrages en recovery dans la metazone (`INTO_RECOVERY_COUNT`, dword 0x101A2).
Procedure de construction et de test : [docs/BUILD.md](../BUILD.md).
