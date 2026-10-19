# UJC201 (Jancar, Autochips AC8257) : notes de l'appareil

Analyses de l'autoradio et de sa ROM stock, utiles au noyau et a LibreHU. Deplacees le 19/10/2026 depuis le depot
TWRP [android_device_alps_ac8257_demo](https://github.com/LibreHU/android_device_alps_ac8257_demo), qui ne garde que
l'arbre TWRP (historique des fichiers dans ce depot-la).

| Fichier | Sujet |
|---|---|
| [kernel_gpl.md](kernel_gpl.md) | sources GPL du noyau envoyees par SIXWIN / Jancar (19/10/2026) : contenu, comparaison au noyau stock #72, ce qui manque, sources de remplacement ; branche [`autochips-gpl`](https://github.com/LibreHU/android_kernel_autochips_ac8257_4.9/tree/autochips-gpl) |
| [tester_un_noyau.md](tester_un_noyau.md) | partition boot (footer AVB), `para` (misc), RAM sous 4 Go, essais rates, compteur de recovery |
| [ethernet.md](ethernet.md) | Ethernet USB et partage de connexion : pilotes du noyau stock, modules ajoutes (`tools/ac8257/usbnet/`), zip `usb-net` |
| [audio_stock.md](audio_stock.md) | chaine audio de la ROM stock (politique, HAL MediaTek et Autochips, effets, FM, deep_buffer) |
| [audio_deep_buffer.md](audio_deep_buffer.md) | musique muette : sortie `deep_buffer` retiree (`tools/ac8257/audio/`) |
| [video_stock.md](video_stock.md) | CVBS IN 1 / 2 (GPIO 167 / 165, camera 3), VIDEO OUT (puce USB MacroSilicon), AUDIO OUT |
| [projection_stock.md](projection_stock.md) | CarPlay, Android Auto, SpeedPlay, CarbitLink, Jlink de la ROM stock ; gain micro et ECNR |
| [reglages_caches.md](reglages_caches.md) | ATC Settings (Autochips) cache, Engineer Mode, adresses metazone, config Jancar |
| [rom_android.md](rom_android.md) | /system en ecriture (system-as-root, dm-verity), zips TWRP audio, applis LibreHU (root, mises a jour) |

Firmware et protocole du MCU : [MCU-tools-app/docs/mcu_firmware.md](https://github.com/LibreHU/MCU-tools-app/blob/main/docs/mcu_firmware.md).

Outils (`tools/ac8257/`) : `usbnet/` (modules reseau USB, verification des CRC stock), `audio/` (deep_buffer par adb
root, zips TWRP deep_buffer et pilote ViPER4Android), `dump/collecte_audio_camera.sh` (extraction d'un dump stock).

Securite : ne pas publier ses propres partitions `nvram`, `nvdata`, `proinfo`, `persist`, `metazone` (identifiants,
certificat CarPlay). Rien n'est ecrit sous dm-verity ; le desactiver soi-meme :
`fastboot --disable-verity flash vbmeta vbmeta.img`.
