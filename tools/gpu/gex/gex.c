// SPDX-License-Identifier: MIT
/*
 * libgex.so: the three calls that MediaTek's DDK 1.11 GLES and Vulkan libraries need and the stock libged.so /
 * libgpu_aux.so of the UJC201 (DDK 1.10 era) do not export. Copyright (C) 2026 LibreHU.
 *
 * The package renames the NEEDED entry "libged.so" of libGLESv1_CM_mtk.so, libGLESv2_mtk.so and vulkan.mt6761.so to
 * "libgex.so" (same length, no other change to those files). This library needs the stock libged.so, so every other
 * GED symbol of those libraries is found there, in the same local group, as before; their libgpu_aux.so symbols come
 * from the stock libgpu_aux.so, which they still need.
 *
 * GuiExtAuxIsSupportPQWhiteList: MediaTek's libgpu_aux.so of DDK 1.11 cannot be used here (it needs the
 * DpAsyncBlitStream2 class of the Android 10 libdpframework.so); its GuiExtAuxIsSupportPQWhiteList() is
 * "mov w0, wzr; ret" (arm64) / "movs r0, #0; bx lr" (arm): always 0, whatever its arguments. Same here.
 *
 * What MediaTek's libged.so of DDK 1.11 does (lib64/libged.so of the TCL Tokyo_TF vendor, disassembled):
 *   ged_gpu_hint_cpu(handle, hint): 7 (GED_ERROR_INVALID_PARAMS) without a handle, 1 (GED_ERROR_FAIL) for a hint
 *     above 1, else the GED bridge ioctl 0xc028670d (command 13), and only on its success some CPU hinting.
 *   ged_hint_force_mdp(handle, flag, out): 7 without a handle, else the GED bridge ioctl 0xc028670e (command 14);
 *     *out is written only on success, else 1 (GED_ERROR_FAIL).
 * The GED driver of this kernel (drivers/misc/mediatek/gpu/ged/include/ged_bridge_id.h) has the commands 0 to 12 and
 * 100 to 105, not 13 nor 14: both ioctls fail there, and MediaTek's library returns GED_ERROR_FAIL. The same results
 * are returned here without the failing ioctl.
 */
#include <stddef.h>

#define GED_ERROR_FAIL 1
#define GED_ERROR_INVALID_PARAMS 7

__attribute__((visibility("default"))) int ged_gpu_hint_cpu(void *handle, int hint)
{
	(void)hint;
	return handle ? GED_ERROR_FAIL : GED_ERROR_INVALID_PARAMS;
}

__attribute__((visibility("default"))) int ged_hint_force_mdp(void *handle, int flag, int *out)
{
	(void)flag;
	(void)out;
	return handle ? GED_ERROR_FAIL : GED_ERROR_INVALID_PARAMS;
}

__attribute__((visibility("default"))) int GuiExtAuxIsSupportPQWhiteList(void)
{
	return 0;
}
