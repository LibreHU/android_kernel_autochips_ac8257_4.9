# Reference Sources

## Primary public kernel skeleton

- https://github.com/mediatek-android-development/android_kernel_mediatek_mt6761-62-4.9 — public MediaTek Linux 4.9 reference tree.
- https://github.com/emteria/kernel_mediatek_mt6761 — secondary complete MT6761 Linux 4.9 reference.

## Secondary MT8766/MT6761 reference

- https://github.com/CoderCharmander/tb8505f-kernel — contains `tb8766p1_64_bsp` MT6761 configuration and Device Tree.
- https://github.com/Kevin233B/TALIH-PD2-Kernel — contains the same `tb8766p1_64_bsp` project structure.
- https://github.com/ExtremeXBB/android_kernel_lenovo_aaron_akita_anna — additional `tb8766p1_64_bsp` reference.

## AC8257 references

- https://github.com/Dok-T/android_kernel_alps_ac8257 — public AC8257-derived kernel skeleton, incomplete for a full BSP reconstruction.
- https://github.com/twrpdtgen/android_device_alps_ac8257_demo — device tree for `ac8257_demo`, useful for device-specific integration references.

## Historical AutoChips reference

- https://github.com/iscle/android_kernel_autochips_ac8227l — older AutoChips 3.18 source, useful only for historical AutoChips driver patterns.

## Private BSP fingerprint

CSDN documentation references an internal Gitolite repository named `ac8257Project/ac8257.git`, branch `master-8257-yige`.

That repository has not been treated as publicly accessible source.

## Official GPL package (SIXWIN / Jancar, 19/10/2026)

Answer to the GPLv2 request for the UJC201: `kernel-4.9.tgz` (156 599 414 bytes, SHA-256
`f0303951aed4ed65eef9da7fdaf0718c50d019f5c956429dc4cdb5cb7d83c577`), shared through OneDrive. Analysis (text
comparison only): [docs/ujc201/kernel_gpl.md](ujc201/kernel_gpl.md). The package itself is the
[`autochips-gpl`](https://github.com/LibreHU/android_kernel_autochips_ac8257_4.9/tree/autochips-gpl) branch; this
reconstruction is `reverse-engineering`; the main working branch is
[`main`](https://github.com/LibreHU/android_kernel_autochips_ac8257_4.9/tree/main) (AutoChips kernel + the changes
ported from `reverse-engineering`).

- Linux 4.9.117, AutoChips reference BSP: `ac8257_{car,demo,evb,jac}` defconfigs, `ac8257*.dts`,
  `drivers/misc/autochips/` (`metazone`, `tvd`, `avin`, `backcar`, `di`, `nr`, `wch`, `dualarm`, `arm2_log`,
  `n4_video_decoder`, `boot_state`, `rsv_cpu`), display with `ext_disp`, GT928 touch.
- `ac8257_demo_defconfig` matches 428 of the 435 options of the stock #72 config (the 7 others are product
  settings: DTBO `mediatek/UJC201_64`, sensors, 1024x600 panel, `SND_SOC_ATC`).
- Against `reference/`: 107 of the 121 stock source files missing or renamed here are in the package (the 14 left
  are ILITEK / Himax / Synaptics touch drivers); 36 of the 42 options without Kconfig are in it (left:
  `JANCAR_GPIOS`, `JANCAR_SOLUTION`, `TOUCHSCREEN_FTS`, `TOUCHSCREEN_FTS_DIRECTORY`, `TOUCHSCREEN_SYNA_TCM2`,
  `TOUCHSCREEN_SYNA_TCM2_I2C`).
- Not in it: the Jancar drivers (`JANCAR_*`), the `tp9963` sensor, the `UJC201_64` overlay, and the out-of-tree
  vendor modules (`wmt_drv`, `bt_drv`, `gps_drv`, `fmradio_drv`, `wmt_chrdev_wifi`, `wlan_drv_gen4m`, `fpsgo`);
  requested again from the distributor.
- Not in it either: Jancar's edits of existing files. Stock strings absent from the package sources: LCM
  `jac_lcm_get_mipi_params_form_MetaZone`, `jac_detect_format_from_metazone`, `jac_push_table`,
  `jac_set_lcd_power_kthread`; lowmemorykiller `jump kill 'jancar.services'` / `/jancar/config/ivi-priapps.ini`;
  IR keymap `rc-jancar`; `show_jancar_board_id`; `gpios_ioctl` (`gpio_get_mode` / `gpio_set_mode`); Sitronix touch.
- Complete otherwise: all 1 137 source paths embedded in the stock image are in the package except the 14 touch
  files; of the 9 674 stock exports, only those of the drivers above have no `EXPORT_SYMBOL` in it. GPU
  `m1.10ED5130912` matches the stock DDK `1.10@5130912`.
