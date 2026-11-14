/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/initramfs.h
 *
 * Copyright (C) 2015, Google
 * Rom Lemarchand <romlem@android.com>
 *
 * LibreHU (4.19 port of the AutoChips AC8257 kernel): from the Android 4.9 kernels, for skip_initramfs.
 */

#ifndef _LINUX_INITRAMFS_H
#define _LINUX_INITRAMFS_H

#include <linux/kconfig.h>

#if IS_BUILTIN(CONFIG_BLK_DEV_INITRD)

int __init default_rootfs(void);

#endif

#endif /* _LINUX_INITRAMFS_H */
