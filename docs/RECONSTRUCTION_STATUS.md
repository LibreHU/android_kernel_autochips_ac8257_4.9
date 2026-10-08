# AC8257 Reconstruction Status

## Objective

Rebuild a usable Linux 4.9 kernel source tree for the AutoChips AC8257 platform used by the Jancar UJC201.

## Baseline

The current repository is a Linux 4.9.117 source tree.

The target stock kernel is also Linux 4.9.117+.

This version match is important because it minimizes unrelated kernel API differences during the reconstruction.

## Target evidence

| Area | Status | Evidence | Notes |
|---|---|---|---|
| Kernel version | Confirmed | Stock kernel + repository Makefile | Both identify Linux 4.9.117 |
| AC8257 platform | Confirmed | Stock DTB/config | `model = AC8257`, `compatible = mediatek,AC8257`, `CONFIG_MACH_AC8257=y` |
| CPU topology | Confirmed | Stock DTB / interrupts | Four Cortex-A53 CPU nodes are present and CPUs 0-3 are active |
| MT6357 | Confirmed | Stock config/DT | `CONFIG_MTK_PMIC_CHIP_MT6357=y` |
| AC8257 pinctrl | Confirmed | Stock config/DT | `CONFIG_PINCTRL_AC8257=y` |
| AC8257 clocks | Confirmed | Stock config/DT | `CONFIG_COMMON_CLK_AC8257=y` |
| GPU | Confirmed | Stock config | PowerVR RGX Clark configuration is enabled |
| IOMMU/SMI | Confirmed | Stock config | MTK IOMMU V2 and SMI are enabled |
| Camera/ISP | Confirmed | Stock config/DT | MTK camera ISP support is enabled, with AC8257-specific DT nodes |
| Video codec | Confirmed | Stock config | MTK video codec driver is enabled |
| ARM2 | Confirmed | Stock DT/BSP evidence | ARM2 firmware and ARM2-related DT nodes exist |
| MDP | Confirmed | Stock DT | AC8257 MDP-related hardware nodes exist |
| TVD/CVBS | Confirmed | Stock DT | AC8257-specific TVD/CVBS nodes exist |
| WCH-DI | Confirmed | Stock DT | AC8257-specific WCH-DI node exists |
| Proprietary display glue | Missing from public skeleton | Stock DT + CSDN BSP evidence | Requires reconstruction or compatible source |
| Full AC8257 BSP | Not recovered | CSDN source fingerprints | Original Gitolite repository is internal and not publicly mirrored in the sources found so far |

## Known proprietary modules

The stock vendor image contains non-stripped AC8257-related kernel modules including wireless, Bluetooth, FM, GPS, MET and performance components.

Known modules include `wmt_drv.ko`, `wmt_chrdev_wifi.ko`, `wlan_drv_gen4m.ko`, `bt_drv.ko`, `fmradio_drv.ko`, `gps_drv.ko`, `met.ko`, and `fpsgo.ko`.

The modules contain source filename strings and AC8257-compatible identifiers that can be used to map missing source files against public MediaTek trees.

## Known BSP structure

CSDN documentation describes a real AC8257 Android 9 BSP containing `kernel-4.9`, preloader, LK, ARM2 and proprietary AutoChips components.

The documented private repository was `ac8257Project/ac8257.git` on an internal Gitolite server, branch `master-8257-yige`.

The documented kernel Device Tree path is `kernel-4.9/arch/arm64/boot/dts/mediatek/ac8257_demo.dts`.

The documented preloader tree contains an `ac8257` platform directory with EMI, DRAMC, GPIO, ARM2 loading, download, platform and PMIC sources.

## Priority order

1. Make the repository structurally match the target AC8257 platform.
2. Reconstruct the AC8257 configuration and Device Tree.
3. Port clocks, pinctrl, PMIC, EMI and DRAMC.
4. Recover or reconstruct ARM2 integration.
5. Reconstruct display, camera, MDP, TVD, CVBS and WCH-DI.
6. Reconstruct GPU integration.
7. Reintegrate CONSYS and peripheral drivers.
8. Build a bootable test kernel only after the platform layer is coherent.

## Stage 1: stock ROM boot (in progress)

Goal: a kernel built from this tree that boots the stock UJC201 Android 9 (same `boot.img` ramdisk, same
`dtbo.img`). Camera, Wi-Fi/BT/GPS/FM, reverse camera and the AutoChips extras come after.

### What the stock kernel tells us

Extracted with `tools/ac8257/extract_stock.py` and `tools/ac8257/stock_symvers.py` into `reference/`:

| Evidence | Finding |
|---|---|
| Source file paths in the stock image (`__FILE__` of WARN/BUG) | 1137 files: 1016 exist here unchanged, 95 exist under `mt6761` / `mt6765` instead of `ac8257`, 26 are missing, all non-boot AutoChips/Jancar drivers (touch panels, TVD/AVIN/DI/NR, CarPlay/iAP2, AC8x UART DMA, camera EEPROM) |
| Strings | no "mt6761" anywhere: the stock tree is the MediaTek MT6761 BSP with the platform renamed to `ac8257` (`CONFIG_MACH_MT6761` is not even listed in the stock config) |
| UART driver | `SERIAL_8250_AC8X` holds `mtk8250_*` functions: MediaTek's `8250_mtk.c` renamed |
| Pin controller | same 8 register banks as MT6761 at the same addresses, plus `syscfg_pctl_8` (0x10001000) and pins 180-182 |
| Clocks | stock DT uses MT6761's clock compatibles (`mediatek,topckgen`, `mediatek,apmixed`...) |
| GPU | config says `rgx clark m1.9ED4971894`, but "clark" selects `m1.10ED5130912` and the stock image contains `rogueddk 1.10@5130912`: the DDK in this tree |
| Configuration | 42 of the 1454 stock options do not exist here (ATC_*, JANCAR_*, touch panels, AC8257 names); see `reference/stock-config-missing-symbols.txt` |
| Device tree | the stock DTB and DTBO decompile and recompile byte-identical (`ac8257.dts`, `UJC201_64.dts`) |
| Display | panel / MIPI-LVDS bridge (I2C 0x2d) are initialised by ARM2 before Linux; the stock `lcm_driver_common` reads a text panel description (`[Lvds_Init_S]`...) kept with the boot logo (`logo.mrf`) / metazone |
| Toolchain | AOSP clang 6.0.2 (4691093) + GCC 4.9 binutils |

### Done

- `MACH_AC8257`, `PINCTRL_AC8257`, `COMMON_CLK_AC8257`, `SERIAL_8250_AC8X`: AC8257 names over the MT6761 code
  (MACH_AC8257 selects MACH_MT6761).
- `CONFIG_MTK_PLATFORM="ac8257"` as in the stock config: per-platform directories `ac8257` are links to the
  `mt6761` ones, or to `mt6765` where MT6761 uses the MT6765 code (display, CCCI, freqhopping, pmic_wrap,
  usb20, MSDC ComboA, sound...), matching the stock file paths.
- `ac8257_demo_defconfig` from the stock configuration; stock DTB/DTBO as sources.
- Stage-1 `lcm_driver_common` placeholder (reports 720x1280 DSI video, the LK frame buffer; never touches
  the panel).
- No battery: no bcct cooler (`ATC_DISABLE_BAT_CHAGE`), as in the stock kernel.
- Build fixes: host dtc with GCC >= 10, gcc 4.9 false positives, missing `gether.no_skb_reserve`, GPU DVFS
  passing a device index where this DDK wants the device node (real bug, clang refuses it).
- Builds and links with the AOSP GCC 4.9 toolchain: `Image.gz-dtb` (see `docs/BUILD.md`).

### On the device (UJC201)

Tests run from the recovery partition (`docs/BUILD.md`), with the bring-up aids on: early pstore console,
timed panic, userspace reboots turned into a panic (with the recovery command cleared from `para`).

| Test | Result | Fix |
|---|---|---|
| stage1, 1b | logo, reboot, no kernel log | the repack dropped the boot partition's AVB footer: the LK refused the image (`repack_boot.py` keeps it now) |
| 1e | kernel runs to 1.67 s, `BUG` in `trusty_dump_logs` | 6 GiB of RAM, 32-bit Trusty: Trusty shared buffers below 4 GiB (`GFP_DMA`, as the stock kernel) |
| 1f | runs to 1.72 s, `mtkfb_probe` oops | AutoChips frame buffer: `autochips,framebuffer` node, `vmap`, 0x6000 header, LCM-initialised magic (stock `mtkfb_probe`) |
| 1g | **kernel boots to the end**: display, sound card, eMMC, system mounted as root, `Kernel_init_done` at 2.25 s | (recovery mode: the LK gives no `init=/init`; `--cmdline-append`) |
| 1h-1j | Android init and services run; GPU clients fail ("Driver already in bad state") | GPU memory below 4 GiB (stock: gfp `0x24302c3` / `0x24000c1`) |
| 1k, 1l | GPU OK, Trusty apps OK, `system_server` up; `hwcomposer` aborts in a loop, Rescue Party asks for recovery | AutoChips display ABI, below |
| 1m | native services stable, backlight dims (power manager runs), black screen, no USB; SystemUI crash-loops: `Failed to find provider com.jancar.settings.provider`, `com.jancar.services` (`/vendor/app/ivi-services`, persistent) "not found"; same package installed and enabled on the stock kernel, `/data` unencrypted | USB: below; Jancar packages: under investigation (needs the full boot log, now possible over adb) |
| 1n | **USB/adb work**. `/data` does not mount: `fs_stat userdata 0x103` (ext4, full mount failed), so init takes the `defaultcrypto` path (`ro.crypto.state=encrypted`, `Cryptfs: Bad magic`), `vold.decrypt=trigger_restart_min_framework`, "only parsing core apps": the Jancar packages missing in 1m come from this | ext4/quota/crypto options identical to the stock config: the kernel error is needed (`log_buf_len=8M`, read-only test mount) |
| 1o | kernel log: `EXT4-fs (mmcblk0p39): Unrecognized mount option "autoformat"`; a mount without it works | AutoChips ext4 options, below |
| 1p | **`/data` mounts, Android shows its UI on the panel** with this kernel; no touch, backlight not working, some artefacts, slow `scrcpy` | below |

Stage 1p findings:
- Stock unit (live): the touch input is `mtk-tpd` (MTK TPD framework, 720x1280, 10 fingers) and the I2C client
  3-0001 is named `gt928`: the active driver is the MTK GT928 TPD driver (`CONFIG_TOUCHSCREEN_MTK_GT928`, stock
  symbols `gtp_*`, `tpd_i2c_probe`, `gt9xx_props_*`), absent from this tree. 0-002d is `lcm_bridge_ic`. Each
  brightness change goes through the light HAL to `disp_pwm_set_backlight_cmdq` and
  `lcm_bridge_ic_detect_ds90ub947`/`ds90ub941`: the backlight is set by the panel driver through the bridge,
  which the placeholder `lcm_driver_common` does not do.
- Touch, first attempt (stage 1q): the GT928 speaks the GT9xx protocol of the mainline `goodix.c` here, which
  already matches `goodix,gt928`; on AC8257 it now takes the address from `slave_addr` (0x5d) instead of the
  `reg` index, skips the GPIO reset/address selection (controller behind the FPD-Link bridge, set up before
  Linux) and names the input device `mtk-tpd` like the stock one. Axis orientation to be checked on the unit.
  Result: the controller answers at 0x5d (`ID 911, version: 1060`), the input device is created, but the i2c
  core found no interrupt for the node (`request IRQ failed: -22`). Stage 1r: interrupt from the `irq-gpios`
  pin (`gpio_to_irq`), else polling every 16 ms (the stock `goodix.c` has a timer mode too).
  Stage 1r: the input device stays (X 0-1080, Y 0-600 from the controller config) but no event; the stock
  `goodix.c` has an "irq mode" and a "polling mode" (`irq_mode`, `goodix_wq`) and the DTBO pin is gpio 0, so
  stage 1s polls (pin left alone) and swaps X/Y (stock `mtk-tpd` reports 720x1280). The i2c ACK-error dumps
  are rate-limited (i2c6 0x40 filled the 8 MiB log in two minutes).
- Stage 1r logcat: `com.jancar.services` cannot open `/dev/gpios_ioctl` (stock `CONFIG_JANCAR_GPIOS`,
  `misc_gpio_ioctl`) and its `CarService` hits ANR after ANR; the BT HAL aborts (no vendor modules);
  GMS crash-loops (`co.g.App`, to compare with the stock kernel). Stage 1t adds the device
  (`drivers/misc/autochips/jancar_gpios.c`): the ioctl argument is the index N of an `ac8227l_pin_N` GPIO
  of the `mediatek,ac8227l-gpio-ioctl` node of the stock DTB; 0x6b00 high, 0x6b01 low, 0x6b02 input,
  0x6b03 read, as the stock driver.
- Stage 1t: **touch works** (polling, events in `getevent`), inverted on the panel; `/dev/gpios_ioctl` is there.
  The display is rotated by 90 degrees and shows glitches. The stock kernel reports the same panel (720x1280
  from the metazone, MIPI 4 lanes) and the same display caps (`lcm_degree` 0, 2 layers): the landscape UI
  comes from userspace (`persist.sf.hwrotation=90` on the stock unit), to be checked under this kernel.
  Stage 1u: `invert_x`/`invert_y` in `/sys/module/goodix/parameters/` set the touch orientation at run time
  (after the X/Y swap), to find the right one on the unit.
  Result: both inverted is right (a 180-degree turn after the swap); the default since stage 1v.
- Stage 1v: exception reboot at 9.7 s: `disp_irq_handler` read RDMA0 (`0x1400d004`) with its clock off
  (`Unhandled fault: Systracker debug exception`, `AR_TRACKER ReadAddr:0x1400d004`), first read timeout at
  7.8 s in `atcavm_server` (AutoChips AVM / fast display, started at 3.95 s). The stock kernel has an AutoChips
  layer sharing the display with ARM2 that this tree lacks (`fast_disp_composer_thread`, `get_fb_or_arm2_status`,
  `set_arm2_backcar_status`, `get_fast_disp_exit_status`, `u4ARM2Start`; its `disp_irq_handler` checks
  `get_arm2_backcar_status`); the intermittent black screens are likely the same issue. Stage 1w, stopgap:
  a display module interrupt arriving with the module clock (or the MM power domain) off is masked instead of
  read, and unmasked when the driver enables that clock again.
- Stage 1w: the first attempt ended in a watchdog reset with nothing in pstore (header only; LK: boot reason
  `wdt_by_pass_pwk`), so no kernel log to tell whether this kernel ran; the next attempt with the same image
  booted and runs (no masked interrupt seen in that boot). Display rotation: `persist.sf.hwrotation` (90 on the
  stock unit) is not set under this kernel. It is set by `/vendor/bin/rotationd` (`on post-fs`, oneshot), which
  reads it from the metazone: the vendor sepolicy gives rotationd `metazone_device (chr_file (ioctl read write
  open))` and the `persist.sf.hwrotation` property; `/dev/mtz` (stock `CONFIG_ATC_METAZONE`: `MTZ_IOControl`,
  `MetaZone_Read`, reserved memory `autochips,metazone` at 0x60700000 and the `metazone` partition) is missing
  here. The LK reads the same value (`char ui rotation:90`). Next: reconstruct the metazone driver.
- Stage 2b (overclock table with its SSPM record table, KernelSU Next): the 1.3 s crash is gone and
  KernelSU Next initialises, but `eem_init01` then spins 16 s waiting for Vproc to reach VBOOT (0.80 V): the
  PTPOD policy fixes the CPU at OPP index 8, the 0.80 V / 1400 MHz entry of the stock tables but 1533 MHz /
  0.85 V in the overclock table. Stage 2c uses index 10 (1400 MHz / 0.80 V) for the overclock level. During
  that wait a display interrupt read `MMSYS_CG_CON0` (0x14000100) through the stage 1w guard (`__clk_is_enabled`
  reads the gate register) while the MM bus was not clocked: Systracker read timeout, exception reboot at
  18.5 s. The guard now uses the software clock enable counts only, with no register read.
- Stage 2c: **boots to Android**, CPU at 2001000 kHz (cap applied on the hybrid path), KernelSU Next runs
  (`/init second_stage executed`, init.rc hook, `on_post_fs_data`); the manager installed was the original
  KernelSU one (`me.weishu.kernelsu`, `is_manager: 0`): KernelSU Next only accepts its own manager. The USB
  disconnections are `system_server` restarts (`UsbDeviceManager` sets `mtp,adb` again each time); the crashes
  look like memory corruption, in several processes and never seen in the stage 1p/1t logs without the
  overclock table: SIGBUS `BUS_ADRALN` in `ConcurrentLinkedQueue.size` (`system_server`, GMS), ART `Invalid
  monitor state ForwardingAddress`, `Invalid address 0xff9f9f9f passed to free`, SIGSEGV in CPU-Z. zram is not
  in use (swap free = total). Prime suspect: the overclock level, which runs on the sign-off voltages without
  the EEM per-chip corrections (`mt_cpufreq_update_volt` ignored). A/B test: the same kernel with
  `ac8257_cpu_oc=0 ac8257_gpu_oc=0` (stock FY table with EEM).
- Stock reference capture (stock kernel #25, 14 min uptime): no native crash, one `system_server` start, BT
  firmware configured, `schedplus` governor, the same 16 CPU frequencies (850 MHz-2.001 GHz). The AEE
  database of the unit (`/data/aee_exp/db.fatal.*`) holds `system_server` native crashes under this tree's
  kernels: SIGABRT pid 639 at 22:58 on 10-07, which is the stage 1w boot (#28: no overclock table, no
  KernelSU, no tweaks; `system_server` was pid 639 in that boot), then SIGABRT/SIGSEGV/SIGBUS on 10-08
  (stage 2c). The memory corruption therefore predates the overclock level; the OC A/B test is kept but is
  unlikely to be the whole answer. Stage 2c itself ended with the bring-up timed panic at 611 s
  (`ac8257_panic_secs=600`).
  Differences with the stock kernel at boot: the stock ION driver has an AutoChips "backcar" heap (type
  RESERVED) on `wch-di-reserved-memory@60800000` (the CVBS AVM heap `cvbs-ion@5e000000` when the metazone
  enables AVM), 11 heaps against 10 here; the stock `reserved-memory` handlers (`autochips,cvbs-ion`,
  `wch-di`, `mdp`, `isp`, `arm2-backcar-ui`, `metazone`) and `CONFIG_MEMBLK_RELEASE_POLICY` are absent (the
  regions are still reserved from their `reg`); config options only in the stock kernel: `ATC_BACKCAR`,
  `ATC_FASTDISP_VERSION`, `ATC_METAZONE`, `ATC_WCH`, `ATC_DI`, `ATC_TVD`, `ATC_NR`, `ATC_AVIN`,
  `ATC_BOOT_STATE`, `ATC_QB_ENHANCEMENT`, `ATC_AOSP_ENHANCEMENT`, `LCM_TRANSFER_IC_SUPPORT`, `MTK_RDI`,
  `SPI_AC8X`, `TOUCHSCREEN_MTK_GT928`, `ATC_USB_BC12`, `ATC_USB_HSRX_DISC`, `JANCAR_SOLUTION`.
- Stage 2d capture with the collector (`ujc201_debug`, same tool as the stock capture), compared with the stock
  one. Live device trees: in a recovery-partition boot the LK passes `atag,boot` boot mode 2 (recovery),
  `firmware/android/mode = "recovery"`, disables `scp@10500000` (hence no `/dev/scp`) and does not add two
  reserved regions it adds in a normal boot: `mblock-10-SCP-reserved` (0x9f900000, 6 MiB) and
  `mblock-9-SPM-reserved` (0x77ff0000, 64 KiB, SPM firmware); they are the 6208 KiB of "System RAM" more than
  the stock kernel. Every test so far ran in this recovery mode, unlike the stock comparison. Under 2d the
  kernel log also shows hundreds of EMI MPU write violations from the GPU (`AXI_MST_GPU`/`MFG_M0`, domain 6)
  into region 0 (0x563xxxxx-0x567xxxxx, inside the ATF reservation, and 0x40000000), from 20 s on; none
  under the stock kernel. A GPU writing to physical addresses it should not reach also explains random
  corruption elsewhere. Stage 2e: GPU soft maximum also applied to the initial OPP and the PTPOD OPP (both
  took index 0, 730 MHz, ignoring `ac8257_gpufreq.max_khz`), and a boot-partition image for a normal-mode
  test. Other differences: no vendor module (`wmt_drv`, `bt_drv`, `gps_drv`, `fmradio_drv`, `wlan_drv_gen4m`,
  `wmt_chrdev_wifi`, `fpsgo` on the stock unit), so no `wpa_supplicant`, BT HAL aborts.
- Stage 2e (recovery-partition boot): display and touch as before, and the **backlight is now adjustable**
  from Android (it was on but fixed up to stage 2c/2d at least). Nothing in 2e touches the backlight path
  directly (2e: GPU soft maximum for the initial/PTPOD OPPs); to be confirmed over several boots. In a
  normal (boot-partition) boot, 2e shows multicoloured artefacts and no USB: the LK then starts what it skips
  in recovery mode (SCP, ARM2 fast display), which this tree does not hand over yet.
- Vendor modules (stage 2i): `wmt_drv: disagrees about version of symbol module_layout` had a simple
  cause. The stock kernel was built with clang, this tree with gcc, and genksyms hashes the preprocessed
  declarations with their attributes; clang presents itself as GCC 4.2.1, so `compiler-gcc.h` leaves out
  what it gates on newer GCC versions (`printk` is `__cold` here, not in the stock kernel): 6365 of the
  9674 stock CRCs differed while the structures are the same (sizes of `task_struct` 0xe40, `sk_buff`
  0xe8, `mm_struct` 0x340, `signal_struct` 0x3e0, `inode` 0x238, `file` 0x100, ... read from both images;
  `struct module` 0x300 with `init` at 0x158 and `exit` at 0x2f0 in the stock modules and here).
  Preprocessing with clang for genksyms confirmed it (`printk` then gets the stock CRC) but left the
  task_struct cluster different (textual differences in the reconstructed headers), so
  `CONFIG_AC8257_STOCK_CRCS` now exports the stock CRCs for every symbol the stock kernel exports
  (`tools/ac8257/stock-crcs.awk`, `tools/ac8257/stock/Module.symvers`): 9519 of 9519 shared exports
  match, every import of the vendor modules matches, vermagic identical. Also exported as in the stock
  kernel: `warn_slowpath_null`/`_fmt`, and `MetaZone_ReadBinary`/`SpecWriteBinary`/`Flush` (first step of
  the metazone driver, returning the stock error value until the metazone is read).
- Stage 2i on the unit: **Bluetooth and GPS work** with the stock vendor modules. Wi-Fi does not:
  `insmod wmt_chrdev_wifi.ko` fails with -EBUSY, so `wlan_drv_gen4m` misses the symbols that module
  exports (`wifi_reset_start/end`, `register_file_buf_handler`, `register_set_p2p_mode_handler`,
  `wifi_fwlog_event_func_register`). wmt_chrdev_wifi registers char major 153 (`WIFI_major`), which spidev
  holds here (`SPIDEV_MAJOR` 153 upstream); in the stock kernel spidev is at 163 (`/proc/devices` of both
  captures). Stage 2j: spidev at 163 on AC8257. The display rotation went back to 0 after a reboot: the
  stock `rotationd` sets `persist.sf.hwrotation` at every boot from the metazone (`/dev/mtz`, still
  missing), so the value set by hand does not last; the backlight was fixed again under 2i (it could be
  set under 2e), cause still unknown.
- Stage 2j on the unit: **Wi-Fi works** (all vendor modules loaded: `wlan_drv_gen4m`, `wmt_chrdev_wifi`,
  `gps_drv`, `fmradio_drv`, `bt_drv`, `wmt_drv`, `fpsgo`), client (ping over Wi-Fi) and hotspot; Bluetooth
  paired with a phone and Android Auto started over it. MAC address: random for now (the driver reads it
  from metazone binary 0x10026, 6 bytes, and generates one when that fails). The stock panel driver also
  reads its MIPI init table from the metazone (`jac_lcm_analysis_mipi_params_form_MetaZone` in the stock
  boot log), one more reason for the metazone driver (with `rotationd` and the backlight path).
- FM under 2j: `fmradio_drv` (MT6631) powers up (`mt6631_PowerUp: pwr on seq ok`), tunes and seeks; the
  Jancar radio app (FMLIB) scanned the band: RSSI between -264 and -192 (driver units), 3 stations marked
  valid (90.7, 93.2, 97.4 MHz) at -192, -210, -240, the rest at the noise floor. Either the unit had no
  antenna signal (bench, antenna amplifier not powered) or something in the RF path differs from the stock
  kernel: to compare with a scan under the stock kernel at the same place.
- Touch: the stock image also has an AutoChips `drivers/input/touchscreen/goodix.c` (`goodix,gt928`, DTBO fragment 66 on
  i2c3, nodes `ctp@01`/`ctp@04` with `slave_addr`, `tps-info`, `ti-link`, `ti-serializer = 0x1a`,
  `ti-deserializer = 0x2c`). The panel is behind a TI FPD-Link III serializer (`ds90ub947`/`ds90ub941`,
  `check_serializer_link_ready`, `init_ti_link` in the stock image): the touch controller is only reachable once
  the serializer/deserializer I2C pass-through is set up. The mainline `goodix.c` here does none of it.
- Missing `/dev` nodes against the stock unit: `dualarm-dev`, `tvd`, `di`, `nr`, `wch`, `rdi0`, `mtz`,
  `backcardrv`, `gpios_ioctl`, `video10`, `camera-isp`, `camera-fdvt`, `scp`, `spidev0.0`-`5.0`, and the
  connectivity nodes created by the vendor modules (`wmtdetect`, `stpwmt`, `stpbt`, `stpgps`, `fm`, `wmtWifi`,
  `fw_log_*`, `gps_emi`): no vendor module is loaded (`lsmod` empty; `wmt_loader` waits for `/dev/wmtdetect`).
- `jancar.services` polls I2C 0x40 on i2c6 every 20 ms and gets no ACK (none on the stock kernel): the kernel log
  fills up with it.

AutoChips ext4 mount options (stock `fs/ext4/super.c` token table and `parse_options`): `autoformat` (token 70,
sets a super block flag; `ext4_clear_journal_err` prints "please add autoformat mount option."),
`autorestore=%s` (copies a path) and `permissioncheck` (another flag). The stock fstab mounts `/data` with
`autoformat`, so refusing it fails the mount and Android falls back to its "encrypted" minimal framework.
They are accepted and ignored here (no automatic format of user data).

USB (stock `musb_probe` / `musb_gadget_pullup`): the board has no charger detection, so nothing calls
`mt_usb_connect()`; the stock kernel calls it on the first gadget pull-up (adbd binding the UDC), which this
tree only did with `musb_force_on`. The stock kernel also reads AutoChips options from the command line given
by the LK: `U0_Mod=host|dev` (port 0 mode; `host` switches VBUS on), `U0_Pro=full|high` (speed), `U0_Dis=enable`
(disconnect detection), `U1_Dis`; the UJC201 passes `U0_Mod=dev`. Not handled here yet; the device mode works
without them.

AutoChips display ABI (from the stock `hwcomposer.ac8257.so` and the stock kernel): `disp_input_config` has
one more u32 (136 bytes), `disp_session_info` and `disp_caps_info` 4 more bytes, two more ioctls
(`DISP_IOCTL_GET_EXT_PANEL_INFO` 228, `DISP_IOCTL_SET_FAST_DISP_FLAG` 232), caps report direct link and 2
layers. Without them the HWC's ioctl numbers do not match the kernel's.

Remaining differences seen in the logs: UART2/3 (pins 180-182, `pctl_8` bank), Goodix touch at 3-0001, the
AutoChips devices of the stock `/dev` (`dualarm-dev`, `wch`, `di`, `nr`, `tvd`, `rdi0`, `mtz`, `backcardrv`,
`gpios_ioctl`, `touch`), vendor modules (CRCs).

### Module ABI (CONFIG_MODVERSIONS)

The stock vendor modules (Wi-Fi, BT, GPS, FM...) only load when the CRCs of the symbols they use match.
`tools/ac8257/stock_symvers.py` compares a build with the stock kernel's 9674 exports. Built with GCC, only
3143 CRCs match; preprocessing with clang (as Jancar did) is required (`printk` matches only with clang),
and the remaining differences point to structure changes (missing ATC options, AutoChips edits in core
headers). Converging on these CRCs is the measure of how close the tree is to the stock kernel.

### Next

1. Display shared with ARM2 (fast display, AVM): reconstruct the stock hand-over layer; rotation, glitches,
   backlight path.
2. Vendor modules (Wi-Fi, BT, GPS, FM): symbol CRCs (see Module ABI).
3. AutoChips drivers in order of need: ARM2 (`dualarm-dev`), metazone, touch, UART2/3 pins, video chain.
4. Display: real `lcm_driver_common` (panel text parser from metazone/logo).
5. Module ABI: clang build, then find the type differences behind the CRC mismatches.

## Rule

No subsystem should be copied wholesale from MT6761 and labelled AC8257 without checking registers, Device Tree bindings, configuration symbols, driver interfaces and stock binary evidence.
