# Vulkan sur l'UJC201 — branche `rc-4.19.325-vulkan` (EXPÉRIMENTAL)

Objectif : Vulkan 1.1 pour les applis, sans rien casser du reste (affichage, vidéo, caméra, Android Auto, CarPlay),
et retour à l'état stock par un simple changement de noyau.

## 1. Pourquoi changer de pilote GPU

| Constat | Preuve |
|---|---|
| GPU : PowerVR Rogue **GE8300**, BVNC **22.68.54.30** | `include/config_kernel_user_ac8257.h` (1.10 et 1.11) |
| Pilote stock : DDK **1.10@5130912** (noyau, firmware, espace utilisateur), **aucun** pilote Vulkan dans le vendor (AIDA64 : « Vulkan non supporté ») | `libsrv_um.so`, `libIMGegl.so`, `libGLESv2_mtk.so`, `gralloc.ac8257.so` d'un vendor stock UJC201 (build 1681 ; build 1516 identique : mêmes empreintes) |
| Aucune version 1.10 du DDK MediaTek n'a eu Vulkan | vendors 1.10@5130912 des Redmi 6 / 6A (Android 9) : toutes les libs GPU, pas de `vulkan.*.so` |
| Le DDK **1.11@5425693** de MediaTek pour le même cœur (MT6761) a `vulkan.mt6761.so` (Vulkan **1.1**, `android.hardware.vulkan.version` = 4198400) | vendor TCL Tokyo_TF (MT6761) ; vendor Xiaomi certus (même build) pour les XML |
| Le pilote noyau refuse un espace utilisateur d'une autre version (`PVRSRV_ERROR_DDK_VERSION_MISMATCH`) | `services/server/common/srvcore.c` |

Donc : Vulkan = **toute** la pile GPU en 1.11 (pilote noyau, firmware, libs, gralloc). Le pilote libre
(drm/imagination + Mesa) n'est pas une option : noyau 6.x, cœur « unsupported » chez Mesa, affichage MTK non DRM.

## 2. Ce que fait la branche

### 2.1 Noyau (`drivers/misc/mediatek/gpu/gpu_rgx/m1.11ED5425693`)
- Source : `drivers/gpu/mediatek/gpu_rgx/m1.11ED5425693` de MediaTek-Labs/common-kernel-4.19 (commit e440f21f), importé
  tel quel (commit d'import séparé), puis adapté :
  - plateforme AC8257 = plateforme GPU MT6761 (même cœur, 460 MHz, `mediatek,clark`) ; code AutoChips de
    `rgx_mtk/ac8257` (alimentation, horloges, DVFS GED) repris du 1.10, même interface de 10 fonctions ;
  - **pages GPU sous 4 Gio** (GPU 32 bits, unité 6 Gio) : le 1.11 n'a plus aucune garde sur ce chemin
    (`PVR_LINUX_PHYSMEM_USE_HIGHMEM_ONLY`), d'où `__GFP_DMA32` + masque DMA cohérent 32 bits, comme `73901824d` pour le
    1.10 (sinon : le blocage au menu de démarrage du noyau r237) ;
  - config `config_kernel_*_ac8257.h` = celle de MediaTek pour le MT6761 (options de build identiques à l'espace
    utilisateur 1.11), sauf le **nom du firmware** : `rgx_ddk111.fw.22.68.54.30` (le firmware 1.10 du vendor reste
    pour les autres noyaux) ;
  - `pvr_dvfs_device.c` (DVFS devfreq, MT6779) non compilé : DVFS GED de MediaTek, comme en 1.10.
- Sélection : `CONFIG_MTK_GPU_VERSION="rgx clark m1.11ED5425693"` (defconfig de cette branche).
- Drapeau : `/firmware/android/librehu_gpu = "1.11"` dans `ac8257_ujc201.dts`, lu par init en
  `ro.boot.librehu_gpu=1.11`. Il est **dans le device tree du noyau** : impossible d'avoir l'un sans l'autre.

### 2.2 Paquet vendor (`tools/gpu`, zip `librehu_gpu_ddk111_twrp.zip`)
N'écrase **aucun** fichier stock. Ajoute :

| Fichier | Rôle | Sous un autre noyau |
|---|---|---|
| `/vendor/librehu/ddk111/lib{,64}/…` (11 × 2) | copies 1.11 de `libsrv_um`, `libIMGegl`, `libusc`, `libglslcompiler`, `libufwriter`, `libPVROCL`, `libPVRScopeServices`, `egl/libEGL_mtk`, `egl/libGLESv1_CM_mtk`, `egl/libGLESv2_mtk`, `hw/gralloc.ac8257` | pas montées |
| `/vendor/librehu/ddk111/permissions/` | XML Vulkan (level 0, version 1.1, compute 0) | pas montés |
| `/vendor/lib{,64}/hw/vulkan.librehu.so` | pilote Vulkan | jamais chargé (`ro.hardware.vulkan` absent) |
| `/vendor/lib{,64}/libgex.so` | 3 fonctions manquantes aux libs stock (voir 4.3) | inutilisée |
| `/vendor/firmware/rgx_ddk111.fw.22.68.54.30` | firmware 1.11 | le pilote 1.10 ne la demande pas |
| `/vendor/etc/init/librehu_gpu_ddk111.rc` | `on early-init && property:ro.boot.librehu_gpu=1.11` : 23 bind mounts + `setprop ro.hardware.vulkan librehu` | ne fait rien |

Les bind mounts sont posés en `early-init`, avant tout utilisateur du GPU (allocator, SurfaceFlinger, applis) ; sous
Android 10, `/` est partagé avant la séparation des espaces de noms (`SetupMountNamespaces`), donc visibles partout.

## 3. Ordre de test sur l'unité

Pré-requis : ROM 17.1 à jour, partition boot de 32 Mio (`librehu_boot32_twrp.zip` déjà passé), sauvegarde TWRP.

1. TWRP : `librehu_gpu_ddk111_twrp.zip` (release `latest-rc-4.19.325-vulkan`). Ne change rien tant qu'un noyau 1.11
   n'est pas dans boot.
2. TWRP : `librehu_kernel32_twrp_a10.zip` de la même release (refuse si l'étape 1 manque).
3. Redémarrer. Si l'affichage ne vient pas en 2 min : forcer le redémarrage, menu de démarrage → TWRP, flasher un
   autre noyau (section 4) : tout revient.
4. Contrôles (PowerShell) :
   ```
   adb shell 'getprop ro.boot.librehu_gpu; getprop ro.hardware.vulkan; ls /vendor/odm/etc/permissions'
   adb shell 'pm list features | grep -i vulkan'
   adb shell 'dmesg | grep -i -E "pvr|rgx|ged" | head -80'
   adb shell 'cat /proc/zoneinfo | grep -E "^Node|present|managed" | head -12'
   ```
   Puis AIDA64 (onglet Vulkan), vidéo YouTube, caméra de recul, Android Auto / CarPlay, veille / réveil.

## 4. Retour arrière
- **Flasher n'importe quel autre noyau** (mise à jour de la ROM 17.1, zip noyau `rc-4.19.325`, `stable-4.9.337`) :
  sans `ro.boot.librehu_gpu`, rien n'est monté, la pile stock 1.10 revient d'elle-même.
- Puis, si on veut nettoyer : `librehu_gpu_ddk111_remove_twrp.zip` (refuse tant qu'un noyau 1.11 est dans boot, sinon
  l'affichage disparaîtrait). Le vendor redevient identique à l'original (vérifié par le CI).

## 5. Problèmes connus et potentiels

| # | Risque | Parade / état |
|---|---|---|
| 1 | Pages GPU au-dessus de 4 Gio (blocage au démarrage, comme r237) | **Corrigé** : `__GFP_DMA32` dans `_GetGFPFlags()` et `OSPhyContigPagesAlloc()`, masque cohérent 32 bits |
| 2 | Noyau 1.11 sans paquet = plus d'affichage | zip noyau de la branche refuse sans paquet ; notes de release ; images `boot_*.img` à ne pas flasher à la main |
| 3 | Paquet retiré alors que le noyau 1.11 reste | zip de retrait refuse (lit `librehu_gpu` dans la partition boot) |
| 4 | Mise à jour OTA de la ROM | remet le noyau standard → pile 1.10, Vulkan disparaît jusqu'au prochain flash du noyau -vulkan ; rien ne casse |
| 5 | Libs 1.11 qui ne se chargent pas sur le vendor Android 9 | **Vérifié** (`check_vendor_abi.py`, CI) : chaque `NEEDED` et chaque symbole résolus avec VNDK 28, LLNDK API 28 et les libs MTK stock ; libs DDK compilées API 28 (`.note.android.ident`) |
| 6 | `libged` / `libgpu_aux` stock trop vieux (3 fonctions absentes) | `libgex.so` : `ged_gpu_hint_cpu` / `ged_hint_force_mdp` renvoient ce que renvoie la lib MTK 1.11 sur ce noyau (commandes GED 13 / 14 absentes du noyau → `GED_ERROR_FAIL`) ; `GuiExtAuxIsSupportPQWhiteList` = 0 comme la lib MTK (`mov w0, wzr; ret`). `libged` 1.11 inutilisable (exige le HIDL `vendor.mediatek.hardware.gpu@1.0`), `libgpu_aux` 1.11 aussi (`DpAsyncBlitStream2` d'Android 10) |
| 7 | Contrat gralloc 1.11 ↔ `libgralloc_extra` / hwcomposer Jancar | mêmes exports ; même borne des requêtes (`0x33`) ; la 1.11 ajoute 2 codes GE en fin de liste (`0x3e`, `0x3f`) que la lib stock refuserait : **à tester** (artefacts d'image, plantage SurfaceFlinger) |
| 8 | Firmware 1.11 ↔ pilote 1.11 | même build 5425693 ; vérifié par le pilote à l'init (`dmesg` : compat check) : **à tester** |
| 9 | DVFS / veille avec le cœur 1.11 et le code plateforme AutoChips 1.10 | compile, mêmes fonctions GED / gpufreq ; **à tester** (fréquence GPU, veille / réveil) |
| 10 | SELinux | `mount` s'exécute dans init, `setprop` dans le sous-contexte `vendor_init` (Android 10, `builtins.cpp`) ; les images de boot sont en `permissive` (comme le stock, `BUILD_INFO`) ; en `enforcing` il faudrait une règle `mounton` et le droit de poser `ro.hardware.vulkan` |
| 11 | Autre build de vendor Jancar | l'installeur compare les empreintes des libs MTK utilisées et du hwcomposer ; différent = refus sans écrire |
| 12 | dm-verity actif sur vendor | refus sans écrire ; accepté si vbmeta a HASHTREE_DISABLED **ou** VERIFICATION_DISABLED (celui qu'exige la ROM) : les deux coupent dm-verity (`libfs_avb`, `SetUpAvbHashtree` → `kDisabled`) |
| 13 | `ro.hardware.vulkan` déjà posé / pilote Vulkan déjà présent | refus sans écrire (sinon fonctionnalité annoncée sans pilote) |
| 14 | memtrack (statistiques mémoire GPU) | peut afficher 0 ; sans effet |
| 15 | Interface Android (HWUI) | reste en OpenGL ES ; Vulkan seulement pour les applis qui le demandent |
| 16 | Conformité Vulkan | non vérifiée ici (pas de dEQP / CTS) ; c'est le pilote que MediaTek livrait sur MT6761 |
| 17 | Noyau 4.9 (16.0) | pas porté ; une source 1.11 pour 4.9 existe (OpenWatchProject/android_kernel-4.9, config MT6761) |
| 18 | Licence | blobs MediaTek / Imagination d'un autre appareil (TCL), récupérés au commit épinglé par le CI |

## 6. Vérifié hors ligne (avant tout essai)
- Le pilote 1.11 compile pour cette config ; ses seuls symboles externes absents du 1.10 (`drm_dev_put`, `proc_create`,
  `remove_proc_entry`, `seq_vprintf`, `single_open`, `single_release`) existent dans ce noyau.
- Les 25 blobs ont l'empreinte de `tools/gpu/ddk111_blobs.sha256` ; tous les fichiers DDK portent `1.11@5425693`.
- ABI du paquet : 0 erreur sur 26 libs (arm64 et arm).
- gralloc 1.11 et gralloc stock 1.10 : même module (`HMI` : `gralloc`, « IMG Rogue Graphics HAL », API 1.0 = gralloc1, celle
  qu'utilise le hwcomposer Jancar via `GrallocDevice`) ; mêmes imports de `libgralloc_extra` sauf `gralloc_extra_perform` et
  `gralloc_extra_sf_set_status2`, que la lib stock exporte.
- Chaîne d'activation (sources LineageOS 17.1 / 16.0) : init lit `/firmware/android/*` en `ro.boot.*` dans
  `property_init()` (`process_kernel_dt`), avant `LoadBootScripts` ; `on early-init && property:…` est accepté par
  `action_parser.cpp` et la condition relue par `GetProperty` ; `mount … bind` (`MS_BIND`) ; `/` partagé avant la
  séparation des espaces de noms ; libvulkan cherche `vulkan.<ro.hardware.vulkan>.so` puis `vulkan.<ro.board.platform>.so`
  dans `sphal` (puis `hw_get_module`) : `vulkan.librehu.so` n'est trouvé que si `ro.hardware.vulkan=librehu` ;
  `SystemConfig` lit `/odm/etc/permissions` avec les fonctionnalités autorisées (9 et 10) ; `/odm/etc` → `/vendor/odm/etc`.
- Linker : chemins `sphal` = `/odm/${LIB}`, `/vendor/${LIB}`, `/vendor/${LIB}/hw` ; toutes les dépendances du paquet
  passent par VNDK-SP, LLNDK, le paquet ou le vendor (aucune n'exige VNDK-core, invisible de `sphal`).
- Pilote ↔ libs : `libsrv_um` 1.11 envoie options `0x80002010`, version `0x1000b` (1.11), build `0x52ca1d` (5425693) ;
  le pilote attend `RGX_BUILD_OPTIONS_KM = 0x2010` (masque `0xFFFF`), 1.11, 5425693 : identiques. Mêmes nœuds
  (`/dev/dri/card%d`, `renderD%d`, `/dev/pvr_sync`) qu'en 1.10.
- Firmware : en-tête à `taille − 4096` (`0x1d000`) au format attendu (`FW_INFO_VERSION` 1, 32 octets, 6 entrées de 24),
  BVNC 22.68.54.30 ; build vérifié par le pilote au démarrage (avertissement seulement sans `PVRSRV_STRICT_COMPAT_CHECK`).
- Noyau publié (r239) : config identique à la RC r238 sauf `CONFIG_MTK_GPU_VERSION`, mêmes lignes de commande ; device
  tree : `/firmware/android/librehu_gpu = "1.11"`, fstab et vbmeta intacts ; 132 objets du pilote 1.11 compilés, aucun
  du 1.10 ; diff source avec `rc-4.19.325` limité au GPU, au device tree (1 propriété), au defconfig (1 ligne), aux outils.
- Zips : 12 scénarios (`tools/gpu/test_twrp_zips.sh`) : installation, réinstallation, 5 refus sans écriture, vbmeta drapeaux 1 / 2 / 3 acceptés, retrait
  refusé sous noyau 1.11, retrait → vendor identique à l'original ; zip noyau : refus sans paquet, flash avec.
