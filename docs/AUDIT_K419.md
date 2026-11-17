# Audit du noyau 4.19.325 AC8257 : trouvailles à appliquer

Document de passation pour une autre instance de Claude (ou un humain) chargée d'APPLIQUER les correctifs.
L'audit lui-même n'a rien modifié dans le code.

- Dépôt : `LibreHU/android_kernel_autochips_ac8257_4.9`, branche `experimental-4.19.325`
- Base auditée : commit `634610b` (« ci: a9 (Android 9) and a10 (Android 10+) builds of the kernel »)
- Matériel : Jancar UJC201, AutoChips AC8257 (≈ MediaTek MT6765), 4 × Cortex-A53, 6 Gio, PMIC MT6357, GPU PowerVR GE8320
- Méthode : 8 agents en lecture seule (cœur, mémoire/IOMMU, affichage/ARM2, connectivité, configuration/perf,
  revue des commits récents, entrées-sorties, compatibilité Android). Chaque point ci-dessous a été revérifié dans
  l'arbre au commit `634610b` (fichiers et lignes cités).
- Journaux de l'unité utilisés : démarrages r174 (Enforcing) et r175 (Permissive) du noyau 4.19, et journaux du 4.9.
  Ils restent privés (non publiés) : ne pas chercher à les télécharger.
- **L'audit est incomplet** : les agents ont été arrêtés par la limite d'usage. Les zones non couvertes sont listées
  à la fin (section « Reste à auditer »).

## État d'application (16/11/2026, à la demande de l'utilisateur : « Applique tout les patchs »)

Appliqué dans la branche `experimental-4.19.325`, un commit par point (messages en anglais). Vérifié par un build
complet local de la variante a9 (clang 14) : compilation sans erreur, `Image.gz-dtb` 9 643 864 octets (9 951 937
avant : -308 Ko malgré le JIT et les jump labels), image boot de 10 Mio qui tient, plus aucun format `trace_printk`
(`__start___trace_bprintk_fmt` = `__stop`), aucun `.ko` avec `__trace_printk_fmt`. La variante a10 est construite
par la CI. À tester sur l'unité (recovery_test d'abord). Ce qui n'est pas appliqué est expliqué.

| Point | État | Commit |
|---|---|---|
| B1 TLB M4U | appliqué : sync après chaque page démappée (comportement 4.9) dans `arm_v7s_unmap()` | `86d2ead` |
| B2, B3 xt_qtaguid | appliqué : les 2 hunks ACK `android-4.9-p` | `ce91282` |
| B4 ION < 4 Gio | appliqué : `ion_gfp_below_4g()` (GFP_DMA32, sans MOVABLE/HIGHMEM), pools et tas contig | `910fb3f` |
| B5 dualarm | appliqué : `spin_lock_irqsave` + attente bornée à 50 ms | `cb473a6` |
| C1 cap_capable | appliqué : une ligne par capacité et par démarrage | `1756712` |
| C2 configs | appliqué : `XT_TARGET_CT` (les deux) ; a10 : L2TP, PPPOL2TP, PPTP, NET_IPGRE_DEMUX, NET_CLS_BPF, XFRM_INTERFACE, XFRM_STATISTICS ; a9 : pppolac/pppopns portés (voir plus bas) | `7bf7d4d` |
| P5 taille | appliqué : DYNAMIC_DEBUG, CGROUP_DEBUG, LOGO, IOSCHED_CFQ à n | `e128f26` |
| P2 BPF JIT | appliqué : `BPF_JIT` + `BPF_JIT_ALWAYS_ON` | `f0c9c88` |
| P3 jump labels | appliqué | `e73645a` |
| P6 Goodix | appliqué : `TOUCHSCREEN_GOODIX=n` | `d4e531c` |
| P1 console UART | appliqué : `mt_disable_uart()` à `late_initcall_sync` (`printk.keep_uart=1` pour la garder) | `02dcb72` |
| P4 KPTI | appliqué : `kpti=0` ajouté par la CI à toutes les images | `93dbc9f` |
| P8 trace_printk | appliqué : `LIBREHU_NO_TRACE_PRINTK` pour `drivers/misc/mediatek` | `04d9d7c` |
| B6 backcar | appliqué : file statique + `wait_event_interruptible()` sur `MSG_ARM2_RESPONSE` | `b5e5e02` |
| B7, B8 mémoire réservée | appliqué : démappage M4U avant `free_memblock_runtime()` (ARM2/MDP/ISP et queue du FB), tas ION FB limité à la partie gardée. Correction de l'audit : le 4.9 avait aussi `MEMBLK_RELEASE_POLICY=y` (`mm/memblock.c`), ce n'est pas une régression ; la politique reste active | `0662a97` |
| B9 LCM | appliqué en partie : pas d'I2C sans client, détection DS90UB947 mise en cache, plus de double `i2c_unregister_device()`. L'indice `lcm_id` n'est PAS remappé (risque : envoyer les tables du pont primaire au composant 0x2d) | `159bb6e` |
| B10 MAC Wi-Fi | appliqué : MAC stable dérivée (jhash) de `androidboot.serialno`, `02:08:22:xx:xx:xx` | `a0a111e` |
| B12 Wi-Fi | appliqué : `MAC2STR` + garde `KERNEL_VERSION(5, 9, 0) >` | `a0a111e` |
| B11 PSCI | appliqué : index borné, power_state 0 au-delà (même comportement qu'avant, sans lecture hors tableau) | `bba1fed` |
| B13 perf_event_paranoid | appliqué : patch du 4.9 (`SECURITY_PERF_EVENTS_RESTRICT`) | `aed4b4a` |
| pppolac / pppopns (a9) | appliqué : pilotes Android du 4.9 portés, `CONFIG_PPPOLAC` / `CONFIG_PPPOPNS` en a9 | `f3e4450` |
| P7 toprgu | NON appliqué : le DT stock 4.9, le DT de l'UJC201 et la référence MediaTek MT6765 déclarent tous `IRQ_TYPE_NONE` ; changer la polarité sans la connaître peut déclencher l'IRQ du watchdog en boucle. Le WARN reste (sans effet) | - |
| P9 IPI 0xf | analysé : aucun émetteur du SGI 15 dans le noyau (recherche de `smp_cross_call`, `gic_send_sgi`, SGI dans `drivers/misc/mediatek`, `autochips`, `irqchip`) : il vient du monde sécurisé (ATF) ; le noyau l'acquitte. Pas de correctif | - |
| H1 VINTF Android 9 | appliqué le 17/11 autrement : le zip TWRP a9 monte depuis /vendor des copies des matrices avec une entrée noyau 4.19 (bind mount, /system jamais modifié) ; marche aussi pour la ROM stock | voir docs/K419.md |
| H2 sepolicy `map` | NON appliqué : `camhal3lite` et `vtservice_hidl` sont dans la politique vendor stock (pas dans l'arbre) ; `librehu_ksunext` est déjà un domaine permissif, ses règles viendront avec le passage en Enforcing | - |
| H3 Wi-Fi 5 GHz | test côté utilisateur (pas un correctif) | - |
| H4 heure MCU | NON appliqué : le dépôt `LibreHU-service` n'a pas pu être attaché à la session | - |
| H5 uartconsole | rendu inutile par P1 (le noyau coupe la console UART lui-même) | - |

## Règles pour l'instance qui applique

1. Travailler sur `experimental-4.19.325` uniquement. Ne pas toucher `stable-4.9.337` ni les autres branches.
2. Un commit par correctif, message en anglais, sujet `sous-système: description`. Ne jamais réécrire l'historique publié.
3. Ne pas recompiler tout le noyau en local pour de petites modifications : pousser, la CI construit les deux variantes
   (`a9` et `a10`). Compiler en local seulement pour vérifier un comportement (ex. une option Kconfig ou la taille).
4. Contrainte de taille : la partition boot fait 10 Mio et `Image.gz-dtb` pèse déjà ~9,95 Mo. Toute option qui grossit le
   noyau (BPF_JIT, JUMP_LABEL…) doit venir APRÈS un gain de place (DYNAMIC_DEBUG=n, point P5). `repack_boot.py` échoue
   si l'image dépasse : vérifier la sortie de la CI.
5. Ne jamais désactiver dm-verity ni écrire `vbmeta`. Ne jamais publier d'adresse MAC Wi-Fi/BT, de numéro de série
   ni de certificat (puce Apple MFi). Pas de GSI.
6. Chaque correctif doit être testé sur l'unité par l'utilisateur : d'abord l'image `recovery_test_ac8257_<v>.img`
   (`fastboot flash recovery` puis `fastboot oem reboot-recovery`), ensuite seulement la partition boot.
7. Demander l'accord de l'utilisateur avant tout changement risqué (lot 3) ou touchant au comportement de l'ARM2.
8. Mettre à jour `docs/K419.md` (section du correctif) à chaque commit.

## Ordre conseillé

Du plus simple et plus sûr au plus complexe.

| Lot | Points | Pourquoi |
|---|---|---|
| 1 | B1, B2, B3, B4, B5, C1, C2 | Bugs prouvés, correctifs courts et locaux |
| 2 | P5, P1, P2, P3, P4, P6, P7, P8 | Performance ; P5 d'abord (libère la place) ; P8 à vérifier d'abord sur l'unité |
| 3 | B6, B7, B8, B9, B10, B11, B12, B13, P9 | Demandent un test ciblé sur l'unité, une analyse ou un choix |
| Hors noyau | H1 à H5 | ROM, service LibreHU, réglage du routeur |

---

## Lot 1 : bugs prouvés, correctifs simples

### B1 [CRITIQUE] M4U : aucune synchronisation du TLB IOMMU après un unmap

- Où : `drivers/iommu/mtk_iommu_v2.c:830` (`static struct iommu_ops mtk_iommu_ops`) : pas de `.iotlb_sync`,
  `.flush_iotlb_all` ni `.iotlb_range_add`. `drivers/iommu/io-pgtable-arm-v7s.c:689` (`arm_v7s_unmap`) : aucune
  `io_pgtable_tlb_sync()`. `drivers/iommu/iommu.c` (`__iommu_unmap` : `if (sync && ops->iotlb_sync) ops->iotlb_sync(domain);`).
- Problème : depuis Linux 4.14, `arm_v7s_unmap()` ne synchronise plus le TLB ; c'est le pilote IOMMU qui doit fournir
  `.iotlb_sync`. En 4.9, `arm_v7s_unmap()` finissait par `if (unmapped) io_pgtable_tlb_sync(&data->iop);`. Ici,
  `__arm_v7s_unmap()` lance seulement l'invalidation (`tlb_add_flush`) et personne n'attend `REG_MMU_CPE_DONE` ni ne le
  remet à 0. Conséquences : l'appelant recycle pages et IOVA avant la fin de l'invalidation ; le `tlb_sync` du map
  suivant sort tout de suite (CPE_DONE resté à 1) ; des centaines de couples flush sans sync s'enchaînent, alors que le
  matériel exige des paires flush/sync strictes.
- Effet probable : un maître multimédia (VDEC, VENC, MDP, DISP, ISP) traduit une IOVA libérée vers l'ANCIENNE page
  physique, déjà rendue au pool ION ou au buddy : écriture DMA dans la mémoire d'un autre processus ou du noyau.
  Compatible avec les SIGSEGV en rafale de `media.codec` (pc dans `/dmabuf:`) vus en 4.19 et absents en 4.9.
- Références : `git show stable-4.9.337:drivers/iommu/io-pgtable-arm-v7s.c` (sync dans unmap) ; pilote MediaTek 4.19
  de la référence `sam/lineage-21` (`drivers/iommu/mtk_iommu_v2.c`, `.iotlb_sync = mtk_iommu_iotlb_sync`) ;
  upstream 4.19 `drivers/iommu/mtk_iommu.c` (même principe).
- Correctif proposé (comme la référence MediaTek et l'upstream 4.19) :

```c
static void mtk_iommu_iotlb_sync(struct iommu_domain *domain)
{
	unsigned long flags;
	struct mtk_iommu_domain *dom = to_mtk_domain(domain);

	spin_lock_irqsave(&dom->pgtlock, flags);	/* paires flush/sync strictes */
	mtk_iommu_tlb_sync(NULL);
	spin_unlock_irqrestore(&dom->pgtlock, flags);
}

static void mtk_iommu_flush_iotlb_all(struct iommu_domain *domain)
{
	mtk_iommu_tlb_flush_all(NULL);
	mtk_iommu_iotlb_sync(domain);
}

static struct iommu_ops mtk_iommu_ops = {
	...
	.flush_iotlb_all = mtk_iommu_flush_iotlb_all,
	.iotlb_sync	= mtk_iommu_iotlb_sync,
};
```

  À vérifier avant d'écrire : la signature réelle de `mtk_iommu_tlb_sync()` / `mtk_iommu_tlb_flush_all()` dans ce
  fichier (argument cookie), le nom du verrou du domaine, et que `pseudo_m4u_v2.c` passe bien par `iommu_unmap()`
  (sinon il n'appellera jamais `.iotlb_sync`). Alternative minimale : remettre `if (unmapped) io_pgtable_tlb_sync(&data->iop);`
  à la fin de `arm_v7s_unmap()` (comportement 4.9 ; `mtk_iommu_unmap()` tient déjà `pgtlock`). Optimisation possible
  ensuite : un seul flush de plage `[iova, iova+size)` puis une sync, au lieu d'un flush par page.
- Test sur l'unité : lecture vidéo longue (VP9/H.264), scrcpy (encodeur), caméra arrière ; plus de SIGSEGV `media.codec`,
  pas de `Partial TLB flush timed out` dans dmesg.
- Confiance : absence de sync prouvée (code) ; lien avec les plantages : probable.

### B2 [HAUT] xt_qtaguid : use-after-free dans `qtaguid_untag()` (variante a9)

- Où : `net/netfilter/xt_qtaguid.c:2421-2445` (commit `8850e19`).
- Problème : le port vient de `stable-4.9.337`, qui n'a pas le correctif ACK de `android-4.9-p`. Si un socket est
  détaché par un processus qui n'a pas ouvert `/dev/xt_qtaguid` (fd passé par SCM_RIGHTS, autre tgid), `pqd_entry` est
  NULL : pas de `list_del()`, mais `kfree(sock_tag_entry)` quand même. La liste du processus d'origine garde un pointeur
  libéré, parcouru ensuite par `qtudev_release()`. Atteignable par une application (`/proc/net/xt_qtaguid/ctrl` en 0666,
  SELinux permissif sur les images boot).
- Correctif proposé (hunk ACK `android-4.9-p`) :

```diff
-	if (IS_ERR_OR_NULL(pqd_entry) || !sock_tag_entry->list.next) {
-		pr_warn_once(...);
-	} else {
-		list_del(&sock_tag_entry->list);
-	}
+	if (IS_ERR_OR_NULL(pqd_entry))
+		pr_warn_once(...);
+	if (sock_tag_entry->list.next)
+		list_del(&sock_tag_entry->list);
```

  Le plus simple et le plus sûr : prendre `net/netfilter/xt_qtaguid.c` de la branche ACK `android-4.9-p` et réappliquer
  les changements d'API 4.19 du commit `8850e19` (`xt_family/xt_in/xt_out/xt_hooknum`, `nf_sk_lookup_slow_v4/v6`,
  `refcount_read`). L'écart avec ACK se limite à ce hunk et à celui de B3.
- Confiance : prouvé (lecture et diff avec ACK).

### B3 [HAUT] xt_qtaguid : use-after-free de `sock_tag_entry` sur le chemin paquet (variante a9)

- Où : `net/netfilter/xt_qtaguid.c:1069` (`get_sock_stat`) et `:1312` (`if_tag_stat_update`).
- Problème : `get_sock_stat()` relâche `sock_tag_list_lock` avant que l'appelant lise `sock_tag_entry->tag`. Un untag ou
  une fermeture concurrente libère l'entrée entre les deux : lecture après libération en softirq.
- Correctif proposé (ACK `android-4.9-p`) : supprimer `get_sock_stat()` et lire sous le verrou :

```c
spin_lock_bh(&sock_tag_list_lock);
sock_tag_entry = sk ? get_sock_stat_nl(sk) : NULL;
if (sock_tag_entry)
	tag = sock_tag_entry->tag;
spin_unlock_bh(&sock_tag_list_lock);
```

- Confiance : prouvé (lecture et diff avec ACK). Traiter B2 et B3 dans le même commit.

### B4 [HAUT] ION : la limite « < 4 Gio » des pools est du code mort en 4.19

- Où : `drivers/staging/android/ion/ion_page_pool.c:33-37` ; `drivers/staging/android/ion/mtk/ion_mm_heap.c:1195`.
- Problème : la garde `#if defined(CONFIG_64BIT) && defined(CONFIG_ZONE_DMA)` est toujours fausse : arm64 4.19 n'a que
  `CONFIG_ZONE_DMA32=y` (vérifié dans la config). `GFP_DMA` n'est donc jamais ajouté, alors que `include/linux/gfp.h`
  a été adapté pour que GFP_DMA serve sous 4 Gio (OPT_ZONE_DMA = ZONE_DMA32 sous CONFIG_MEDIATEK_SOLUTION). Le tas
  caméra (`ION_HEAP_TYPE_MULTIMEDIA_FOR_CAMERA`) alloue avec `__GFP_HIGHMEM | __GFP_MOVABLE` : pages en ZONE_MOVABLE,
  jusqu'à 0x1c0000000, donc au-dessus de 4 Gio. Les autres tas ne sont protégés que par la disposition des zones. Le 4.9
  avait corrigé justement une corruption « venant de la mémoire au-dessus de 4 Gio ».
- Effet probable : un buffer caméra importé en adresse physique par le GPU ou l'affichage (masque 32 bits) est tronqué :
  écriture dans une autre page. Les pages ION en ZONE_MOVABLE gênent aussi la compaction.
- Correctif proposé :

```diff
-#if defined(CONFIG_64BIT) && defined(CONFIG_ZONE_DMA)
+#if defined(CONFIG_64BIT) && (defined(CONFIG_ZONE_DMA) || defined(CONFIG_ZONE_DMA32))
 	/* 6G DDR support: limit ion alloc pages in 4G DMA ZONE */
-	pool->gfp_mask = pool->gfp_mask | GFP_DMA;
+	pool->gfp_mask = (pool->gfp_mask | GFP_DMA32) & ~(__GFP_MOVABLE | __GFP_HIGHMEM);
 #endif
```

  Mieux : le faire une seule fois dans `ion_page_pool_create()`, et retirer `__GFP_MOVABLE` du tas caméra
  (`ion_mm_heap.c:1195`, et les autres occurrences lignes 1652, 1813, 1816 à examiner).
- Test : caméra arrière et AVM, lecture vidéo ; `cat /proc/pagetypeinfo` avant/après (moins de pages non migrables en Movable).
- Confiance : prouvé (code et config) ; impact réel selon l'usage du tas caméra.

### B5 [HAUT] dualarm : spinlock pris sans couper les IRQ, aussi pris en IRQ dure

- Où : `drivers/misc/autochips/dualarm/dual_hal.c:109, 125, 139, 155, 169` (`spin_lock(&Dualarm_Lock)` dans
  backcar/display send/get message et le tampon de log ARM2). Appelants : ioctl de `backcar.c` (contexte processus) et
  gestionnaires d'IRQ dures `backcar_isr_handler` / `disp_dualarm_isr_handler`.
- Problème : si l'IRQ ARM2 → ARM1 arrive sur le CPU qui tient déjà le verrou (c'est justement ce qui suit l'envoi de
  `MSG_ANDROID_APP_READY`), le CPU tourne pour toujours. `WAIT_INT_CLEAR()` boucle sans limite, verrou pris.
- Correctif proposé : `spin_lock_irqsave()` / `spin_unlock_irqrestore()` dans les 5 fonctions ; borner l'attente
  (`readl_poll_timeout_atomic(..., 1, 10000)`) et sortir en erreur.
- Confiance : prouvé (code) ; blocage non observé sur l'unité. Code identique au 4.9 (bug latent).

### C1 [MOYEN] cap_capable : 3 printk non limités à chaque octroi implicite NET_RAW / NET_ADMIN

- Où : `security/commoncap.c:122-133` (`CONFIG_ANDROID_PARANOID_NETWORK`).
- Problème : chaque appel iptables/ip6tables de netd écrit 3 lignes au niveau par défaut, donc sur la console UART
  (voir P1). r175 : 657 lignes, 54 Ko, entre 10 et 20 s, soit ~0,6 s d'attente active IRQ masquées.
- Correctif proposé : `pr_info_ratelimited()` (ou une seule ligne `pr_info_once` par capacité). L'octroi reste identique.
- Confiance : prouvé.

### C2 [MOYEN] Configuration : options manquantes pour le partage de connexion, les VPN et Android 10

- Où : `arch/arm64/configs/ac8257_ujc201_defconfig` (774 `# CONFIG_NETFILTER_XT_TARGET_CT is not set`,
  918 `# CONFIG_L2TP is not set`), `arch/arm64/configs/ac8257_a10.config`.
- Problèmes :
  - Partage de connexion avec liaison montante : netd (Android 9 `TetherController::setForwardRules`, Android 10 aussi)
    envoie `-j CT --helper ftp` dans le même lot iptables-restore que les règles FORWARD. Sans `xt_CT`, tout le lot
    échoue : clients du hotspot sans Internet. Absent aussi du 4.9 stock (pas une régression).
  - VPN L2TP/IPsec et PPTP des Paramètres : Android 10 utilise `PX_PROTO_OL2TP` / `PX_PROTO_PPTP` (amont), absents.
    Android 9 utilise `PX_PROTO_OLAC` / `PX_PROTO_OPNS` (pilotes Android `pppolac.c` / `pppopns.c` du 4.9, non portés).
  - Exigences `q/android-4.19/android-base.config` non satisfaites en a10 : L2TP, PPPOL2TP, PPTP, NET_CLS_BPF,
    NET_IPGRE_DEMUX, NETFILTER_XT_TARGET_CT, XFRM_INTERFACE, XFRM_STATISTICS (SECURITY_PERF_EVENTS_RESTRICT : voir B13).
    Source : https://android.googlesource.com/kernel/configs/+/refs/heads/android10-release/q/android-4.19/android-base.config
- Correctif proposé :
  - defconfig commun : `CONFIG_NETFILTER_XT_TARGET_CT=y` (les helpers FTP/PPTP sont déjà `=y`).
  - `ac8257_a10.config` : `CONFIG_L2TP=y`, `CONFIG_PPPOL2TP=y`, `CONFIG_PPTP=y`, `CONFIG_NET_CLS_BPF=y`,
    `CONFIG_NET_IPGRE_DEMUX=y`, `CONFIG_XFRM_INTERFACE=y`, `CONFIG_XFRM_STATISTICS=y`.
  - a9 : porter `pppolac.c` / `pppopns.c` du 4.9 (déjà listés « non portés » dans `docs/K419.md`) ou accepter la perte
    (lot 3, demander à l'utilisateur).
  - Vérifier la taille après coup (règle 4).
- Confiance : prouvé (config, code netd et mtpd) ; effet sur le hotspot : probable.

---

## Lot 2 : performance et taille

### P5 [MOYEN] À faire en premier : `CONFIG_DYNAMIC_DEBUG=y` coûte ~7 300 descripteurs pr_debug

- Où : `ac8257_ujc201_defconfig:4405`.
- Mesure : `__start___verbose` → `__stop___verbose` = 293 432 octets (7 336 × 40 octets) en `.data`, plus formats et code.
  Gain estimé 150 à 300 Ko sur `Image.gz` (à mesurer). Aucun effet fonctionnel (messages muets par défaut).
- Correctif : `# CONFIG_DYNAMIC_DEBUG is not set`. Petits gains annexes sans effet en production : `CONFIG_CGROUP_DEBUG=n`,
  `CONFIG_LOGO=n` (pas de FRAMEBUFFER_CONSOLE), `CONFIG_IOSCHED_CFQ=n` (eMMC en blk-mq). Plus tard, quand les paniques
  seront réglées : `CONFIG_DEBUG_BUGVERBOSE=n` (+42 Kio mesurés par `745279d`).
- Confiance : prouvé (taille) ; gain total à mesurer.

### P1 [HAUT] Console UART ttyS0 (921600 bauds) jamais coupée après le démarrage

- Où : `ac8257_ujc201_defconfig:1697` (`# CONFIG_MTPROF is not set`) ; `kernel/printk/printk.c` (`printk_disable_uart`) ;
  `drivers/misc/mediatek/mtprof/bootprof.c` (`bootup_finish()` → `mt_disable_uart()`).
- Problème : le LK passe `console=ttyS0,921600n1 printk.disable_uart=0`. En 4.9 (MTPROF=y), la fin du démarrage écrite
  dans `/proc/bootprof` appelait `mt_disable_uart()`. En 4.19 plus rien ne l'appelle. Le chemin `init.ac8257.rc`
  (`write /proc/mtprintk ${persist.vendor.uartconsole.enable}`) échoue : la propriété n'existe pas. Chaque printk part
  octet par octet sur l'UART, IRQ masquées, ~10,9 µs par octet. r175 : 869 Ko de dmesg en 238 s ≈ 9,4 s de CPU en attente
  active ; 300 ms de rejeu de la console sur le chemin d'initcall.
- Correctif proposé :
  1. Côté noyau : appeler `mt_disable_uart()` en fin de démarrage sans MTPROF, par exemple un `/proc/bootprof` minimal
     qui le fait sur écriture de « 0 », ou un travail différé (~60 s après `late_initcall_sync`) qui met
     `printk_disable_uart = 1` sauf si la ligne de commande demande 2.
  2. Contournement immédiat côté ROM : `setprop persist.vendor.uartconsole.enable 0` (propriété persistante).
  3. Ne PAS ajouter `printk.disable_uart=1` à l'en-tête boot.img : le LK ajoute sa propre valeur après, et la dernière gagne.
- Garder la console UART utilisable en recovery_test pour le débogage (ne pas la couper avant la fin du démarrage).
- Confiance : prouvé (coût mesuré, absence de désactivation).

### P2 [PERF] `CONFIG_BPF_JIT=n` : seccomp et eBPF de netd interprétés

- Où : `ac8257_ujc201_defconfig:1029`.
- Problème : le filtre seccomp de chaque application est interprété à chaque appel système ; le programme cgroup skb de
  netd (accepté depuis `ec8f6b0`) est interprété à chaque paquet.
- Correctif : `CONFIG_BPF_JIT=y` et `CONFIG_BPF_JIT_ALWAYS_ON=y` (sinon `bpf_jit_enable` vaut 0 par défaut). Après P5.
  Vérifier ensuite que bpfloader charge toujours le programme de netd (le vérificateur est commun JIT/interpréteur) et
  que le patch `ec8f6b0` (`cgroup_skb_run_prog`) reste correct avec le JIT.
- Confiance : JIT absent prouvé ; gain probable (×3 à ×10 sur l'exécution du programme).

### P3 [PERF] `CONFIG_JUMP_LABEL=n`

- Où : `ac8257_ujc201_defconfig:223`. Table `__jump_table` vide ; 866 tracepoints testés par lecture mémoire + branche.
- Correctif : `CONFIG_JUMP_LABEL=y`, SEULEMENT avec `CONFIG_DYNAMIC_DEBUG=n` (sinon +~290 Ko). Vérifier au démarrage
  l'absence de `jump_label: Fatal kernel bug`.
- Confiance : désactivé prouvé ; gain faible mais général (0,5 à 2 % sur micro-bancs ordonnanceur, à mesurer).

### P4 [PERF] KPTI actif sur Cortex-A53, seulement parce que KASLR est actif

- Preuve : `CPU features: kernel page table isolation forced ON by KASLR` (r175). L'A53 n'est pas vulnérable à Meltdown
  (`arch/arm64/kernel/cpufeature.c`, liste `kpti_safe_list`).
- Correctif : ajouter `kpti=0` à `BOOT_APPEND` et à la ligne de l'image recovery_test dans `.github/workflows/build.yml`
  (même mécanisme que `androidboot.selinux=permissive`). Le LK ne passe pas `kpti`, donc pas de conflit d'ordre.
  Attendu ensuite dans dmesg : `forced OFF by kpti command line option`.
- Contrepartie : KASLR plus facile à contourner. Demander l'accord de l'utilisateur.
- Confiance : KPTI actif prouvé ; gain probable sur les charges riches en appels système (binder), à mesurer.

### P6 [PERF] Goodix-TS upstream : ~155 ms de démarrage perdus

- Où : `CONFIG_TOUCHSCREEN_GOODIX=y` (`ac8257_ujc201_defconfig:2096`), `drivers/input/touchscreen/goodix.c`.
- Problème : le `goodix.c` AutoChips du 4.9 testait d'abord la propriété DT `ti-link` et abandonnait en 0,3 ms. Le
  pilote upstream 4.19 fait un reset complet puis 2 essais I2C et échoue (`-121`) après ~155 ms, dans le thread d'init.
  Le vrai tactile est le GT928 du pilote MTK `mtk-tpd`, non concerné.
- Correctif : `# CONFIG_TOUCHSCREEN_GOODIX is not set`.
- Confiance : prouvé (journaux 4.9 et 4.19).

### P7 [BAS] Bruit irqchip au démarrage (optionnel)

- Où : `arch/arm64/boot/dts/mediatek/ac8257.dts:452` : `toprgu` déclare `interrupts = <GIC_SPI 83 IRQ_TYPE_NONE>`,
  ce qui provoque un WARN `gic_irq_domain_translate` (~40 lignes) à chaque démarrage. Les messages « type mismatch »
  hwirq-189 (dualarm/utos) et hwirq-0 (pinctrl) existent aussi en 4.9 : pas une régression, sans effet.
- Correctif optionnel : `IRQ_TYPE_LEVEL_LOW` sur toprgu, après vérification du type réel de l'IRQ watchdog.
- Confiance : absence de régression prouvée ; type exact à vérifier.

---

## Lot 3 : correctifs qui demandent un test ciblé ou un choix

### B6 [HAUT] backcar : attente de la fin de l'ARM2 sans condition

- Où : `drivers/misc/autochips/backcar/backcar.c:349` (`schedule();` seul) et `:384-386` (`free_memblock_runtime()` des
  zones UI, MDP, ISP) ; `backcar_isr_handler`.
- Problème : un seul `schedule()` sans boucle ni condition. Tout `wake_up()` (n'importe quel message ARM2, même le cas
  « unnormal ») déclenche l'arrêt des horloges ISP/CMDQ et la libération des mémoires ARM2. `g_ioctlwaitq` pointe sur
  une file d'attente sur la pile de l'ioctl, lue par l'ISR sans verrou : sur la voie signal, une IRQ entre
  `remove_wait_queue()` et `g_ioctlwaitq = NULL` réveille une pile déjà dépilée.
- Correctif proposé : file statique `DECLARE_WAIT_QUEUE_HEAD(arm2_wq)` + drapeau `arm2_resp` ; l'ISR ne le pose que pour
  `MSG_ARM2_RESPONSE` valide ; l'ioctl fait `wait_event_interruptible_timeout(arm2_wq, READ_ONCE(arm2_resp), ...)`.
  Avant `free_memblock_runtime()`, démapper les MVA fixes (voir B7).
- Test : marche arrière pendant et après le démarrage, plusieurs cycles ; vérifier l'affichage et l'absence de plantage.
- Confiance : prouvé (code), identique au 4.9 ; lien avec les plantages userspace à vérifier.

### B7 [MOYEN] Mémoires réservées rendues au buddy alors que l'IOMMU les mappe encore 1:1

- Où : `mm/page_alloc.c:8480` (`free_memblock_runtime`, `CONFIG_MEMBLK_RELEASE_POLICY=y` en defconfig ligne 463) ;
  appelants `mtkfb.c:3206` et `backcar.c:384-386` ; mappings `drivers/iommu/mtk_iommu_v2.c` (région `IOMMU_RESV_DIRECT`
  du framebuffer) et `drivers/misc/mediatek/pseudo_m4u/pseudo_m4u_v2.c` (`pseudo_disp_reserve_mva`, MVA fixes
  MDP 0x65100000, UI ARM2 0x77400000, DI 0x60800000, ISP 0x76000000, jamais démappées).
- Problème : ces pages repartent dans ZONE_DMA32 (slab, tables de pages, ION) mais restent accessibles en lecture/écriture
  par tous les ports M4U. r175 : `free_memblock_runtime: 44008K freed from 0x62506000` dans la zone FB mappée 1:1, puis
  5120K, 15360K, 20480K libérés à 64,7 s. Le noyau 4.9 LibreHU qui marche n'avait pas cette libération.
- Correctif proposé : démapper avant de libérer (`m4u_dealloc_mva_sg()` / `pseudo_unmap_free_iova()` sur les entrées
  réservées ARM2/MDP/ISP via une fonction exportée `pseudo_release_reserve_mva(id)` ; `iommu_unmap()` de la partie
  libérée du FB). Test simple d'abord : `CONFIG_MEMBLK_RELEASE_POLICY=n` comme le 4.9 stable (coût ~85 Mio sous 4 Gio).
- Confiance : probable (mappings et libérations prouvés ; accès matériel après libération non prouvé).

### B8 [MOYEN] mtkfb : le tas ION FB couvre la queue du framebuffer rendue au buddy

- Où : `drivers/misc/mediatek/video/ac8257/videox/mtkfb.c:3194-3213` (`mtkfb_fb_reserve_mem_free`), `:3339-3367`,
  `:3471` (`ion_drv_create_FB_heap(mtkfb_get_fb_base(), mtkfb_get_fb_size())`).
- Problème : le tas `ion_fb_heap` et la MVA `disp_fb0` couvrent toute la réserve, puis la fin de la réserve est rendue
  au buddy. Une allocation dans le tas FB pourrait rendre des pages déjà utilisées ailleurs.
- Correctif proposé : créer le tas FB et la MVA sur la seule partie conservée (`vramsize - free_mem_size`).
- Confiance : prouvé (code) ; impact faible tant que gralloc n'alloue pas dans ce tas.

### B9 [MOYEN] LCM / pont LVDS : `interface_client NULL!` vient d'un indice, pas d'un ordre de sondage

- Où : `drivers/misc/mediatek/lcm/inc/lcm_i2c_interface.h:24` (`LCM_ID_SECONDARY = 0`, `LCM_ID_PRIMARY = 1`),
  `lcm_i2c_interface.c` (`interface_driver_probe`), `lcm_driver_common.c`, `ddp_pwm.c` ; le DTBO stock déclare
  `lcm_id = <0>` pour le pont I2C 0x2d.
- Problème : le client est rangé en SECONDARY ; tout le code « primaire » lit l'indice 1, vide. À chaque réglage de
  luminosité, `modules_lcm_backlight_ctrl()` tente aussi une détection DS90UB947 sans cache : accès NULL + transaction
  I2C 0x0e en NACK + ~6 lignes de log. Identique en 4.9.
- Correctif proposé : dans `interface_driver_probe()`, traiter `lcm_id` 0 comme PRIMARY quand aucun nœud n'a
  `lcm_id = 1` ; mettre en cache la détection DS90UB947 par indice ; ne pas appeler le rétroéclairage pour un indice
  sans client. Supprimer le double `i2c_unregister_device()` de `interface_driver_remove()`.
- Confiance : prouvé (DT, code, journal) ; effet sur la reprise après veille à vérifier.

### B10 [MOYEN] Adresse MAC Wi-Fi différente à chaque activation

- Où : `drivers/misc/mediatek/connectivity/wlan/core/gen4m/os/linux/gl_init.c:109`
  (`#define WIFI_MAC_ADDRESS_FILE "efs/wifi/.mac.info"`, chemin RELATIF ouvert depuis un kthread) ; `common/wlan_lib.c`
  (`wlanUpdateNetworkAddress` : MAC nulle → préfixe 00:08:22 + compteur d'horloge).
- Problème : la NVRAM Wi-Fi envoyée par l'espace utilisateur a une MAC nulle et le fichier efs n'existe pas
  (`Open mac info file fail, errno=-2`) : MAC générée à partir de l'horloge, nouvelle à chaque activation, sans le bit
  « localement administrée ».
- Correctif proposé : trouver où le stock range la MAC (comparer `/sys/class/net/wlan0/address` sur 2 démarrages en 4.9,
  partitions nvram/proinfo) ; sinon générer une MAC stable une fois (hachage d'un identifiant de l'appareil, bit
  « locally administered » posé) ; au minimum un chemin absolu. Ne jamais publier ni journaliser la MAC lue.
- Pas la cause de l'échec WPA2 (voir H3). Confiance : prouvé (journal).

### B11 [BAS] PSCI : index 4..7 lus au-delà de la table de 3 états

- Où : `drivers/firmware/psci.c` (`psci_suspend_finisher`, `psci_cpu_suspend_enter`, commit `ba7e90c`).
- Problème : la table per-CPU a 3 entrées (`cpu-idle-states = <&STANDBY &MCDI_CPU &MCDI_CLUSTER>`), mais SODI (4),
  SODI3 (5), DPIDLE (6) et SUSPEND (7) arrivent aussi ici : `state[index - 1]` lit hors du `kcalloc`. En pratique la
  mémoire vaut 0 (même comportement qu'en 4.9 et chez MediaTek).
- Correctif proposé : lister les 7 états dans `cpu-idle-states` (le pilote `cpuidle-mtk_acao` n'utilise pas cette liste
  pour lui-même), ou borner l'index avec un compteur per-CPU rempli dans `psci_dt_cpu_init_idle()`.
  ATTENTION : MCDI venait de faire planter le démarrage depuis la partition boot (correctif `ba7e90c`). Ne changer
  qu'avec un test recovery_test puis boot, et vérifier que SODI/DPIDLE se comportent comme en 4.9.
- Confiance : lecture hors tableau prouvée ; effet à l'exécution à vérifier.

### B12 [BAS] Petits défauts du pilote Wi-Fi

- `gl_kal.c` (~ligne 791, `kalUpdateMACAddress`) : un pointeur passé à `MACSTR` sans `MAC2STR` → arguments décalés,
  adresse noyau imprimée. Correctif : `MAC2STR(prGlueInfo->prDevHandler->dev_addr)`.
- `gl_init.c:5088` : `#if KERNEL_VERSION(4, 19, 0) >= LINUX_VERSION_CODE` est faux en 4.19.325 (LINUX_VERSION_CODE
  vaut 4.19.255). Sans effet aujourd'hui (priorité de thread 0, pas de `wifi.cfg`). Correctif :
  `#if KERNEL_VERSION(5, 9, 0) > LINUX_VERSION_CODE`.
- Confiance : prouvé.

### B13 [BAS] `SECURITY_PERF_EVENTS_RESTRICT` absent : `perf_event_paranoid=3` d'Android sans effet

- Où : `kernel/events/core.c` (paranoid = 2, plus de branche pour 3), `security/Kconfig`.
- Correctif proposé : réintroduire le patch « security,perf: Allow further restriction of perf_event_open »
  (présent dans `stable-4.9.337`, `security/Kconfig` et `kernel/events/core.c`, `perf_paranoid_any()`), puis
  `CONFIG_SECURITY_PERF_EVENTS_RESTRICT=y`. Sécurité uniquement, aucun effet fonctionnel.
- Confiance : prouvé.

---

## Hors noyau (ne pas corriger dans ce dépôt)

- H1 ROM LOS 16 (a9) : VINTF refuse le noyau 4.19 (`Framework is incompatible with kernel version 4.19.325`) →
  dialogue « Un problème interne est survenu ». Ajouter une entrée `<kernel version="4.19.x">` à la matrice du framework
  (dépôt de l'arbre de l'appareil / de la ROM).
- H2 Politique SELinux vendor, seulement si Android passe un jour en Enforcing : le noyau 4.19 contrôle la permission
  `map`. Ajouter `allow camhal3lite gpu_device:chr_file map;`, remplacer l'accès de `vtservice_hidl` à
  `hwservicemanager_prop` par `get_prop()`, règle `map` pour `librehu_ksunext`. Les autres refus en Enforcing
  (MCU ttyS1, batterie, FM, bootanim) sont des trous de la politique vendor, identiques en 4.9. Ne pas affaiblir le
  noyau pour ça.
- H3 Wi-Fi station, échec du 4-way WPA2 sur un point d'accès 5 GHz canal 100 (DFS) : les chemins du pilote (IE RSN,
  protection des trames EAPOL, débit, MAC) ont été vérifiés un par un, aucune divergence. Le seul écart 4.9/4.19 trouvé
  est la région : en 4.9 le pilote restait en CN (canal 100 invisible) et l'unité prenait le BSS 2,4 GHz du même
  SSID ; en 4.19 la base réglementaire est intégrée (`alpha2=FR`) et le framework choisit le BSS 5 GHz canal 100. Tests
  à faire par l'utilisateur : forcer le BSS 2,4 GHz (`wpa_cli -i wlan0 bssid <id> <BSSID 2,4 GHz>`) ou couper le 5 GHz
  du routeur ; vérifier sur le routeur que le BSS 5 GHz est en WPA2-PSK avec la même phrase de passe (pas WPA3 seul ni
  PMF obligatoire). Contournement pilote possible ensuite : transmettre tel quel l'IE RSN du supplicant en mode station
  (comme `_addRSNIE_impl` le fait déjà en mode AP, `mgmt/rsn.c:1502`).
- H4 Service LibreHU (`org.librehu.service`) : l'heure est remise au 1er janvier 2000 vers 62 s parce que le service
  recopie l'heure du MCU non réglé (`Clock set from MCU: 2000-01-01...`), puis réécrit la RTC. Le noyau et la RTC sont
  sains. Correctif dans le service : ignorer l'heure MCU si l'année est < 2024 ou plus ancienne que l'heure système.
- H5 Contournement P1 côté ROM : `persist.vendor.uartconsole.enable=0`.

## Vérifié, à NE PAS corriger

- Messages SELinux « unknown classes and permissions will be denied » (process2, nlmsg_readpriv/getneigh, sctp, xdp,
  infiniband) : sans effet en Permissive ; `process2` sans effet car `nnp_nosuid_transition=0`.
- `irq: type mismatch` hwirq-189 et hwirq-0 : présents en 4.9 aussi, sans effet (`CONFIG_MICROTRUST_TEE_SUPPORT` inactif).
- RTC MT6357 : lecture/écriture correctes ; « read al time = 2010/01/01 » est le registre d'alarme, identique en 4.9.
- Priorité temps réel des threads Wi-Fi : jamais appliquée, ni en 4.9 ni en 4.19 (pas de `wifi.cfg`). L'idée « priorité
  RT en 4.9, pas en 4.19 » est fausse.

## Traces non rédigées de l'agent performance

L'agent « configuration / performance » a été coupé en pleine analyse. Ses commandes et leurs résultats ont été
relus après coup. Voici ce qu'il avait trouvé sans l'écrire. Ce sont des pistes : elles ne sont pas encore au
niveau de preuve des fiches ci-dessus.

### P8 [PERF, à vérifier] `trace_printk()` dans des pilotes intégrés : tampons de trace alloués à chaque démarrage

- Preuve : dmesg r175, à 0 s : `** trace_printk() being used. Allocating extra memory.  **`. Objets intégrés qui
  contiennent une section `__trace_printk_fmt` : `fiq_cache/mtk-fiq-cache.o` (`mt_fiq_cache_flush_all()`),
  `cmdq/v3/{cmdq_driver,cmdq_helper_ext,cmdq_mdp_common,cmdq_record}.o`, `gpu/ged/src/ged_log.o`,
  `video/ac8257/videox/mtkfb_fence.o`, `video/ac8257/dispsys/display_recorder.o`, `mmp/src/mmprofile.o`,
  `performance/base/utility.o`, `performance/cpu_loading/cpu_loading.o`, `smi/met/met_smi.o` ; et dans les modules
  de connectivité (`osal.o` : `osal_ftrace_print()`, actif seulement si `ftrace_flag`, `bt_drv.o`, `stp_chrdev_bt.o`).
- Mécanisme (`kernel/trace/trace.c`, `trace_printk_init_buffers()`) : la présence d'un seul format suffit pour
  allouer les tampons de trace par CPU à leur taille complète (`TRACE_BUF_SIZE_DEFAULT` = 1 441 792 octets par CPU,
  soit ~5,5 Mio pour 4 CPU, à confirmer sur l'unité), puis lancer l'enregistrement des noms de tâches
  (`tracing_start_cmdline_record()`) : sondes permanentes sur `sched_switch` et `sched_wakeup`.
- Effet probable : quelques Mio de RAM et un petit coût à chaque changement de contexte. Les appels fréquents
  (CMDQ par trame ?) écrivent aussi dans le tampon. Probablement pareil en 4.9 (même code vendor) : non vérifié.
- Vérification sur l'unité : `cat /sys/kernel/debug/tracing/buffer_total_size_kb` et `cat /sys/kernel/debug/tracing/trace | head`.
- Correctif proposé : remplacer ces `trace_printk()` par `pr_debug()` ou des événements de trace, ou les compiler hors
  production. Le plus large : un en-tête commun aux dossiers vendor qui redéfinit `trace_printk` en no-op hors `DEBUG`.

### P9 [À ANALYSER] `CPUn: Unknown IPI message 0xf` : 83 messages pr_crit (console UART)

- Preuve : dmesg r175 complet, 83 lignes `Unknown IPI message 0xf`, dès 0,008 s sur chaque CPU, puis en rafales
  (24 vers 153 s, 25 vers 172 s, 11 vers 188 s). Message de `arch/arm64/kernel/smp.c:927` (`handle_IPI`).
- Contexte : 0xf = SGI 15, inconnu des deux noyaux (4.19 : 7 IPI, 0 à 6 ; 4.9 : 6 IPI). Les autres journaux 4.19 et 4.9
  disponibles sont des captures vides (2 lignes) : impossible de dire si le 4.9 l'avait aussi. À noter : 4.19 a inséré
  `IPI_CPU_CRASH_STOP` en position 3, ce qui décale `IPI_TIMER`, `IPI_IRQ_WORK` et `IPI_WAKEUP` d'un cran par rapport
  au 4.9. Vérifier qu'aucun code vendor ou firmware (ATF, MCDI, SPM) n'envoie de SGI avec les numéros du 4.9.
- Effet : au minimum, du bruit sur la console UART (voir P1) ; au pire, un réveil ou un IPI attendu par le firmware
  qui n'est pas traité.
- Pistes : chercher qui envoie le SGI 15 (`grep -rn "SGI\|ipi" drivers/misc/mediatek/base/power/{mcdi,spm}`, ATF
  MediaTek MT6765) ; corréler les rafales avec l'écran éteint ou les états SODI ; comparer avec un dmesg 4.9 complet.

### Vérifié par l'agent, sans problème

- `slub_debug=OFZPU` et `page_owner=on` passés par le LK : sans effet, car `CONFIG_SLUB_DEBUG` et `CONFIG_PAGE_OWNER`
  ne sont pas activés (pareil en 4.9). Aucun coût.
- zram : le swap de 1 Gio est actif (bugreport : `1,048,572K total swap`). Le message
  `zram: Cannot change disksize for initialized device` puis `swapon failed for /dev/block/zram0` à 60 s semble venir
  d'un second `swapon` (fstab et .rc ?) : sans effet, à confirmer côté ROM.
- eMMC : file blk-mq avec `mq-deadline` ; `CONFIG_MTK_EMMC_CQ_SUPPORT=y` comme en 4.9.

### Pistes laissées en cours

- cpufreq MediaTek (`drivers/misc/mediatek/base/power/cpufreq_v1/src/mtk_cpufreq_main.c:1919`) : `.target` sans
  `fast_switch`, donc schedutil passe par son kthread pour chaque changement de fréquence. Coût et intérêt d'un
  `fast_switch` à évaluer.
- Options du 4.9 absentes en 4.19 côté stockage : `CONFIG_MTK_BLOCK_TAG`, `CONFIG_MMC_BLOCK_BOUNCE`, `CONFIG_HIE`,
  `CONFIG_MMC_BLOCK_IO_LOG`. Dire pour chacune si elle a un équivalent 4.19 ou si elle compte pour les performances.
- `CONFIG_SCHED_DEBUG=y` et `CONFIG_SCHEDSTATS=y` (comme en 4.9) : coût faible tant que schedstats est désactivé à
  l'exécution, mais chaque test coûte une lecture mémoire tant que `JUMP_LABEL=n` (P3).
- Volume du journal noyau (r175) : I2C en NACK (~865 lignes entre 210 et 240 s, bus i2c-0 à i2c-5), `[wlan` 1 128
  lignes, `[MTZ` (metazone) 499, KernelSU 396, refus SELinux 441 lignes (107 Ko). Tout passe par la console UART
  (P1). Réduire la verbosité de ces pilotes (niveau de log Wi-Fi, metazone, KernelSU) et limiter les erreurs I2C.
- Côté ROM, pas noyau :
  - cpusets `critical` et `camera-daemon` absents (`couldn't write ... /dev/cpuset/critical/tasks`, pour composer,
    backcar, car_event_monitor, fastdisplay, avmhal, cameraserver) : un .rc vendor attend des dossiers qu'aucun
    script ne crée ;
  - libPowerHal : `error : 2, No such file or directory` (×8) et `cpu_ctrl set freq: 2601000` alors que le maximum
    est 1 917 000 kHz : configuration PowerHAL faite pour un autre SoC ;
  - `com.android.phone` attend `android.hardware.radio@1.0::IRadio/slot1` chaque seconde (pas de modem).

## Reste à auditer (agents arrêtés avant la fin)

- Cœur : recherche systématique des tests `KERNEL_VERSION(4, 19, ...)` faux (LINUX_VERSION_CODE = 4.19.255) dans tout
  l'arbre vendor ; compat des minuteurs (`setup_timer`, `.function` après `timer_setup`, `legacy_function`) dans
  `drivers/misc/mediatek` et `drivers/misc/autochips` ; `cpuidle-mtk_acao.c`, MCDI, SPM, PPM, cpufreq ; pstore.
- Mémoire : ION (kref, `dma_buf_ops` 4.19, cache sync, shrinker), CMA et zone-movable, coût du map/unmap page par page.
- Affichage : CMDQ (`chan 16 ... err:-19`), GPU PowerVR (`3D command buffer usage high`), dma-fence/sync, perte de
  transparence signalée en r174, hwcomposer `No overlay input queue`, encodeur vidéo (scrcpy), avin/tvd/di/metazone.
- Connectivité : comportements cfg80211/nl80211 4.19 (connect, add_key, external_auth), `select_queue`, autres erreurs
  du journal (`Netd: invalid interface index: wlan%d(0)`, `enable_irq_wake ERROR(-6)`, firmware FM manquant).
- Configuration/perf : options de débogage imposées par le LK (`slub_debug=OFZPU`, `page_owner=on`) et leur coût ;
  ordonnanceur/EAS/schedtune, cpusets Android (`couldn't write to /dev/cpuset/critical/tasks`) ; ordonnanceur de blocs,
  read-ahead, zram (`Cannot change disksize for initialized device`) ; congestion TCP ; vmalloc=496M, LMK/PSI, THP, KSM.
- Revue des commits récents : `ec8f6b0` (eBPF), `e9c32fd` (SELinux/KernelSU, coût de `security_secid_to_secctx` dans
  exec), `11a0b7d`, `ff9ddef`, outils `tools/twrp`, `tools/ac8257`, workflow CI.
- Entrées-sorties : MSDC, USB/musb (`otg_iddig: probe failed`), audio (`AudDrv_GPIO_probe ... fail`), PMIC et
  régulateurs (`failed to register vcama/vcamd`), batterie factice LibreHU, thermique (sondes absentes signalées en r174),
  clavier, IR, NACK I2C en boucle (0x10, 0x11, 0x0e, 0x44), imgsensor (`No imgsensor alive`).
- Compatibilité Android : binder/ashmem/ION (compat 32 bits), cgroups/cpusets, `uid_sys_stats`, netlink
  (`'hostapd': attribute type N has an invalid length`), lmkd/PSI, keychord, KernelSU (`/metadata/ksu/modules.rc`).
- Symptômes de l'utilisateur à recontrôler avec un noyau récent (r176) : transparence, sondes de température,
  animation de démarrage, batterie factice, `/dev/fm`.
