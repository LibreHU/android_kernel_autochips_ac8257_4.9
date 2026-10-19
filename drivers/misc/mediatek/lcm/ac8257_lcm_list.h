/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __MT65XX_LCM_LIST_H__
#define __MT65XX_LCM_LIST_H__

#include <lcm_drv.h>

extern struct LCM_DRIVER nt35521_hd_dsi_vdo_truly_rt5081_lcm_drv;

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
extern struct LCM_DRIVER lcm_driver_common_lcm_drv;
extern struct LCM_DRIVER sn65dsi83_bp101wx1_206_1280_800_lcm_drv;
#endif

#ifdef BUILD_LK
extern void mdelay(unsigned long msec);
#endif

#endif
