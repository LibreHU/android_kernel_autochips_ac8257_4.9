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

#include <linux/kernel.h>
#include <ddp_clkmgr.h>

#include <cmdq_record.h>
#include <ddp_reg.h>
#include <ddp_path.h>
#include <ddp_dither.h>
#include <ddp_drv.h>
#include <disp_drv_platform.h>
#include <disp_helper.h>

#include <primary_display.h>
#define DITHER0_BASE_NAMING (DISPSYS_DITHER0_BASE)
#define DITHER0_MODULE_NAMING (DISP_MODULE_DITHER0)
#define DITHER0_CLK_NAMING (DISP0_DISP_DITHER0)

#define DITHER_SUPPORT_PARTIAL_UPDATE

#define DITHER0_OFFSET (0)
#define DITHER_TOTAL_MODULE_NUM (1)

#define dither_get_offset(module) (DITHER0_OFFSET)
#define index_of_dither(module) (0)

int dither_dbg_en;
#define DITHER_ERR(fmt, arg...) \
	pr_notice("[DITHER] %s: " fmt "\n", __func__, ##arg)
#define DITHER_DBG(fmt, arg...) \
	do { if (dither_dbg_en) \
		pr_debug("[DITHER] %s: " fmt "\n",  __func__, ##arg); \
		} while (0)

#define DITHER_REG(reg_base, index) ((reg_base) + 0x100 + (index) * 4)

static unsigned int g_dither_relay_value[DITHER_TOTAL_MODULE_NUM];

void disp_dither_init(enum DISP_MODULE_ENUM module, int width, int height,
			     unsigned int dither_bpp, void *cmdq)
{
	const int offset = dither_get_offset(module);
	unsigned long reg_base = DITHER0_BASE_NAMING + offset;
	unsigned int enable;
	unsigned int cfg_val;

	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 5), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 6), 0x00003002, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 7), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 8), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 9), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 10), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 11), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 12), 0x00000011, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 13), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 14), 0x00000000, ~0);

	enable = 0x1;
	if (dither_bpp == 16) {	/* 565 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x50500001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x50504040, ~0);
	} else if (dither_bpp == 18) {	/* 666 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x40400001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x40404040, ~0);
	} else if (dither_bpp == 24) {	/* 888 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x20200001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x20202020, ~0);
	} else if (dither_bpp > 24) {
		DITHER_DBG("High depth LCM (bpp = %d), no dither\n",
			dither_bpp);
		enable = 1;
	} else {
		DITHER_DBG("Invalid dither bpp = %d\n", dither_bpp);
		/* Bypass dither */
		enable = 0;
	}

	DISP_REG_MASK(cmdq, DISP_REG_DITHER_EN + offset, enable, 0x1);
	cfg_val = (enable << 1) | g_dither_relay_value[index_of_dither(module)];
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_CFG + offset, cfg_val, 0x3);
	/* Disable dither MODULE_STALL / SUB_MODULE_STALL  */
	DISP_REG_SET(cmdq, DISP_REG_DITHER_SIZE + offset,
		(width << 16) | height);

#ifdef DISP_PLATFORM_HAS_SHADOW_REG
	if (disp_helper_get_option(DISP_OPT_SHADOW_REGISTER)) {
		if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 0) {
			/* full shadow mode*/
			DISP_REG_MASK(cmdq, DISP_REG_DITHER_0 + offset,
				0x0, 0x7);
		} else if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 1) {
			/* force commit */
			DISP_REG_MASK(cmdq, DISP_REG_DITHER_0 + offset,
				0x1 << 1, 0x7);
		} else if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 2) {
			/* bypass shadow */
			DISP_REG_MASK(cmdq, DISP_REG_DITHER_0 + offset,
				0x1, 0x7);
		}
	}
#endif

	DITHER_DBG("Module(%d) bpp = %d", module, dither_bpp);
	DITHER_DBG("Module(%d) width = %d height = %d", module, width, height);

}


static int disp_dither_config(enum DISP_MODULE_ENUM module,
	struct disp_ddp_path_config *pConfig, void *cmdq)
{
	if (pConfig->dst_dirty) {
		disp_dither_init(module, pConfig->dst_w, pConfig->dst_h,
				 pConfig->lcm_bpp, cmdq);
	}

	return 0;
}


static int disp_dither_bypass(enum DISP_MODULE_ENUM module, int bypass)
{
	int relay = 0;

	if (bypass) {
		relay = 1;
		g_dither_relay_value[index_of_dither(module)] = 0x1;
	} else {
		g_dither_relay_value[index_of_dither(module)] = 0x0;
	}

	DISP_REG_MASK(NULL, DISP_REG_DITHER_CFG + dither_get_offset(module),
		relay, 0x1);

	DITHER_DBG("Module(%d) (bypass = %d)", module, bypass);

	return 0;
}


static int disp_dither_power_on(enum DISP_MODULE_ENUM module, void *handle)
{
	ddp_clk_prepare_enable(ddp_get_module_clk_id(module));
	return 0;
}

static int disp_dither_power_off(enum DISP_MODULE_ENUM module, void *handle)
{
	ddp_clk_disable_unprepare(ddp_get_module_clk_id(module));
	return 0;
}

#ifdef DITHER_SUPPORT_PARTIAL_UPDATE
static int _dither_partial_update(enum DISP_MODULE_ENUM module, void *arg,
	void *cmdq)
{
	struct disp_rect *roi = (struct disp_rect *) arg;
	int width = roi->width;
	int height = roi->height;

	DISP_REG_SET(cmdq, DISP_REG_DITHER_SIZE + dither_get_offset(module),
		(width << 16) | height);
	return 0;
}

static int dither_ioctl(enum DISP_MODULE_ENUM module, void *handle,
		enum DDP_IOCTL_NAME ioctl_cmd, void *params)
{
	int ret = -1;

	if (ioctl_cmd == DDP_PARTIAL_UPDATE) {
		_dither_partial_update(module, params, handle);
		ret = 0;
	}
	return ret;
}
#endif

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
static void ddp_dither_bypass_dither(enum DISP_MODULE_ENUM module, int bypass, void *cmdq)
{
	int relay = 0;

	if (bypass) {
		relay = 1;
		g_dither_relay_value[index_of_dither(module)] = 0x1;
	} else {
		g_dither_relay_value[index_of_dither(module)] = 0x0;
	}

	DISP_REG_MASK(cmdq, DISP_REG_DITHER_CFG + dither_get_offset(module),
		relay, 0x1);

	DITHER_DBG("Module(%d) (bypass = %d)", module, bypass);

}

static int disp_dither_io(enum DISP_MODULE_ENUM module, unsigned int msg,
	unsigned long arg, void *cmdq)
{
	int value = 0;
	switch (msg) {
	case DISP_IOCTL_BYPASS_DITHER:
		if (copy_from_user(&value, (void *)arg, sizeof(int))) {
			DITHER_ERR("DISP_IOCTL_BYPASS_DITHER Copy from user fail");
			return -EFAULT;
		}

		ddp_dither_bypass_dither(module, value, cmdq);
		break;
	}

	return 0;
}
#endif

struct DDP_MODULE_DRIVER ddp_driver_dither = {
	.config = disp_dither_config,
	.bypass = disp_dither_bypass,
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	.cmd = disp_dither_io,
#endif
	.init = disp_dither_power_on,
	.deinit = disp_dither_power_off,
	.power_on = disp_dither_power_on,
	.power_off = disp_dither_power_off,
#ifdef DITHER_SUPPORT_PARTIAL_UPDATE
	.ioctl = dither_ioctl,
#endif
};


void disp_dither_select(enum DISP_MODULE_ENUM module, unsigned int dither_bpp,
	void *cmdq)
{
	const int offset = dither_get_offset(module);
	unsigned long reg_base = DITHER0_BASE_NAMING + offset;
	unsigned int enable;

	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 5), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 6), 0x00003004, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 7), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 8), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 9), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 10), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 11), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 12), 0x00000011, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 13), 0x00000000, ~0);
	DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 14), 0x00000000, ~0);

	enable = 0x1;
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_0, 0 << 4, 1 << 4);
	if (dither_bpp == 16) {	/* 565 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x50500001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x50504040, ~0);
	} else if (dither_bpp == 18) {	/* 666 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x40400001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x40404040, ~0);
	} else if (dither_bpp == 24) {	/* 888 */
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 15), 0x20200001, ~0);
		DISP_REG_MASK(cmdq, DITHER_REG(reg_base, 16), 0x20202020, ~0);
	} else if (dither_bpp > 24) {
		DITHER_DBG("High depth LCM (bpp = %d), no dither\n",
			dither_bpp);
		enable = 1;
	} else {
		DITHER_DBG("Invalid dither bpp = %d\n", dither_bpp);
		/* Bypass dither */
		DISP_REG_MASK(cmdq, DISP_REG_DITHER_0 + offset, 1 << 4, 1 << 4);
		enable = 0;
	}
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_EN + offset, enable, 0x1);
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_CFG + offset, enable << 1, 1 << 1);
	DISP_REG_MASK(cmdq, DISP_REG_DITHER_CFG + offset, 0x0, 0x1);
}


void disp_dither_dump(void)
{
}


void dither_test(const char *cmd, char *debug_output)
{
	enum DISP_MODULE_ENUM module = DITHER0_MODULE_NAMING;
	int i;
	int config_module_num = 1;

	debug_output[0] = '\0';
	DITHER_DBG("(%s)", cmd);

	if (strncmp(cmd, "log:", 4) == 0) {
		dither_dbg_en = (int)cmd[4];
		DITHER_DBG("dither dbg: %d", dither_dbg_en);
	} else if (strncmp(cmd, "sel:", 4) == 0) {
		if (cmd[4] == '0') {
			for (i = 0; i < config_module_num; i++) {
				module += i;
				disp_dither_select(module, 0, NULL);
			}
			DITHER_DBG("bbp=0");
		} else if (cmd[4] == '1') {
			for (i = 0; i < config_module_num; i++) {
				module += i;
				disp_dither_select(module, 16, NULL);
			}
			DITHER_DBG("bbp=16");
		} else if (cmd[4] == '2') {
			for (i = 0; i < config_module_num; i++) {
				module += i;
				disp_dither_select(module, 18, NULL);
			}
			DITHER_DBG("bbp=18");
		} else if (cmd[4] == '3') {
			for (i = 0; i < config_module_num; i++) {
				module += i;
				disp_dither_select(module, 24, NULL);
			}
			DITHER_DBG("bbp=24");
		} else {
			DITHER_DBG("Unknown bbp");
		}
	}
}
