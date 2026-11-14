# Essai QEMU du noyau 4.19.325 de l'AC8257

Fait demarrer le noyau `experimental-4.19.325` de l'UJC201 dans QEMU (machine `virt`, 4 x Cortex-A53, sans materiel
MediaTek) pour verifier le coeur et le chemin de demarrage, avant un essai sur l'unite. **Ne remplace pas l'essai
sur l'unite** : eMMC, ecran, tactile, audio, Wi-Fi / BT, USB et CAN ne peuvent etre valides que la.

## Ce qui est teste (`run.sh`)
| Scenario | Ligne de commande | Attendu |
|---|---|---|
| `initramfs` | `rdinit=/init` + initramfs de l'init de test | espace utilisateur atteint (4 CPU, /proc, /sys, socket, systemes de fichiers) |
| `dm-verity` | `skip_initramfs dm="1 vroot none ro 1,0 ... verity 1 PARTUUID=... ..." root=/dev/dm-0`, comme le LK avec un vbmeta qui verifie system | dm-0 cree, ext4 monte en lecture seule, `/init` lance |
| `dm-verity-corrupt` | idem sur un disque avec un octet de `/init` change | « data block N is corrupted », redemarrage « dm-verity device corrupted » |
| `partuuid` | `skip_initramfs root=PARTUUID=...`, comme le LK avec un vbmeta en `--disable-verity` / `--disable-verification` | partition montee, `/init` lance |
| `partuuid-missing` | `root=PARTUUID=` d'aucune partition | le noyau abandonne apres son attente de 10 s |

## Ecarts avec l'unite
- Variante du noyau : `ac8257_ujc201_defconfig` + `qemu.config` (console PL011 et disque virtio de QEMU).
- GICv2 : l'init GICv3 de ce noyau appelle l'extension GIC de MediaTek (`mt_gic_ext_init()`), qui a besoin des
  registres `mediatek,mcucfg` absents de QEMU.
- `initcall_blacklist.txt` : 19 initcalls MediaTek / AutoChips sautes, ils ecrivent des registres du SoC (horloges,
  PMIC, watchdog, thermique, SSPM...) absents de QEMU. Comment la liste a ete trouvee : `logs/blacklist-search.txt`.
- L'extinction finit par une oops dans les pilotes MediaTek (pas de RGU ni de PMIC dans QEMU) : `run.sh` arrete QEMU
  des que le resultat est connu.

## Lancer en local
Paquets (Ubuntu 22.04, comme la CI) :

```bash
sudo apt-get install -y --no-install-recommends bc bison flex libssl-dev libelf-dev cpio python2 clang-14 lld-14 llvm-14 binutils-aarch64-linux-gnu qemu-system-arm cryptsetup-bin gdisk e2fsprogs
```

`python` doit etre Python 2 (generateur DTS d'AutoChips) ; sur Ubuntu 24.04 il n'y a plus de paquet `python2`, le
compiler (2.7.18) et le mettre en tete du `PATH`. Puis, depuis la racine du noyau :

```bash
tools/qemu/build_kernel.sh out-qemu
```

```bash
tools/qemu/build_init.sh qemu-work out-qemu
```

```bash
tools/qemu/make_disk.sh qemu-work qemu-work/init
```

```bash
for s in initramfs dm-verity dm-verity-corrupt partuuid partuuid-missing; do tools/qemu/run.sh out-qemu/arch/arm64/boot/Image qemu-work $s; done
```

Les consoles vont dans `qemu-work/<scenario>.log`. Apres une modification du noyau qui fait planter un nouvel
initcall MediaTek dans QEMU : `tools/qemu/qemu_iter.sh out-qemu/arch/arm64/boot/Image qemu-work` donne les noms a
ajouter a `initcall_blacklist.txt`.

## CI
`.github/workflows/qemu.yml` fait tout cela a chaque push sur la branche `qemu` (ou a la main) et garde les consoles
en artefact. Il ne publie rien.

## Fichiers
- `qemu.config` : options ajoutees a `ac8257_ujc201_defconfig` ;
- `initcall_blacklist.txt` : initcalls sautes ;
- `init.c` : init de test (statique, appels systeme bruts) ;
- `build_kernel.sh`, `build_init.sh`, `make_disk.sh`, `run.sh`, `qemu_iter.sh` : voir leur en-tete ;
- `logs/` : consoles de la session du 14/11/2026 (`initcall-debug.log` : demarrage avec `initcall_debug`, durees de
  tous les initcalls).
