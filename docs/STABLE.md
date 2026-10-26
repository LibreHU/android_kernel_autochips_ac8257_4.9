# Mises a jour linux-stable 4.9.y (branches `stable-4.9.*`)

Le paquet Autochips est un Linux **4.9.117** (aout 2018) ; la branche 4.9 s'est arretee a **4.9.337** (7 janvier 2023,
derniere version de Greg Kroah-Hartman). On rattrape les 220 versions par lots, une branche par lot, testee sur
l'unite avant la suivante, puis tout revient dans `LibreHU-main`.

## Methode
- **Branches empilees** : `stable-4.9.150` part de `LibreHU-main`, `stable-4.9.200` de `stable-4.9.150`, etc. On ne
  cree la suivante qu'une fois la precedente validee sur l'unite ; une correction dans un lot est fusionnee dans les
  suivants. La CI construit toute branche `stable-4.9.*` et publie la prerelease `latest-<branche>` (pas de zip dans
  LibreHU Apps : flashablesZips ne prend que les branches declarees).
- **Source** : les commits officiels de linux-stable (`git fetch stable` sur
  `https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git`, tags `v4.9.117`..`v4.9.337`), appliques un par
  un dans l'ordre par `tools/stable/apply.py` avec `git am -3` (fusion a trois voies). Auteur et message d'origine
  gardes, avec `(linux-stable commit <hash>)` ajoute ; les commits `Linux 4.9.N` font monter `SUBLEVEL`.
- **Decisions** (journal `docs/stable/lot*.tsv`, une ligne par commit) :
  - conflit dans un fichier **non compile** avec `ac8257_ujc201_defconfig` (`tools/stable/compiled.py`) : notre
    version est gardee, le reste du correctif applique (ou le correctif ignore s'il ne reste rien) ;
  - conflit ou les lignes amont sont **deja presentes** : notre version gardee ;
  - autre conflit : **resolu a la main**, le commit porte `[LibreHU: conflit resolu ... ]` ;
  - correctif deja la sous une autre forme (noyau commun Android, MediaTek) ou sans objet : non applique, raison notee.
- **Controles** : compilation locale complete (meme chaine que la CI) ; `tools/stable/dupaudit.py` cherche les blocs
  qu'un correctif aurait ajoutes une deuxieme fois (code deja present ailleurs dans le fichier) en comparant au
  fichier amont ; comparaison des CRC de `Module.symvers` avec `LibreHU-main` et le noyau stock.

## Modules du vendor : recompiles avec le noyau (module KernelSU ou zip TWRP)
`CONFIG_MODVERSIONS=y` : le numero de version (4.9.117 -> 4.9.150) n'empeche pas le chargement (`same_magic()` ignore
la version quand les CRC sont presents), mais les CRC des symboles, oui. `LibreHU-main` a le CRC du stock pour
9 635 des 9 643 symboles communs ; apres le lot 1, 5 776 seulement : des structures centrales changent pour
genksyms (`sk_buff` : union `sk` / `ip_defrag_offset` ; `net` / `netns_frags` : defragmentation IP passee en
rhashtable, FragmentSmack ; `ctl_table_header`, `kmem_cache`). Les modules Wi-Fi, BT, GPS et FM de
`/vendor/lib/modules` seraient refuses (`disagrees about version of symbol`), et masquer ces changements a genksyms
ne serait pas sur (la disposition de `struct net` change vraiment).

Donc, sur les branches `stable-4.9.*` (`CONFIG_LIBREHU_CONN_MODULES=y`), ces pilotes sont **recompiles avec le
noyau, en modules** : memes sources GPL MediaTek que `connectivity-builtin` (`drivers/misc/mediatek/connectivity`,
`README.librehu`), memes noms que ceux du stock (`wmt_drv`, `wmt_chrdev_wifi`, `wlan_drv_gen4m`, `bt_drv`,
`gps_drv`, `fmradio_drv`). Le noyau ne grossit pas (ils ne sont pas dans la partition `boot`).

La CI publie `librehu_connectivity_ksu.zip` dans la prerelease : un module **KernelSU** (KernelSU Next -> Modules ->
Installer, puis redemarrer). Son `post-fs-data.sh` (etape bloquante, avant le demarrage de `wmt_loader`) charge les
six modules dans l'ordre des dependances, **seulement si `/proc/version` est exactement celui du noyau compile dans
le meme run** : avec un autre noyau (retour a `LibreHU-main`), il ne fait rien et les modules du vendor servent comme
avant. `/vendor` n'est pas touche. Si un script init du vendor charge ses modules plus tard, ils sont refuses (meme
nom deja charge) ; s'il le fait avant, ils echouent sur les CRC et les notres prennent le relais. Journal :
`/data/adb/modules/librehu_connectivity/last.log`. Outil : `tools/stable/conn_ksu_module.sh`.

**Sans KernelSU : zip TWRP** `conn-modules-stable` du depot
[LibreHU-flashablesZips](https://github.com/LibreHU/LibreHU-flashablesZips) (LibreHU Apps -> Zips -> Noyau). Pour une
installation durable, une fois le noyau flashe dans `boot` (`boot_ac8257.img` ou `boot_ac8257_bootmenu.img` de
`latest-stable-4.9.150`) : il remplace les six modules de `/vendor/lib/modules` par ceux-ci.
- verifie d'abord que la section noyau de la partition `boot` a l'empreinte de l'une de ces deux images, sinon s'arrete
  sans rien modifier (la methode `recovery_test` ne convient donc pas : utiliser le module KernelSU) ;
- s'arrete si dm-verity est actif sur `/vendor` (`fastboot --disable-verity flash vbmeta vbmeta.img`, a faire soi-meme) ;
- sauvegarde les modules d'origine une seule fois dans `/sdcard/LibreHU/backup/conn-modules-orig` (jamais ecrasee),
  et l'etat d'avant chaque passage dans `conn-modules-<date>` (fichiers, droits, proprietaire, contexte SELinux,
  empreintes) ; ecrit chaque module a cote, relit son empreinte, puis renomme ; marqueur `/vendor/lib/modules/.librehu_conn` ;
- **retour arriere** : zip `conn-modules-stable-restore` (remet les originaux, empreintes verifiees avant toute
  modification), a flasher **avant** de revenir a `LibreHU-main` ou au noyau stock, sinon Wi-Fi, BT, GPS et FM ne
  marchent plus avec ce noyau-la.

Un seul des deux suffit ; ensemble, ils chargent les memes fichiers et le second voit les modules deja charges (sans effet).

`fpsgo` (module de performance du vendor, sans sources ici) ne se chargera pas non plus : pas d'effet attendu hors
regulation des images par seconde du service de performance MediaTek.

## Plan des lots

| Branche | Versions | Etat |
|---|---|---|
| `stable-4.9.150` | 4.9.118 -> 4.9.150 | **valide sur l'unite** (demarrage, Wi-Fi, partage de connexion, Bluetooth, FM ; installation durable) |
| `stable-4.9.200` | 4.9.151 -> 4.9.200 | applique, compile (noyau + modules de connectivite) ; **a tester sur l'unite** |
| `stable-4.9.225` | 4.9.201 -> 4.9.225 | applique, compile ; **a tester sur l'unite** |
| `stable-4.9.250` | 4.9.226 -> 4.9.250 | applique, compile ; **a tester sur l'unite** |
| `stable-4.9.300` | 4.9.251 -> 4.9.300 | applique, compile ; **a tester sur l'unite** (CVE-2021-0920 af_unix, binder CVE-2021-39686) |
| `stable-4.9.319` | 4.9.301 -> 4.9.319 | applique, compile ; **a tester sur l'unite** (Spectre-BHB arm64) |
| `stable-4.9.337` | 4.9.320 -> 4.9.337 | a faire (reecriture de `random.c` en 4.9.320) |

## Lot 1 : `stable-4.9.150` (4.9.118 -> 4.9.150)

2178 commits linux-stable : **2086 appliques tels quels**, 11 avec un conflit resolu a la
main, 4 avec un conflit dont les lignes amont etaient deja la,
8 en partie (conflit dans un fichier non compile),
39 deja presents, 23 ignores (conflit
seulement dans du code non compile), 7 non repris volontairement ou sans objet.
Detail complet : [stable/lot1-4.9.118-150.tsv](stable/lot1-4.9.118-150.tsv).

Correctifs utiles a l'unite dans ce lot : fuites d'information du pilote CD-ROM (CVE-2018-10940, CVE-2018-16658,
`cdrom_ioctl_drive_status`), `sr` (ouverture d'un CD-ROM bloquee avec la gestion d'energie, taille du tampon de
sense), use-after-free ION (`ION_IOC_SHARE` / `ION_IOC_MAP`), ashmem (taille du mmap), ext4 (CVE-2018-10880 / 10883
deja presents, les autres appliques), FragmentSmack (CVE-2018-5391), r8152, `protected_fifos` / `protected_regular`.

### Corrections apres application
- `crypto: arm64/sha - avoid non-standard inline asm tricks` (`0a953c058f73`) : applique sans conflit, mais les
  constantes `sha1/sha256_ce_offsetof_*` existaient deja ailleurs dans le fichier (noyau commun Android) : definies
  deux fois, erreur de compilation. Commit retire de la branche (seule duplication trouvee par `dupaudit.py`).
- Makefile : `kbuild: Set KBUILD_CFLAGS before incl. arch Makefile` ajoutait un deuxieme bloc clang (deux cibles
  passees a clang). Un seul bloc, a la place amont, avec le triplet et les avertissements du noyau Android.
- Effet voulu de ce bloc et de `kbuild: fix linker feature test macros when cross compiling with Clang` : les tests
  `ld-option` / `cc-option` marchent enfin avec clang. `--fix-cortex-a53-843419` (erratum du Cortex-A53, notre CPU)
  et `--no-apply-dynamic-relocs` sont reellement passes a l'editeur de liens (ils ne l'etaient pas sur
  `LibreHU-main`), d'ou une image compressee plus petite (9,68 Mo contre 10,11 Mo) ; `-fshort-wchar`,
  `-DCONFIG_AS_LSE=1` (atomiques LSE choisies au demarrage selon le CPU : LL/SC sur l'A53) en plus.

### Conflits resolus a la main
| Commit linux-stable | Correctif | Decision |
|---|---|---|
| `f37486c0a1d0` | cpu/hotplug: Provide knobs to control SMT | cpu.h : idle_notifier (Android) et cpuhp_smt_control gardes tous les deux |
| `3fedc0cd376b` | staging: android: ion: fix ION_IOC_{MAP,SHARE} use-after-free | ION_IOC_SHARE/MAP sous le verrou du client (correctif de securite), journaux et mmprofile MediaTek gardes ; ion_handle_get_by_id() garde pour ion_drv_get_handle() de MediaTek |
| `2679c2231bc3` | xhci: Fix use-after-free in xhci_free_virt_device | xhci : virt_dev->udev = NULL toujours (Autochips le faisait sous CONFIG_ATC_AOSP_ENHANCEMENT) |
| `f5ff43c8837a` | HID: sony: Update device ids | identifiants PS4 deja dans hid_have_special_driver |
| `72386b22d61f` | HID: sony: Support DS4 dongle | identifiant DS4 dongle deja present |
| `e757c1e6e71b` | arm/arm64: smccc-1.1: Make return values unsigned long | SMCCC 1.1 : registres en unsigned long et a0 en (u32), reporte sur les macros SMCCC_REG de l'ACK (clang) |
| `4f689d02c0bf` | arm/arm64: smccc-1.1: Handle function result as parameters | SMCCC 1.1 : resultats de fonction evalues avant les registres (__aN), reporte sur SMCCC_REG (ACK, clang) |
| `b475cf3bf1e8` | net: add rb_to_skb() and other rb tree helpers | rb_to_skb() deja present : seules les macros skb_rb_* ajoutees |
| `2edec22d1875` | Revert "netfilter: ipv6: nf_defrag: drop skb dst before queueing" | annulation stable de nf_defrag drop skb dst (repris dans l'ACK, problematique en 4.9) appliquee |
| `3d26b729ddb2` | ipv6: Fix PMTU updates for UDP/raw sockets in presence of VRF | PMTU VRF : oif = maitre l3mdev si non lie, sk_uid de l'ACK garde |
| `43bc9c671e5e` | usb: xhci: fix timeout for transition from RExit to U0 | delai RExit->U0 en ms (XHCI_MAX_REXIT_TIMEOUT_MS), journal Autochips garde |

### Non repris volontairement ou sans objet
| Commit linux-stable | Correctif | Decision |
|---|---|---|
| `fbf12e19c9f1` | Mark HI and TASKLET softirq synchronous | ksoftirqd_running() absent : le noyau Android a retire le report des softirqs vers ksoftirqd, HI/TASKLET deja synchrones |
| `dbf9a0532e02` | sched/cputime: Convert kcpustat to nsecs | serie 4.9.135 kcpustat en ns : corrige le comptage de ksoftirqd avec CONFIG_IRQ_TIME_ACCOUNTING (desactive chez nous) et fausserait rq_stats, mtk_thermal_platform et blocktag de MediaTek qui lisent kcpustat en cputime |
| `74d6f8eac6ea` | macintosh/rack-meter: Convert cputime64_t use to u64 | serie 4.9.135 kcpustat en ns : corrige le comptage de ksoftirqd avec CONFIG_IRQ_TIME_ACCOUNTING (desactive chez nous) et fausserait rq_stats, mtk_thermal_platform et blocktag de MediaTek qui lisent kcpustat en cputime |
| `034cd8a20fac` | sched/cputime: Increment kcpustat directly on irqtime account | serie 4.9.135 kcpustat en ns : corrige le comptage de ksoftirqd avec CONFIG_IRQ_TIME_ACCOUNTING (desactive chez nous) et fausserait rq_stats, mtk_thermal_platform et blocktag de MediaTek qui lisent kcpustat en cputime |
| `aee0c9e837c2` | sched/cputime: Fix ksoftirqd cputime accounting regression | serie 4.9.135 kcpustat en ns : corrige le comptage de ksoftirqd avec CONFIG_IRQ_TIME_ACCOUNTING (desactive chez nous) et fausserait rq_stats, mtk_thermal_platform et blocktag de MediaTek qui lisent kcpustat en cputime |
| `c466d17e3ab1` | kbuild: drop -Wno-unknown-warning-option from clang options | options de compilation clang validees pour clang 6 (stock) et du code MediaTek en -Werror : -Wno-unknown-warning-option garde |
| `32445bd5c72b` | tty: serial: 8250_mtk: always resume the device in probe. | 8250_mtk tres modifie par MediaTek/Jancar (UART de la MCU, ttyS1) : ordre pm_runtime du probe d'origine garde |

### Deja presents sous une autre forme (noyau commun Android, MediaTek)
| Commit linux-stable | Correctif | Decision |
|---|---|---|
| `50bed434ad9c` | kasan: add no_sanitize attribute for clang builds | KASAN_ABI_VERSION / no_sanitize deja dans compiler-clang.h (ACK) |
| `87a2d84d2ff4` | fix mntput/mntput race | mntput_no_expire deja corrige (avec le smp_mb du correctif suivant) |
| `a96371149006` | arm64: mm: check for upper PAGE_SHIFT bits in pfn_valid() | pfn_valid verifie deja les bits hauts (PFN_MASK) |
| `b469713e0c0c` | ext4: add corruption check in ext4_xattr_set_entry() | CVE-2018-10879 : ext4_xattr_set_entry() verifie deja next >= s->end (ext4 plus recent dans l'ACK) |
| `0050338e0d3c` | ext4: always verify the magic number in xattr blocks | auto : lignes amont deja dans fs/ext4/xattr.c |
| `52f305530ab6` | xhci: Fix USB3 NULL pointer dereference at logical disconnect. | auto : lignes amont deja dans drivers/usb/host/xhci-hub.c |
| `7135c75e00ed` | kbuild: Add better clang cross build support | support clang croise deja dans le Makefile (CLANG_TRIPLE de l'ACK) |
| `85ab13ffcc69` | kbuild: Add support to generate LLVM assembly files | nettoyage *.symversions (ACK) garde en plus des fichiers .ll |
| `ebcc69a97a24` | kbuild: clang: Disable 'address-of-packed-member' warning | auto : lignes amont deja dans Makefile |
| `0a953c058f73` | crypto: arm64/sha - avoid non-standard inline asm tricks | offsetof sha1/sha256 deja exportes en constantes (ACK) : le correctif les dupliquait (erreur de compilation), retire |
| `c0d75dacffb9` | binder: fix proc->files use-after-free | binder MediaTek : files_lock protege deja proc->files (task_get_unused_fd_flags, task_fd_install, task_close_fd, mmap, deferred) |

### Premier essai sur l'unite (26/10/2026)
Noyau `r77` : demarre (methode recovery, puis `boot` avec et sans menu). Pilotes de connectivite : **oops** des que
`wmt_drv` est charge (zip TWRP : boucle de demarrage ; a la main : panique), dans
`mtk_wcn_consys_detect_adie_chipid` appele par `wmt_loader` (`COMBO_IOCTL_DO_MODULE_INIT`) : le `wmt_loader` de
l'AC8257 n'envoie pas `COMBO_IOCTL_CONNSYS_SOC_HW_INIT`, le pilote de plateforme consys n'etait pas enregistre et
un verrou non initialise etait pris. Corrige (`README.librehu`) : enregistrement dans `do_common_drv_init`, arret
propre si la sonde echoue. Le module KernelSU ne s'etait pas active (reste dans `modules_update`) : a revoir.

Comparaison avec les modules stock (symboles et desassemblage des `.ko` de `/vendor/lib/modules`, `wmt_loader`,
`wmt_launcher`, scripts init du vendor ; fichiers non publies) :
- `wmt_loader` stock : `SET_CHIP_ID` (0x8257), `MODULE_CLEANUP`, `DO_MODULE_INIT`, puis
  `vendor.connsys.driver.ready=yes` ; les scripts init chargent alors `wmt_chrdev_wifi`, `wlan_drv_gen4m`,
  `bt_drv`, `gps_drv`, `fmradio_drv`. Commandes `wmt_launcher` (ioctl 5, 7, 12 a 15, 20, 22, 29 a 31 ;
  `open_stp`, `srh_patch`, `srh_rom_patch`), noeuds `/dev/stpbt`, `/dev/stpgps`, `/dev/wmtWifi`, `/dev/fm`,
  commandes Wi-Fi (`0`, `1`, `S`, `P`, `A`) et dependances des modules : identiques ;
- plateforme consys (reset, alimentation, AFE, EMI, broches, decalages EMI `0xf0068000`...) : celle de `mt6761.c` ;
  differences : les registres PMIC `MT6357_LDO_VCN18_OP_EN` / `VCN33_OP_EN` ecrits par le stock (ajoutes) et la
  region MPU EMI du GPS, que le `gps_drv.ko` stock ne programme pas (retiree pour l'AC8257) ;
- `bt_drv`, `wmt_chrdev_wifi`, `fmradio_drv` et `wlan_drv_gen4m` n'avaient **pas de `module_init`** (charges, ils ne
  faisaient rien) et `fmradio_drv` pas de licence : corrige ;
- module KernelSU : `wmt_drv` en post-fs-data, les autres dans `service.sh` une fois `vendor.connsys.driver.ready=yes`
  (meme ordre que le vendor ; le zip TWRP, qui remplace les fichiers, suit deja cet ordre).
Tant que les pilotes ne sont pas valides a la main (`insmod` un par un, `dmesg -w`), ne pas installer le zip TWRP
`conn-modules-stable` ni le module KernelSU.

### Deuxieme essai (26/10/2026, modules charges a la main, noyau r77)
`insmod` des six modules dans l'ordre du vendor : aucun plantage. Consys allume (version `0x10020300`, HW/FW
`0x8a00`), ROM patch et patch MCU telecharges, calibration faite. **Wi-Fi** : firmware `WIFI_RAM_CODE_soc1_0_1_1.bin`
charge, `wlan0`, scan et connexion WPA2 OK. **Bluetooth** : allume, scan OK. **Radio FM** : OK (appli LibreHU FM).
**Partage de connexion** : ne demarrait pas, interface nommee `swlan0` (nom Samsung) au lieu de `ap0` : corrige.
GPS : pas encore teste (antenne non branchee). Adresse MAC Wi-Fi : generee (`Using dynamically generated MAC
address`), la NVRAM ayant ete envoyee par `wlan_assistant` avant le chargement manuel du pilote : a revoir au
demarrage normal.

### Troisieme essai (26/10/2026, modules `b6dfc1aa88` charges a la main)
**Wi-Fi, partage de connexion (`ap0`, 5 GHz canal 36), Bluetooth et radio FM : OK.** GPS : `mnld` allume la puce
GPS (`g:2`), position a verifier avec l'antenne. Reste : adresse MAC Wi-Fi generee a chaque demarrage
(`00:08:22:...`, la MAC de la NVRAM est vide) ; installation durable (zip TWRP ou module KernelSU).

### Installation durable (26/10/2026)
`boot_ac8257_bootmenu.img` de `r81` + zip TWRP `conn-modules-stable` `r81-b6dfc1aa8` : demarre, pilotes charges par
les scripts du vendor (meme ordre qu'a la main). Lot 1 valide pour la connectivite ; lot 2 lance.

### A tester sur l'unite
Methode sans risque (la partition boot n'est pas touchee) : `recovery_test_ac8257.img` de la prerelease
`latest-stable-4.9.150`, `fastboot flash recovery recovery_test_ac8257.img` puis `fastboot oem reboot-recovery`
([BUILD.md](BUILD.md)). Un redemarrage normal revient au noyau du boot.

- installer `librehu_connectivity_ksu.zip` de la meme prerelease dans KernelSU Next, puis redemarrer (avec la methode
  recovery : redemarrer par `fastboot oem reboot-recovery` pour rester sur ce noyau) ; ou, noyau flashe dans `boot`,
  le zip TWRP `conn-modules-stable` (voir plus haut) ;
- demarrage, `uname -r` = `4.9.150+` ;
- `cat /data/adb/modules/librehu_connectivity/last.log` : six `loaded` ; `lsmod` ; `dmesg | grep -iE "wmt|wlan|disagrees"` ;
- ecran, tactile, son, Wi-Fi, Bluetooth (appairage, A2DP, appel), GPS, radio FM ;
- MCU / CAN (touches volant, marche arriere), camera de recul ;
- USB : cle, Ethernet RTL8152, lecteur CD / DVD (LibreHU Disc), adb ;
- mise en veille / reprise si possible.

Resultat a reporter ici et dans [STATUS.md](STATUS.md).

## Lot 2 : `stable-4.9.200` (4.9.151 -> 4.9.200)

Branche empilee sur `stable-4.9.150` (correctifs de connectivite compris). 3043 commits linux-stable :
**2975 appliques tels quels**, 21 avec un conflit resolu a la main, 4 avec un conflit dont les lignes amont etaient
deja la, 2 en partie (conflit dans un fichier non compile), 18 deja presents (9 identiques, 9 sous une forme
Android / MediaTek), 23 ignores (conflit seulement dans du code non compile ou fichier absent du noyau Jancar).
Detail complet : [stable/lot2-4.9.151-200.tsv](stable/lot2-4.9.151-200.tsv).

Securite : la faille binder **CVE-2019-2215** (« Bad Binder ») etait deja corrigee dans le binder MediaTek
(`BINDER_LOOPER_STATE_POLL`, `POLLFREE`, `synchronize_rcu`) ; course coredump / `mmget_not_zero()`
(**CVE-2019-11599**) appliquee, partie binder reportee dans `binder_alloc.c` ; nombreux correctifs reseau (ipv6,
tunnels, netfilter), ext4, f2fs, USB, `mitigations=`.

### Conflits resolus a la main (choix)
- `Makefile` : bloc clang deplace apres le Makefile d'architecture (amont) avec nos options Android, `CLANG_FLAGS`
  (amont) avec le triplet Android `CLANG_TRIPLE`, `GCC_TOOLCHAIN_DIR` depuis `elfedit` (present dans la chaine AOSP) ;
- `arch/arm64/kernel/traps.c` `die()` : logique MediaTek gardee (trylock, `die_owner`, AEE), principe amont reporte
  (etat des IRQ sauvegarde / restaure, deverrouillage apres les `panic()`, attente avec IRQ actives) ;
- `arch/arm64/mm/proc.S` : `osdlr_el1` sauvegarde dans l'emplacement libre, `tpidr_el1` / `sp_el0` Android gardes,
  meme disposition dans `cpu_do_suspend` et `cpu_do_resume` ;
- `drivers/tty/serial/8250/8250_port.c` : annulation amont de « Don't service RX FIFO if interrupts are disabled »
  prise (bloc AutoChips garde). **UART de la MCU : a verifier en priorite** (touches volant, boitier CAN) ;
- `f_fs.c` (gadget USB, adb) : forme MediaTek + attente de la requete annulee ; `fault.c`, `vdso.c`, `sd.c`, `ext4`,
  `fscrypt`, `bpf`, `ip6_tunnel`, `fib_rules` (Android UID gardes) : voir le journal.

### Corrections apres application
`sd.h` : `logical_to_bytes()` ; `f_fs.c` : la completion MediaTek est dans `ffs_data` ; KernelSU : linux-stable a
maintenant son `<linux/overflow.h>` (sans `struct_size()`), repli dans `compat/k49/ksu_k49.h`.

### A tester sur l'unite
Meme methode que le lot 1 (`recovery_test`, puis `boot` + zip des modules de connectivite du meme run), puis :
demarrage ; MCU / CAN et touches au volant (UART 8250) ; mise en veille et reprise (`proc.S`) ; Wi-Fi, partage de
connexion, Bluetooth, FM, GPS ; USB (adb, cle, Ethernet, lecteur CD/DVD) ; camera de recul ; son.

## Lot 3 : `stable-4.9.225` (4.9.201 -> 4.9.225)

Branche empilee sur `stable-4.9.200` (avec la mise a jour KernelSU Next v3.4.2-legacy). 2552 commits linux-stable :
**2529 appliques tels quels**, 12 avec un conflit resolu a la main, 1 avec un conflit dont les lignes amont etaient
deja la, 2 deja presents (1 identique, 1 sous une forme Android), 8 ignores (conflit seulement dans du code non
compile). Detail complet : [stable/lot3-4.9.201-225.tsv](stable/lot3-4.9.201-225.tsv).

Securite (parties compilees chez nous) : ext4 (**CVE-2019-19037**), selection de la console virtuelle
(**CVE-2020-8648**), course `pivot_root` / montages (**CVE-2020-12114**) ; nombreux correctifs USB gadget (adb,
CarPlay en NCM), reseau, f2fs, ALSA, hrtimer, thermal.

### Conflits resolus a la main (choix)
- `drivers/usb/gadget/configfs.c` (gadget USB : adb, MTP, CarPlay) : verrou `gi->spinlock` et drapeau `unbind`
  ajoutes ; avec `CONFIG_USB_CONFIGFS_UEVENT` (notre cas) `android_setup` / `android_disconnect` restent les
  fonctions du gadget, comme dans android-4.9 ; suspend / resume passent par les versions protegees ;
- `drivers/usb/gadget/function/f_ncm.c` (CarPlay) : compteur `notify_count` amont, tests NULL et journaux Jancar
  gardes ;
- `drivers/thermal/thermal_core.c` : version amont (`cancel_delayed_work_sync` a la suppression d'une zone ;
  MediaTek avait mis cet arret en commentaire) ;
- `kernel/time/hrtimer.c` : etat `PINNED` MediaTek garde, ecritures de `timer->state` en `WRITE_ONCE` ;
- `sound/core/rawmidi.c` : `realloc_mutex` Android garde, compteur de reference du tampon amont ;
- `kernel/cpu.c`, `printk.c`, `spidev.c`, `xhci-hub.c`, `mmu.c`, `dm.c`, `ext4.h` : voir le journal.

### Corrections apres application
`configfs.c` : `configfs_composite_setup` / `_disconnect` seulement sans `CONFIG_USB_CONFIGFS_UEVENT` (fonctions
inutilisees sinon, erreur en `-Werror`). `tools/stable/apply.py` cherchait les commits deja appliques dans les 4000
derniers seulement : toute la branche maintenant.

### A tester sur l'unite
Comme le lot 2, en insistant sur l'USB (adb, CarPlay, cle, Ethernet), la mise en veille, la temperature (zones
thermiques) et le son.

## Lot 4 : `stable-4.9.250` (4.9.226 -> 4.9.250)

Branche empilee sur `stable-4.9.225`. 1956 commits linux-stable : **1948 appliques tels quels**, 2 avec un conflit
resolu a la main, 1 en partie (conflit dans un fichier non compile), 3 deja presents (1 identique, 2 sous une forme
Android), 2 ignores (conflit seulement dans du code non compile). Detail complet :
[stable/lot4-4.9.226-250.tsv](stable/lot4-4.9.226-250.tsv).

Securite (parties compilees chez nous) : `af_packet` (**CVE-2020-14386**, ecriture hors limites depuis un socket
paquet), `mremap` (**CVE-2020-10757**), clavier de la console virtuelle (**CVE-2020-25656**) ; binder : le
gestionnaire de contexte ne peut plus prendre de reference sur lui-meme.

### Conflits resolus a la main (choix)
- `drivers/android/binder.c` : le correctif amont vise l'ancien binder ; reporte sur le binder MediaTek (nouveau
  binder) comme sa version pour 4.14 : `BC_INCREFS` / `BC_ACQUIRE` sur la reference 0 refuse au gestionnaire de
  contexte lui-meme ; le refus d'une transaction vers son propre gestionnaire existait deja ;
- `drivers/tty/serial/8250/8250_port.c` (**UART de la MCU**) : bloc AutoChips garde, `skip_rx` amont ajoute. Il
  n'agit qu'avec le controle de flux materiel automatique (`UPSTAT_AUTOCTS` / `UPSTAT_AUTORTS`) ;
- serie fscrypt de 4.9.2xx (`fscrypto: move ioctl processing...`, `use EEXIST...`) : deja dans le fscrypt plus
  recent du noyau Android.

### Corrections apres application
`mm/memblock.c` (code MediaTek de mise en ligne de memoire) : `build_all_zonelists()` prend maintenant un troisieme
argument (`mm: Avoid calling build_all_zonelists_init under hotplug context`), `true` comme les autres appels a
chaud.

### A tester sur l'unite
Comme le lot 3, en insistant sur la MCU (touches au volant, CAN : UART 8250) et le Wi-Fi / partage de connexion
(`af_packet`).

## Lot 5 : `stable-4.9.300` (4.9.251 -> 4.9.300)

Branche empilee sur `stable-4.9.250`. 2693 commits linux-stable : **2659 appliques tels quels**, 15 avec un conflit
resolu a la main, 3 avec un conflit dont les lignes amont etaient deja la, 1 en partie, 4 deja presents, 11 ignores
(9 conflits seulement dans du code non compile, 2 correctifs d'un pilote absent). Detail complet :
[stable/lot5-4.9.251-300.tsv](stable/lot5-4.9.251-300.tsv).

Securite (parties compilees chez nous) : ramasse-miettes `af_unix` contre `MSG_PEEK` (**CVE-2021-0920**, exploitee
sur Android) ; binder : controles SELinux et euid par les identifiants du processus (**CVE-2021-39686**), reportes a
la main sur le binder MediaTek ; boucle `LDXR/STXR` bornee dans les futex arm64 ; `fget` contre la fermeture
concurrente ; compteur de references utilisateur des handles ION ; `wake_up_pollfree()` (binder, signalfd) ; USB
gadget (ep0) ; Bluetooth.

### Conflits resolus a la main (choix)
- `drivers/android/binder.c` (binder MediaTek, nouveau binder) : `proc->cred` pris a l'ouverture et rendu dans
  `binder_free_proc()`, `security_binder_transaction` / `transfer_binder` / `transfer_file` avec les `cred` (comme
  la version amont 4.14+) ; `wake_up_pollfree()` sous le verrou interne ;
- `arch/arm64/include/asm/futex.h` : boucle bornee (`loops`, `-EAGAIN`) avec `uaccess_enable()` / `uaccess_disable()`
  de l'ACK a la place des `ALTERNATIVE` PAN ;
- ION (`ion-ioctl.c`, `ion.c`, `ion_system_heap.c`) : compteur utilisateur des handles et ordre des references
  amont, journaux MediaTek et `ion_handle_get_by_id()` (pour `ion_drv_get_handle()`) gardes ; les ioctl MediaTek ne
  creent pas de handle ;
- `drivers/mmc/core/mmc.c` (**eMMC**) : delai de changement de partition x4 de MediaTek garde, minimum et repli sur
  `GENERIC_CMD6_TIME` amont ;
- `fs/ext4/ioctl.c` : argument `flags` (discard securise, Android) garde ; `fs/crypto/fname.c` : `hash` et
  `minor_hash` toujours copies dans la forme Android ; `include/net/sock.h` : `sk_uid` (Android) et `sk_peer_lock` ;
  `vmlinux.lds.h`, `thread_info.h`, `composite.c`, `configfs.c`, `usbstring.c` : voir le journal.

### Corrections apres application
- `drivers/spi/spi-ac82xx.c` : deux correctifs de `spi-mt65xx.c` (absent ici) avaient ete poses sur le pilote SPI
  AutoChips par la detection de renommage de `git am -3` ; « Fix fifo transfer » cassait la compilation : annule ;
  « Avoid NULL pointer crash in interrupt » garde (sans effet de bord ici). `tools/stable/apply.py` applique
  maintenant sans detection de renommage et s'arrete si un commit touche d'autres fichiers que l'original ;
  `tools/stable/renameaudit.py` verifie une branche (lots 1 a 5 : seul ce cas) ;
- `fs/fs-writeback.c` : appel MediaTek a `block_dump___mark_inode_dirty()` retire avec la fonction ;
- `drivers/block/zram/zram_drv.c` : `pages_compacted` est un `atomic_long_t` (fichier proc MediaTek).

### A tester sur l'unite
Comme le lot 4, en insistant sur tout ce qui passe par binder et ION (demarrage complet, camera de recul, video,
applications), le stockage eMMC (changement de partition : demarrage, OTA) et le Bluetooth.

## Lot 6 : `stable-4.9.319` (4.9.301 -> 4.9.319)

Branche empilee sur `stable-4.9.300`. 843 commits linux-stable : **829 appliques tels quels**, 7 avec un conflit
resolu a la main, 1 avec un conflit dont les lignes amont etaient deja la, 1 deja present, 1 non repris (forme
MediaTek gardee), 4 ignores (conflit seulement dans du code non compile). Detail complet :
[stable/lot6-4.9.301-319.tsv](stable/lot6-4.9.301-319.tsv).

Contenu principal : la serie arm64 **Spectre-BHB** (CVE-2022-23960 ; le Cortex-A53 de l'AC8257 n'est pas dans la
liste des coeurs touches, la serie change surtout l'infrastructure des capacites CPU et des vecteurs d'exception),
`exec` avec `argv` vide (noyau, **CVE-2021-4034**), USB gadget `rndis` (verrou de la file de reponses : partage de
connexion USB), dm-verity marque `DM_TARGET_IMMUTABLE` (verity reste actif, rien n'est desactive).

### Conflits resolus a la main (choix)
- `arch/arm64/kernel/entry.S`, `cpufeature.c` : vecteurs et trampoline amont, `__nocfi` (CFI de l'ACK) garde ;
- `drivers/usb/gadget/function/rndis.c` : `resp_lock` amont pose sur les parcours MediaTek de la file ;
- `drivers/md/dm-verity-target.c` : `DM_TARGET_IMMUTABLE` ajoute, version 1.4.0 (Android) gardee ;
- `include/linux/bpf.h` : fonctions Android et `unprivileged_ebpf_enabled()` gardees ;
- `pinctrl-mtk-common.c` (fuite d'une reference `of_node` au demarrage) : version MediaTek gardee.

### Corrections apres application
- `arch/arm64/kernel/cpu_errata.c` : l'entree ACK de l'erratum 855873 (Cortex-A53 r0p0 a r0p4, donc l'AC8257)
  passe a la nouvelle macro `ERRATA_MIDR_REV_RANGE`, meme plage ;
- `drivers/clocksource/Kconfig` : `tools/stable/apply.py` comptait les `Kconfig` / `Makefile` hors du code compile
  et avait laisse de cote le symbole `ARM_ARCH_TIMER_OOL_WORKAROUND` (le code du timer le reference) : ajoute comme en
  amont. `apply.py` traite maintenant les `Kconfig`, `Makefile` et `Kbuild` des repertoires compiles, de `arch/arm64`
  et de `scripts` comme du code compile. Releve des lots precedents : seuls des fichiers x86 / EFI / KASAN et trois
  remaniements de `scripts/` (lot 1, la compilation n'en depend pas) avaient ete laisses ainsi.

### A tester sur l'unite
Comme le lot 5, en insistant sur la mise en veille / reprise (vecteurs d'exception), le partage de connexion USB
(`rndis`) et le demarrage avec dm-verity actif.
