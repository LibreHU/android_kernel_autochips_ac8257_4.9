/* Copyright Statement:
*
* This software/firmware and related documentation ("AutoChips Software") are
* protected under relevant copyright laws. The information contained herein is
* confidential and proprietary to AutoChips Inc. and/or its licensors. Without
* the prior written permission of AutoChips inc. and/or its licensors, any
* reproduction, modification, use or disclosure of AutoChips Software, and
* information contained herein, in whole or in part, shall be strictly
* prohibited.
*
* AutoChips Inc. (C) 2019. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AUTOCHIPS SOFTWARE")
* RECEIVED FROM AUTOCHIPS AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
* ON AN "AS-IS" BASIS ONLY. AUTOCHIPS EXPRESSLY DISCLAIMS ANY AND ALL
* WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
* NONINFRINGEMENT. NEITHER DOES AUTOCHIPS PROVIDE ANY WARRANTY WHATSOEVER WITH
* RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
* INCORPORATED IN, OR SUPPLIED WITH THE AUTOCHIPS SOFTWARE, AND RECEIVER AGREES
* TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
* RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
* OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN AUTOCHIPS
* SOFTWARE. AUTOCHIPS SHALL ALSO NOT BE RESPONSIBLE FOR ANY AUTOCHIPS SOFTWARE
* RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AUTOCHIPS'S
* ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE AUTOCHIPS SOFTWARE
* RELEASED HEREUNDER WILL BE, AT AUTOCHIPS'S OPTION, TO REVISE OR REPLACE THE
* AUTOCHIPS SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
* CHARGE PAID BY RECEIVER TO AUTOCHIPS FOR SUCH AUTOCHIPS SOFTWARE AT ISSUE.
*/

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include "lcm_drv.h"
#include "ddp_hal.h"
#include "lcm_i2c_interface.h"
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/delay.h>

#define LCM_DRIVER_FATAL(string, args...)  \
	pr_crit("[F] [DISP][LCM] [%s: %d] "string, __FUNCTION__, __LINE__, ##args)
#define LCM_DRIVER_ERROR(string, args...)  \
	pr_err("[E] [DISP][LCM] [%s: %d] "string,  __FUNCTION__, __LINE__, ##args)
#define LCM_DRIVER_INFO(string, args...)   \
	pr_info("[I] [DISP][LCM] [%s] "string, __FUNCTION__, ##args)
#define LCM_DRIVER_PRINTF(string, args...) pr_info(string, ##args)

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)	\
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)		\
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)			\
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define write_regs(addr, pdata, byte_nums)				\
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define wrtie_cmd(cmd)							\
	lcm_util.dsi_write_cmd(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)				\
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define read_reg(cmd)							\
	lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define SET_RESET_PIN(v)						\
	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)	msleep(n)
#define UDELAY(n)	udelay(n)

#define LCM_POWER_EN_PIN			159

#define LVDS_REGFLAG_BRIGE_IC_ADDR		0x2D

#define LVDS_REGFLAG_MDELAY			0xFC
#define LVDS_REGFLAG_ADDR_SELECT		0xFE
#define LVDS_REGFLAG_END_OF_TABLE		0xFF

#define MIPI_REGFLAG_UDELAY			0xFFFB
#define MIPI_REGFLAG_MDELAY			0xFFFC
#define MIPI_REGFLAG_END_OF_TABLE		0xFFFD

#define LVDS_BRIDGE_IC_RESET_PIN		45
#define LCM_POWER_EN_PIN			159

static struct LCM_UTIL_FUNCS lcm_util;
static struct pinctrl *pinctrl1;
static struct pinctrl_state *rst_output0, *rst_output1, *power_en_output0, *power_en_output1;


struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table init_setting[] = {
	{0x09, 1, {0x00} },
	{0x0A, 1, {0x05} },
	{0x0B, 1, {0x10} },
	{0x0D, 1, {0x00} },
	{0x10, 1, {0x26} },
	{0x11, 1, {0x00} },
	{0x12, 1, {0x26} },
	{0x13, 1, {0x00} },
	{0x18, 1, {0x78} },
	{0x19, 1, {0x00} },
	{0x1A, 1, {0x03} },
	{0x1B, 1, {0x00} },
	{0x20, 1, {0x00} },
	{0x21, 1, {0x05} },
	{0x22, 1, {0x00} },
	{0x23, 1, {0x00} },
	{0x24, 1, {0x00} },
	{0x25, 1, {0x00} },
	{0x26, 1, {0x00} },
	{0x27, 1, {0x00} },
	{0x28, 1, {0x21} },
	{0x29, 1, {0x00} },
	{0x2A, 1, {0x00} },
	{0x2B, 1, {0x00} },
	{0x2C, 1, {0x0A} },
	{0x2D, 1, {0x00} },
	{0x2E, 1, {0x00} },
	{0x2F, 1, {0x00} },
	{0x30, 1, {0x0A} },
	{0x31, 1, {0x00} },
	{0x32, 1, {0x00} },
	{0x33, 1, {0x00} },
	{0x34, 1, {0x14} },
	{0x35, 1, {0x00} },
	{0x36, 1, {0x00} },
	{0x37, 1, {0x00} },
	{0x38, 1, {0x00} },
	{0x39, 1, {0x00} },
	{0x3A, 1, {0x00} },
	{0x3B, 1, {0x00} },
	{0x3C, 1, {0x00} },
	{0x3D, 1, {0x00} },
	{0x3E, 1, {0x00} },
	{0x0D, 1, {0x01} },
	{0xFC, 1, {0x0A} },
	{0x09, 1, {0x01} },
	{0xFF, 1, {0x00} },
};

void lcm_gpio_output(int pin, int level)
{
	LCM_DRIVER_INFO("lcm_gpio_output pin = %d, level = %d\n", pin, level);

	if ((!pinctrl1) || (!rst_output1) || (!rst_output0) || (!power_en_output1) || (!power_en_output0)) {
		LCM_DRIVER_ERROR("invalid params!\n");

		return ;
	}
	if (LVDS_BRIDGE_IC_RESET_PIN == pin) {
		if (level) {
			pinctrl_select_state(pinctrl1, rst_output1);
		} else {
			pinctrl_select_state(pinctrl1, rst_output0);
		}

	} else if (LCM_POWER_EN_PIN == pin) {
		if (level) {
			pinctrl_select_state(pinctrl1, power_en_output1);
		} else {
			pinctrl_select_state(pinctrl1, power_en_output0);
		}
	}
}

int lcm_get_gpio_info(struct platform_device *pdev)
{
	int ret;

	LCM_DRIVER_INFO("enter\n");
	if (NULL == pdev) {
		LCM_DRIVER_ERROR("invalid params!\n");
		ret = -EINVAL;
		goto error;
	}
	pinctrl1 = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pinctrl1)) {
		ret = PTR_ERR(pinctrl1);
		pinctrl1 = NULL;
		LCM_DRIVER_ERROR("Cannot find pinctrl1, ret =%d!\n", ret);
		goto error;
	}
	rst_output0 = pinctrl_lookup_state(pinctrl1, "state_rst_output0");
	if (IS_ERR(rst_output0)) {
		ret = PTR_ERR(rst_output0);
		rst_output0 = NULL;
		LCM_DRIVER_ERROR("Cannot find pinctrl staterst_output0!\n");
		goto error;
	}
	rst_output1 = pinctrl_lookup_state(pinctrl1, "state_rst_output1");
	if (IS_ERR(rst_output1)) {
		ret = PTR_ERR(rst_output1);
		rst_output1 = NULL;
		LCM_DRIVER_ERROR("Cannot find pinctrl staterst_output1!\n");
		goto error;
	}
	power_en_output0 = pinctrl_lookup_state(pinctrl1, "state_power_en_output0");
	if (IS_ERR(power_en_output0)) {
		ret = PTR_ERR(power_en_output0);
		power_en_output0 = NULL;
		LCM_DRIVER_ERROR("Cannot find pinctrl state_power_en_output0!\n");
		goto error;
	}
	power_en_output1 = pinctrl_lookup_state(pinctrl1, "state_power_en_output1");
	if (IS_ERR(power_en_output1)) {
		ret = PTR_ERR(power_en_output1);
		power_en_output1 = NULL;
		LCM_DRIVER_ERROR("Cannot find pinctrl state_power_en_output1!\n");
		goto error;
	}

	LCM_DRIVER_INFO("leave\n");

	return 0;

error:
	LCM_DRIVER_ERROR("fail, ret=%d.\n", ret);

	return ret;
}
EXPORT_SYMBOL(lcm_get_gpio_info);

void lvds_push_table(struct LCM_setting_table *table, unsigned int count)
{
	unsigned int i;
	unsigned char cmd = 0;
	unsigned char addr = LVDS_REGFLAG_BRIGE_IC_ADDR;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {
		case LVDS_REGFLAG_MDELAY:
			MDELAY(table[i].count);
			break;
		case LVDS_REGFLAG_ADDR_SELECT:
			addr = table[i].count;
			break;
		case LVDS_REGFLAG_END_OF_TABLE:
			break;
		default:
			interface_write_byte(addr, cmd, table[i].para_list[0]);
		}
	}
}

void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

		case MIPI_REGFLAG_MDELAY:
			MDELAY(table[i].count);
			break;

		case MIPI_REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case MIPI_REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	LCM_DRIVER_INFO("enter\n");
	if (NULL == params) {
		LCM_DRIVER_FATAL("invalid parameters!\n");

		return;
	}
	memset(params, 0, sizeof(*params));

	/* Default LCM params for interface of MIPI2LVDS(MIPI2LVDS DRIGE IC: SN65DSI83,LVDS LCM:BP101WX1-206)*/
	params->type = LCM_TYPE_DSI;
	params->width = 1280;
	params->height = 800;
	params->dsi.mode = SYNC_EVENT_VDO_MODE;
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 5;
	params->dsi.vertical_frontporch = 5;
	params->dsi.vertical_active_line = 800;
	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 20;
	params->dsi.horizontal_frontporch = 20;
	params->dsi.horizontal_active_pixel = 1280;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.pll_select = 0;	/* 0: MIPI_PLL; 1: LVDS_PLL */
	params->dsi.PLL_CLOCK = 195;
	params->dsi.cont_clock = 1;
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range = 1;
	LCM_DRIVER_INFO("leave\n");
}

static void lcm_init(void)
{
	LCM_DRIVER_INFO("enter\n");

	SET_RESET_PIN(0);
	MDELAY(15);
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);

	/*for lvds panel*/
	lvds_push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table));
	/*for mipi panel*/
	/*push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);*/

	lcm_gpio_output(LCM_POWER_EN_PIN, 1);
	LCM_DRIVER_INFO("leave\n");
}

static void lcm_suspend(void)
{
	LCM_DRIVER_INFO("enter\n");
	lcm_gpio_output(LCM_POWER_EN_PIN, 0);
	MDELAY(5);
	SET_RESET_PIN(0);
	LCM_DRIVER_INFO("leave\n");
}

static void lcm_resume(void)
{
	LCM_DRIVER_INFO("enter\n");
	lcm_init();
	LCM_DRIVER_INFO("leave\n");
}

void lcm_debug_suspend(void)
{
	LCM_DRIVER_INFO("enter\n");
	lcm_suspend();
	LCM_DRIVER_INFO("leave\n");
}
void lcm_debug_resume(void)
{
	LCM_DRIVER_INFO("enter\n");
	lcm_resume();
	LCM_DRIVER_INFO("leave\n");
}
void lcm_read_reg(unsigned char slave_addr, unsigned char reg_addr)
{
	unsigned char data = 0;
	LCM_DRIVER_INFO("enter\n");
	interface_read_byte(slave_addr, reg_addr, &data);
	LCM_DRIVER_INFO("read slave_addr:%x, reg_addr:%x, value:%x\n", slave_addr, reg_addr, data);
	LCM_DRIVER_INFO("leave\n");
}
void lcm_write_reg(unsigned char slave_addr, unsigned char reg_addr, unsigned char value)
{
	LCM_DRIVER_INFO("enter\n");
	interface_write_byte(slave_addr, reg_addr, value);
	LCM_DRIVER_INFO("write slave_addr:%x, reg_addr:%x, value:%x\n", slave_addr, reg_addr, value);
	LCM_DRIVER_INFO("leave\n");
}

void lcm_dump_info(unsigned int index)
{

}

struct LCM_DRIVER sn65dsi83_bp101wx1_206_1280_800_lcm_drv = {
	.name = "sn65dsi83_bp101wx1_206_1280_800",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = NULL,
	.init_power = NULL,
	.resume_power = NULL,
	.suspend_power = NULL,
	.esd_check = NULL,
	.set_backlight = NULL,
	.ata_check = NULL,
	.update = NULL,
	.switch_mode = NULL,
};
