# Frequences et tensions CPU / GPU (`LibreHU-main`)

Tables d'overclock chargees par defaut, plafonnees au demarrage, reglables a chaud (root) depuis
`/sys/module/...` ou une appli de reglage (Kernel Adiutor, SmartPack Kernel Manager, onglet Tweaks de
LibreHU-service). Sur l'unite avec ce noyau (21/10/2026) : 2,3 GHz au demarrage et 2,4 GHz stables (Geekbench 5),
2,6 GHz instable a l'ancienne tension de 1,10 V ; les autres valeurs « testees » viennent de la branche
`reverse-engineering`.

## CPU (4 x Cortex-A53, un seul cluster)

Table `CPU_LEVEL_3` (`cpufreq_v1/src/mach/ac8257/mtk_cpufreq_opp_table.h`, 16 entrees imposees par la table
d'enregistrement de la SSPM, qui applique le DVFS) :

| MHz | Tension | Etat |
|---|---|---|
| 2601 | 1,050 V | **instable a 1,10 V** (20/10/2026, Vsram 50 mV sous Vproc) ; a 1,05 V : a tester |
| 2501 | 1,050 V | **instable a 1,10 V** (Geekbench, 30/10/2026, r127) ; a 1,05 V : a tester |
| 2401 | 1,050 V | **stable a 1,050 V** (Geekbench 5, 21/10/2026, avant `r127`) |
| 2301 | 1,050 V | **stable a 1,025 V** (`reverse-engineering`) ; **maximum au demarrage** |
| 2201 | 1,050 V | stable a 1,025 V |
| 2101 | 1,0375 V | teste a 1,025 V |
| 2001 ... 987 | tensions stock (table FY) | maximum stock 2001 MHz, 1,025 V |

Tensions au-dessus de 2,0 GHz : +12,5 mV par 100 MHz jusqu'a 1,05 V a 2,2 GHz, puis **1,05 V au plus**. La SSPM
garde Vsram (SRAM des coeurs) = Vproc + 0,1 V, entre 0,80 et 1,05 V : les `MIN_VSRAM_VOLT` / `MAX_VSRAM_VOLT` du paquet
Autochips. Lu sur l'unite le 30/10/2026 (`cpufreq_volt`, en charge) :

| MHz | Vproc | Vsram |
|---|---|---|
| 987 | 0,6875 V | 0,800 V |
| 1400 | 0,800 V | 0,900 V |
| 2000 | 1,025 V | 1,050 V |
| 2400 (r127 : 1,08125 V) | 1,08125 V | **1,050 V** |

Au-dessus de 1,05 V de Vproc, Vsram reste sous Vproc, etat que le pilote MediaTek lui-meme traite comme invalide
(`assert_volt_valid`) : c'est le mur des plantages a 2,5 et 2,6 GHz (1,10 V). La table s'arrete donc a 1,05 V (2,3 a
2,6 GHz ; r127 : 1,0625 a 1,11875 V). Aller au-dela demanderait de changer le plafond de Vsram dans le firmware de la
SSPM (partition `sspm`, fermee).
Reference : le MediaTek MT6765 (Helio P35 : memes Cortex-A53, 12 nm, meme PMIC MT6357) monte de 0,95 V a 2,033 GHz
a 1,11875 V a 2,351 GHz (puces FY), 2,501 GHz pour les meilleures (SB), soit environ +44 mV par 80 a 100 MHz, avec
des limites `MAX_VPROC_VOLT` / `MAX_VSRAM_VOLT` de 1,12 V (`mtk_cpufreq_opp_table.h` et `mtk_cpufreq_platform.h`
du noyau MT6765 de la Galaxy Tab A7 Lite, [gta7lite/android_kernel_samsung_mt6765](https://github.com/gta7lite/android_kernel_samsung_mt6765)).
L'AC8257 est donne pour 2,0 GHz : rien ne garantit 2,5 ou 2,6 GHz a 1,05 V.

**Jusqu'a `r127` (30/10/2026), ces tensions n'etaient pas appliquees au-dessus de 2,0 GHz** : `d95b3be61` avait
releve la table OPP et la table de puissance (upower), pas la table d'enregistrement `ocTbl`
(`mtk_cpufreq_opp_pv_table.h`) que `cpuhvfs_pvt_tbl_create()` copie dans la memoire de la SSPM et que la SSPM
applique (l'EEM est coupee avec la table OC). La SSPM gardait donc 1,10 / 1,075 / 1,05 V a 2,6 / 2,5 / 2,4 GHz et
1,025 V de 2,1 a 2,3 GHz, alors que `volt_uv` affichait la table relevee. Corrige par `dfccf2efa` (codes Vproc 96,
93, 90, 87, 85, 83, 81, soit 21 + (uV - 650000) / 6250). Tension reelle, lue dans le registre DA du PMIC MT6357
(root ; fixe le minimum a 2,3 GHz le temps de la lecture, puis le remet a 987 MHz) :
```
adb shell "su -c 'echo 2301000 > /sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq; sleep 2; cat /proc/cpufreq/MT_CPU_DVFS_LL/cpufreq_freq /proc/cpufreq/MT_CPU_DVFS_LL/cpufreq_volt; echo 987000 > /sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq'"
```
Attendu avec les reglages par defaut : `2301000 KHz` et `Vproc: 1050000 uV` (r127 : 1062500 uV, avant : 1025000 uV).

Le stock tourne a 2,0 GHz maximum (table FY, 1,025 V). 1400 MHz / 0,80 V reste l'OPP de calibration de l'EEM
(PTPOD).

Parametres (`/sys/module/ac8257_cpufreq/parameters/`, ou `ac8257_cpufreq.<nom>=` sur la ligne de commande) :

| Parametre | Defaut | Role |
|---|---|---|
| `max_khz` | 2601000 | frequence la plus haute atteignable ; `scaling_max_freq` peut monter jusque-la |
| `boot_khz` | 2301000 | maximum au demarrage (`scaling_max_freq` initial), lecture seule a chaud |
| `volt_uv` | table | les 16 tensions en uV, frequence la plus haute d'abord ; ecrire 16 valeurs separees par des virgules, 0 = valeur de la table |
| `volt_offset_uv` | 0 | decalage ajoute a toutes les tensions (negatif = sous-tension) |

Chaque tension est arrondie au pas du PMIC (6,25 mV) et gardee entre **0,65 et 1,05 V** (le regulateur Vproc accepte
jusqu'a 1,3125 V, mais Vsram ne depasse pas 1,05 V). La nouvelle table est envoyee a la SSPM
comme le fait l'EEM (`cpuhvfs_update_volt`). L'EEM est desactivee avec la table OC (elle est calibree pour la
table stock). `ac8257_cpu_oc=0` sur la ligne de commande : tables stock, EEM active, pas de reglage de tension.

Essais sur l'unite (noyau `LibreHU-main`, tensions par defaut) :
- 2,3 GHz : stable (deja valide avec `reverse-engineering`) ; c'est le maximum au demarrage.
- **2,6 GHz a 1,10 V : instable.** Le 20/10/2026 (run 59), apres environ 14 min, le coeur CPU 0 s'est fige
  (chien de garde materiel : `Qwdt`, `kick=0x0e check=0x0f`, `Non-crashing CPUs did not react to IPI`), a
  46 degres. Avec r127 (2,6 GHz a 1,11875 V, 2,5 GHz a 1,10 V), plantage a 2,5 GHz sous Geekbench le 30/10/2026 :
  Vsram restait a 1,05 V, sous Vproc. Table ramenee a 1,05 V au plus ; 2,5 / 2,6 GHz a 1,05 V : a tester,
  deconseille au quotidien.

Exemples (root) :
```
# monter a 2,3 GHz
echo 2301000 > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq
# sous-tension de 25 mV sur toute la table
echo -25000 > /sys/module/ac8257_cpufreq/parameters/volt_offset_uv
# tensions actuelles
cat /sys/module/ac8257_cpufreq/parameters/volt_uv
```

### Limites, gouverneurs et boosts (`r131`, 31/10/2026)

Avec la SSPM, le noyau ne regle pas la frequence lui-meme : il ecrit dans la memoire de la SSPM la requete du
gouverneur (registre `WFI`) et les bornes en vigueur (registre `PPM_LIMIT` : `scaling_min_freq` /
`scaling_max_freq`, `max_khz`, limites PPM et thermiques, boosts), et la SSPM applique la requete dans ces bornes.
Signale sur l'unite : en powersave avec un minimum de 1,5 GHz et un maximum de 2,2 GHz, le CPU restait au maximum,
quel que soit le gouverneur. Defauts trouves dans le code et corriges :

| Defaut | Effet | Correction |
|---|---|---|
| le thread de reception de la SSPM recopiait les bornes en vigueur (boosts, limites thermiques) dans `scaling_min_freq` / `scaling_max_freq` | un changement de gouverneur recopie la politique : le notifier LibreHU prenait ces bornes pour celles de l'utilisateur. Plancher fige a celui d'un boost, plafond fige a une limite thermique, et powersave / performance visaient ces valeurs | `scaling_min_freq` / `scaling_max_freq` gardent les valeurs ecrites ; la SSPM applique toujours les bornes effectives |
| les requetes passant par le coeur cpufreq (performance, powersave, userspace, ondemand, conservative, interactive) etaient bornees une seconde fois cote noyau, avec les bornes du moment | un gouverneur qui ne demande qu'une fois gardait la borne du moment (plancher d'un boost, limite thermique) apres sa fin | requete envoyee telle quelle, comme celles de schedutil et schedplus ; la SSPM la borne |
| le coeur cpufreq ne transmet pas une requete egale a `scaling_cur_freq`, qui est la frequence de la SSPM | la requete precedente, parfois celle d'un autre gouverneur, restait en vigueur | drapeau `CPUFREQ_NEED_UPDATE_LIMITS` de Linux 5.10 retroporte (`1c534352f47f`, `a62f68f5ca53`) et pose par mt-cpufreq ; interactive et ondemand en tiennent compte |
| au changement de `scaling_min/max_freq`, interactive, schedutil, ondemand et conservative demandaient le min / max d'apres `scaling_cur_freq` | la requete du gouverneur etait remplacee : CPU au nouveau maximum alors que le gouverneur voulait le minimum | ils redonnent leur propre requete, calculee dans les nouvelles limites |
| index d'OPP fournis par le vendor (`boot_freq` de `cpu_ctrl`, boost tactile utilisateur `usrtch`) lus dans la table OC | OPP 0 = 2601 MHz au lieu de 2001 MHz, OPP 2 = 2401 au lieu de 1834 MHz | lus dans la table d'origine de la puce : memes frequences que le stock |
| la SSPM demarre avec la requete 0 et sans borne, jusqu'au premier rappel de la PPM | OPP 0 = 2601 MHz avec la table OC (2001 MHz en stock) | requete et borne de depart a `boot_khz` (2,3 GHz) |

Lecture des valeurs :
- `scaling_min_freq` / `scaling_max_freq` : les limites ecrites (applis, LibreHU-service) ; elles n'affichent plus
  les limites thermiques ni les boosts.
- `scaling_cur_freq` : la frequence de la SSPM (moyenne de son journal) ; `/proc/cpufreq/MT_CPU_DVFS_LL/cpufreq_freq`
  : celle de la PLL.
- Planchers en vigueur : boost tactile noyau `ktch` (1183 MHz doigt pose, `/proc/perfmgr/tchbst/kernel/`), boost
  LibreHU (1533 MHz pendant 100 ms, `ac8257_input_boost`), demandes du vendor par
  `/proc/perfmgr/boost_ctrl/cpu_ctrl/` (`perfserv_freq` du HAL power, `boot_freq` des scripts d'init, resume dans
  `current_freqy`). Le plus haut l'emporte, sous `scaling_max_freq` et `max_khz`. `usrtch` n'est pas compile
  (il depend de `CONFIG_MTK_FPSGO`).

## GPU (PowerVR GE8300)

Table OC : 730, 660, 600, 500, 450, 390, 300 MHz (730 MHz = OPP haute du MT6761T, a la tension stock de 660 MHz).

| Parametre (`/sys/module/ac8257_gpufreq/parameters/`) | Defaut | Role |
|---|---|---|
| `max_khz` | 730000 | frequence la plus haute (660000 = maximum stock). Demarrage et PTPOD restent a 660 MHz au plus (violations EMI MPU constatees en demarrant a 730 MHz) |
| `min_khz` | 0 | frequence la plus basse |
| `vcore_uv` | 800000,700000,700000,650000 | tensions des niveaux VCORE_OPP_0..3, ecrire 4 valeurs, 0 = inchange, bornes **0,60-0,90 V** |

Le GPU n'a **pas d'alimentation propre** : il tourne sur VCORE, partagee avec le controleur de RAM et le reste du
SoC. Chaque OPP GPU demande un niveau VCORE au DVFSRC : 730-600 MHz -> VCORE_OPP_0 (0,80 V), 500-450 MHz ->
VCORE_OPP_1 (0,70 V), 390-300 MHz -> VCORE_OPP_3 (0,65 V). Changer `vcore_uv` change donc la tension de tout le
SoC a ce niveau, pas seulement du GPU.

Noeud pour les applis : `/sys/devices/platform/dfrgx/devfreq/dfrgx/` (`cur_freq`, `available_frequencies`,
`max_freq`, `min_freq`).

Bornes du GPU (`r131`) : un OPP fixe par `/proc/gpufreq/gpufreq_opp_freq` (reglage GPU de LibreHU-service Tweaks)
ou, sinon, `max_freq` / `min_freq` (`max_khz` / `min_khz`). La limite thermique / puissance l'emporte toujours.
Corrige :
- un OPP fixe passait apres la limite thermique et la remplacait : un GPU fixe a 730 MHz n'etait jamais bride ;
- GED (le DVFS GPU de MediaTek) ne connaissait pas `min_freq` / `max_freq`. Une requete hors bornes ne
  correspondait jamais a l'OPP applique : a chaque periode, GED la renvoyait, avec a chaque fois une poignee de main
  de changement d'horloge avec le pilote PowerVR, pour rien. GED vise maintenant dans ces bornes.

## Thermique CPU et carte (politique LibreHU)

Au démarrage, `thermal_manager` (vendor) écrit sa configuration (`/vendor/etc/.tp/thermal.conf`, chiffrée) dans
`/proc/driver/thermal/`. Le noyau remplace les seuils CPU qu'elle écrit (`thermal/common/thermal_zones/mtk_ts_cpu_noBank.c`)
et ceux de la carte (`mtk_ts_bts.c`), et coupe le CTM (`coolers/mtk_cooler_atm.c`) ; ses valeurs par défaut (avant
`thermal_manager`) sont les mêmes (`thermal/ac8257/inc/tzcpu_initcfg.h`, `clatm_initcfg.h`, `mtk_ts_bts.c`).

| Seuil CPU (capteur de la puce) | Vendor | LibreHU |
|---|---|---|
| reset `mtktscpu-sysrst` (règle aussi la protection matérielle) | 125 °C | 125 °C |
| frein `cpu00` (budget total CPU + GPU 700 mW) | — | 105 °C |
| frein `cpu03` (budget total CPU + GPU 1000 mW) | — | 105 °C |
| régulation ATM `cpu_adaptive_0` / `cpu_adaptive_1` (cible = température du seuil) | 55 / 54 °C | 100 °C |
| CTM (cible ramenée de 85 à 55 °C quand la carte passe de 43 à 46 °C) | actif | coupé |

| Seuil carte (thermistance AP, `tzbts`) | Vendor | LibreHU |
|---|---|---|
| arrêt d'Android `mtk-cl-shutdown00` | — (la ligne du vendor active 0 seuil : aucune protection) | 105 °C |
| reset `mtktsAP-sysrst` | — | 107 °C |

L'arrêt passe par le démon déclaré dans `/proc/driver/thermal/clsd_pid` ; sans lui, le noyau s'éteint lui-même
(`orderly_poweroff`). Inchangé : budgets de l'ATM (`clatm_setting`, plancher 685 mW, pas de plafond) et autres
capteurs. Vérifier : `cat /proc/driver/thermal/tzcpu`, `cat /proc/driver/thermal/tzbts` et
`cat /proc/driver/thermal/clctm` (`ctm 0`). Valeurs du vendor : `ac8257_thermal.policy=0` sur la ligne de commande ;
à chaud, `echo 0 > /sys/module/ac8257_thermal/parameters/policy`, puis les écritures suivantes de `tzcpu`, `tzbts` et
`clctm` sont prises telles quelles.

## Audit du 20/10/2026 : fréquence « capricieuse »

Le comportement erratique vu avec les noyaux `reverse-engineering` (meme code) avait trois causes, corrigees :

| Cause | Effet | Correction |
|---|---|---|
| Le thread de reception SSPM (`mtk_cpufreq_hybrid.c`) recopie les bornes min / max que la SSPM lui renvoie dans `policy->min` / `max` via `opp_tbl[index]` ; un index -1 (« pas de limite ») lisait `opp_tbl[-1]` | `scaling_min_freq` / `scaling_max_freq` aleatoires | bornes toujours dans la table, cote envoi (`ac8257_apply_soft_max`) et cote reception |
| Plancher au-dessus du plafond (`scaling_min_freq` au-dessus d'une limite PPM / thermique) | la frequence cible oscille entre les deux | le plafond l'emporte : plancher ramene au plafond |
| Pas de table de puissance (upower) pour la table OC : repli sur la table FY lue par index d'OPP, 2,6 GHz estime comme 2,0 GHz | budget de puissance / thermique de la PPM et modele d'energie de l'ordonnanceur faux en haut : la frequence monte trop, puis le thermique bride brutalement | table upower construite pour l'OC (lignes FY reprises, au-dessus de 2001 MHz extrapolees par exces : f.V^2 dynamique, V^3 fuite ; capacite ramenee a 1024) |

Verifie et sain : chemin gouverneur -> `_calc_new_opp_idx` -> SSPM, plafond utilisateur via le notifier de
politique (enregistre avant le pilote), cote GPU ordre des limites (plafond LibreHU, plancher, puis thermique /
puissance prioritaires) et table de puissance GPU calculee depuis les vraies frequence et tension. Revu le
31/10/2026 : ce chemin et la recopie des bornes dans la politique figeaient des limites, voir
[Limites, gouverneurs et boosts](#limites-gouverneurs-et-boosts-r131-31102026).

**Correction du 30/10/2026 : le boost tactile ne faisait rien** (il figurait a tort dans la liste ci-dessus).
`ac8257_input_boost` passait par le sysboost de la PPM (`mt_ppm_sysboost_freq`), politique que l'init vendor laisse
desactivee : chaque toucher ne donnait que « sysboost policy is not enabled! » dans dmesg (88 fois dans les logs
`r127`). Depuis `r128` (`3ccc70f3f`), il passe par le plancher LibreHU (`ac8257_cpufreq_boost_min()`, replie comme
`scaling_min_freq` ; plafond et limites PPM / thermiques prioritaires). Reglages inchanges dans
`/sys/module/ac8257_input_boost/parameters/` : `freq_khz` (1533000, 0 = coupe), `duration_ms` (100). Limites restantes : une tension changee a chaud (`volt_uv`, `vcore_uv`) n'est pas reportee
dans les estimations de puissance (upower, table GPU) ; la frequence affichee est celle que la SSPM rapporte
(moyenne de son journal), pas une mesure de la PLL.

## Prudence

- Au-dessus de 2,3 GHz et au-dessus de 1,025 V, rien n'a ete essaye sur l'unite. La tenue depend de la puce
  (chaque AC8257 est differente), la chaleur aussi : l'autoradio est souvent enferme sans ventilation.
- Le firmware SSPM (partition `sspm`, ferme) applique les tensions ; s'il les borne lui-meme, une tension
  au-dessus de 1,025 V peut etre ignoree : a verifier au premier test (tension lue au regulateur Vproc,
  stabilite) avant de conclure qu'une frequence tient.
- Monter par petits pas, tester (charge CPU complete plusieurs minutes), et garder une sortie : les reglages ne
  survivent pas a un redemarrage (sauf ligne de commande), un plantage revient donc aux valeurs par defaut.
