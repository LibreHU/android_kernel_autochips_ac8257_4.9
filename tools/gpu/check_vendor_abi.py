#!/usr/bin/env python3
# Copyright (C) 2026 LibreHU
# SPDX-License-Identifier: GPL-2.0-only
"""Can the PowerVR DDK 1.11 user space run on the stock Android 9 vendor of the UJC201 (VNDK 28)?

    tools/gpu/check_vendor_abi.py <ddk111 dir> <unit vendor> <vndk v28 dir> <ndk dir>

<ddk111 dir>       lib/ and lib64/ of the DDK 1.11 package (libsrv_um.so, egl/, hw/...), as installed under
                   /vendor/librehu/ddk111.
<unit vendor>      the vendor libraries of the unit that the package does not replace and that its libraries need
                   (libged, libgralloc_extra, libgpu_aux, libladder, libion_mtk, libdrm): tools/gpu/unit_vendor.json
                   (sha256, NEEDED and exported symbols of those of a stock UJC201 vendor), or a directory with their
                   lib/ and lib64/ copies.
<vndk v28 dir>     prebuilts/vndk/v28 of AOSP (arm64/arch-arm64-armv8-a/shared, arm64/arch-arm-armv8-a/shared): the
                   VNDK 28 libraries (vndk-core, vndk-sp) that both LineageOS 16.0 and 17.1 give the vendor.
<ndk dir>          an Android NDK: its API 28 stubs of the LLNDK (libc, libm, libdl, liblog, libnativewindow, libsync,
                   libz...) are the system libraries of Android 9, the oldest system the package can meet.

For each ELF of the package, every NEEDED library is looked up in the package (the same ABI), then among the unit's
vendor libraries, VNDK 28 and the LLNDK; a library found nowhere is an error. Then every undefined, non-weak dynamic
symbol must be exported by one of the libraries of its NEEDED tree (breadth first, as the Android linker resolves a
dlopen()ed library's local group): a missing symbol means the library would not load. Exit 1 on any error.
"""
import json
import os
import re
import subprocess
import sys

LLNDK = {"libc.so", "libm.so", "libdl.so", "liblog.so", "libnativewindow.so", "libsync.so", "libvndksupport.so",
         "libEGL.so", "libGLESv1_CM.so", "libGLESv2.so", "libGLESv3.so", "libz.so", "libandroid_net.so",
         "libmediandk.so", "libvulkan.so"}
# LLNDK symbols that Android 9 gives the vendor beyond the NDK API ("# vndk" in pie-release
# system/core/libsync/libsync.map.txt and frameworks/native/libs/nativewindow/libnativewindow.map.txt): not in the
# NDK stubs, present on every LineageOS 16.0 / 17.1 system.
LLNDK_VNDK = {
    "libsync.so": {"sync_wait", "sync_fence_info", "sync_pt_info", "sync_fence_info_free"},
    "libnativewindow.so": {"AHardwareBuffer_getNativeHandle", "ANativeWindowBuffer_getHardwareBuffer",
                           "ANativeWindow_OemStorageGet", "ANativeWindow_OemStorageSet", "ANativeWindow_cancelBuffer",
                           "ANativeWindow_dequeueBuffer", "ANativeWindow_query", "ANativeWindow_queryf",
                           "ANativeWindow_queueBuffer", "ANativeWindow_setAutoRefresh", "ANativeWindow_setBufferCount",
                           "ANativeWindow_setBuffersDimensions", "ANativeWindow_setBuffersFormat",
                           "ANativeWindow_setBuffersTimestamp", "ANativeWindow_setSharedBufferMode",
                           "ANativeWindow_setSwapInterval", "ANativeWindow_setUsage"},
}
ARCHES = {"lib64": ("arm64/arch-arm64-armv8-a/shared", "aarch64-linux-android"),
          "lib": ("arm64/arch-arm-armv8-a/shared", "arm-linux-androideabi")}


def readelf(args, path):
    return subprocess.run(["readelf", "-W"] + args + [path], check=True, capture_output=True, text=True).stdout


def needed(path):
    return re.findall(r"\(NEEDED\)\s+Shared library: \[([^\]]+)\]", readelf(["-d"], path))


def dynsyms(path):
    """(defined, undefined strong) dynamic symbol names, version suffixes removed"""
    defined, undef = set(), set()
    for line in readelf(["--dyn-syms"], path).splitlines():
        f = line.split()
        if len(f) < 8 or not f[0].rstrip(":").isdigit():
            continue
        bind, ndx, name = f[4], f[6], f[7].split("@")[0]
        if not name:
            continue
        if ndx == "UND":
            if bind != "WEAK":
                undef.add(name)
        else:
            defined.add(name)
    return defined, undef


def index(dirs):
    """library name -> path, first directory wins"""
    found = {}
    for d in dirs:
        if os.path.isdir(d):
            for n in sorted(os.listdir(d)):
                if n.endswith(".so"):
                    found.setdefault(n, os.path.join(d, n))
    return found


def main():
    pkg, unit, vndk, ndk = sys.argv[1:5]
    unit_json = json.load(open(unit))["libs"] if unit.endswith(".json") else None
    errors = 0
    for arch, (vndk_sub, triple) in ARCHES.items():
        package = index([os.path.join(pkg, arch, s) for s in ("", "egl", "hw")])
        if unit_json is not None:
            vendor = {k.split("/", 1)[1]: "unit:" + k for k in unit_json if k.startswith(arch + "/")}
        else:
            vendor = index([os.path.join(unit, arch)])
        snapshot = index([os.path.join(vndk, vndk_sub, s) for s in ("vndk-core", "vndk-sp")])
        sysroot = os.path.join(ndk, "toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib", triple, "28")
        llndk = index([sysroot])

        def resolve(n):
            if n in package:
                return "package", package[n]
            if n in vendor:
                return "unit vendor", vendor[n]
            if n in snapshot:
                return "VNDK 28", snapshot[n]
            if n in LLNDK and n in llndk:
                return "LLNDK API 28", llndk[n]
            return None, None

        syms, deps = {}, {}

        def symbols(path):
            if path not in syms:
                if path.startswith("unit:"):
                    syms[path] = (set(unit_json[path[5:]]["exports"]), set())
                else:
                    defined, undef = dynsyms(path)
                    if path.startswith(sysroot):
                        defined |= LLNDK_VNDK.get(os.path.basename(path), set())
                    syms[path] = (defined, undef)
            return syms[path]

        def needs(path):
            if path not in deps:
                deps[path] = unit_json[path[5:]]["needed"] if path.startswith("unit:") else needed(path)
            return deps[path]

        print(f"== {arch}")
        for n in sorted(package):
            path = package[n]
            print(f"{arch}/{os.path.relpath(path, os.path.join(pkg, arch))}")
            group, queue, seen = [], [path], {path}
            while queue:
                cur = queue.pop(0)
                group.append(cur)
                for dep in needs(cur):
                    kind, dpath = resolve(dep)
                    if dpath is None:
                        if cur == path:
                            print(f"   NEEDED {dep}: NOT FOUND (package, unit vendor, VNDK 28, LLNDK)")
                            errors += 1
                        continue
                    if cur == path:
                        print(f"   NEEDED {dep}: {kind}")
                    if dpath not in seen:
                        seen.add(dpath)
                        queue.append(dpath)
            provided = set()
            for g in group:
                provided |= symbols(g)[0]
            missing = sorted(symbols(path)[1] - provided)
            for s_ in missing:
                print(f"   SYMBOL {s_}: not exported by its libraries")
            errors += len(missing)
    print("result:", "OK" if errors == 0 else f"{errors} error(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
