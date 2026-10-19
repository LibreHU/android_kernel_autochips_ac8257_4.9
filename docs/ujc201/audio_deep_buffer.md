# Musique muette, sonneries OK : retirer la sortie `deep_buffer`

Chaine audio complete de la ROM stock : [audio_stock.md](audio_stock.md).

## Symptome
Sur l'UJC201 (AC8257, Android 9), toute la musique est muette (lecteur, Android Auto, radio FM), alors que les
sonneries et les clics du clavier s'entendent. ViPER4Android coupe ou non : aucun changement.

## Diagnostic (`dumpsys media.audio_flinger`)
Android ouvre deux sorties vers la meme carte son :

| Sortie | PCM ALSA | Blocs | Utilisee pour | Resultat |
|---|---|---|---|---|
| `primary` (`AUDIO_OUTPUT_FLAG_PRIMARY\|FAST`) | `pcm0` MultiMedia1 | 1024 trames | sonneries, clavier | son OK |
| `deep_buffer` (`AUDIO_OUTPUT_FLAG_DEEP_BUFFER`) | `pcm3` MultiMedia2 | 2048 trames | musique (par defaut) | muet |

Les trames sont bien ecrites et `pcm3p` est `RUNNING` : le son se perd apres Android, sur le chemin MultiMedia2.

`deep_buffer` ne sert qu'a economiser la batterie (gros blocs, moins de reveils CPU) au prix de la latence
(commits AOSP : [deep audio buffers](https://gerrit.omnirom.org/plugins/gitiles/android_frameworks_av/+/1948eb3ea6eee336e8cdab9b0c693f93f5f19993%5E%21),
[deep buffer par defaut pour la musique](https://android.googlesource.com/platform/frameworks/av/+/439e4ed)).
Inutile sur un autoradio.

## Contournement
Sans `deep_buffer` dans `audio_policy_configuration.xml`, la musique passe par `primary`. Les effets globaux
(ViPER4Android, session 0) suivent la sortie de la musique et fonctionnent sur `primary`.

```
tools/ac8257/audio/no_deep_buffer.sh apply     # sauvegarde (/sdcard/librehu-audio + copie locale), modifie, verifie
adb reboot
tools/ac8257/audio/no_deep_buffer.sh status    # deep_buffer : 0, une seule sortie (AudioOut_D) dans audio_flinger
tools/ac8257/audio/no_deep_buffer.sh restore   # retour au fichier d'origine
```
Il faut `adb root` et `/vendor` inscriptible (`adb disable-verity`, redemarrage, `adb remount`).
Le fichier est reecrit sur place (`cat > fichier`) pour garder son label SELinux.

Sans `adb root` (ROM `user` d'origine) : zips TWRP `ujc201_nodeepbuffer_install.zip` / `_restore.zip`
(`tools/ac8257/audio/twrp/build.sh`, voir le README) ; ils s'arretent sans rien modifier si `/vendor` est sous dm-verity.

Effet de bord : traitement par blocs de 1024 trames, donc un peu plus de charge CPU pour les effets lourds
(Convolver avec un long IRS, reverberation).

## Cause racine : non identifiee
Retirer `deep_buffer` est un **contournement** : la sortie est facultative (economie d'energie, inutile sur un
autoradio alimente), mais on ne sait pas encore **pourquoi** MultiMedia2 est muet. Pistes, du plus probable au moins :
1. Routage ALSA : chez MediaTek, chaque frontal (MultiMedia1 = memif DL1, MultiMedia2 = DL2) doit etre relie a la
   sortie (I2S / codec) par des controles de mixage DAPM que le HAL pose a l'ouverture du flux (chemins decrits dans
   `/vendor/etc/audio_device.xml` du HAL MTK). Si DL2 n'est jamais relie, les trames sont lues (`RUNNING`) mais
   n'arrivent nulle part.
2. Noyau reconstruit : si le symptome n'existe qu'avec le noyau LibreHU, c'est un manque du pilote ASoC (route DL2).
3. Pile Jancar : `ivi-services` (absent avec LibreHU-service) poserait ces controles lui-meme.

Pour trancher (root, pendant la lecture d'une musique) :
```
cat /proc/asound/pcm; cat /proc/asound/card0/pcm3p/sub0/status    # DL2 RUNNING, pointeurs qui avancent
tinymix > /sdcard/mix_deep.txt                                     # avec deep_buffer
# idem apres retrait de deep_buffer (musique sur pcm0) -> /sdcard/mix_primary.txt, puis comparer les deux
grep -n -i "dl2\|deep\|MultiMedia2" /vendor/etc/audio_device.xml
tinymix "Audio_DL2_DataTransfer"                                   # controle du HAL MTK (voir audio_stock.md)
```
Piste concrete : le HAL MediaTek de cette ROM contient un controle `Audio_DL2_DataTransfer` (lu dans
`audio.primary.ac8257.so`). Si sa valeur est `Off` pendant une musique sur `deep_buffer`, essayer
`tinymix "Audio_DL2_DataTransfer" On` : si le son revient, c'est la cause (a confirmer, non verifie sur l'unite).
et noter si le probleme existe avec le noyau stock. Si un controle manque, le vrai correctif est de le poser
(chemin du HAL, ou au demarrage), et `deep_buffer` peut rester.
