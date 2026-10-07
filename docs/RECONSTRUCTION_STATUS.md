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

1. Android up to the launcher: USB/adb on the test kernel, then why the Jancar packages under `/vendor/app` are
   not found (full logcat over adb).
2. AutoChips drivers in order of need: ARM2 (`dualarm-dev`), metazone, touch, UART2/3 pins, video chain.
3. Display: real `lcm_driver_common` (panel text parser from metazone/logo).
4. Module ABI: clang build, then find the type differences behind the CRC mismatches.

## Rule

No subsystem should be copied wholesale from MT6761 and labelled AC8257 without checking registers, Device Tree bindings, configuration symbols, driver interfaces and stock binary evidence.
