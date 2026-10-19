# Video de la ROM stock : CVBS IN 1 / 2, VIDEO OUT, AUDIO OUT

Analyse de la ROM Jancar (UJC201, AC8257) pour les RCA du faisceau : **CVBS IN 1**, **CVBS IN 2** (video seule),
**VIDEO OUT** et **AUDIO OUT**. Mise en oeuvre sans les applis Jancar : LibreHU-service, Vehicule -> Video
(`docs/video.md` de LibreHU-service).

## Entrees CVBS
- Un seul decodeur CVBS, camera Android `3` (`/jancar/config/ivi-config.ini`, `[CameraIndex]` : `AV=3`, `REAR=3`,
  `FRONT=11`, `TV=6`).
- Commutateur video analogique devant le decodeur : ivi-services
  `Platform_AutoChips_8257_Base.platformSelectInput(avId)`, GPIO `167` (SW1) et `165` (SW2) :
  AV / TV -> 0/0, FRONT -> 1/0, REAR -> 1/1 ; 0/1 n'est jamais ecrit ; AV2 (`avId 2`) n'a pas de cas sur l'AC8257.
  `[VideoChannelConfig]` (vide ici) permettrait de renvoyer un `avId` sur un autre (`IVIConfig.getConfigVideoChannel`).
- Appli AV `com.jancar.avin` (`/vendor/app/ivi-input`) : `setAndroidCameraOpenPrepared(1)` (-> `selectInput`), camera
  3, taille selon le type CVBS (`[AVIn] DefaultCVBSType`). Si la camera de recul est CVBS
  (`ivi-customer.ini BackCarVideoSource=0`), elle lache la camera en marche arriere, met REAR (32), puis reprend ;
  propriete `persist.avin.frontavin` pendant ce temps (lue par `com.jancar.hd2cam`).
- Video bloquee frein desserre (`IVICore.isMediaVideoPermit`).
- Son : `IVIAVIn.Id.getAudioChannel` AV -> 25, AV2 -> 26 ; `platformSwitchAudioTo(25)` -> BD37534 entree
  `mAVPhyAudioChannel = 0`, la meme que l'AUX (`Platform_AutoChips_8257_37534`). Les RCA CVBS IN de cette unite
  n'ont pas de son.

## VIDEO OUT
Verifie avec l'APK `ivi-usbdisplay` (`com.jancar.usbdisplay` 2.1.6, fourni depuis l'autoradio, decompile) : VIDEO OUT
est une **puce d'affichage USB MacroSilicon** (USB `534d:6021` ou `345f:9132`, sortie CVBS PAL / NTSC), alimentee
par une capture d'ecran, pas une sortie du SoC.
- `Settings.Global key_rear_display = 1` : `MainService` (appli persistante, uid system) lance `CaptureService` au
  demarrage ; `UsbReceiver` le lance quand la puce apparait.
- `CaptureService` : attend la sortie de l'ARM2 (`BackCar.getArm2Status() == 2`), trouve la puce (USBMonitor, point
  d'acces 0x04), `MediaProjectionActivity` (capture accordee d'office, `CAPTURE_VIDEO_OUTPUT`), envoi USB bulk ;
  `change_rear_display` (extra `change`) demarre / arrete, `runtime.reardisplay.status` 54 = envoi, 55 = arret.
- Reglages : `SettingActivity` (exportee) -> `/jancar/config/usb_display.json` (qualite, dimension, decalages,
  echelle).
- Non lus par cette appli : metazone `65550` (« Rear display » d'ATC Settings, `hwc.external_display_*` du
  compositeur) et `hwc.rear_disp_*` (`RearOutputActivity` d'ivi-settings, puis `SettingsService.setRearDisplay` ->
  `CaptureService`).

## AUDIO OUT
Sortie ligne du BD37534 (meme son que les haut-parleurs). Aucun reglage dedie dans ivi-settings / ivi-audio-settings
(seulement « Digital audio output », SPDIF).
