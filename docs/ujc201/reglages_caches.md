# Reglages caches de la ROM stock UJC201

Trouves dans `priv-app/MtkSettings/MtkSettings.apk` (paquet `com.android.settings`, Reglages MediaTek modifies par
Autochips), `system/app` et la partition `jancar`. Ouvrir une activite depuis un PC :
`adb shell am start -n <paquet>/<activite>`.

## ATC Settings (Autochips), dans les Reglages Android
`adb shell am start -n com.android.settings/com.autochips.atcsettings.mainui.MainActivity`
(declaree `MAIN` / `LAUNCHER`, mais pas affichee par le lanceur Jancar). Quatre onglets :

| Onglet | Reglages | Stockage |
|---|---|---|
| Ecran (`DisplayFragment`) | source de la camera de recul (CVBS / MIPI-AHD), image arriere, AVM rapide, AVM sur CVBS, nettete, tramage (dither), retro-eclairage, **recadrage de l'image de recul** (x, y, largeur, hauteur, `BackcarFragment`), miroir, « generate MDP » (`BackCar.updateArm2MdpSetting`) | **metazone** (lue par le LK et l'ARM2 du recul rapide) |
| Audio (`AudioFragment`, `AtcAudioSettings`) | graves / medium / aigus (niveau, frequence centrale, Q), loudness (frequence, accentuation des aigus, attenuation), balance / fader (attenuateurs par voie), volume pendant le recul | `com.autochips.audiosettings.AudioSettings` (puce audio des cartes Autochips de reference ; sur l'UJC201 la puce est un BD37534 pilote par ivi-services : effet reel a verifier) |
| Logo (`LogoFragment`) | logo de demarrage depuis une cle USB / la memoire (redimensionne a l'ecran) | partition logo / metazone |
| Autre (`OtherFragment`, `SensorBusFragment`) | mode USB (hote / peripherique, active adb), **Engineer Mode**, mode avion du demarrage rapide (QB), mode capteur AHD / AVM | `Settings.Global`, metazone |

Adresses metazone (`AtcMetazone.readval/writeval`, puis `flush`) :

| Adresse | Valeur |
|---|---|
| 65550 | ecran arriere (0 / 1) |
| 65587 / 65589..65593 | rotation / zone d'affichage et direction forcee (orientation de l'ecran) |
| 65588 / 65600 | miroir (selon la source : CVBS / AHD), 1 gauche-droite + 2 haut-bas |
| 65594..65597 / 65601..65604 | recadrage x, y, largeur, hauteur (selon la source) |
| 65598 | nettete |
| 65599 | tramage |
| 65605 | mode camera (0 auto, 1 NTSC, 2 PAL, 3 720p25, 4 720p30, 5 1080p25, 6 1080p30 ; ATC Settings ne propose que 4 / 6) |
| 65606 | mode camera AVM (0 auto, 1 NTSC, 2 PAL, 3 720p25, 4 720p30, 5 1080p25, 6 1080p30) |
| 65607 | source du recul (0 debug, 1 CVBS, 2 MIPI, 3 360 / AVM : l'AHD de l'UJC201 passe par 3 selon Jancar) |
| 66018 | valeur capteur Jancar (0 AHD 720p25, 1 1080p25, 2 720p30, 3 1080p30) |
| 65608 | AVM rapide |
| 65611 | AVM sur CVBS |
| 65616 | mode USB du port 0 (2 hote, 1 peripherique) |
| 65617 / 65618 | protocole des ports USB 0 / 1, amelioration DVR (bits 0 / 1) |
| 65634 | mode capteur AHD |
| 65635 | mode capteur AVM |

Format de la camera selon Jancar (`ImportSingleConfigUtil.importBackCarVideoSource`, `BackCarVideoSource` de
`ivi-customer.ini`) : 65607 = 3, 66018, 65605 = 65606 = index + 2, recadrage AHD 65601..65604, puis redemarrage ;
detail dans [LibreHU-service/docs/atc.md](https://github.com/LibreHU/LibreHU-service/blob/main/docs/atc.md#format-de-la-caméra).

Logo : `AtcLogo.writelogo(2, 0, taille, donnees)`, pixels BGRA a la taille reelle de l'ecran, ligne du haut en
premier, alpha 0xFF. USB : `UsbManager.setPortRoles(port, 1, role)` (role de donnees 1 hote, 2 peripherique) apres
l'ecriture de 65616.

**Dans LibreHU-service** (Avance → Autochips, sans ATC Settings) : mode USB, reglages du recul rapide de l'ARM2 puis
envoi a l'ARM2, logo de demarrage (avec copie prealable de la partition logo), boutons vers Engineer Mode / ATC
Settings / ATCLogger : [LibreHU-service/docs/atc.md](https://github.com/LibreHU/LibreHU-service/blob/main/docs/atc.md).

## Engineer Mode MediaTek
`adb shell am start -n com.mediatek.engineermode/.EngineerMode` (`system/priv-app/EngineerMode`), aussi via l'onglet
Autre d'ATC Settings. Menus materiel dont **Audio** (volumes par mode et par sortie, amelioration de la parole,
journaux du HAL) : utile pour l'etude de l'audio, a manier avec precaution (les valeurs vont dans les parametres
NVRAM / `audio_param`).

## ATCLogger
`com.autochips.atclogger/.MainActivity` (`system/app/ATCLogger`) : journaux Android / noyau / modem sur la
carte memoire.

## Configuration Jancar (partition `jancar`, `/jancar/config`)
`ivi-customer.ini` (options client : `CameraType`, `BackCarVideoSource`, volumes par defaut et de recul…),
`ivi-config.ini` (index des cameras Android, canaux AV / audio, courbe de volume `BD37534VolumeCurve`, touche MODE,
volume CCD), `ivi-settings.ini` (reglages Jancar et leur visibilite : `CCDCamera`, `CCDCameraLine` lignes de recul,
miroirs, tension CCD…), `can_config.json`, `ivi-radio-cfg.ini`, `pointercal-*.xml` (calibration tactile).
Voir aussi [LibreHU-service/docs/ivi-services/10-config.md](https://github.com/LibreHU/LibreHU-service/blob/main/docs/ivi-services/10-config.md).
