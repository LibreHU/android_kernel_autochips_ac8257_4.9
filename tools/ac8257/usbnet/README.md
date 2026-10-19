# tools/usbnet : pilotes reseau USB pour le noyau stock

Modules `.ko` des pilotes reseau USB absents du noyau stock Jancar (Linux 4.9.117+, #25 et #72) : Realtek
RTL8152 / RTL8153, RNDIS (partage de connexion USB d'un telephone Android), iPhone (`ipheth`), LAN78xx, SMSC,
DM9601, SR9700 / SR9800, MCS7830, CH9200, RTL8150, Pegasus, CDC EEM. Analyse et usage : [docs/ethernet.md](../../../docs/ujc201/ethernet.md).

| Fichier | Role |
|---|---|
| `build.sh` | construit tout : `tools/ac8257/usbnet/build.sh <android_kernel_autochips_ac8257_4.9> <dossier>` → `<dossier>/modules/*.ko`, `SHA256SUMS`, `CHECK.txt` |
| `Kbuild` | liste des modules (`rndis_host` = `rndis_host_main.o` + `cdc_rndis_bind.o`) |
| `make_rndis.py` | ecrit `cdc_rndis_bind.c` (bind CDC de `cdc_ether.c` avec RNDIS active, non exporte) et `rndis_host_main.c` (l'utilise, interfaces `eth*`) |
| `check_ko.py` | verifie chaque module : vermagic stock, chaque import present dans `stock-Module.symvers` avec le meme CRC |

Prerequis : clang, binutils AOSP GCC 4.9 aarch64 (`aarch64-linux-android-`) dans le `PATH`, `llvm-strip`. Les sources
des pilotes viennent de l'arbre du noyau (Linux 4.9, GPL-2.0). Les modules construits sont publies dans le zip
`usb-net` du depot LibreHU-flashablesZips (`zips/kernel/usb-net`).
