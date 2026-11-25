#!/usr/bin/env python3
# Copyright (C) 2026 LibreHU
# SPDX-License-Identifier: GPL-2.0-only
"""Vulkan package of the rc-4.19.325-vulkan kernel: the PowerVR DDK 1.11 user space for the stock UJC201 vendor.

    tools/gpu/make_vulkan_package.py <blobs> <ndk> <vndk v28> <dist> <version>

<blobs>     proprietary/vendor/ files of tools/gpu/ddk111_blobs.sha256 (lib/, lib64/, firmware/), as downloaded from the
            pinned commit of AndroidBlobs/vendor_tcl_Tokyo_TF (MediaTek MT6761: same Rogue core 22.68.54.30, DDK 1.11@5425693,
            Android 9 user space builds: .note.android.ident API 28).
<ndk>       an Android NDK (builds libgex.so for API 28, gives the LLNDK stubs of the ABI check).
<vndk v28>  prebuilts/vndk/v28 of AOSP (arm64/arch-arm64-armv8-a/shared, arm64/arch-arm-armv8-a/shared).
<dist>      where librehu_gpu_ddk111_twrp.zip and librehu_gpu_ddk111_remove_twrp.zip are written.

What the vendor gets (nothing of the stock vendor is overwritten):
  /vendor/librehu/ddk111/lib{,64}/...         DDK 1.11 copies of the 11 stock DDK 1.10 files (libsrv_um.so, libIMGegl.so,
                                              libusc.so, libglslcompiler.so, libufwriter.so, libPVROCL.so,
                                              libPVRScopeServices.so, egl/libEGL_mtk.so, egl/libGLESv1_CM_mtk.so,
                                              egl/libGLESv2_mtk.so, hw/gralloc.ac8257.so), bind-mounted over them
  /vendor/librehu/ddk111/permissions/         android.hardware.vulkan.{level,version,compute}.xml (Vulkan 1.1, level 0),
                                              bind-mounted over /vendor/odm/etc/permissions (created empty if missing)
  /vendor/lib{,64}/hw/vulkan.librehu.so       the Vulkan driver, only looked up when ro.hardware.vulkan=librehu
  /vendor/lib{,64}/libgex.so                  tools/gpu/gex: GED / GPU aux calls of DDK 1.11 missing from the stock libs
  /vendor/firmware/rgx_ddk111.fw.22.68.54.30  firmware of the DDK 1.11 driver (own name, the 1.10 one stays)
  /vendor/etc/init/librehu_gpu_ddk111.rc      bind mounts and ro.hardware.vulkan at early-init, ONLY when
                                              ro.boot.librehu_gpu=1.11 (device tree of a DDK 1.11 kernel)
Under any other kernel (stock, stable-4.9.337, rc-4.19.325, the ROM zips' own) none of it is used: no bind mount, no
Vulkan driver looked up, no feature declared, and the 1.10 driver keeps the stock 1.10 user space and firmware.

Checks made here, the build fails otherwise: sha256 of every blob (tools/gpu/ddk111_blobs.sha256); the NEEDED rename
libged.so -> libgex.so of libGLESv1_CM_mtk.so, libGLESv2_mtk.so and vulkan.mt6761.so (tools/gpu/rename_needed.py);
every library of the package loads on the stock vendor (tools/gpu/check_vendor_abi.py with tools/gpu/unit_vendor.json,
VNDK 28 and the API 28 LLNDK). The installer checks the unit itself: see tools/gpu/twrp/gpu-ddk111-install.sh.
"""
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
TWRP = os.path.join(os.path.dirname(HERE), "twrp")
DATE = (2020, 1, 1, 0, 0, 0)
DEVICES = "ac8257_demo"
ARCHES = {"lib64": "aarch64-linux-android28", "lib": "armv7a-linux-androideabi28"}
# stock DDK 1.10 file of /vendor/<arch>/ -> DDK 1.11 blob of <blobs>/<arch>/
BOUND = {"libsrv_um.so": "libsrv_um.so", "libIMGegl.so": "libIMGegl.so", "libusc.so": "libusc.so",
         "libglslcompiler.so": "libglslcompiler.so", "libufwriter.so": "libufwriter.so", "libPVROCL.so": "libPVROCL.so",
         "libPVRScopeServices.so": "libPVRScopeServices.so", "egl/libEGL_mtk.so": "egl/libEGL_mtk.so",
         "egl/libGLESv1_CM_mtk.so": "egl/libGLESv1_CM_mtk.so", "egl/libGLESv2_mtk.so": "egl/libGLESv2_mtk.so",
         "hw/gralloc.ac8257.so": "hw/gralloc.mt6761.so"}
RENAMED = ("egl/libGLESv1_CM_mtk.so", "egl/libGLESv2_mtk.so", "hw/vulkan.mt6761.so")
FEATURES = {"android.hardware.vulkan.level.xml": ("android.hardware.vulkan.level", 0),
            # 4198400 = 0x401000 = Vulkan 1.1.0, what MediaTek declares with this DDK 1.11 driver (vendor of the Xiaomi
            # Redmi 6A certus, DDK 1.11@5425693, vendor/etc/permissions/android.hardware.vulkan.version.xml)
            "android.hardware.vulkan.version.xml": ("android.hardware.vulkan.version", 4198400),
            "android.hardware.vulkan.compute.xml": ("android.hardware.vulkan.compute", 0)}


def sha256(path):
    return hashlib.sha256(open(path, "rb").read()).hexdigest()


def check_blobs(blobs):
    for line in open(os.path.join(HERE, "ddk111_blobs.sha256")):
        if line.startswith("#") or not line.strip():
            continue
        want, rel = line.split()
        if sha256(os.path.join(blobs, rel)) != want:
            sys.exit(f"{rel}: sha256 differs from tools/gpu/ddk111_blobs.sha256")


def rc_file():
    lines = ["# LibreHU Vulkan package (tools/gpu/make_vulkan_package.py of the kernel repository, branch rc-4.19.325-vulkan).",
             "# PowerVR DDK 1.11 user space (Vulkan 1.1) in place of the stock DDK 1.10 one, ONLY under a kernel built with the",
             "# DDK 1.11 driver: its device tree has /firmware/android/librehu_gpu = \"1.11\", read by init as ro.boot.librehu_gpu.",
             "# Under any other kernel nothing here runs and the stock stack is used. The stock files are never changed: the",
             "# DDK 1.11 copies of /vendor/librehu/ddk111 are bind-mounted over them before any GPU user starts (early-init).",
             "on early-init && property:ro.boot.librehu_gpu=1.11"]
    for arch in ("lib", "lib64"):
        for dst in BOUND:
            lines.append(f"    mount none /vendor/librehu/ddk111/{arch}/{dst} /vendor/{arch}/{dst} bind")
    lines.append("    mount none /vendor/librehu/ddk111/permissions /vendor/odm/etc/permissions bind")
    lines.append("    # libvulkan looks up /vendor/lib{,64}/hw/vulkan.<ro.hardware.vulkan>.so first")
    lines.append("    setprop ro.hardware.vulkan librehu")
    return "\n".join(lines) + "\n"


def feature_xml(name, version):
    return ('<?xml version="1.0" encoding="utf-8"?>\n'
            '<!-- LibreHU Vulkan package: declared only while the DDK 1.11 driver is in use (librehu_gpu_ddk111.rc) -->\n'
            f'<permissions>\n    <feature name="{name}" version="{version}" />\n</permissions>\n')


def stage(blobs, ndk, root):
    """Files of the package under <root>/vendor/..., as installed"""
    v = os.path.join(root, "vendor")
    clang = os.path.join(ndk, "toolchains/llvm/prebuilt/linux-x86_64/bin")
    for arch, target in ARCHES.items():
        for dst, src in BOUND.items():
            out = os.path.join(v, "librehu/ddk111", arch, dst)
            os.makedirs(os.path.dirname(out), exist_ok=True)
            shutil.copyfile(os.path.join(blobs, arch, src), out)
            if src in RENAMED:
                subprocess.run([sys.executable, os.path.join(HERE, "rename_needed.py"), out, "libged.so", "libgex.so"],
                               check=True)
        out = os.path.join(v, arch, "hw", "vulkan.librehu.so")
        os.makedirs(os.path.dirname(out), exist_ok=True)
        shutil.copyfile(os.path.join(blobs, arch, "hw/vulkan.mt6761.so"), out)
        subprocess.run([sys.executable, os.path.join(HERE, "rename_needed.py"), out, "libged.so", "libgex.so"], check=True)
        # libgex.so needs the stock libged.so (NEEDED, so that it stays in the local group of the libraries that need
        # libgex.so instead), but calls none of its functions: an empty libged.so is enough to link it
        with tempfile.TemporaryDirectory() as t:
            open(os.path.join(t, "ged.c"), "w").write("void librehu_ged_link_stub(void) {}\n")
            subprocess.run([os.path.join(clang, f"{target}-clang"), "-shared", "-fPIC", "-Wl,-soname,libged.so",
                            "-o", os.path.join(t, "libged.so"), os.path.join(t, "ged.c")], check=True)
            out = os.path.join(v, arch, "libgex.so")
            subprocess.run([os.path.join(clang, f"{target}-clang"), "-O2", "-fPIC", "-shared", "-Wall", "-Wextra",
                            "-Werror", "-Wl,-soname,libgex.so", "-Wl,--no-undefined", "-Wl,-z,max-page-size=4096",
                            "-Wl,--build-id=sha1", "-o", out, os.path.join(HERE, "gex/gex.c"), "-L" + t,
                            "-Wl,--no-as-needed", "-lged"], check=True)
            if "libged.so" not in subprocess.run(["readelf", "-d", out], capture_output=True, text=True).stdout:
                sys.exit("libgex.so: NEEDED libged.so missing")
    os.makedirs(os.path.join(v, "firmware"))
    shutil.copyfile(os.path.join(blobs, "firmware/rgx.fw"), os.path.join(v, "firmware/rgx_ddk111.fw.22.68.54.30"))
    os.makedirs(os.path.join(v, "librehu/ddk111/permissions"))
    for n, (feature, version) in FEATURES.items():
        open(os.path.join(v, "librehu/ddk111/permissions", n), "w").write(feature_xml(feature, version))
    os.makedirs(os.path.join(v, "etc/init"))
    open(os.path.join(v, "etc/init/librehu_gpu_ddk111.rc"), "w").write(rc_file())


def abi_check(root, ndk, vndk):
    """check_vendor_abi.py on the libraries as the linker sees them (lib/, lib64/ with egl/ and hw/)"""
    with tempfile.TemporaryDirectory() as t:
        for arch in ARCHES:
            for sub in ("", "egl", "hw"):
                os.makedirs(os.path.join(t, arch, sub), exist_ok=True)
            for dst in BOUND:
                shutil.copyfile(os.path.join(root, "vendor/librehu/ddk111", arch, dst), os.path.join(t, arch, dst))
            for rel in ("hw/vulkan.librehu.so", "libgex.so"):
                shutil.copyfile(os.path.join(root, "vendor", arch, rel), os.path.join(t, arch, rel))
        r = subprocess.run([sys.executable, os.path.join(HERE, "check_vendor_abi.py"), t,
                            os.path.join(HERE, "unit_vendor.json"), vndk, ndk], capture_output=True, text=True)
        return r.returncode, r.stdout


def write_zip(path, zid, version, body, files):
    common = open(os.path.join(TWRP, "twrp-common.sh")).read() + open(os.path.join(HERE, "twrp/gpu-ddk111-common.sh")).read()
    check = ('DEV=$(getprop ro.product.device)\n'
             f'case " {DEVICES} " in *" $DEV "*) ;; *) abort "device \\"$DEV\\" not supported ({DEVICES})";; esac\n')
    ub = f'#!/sbin/sh\nOUTFD=$2; ZIPFILE=$3; ZIP_ID={zid}; VERSION="{version}"\n' + common + "\n" + check + \
        open(os.path.join(HERE, "twrp", body)).read()
    with zipfile.ZipFile(path, "w") as z:
        for name, data, mode in [("META-INF/com/google/android/update-binary", ub, 0o755),
                                 ("META-INF/com/google/android/updater-script", "# voir update-binary\n", 0o755)] + \
                                [("files/" + n, d, 0o644) for n, d in sorted(files.items())]:
            info = zipfile.ZipInfo(name, date_time=DATE)
            info.external_attr = mode << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, data)


def main():
    if len(sys.argv) != 6:
        sys.exit(__doc__)
    blobs, ndk, vndk, dist, version = sys.argv[1:]
    check_blobs(blobs)
    with tempfile.TemporaryDirectory() as root:
        stage(blobs, ndk, root)
        rc, report = abi_check(root, ndk, vndk)
        open(os.path.join(dist, "gpu_ddk111_vendor_abi.txt"), "w").write(report)
        if rc:
            print(report)
            sys.exit("the DDK 1.11 package does not load on the stock vendor (gpu_ddk111_vendor_abi.txt)")
        files, manifest = {}, []
        for d, _, names in sorted(os.walk(os.path.join(root, "vendor"))):
            for n in sorted(names):
                p = os.path.join(d, n)
                rel = os.path.relpath(p, root)  # vendor/...
                files["payload/" + rel] = open(p, "rb").read()
                manifest.append(f"{sha256(p)}  /{rel}")
        files["MANIFEST"] = "\n".join(manifest) + "\n"
        # what the installer checks on the unit before changing anything: the stock files the package binds over, and
        # the stock libraries its own libraries need (sha256 of a stock UJC201 vendor, tools/gpu/unit_vendor.json)
        import json
        unit = json.load(open(os.path.join(HERE, "unit_vendor.json")))
        files["unit_vendor.sha256"] = "".join(f"{v['sha256']}  /vendor/{k}\n" for k, v in sorted(unit["libs"].items())) + \
            "".join(f"{h}  /vendor/{k}\n" for k, h in sorted(unit["files"].items()))
        files["bound"] = "".join(f"/vendor/librehu/ddk111/{a}/{d} /vendor/{a}/{d}\n" for a in ("lib", "lib64") for d in BOUND)
        files["version"] = version + "\n"
        write_zip(os.path.join(dist, "librehu_gpu_ddk111_twrp.zip"), "librehu_gpu_ddk111", version,
                  "gpu-ddk111-install.sh", files)
        write_zip(os.path.join(dist, "librehu_gpu_ddk111_remove_twrp.zip"), "librehu_gpu_ddk111_remove", version,
                  "gpu-ddk111-remove.sh", {"MANIFEST": files["MANIFEST"], "version": files["version"]})
    print(f"{dist}/librehu_gpu_ddk111_twrp.zip, librehu_gpu_ddk111_remove_twrp.zip: {len(manifest)} files")


if __name__ == "__main__":
    main()
