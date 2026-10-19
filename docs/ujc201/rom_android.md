# ROM Android de l'UJC201 : /system, zips audio, applis LibreHU

<!-- Deplace depuis le README du depot TWRP (android_device_alps_ac8257_demo), 19/10/2026. -->

## /system en ecriture (root)
- System-as-root (`BOARD_BUILD_SYSTEM_ROOT_IMAGE`) : /system est un dossier de `/`, qui apparait comme `/dev/root`
  dans `/proc/mounts`, un noeud qui n'existe pas : on ne peut ni identifier le vrai bloc par lui, ni lever par lui
  un drapeau lecture seule du bloc (BLKROSET, que le fs_mgr d'Android pose sur les partitions montees en ro), qui
  fait echouer `mount -o rw,remount` (« write-protected »). Le vrai bloc :
  `/dev/block/platform/bootdevice/by-name/system`, ou celui derriere `/sys/dev/block/<majeur:mineur>` de
  `/proc/self/mountinfo`.
- Methode sure (LibreHU Apps) : `blockdev --setrw` sur ce bloc, le monter une seconde fois dans un dossier prive,
  `mount -o remount,rw` de ce montage, ecrire dedans (sous-dossier `system/`), puis `remount,ro`, `umount`,
  `blockdev --setro`. Le montage d'Android reste en lecture seule, et on evite le tmpfs de Magisk sur
  `/system/priv-app`.
- Sous dm-verity (le bloc est esclave d'un `/dev/block/dm-*`), ne rien ecrire : Android ne demarrerait plus.
  Desactiver soi-meme : `fastboot --disable-verity flash vbmeta vbmeta.img`.

## Audio : zips TWRP (deep_buffer, pilote ViPER4Android sans Magisk)
Magisk ne tient pas dans le `boot` de l'unite (ramdisk trop gros) et la ROM est une build `user` (pas d'`adb root`) :
ces modifications audio passent par TWRP. `tools/ac8257/audio/twrp/build.sh [ViPER4Android-RE-vX.zip]` construit :
- `ujc201_nodeepbuffer_install.zip` / `_restore.zip` : retire la sortie `deep_buffer` (musique muette, voir
  [audio_deep_buffer.md](audio_deep_buffer.md)) du premier `audio_policy_configuration.xml` qu'Android 9
  utilise ; sauvegarde `<fichier>.nodeep-orig` + `/sdcard/UJC201_audio_backup/<date>/` ;
- `ujc201_viper_install.zip` / `_restore.zip` (si le module non-AIDL de
  [ViPERFX_RE](https://github.com/likelikeslike/ViPERFX_RE) est donne) : copie `libv4a_re.so` dans
  `/vendor/lib/soundfx` (et `lib64`) et declare l'effet (librairie `v4a_re`, UUID
  `90380da3-8536-4744-a6a3-5731970e640f`) dans les `audio_effects*.xml` / `.conf` de `/vendor/etc`, ceux qu'Android 9
  lit en priorite (ceux de `/system/etc` seulement si vendor n'en a pas) ; sauvegarde `<fichier>.v4a-orig` +
  `/sdcard/UJC201_viper_backup/<date>/`. L'appli ViPER4Android s'installe a part (en `/system/priv-app` pour le
  mode par appli sans root, voir LibreHU Apps). Avec KernelSU (Next), le module d'origine s'installe aussi tel quel
  depuis le gestionnaire, sans toucher aux partitions.

Zips prets a flasher : [LibreHU-flashablesZips](https://github.com/LibreHU/LibreHU-flashablesZips) (release `zips`,
reconstruite par la CI a chaque push et chaque jour : pilote ViPER, deep_buffer, derniere TWRP du depot android_device_alps_ac8257_demo), classes
par categorie et installables depuis l'onglet Zips de LibreHU Apps (flash TWRP automatique, ou image TWRP ecrite
directement dans `recovery` en root, avec sauvegarde).

Tous reinstallables (repartent de l'original sauvegarde). **dm-verity** : si la partition a modifier est verifiee
(drapeaux fs_mgr `avb` / `verify` du fstab du device tree) et que vbmeta ne l'a pas desactive (drapeau
`HASHTREE_DISABLED`), le zip s'arrete sans rien modifier : modifier une partition sous dm-verity empeche Android de
demarrer. Pour le desactiver soi-meme : sauvegarder VBMeta dans TWRP, puis
`fastboot --disable-verity flash vbmeta vbmeta.img` avec cette sauvegarde (pas `--disable-verification`).

## Applis LibreHU : root (SuperSU) et mises a jour
- **SuperSU est obligatoire** dans la ROM : LibreHU-service, LibreHU Launcher, LibreHU Apps, la radio FM,
  ViPER4Android, MCU Toolkit et Android Auto (Open Headunit LibreHU) passent par lui, et chaque demande doit etre
  accordee avec « Se souvenir ». L'assistant de premier demarrage du launcher le dit et propose **« Autoriser le
  root pour toutes les applis LibreHU »** : chaque appli installee demande le root l'une apres l'autre (diffusions
  `org.librehu.action.REQUEST_ROOT` / `ROOT_RESULT`, l'appli lance seulement `su -c id`), donc toutes les fenetres
  SuperSU arrivent une seule fois, au meme endroit.
- **Mises a jour** : LibreHU Apps lit la `release.txt` de la pre-version `latest-<branche>` de chaque appli (branche
  de l'edition choisie : `librehu-service`, `ivi` ou `main`) et propose la mise a jour quand son `versionCode`
  (nombre de commits de la branche) depasse celui installe. Un correctif pousse seulement sur `main` n'arrive dans
  une edition qu'une fois `main` fusionnee dans sa branche ; `tools/check_releases.py` du depot LibreHU-OTA-app
  verifie toutes les applis et toutes les branches.
- **Forks** : les modifications LibreHU des applis reprises d'autres projets sont listees fichier par fichier dans
  leur `README_LIBREHU.md` : [Open Headunit](https://github.com/LibreHU/LibreHU-open-headunit-app/blob/main/README_LIBREHU.md),
  [xcertplay](https://github.com/LibreHU/LibreHU-xcertplay-app/blob/master/README_LIBREHU.md),
  [ViPER4Android](https://github.com/LibreHU/LibreHU-ViPER4Android-app/blob/main/README_LIBREHU.md).
