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
2. Put the new kernel into the stock boot image; ramdisk, cmdline and addresses stay as they are:
   ```
   tools/ac8257/repack_boot.py boot-stock.img out/arch/arm64/boot/Image.gz-dtb boot-test.img
   ```
3. Flash `boot-test.img` to the boot partition (fastboot, TWRP, or SP Flash Tool). A locked bootloader with
   enforced verified boot refuses a modified boot image.
4. What to look at: `adb logcat`, `adb shell dmesg`, `/proc/last_kmsg` (or
   `/sys/fs/pstore/console-ramoops*`) after a failed boot, and the UART console (`ttyS0`, 921600 8N1) when
   wired.

## Stock references

`tools/ac8257/extract_stock.py` extracts from the stock images what this reconstruction is checked
against (configuration, device tree, overlays, list of stock source files); `tools/ac8257/stock_symvers.py`
rebuilds the stock `Module.symvers` and compares it with a build of this tree. Results are kept in
`reference/`.
