# Building and testing the AC8257 kernel

## Toolchain

The stock kernel was built by Jancar with AOSP clang 6.0.2 (`clang-4691093`, r316199) and the AOSP
aarch64 GCC 4.9 binutils (see `reference/stock-version.txt`).

- GCC 4.9 (binutils, and a working compiler): the AOSP prebuilt, mirrored by LineageOS:
  ```
  git clone --depth 1 -b lineage-17.1 https://github.com/LineageOS/android_prebuilts_gcc_linux-x86_aarch64_aarch64-linux-android-4.9 gcc49
  export PATH=$PWD/gcc49/bin:$PATH
  ```
- Clang: needed for module compatibility with the stock kernel (`CONFIG_MODVERSIONS` CRCs are computed on
  the preprocessed source, and clang's predefined macros give the stock CRCs: `printk` matches only with
  clang). Any recent clang works for that; `clang-4691093` is the exact one.

Host GCC 10 or newer needs the `scripts/dtc` fix already in this tree (`extern YYLTYPE yylloc`).

## Build

```
export PATH=/path/to/gcc49/bin:$PATH
make O=out ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- ac8257_demo_defconfig
make O=out ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- -j$(nproc) Image.gz-dtb
# with clang (stock-compatible CRCs):
make O=out ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- CC=clang CLANG_TRIPLE=aarch64-linux-gnu- HOSTCC=gcc -j$(nproc) Image.gz-dtb
```

Outputs:

- `out/arch/arm64/boot/Image.gz-dtb`: kernel + `ac8257.dtb` (the stock device tree, decompiled into
  `arch/arm64/boot/dts/mediatek/ac8257.dts`; the rebuilt DTB is byte-identical to the stock one).
- `out/arch/arm64/boot/dts/mediatek/UJC201_64.dtb`: the board overlay (stock `dtbo.img` content, also
  byte-identical). The stock `dtbo.img` partition does not need to change.

## Boot test (UJC201)

1. Keep a copy of the stock `boot.img` (from the firmware package, or `dd` of the boot partition) and a
   way back: SP Flash Tool with the stock scatter, or TWRP (recovery is untouched).
2. Put the new kernel into the stock boot image; ramdisk, cmdline, addresses and the AVB footer stay as
   they are (the stock boot partition ends with an AVB footer and its own vbmeta: without it the LK
   refuses the image and the unit reboots at the logo, with no kernel log):
   ```
   tools/ac8257/repack_boot.py boot-stock.img out/arch/arm64/boot/Image.gz-dtb boot-test.img
   ```
3. Flash it. `fastboot boot` does not work on this unit (the LK reboots instead of starting the image).
   Safest for a kernel that may not boot: the **recovery** partition, with the stock recovery vbmeta:
   ```
   tools/ac8257/repack_boot.py boot-stock.img out/arch/arm64/boot/Image.gz-dtb recovery-test.img \
       --vbmeta <device tree>/prebuilt/avb/recovery_stock_vbmeta_250718.bin --size 33554432 \
       --cmdline-append "init=/init"
   fastboot flash recovery recovery-test.img
   adb reboot recovery
   ```
   (`init=/init`: in recovery mode the LK does not add it, and without it the kernel mounts system as
   root and then panics with no init.) A crash then ends in a normal boot (stock boot partition), with the RAM, so the log, kept. Flash
   TWRP back to recovery afterwards. Flashing `boot` directly works too, but a kernel that loops can
   only be replaced with SP Flash Tool, which reinitialises the RAM and loses the log.
4. What to look at: `adb logcat`, `adb shell dmesg`, `/proc/last_kmsg` (or
   `/sys/fs/pstore/console-ramoops*`) after a failed boot, and the UART console (`ttyS0`, 921600 8N1) when
   wired.

   During bring-up the defconfig sets `CONFIG_AC8257_EARLY_PSTORE_CONSOLE=y`: the kernel log is copied
   into the pstore console zone from the start of `setup_arch()`, before ramoops registers, so a kernel that dies
   early still leaves its log. After the failed boot, boot the stock kernel and read
   `/sys/fs/pstore/console-ramoops*` (or `/proc/last_kmsg`): this kernel's lines follow the marker
   `==== ac8257 early pstore console ====` (banner `root@vm`, `gcc version 4.9`). Lines `ac8257: setup_arch: ...` mark the early boot steps. No marker means it died before
   `setup_arch()` (or was never started). Set the option to N once the kernel boots.

   `CONFIG_AC8257_BRINGUP_PANIC` (also on during bring-up) keeps such tests from ending in a loop: it panics
   `CONFIG_AC8257_BRINGUP_PANIC_SECS` after boot, turns any reboot asked by userspace (Rescue Party,
   `reboot recovery`...) into the same panic, and clears the recovery command (`boot-...`) from the
   bootloader message in `para` (the misc partition of this unit). The MTK exception reboot then starts
   the normal boot partition, with the log kept. Read it from Android with
   `cat /sys/fs/pstore/console-ramoops*`, `cat /proc/last_kmsg` and `logcat -L` (userspace log, only the
   last 64 KiB). `ac8257_panic_secs=N` on the command line (`--cmdline-append`) overrides the delay
   (0: no timed panic), e.g. a longer window for a live `adb` session. At run time, as root:

       cat /sys/module/ac8257_bringup/parameters/time_left             # seconds left, 0 = disarmed
       echo 1800 > /sys/module/ac8257_bringup/parameters/panic_secs    # panic 30 min from now
       echo 0 > /sys/module/ac8257_bringup/parameters/panic_secs       # no timed panic
       echo 0 > /sys/module/ac8257_bringup/parameters/intercept_reboot # adb reboot reboots normally

   Not kept across reboots.

   Normal-mode (boot partition) tests: a kernel booted from the recovery partition runs in recovery
   mode, where the LK skips the SCP, its reserved memory and ARM2's fast display; a normal boot needs the
   test kernel in the boot partition, with no fallback if it hangs. `ac8257_bringup.oneshot=1` on its
   command line gives one: the kernel writes "boot-recovery" into the bootloader message (`para`) at 10 s
   and 30 s, and its timer (or an intercepted reboot) restarts with the "recovery" command, which the
   MTK restart handler marks in the RTC as `adb reboot recovery` does (stage 2g panicked instead, and
   after a panic reboot the LK ignores the bootloader message: it looped on the boot partition). The next
   start is then from the recovery partition, which must hold a kernel known to boot to adb (a recovery test image; it clears
   the message again). Procedure: test image in recovery, `boot_oneshot_*.img` in boot, reboot; the unit
   comes back in the recovery test kernel with the normal-mode log in pstore (`get_log.bat`); then put
   the stock boot.img back (`dd` of the stock image to `/dev/block/by-name/boot`, or fastboot).
   Keep a copy of `para` before testing (`dd if=/dev/block/by-name/para of=...`).

   Recovery-partition tests that hang now and then (stage 2l: black screen then reset, about one boot
   in five reached Android): `ac8257_bringup.retry=1` on the command line (stage 2m images) sets the
   RTC recovery flag at boot and clears it after `ac8257_bringup.retry_secs` seconds (90). A boot that
   hangs before that is reset by the hardware watchdog and starts the recovery partition again directly,
   instead of the stock normal boot that overwrites the RAM console and pstore, so the next boot keeps
   the failed boot's log (`/proc/last_kmsg`, `/sys/fs/pstore`). A test kernel that hangs at every boot
   loops until SP Flash Tool flashes the recovery partition.
   Stage 2m looped this way on the unit (no boot reached Android without a stock boot before it): keep
   it off.

## CPU/GPU frequencies, governors

The overclock tables are loaded by default, with the stock maxima as run-time caps:

- CPU: 16 OPPs (the SSPM record table has 16 entries) from 987 MHz to 2.301 GHz: the stock "FY"
  voltages up to 2.001 GHz, then 2.101, 2.201 and 2.301 GHz at the same 1.025 V top voltage (no voltage
  above the stock maximum; 2.301 GHz took the place of 850 MHz). Ceiling: `ac8257_cpufreq.max_khz`,
  **2001000** by default; `ac8257_cpufreq.boot_khz` (0 = max_khz) is the maximum in force at boot. CPU
  DVFS is "hybrid" on this SoC (`CONFIG_HYBRID_CPU_DVFS`): the SSPM applies the OPPs from the record table
  (`xrecordTbl`, `mtk_cpufreq_opp_pv_table.h`, `ocTbl` for this level) and the limits given by
  `cpuhvfs_set_min_max()`, where the ceiling is applied. The PTPOD OPP, which must be at the 0.80 V boot
  voltage, is index 11 of this table (1.4 GHz; stage 2b hung in the EEM init with a wrong index). The EEM
  voltage adjustments, calibrated against the stock table, are not applied with this table (sign-off
  voltages instead, slightly higher).
- GPU: 730 MHz (the MT6761T top OPP of this GPU) at the 0.80 V of the stock 660 MHz, plus 600, 450 and
  300 MHz at the voltage of the stock OPP above each (730/660/600/500/450/390/300 MHz). Ceiling:
  `ac8257_gpufreq.max_khz`, **660000** by default; floor: `ac8257_gpufreq.min_khz` (0 = none). The boot
  OPP and the PTPOD OPP stay at 660 MHz or below even with a 730 MHz ceiling (stage 2d, which started the
  GPU at 730 MHz, had GPU EMI MPU violations).

Tuning apps (Kernel Adiutor, SmartPack Kernel Manager...):

- CPU: `scaling_max_freq` / `scaling_min_freq` / `scaling_governor` of
  `/sys/devices/system/cpu/cpu0/cpufreq/` work: a cpufreq policy notifier folds the policy limits into
  the SSPM limits (the PPM ignores them otherwise), within `max_khz`. `scaling_available_frequencies`
  lists the whole table.
- GPU: the PowerVR devfreq layout these apps know, `/sys/devices/platform/dfrgx/devfreq/dfrgx/`
  (kHz): `cur_freq`, `available_frequencies`, `max_freq` and `min_freq` (the `max_khz` / `min_khz`
  limits); `governor` reports the MediaTek GPU DVFS (`mtk_ged`), writes ignored.

As root, at run time (not kept across reboots):

    echo 2301000 > /sys/module/ac8257_cpufreq/parameters/max_khz   # CPU ceiling 2.3 GHz
    echo 1800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
    echo 730000 > /sys/devices/platform/dfrgx/devfreq/dfrgx/max_freq
    echo 500000 > /sys/devices/platform/dfrgx/devfreq/dfrgx/min_freq
    echo 0 1533000 > /proc/ppm/policy/userlimit_max_cpu_freq       # lower limit through the PPM

or on the kernel command line (`--cmdline-append`): `ac8257_cpufreq.max_khz=2301000`,
`ac8257_cpufreq.boot_khz=2201000`, `ac8257_gpufreq.max_khz=730000`. `ac8257_cpu_oc=0` /
`ac8257_gpu_oc=0` select the stock tables (with EEM). Stability above the stock frequencies depends on
the chip and is not guaranteed (thermal throttling still applies).

Stage 2q images (`recovery_test_ac8257_stage2q_oc`): CPU ceiling 2.301 GHz with 2.201 GHz at boot, GPU
730 MHz, interactive governor and touch boost (defaults), no timed panic (`ac8257_panic_secs=0`).

Stage 2s images (UART2/3 and `spidev`), both with the stage 2q settings (CPU 2.301 GHz, 2.201 GHz at
boot, GPU 730 MHz, no timed panic, `adb reboot` not intercepted):

- `recovery_ac8257_stage2s.img`: recovery partition, as before (`init=/init`).
- `boot_ac8257_stage2s.img`: boot partition (10 MiB, stock boot vbmeta kept), for a normal-mode boot.
  Stage 2e showed artefacts and no USB in a normal boot (SCP and ARM2 started by the LK, not handed
  over); 2e predates the 4 GiB fix of stage 2o, so the artefacts may be gone, USB is still to check.
  Keep the stock `boot.img` at hand: `adb reboot recovery` (or `su -c "reboot recovery"` in a terminal
  on the unit) starts the recovery test kernel, and from there
  `dd if=boot-stock.img of=/dev/block/by-name/boot`; SP Flash Tool otherwise.

On the unit: the recovery image boots, the six spidev devices appear (`spidev32761.0`-`32766.0`, bus
numbers fixed in 2t), ttyS2/ttyS3 still fail (-2, bus clocks, fixed in 2t). The boot-partition image
stays at the logo with resets now and then, like the failed recovery tries (no stock kernel before it,
see RECONSTRUCTION_STATUS.md); way back: SP Flash Tool, download only, stock `boot.img` in boot.

Stage 2t image (`recovery_ac8257_stage2t.img`, recovery partition only, same settings): UART2/3 bus
clocks, spidev bus numbers.

Checks (as root):

    ls -l /dev/ttyS* /dev/spidev*
    cat /proc/tty/driver/serial                       # uart:... port:... for ttyS2 and ttyS3
    dmesg | grep -i -E "11004000|11005000|ttyS|spi|pinctrl"
    cat /sys/kernel/debug/pinctrl/1000b000.pinctrl/pinmux-pins | grep -E "pin 1(79|8[0-2])"

CPU governors: interactive (default), schedutil, conservative, ondemand, performance, powersave,
userspace, schedplus (`scaling_governor`). I/O schedulers: deadline (default), cfq, noop.

Other tweaks (kept to what is measurable and leaves the vendor module ABI alone):

- Touch interrupt (stage 2r): `goodix.c` tries EINT 42 next to polling and keeps it only if it delivers
  the touches (see RECONSTRUCTION_STATUS.md); `/sys/module/goodix/parameters/irq_mode` tells the mode,
  `eint_gpio=-1` on the command line (`goodix.eint_gpio=-1`) forces polling, `poll_ms` sets the period.
- Touch input boost (`CONFIG_AC8257_INPUT_BOOST`): on a new touch, CPU minimum raised to 1.533 GHz for
  100 ms through the PPM system boost (its unused `BOOST_BY_UT` user, so the PPM API the vendor modules
  use is unchanged). `/sys/module/ac8257_input_boost/parameters/freq_khz` (0 disables), `duration_ms`.
- zram compresses with lz4 by default (lzo upstream): 1 GiB zram swap set up by `fstab.enableswap`.
- deadline as the default I/O scheduler (cfq before) for the eMMC.
- Not needed: `slub_debug=OFZPU page_owner=on` given by the LK are no-ops (`CONFIG_SLUB_DEBUG`,
  `CONFIG_PAGE_OWNER` off). Config/debug cleanup that changes structure layouts waits for the module ABI work.

## KernelSU Next

`drivers/kernelsu` is the KernelSU-Next kernel driver, tag `v3.4.0-legacy` (the branch for non-GKI
kernels), vendored without its git history (`drivers/kernelsu/README.ac8257`); version pinned to 33294 /
`v3.4.0-legacy` in its Kbuild, the value upstream derives from its git history, so the manager recognises it.
`CONFIG_KSU=y`, `CONFIG_KSU_MANUAL_HOOK=y` (no kprobes on this 4.9 kernel). Manual hooks, under `CONFIG_KSU`:

| Hook | Where |
|---|---|
| `ksu_handle_execveat` / `ksu_handle_execveat_sucompat` | `do_execveat_common`, `fs/exec.c` |
| `ksu_handle_faccessat` | `faccessat`, `fs/open.c` |
| `ksu_handle_sys_read` | `read`, `fs/read_write.c` |
| `ksu_handle_stat`, `ksu_handle_newfstat_ret`, `ksu_handle_fstat64_ret` | `vfs_fstatat`, `newfstat`, `fstat64`, `fs/stat.c` |
| `ksu_handle_input_handle_event` | `input_handle_event`, `drivers/input/input.c` (safe mode: volume down) |
| `ksu_handle_sys_reboot` | `reboot`, `kernel/reboot.c` (supercalls, before the capability check) |
| `ksu_handle_slow_avc_audit` | `slow_avc_audit`, `security/selinux/avc.c` |

Linux 4.9 adaptations, all inside `drivers/kernelsu`: `compat/k49/` (newer headers: `linux/sched/*.h`,
`linux/overflow.h`, `linux/compiler_types.h`; `kvmalloc`/`kvcalloc`, `strscpy_pad`, 4.14-style
`kernel_read`/`kernel_write` mapped to the driver's compat helpers), `ns_get_path()` returning a pointer,
no clang-only warning flags, `-Wno-error` for this directory (its pointer/integer conversions are warnings
upstream). The backports its Kbuild applies to the kernel (`can_umount`/`path_umount` in
`fs/namespace.c`, `selinux_inode()`/`selinux_cred()` in `security/selinux`) are committed, so builds do not
rewrite the tree; its `struct seccomp` change is dropped (only used on 5.9+, and it would change
`task_struct`, hence the symbol CRCs of the vendor modules).

Manager: the KernelSU Next app of the same release
(https://github.com/KernelSU-Next/KernelSU-Next/releases, `v3.4.0-legacy`); the kernel checks the
manager signature, so the original KernelSU app (`me.weishu.kernelsu`) is not accepted (`is_manager: 0` in
the kernel log). Remove with `CONFIG_KSU=n`.

## GitHub Actions

`.github/workflows/build.yml` builds `Image.gz-dtb` at each push on master (changes outside `docs/` and
`*.md`) and on demand ("Run workflow", with the recovery command line as input). It fetches the AOSP GCC 4.9
toolchain (LineageOS mirror, pinned commit), builds with `ac8257_demo_defconfig` and packs:

- `recovery_test_ac8257.img`: 32 MiB, stock recovery vbmeta, command line + `init=/init ac8257_panic_secs=600
  log_buf_len=8M` (for the recovery test method above);
- `boot_ac8257.img`: 10 MiB, stock boot vbmeta, for the boot partition;
- `Image.gz-dtb`, `config`, `BUILD_INFO.txt` (commit, SHA-256).

The packing uses `tools/ac8257/stock/boot_template_250718.img` (8 KiB: the stock boot header, command line
and boot vbmeta, without the stock kernel; `repack_boot.py` gives the same bytes as with the full stock
`boot.img`) and `tools/ac8257/stock/recovery_stock_vbmeta_250718.bin`. Both images keep the bring-up
options of the defconfig. Artifacts are kept 30 days.

## Stock references

`tools/ac8257/extract_stock.py` extracts from the stock images what this reconstruction is checked
against (configuration, device tree, overlays, list of stock source files); `tools/ac8257/stock_symvers.py`
rebuilds the stock `Module.symvers` and compares it with a build of this tree. Results are kept in
`reference/`.
