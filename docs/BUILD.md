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

   Not kept across reboots. Keep a copy of `para` before testing (`dd if=/dev/block/by-name/para of=...`).

## CPU/GPU frequencies, governors

The overclock tables are loaded by default, with the stock maxima as run-time caps:

- CPU: 16 OPPs from 850 MHz to 2.201 GHz: the stock "FY" voltages up to 2.001 GHz, 2.101 and 2.201 GHz
  at the same 1.025 V top voltage (2.201 GHz is the top of MediaTek's "SB" table for this CPU family; the
  stock FY/SB tables are the same in the stock kernel). Cap: `ac8257_cpufreq.max_khz`, **2001000** by
  default, applied on top of the PPM limits. CPU DVFS is "hybrid" on this SoC (`CONFIG_HYBRID_CPU_DVFS`,
  defined by `mtk_cpufreq_platform.h` with SSPM support): the SSPM applies the OPPs from the record table
  (`xrecordTbl`, `mtk_cpufreq_opp_pv_table.h`, with an `ocTbl` for this level) and the limits given by
  `cpuhvfs_set_min_max()`, where the cap is applied. (Stages 1y-2a lacked the record table entry and
  crashed at 1.3 s in `cpuhvfs_pvt_tbl_create`; stage 2b hung in the EEM init because the PTPOD OPP, which
  must be at the 0.80 V boot voltage, is index 10 in this table, not 8.) The EEM voltage adjustments, calibrated against the stock
  table, are not applied with this table (sign-off voltages instead, slightly higher).
- GPU: 730 MHz (the MT6761T top OPP of this GPU) added at the 0.80 V of the stock 660 MHz. Cap:
  `ac8257_gpufreq.max_khz`, **660000** by default.

As root, at run time (not kept across reboots):

    echo 2201000 > /sys/module/ac8257_cpufreq/parameters/max_khz   # CPU up to 2.2 GHz
    echo 2001000 > /sys/module/ac8257_cpufreq/parameters/max_khz   # back to 2.0 GHz
    echo 730000 > /sys/module/ac8257_gpufreq/parameters/max_khz    # GPU up to 730 MHz
    echo 0 1533000 > /proc/ppm/policy/userlimit_max_cpu_freq       # lower limit through the PPM
    echo 500000 > /proc/gpufreq/gpufreq_opp_freq                   # fix the GPU OPP (0: automatic)

or on the kernel command line (`--cmdline-append`): `ac8257_cpufreq.max_khz=2201000`,
`ac8257_gpufreq.max_khz=730000`. `ac8257_cpu_oc=0` / `ac8257_gpu_oc=0` select the stock tables (with EEM).
No voltage is raised above the stock maximum; stability above the stock frequencies depends on the chip
and is not guaranteed (thermal throttling still applies).

CPU governors: interactive (default), schedutil, conservative, ondemand, performance, powersave,
userspace, schedplus (`scaling_governor`). I/O schedulers: deadline (default), cfq, noop.

Other tweaks (kept to what is measurable and leaves the vendor module ABI alone):

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
