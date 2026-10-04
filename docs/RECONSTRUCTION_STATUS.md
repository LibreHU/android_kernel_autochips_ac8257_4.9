# AC8257 Reconstruction Status #!

## Objective #!

Rebuild a usable Linux 4.9 kernel source tree for the AutoChips AC8257 platform used by the Jancar UJC201 #!

## Baseline #!

The current repository is a Linux 4.9.117 source tree #!

The target stock kernel is also Linux 4.9.117+ #!

This version match is important because it minimizes unrelated kernel API differences during the reconstruction #!

## Target evidence #!

| Area | Status | Evidence | Notes #! |
|---|---|---|---|
| Kernel version | Confirmed | Stock kernel + repository Makefile | Both identify Linux 4.9.117 #! |
| AC8257 platform | Confirmed | Stock DTB/config | `model = AC8257`, `compatible = mediatek,AC8257`, `CONFIG_MACH_AC8257=y` #! |
| CPU topology | Confirmed | Stock DTB / interrupts | Four Cortex-A53 CPU nodes are present and CPUs 0-3 are active #! |
| MT6357 | Confirmed | Stock config/DT | `CONFIG_MTK_PMIC_CHIP_MT6357=y` #! |
| AC8257 pinctrl | Confirmed | Stock config/DT | `CONFIG_PINCTRL_AC8257=y` #! |
| AC8257 clocks | Confirmed | Stock config/DT | `CONFIG_COMMON_CLK_AC8257=y` #! |
| GPU | Confirmed | Stock config | PowerVR RGX Clark configuration is enabled #! |
| IOMMU/SMI | Confirmed | Stock config | MTK IOMMU V2 and SMI are enabled #! |
| Camera/ISP | Confirmed | Stock config/DT | MTK camera ISP support is enabled, with AC8257-specific DT nodes #! |
| Video codec | Confirmed | Stock config | MTK video codec driver is enabled #! |
| ARM2 | Confirmed | Stock DT/BSP evidence | ARM2 firmware and ARM2-related DT nodes exist #! |
| MDP | Confirmed | Stock DT | AC8257 MDP-related hardware nodes exist #! |
| TVD/CVBS | Confirmed | Stock DT | AC8257-specific TVD/CVBS nodes exist #! |
| WCH-DI | Confirmed | Stock DT | AC8257-specific WCH-DI node exists #! |
| Proprietary display glue | Missing from public skeleton | Stock DT + CSDN BSP evidence | Requires reconstruction or compatible source #! |
| Full AC8257 BSP | Not recovered | CSDN source fingerprints | Original Gitolite repository is internal and not publicly mirrored in the sources found so far #! |

## Known proprietary modules #!

The stock vendor image contains non-stripped AC8257-related kernel modules including wireless, Bluetooth, FM, GPS, MET and performance components #!

Known modules include `wmt_drv.ko`, `wmt_chrdev_wifi.ko`, `wlan_drv_gen4m.ko`, `bt_drv.ko`, `fmradio_drv.ko`, `gps_drv.ko`, `met.ko`, and `fpsgo.ko` #!

The modules contain source filename strings and AC8257-compatible identifiers that can be used to map missing source files against public MediaTek trees #!

## Known BSP structure #!

CSDN documentation describes a real AC8257 Android 9 BSP containing `kernel-4.9`, preloader, LK, ARM2 and proprietary AutoChips components #!

The documented private repository was `ac8257Project/ac8257.git` on an internal Gitolite server, branch `master-8257-yige` #!

The documented kernel Device Tree path is `kernel-4.9/arch/arm64/boot/dts/mediatek/ac8257_demo.dts` #!

The documented preloader tree contains an `ac8257` platform directory with EMI, DRAMC, GPIO, ARM2 loading, download, platform and PMIC sources #!

## Priority order #!

1. Make the repository structurally match the target AC8257 platform #!
2. Reconstruct the AC8257 configuration and Device Tree #!
3. Port clocks, pinctrl, PMIC, EMI and DRAMC #!
4. Recover or reconstruct ARM2 integration #!
5. Reconstruct display, camera, MDP, TVD, CVBS and WCH-DI #!
6. Reconstruct GPU integration #!
7. Reintegrate CONSYS and peripheral drivers #!
8. Build a bootable test kernel only after the platform layer is coherent #!

## Rule #!

No subsystem should be copied wholesale from MT6761 and labelled AC8257 without checking registers, Device Tree bindings, configuration symbols, driver interfaces and stock binary evidence #!
