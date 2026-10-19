# Ethernet et partage de connexion USB sur l'UJC201 (AC8257, Android 9)

Pas de port Ethernet sur la carte : le reseau filaire passe par un **adaptateur USB Ethernet** ou par un **telephone
en partage de connexion USB**, sur un port USB en mode hote.

## Releve sur l'unite (firmware Jancar de janvier 2026)
```
$ pm list features | grep -iE "usb.host|ethernet"
feature:android.hardware.usb.host
$ dumpsys ethernet
  EthernetTracker
  Ethernet interface name filter: eth\d
  ...
  Tracking interfaces:            (aucune)
$ lsusb                            (adaptateur branche)
Bus 002 Device 002: ID 0bda:8153   Realtek RTL8153
$ ping 1.1.1.1
connect: Network is unreachable
```
- **Android** : `EthernetService` tourne (demarre par `SystemServer` des que la feature `android.hardware.usb.host`
  ou `android.hardware.ethernet` est declaree) et prend les interfaces `eth\d` en DHCP.
- **Noyau** : l'adaptateur Realtek RTL8153 n'a pas de pilote, donc pas d'interface `eth0`.

## Noyau stock : pilotes presents
Noyaux stock **#25** (250718) et **#72** (mise a jour de janvier 2026) : meme configuration (`IKCONFIG`, comparee
octet par octet), memes 9674 symboles exportes avec les memes CRC (`stock_symvers.py` de la source reconstruite).

| Puce / usage | Pilote | Noyau stock | Avec le zip `usb-net` |
|---|---|---|---|
| ASIX AX88179 / AX88178A (USB 3, gigabit) | `ax88179_178a` | ✅ integre | ✅ |
| ASIX AX88772 / AX8817x (USB 2, 100 Mb/s) | `asix` | ✅ integre | ✅ |
| Adaptateurs CDC ECM / NCM generiques | `cdc_ether`, `cdc_ncm` | ✅ integre | ✅ |
| **Realtek RTL8152 / RTL8153** (la plupart des adaptateurs) | `r8152` | ❌ (et `cdc_ether` 4.9 refuse expres `0bda:8152` / `8153` en ECM) | ✅ |
| **Telephone Android en partage USB** (RNDIS) | `rndis_host` | ❌ | ✅ (interface nommee `eth*`) |
| **iPhone en partage USB** | `ipheth` | ❌ | 🟡 experimental |
| Microchip LAN7800 / LAN7850 | `lan78xx` | ❌ | ✅ |
| SMSC LAN95xx / LAN7500 | `smsc95xx`, `smsc75xx` | ❌ | ✅ |
| Davicom DM9601, CoreChips SR9700 / SR9800, MosChip MCS7830, QinHeng CH9200, RTL8150, Pegasus, CDC EEM | divers | ❌ | ✅ |

## Les modules (`tools/ac8257/usbnet/`)
- Compiles hors arbre contre la source reconstruite (`android_kernel_autochips_ac8257_4.9`) configuree avec la
  configuration stock, avec clang ; `modpost` prend les CRC du noyau stock (`reference/stock-Module.symvers`).
- Verifies (`check_ko.py`) : vermagic `4.9.117+ SMP preempt mod_unload modversions aarch64` et chaque symbole importe
  present dans le noyau stock avec ce CRC. Les CRC de la source reconstruite ne servent pas de preuve de disposition
  (textes d'en-tetes differents, voir sa doc) : les structures utilisees ont ete comparees dans le code machine du
  noyau stock : `netdev_priv()` = +0x880 (`sizeof(struct net_device)`, `usbnet_open`), `struct usbnet` = 0x260
  (`alloc_etherdev_mqs` de `usbnet_probe`), `sk_buff` = 0xe8, identiques dans les modules.
- **RNDIS** : le `cdc_ether` integre est compile sans `CONFIG_USB_NET_RNDIS_HOST`, son
  `usbnet_generic_cdc_bind()` refuse donc les fonctions RNDIS (« bad CDC descriptors »). `make_rndis.py` joint a
  `rndis_host.ko` une copie privee de cette fonction avec RNDIS active (non exportee, pas de conflit), et retire
  `FLAG_POINTTOPOINT` (qui ne sert qu'au nom) pour que l'interface s'appelle `eth*` : Android la prend comme un reseau
  filaire, avec le DHCP du telephone.
- **iPhone** : Linux choisit la premiere configuration de l'iPhone (transfert de photos), sans l'interface de partage
  (classe ff / sous-classe fd / protocole 1) ; sur un ordinateur, `usbmuxd` la change. LibreHU-service choisit la bonne
  configuration d'apres les descripteurs (`bConfigurationValue`, root), sur demande ou au branchement (option
  desactivee par defaut : CarPlay filaire utilise une autre configuration). iOS peut aussi exiger que l'autoradio soit
  « approuve » (appairage `usbmuxd` / lockdown, absent ici) : non garanti.
- Construction : `tools/ac8257/usbnet/build.sh <source du noyau> <dossier>` (reproductible : memes octets d'un build a
  l'autre).

## Installation : zip flashable `usb-net`
Depuis LibreHU Apps → Zips → **Noyau** → « Ethernet USB et partage de connexion USB » (depot LibreHU-flashablesZips),
flashe avec TWRP ; dm-verity desactive sur `/vendor` (sinon le zip s'arrete sans rien modifier).
- Modules dans `/vendor/lib/modules/librehu-usbnet/`, script `/vendor/etc/init/init.librehu_usbnet.rc`, comme les
  modules stock (`init.bt_drv.rc`, `init.wmt_drv.rc`).
- Chargement une fois Android demarre (`sys.boot_completed=1`), avec une protection : `persist.vendor.librehu.usbnet.loading`
  vaut 1 pendant le chargement ; si l'autoradio redemarre pendant ce temps, le demarrage suivant les laisse de cote
  (`persist.vendor.librehu.usbnet.off=1`) jusqu'a reactivation (LibreHU-service, ou `setprop persist.vendor.librehu.usbnet.off 0`).
  Charger tout de suite : `setprop vendor.librehu.usbnet.arm 1`.
- Retrait : zip `usb-net-restore`.

## Compatibilite ROM stock et noyau stock
- **Noyau** : modules pour le noyau stock Jancar 4.9.117+ (#25 et #72 ; aussi le #72 patche `want_initramfs` du menu de
  demarrage et de TWRP v7, meme binaire a 4 octets pres, et la reconstruction LibreHU qui exporte les CRC stock).
  Pas de noyau a flasher.
- **ROM stock** : rien a remplacer ; ajout de fichiers dans `/vendor` (pas dans `/system`), comme les modules stock.
  Il faut seulement TWRP et dm-verity desactive sur `/vendor` (`androidboot.veritymode=disabled` dans la ligne de
  commande relevee sur l'unite).
- **SELinux** (la ligne de commande stock ne le passe pas en permissive) : la politique stock autorise `init` et
  `vendor_init` a charger des modules etiquetes `vendor_file` (`allow vendor_init vendor_file (system (module_load))`
  dans `vendor_sepolicy.cil`) ; le zip pose ce label. Les proprietes du script sont en `vendor.*` /
  `persist.vendor.*` (contexte `vendor_default_prop`), les seules que `vendor_init` peut ecrire (`sys.*` et `persist.*`
  generiques sont des proprietes « core » qu'il ne peut pas poser :
  [vendor_init.te](https://github.com/LineageOS/android_system_sepolicy/blob/lineage-16.0/public/vendor_init.te),
  [property_contexts](https://github.com/LineageOS/android_system_sepolicy/blob/lineage-16.0/private/property_contexts)).
- **Root** : pas necessaire pour que les pilotes se chargent et que l'Ethernet marche ; seulement pour la page Reseau de
  LibreHU-service (lecture des appareils, reactivation, iPhone).

## LibreHU-service : Systeme → Reseau
Adaptateurs et telephones branches (puce reconnue, pilote lie ou manquant), interfaces `eth*` / `usb*` (MAC, debit,
IPv4), reseau Ethernet d'Android et acces Internet, modules du zip (charges, chargement immediat, desactivation au
demarrage), partage USB de l'iPhone.

## A tester sur l'unite
1. Flasher le zip, redemarrer : `lsmod | grep r8152`, `ip link` (`eth0`), `dumpsys ethernet` (interface suivie).
2. Telephone Android en partage de connexion USB : `eth*` avec une adresse du telephone.
3. iPhone : bouton de LibreHU-service, partage de connexion active sur l'iPhone.
4. IP fixe : pas d'ecran dans Android 9 (API cachee `EthernetManager`), DHCP seulement pour l'instant.

## Sources
- Linux 4.9 : [drivers/usb/core/generic.c](https://github.com/torvalds/linux/blob/v4.9/drivers/usb/core/generic.c)
  (choix de la configuration USB), [drivers/net/usb/cdc_ether.c](https://github.com/torvalds/linux/blob/v4.9/drivers/net/usb/cdc_ether.c)
  (Realtek exclus, RNDIS conditionnel), [drivers/net/usb/usbnet.c](https://github.com/torvalds/linux/blob/v4.9/drivers/net/usb/usbnet.c)
  (nom `eth%d` / `usb%d`), [drivers/net/usb/r8152.c](https://github.com/torvalds/linux/blob/v4.9/drivers/net/usb/r8152.c)
  (passage en configuration 1), [drivers/net/usb/ipheth.c](https://github.com/torvalds/linux/blob/v4.9/drivers/net/usb/ipheth.c).
- Android 9 : [SystemServer.java](https://github.com/aosp-mirror/platform_frameworks_base/blob/pie-release/services/java/com/android/server/SystemServer.java)
  (demarrage d'EthernetService), [config.xml](https://github.com/aosp-mirror/platform_frameworks_base/blob/pie-release/core/res/res/values/config.xml)
  (`config_ethernet_iface_regex`).
