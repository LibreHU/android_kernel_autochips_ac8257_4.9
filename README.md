# LibreHU AC8257 Linux 4.9 Kernel Reconstruction

This repository is the working base for reconstructing an AutoChips AC8257 Linux 4.9 kernel tree from publicly available MediaTek sources, the UJC201 stock firmware, and reverse-engineering evidence collected from the target device.

## Current status

The repository currently contains a Linux 4.9.117 kernel source tree.

The top-level Makefile identifies the kernel as Linux 4.9.117.

The repository is therefore being treated as the **kernel reconstruction base**, not as an already recovered official AutoChips BSP.

No claim is made that the current tree is an original AutoChips AC8257 source release.

**Progress (stage 1, stock ROM boot):** a kernel built from this tree boots on the UJC201 and runs the stock
Android 9 userspace (init, services, GPU, Trusty, `system_server`, display HAL). USB/adb work, `/data` mounts and **Android shows its
UI on the panel**; the individual functions are being checked next.
Details and the list of AutoChips-specific changes found so far: `docs/RECONSTRUCTION_STATUS.md`; building
and testing safely on the unit: `docs/BUILD.md`.

## Hardware status (UJC201, stock Android ROM, recovery test method)

Tested on the unit with the test images of `docs/BUILD.md` (last test: stage 1v). "Untested" means the
driver loads but nobody has checked the function yet.

| Function | Status | Notes |
|---|---|---|
| Boot to the Android UI | ✅ Works | stage 1p; stock Android userspace, `system_server`, launcher |
| eMMC, partitions, `/data` (ext4) | ✅ Works | AutoChips ext4 mount options (`autoformat`...) accepted |
| GPU (PowerVR GE8300) | ✅ Works | allocations below 4 GiB, as the stock kernel |
| Trusty TEE, keymaster | ✅ Works | shared buffers below 4 GiB |
| Display output (HWC, frame buffer) | ⚠️ Partial | picture shown; **rotated by 90°**, glitches, occasional crash (display shared with ARM2, stopgap in stage 1w) |
| Backlight | ⚠️ Partial | on, **not adjustable** (set by the panel driver through the TI DS90UB947 bridge) |
| USB device mode, adb | ✅ Works | connect on the first gadget pull-up |
| Touch (Goodix GT928 behind FPD-Link) | ✅ Works | polling every 16 ms (no interrupt wired), orientation fixed |
| `/dev/gpios_ioctl` (Jancar GPIOs) | 🧪 Untested | device present (stage 1t); used by `com.jancar.services` |
| Keys (`mtk-kpd`), IR receiver | 🧪 Untested | input devices present |
| Audio | 🧪 Untested | sound card registers |
| Wi-Fi, Bluetooth, GPS, FM | ❌ Not working | stock vendor modules do not load (symbol CRCs); BT HAL aborts |
| Rear camera, AV-in, AVM (TVD, DI, NR, WCH, backcar) | ❌ Not working | AutoChips drivers missing; i2c6 device 0x40 does not answer |
| ARM2 (`dualarm-dev`), metazone (`mtz`) | ❌ Not working | AutoChips drivers missing |
| UART2/3, `spidev` | ❌ Not working | pins 180-182 (`pctl_8` bank) and SPI devices not set up |
| MCU / CAN | ❓ Unknown | not checked yet |
| Suspend / resume | ❓ Unknown | not checked yet |
| Google Play services | ❓ Unknown | crash loop seen (`co.g.App`), to compare with the stock kernel |

## Roadmap

1. **Stage 1, boot the stock ROM** - ✅ done (stage 1p).
2. **Stage 2, usable on the stock ROM**
   - [x] Touch (GT928, polling, orientation)
   - [x] Jancar `/dev/gpios_ioctl`
   - [ ] Display shared with ARM2 (fast display / AVM hand-over of the stock kernel): crashes, glitches
   - [ ] Display rotation: metazone driver (`/dev/mtz`), read by `rotationd` to set `persist.sf.hwrotation`
   - [ ] Backlight control: real `lcm_driver_common` with the DS90UB947/941 bridge (`CONFIG_LCM_TRANSFER_IC_SUPPORT`)
   - [ ] Wi-Fi / BT / GPS / FM: build with clang, converge on the stock symbol CRCs so the vendor modules load
   - [ ] Audio, keys, IR: check on the unit
   - [ ] MCU / CAN, UART2/3 (pins 180-182)
   - [ ] AutoChips devices: ARM2 (`dualarm-dev`), metazone, rear camera / AV-in / AVM (TVD, DI, NR, WCH, backcar)
   - [ ] Suspend / resume
3. **Stage 3, daily use**
   - [ ] Remove the bring-up options (early pstore console, timed panic) from the release configuration
   - [ ] Install in the boot partition instead of the recovery test method
   - [ ] Release images from the GitHub Actions build
   - [x] CPU governors (schedutil, conservative added); CPU/GPU overclock tables loaded, capped at 2.0 GHz /
     660 MHz by default, raised as root (`docs/BUILD.md`)
   - [x] Touch input boost, zram lz4, deadline I/O scheduler
   - [x] KernelSU Next (v3.4.0-legacy, manual hooks), to be tested with its manager
4. **Later**: real panel driver from the metazone description, touch interrupt, cleanup of the AutoChips code
   for review.

## Target platform

- SoC: AutoChips AC8257.
- CPU: 4 × ARM Cortex-A53.
- GPU: PowerVR GE8300.
- Android target: Android 9.
- Target project: `ac8257_demo`.
- Target device: Jancar UJC201 / UJC201_64.
- Stock kernel: Linux 4.9.117+.
- PMIC: MediaTek MT6357.
- Wireless/CONSYS: AutoChips AC8257 / MediaTek-compatible CONSYS stack.

## Reconstruction method

The starting point is a public MediaTek Linux 4.9 tree with the closest known software architecture to AC8257.

AC8257-specific hardware support will be reconstructed progressively from the stock kernel, DTB/DTBO, kernel configuration, proprietary modules, preloader/LK strings, and the AC8257 BSP structure documented by Chinese development articles.

The repository must distinguish between code directly recovered from the target, code adapted from a public reference tree, and code that is still inferred.

## Main work areas

1. AC8257 architecture and platform definitions.
2. Device Tree and DT bindings.
3. Clock and reset controllers.
4. Pin control and GPIO.
5. EMI and DRAMC.
6. MT6357 PMIC integration.
7. MMC/eMMC and storage.
8. USB and peripheral controllers.
9. CONSYS / Wi-Fi / Bluetooth / FM / GPS.
10. GPU integration.
11. Camera / ISP.
12. MDP / display pipeline.
13. TVD / CVBS / WCH-DI.
14. ARM2 firmware and Linux↔ARM2 integration.
15. Backcar / AVM integration.
16. AC8257-specific framebuffer and display glue.

## Evidence policy

Every reconstructed subsystem should have a source note describing where the implementation came from.

Possible evidence levels are **confirmed**, **strong inference**, **reference implementation**, and **unknown**.

A public MT6761 implementation must not be presented as native AC8257 code merely because the two platforms share software components.

The stock scatter file identifies `MT6761` as its platform field, but this is not treated as proof that the physical SoC is MT6761.

The stock kernel Device Tree identifies the target as `AC8257`, and the target kernel configuration contains AC8257-specific symbols such as `CONFIG_MACH_AC8257` and `CONFIG_MTK_PLATFORM="ac8257"`.

## Reference sources

- MediaTek MT6761 Linux 4.9 kernel trees are used as the primary public skeleton.
- `tb8766p1_64_bsp` trees are used as a secondary MT6761/MT8766-related reference where useful.
- Public AutoChips AC8227L sources are used only for historical AutoChips driver patterns.
- The public AC8257 device-tree/TWRP repositories are used as device-specific structural references.
- The stock UJC201 firmware remains the authoritative reference for the actual target hardware.

## Directory documentation

Detailed reconstruction notes are maintained under `docs/`.

See `docs/RECONSTRUCTION_STATUS.md` for the current subsystem matrix and evidence status.

See `docs/SOURCES.md` for public source repositories and their intended role.
