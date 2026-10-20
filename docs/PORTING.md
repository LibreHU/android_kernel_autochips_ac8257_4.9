# Reprendre les modifications de `reverse-engineering` dans `LibreHU-main`

48 commits de code sur `reverse-engineering` (`git log reverse-engineering -- . ':!docs' ':!*.md' ':!reference'`).
Une partie contournait l'absence des sources Autochips et existe deja ici (verifie dans le paquet) :

| Groupe | Commits | Dans le paquet Autochips | A faire |
|---|---|---|---|
| Compatibles renommes du device tree stock | `179bd19dc` | oui (arbre `ac8257` natif) | rien, a verifier au demarrage |
| Frame buffer `autochips,framebuffer`, ABI HWC (ecran externe, affichage rapide) | `a9ea3a022`, `dfcd7fa01` | oui (`video/ac8257/videox`) | rien |
| Options ext4 `autoformat` / `autorestore` / `permissioncheck` | `abdcad3a5` | oui (`fs/ext4/super.c`) | rien |
| Tactile GT928 (adresse, polling, EINT 42, axes) | `e74ff9a2c`, `a26190740`, `474264ca4`, `bf5607775`, `b6651b70d`, `c4e33f390` | oui : `goodix.c` Autochips + `atc_customize.c` (axes par ecran, `kps_info`), compiles par le stock | rien ; tester |
| Tampons sous 4 Go (Trusty, GPU, ION) | `6a44c9b82`, `4fa7c72d4`, `35b0f246e` | `GFP_DMA` dans ION et Trusty | verifier le GPU et ION avec 6 Go de RAM |
| metazone (`/dev/mtz`, API noyau) | `3c421dd1b` (stubs), `7b96133c9` | oui (`drivers/misc/autochips/metazone`) | rien |
| spidev, UART2/3, horloges | `1e95903d7`, `010e78855`, `dafab7e4c` | `spi-ac82xx-dev` | verifier le majeur 163 pour `wmt_chrdev_wifi` |
| Memoire SPM / SCP reservee | `de3d52f70`, `d4b3d57aa` | non (le DT stock ne les reserve pas : le LK le fait en boot normal) | **fait** (`recovery_mem.c`, boot recovery seulement) |
| IRQ d'affichage sans horloge | `c8cda0229`, `18f94cea6` (garde IRQ) | corrections du pilote `mt6765/dispsys` de la reconstruction ; ici `video/ac8257` du stock | rien ; tester |
| Jancar `/dev/gpios_ioctl` | `ef2ff5ff3` | non (`CONFIG_JANCAR_GPIOS` absent) | **fait** (`drivers/misc/jancar`, DT stock) |
| **Export des CRC du noyau stock** (modules vendor wifi/bt/gps/fm) | `3c421dd1b` (+ `tools/ac8257/stock/Module.symvers`) | non | reprendre si les modules stock refusent de se charger |
| USB : connexion au premier pull-up | `9f32427be` | oui (`musb_gadget.c`, `CONFIG_ATC_AOSP_ENHANCEMENT` sans chargeur) | rien |
| Mise au point (console pstore precoce, panique minutee, retour en recovery, one-shot) | `5d50c2353`, `e833e4408`, `66b3140c6`, `d415557dc`, `27cc3b086`, `33bf59608`, `912ebc302`, `8ef6d980f`, `144a90eb6` | non | **fait** (`drivers/misc/librehu`) |
| Outils d'image (`repack_boot`) et CI | `d0c82e5d5`, `cd7e19590`, `dc209546f` | — | **fait** |
| Overclock CPU / GPU, gouverneurs, reglages | `eb3b1ba14`, `59871d544`, `ca26db327`, `3e3d3eda1`, `53bd202ce`, `365db127c`, `0da8dc694` | non | **fait**, etendu : CPU jusqu'a 2,6 GHz (2,2 GHz au demarrage), tensions CPU et VCORE reglables ([TUNING.md](TUNING.md)) |
| KernelSU Next | `54f36f715` | non | **fait** |

Methode : `git show <commit>` sur `reverse-engineering`, adapter aux chemins Autochips (`mt6765` / `mt6761` cote
reconstruction, `ac8257` ici), un groupe par commit, CI verte puis test sur l'unite par la methode recovery
(`docs/BUILD.md` de `reverse-engineering`). Ne jamais desactiver dm-verity ni ecrire vbmeta automatiquement.

## Suivi
Une ligne par groupe repris (commit de `LibreHU-main`, CI, test sur l'unite) :

| Groupe | Commit `LibreHU-main` | CI | Test unite |
|---|---|---|---|
| Outils d'image et CI | `a70448108`, `ba5b44fd2`, `26bbe7fbc` | OK (run 40) | — |
| defconfig de l'unite (config stock #72) | `a35f816ad` | OK (run 40) | non teste |
| Compilateur du stock (clang `4691093`, `libtinfo5`) | `98b34d316`, `0b1bd57c7` | OK (run 40) | — |
| Paquet Windows : 27 liens, 3 bits d'execution | `069a4cf4d`, `65918ba8b` | OK (run 40) | — |
| Fichiers generes par le build Android : memoire reservee, table des partitions ; `ATC_FASTDISP_VERSION=2.0` | `65918ba8b`, `710d83310` | OK (run 40) | non teste |
| DT stock de l'unite (`mediatek/ac8257_ujc201`), `/dev/gpios_ioctl` | `902fe08c6` | OK (run 44) | non teste |
| KernelSU Next | `fe434692f` | OK (run 45) | non teste |
| Reglages et outils de mise au point (`drivers/misc/librehu`) | `ca58c1354` | OK (run 45) | non teste |
| Overclock CPU 2,6 GHz / GPU 730 MHz, tensions CPU et VCORE | `c9be1fcd8` | en cours | non teste |
| Ethernet USB integre | `21075fda0` | en cours | non teste |
