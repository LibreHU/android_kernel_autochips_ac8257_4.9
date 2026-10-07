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
- Stage-1 `lcm_driver_common` placeholder (reports 1024x600 DSI video, never touches the panel).
- No battery: no bcct cooler (`ATC_DISABLE_BAT_CHAGE`), as in the stock kernel.
- Build fixes: host dtc with GCC >= 10, gcc 4.9 false positives, missing `gether.no_skb_reserve`, GPU DVFS
  passing a device index where this DDK wants the device node (real bug, clang refuses it).
- Builds and links with the AOSP GCC 4.9 toolchain: `Image.gz-dtb` (see `docs/BUILD.md`). **Not tested on
  the device yet.**

### Module ABI (CONFIG_MODVERSIONS)

The stock vendor modules (Wi-Fi, BT, GPS, FM...) only load when the CRCs of the symbols they use match.
`tools/ac8257/stock_symvers.py` compares a build with the stock kernel's 9674 exports. Built with GCC, only
3143 CRCs match; preprocessing with clang (as Jancar did) is required (`printk` matches only with clang),
and the remaining differences point to structure changes (missing ATC options, AutoChips edits in core
headers). Converging on these CRCs is the measure of how close the tree is to the stock kernel.

### Next

1. Boot test on the UJC201, then fix what fails (dmesg / last_kmsg).
2. Module ABI: clang build, then find the type differences behind the CRC mismatches.
3. Display: real `lcm_driver_common` (panel text parser), then the AutoChips drivers in order of need.

## Rule

No subsystem should be copied wholesale from MT6761 and labelled AC8257 without checking registers, Device Tree bindings, configuration symbols, driver interfaces and stock binary evidence.
