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
       --vbmeta <device tree>/prebuilt/avb/recovery_stock_vbmeta_250718.bin --size 33554432
   fastboot flash recovery recovery-test.img
   adb reboot recovery
   ```
   A crash then ends in a normal boot (stock boot partition), with the RAM, so the log, kept. Flash
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

## Stock references

`tools/ac8257/extract_stock.py` extracts from the stock images what this reconstruction is checked
against (configuration, device tree, overlays, list of stock source files); `tools/ac8257/stock_symvers.py`
rebuilds the stock `Module.symvers` and compares it with a build of this tree. Results are kept in
`reference/`.
