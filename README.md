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

Tested on the unit with the test images of `docs/BUILD.md` (last test: stage 2t). "Untested" means the
driver loads but nobody has checked the function yet.

| Function | Status | Notes |
|---|---|---|
| Boot to the Android UI | ✅ Works | stage 1p; stock Android userspace, `system_server`, launcher |
| eMMC, partitions, `/data` (ext4) | ✅ Works | AutoChips ext4 mount options (`autoformat`...) accepted |
| GPU (PowerVR GE8300) | ✅ Works | 32-bit DMA on MT6761: its own allocations and the ION buffers it renders into are kept below 4 GiB (stage 2o); OPPs 300-730 MHz (stage 2q) |
| Trusty TEE, keymaster | ✅ Works | shared buffers below 4 GiB |
| Display output (HWC, frame buffer) | ✅ Works (stage 2o) | no artefacts on the panel or in scrcpy captures and no userspace crashes with 6 GiB of RAM since ION buffers stay below 4 GiB (`ac8257_ion_low`); rotation set at boot by `rotationd` from the metazone (stage 2l). Normal-boot hand-over with ARM2 (fast display) not reconstructed |
| Backlight | ⚠️ Works on most boots | adjustable from Android (disp PWM, as the stock kernel on this unit: no TI bridge answers); on some boots it does not follow the slider, cause not found yet (2n with 3 GiB too) |
| USB device mode, adb | ✅ Works | connect on the first gadget pull-up |
| Touch (Goodix GT928 behind FPD-Link) | ✅ Works | interrupt on EINT 42 since stage 2r (checked against polling at boot, safety poll once a second, falls back to polling every 16 ms if the interrupt fails), orientation fixed; follows the display rotation (`persist.sf.hwrotation=90`) |
| `/dev/gpios_ioctl` (Jancar GPIOs) | 🧪 Untested | device present (stage 1t); used by `com.jancar.services` |
| Keys (`mtk-kpd`), IR receiver | 🧪 Untested | input devices present |
| Audio | 🧪 Untested | sound card registers |
| Bluetooth | ✅ Works (stage 2i) | stock vendor modules (`wmt_drv`, `bt_drv`) load: stock symbol CRCs exported (`CONFIG_AC8257_STOCK_CRCS`); phone paired, Android Auto starts over it |
| GPS | ✅ Works (stage 2i) | stock `gps_drv` |
| Wi-Fi | ✅ Works (stage 2j) | stock `wmt_chrdev_wifi` + `wlan_drv_gen4m` (spidev moved to char major 163 as in the stock kernel); client and hotspot work; MAC address from the metazone (binary 0x10026, `00:08:22:…` confirmed) since stage 2l, random before |
| FM radio | ⚠️ Driver works, reception weak (stage 2j) | stock `fmradio_drv` (MT6631 FM): power-up, tuning and seek work; a full scan found 3 stations at very low RSSI (noise floor ~-25): antenna, or its supply, to compare with the stock kernel |
| Rear camera, AV-in, AVM (TVD, DI, NR, WCH, backcar) | ❌ Not working | AutoChips drivers missing; i2c6 device 0x40 does not answer |
| Metazone (`/dev/mtz`, kernel API) | ✅ Works (stage 2l) | reads the copy the LK loads at 0x60700000 (format checked against a dump of the unit's `metazone` partition); writes are kept in memory only, nothing is written back to the eMMC |
| ARM2 (`dualarm-dev`) | ❌ Not working | AutoChips driver missing |
| UART2/3, `spidev` | ✅ Registered (stage 2t) | spidev: six devices since stage 2s (`CONFIG_SPI_MT65XX`, 32-bit DMA, `Autochips,spidev` of the stock DTBO), named `spidev0.0`-`5.0` from stage 2t as on the stock unit (DT `busnum`); UART2/3: pins 179-182 added (2s), the probe then failed with -2 on the missing bus clocks `ifr_uart2`/`ifr_uart3`, added in 2t: ttyS2/ttyS3 now registered (ST16650V2, IRQ 228/229, as on the stock unit); nothing known is wired to them yet, no data exchanged |
| MCU / CAN | ✅ Works | `/dev/ttyS1` (115200 8N1), used by `jancar.services` / LibreHU service |
| Suspend / resume | ❓ Unknown | not checked yet |
| Google Play services | ⚠️ Partial | GMS crashes seen (IllegalArgumentException), unrelated to the kernel as far as seen |
| Boot from the recovery partition | ⚠️ Retries | often several tries (black screen, cold reset) before Android; the try that works follows a stock boot; under investigation |
| KernelSU Next | ✅ Works | KernelSU Next manager v3.4.0 (the official KernelSU app refuses non-GKI kernels) |

## Roadmap

1. **Stage 1, boot the stock ROM** - ✅ done (stage 1p).
2. **Stage 2, usable on the stock ROM**
   - [x] Touch (GT928, polling, orientation); EINT 42 interrupt (stage 2r, works)
   - [x] Jancar `/dev/gpios_ioctl`
   - [x] Memory corruption above 4 GiB (artefacts, crashes): ION buffers below 4 GiB (stage 2o)
   - [ ] Display shared with ARM2 (fast display / AVM hand-over of the stock kernel) for the normal boot
   - [ ] Boot retries from the recovery partition (cold resets before Android); also blocks the boot partition (stage 2s: stuck at the logo, resets)
   - [x] Display rotation: metazone driver (`/dev/mtz`, stage 2l, works), read by `rotationd` to set `persist.sf.hwrotation`
     (workaround confirmed on the unit: `su -c "setprop persist.sf.hwrotation 90 && stop && start"`)
   - [x] Backlight control: adjustable since stage 2e (recovery-partition boot); to confirm in a normal boot
   - [x] Vendor modules load (stock symbol CRCs, `CONFIG_AC8257_STOCK_CRCS`): BT and GPS work
   - [x] Wi-Fi (client and hotspot, stage 2j)
   - [ ] FM reception to compare with the stock kernel; metazone driver (stage 2l: Wi-Fi MAC address, display rotation; panel settings next)
   - [ ] Audio, keys, IR: check on the unit
   - [x] `spidev` (stage 2s, works)
   - [x] UART2/3 registered (pins 179-182, bus clocks: stage 2t); no known user to test traffic with
   - [ ] AutoChips devices: ARM2 (`dualarm-dev`), rear camera / AV-in / AVM (TVD, DI, NR, WCH, backcar)
   - [ ] Suspend / resume
3. **Stage 3, daily use**
   - [ ] Remove the bring-up options (early pstore console, timed panic) from the release configuration
   - [ ] Install in the boot partition instead of the recovery test method (stage 2s boot-partition image: stuck at the logo, see the boot retries)
   - [ ] Release images from the GitHub Actions build
   - [x] CPU governors (interactive default, schedutil, conservative...); CPU/GPU overclock tables (CPU up to
     2.3 GHz, GPU 300-730 MHz), capped at the stock maxima by default, settable from Kernel Adiutor /
     SmartPack (`scaling_max_freq`/`scaling_min_freq`, `/sys/devices/platform/dfrgx/devfreq/dfrgx/`), see `docs/BUILD.md`
   - [x] Touch input boost, zram lz4, deadline I/O scheduler
   - [x] KernelSU Next (v3.4.0-legacy, manual hooks), works with the KernelSU Next v3.4.0 manager
4. **Later**: real panel driver from the metazone description, cleanup of the AutoChips code
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
