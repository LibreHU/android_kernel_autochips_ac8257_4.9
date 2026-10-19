# Audio de la ROM stock UJC201 (AC8257, Android 9)

Analyse du dump stock (`system` + `vendor`, extraits par `tools/ac8257/dump/collecte_audio_camera.sh`) et des notes
d'ivi-services ([LibreHU-service/docs/ivi-services/05-audio.md](https://github.com/LibreHU/LibreHU-service/blob/main/docs/ivi-services/05-audio.md)).
[D] = lu dans le dump, [N] = notes ivi-services (verifie sur l'unite), [?] = a confirmer sur l'unite.

## Chaine complete
```
applis ─► AudioFlinger (system) ─► politique : vendor/etc/audio_policy_configuration.xml
        ─► HAL audio HIDL 4.0 (service vendor.audio-hal-4-0, user audioserver)
           android.hardware.audio@4.0-impl-mediatek ─► audio.primary.ac8257.so (HAL MediaTek « common/V3 » + ajouts Autochips)
        ─► ALSA (tinyalsa) : PCM choisi par nom (Deep_Buffer_PLayback, FM_I2S_Playback, BTCVSD_Playback…)
        ─► I2S du SoC ─► BD37534 (i2c-6 @0x40, entree 11 « PC/Android ») ─► ampli (mute GPIO 166)
radio (tuner) ─► BD37534 entree 3   AUX/AV ─► BD37534 entree 0
```
- Android ne sort qu'en **stereo 48 kHz 16 bits** ; volume, tonalite, fader 4 voies et caisson sont dans le BD37534
  [N]. Le flux media Android reste a niveau fixe (`persist.action.volume.percent`, 40 %), le volume de l'utilisateur
  est applique dans la puce [N].

## Politique audio [D]
Android 9 lit le premier fichier trouve dans `/odm/etc`, `/vendor/etc`, `/system/etc` : ici
**`/vendor/etc/audio_policy_configuration.xml`** (celui de `/system/etc` est le modele AOSP generique, inutilise).
- Module `primary` (`halVersion 3.0`) : sorties **`primary output`** (`PRIMARY|FAST`, 48 kHz stereo 16 bits) et
  **`deep_buffer`** (`DEEP_BUFFER`) ; entree `primary input`.
- Peripheriques attaches : `Speaker` (sortie par defaut), `Built-In Mic`, `Built-In Back Mic`, `FM Tuner In`,
  `Voice Call In`. Aussi `USB Primary Out/In` (`AUDIO_DEVICE_OUT/IN_BUS`, adresse `usb_phone_call`) : appels par
  telephone USB (CarPlay / Android Auto).
- Routes : `Speaker`, casques ← `primary output`, `deep_buffer`, `FM Tuner In` ; BT SCO, AUX, dock, FM Tuner Out,
  USB ← `primary output` seulement.
- Inclus : `a2dp_`, `usb_`, `r_submix_audio_policy_configuration.xml`, `audio_policy_volumes.xml`,
  `default_volume_tables.xml`.

## HAL audio MediaTek [D]
- Service `vendor.audio-hal-4-0` (`/vendor/bin/hw/android.hardware.audio@4.0-service-mediatek`, `class hal`, user
  `audioserver`, groupes `audio media system`…) ; `/data/vendor/audiohal` pour les vidages.
- `audio.primary.ac8257.so` (32 et 64 bits) : gestionnaires `AudioALSAPlaybackHandlerNormal / Fast / Offload /
  Mixer / BTSCO / BTCVSD / FMTransmitter / HDMI / Usb / Voice`, PCM par nom :
  `Deep_Buffer_PLayback`, `DL1SCPSPK_PLayback`, `FM_I2S_Playback` / `_Capture`, `FM_MRGTX_Playback`,
  `BTCVSD_Playback` / `_Capture`, `DL1_AWB_Record`, `HMDI_PLayback`… La correspondance nom → numero de PCM vient de
  `/proc/asound/pcm` [?].
- Commandes de mixage : `audio_device.xml` ne decrit que l'analogique (amplis `Speaker_Amp_Switch`,
  `Audio_Amp_L/R_Switch`, `Ext_Speaker_Amp_Switch`, micros ADC / preamp) ; le routage des memif (DL1, DL2…) vers
  l'I2S est fait par le pilote ASoC du noyau a l'ouverture du PCM. Controles notables du HAL :
  `Audio_DL2_DataTransfer`, `Audio_i2s0_hd_Switch`, `Audio_I2S1_Setting`, `Audio FM I2S Volume`.
- `aurisys_config.xml` : `mtk_bessound` sur `PLAYBACK_NORMAL` et `PLAYBACK_LOW_LATENCY`, `mtk_speech_enh` sur les
  enregistrements et la VoIP ; pas de scenario `DSP_DEEP_BUF`. `audio_param/` : parametres de reglage MTK
  (`AudioParamOptions.xml`, volumes, egaliseurs…).
- `build.prop` vendor : `vendor.audiohal.useplc=1`, ALAC / APE, `ro.atc.carplay.audio_duck_pct=25`.

## Effets [D]
- `/vendor/etc/audio_effects.xml` (celui qu'Android 9 utilise) : librairies AOSP de `/vendor/lib[64]/soundfx`
  (`bundle` : bassboost, virtualizer, equalizer, volume ; `reverb` ; `visualizer` ; `downmix` ;
  `loudness_enhancer` ; `dynamics_processing` ; `pre_processing` : agc, aec, ns). Pas de ViPER dans la ROM.
- Les effets tournent dans le processus du HAL (`android.hardware.audio.effect@4.0-impl.so`, vendor) : un pilote
  d'effet tiers doit donc etre dans `/vendor/lib[64]/soundfx` (espace de noms du linker et SELinux vendor),
  voir `tools/ac8257/audio/twrp/` pour ViPER.

## HAL audio Autochips [D]
`vendor.autochips.hardware.audio@1.0-service` (service `atc_hal_audhw`, user `system`), interfaces
`IAudioAuxIn` et `IAudioEffect` declarees dans `vendor/etc/vintf/manifest.xml`. Son source est
`vendor/autochips/proprietary/hardware/audio_tda7729/` : ecrit pour les cartes a **ST TDA7729**. Cette carte a un
**BD37534**, pilote par ivi-services (JNI `com.jancar.services.jni.AudioDevice`) ou LibreHU-service : role reel ici
[?] (probablement inactif).

## Radio FM [D]
Puce combo MediaTek, `/dev/fm` (`vendor/lib/modules/fmradio_drv.ko`) ; le HAL route le son par I2S
(`WCNChipController`, `FM_I2S_Playback` / `_Capture`, `FM_IOCTL_I2S_SETTING`, modes maitre / esclave 32 / 44 / 48 kHz).
Dans la politique : `FM Tuner In` (attache) et `FM Tuner Out`. Appli stock : `com.jancar.radio` (`vendor/app/ivi-radio`),
emetteur FM : `vendor/app/ivi-FmTransmit`.

Configuration de la bande : `/system/lib64/libfmcust.so` (`CUST_get_cfg`, desassemble) donne la puce `0x6631` quand
`persist.vendor.connsys.chipid` vaut `0x8257`, la bande US / Europe en **pas de 10 kHz** (8750 a 10800), pas de
recherche 100 kHz, 80 stations au plus, seuil RSSI -102. Une appli FM installee en priv-app charge cette bibliotheque ;
la mise sous tension du pilote se fait pourtant en pas de 100 kHz (1000 = 100 MHz) : comparer la frequence a la bande
dans la meme unite (cause du « La puce FM n'a pas demarre » de LibreHU FM, journal `FMR_pwr_up error freq: 1000`, et
dans `dmesg` : `/dev/fm` ouvert, identifiant lu, ferme sans `POWERUP`). SELinux est permissif sur la ROM stock
(`avc: denied { ioctl } ... permissive=1`).

## Bluetooth [D]
Appels : PCM `BTCVSD_Playback` / `_Capture` (CVSD / mSBC par le SoC) ; A2DP en sortie via le module `a2dp`.
La musique d'un telephone (A2DP sink) passe par la pile Bluetooth puis AudioFlinger [?] (`MtkBluetooth`,
`ivi-bt` / `ivi-btservice` de Jancar).

## deep_buffer muet : ce qu'on sait et ce qui reste
- Le HAL ouvre le PCM nomme `Deep_Buffer_PLayback` pour la sortie `deep_buffer`, ou passe la musique par defaut ;
  sonneries et sons systeme passent par `primary output` (PCM normal / fast) et s'entendent.
- Rien dans la configuration (`audio_device.xml`, `aurisys`) ne differencie les deux chemins cote mixage : la
  difference est dans le pilote ASoC (frontal deep buffer, memif DL2 et son raccordement a l'I2S vers le BD37534).
- **A confirmer sur l'unite** : (1) le probleme existe-t-il avec le noyau stock et ivi-services ? (2)
  `cat /proc/asound/pcm` (numero de `Deep_Buffer_PLayback`) ; (3) `tinymix` pendant une musique et pendant une
  sonnerie, a comparer. Commandes : [audio_deep_buffer.md](audio_deep_buffer.md).

## ViPER4Android et routage : ce que confirme l'analyse
| Point | Conclusion | Base |
|---|---|---|
| Ou mettre le pilote ViPER | `/vendor/lib[64]/soundfx/libv4a_re.so` + declaration dans `/vendor/etc/audio_effects.xml` : c'est ce que fait `tools/ac8257/audio/twrp/viper_install.sh` | les effets tournent dans le processus du HAL vendor [D] |
| L'appli ViPER dans `/system/priv-app` | ne remplace pas le pilote : elle ne fait que piloter l'effet (et obtenir ses permissions) | idem |
| Fichier de politique a modifier | `/vendor/etc/audio_policy_configuration.xml` (celui de `/system/etc` est ignore) : c'est celui que modifie `nodeep_install.sh` | ordre de lecture d'Android 9 [D] |
| ViPER en mode global (session 0) | suit la sortie de la musique : sur `deep_buffer` tant qu'elle existe (donc muet avec elle), sur `primary` une fois `deep_buffer` retiree | politique [D] + essais (`audio_deep_buffer.md`) |
| Traitements empiles | Android → (MTK BesSound sur `PLAYBACK_NORMAL` / `LOW_LATENCY`) → ViPER → I2S → BD37534 (volume, graves / mediums / aigus, loudness, fader). Avec l'egaliseur de ViPER, laisser la tonalite et le loudness de la puce a plat pour eviter double correction et saturation | aurisys [D], puce [N] |
| Niveau numerique | le flux media Android reste a niveau fixe (`persist.action.volume.percent`), le volume est dans la puce : la marge de ViPER ne depend pas du volume | [N] |
| AUX avec Android (LibreHU-service) | le son d'Android, deja traite par ViPER, arrive par l'entree de mixage mono de la puce pendant l'AUX | `AudioBD37534::setMixer` / `setMonoVolume` (libJanCarIVI) |
| Cause du silence de `deep_buffer` | **non confirmee** : piste la plus concrete `Audio_DL2_DataTransfer` (memif DL2), voir `audio_deep_buffer.md` | a mesurer sur l'unite [?] |

## Fichiers du dump utiles
`vendor/etc/audio_policy_configuration.xml`, `audio_device.xml`, `audio_effects.xml`, `aurisys_config.xml`,
`audio_param/`, `vendor/etc/init/android.hardware.audio@4.0-service-mediatek.rc`,
`vendor.autochips.hardware.audio@1.0-service.rc`, `vendor/lib[64]/hw/audio.primary.ac8257.so`,
`vendor/etc/vintf/manifest.xml`.

## Micro [D]
Gain materiel `MIC1_HW_GAIN` / `MIC2_HW_GAIN` = 3, 4 ou 5 (`AudioManager.setParameters`, `AudioALSAHardware`),
garde dans `persist.jancar.mic.hw.gain` (`MIC1_HW_GAIN=3` sur l'unite) ; ECNR par
`persist.audiohal.third_ecnr_lib.enable` / `vendor.audiohal.third_ecnr_lib.enable`. Details et reprise par
LibreHU-service : [projection_stock.md](projection_stock.md#micro--gain-et-annulation-decho-reglages-usine).

