/*
* Copyright Statement:
*
* This software/firmware and related documentation ("AutoChips Software") are
* protected under relevant copyright laws. The information contained herein is
* confidential and proprietary to AutoChips Inc. and/or its licensors. Without
* the prior written permission of AutoChips inc. and/or its licensors, any
* reproduction, modification, use or disclosure of AutoChips Software, and
* information contained herein, in whole or in part, shall be strictly
* prohibited.
*
* AutoChips Inc. (C) 2020. All rights reserved.
*
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
#include "lcm_driver_common.h"
#include "lcm_i2c_interface.h"
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include "metazone.h"
#include "metazone_ioctl.h"

#define LVDS_BRIGE_IC_DS90UB941_DS90UB948_ENABLE
#define LVDS_BRIGE_IC_SN65DSI83_ENABLE
#define LVDS_BRIGE_IC_GAOWIN_ENABLE
#define LVDS_BRIGE_IC_DS90UB947_ENABLE

/*multidisplay enable secondary panel*/
#define LVDS_INIT_SECONDARY_PANEL_ENABLE_S	"[Lvds_Init_Secondary_Panel_Enable_S]"
#define LVDS_INIT_SECONDARY_PANEL_ENABLE_E	"[Lvds_Init_Secondary_Panel_Enable_E]"

#define LVDS_INIT_S				"[Lvds_Init_S]"
#define LVDS_INIT_E				"[Lvds_Init_E]"
#define MIPI_INIT_S				"[Mipi_Init_S]"
#define MIPI_INIT_E				"[Mipi_Init_E]"
#define MIPI_SUSPEND_S				"[Mipi_Suspend_S]"
#define MIPI_SUSPEND_E				"[Mipi_Suspend_E]"
#define POWER_ON_SEQUENCE_S			"[Power_On_Sequence_S]"
#define POWER_ON_SEQUENCE_E			"[Power_On_Sequence_E]"
#define POWER_OFF_SEQUENCE_S			"[Power_Off_Sequence_S]"
#define POWER_OFF_SEQUENCE_E			"[Power_Off_Sequence_E]"

#define METAZONE_OPEN_EXT_DISP_INDEX     	0x1000E
#define BACKLIGHT_INDEX 			0x1000C

#define LCM_ADDR 		0x1A/*LCM*/

#ifdef LVDS_BRIGE_IC_DS90UB941_DS90UB948_ENABLE
#define LVDS_REGFLAG_TI941_ADDR			0x1A
#define LVDS_REGFLAG_TI948_ADDR			0x2C
#endif

#ifdef LVDS_BRIGE_IC_GAOWIN_ENABLE
#define LVDS_REGFLAG_GAOWIN_ADDR		0x18
#endif

#ifdef LVDS_BRIGE_IC_DS90UB947_ENABLE
#define LVDS_REGFLAG_DS90UB947_ADDR		0xE
#endif

#define LVDS_REGFLAG_BRIGE_IC_ADDR		0x2D

#define LVDS_REGFLAG_MDELAY			0xFC
#define LVDS_REGFLAG_ADDR_SELECT		0xFE
#define LVDS_REGFLAG_END_OF_TABLE		0xFF

#define MIPI_REGFLAG_UDELAY			0xFFFB
#define MIPI_REGFLAG_MDELAY			0xFFFC
#define MIPI_REGFLAG_END_OF_TABLE		0xFFFD

#define TIMING_CTL_END_OF_TABLE			0xFF

#define MAX_DATA_LEN				(1024*20)
#define DUMP_DATA_LEN				100

#define LVDS_BRIDGE_IC_RESET_PIN		45
#define LCM_POWER_EN_PIN			159
#define GOWIN_BRIDGE_IC_POWER_EN_PIN0		3
#define GOWIN_BRIDGE_IC_POWER_EN_PIN1		44

/*
 * Jancar boards (UJC201): the panel is a MIPI DSI panel whose init table and
 * timings are stored in the MetaZone. LK runs that table at boot; the kernel
 * must run it again on every resume, after the panel lost its power. GPIO
 * numbers are those of the stock dtbo lcm_pins_* nodes.
 */
#define JAC_LCD_POWER_PIN			170	/* state_power_en_output* */
#define JAC_BRIDGE_POWER_PIN			164	/* state_gowin_bridge_chip_power_en* */
#define JAC_LCD_RESET_PIN			152	/* state_jac_rst_output* */

#define JAC_MZ_LCM_TYPE				0x1019C
#define JAC_MZ_LCD_POWER_DELAY			0x101A4
#define JAC_MZ_MIPI_TABLE			0x101A5	/* binary items of 100 bytes */
#define JAC_MZ_MIPI_TABLE_ITEMS			35
#define JAC_MZ_MIPI_LANE			0x101DA
#define JAC_MZ_MIPI_CLK				0x101DC
#define JAC_MZ_WIDTH				0x101DD
#define JAC_MZ_HEIGHT				0x101DE
#define JAC_MZ_CONFIG_ENABLE			0x101DF
#define JAC_MZ_VFP				0x101E4
#define JAC_MZ_VBP				0x101E5
#define JAC_MZ_VPW				0x101E6
#define JAC_MZ_HFP				0x101E7
#define JAC_MZ_HBP				0x101E8
#define JAC_MZ_HPW				0x101E9
#define JAC_MZ_BIT				0x101EE
#define JAC_MZ_MIPI_TABLE_LEN			0x102DB

#define JAC_CONFIG_ENABLE_MAGIC			0x51525354
#define JAC_MIPI_ITEM_LEN			100
#define JAC_MIPI_TABLE_LEN_MAX			3400
#define JAC_MIPI_PARA_MAX			99
#define JAC_MIPI_CMD_MAX			512
#define JAC_LCD_POWER_DELAY_MAX			3000

#define MRF_HEADER_INFO_OFF			0
#define MRF_HEADER_INFO_LEN			0xC00
#define MRF_PANEL_INFO_OFF			(MRF_HEADER_INFO_OFF+0x100)
#define MRF_PANEL_INFO_LEN			0x200
#define MRF_CRC_LEN				4

#define UINT32_INVALD_FLAG			0xFFFFFFFF
#define UINT16_INVALD_FLAG			0xFFFF
#define UINT8_INVALD_FLAG			0xFF

/*BRIDGE_IC_IRQ_ENABLE for SN65DSI8X, MULTIDISPLAY not use this bridge IC*/
#ifdef LVDS_BRIGE_IC_SN65DSI83_ENABLE
#define BRIDGE_IC_IRQ_ENABLE
#endif

#ifdef BRIDGE_IC_IRQ_ENABLE
#define ERROR_STATUS_REG			0xE5
#define IRQ_EN_REG				0xE0
#define IRQ_EN_MASK_REG				0XE1
#endif
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

static unsigned char lvds_bridge_ic_addr = LVDS_REGFLAG_TI941_ADDR;

static  struct LCM_UTIL_FUNCS lcm_util;

static unsigned char  g_secondary_panel_enable;

static struct pinctrl *pinctrl1;
static struct pinctrl_state *rst_output0, *rst_output1, *power_en_output0, *power_en_output1;
#ifdef BRIDGE_IC_IRQ_ENABLE
static struct pinctrl_state *lcm_bridge_chip_eint;
#endif
static struct pinctrl_state *lcm_gowin_bridge_chip_power_en0;
static struct pinctrl_state *lcm_gowin_bridge_chip_power_en1;
static struct pinctrl_state *jac_rst_output0, *jac_rst_output1;

struct jac_mipi_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[JAC_MIPI_PARA_MAX];
};

static unsigned int jac_lcm_enabled;
static unsigned int jac_lcm_type;
static unsigned int jac_lcd_power_delay;
static unsigned int jac_mipi_cnt;
static struct jac_mipi_setting_table jac_mipi_table[JAC_MIPI_CMD_MAX];
static char jac_mipi_raw[JAC_MZ_MIPI_TABLE_ITEMS * JAC_MIPI_ITEM_LEN];

static char *lvds_init_table_text;
static char *mipi_init_table_text;
static char *mipi_suspend_table_text;
static char *power_on_timing_text;
static char *power_off_timing_text;
static char *extended_panel_info;

static struct mipi_setting_info mipi_init;
static struct mipi_setting_info mipi_suspend;
static struct lcm_timing_info 	power_on_timing;
static struct lcm_timing_info 	power_off_timing;
static struct lvds_setting_info lvds_init;
static struct lcm_panel_info 	panel_info;

static unsigned int secondary_width;
static unsigned int secondary_height;


static int crc_check_flag;
extern unsigned int g_mobilelog;
extern size_t g_fb_rsv_mem_vbase;

static const unsigned int table_32[] = {
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static char *default_extended_panel_info =
#if defined(K61V1_64_BSP)
MIPI_INIT_S\
"\r\n"\
"{F0, 05, {55, AA, 52, 08, 00}},\r\n"\
"{6F, 01, {02}},\r\n"\
"{B8, 01, {0C}},\r\n"\
"{B1, 02, {68, 27}},\r\n"\
"{BD, 05, {01, A3, 20, 10, 01}},\r\n"\
"{BB, 02, {74, 74}},\r\n"\
"{BC, 02, {00, 00}},\r\n"\
"{B6, 01, {01}},\r\n"\
"{C8, 01, {83}},\r\n"\
"{F0, 05, {55, AA, 52, 08, 01}},\r\n"\
"{B0, 02, {0F, 0F}},\r\n"\
"{B1, 02, {0F, 0F }},\r\n"\
"{CE, 01, {66}},\r\n"\
"{C0, 01, {0C}},\r\n"\
"{B5, 02, {04, 04}},\r\n"\
"{BE, 01, {2F}},\r\n"\
"{B3, 02, {2B, 2B}},\r\n"\
"{B4, 02, {0F, 0F}},\r\n"\
"{B9, 02, {46, 46}},\r\n"\
"{BA, 02, {16, 16}},\r\n"\
"{BC, 02, {A2, 00}},\r\n"\
"{BD, 02, {A2, 00}},\r\n"\
"{CA, 01, {00}},\r\n"\
"{F0, 05, {55, AA, 52, 08, 02}},\r\n"\
"{ED, 01, {01}},\r\n"\
"{EF, 04, {11, 0D, 16, 19}},\r\n"\
"{F0, 05, {55, AA, 52, 08, 06}},\r\n"\
"{B0, 02, {2E, 09}},\r\n"\
"{B1, 02, {0B, 23}},\r\n"\
"{B2, 02, {1D, 25}},\r\n"\
"{B3, 02, {1F, 11}},\r\n"\
"{B4, 02, {2E, 2E}},\r\n"\
"{B5, 02, {2E, 17}},\r\n"\
"{B6, 02, {13, 19}},\r\n"\
"{B7, 02, {2E, 2E}},\r\n"\
"{B8, 02, {01, 03}},\r\n"\
"{B9, 02, {2E, 2E}},\r\n"\
"{BA, 02, {2E, 2E}},\r\n"\
"{BB, 02, {02, 00}},\r\n"\
"{BC, 02, {2E, 2E}},\r\n"\
"{BD, 02, {18, 12}},\r\n"\
"{BE, 02, {16, 2E}},\r\n"\
"{BF, 02, {2E, 2E}},\r\n"\
"{C0, 02, {10, 1E}},\r\n"\
"{C1, 02, {24, 1C}},\r\n"\
"{C2, 02, {22, 0A}},\r\n"\
"{C3, 02, {08, 2E}},\r\n"\
"{E5, 02, {2E, 2E}},\r\n"\
"{C4, 02, {2E, 02}},\r\n"\
"{C5, 02, {00, 24}},\r\n"\
"{C6, 02, {1E, 22}},\r\n"\
"{C7, 02, {1C, 18}},\r\n"\
"{C8, 02, {2E, 2E}},\r\n"\
"{C9, 02, {2E, 12}},\r\n"\
"{CA, 02, {16, 10}},\r\n"\
"{CB, 02, {2E, 2E}},\r\n"\
"{CC, 02, {0A, 08}},\r\n"\
"{CD, 02, {2E, 2E}},\r\n"\
"{CE, 02, {2E, 2E}},\r\n"\
"{CF, 02, {09, 0B}},\r\n"\
"{D0, 02, {2E, 2E}},\r\n"\
"{D1, 02, {11, 17}},\r\n"\
"{D2, 02, {13, 2E}},\r\n"\
"{D3, 02, {2E, 2E}},\r\n"\
"{D4, 02, {19, 1D}},\r\n"\
"{D5, 02, {23, 1F}},\r\n"\
"{D6, 02, {25, 01}},\r\n"\
"{D7, 02, {03, 2E}},\r\n"\
"{E6, 02, {2E, 2E }},\r\n"\
"{D8, 05, {00, 00, 00, 00, 00}},\r\n"\
"{D9, 05, {00, 00, 00, 00, 00}},\r\n"\
"{E7, 01, {00}},\r\n"\
"{F0, 05, {55, AA, 52, 08, 05}},\r\n"\
"{B0, 02, {17, 06 }},\r\n"\
"{B1, 02, {17, 06}},\r\n"\
"{B2, 02, {17, 06}},\r\n"\
"{B3, 02, {17, 06}},\r\n"\
"{B4, 02, {17, 06}},\r\n"\
"{B5, 02, {17, 06}},\r\n"\
"{B6, 02, {17, 06}},\r\n"\
"{B7, 02, {17, 06}},\r\n"\
"{B8, 01, {00}},\r\n"\
"{B9, 02, {00, 03 }},\r\n"\
"{BA, 02, {00, 03}},\r\n"\
"{BB, 02, {00, 00}},\r\n"\
"{BC, 02, {00, 01}},\r\n"\
"{BD, 05, {0F, 03, 03, 00, 03}},\r\n"\
"{C0, 01, {07}},\r\n"\
"{C1, 01, {05}},\r\n"\
"{C4, 01, {82}},\r\n"\
"{C5, 01, {80}},\r\n"\
"{C8, 02, {03, 20}},\r\n"\
"{C9, 02, {01, 21}},\r\n"\
"{CA, 02, {03, 20}},\r\n"\
"{CB, 02, {07, 20}},\r\n"\
"{D1, 0B, {03, 05, 00, 05, 00, 00, 00, 00, 00, 00, 00}},\r\n"\
"{D2, 0B, {03, 05, 00, 03, 00, 00, 00, 00, 00, 00, 00}},\r\n"\
"{D3, 0B, {03, 05, 04, 05, 00, 00, 00, 00, 00, 00, 00}},\r\n"\
"{D4, 0B, {03, 05, 04, 03, 00, 00, 00, 00, 00, 00, 00}},\r\n"\
"{E5, 01, {06}},\r\n"\
"{E6, 01, {06}},\r\n"\
"{E7, 01, {06}},\r\n"\
"{E8, 01, {06}},\r\n"\
"{E9, 01, {0A}},\r\n"\
"{EA, 01, {06}},\r\n"\
"{EB, 01, {06}},\r\n"\
"{EC, 01, {06}},\r\n"\
"{ED, 01, {30}},\r\n"\
"{F0, 05, {55, AA, 52, 08, 03}},\r\n"\
"{B0, 02, {00, 00}},\r\n"\
"{B1, 02, {00, 00}},\r\n"\
"{B2, 05, {04, 00, 40, 01, 58}},\r\n"\
"{B3, 05, {04, 00, 40, 01, 58}},\r\n"\
"{B6, 05, {04, 00, 46, 01, 68}},\r\n"\
"{B7, 05, {04, 00, 46, 01, 68}},\r\n"\
"{BA, 05, {44, 00, 46, 01, 55}},\r\n"\
"{BB, 05, {44, 00, 46, 01, 55}},\r\n"\
"{BC, 05, {53, 00, 15, 00, 48}},\r\n"\
"{BD, 05, {53, 00, 15, 00, 48}},\r\n"\
"{35, 01, {00}},\r\n"\
"{44, 02, {01, f4}},\r\n"\
"{11, 00, {}},\r\n"\
"{29, 00, {}},\r\n"\
"{F0, 05, {55, AA, 52, 08, 04}},\r\n"\
"{SS, 00, {}},\r\n"\
MIPI_INIT_E\
"\r\n"\
MIPI_SUSPEND_S\
"\r\n"\
"{28, 00, {}},\r\n"\
"{10, 00, {}},\r\n"\
"{MM, 78, {}},\r\n"\
"{4F, 01, {01}},\r\n"\
"{MM, 78, {}},\r\n"\
"{SS, 00, {}},\r\n"\
MIPI_SUSPEND_E\
"\r\n"\
POWER_ON_SEQUENCE_S\
"\r\n"\
"{00, 0F},\r\n"\
"{01, 01},\r\n"\
"{00, 0A}\r\n"\
"{01, 0A}\r\n"\
"{FF, 00}\r\n"\
POWER_ON_SEQUENCE_E\
"\r\n"\
POWER_OFF_SEQUENCE_S\
"\r\n"\
"{00, 10},\r\n"\
"{FF, 00}\r\n"\
POWER_OFF_SEQUENCE_E;
#else
LVDS_INIT_S\
"\r\n"\
"{09, 00},\r\n"\
"{0A, 05},\r\n"\
"{0B, 10},\r\n"\
"{0D, 00},\r\n"\
"{10, 26},\r\n"\
"{11, 00},\r\n"\
"{12, 26},\r\n"\
"{13, 00},\r\n"\
"{18, 78},\r\n"\
"{19, 00},\r\n"\
"{1A, 03},\r\n"\
"{1B, 00},\r\n"\
"{20, 00},\r\n"\
"{21, 05},\r\n"\
"{22, 00},\r\n"\
"{23, 00},\r\n"\
"{24, 00},\r\n"\
"{25, 00},\r\n"\
"{26, 00},\r\n"\
"{27, 00},\r\n"\
"{28, 21},\r\n"\
"{29, 00},\r\n"\
"{2A, 00},\r\n"\
"{2B, 00},\r\n"\
"{2C, 0a},\r\n"\
"{2D, 00},\r\n"\
"{2E, 00},\r\n"\
"{2F, 00},\r\n"\
"{30, 0a},\r\n"\
"{31, 00},\r\n"\
"{32, 00},\r\n"\
"{33, 00},\r\n"\
"{34, 14},\r\n"\
"{35, 00},\r\n"\
"{36, 00},\r\n"\
"{37, 00},\r\n"\
"{38, 00},\r\n"\
"{39, 00},\r\n"\
"{3A, 00},\r\n"\
"{3B, 00},\r\n"\
"{3C, 00},\r\n"\
"{3D, 00},\r\n"\
"{3E, 00},\r\n"\
"{0D, 01},\r\n"\
"{FC, 0a},\r\n"\
"{09, 01},\r\n"\
"{FF, 00}\r\n"\
LVDS_INIT_E;
#endif

#ifdef BRIDGE_IC_IRQ_ENABLE
static const struct of_device_id mtkfb_of_ids[] = {
	{.compatible = "mediatek,MTKFB",},
	{}
};
static unsigned int g_lcm_irq;
static struct work_struct g_eint_work;
static struct workqueue_struct *g_eint_workqueue;
#endif

typedef void (*customer_lcm_callback)(void);
extern void install_lcm_valid_data_start_early_func(customer_lcm_callback func);
extern void install_lcm_valid_data_start_late_func(customer_lcm_callback func);
extern void install_lcm_valid_data_stop_early_func(customer_lcm_callback func);
extern void install_lcm_valid_data_stop_late_func(customer_lcm_callback func);
static void lcm_valid_data_stop_late(void);
static void lcm_valid_data_stop_early(void);
static void lcm_valid_data_start_early(void);
static void lcm_valid_data_start_late(void);
static unsigned char lcm_bridge_ic_detect(unsigned char i2c_id);
void lcm_gpio_output(int pin, int level);


#ifdef BRIDGE_IC_IRQ_ENABLE
static irqreturn_t lcm_interrupt_handler(int irq, void *dev_id);
static void lcm_enable_irq(void);
#endif

unsigned int get_crc_32(char *buf, unsigned int len)
{
	unsigned int crc = 0;
	unsigned int i = 0;

	if ((len < 1) || (NULL == buf)) {

		return 0xffffffff;
	}

	for (i = 0; i != len; ++i) {
		crc = table_32[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
	}

	crc = crc ^ 0xffffffff;

	return crc;
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

#ifdef BRIDGE_IC_IRQ_ENABLE
	lcm_bridge_chip_eint = pinctrl_lookup_state(pinctrl1, "state_eint_as_int");
	if (IS_ERR(lcm_bridge_chip_eint)) {
		ret = PTR_ERR(lcm_bridge_chip_eint);
		lcm_bridge_chip_eint = NULL;
		LCM_DRIVER_ERROR("Cannot find pinctrl state_eint_as_int!\n");
		goto error;
	}
#endif

	lcm_gowin_bridge_chip_power_en0 = pinctrl_lookup_state(pinctrl1, "state_gowin_bridge_chip_power_en0");
	if (IS_ERR(lcm_gowin_bridge_chip_power_en0)) {
		ret = PTR_ERR(lcm_gowin_bridge_chip_power_en0);
		lcm_gowin_bridge_chip_power_en0 = NULL;
		LCM_DRIVER_ERROR("Cannot find pinctrl state_gowin_bridge_chip_power_en0!\n");
		goto error;
	}

	lcm_gowin_bridge_chip_power_en1 = pinctrl_lookup_state(pinctrl1, "state_gowin_bridge_chip_power_en1");
	if (IS_ERR(lcm_gowin_bridge_chip_power_en1)) {
		ret = PTR_ERR(lcm_gowin_bridge_chip_power_en1);
		lcm_gowin_bridge_chip_power_en1 = NULL;
		LCM_DRIVER_ERROR("Cannot find pinctrl state_gowin_bridge_chip_power_en1!\n");
		goto error;
	}

	/* Jancar panel reset; only boards with a MetaZone panel table have it */
	jac_rst_output0 = pinctrl_lookup_state(pinctrl1, "state_jac_rst_output0");
	jac_rst_output1 = pinctrl_lookup_state(pinctrl1, "state_jac_rst_output1");
	if (IS_ERR(jac_rst_output0) || IS_ERR(jac_rst_output1)) {
		jac_rst_output0 = NULL;
		jac_rst_output1 = NULL;
		LCM_DRIVER_INFO("no pinctrl state_jac_rst_output*\n");
	}

	LCM_DRIVER_INFO("leave\n");

	return 0;

error:
	LCM_DRIVER_ERROR("fail, ret=%d.\n", ret);

	return ret;
}
EXPORT_SYMBOL(lcm_get_gpio_info);


void lcm_gpio_output(int pin, int level)
{
	LCM_DRIVER_INFO("lcm_gpio_output pin = %d, level = %d\n", pin, level);

	if ((!pinctrl1) || (!rst_output1) || (!rst_output0) || (!power_en_output1) ||
		(!power_en_output0) || (!lcm_gowin_bridge_chip_power_en0) ||
		(!lcm_gowin_bridge_chip_power_en1)) {
		LCM_DRIVER_ERROR("invalid params!\n");

		return ;
	}
	if (LVDS_BRIDGE_IC_RESET_PIN == pin) {
		if (level) {
			pinctrl_select_state(pinctrl1, rst_output1);
		} else {
			pinctrl_select_state(pinctrl1, rst_output0);
		}

	} else if ((LCM_POWER_EN_PIN == pin) || (JAC_LCD_POWER_PIN == pin)) {
		if (level) {
			pinctrl_select_state(pinctrl1, power_en_output1);
		} else {
			pinctrl_select_state(pinctrl1, power_en_output0);
		}
	} /*else if (GOWIN_BRIDGE_IC_POWER_EN_PIN0 == pin) {
		if (level) {
			pinctrl_select_state(pinctrl1, lcm_gowin_bridge_chip_power_en0);
		} else {
			pinctrl_select_state(pinctrl1, lcm_gowin_bridge_chip_power_en0);
		}
	}*/ else if ((GOWIN_BRIDGE_IC_POWER_EN_PIN1 == pin) || (JAC_BRIDGE_POWER_PIN == pin)) {
		if (level) {
			pinctrl_select_state(pinctrl1, lcm_gowin_bridge_chip_power_en1);
		} else {
			pinctrl_select_state(pinctrl1, lcm_gowin_bridge_chip_power_en0);
		}
	} else if (JAC_LCD_RESET_PIN == pin) {
		if ((!jac_rst_output0) || (!jac_rst_output1)) {
			LCM_DRIVER_ERROR("no jac_rst pinctrl state!\n");
			return;
		}
		if (level) {
			pinctrl_select_state(pinctrl1, jac_rst_output1);
		} else {
			pinctrl_select_state(pinctrl1, jac_rst_output0);
		}
	}

}

#ifdef LVDS_BRIGE_IC_DS90UB941_DS90UB948_ENABLE
static int lcm_bridge_ic_detect_ds90ub941(unsigned char i2c_id)
{
	static int detect_flag;
	unsigned char val = 0;

	if (0 == detect_flag) {
		interface_read_byte(i2c_id, lvds_bridge_ic_addr, 0x00, &val);
		LCM_DRIVER_INFO("TI941:I2C DEVICE ID(7BIT):%02x\n", val);
		if (lvds_bridge_ic_addr == ((val >> 1) & 0x7F)) {
			detect_flag = BRIDGE_IC_DETECT_SUCCESS;
		} else {
			detect_flag = BRIDGE_IC_DETECT_FAILED;
		}
	}

	LCM_DRIVER_INFO("detect_flag = %d\n", detect_flag);
	return detect_flag;
}
#endif/*LVDS_BRIGE_IC_DS90UB941_DS90UB948_ENABLE*/

#ifdef LVDS_BRIGE_IC_DS90UB947_ENABLE
int lcm_bridge_ic_detect_ds90ub947(unsigned char i2c_id)
{
	int detect_flag = 0;
	unsigned char val = 0;

	interface_read_byte(i2c_id, LVDS_REGFLAG_DS90UB947_ADDR, 0x00, &val);
	LCM_DRIVER_INFO("TI947:I2C DEVICE ID(7BIT):%02x\n", val);
	if (LVDS_REGFLAG_DS90UB947_ADDR == ((val >> 1) & 0x7F)) {
		detect_flag = BRIDGE_IC_DETECT_SUCCESS;
	} else {
		detect_flag = BRIDGE_IC_DETECT_FAILED;
	}

	LCM_DRIVER_INFO("detect_flag = %d\n", detect_flag);
	return detect_flag;
}
#endif/*LVDS_BRIGE_IC_DS90UB947_ENABLE*/


#ifdef LVDS_BRIGE_IC_GAOWIN_ENABLE
int lcm_bridge_ic_detect_gaowin(unsigned char i2c_id)
{
	static int detect_flag;
	unsigned char val = 0;

	if (0 == detect_flag) {
		interface_read_byte(i2c_id, LVDS_REGFLAG_GAOWIN_ADDR, 0x00, &val);
		LCM_DRIVER_INFO("gaowin:I2C DEVICE ID(7BIT):0x%02x\n", val);
		if (LVDS_REGFLAG_GAOWIN_ADDR == val) {
			detect_flag = BRIDGE_IC_DETECT_SUCCESS;
		} else {
			detect_flag = BRIDGE_IC_DETECT_FAILED;
		}
	}

	LCM_DRIVER_INFO("detect_flag = %d\n", detect_flag);
	return detect_flag;
}
#endif/*LVDS_BRIGE_IC_GAOWIN_ENABLE*/

#ifdef BRIDGE_IC_IRQ_ENABLE
static int lcm_bridge_ic_detect_sn65dsi83(unsigned char i2c_id)
{
	/*sn65dsi83 ID*/
	unsigned char id[9] = {0x35, 0x38, 0x49, 0x53, 0x44, 0x20, 0x20, 0x20, 0x01};
	unsigned char val = 0;
	int i = 0, ret = 0;

	for (i = 0; i <= 8; i++) {
		interface_read_byte(i2c_id, LVDS_REGFLAG_BRIGE_IC_ADDR, i, &val);
		LCM_DRIVER_INFO("id[%d]:%02x\n", i, val);
		if (val != id[i]) {
			ret = -1;
			break;
		}
	}
	if ((9 == i) && (ret != -1)) {
		ret = 1;
	}
	LCM_DRIVER_INFO("ret = %d\n", ret);
	return ret;
}

void lcm_eint_register(void)
{
	struct device_node *node = NULL;
	int ret = 0;
	if ((!pinctrl1) || (!lcm_bridge_chip_eint)) {
		LCM_DRIVER_ERROR("invalid params!\n");
		return;
	}
	pinctrl_select_state(pinctrl1, lcm_bridge_chip_eint);
	node = of_find_matching_node(node, mtkfb_of_ids);
	if (NULL == node) {
		LCM_DRIVER_ERROR("get node error!\n");
		return;
	}
	g_lcm_irq = irq_of_parse_and_map(node, 0);
	if (0 == g_lcm_irq) {
		LCM_DRIVER_ERROR("irq_of_parse_and_map fail!\n");
		return;
	}
	ret = request_irq(g_lcm_irq, lcm_interrupt_handler, IRQF_TRIGGER_HIGH, "LCM IRQ", NULL);
	if (ret < 0) {
		LCM_DRIVER_ERROR("request_irq fail:%d!\n", g_lcm_irq);
		g_lcm_irq = 0;
	}
}

static irqreturn_t lcm_interrupt_handler(int irq, void *dev_id)
{
	disable_irq_nosync(g_lcm_irq);
	queue_work(g_eint_workqueue, &g_eint_work);
	return IRQ_HANDLED;
}

static void g_eint_work_callback(struct work_struct *work)
{
	LCM_DRIVER_INFO("in\n");
	lcm_valid_data_stop_late();
	lcm_valid_data_start_early();
	lcm_valid_data_start_late();
	LCM_DRIVER_INFO("out!\n");
}

static void lcm_enable_irq(void)
{
	unsigned char data = 0;
	LCM_DRIVER_INFO("enter\n");
	if ((INTERFACE_TYPE_LVDS == panel_info.interface_type) && (lvds_init.cnt)) {
		interface_write_byte(LCM_ID_PRIMARY, LVDS_REGFLAG_BRIGE_IC_ADDR, ERROR_STATUS_REG, 0xff);
		data = 0;
		interface_read_byte(LCM_ID_PRIMARY, LVDS_REGFLAG_BRIGE_IC_ADDR, ERROR_STATUS_REG, &data);
		LCM_DRIVER_INFO("after addr:%x, value:%x\n", ERROR_STATUS_REG, data);
		interface_write_byte(LCM_ID_PRIMARY, LVDS_REGFLAG_BRIGE_IC_ADDR, IRQ_EN_REG, 0x01);
		interface_write_byte(LCM_ID_PRIMARY, LVDS_REGFLAG_BRIGE_IC_ADDR, IRQ_EN_MASK_REG, 0xff);
	}
	LCM_DRIVER_INFO("leave\n");
}

void lcm_init_eint(void)
{
	int ret = -1;
	LCM_DRIVER_INFO("in\n");

	ret = lcm_bridge_ic_detect(LCM_ID_PRIMARY);
	if (ret != SN65DSI83_BRIDGE_IC) {
		LCM_DRIVER_INFO("bridge ic is not SN65DSI83, SN65DSI83 bridge chip EINT not register!\n");
		return;
	}

	g_eint_workqueue = create_singlethread_workqueue("LCM_eint");
	INIT_WORK(&g_eint_work, g_eint_work_callback);
	if (!g_eint_workqueue) {
		LCM_DRIVER_ERROR("create g_eint_workqueue error\n!");
		return;
	}
	lcm_eint_register();
	lcm_enable_irq();
	LCM_DRIVER_INFO("out!\n");
}
#endif/*BRIDGE_IC_IRQ_ENABLE*/

static unsigned char lcm_bridge_ic_detect(unsigned char i2c_id)
{
	int ret = -1;
	static unsigned char type;
	static unsigned char inited;

	if (1 == inited)
		return type;

#ifdef LVDS_BRIGE_IC_GAOWIN_ENABLE
	ret = lcm_bridge_ic_detect_gaowin(i2c_id);
	if (BRIDGE_IC_DETECT_SUCCESS == ret) {
		type = GAOWIN_BRIDGE_IC;
		goto exit;
	}
#endif

#ifdef LVDS_BRIGE_IC_DS90UB941_DS90UB948_ENABLE
	ret = lcm_bridge_ic_detect_ds90ub941(i2c_id);
	if (BRIDGE_IC_DETECT_SUCCESS == ret) {
		type = DS90UB941_BRIDGE_IC;
		goto exit;
	}
#endif

#ifdef LVDS_BRIGE_IC_SN65DSI83_ENABLE
	ret = lcm_bridge_ic_detect_sn65dsi83(i2c_id);
	if (BRIDGE_IC_DETECT_SUCCESS == ret) {
		type =  SN65DSI83_BRIDGE_IC;
		goto exit;
	}
#endif


exit:
	inited = 1;

	return type;
}

static int text_prepare(char *text, int len)
{
	int ret = 0, i = 0, j = 0;
	unsigned char a = 0;

	LCM_DRIVER_INFO("enter\n");
	if (NULL == text) {
		ret = -EINVAL;
		goto error;
	}

	for (i = 0, j = 0; i < len ; i++) {
		a = *(text+i);
		if ((' ' == a) || ('{' == a) || ('}' == a) || (',' == a) || ('\r' == a) || ('\n' == a)) {
			continue;
		}

		if (((a >= '0') && (a  <= '9')) || ((a >= 'A') && (a  <= 'F')) || ((a >= 'a') && (a  <= 'f'))) {
			*(text+j++) = a;
		} else {
			LCM_DRIVER_FATAL("invalid character (offset=0x%X):0x%X(%c).\n", i, a, a);
			ret = -EINVAL;
			goto error;
		}
	}

	*(text+j) = '\0';
	LCM_DRIVER_INFO("leave\n");

	return 0;
error:
	LCM_DRIVER_FATAL("fail, ret=%d.\n", ret);

	return ret;
}

static int lvds_text_deal(struct lvds_setting_info *setting_info, char *text)
{
	int i = 0, j = 0, len = 0, ret = 0;
	unsigned char tmp[3] = {0};
	struct lvds_setting_table_list *new_ptr = NULL, *tmp_ptr = NULL;
	LCM_DRIVER_INFO("enter\n");
	if (!setting_info) {
		LCM_DRIVER_FATAL("invalid param, setting_info is NULL.\n");
		ret = -EINVAL;
		goto error;
	}

	if (!text) {
		LCM_DRIVER_FATAL("invalid param, Text is NULL.\n");
		ret = -EINVAL;
		goto error;
	}

	len = strlen(text);

	if (0 != len%4) {
		LCM_DRIVER_FATAL("invalid text len, len=0x%X.\n", len);
		ret = -EINVAL;
		goto error;
	}
	/*len check*/
	tmp_ptr = &(setting_info->head_list);
	for (i = 0, j = 0; i < len; i = i+4, j++) {
		new_ptr = (struct lvds_setting_table_list *)kzalloc(sizeof(struct lvds_setting_table_list),
								    GFP_KERNEL);
		if (!new_ptr) {
			LCM_DRIVER_FATAL("kzalloc fail!\n");
			ret = -EINVAL;
			goto error;
		}
		new_ptr->next = NULL;
		/*cmd*/
		tmp[2] = '\0';
		tmp[0] =  *(text+i);
		tmp[1] = *(text+i+1);
		new_ptr->setting_table.cmd = (unsigned char)simple_strtol((char *)tmp, NULL, 16);
		/*data*/
		tmp[0] = *(text+i+2);
		tmp[1] = *(text+i+3);
		new_ptr->setting_table.data = (unsigned char)simple_strtol(tmp, NULL, 16);

		tmp_ptr->next = new_ptr;
		tmp_ptr = new_ptr;
	}

	if (i != len) {
		LCM_DRIVER_FATAL("invalid text len (len:0x%X, i:0x%X)!\n", len, i);
		ret = -EINVAL;
		goto error;
	}
	setting_info->cnt = j;
	LCM_DRIVER_INFO("leave\n");

	return 0;
error:
	new_ptr = setting_info->head_list.next;
	while (NULL != new_ptr) {
		tmp_ptr = new_ptr;
		new_ptr = new_ptr->next;
		kfree(tmp_ptr);
	}
	setting_info->head_list.next = NULL;
	LCM_DRIVER_FATAL("fail, ret=%d.\n", ret);

	return ret;
}
static int lvds_setting_info_init(struct lvds_setting_info *setting_info, char *text, int text_len)
{
	int ret = 0;

	LCM_DRIVER_INFO("enter\n");
	if ((NULL == setting_info) || (NULL == text)) {
		LCM_DRIVER_FATAL("invalid param!\n");
		ret = -EINVAL;
		goto error;
	}

	ret = text_prepare(text, text_len);
	if (ret) {
		LCM_DRIVER_FATAL("Text invalid, text:%s.\n", text);
		goto error;
	}

	ret = lvds_text_deal(setting_info, text);
	if (ret) {
		LCM_DRIVER_FATAL("Text deal fail!\n");
		goto error;
	}
	LCM_DRIVER_INFO("leave\n");

	return 0;
error:
	LCM_DRIVER_FATAL("fail, ret=%d.\n", ret);

	return ret;
}

static void check_serializer_link_ready(unsigned char i2c_id)
{
	unsigned int i = 0;
	int ret = 0;
	char send_buf[3] = {0};
	char rcv_buf[3] = {0};

	ret = lcm_bridge_ic_detect_ds90ub947(i2c_id);
	if (BRIDGE_IC_DETECT_SUCCESS != ret)
		return;

	/*check serializer FPD-Link III Ready?*/
	for (i = 0; i <= 10; i++) {
		MDELAY(5);//wait serializer FPD-Link III Ready again
		send_buf[0] = 0x5A;
		ret = interface_read_byte(i2c_id, LVDS_REGFLAG_DS90UB947_ADDR, send_buf[0], &rcv_buf[0]);
		if (ret < 0) {
			LCM_DRIVER_ERROR("I2C read fail(%d), addr:0x%x, send_buf:0x%x\n",
					ret, LVDS_REGFLAG_DS90UB947_ADDR, send_buf[0]);
			break;
		} else {
			if ( !((1 << 7) & rcv_buf[0]) ) {
				LCM_DRIVER_ERROR("serializer FPD-Link III not ready,	\
						 I2C read(addr(0x%02x)=val(0x%02x)) i = %d\n",
						 send_buf[0], rcv_buf[0], i);
				continue;
			}
			LCM_DRIVER_INFO("serializer FPD-Link III Ready\n");
			break;
		}
	}
}

static void lvds_push_table(unsigned char i2c_id, struct lvds_setting_info *setting_info)
{
	unsigned int i = 0;
	unsigned char cmd = 0, data = 0;
	unsigned char addr = LVDS_REGFLAG_BRIGE_IC_ADDR;
	struct lvds_setting_table_list *tmp_list = NULL;

	LCM_DRIVER_INFO("enter\n");

	if (NULL == setting_info) {
		LCM_DRIVER_FATAL("invalid param!\n");

		return;
	}

	tmp_list = setting_info->head_list.next;
	for (; NULL != tmp_list; tmp_list = tmp_list->next) {
		cmd = tmp_list->setting_table.cmd;
		data = tmp_list->setting_table.data;
		switch (cmd) {
		case LVDS_REGFLAG_MDELAY:
			MDELAY(data);
			break;

		case LVDS_REGFLAG_ADDR_SELECT:
			addr = data;
			if (LVDS_REGFLAG_TI948_ADDR == addr) {
				check_serializer_link_ready(i2c_id);
			}
			break;
		case LVDS_REGFLAG_END_OF_TABLE:
			break;
		default:
			interface_write_byte(i2c_id, addr, cmd, data);
		}
		i++;
	}

	if ((setting_info->cnt != i) || (LVDS_REGFLAG_END_OF_TABLE != cmd)) {
		LCM_DRIVER_FATAL("cmd send incomplete, cnt= %d, send = %d, end cmd=%X!\n", setting_info->cnt, i, cmd);
	}
	LCM_DRIVER_INFO("leave\n");

}

static void push_table(struct mipi_setting_info *setting_info)
{
	unsigned int cmd = 0, i = 0;
	unsigned char cnt = 0;
	unsigned char *para_list = NULL;
	struct mipi_setting_table_list *tmp_list = NULL;

	LCM_DRIVER_INFO("enter\n");
	if (NULL == setting_info) {
		LCM_DRIVER_FATAL("invalid param!\n");

		return;
	}

	LCM_DRIVER_DEBUG("setting_info.cnt:%d\n", setting_info->cnt);
	for (tmp_list = setting_info->head_list.next; NULL != tmp_list; tmp_list = tmp_list->next) {
		cmd = tmp_list->setting_table.cmd;
		cnt = tmp_list->setting_table.cnt;
		para_list = tmp_list->setting_table.para_list;
		i++;
		switch (cmd) {
		case MIPI_REGFLAG_MDELAY:
			MDELAY(cnt);
			break;
		case MIPI_REGFLAG_UDELAY:
			UDELAY(cnt);
			break;
		case MIPI_REGFLAG_END_OF_TABLE:
			break;
		default:
			dsi_set_cmdq_V22(NULL, cmd, cnt, para_list, 1);
		}

	}
	if ((setting_info->cnt != i) || (MIPI_REGFLAG_END_OF_TABLE != cmd)) {
		LCM_DRIVER_FATAL("cmd send incomplete, cnt= %d, send = %d, end cmd=%X!\n", setting_info->cnt, i, cmd);
	}

	LCM_DRIVER_INFO("leave\n");
}

/*
 * Jancar MetaZone panel table: a byte stream of records <len><cmd><len - 1
 * parameters>, where <3><0xff><0xff><ms> is a delay.
 */
static void jac_lcm_analysis_mipi_params_form_MetaZone(void)
{
	unsigned int total = 0, left, i = 0, n;
	unsigned char cnt;
	unsigned char *p = (unsigned char *)jac_mipi_raw;
	unsigned int idx;

	jac_mipi_cnt = 0;
	MetaZone_Read(JAC_MZ_MIPI_TABLE_LEN, &total);
	if (0 == total) {
		LCM_DRIVER_INFO("[LCM] no mipi para\n");
		return;
	}
	if (total > JAC_MIPI_TABLE_LEN_MAX) {
		LCM_DRIVER_INFO("[LCM] mipi para to big\n");
		return;
	}

	for (idx = 0; idx * JAC_MIPI_ITEM_LEN < total; idx++) {
		MetaZone_ReadBinary(JAC_MZ_MIPI_TABLE + idx, jac_mipi_raw + idx * JAC_MIPI_ITEM_LEN,
				    JAC_MIPI_ITEM_LEN);
	}
	LCM_DRIVER_INFO("total :%d\n", total);

	left = total;
	while (left && (i < JAC_MIPI_CMD_MAX)) {
		cnt = p[0];
		if ((0 == cnt) || (cnt + 1 > left)) {
			break;
		}
		if ((3 == cnt) && (0xff == p[1]) && (0xff == p[2])) {
			jac_mipi_table[i].cmd = MIPI_REGFLAG_MDELAY;
			jac_mipi_table[i].count = p[3];
		} else {
			n = cnt - 1;
			if (n > JAC_MIPI_PARA_MAX) {
				LCM_DRIVER_FATAL("mipi[%d]:%x, %d parameters, too many!\n", i, p[1], n);
				break;
			}
			jac_mipi_table[i].cmd = p[1];
			jac_mipi_table[i].count = n;
			memcpy(jac_mipi_table[i].para_list, p + 2, n);
		}
		LCM_DRIVER_DEBUG("mipi[%d]:%x, %d\n", i, jac_mipi_table[i].cmd, jac_mipi_table[i].count);
		p += cnt + 1;
		left -= cnt + 1;
		i++;
	}
	jac_mipi_cnt = i;
	LCM_DRIVER_INFO("mipi cmd:%d\n", jac_mipi_cnt);
}

/*
 * Stock overwrites the DSI params with the MetaZone ones; here they only get
 * logged (and compared), the boot params from LK already drive this panel.
 */
static void jac_lcm_get_mipi_params_form_MetaZone(struct LCM_PARAMS *params)
{
	unsigned int enable = 0, lane = 0, clk = 0, width = 0, height = 0, bit = 0;
	unsigned int hpw = 0, hbp = 0, hfp = 0, vpw = 0, vbp = 0, vfp = 0;
	static unsigned int inited, config_found;

	if (inited) {
		goto exit;
	}
	inited = 1;

	MetaZone_Read(JAC_MZ_CONFIG_ENABLE, &enable);
	LCM_DRIVER_INFO("[LCM] lcm_config_enable():%x\n", enable);
	if (JAC_CONFIG_ENABLE_MAGIC != enable) {
		return;
	}
	config_found = 1;

	MetaZone_Read(JAC_MZ_MIPI_LANE, &lane);
	MetaZone_Read(JAC_MZ_MIPI_CLK, &clk);
	MetaZone_Read(JAC_MZ_HPW, &hpw);
	MetaZone_Read(JAC_MZ_HBP, &hbp);
	MetaZone_Read(JAC_MZ_HFP, &hfp);
	MetaZone_Read(JAC_MZ_VPW, &vpw);
	MetaZone_Read(JAC_MZ_VBP, &vbp);
	MetaZone_Read(JAC_MZ_VFP, &vfp);
	MetaZone_Read(JAC_MZ_HEIGHT, &height);
	MetaZone_Read(JAC_MZ_WIDTH, &width);
	MetaZone_Read(JAC_MZ_BIT, &bit);
	MetaZone_Read(JAC_MZ_LCD_POWER_DELAY, &jac_lcd_power_delay);
	MetaZone_Read(JAC_MZ_LCM_TYPE, &jac_lcm_type);

	LCM_DRIVER_INFO("[LCM] mipi lane(%d) clk(%d) hpw(%d) hbp(%d) hfp(%d) vpw(%d) vbp(%d) vfp(%d)\n",
			lane, clk, hpw, hbp, hfp, vpw, vbp, vfp);
	LCM_DRIVER_INFO("[LCM] %dx%d bit(%d) lcd_power_delay(%d) jac_lcm_tpye(%d)\n",
			width, height, bit, jac_lcd_power_delay, jac_lcm_type);
	if ((lane != params->dsi.LANE_NUM) || (clk != params->dsi.PLL_CLOCK) ||
	    (width != params->width) || (height != params->height) ||
	    (hpw != params->dsi.horizontal_sync_active) || (hbp != params->dsi.horizontal_backporch) ||
	    (hfp != params->dsi.horizontal_frontporch) || (vpw != params->dsi.vertical_sync_active) ||
	    (vbp != params->dsi.vertical_backporch) || (vfp != params->dsi.vertical_frontporch)) {
		LCM_DRIVER_ERROR("MetaZone panel params differ from LK ones, LK ones kept!\n");
	}

	jac_lcm_analysis_mipi_params_form_MetaZone();

exit:
	/* the reset pin comes from lcm_get_gpio_info() */
	jac_lcm_enabled = (config_found && jac_rst_output0 && jac_rst_output1) ? 1 : 0;
	LCM_DRIVER_INFO("jac panel init on resume:%d\n", jac_lcm_enabled);
}

static void jac_push_table(struct jac_mipi_setting_table *table, unsigned int count,
			   unsigned char force_update)
{
	unsigned int i;

	LCM_DRIVER_INFO("[LCM] :%d\n", count);
	for (i = 0; i < count; i++) {
		switch (table[i].cmd) {
		case MIPI_REGFLAG_MDELAY:
			MDELAY(table[i].count);
			break;
		case MIPI_REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;
		case MIPI_REGFLAG_END_OF_TABLE:
			break;
		default:
			dsi_set_cmdq_V22(NULL, table[i].cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

/* power on, reset and init sequence of the stock lcm_init */
static void jac_lcm_init(void)
{
	if (jac_lcd_power_delay > JAC_LCD_POWER_DELAY_MAX) {
		jac_lcd_power_delay = JAC_LCD_POWER_DELAY_MAX;
	}

	if (1 == jac_lcm_type) {
		if (jac_lcd_power_delay < 3) {
			lcm_gpio_output(JAC_LCD_POWER_PIN, 1);
		}
	} else {
		lcm_gpio_output(JAC_LCD_POWER_PIN, 1);
		lcm_gpio_output(JAC_BRIDGE_POWER_PIN, 1);
	}

	lcm_gpio_output(JAC_LCD_RESET_PIN, 0);
	lcm_gpio_output(LVDS_BRIDGE_IC_RESET_PIN, 0);
	MDELAY(20);
	lcm_gpio_output(JAC_LCD_RESET_PIN, 1);
	lcm_gpio_output(LVDS_BRIDGE_IC_RESET_PIN, 1);
	MDELAY(30);

	if ((0 == jac_lcm_type) && (jac_lcd_power_delay >= 3)) {
		MDELAY(50);
		lcm_gpio_output(JAC_LCD_RESET_PIN, 0);
		MDELAY(jac_lcd_power_delay);
		lcm_gpio_output(JAC_LCD_RESET_PIN, 1);
		MDELAY(140);
	}

	if (jac_mipi_cnt) {
		jac_push_table(jac_mipi_table, jac_mipi_cnt, 1);
	} else {
		lvds_push_table(LCM_ID_PRIMARY, &lvds_init);
	}
	MDELAY(10);

	if ((1 == jac_lcm_type) && (jac_lcd_power_delay >= 3)) {
		MDELAY(jac_lcd_power_delay);
		lcm_gpio_output(JAC_LCD_POWER_PIN, 1);
	}
}

static int jac_set_lcd_power_kthread(void *data)
{
	MDELAY(80);
	lcm_gpio_output(JAC_LCD_POWER_PIN, 0);
	lcm_gpio_output(JAC_BRIDGE_POWER_PIN, 0);
	LCM_DRIVER_INFO("leave\n");

	return 0;
}

static int lcm_timing_text_deal(struct lcm_timing_info *lcm_timing, char *text)
{
	int i = 0, j = 0, len = 0, ret = 0;
	unsigned char tmp[3] = {0};
	struct gpio_ctl_cmd_list *new_ptr = NULL, *tmp_ptr = NULL;

	LCM_DRIVER_INFO("enter\n");
	if (!lcm_timing) {
		LCM_DRIVER_FATAL("invalid param, setting_info is NULL.\n");
		ret = -EINVAL;
		goto error;
	}

	if (!text) {
		LCM_DRIVER_FATAL("invalid param, Text is NULL.\n");
		ret = -EINVAL;
		goto error;
	}

	len = strlen(text);

	if (0 != len%4) {
		LCM_DRIVER_FATAL("invalid text len, len=0x%X.\n", len);
		ret = -EINVAL;
		goto error;
	}
	/*len check*/
	tmp_ptr = &(lcm_timing->head_list);
	for (i = 0, j = 0; i < len; i = i+4, j++) {
		new_ptr = (struct gpio_ctl_cmd_list *)kzalloc(sizeof(struct gpio_ctl_cmd_list), GFP_KERNEL);
		if (!new_ptr) {
			LCM_DRIVER_FATAL("kzalloc fail!\n");
			ret = -EINVAL;
			goto error;
		}
		new_ptr->next = NULL;
		/*level*/
		tmp[2] = '\0';
		tmp[0] = *(text+i);
		tmp[1] = *(text+i+1);
		new_ptr->cmd.level = (unsigned char)simple_strtol((char *)tmp, NULL, 16);
		/*duration*/
		tmp[0] = *(text+i+2);
		tmp[1] = *(text+i+3);
		new_ptr->cmd.duration = (unsigned char)simple_strtol((char *)tmp, NULL, 16);

		tmp_ptr->next = new_ptr;
		tmp_ptr = new_ptr;
	}

	if (i != len) {
		LCM_DRIVER_FATAL("invalid text len (len:0x%X, i:0x%X)!\n", len, i);
		ret = -EINVAL;
		goto error;
	}
	lcm_timing->cnt = j;
	LCM_DRIVER_INFO("leave\n");

	return 0;
error:
	new_ptr = lcm_timing->head_list.next;
	while (NULL != new_ptr) {
		tmp_ptr = new_ptr;
		new_ptr = new_ptr->next;
		kfree(tmp_ptr);
	}
	lcm_timing->head_list.next = NULL;
	LCM_DRIVER_FATAL("fail, ret=%d.\n", ret);

	return ret;
}


static int lcm_timing_info_init(struct lcm_timing_info *lcm_timing, char *text, int text_len)
{
	int ret = 0;

	LCM_DRIVER_INFO("enter\n");
	if (!lcm_timing) {
		LCM_DRIVER_FATAL("mipiIcInit:NULL\n");
		ret = -EINVAL;
		goto error;
	}

	if (!text) {
		LCM_DRIVER_FATAL("text:NULL\n");
		ret = -EINVAL;
		goto error;
	}

	if (!text_len) {
		LCM_DRIVER_FATAL("textLen:0\n");
		ret = -EINVAL;
		goto error;
	}

	ret = text_prepare(text, text_len);
	if (ret) {
		LCM_DRIVER_FATAL("Text invalid, text:%s.\n", text);
		ret = -EINVAL;
		goto error;
	}

	ret = lcm_timing_text_deal(lcm_timing, text);
	if (ret) {
		LCM_DRIVER_FATAL("Text deal fail!\n");
		ret = -EINVAL;
		goto error;
	}
	LCM_DRIVER_INFO("leave\n");

	return 0;

error:
	LCM_DRIVER_FATAL("fail, ret=%d.\n", ret);

	return ret;
}

static int mipi_text_prepare(char *text, int len)
{
	int ret = 0, i = 0, j = 0;
	unsigned char a = 0;

	LCM_DRIVER_INFO("enter\n");
	for (i = 0, j = 0; i < len ; i++) {
		a = *(text+i);
		if ((' ' == a) || ('{' == a) || ('}' == a) || (',' == a) || ('\r' == a) || ('\n' == a)) {
			continue;
		}

		if (((a >= '0') && (a  <= '9')) || ((a >= 'A') && (a  <= 'F')) || ((a >= 'a') && (a  <= 'f')) ||
		    (a == 'M') || (a == 'U') || (a == 'S')) {
			*(text+j++) = a;
		} else {
			LCM_DRIVER_FATAL("invalid character (offset=0x%X):0x%X(%c).\n", i, a, a);
			ret = -EINVAL;
			goto error;
		}
	}

	*(text+j) = '\0';
	LCM_DRIVER_INFO("leave\n");

	return 0;
error:
	LCM_DRIVER_FATAL("fail, ret=%d.\n", ret);

	return ret;
}


static int mipi_setting_info_text_deal(struct mipi_setting_info *setting_info, char *text)
{
	int i = 0, j = 0, k = 0, len = 0, ret = 0, cnt = 0;
	unsigned char tmp[3] = {0};
	struct mipi_setting_table_list *new_ptr = NULL, *tmp_ptr = NULL;

	LCM_DRIVER_INFO("enter\n");
	if (!setting_info) {
		LCM_DRIVER_FATAL("invalid param, setting_info is NULL.\n");
		ret = -EINVAL;
		goto error;
	}

	if (!text) {
		LCM_DRIVER_FATAL("invalid param, Text is NULL.\n");
		ret = -EINVAL;
		goto error;
	}

	len = strlen(text);
	LCM_DRIVER_DEBUG("text len:=0x%X\n", len);
	tmp_ptr = &(setting_info->head_list);
	for (i = 0, j = 0, k = 0; i < len; i = i+4+j*2, k++) {
		new_ptr = (struct mipi_setting_table_list *)kzalloc(sizeof(struct mipi_setting_table_list),
								    GFP_KERNEL);
		if (!new_ptr) {
			LCM_DRIVER_FATAL("kzalloc fail!\n");
			ret = -EINVAL;
			goto error;
		}
		new_ptr->next = NULL;
		/*cmd*/
		tmp[2] = '\0';
		tmp[0] = *(text+i);
		tmp[1] = *(text+i+1);
		if (('M' != tmp[0]) && ('U' != tmp[0]) && ('S' != tmp[0]) &&
		    ('M' != tmp[1]) && ('U' != tmp[1]) && ('S' != tmp[1])) {
			new_ptr->setting_table.cmd = (unsigned char)simple_strtol((char *)tmp, NULL, 16);
		} else if (('M' == tmp[0]) && ('M' == tmp[1])) {
			new_ptr->setting_table.cmd = MIPI_REGFLAG_MDELAY;
		} else if (('U' == tmp[0]) && ('U' == tmp[1])) {
			new_ptr->setting_table.cmd = MIPI_REGFLAG_UDELAY;
		} else if (('S' == tmp[0]) && ('S' == tmp[1])) {
			new_ptr->setting_table.cmd = MIPI_REGFLAG_END_OF_TABLE;
		} else {
			LCM_DRIVER_FATAL("cmd invalid character (offset=0x%X):(%c%c)\n", i, tmp[0], tmp[1]);
			ret = -EINVAL;
			goto error;
		}

		/*cnt*/
		tmp[0] = *(text+i+2);
		tmp[1] = *(text+i+3);
		if (('M' == tmp[0]) || ('U' == tmp[0]) || ('S' == tmp[0]) ||
		    ('M' == tmp[1]) || ('U' == tmp[1]) || ('S' == tmp[1])) {
			LCM_DRIVER_FATAL("cnt invalid character (offset=0x%X):(%c%c)\n", i, tmp[0], tmp[1]);
			ret = -EINVAL;
			goto error;
		}
		cnt = simple_strtol((char *)tmp, NULL, 16);
		new_ptr->setting_table.cnt = (unsigned char)cnt;
		if ((MIPI_REGFLAG_MDELAY == new_ptr->setting_table.cmd) ||
		    (MIPI_REGFLAG_UDELAY == new_ptr->setting_table.cmd) ||
		    (MIPI_REGFLAG_END_OF_TABLE == new_ptr->setting_table.cmd)) {
			cnt = 0;
		}
		/*para_list*/
		if (cnt*2 > (len - i - 4)) {
			LCM_DRIVER_FATAL("para_list invalid cnt:0x%X, (offset=0x%X):(%c%c)\n",
					 cnt, i, tmp[0], tmp[1]);
			ret = -EINVAL;
			goto error;
		}
		for (j = 0; j < cnt; j++) {
			tmp[0] = *(text+i+4+j*2);
			tmp[1] = *(text+i+5+j*2);
			if (('M' == tmp[0]) || ('U' == tmp[0]) || ('S' == tmp[0]) ||
			    ('M' == tmp[1]) || ('U' == tmp[1]) || ('S' == tmp[1])) {
				LCM_DRIVER_FATAL("para_list invalid character (offset=0x%X):(%c%c)\n",
						 i, tmp[0], tmp[1]);
				ret = -EINVAL;
				goto error;
			}
			new_ptr->setting_table.para_list[j] = (unsigned char)simple_strtol((char *)tmp, NULL, 16);
		}
		tmp_ptr->next = new_ptr;
		tmp_ptr = new_ptr;
	}

	if (i != len) {
		LCM_DRIVER_FATAL("invalid text len (len:0x%X, i:0x%X)!\n", len, i);
		ret = -EINVAL;
		goto error;
	}
	setting_info->cnt = k;
	LCM_DRIVER_INFO("leave\n");

	return 0;
error:
	new_ptr = setting_info->head_list.next;
	while (NULL != new_ptr) {
		tmp_ptr = new_ptr;
		new_ptr = new_ptr->next;
		kfree(tmp_ptr);
	}
	setting_info->head_list.next = NULL;
	LCM_DRIVER_FATAL("fail, ret=%d.\n", ret);

	return ret;
}

void lcm_timing_ctl(struct lcm_timing_info *lcm_timing)
{
	unsigned char level = 0, duration = 0, i = 0;
	struct gpio_ctl_cmd_list *tmp_list = NULL;

	LCM_DRIVER_INFO("enter\n");
	if (!lcm_timing) {
		LCM_DRIVER_FATAL("lcm_timing:NULL\n");

		return;
	}

	if (!lcm_timing->cnt) {
		LCM_DRIVER_FATAL("cnt:%d, return.\n", lcm_timing->cnt);

		return;
	}
	LCM_DRIVER_DEBUG("lcm_timing.cnt= %d\n", lcm_timing->cnt);
	tmp_list = lcm_timing->head_list.next;
	for (i = 0; NULL != tmp_list; tmp_list = tmp_list->next, i++) {
		level = tmp_list->cmd.level;
		if (TIMING_CTL_END_OF_TABLE == level) {
			continue;
		}
		duration = tmp_list->cmd.duration;
		SET_RESET_PIN(level);
		if (duration != 0) {
			MDELAY(duration);
		}
	}
	if ((lcm_timing->cnt != i) || (TIMING_CTL_END_OF_TABLE != level)) {
		LCM_DRIVER_FATAL("cmd send incomplete, cnt= %d, send= %d, end cmd=%X!\n", lcm_timing->cnt, i, level);
	}
	LCM_DRIVER_INFO("leave\n");
}

void lcm_panel_info_init(struct lcm_panel_info *panel_info)
{
	struct lcm_panel_header *panel_header = NULL;
	int valid_flag = 0;
	unsigned int crc_val = 0, crc_val1 = 0;

	LCM_DRIVER_INFO("enter\n");
	if (!panel_info) {
		LCM_DRIVER_FATAL("panel_info:NULL\n");
		goto check_valid;
	}
	LCM_DRIVER_INFO("fb rsv memory vbase: 0x%x\n", g_fb_rsv_mem_vbase);

	panel_header = (struct lcm_panel_header *)(g_fb_rsv_mem_vbase + MRF_PANEL_INFO_OFF);

	crc_val = get_crc_32((char *)&(panel_header->primary_panel_info),
			     sizeof(struct lcm_panel_info) - MRF_CRC_LEN);
	if (crc_val != panel_header->primary_panel_info.crc) {
		LCM_DRIVER_FATAL("primary_panel_info:CRC check fail, [%08x, %08x]!\n",
				 crc_val, panel_header->primary_panel_info.crc);
		goto check_valid;
	}
	LCM_DRIVER_INFO("primary_panel_info:CRC check success, [%08x, %08x]!\n",
			crc_val, panel_header->primary_panel_info.crc);
	LCM_DRIVER_INFO("panel_header->panel_count:%d\n", panel_header->panel_count);
	if (2 == panel_header->panel_count) {
		crc_val = get_crc_32((char *)&(panel_header->secondary_panel_info),
				     sizeof(struct lcm_panel_info) - MRF_CRC_LEN);
		if (crc_val != panel_header->secondary_panel_info.crc) {
			LCM_DRIVER_FATAL("secondary_panel_info:CRC check fail, [%08x, %08x]!\n",
					 crc_val, panel_header->secondary_panel_info.crc);
			goto check_valid;
		}
		LCM_DRIVER_INFO("secondary_panel_info:CRC check success, [%08x, %08x]!\n",
			crc_val, panel_header->secondary_panel_info.crc);
		secondary_height = panel_header->secondary_panel_info.pixel_height;
		secondary_width = panel_header->secondary_panel_info.pixel_width;
	}

	crc_check_flag = 1;

	memcpy(panel_info, &panel_header->primary_panel_info, sizeof(struct lcm_panel_info));
	LCM_DRIVER_INFO("panel_info->size:0x%X!\n", panel_info->size);
	if ((UINT32_INVALD_FLAG == panel_info->size) || (0 == panel_info->size)  ||
	    (MAX_DATA_LEN <= panel_info->size)) {
		LCM_DRIVER_INFO("invalid size:0x%X!\n", panel_info->size);
		goto check_valid;
	}

	extended_panel_info = kzalloc(panel_info->size + MRF_CRC_LEN, GFP_KERNEL);
	if (NULL == extended_panel_info) {
		LCM_DRIVER_FATAL("extended_panel_info:NULL!\n");
		goto check_valid;
	}
	memcpy(extended_panel_info, (char *)g_fb_rsv_mem_vbase + MRF_HEADER_INFO_LEN, panel_info->size + MRF_CRC_LEN);
	crc_val = get_crc_32((char *)extended_panel_info, panel_info->size);
	crc_val1 = ((*(extended_panel_info + panel_info->size)) |
		    ((*(extended_panel_info + panel_info->size + 1)) << 8) |
		    ((*(extended_panel_info + panel_info->size + 2)) << 16) |
		    ((*(extended_panel_info + panel_info->size + 3)) << 24));
	if (crc_val  == crc_val1) {
		LCM_DRIVER_INFO("extended_panel_info:CRC check success, [%08x, %08x]!\n", crc_val, crc_val1);
		*(extended_panel_info + panel_info->size) = '\0';
		valid_flag = 1;
	} else {
		LCM_DRIVER_FATAL("extended_panel_info:CRC check fail, [%08x, %08x]!\n", crc_val, crc_val1);
		kfree(extended_panel_info);
		extended_panel_info = NULL;
	}

check_valid:
	if (!valid_flag) {
		LCM_DRIVER_INFO("use default_extended_panel_info!\n");
		extended_panel_info = default_extended_panel_info;
	}

#if defined(K61V1_64_BSP)
	if (crc_check_flag) {
		if (UINT16_INVALD_FLAG == panel_info->interface_type) {
			panel_info->interface_type = INTERFACE_TYPE_MIPI;
		}
	} else {
		panel_info->interface_type = INTERFACE_TYPE_MIPI;
	}
#else
	if (crc_check_flag) {
		if (UINT16_INVALD_FLAG == panel_info->interface_type) {
			panel_info->interface_type = INTERFACE_TYPE_LVDS;
		}
	} else {
		panel_info->interface_type = INTERFACE_TYPE_LVDS;
	}
#endif

	LCM_DRIVER_INFO("leave\n");
}
int mipi_setting_info_init(struct mipi_setting_info *setting_info, char *text, int textLen)
{
	int ret = -1;

	LCM_DRIVER_INFO("enter\n");
	if (!setting_info) {
		LCM_DRIVER_FATAL("setting_info:NULL\n");
		ret = -EINVAL;
		goto error;
	}

	if (!text) {
		LCM_DRIVER_FATAL("text:NULL\n");
		ret = -EINVAL;
		goto error;
	}

	if (!textLen) {
		LCM_DRIVER_FATAL("textLen:0\n");
		ret = -EINVAL;
		goto error;
	}

	ret = mipi_text_prepare(text, textLen);
	if (ret) {
		LCM_DRIVER_FATAL("Text invalid, text:%s.\n", text);
		ret = -EINVAL;
		goto error;
	}

	ret = mipi_setting_info_text_deal(setting_info, text);
	if (ret) {
		LCM_DRIVER_FATAL("Text deal fail!\n");
		ret = -EINVAL;
		goto error;
	}
	LCM_DRIVER_INFO("leave\n");

	return 0;

error:
	LCM_DRIVER_FATAL("fail, ret=%d.\n", ret);

	return ret;

}

int get_specific_part_extended_panel_info(char **ptr, char *start, char *end)
{
	int ret = 0, len = 0, sum = 0;
	char *ptr_s = NULL, *ptr_e = NULL;
	static int processed_len;

	if (NULL == ptr) {
		LCM_DRIVER_FATAL("ptr is NULL.\n");
		ret = -EINVAL;
		goto error;
	}
	if ((NULL == start) || (NULL == end)) {
		LCM_DRIVER_FATAL("invalid parameters!\n");
		ret = -EINVAL;
		goto error;
	}
	if (extended_panel_info == NULL) {
		LCM_DRIVER_FATAL("extended_panel_info is NULL.\n");
		ret = -EINVAL;
		goto error;
	}

	sum = strlen(extended_panel_info);
	LCM_DRIVER_INFO("extended_panel_info:0x%X!\n", sum);
	ptr_s = strstr(extended_panel_info, start);
	ptr_e = strstr(extended_panel_info, end);
	if ((NULL != ptr_s) || (NULL != ptr_e)) {
		if (NULL == ptr_s) {
			LCM_DRIVER_FATAL("invalid para, missing %s!\n", start);
			processed_len +=  strlen(end);
			ret = -EINVAL;
			goto error;
		} else {
			processed_len +=  strlen(start);
		}

		if (NULL == ptr_e) {
			LCM_DRIVER_FATAL("invalid para, missing %s!\n", end);
			processed_len +=  strlen(start);
			ret = -EINVAL;
			goto error;
		} else {
			processed_len +=  strlen(end);
		}

		len = ptr_e - ptr_s - strlen(start);
		if (((len <= 0) || (len > (sum - processed_len)))) {
			LCM_DRIVER_FATAL("invalid data len:0x%X!\n", len);
			goto error;
		}
		*ptr = kzalloc(len+1, GFP_KERNEL);
		memcpy(*ptr,  ptr_s + strlen(start), len);
		processed_len += len;
		LCM_DRIVER_INFO("start:%s, end:%s: get valid data,len=0x%X.\n", start, end, len);
	}
	LCM_DRIVER_INFO("processed len:0x%X, left:0x%X.\n", processed_len, (sum - processed_len));

	return len;
error:

	return ret;
}

int lcm_params_info_init(void)
{
	static int lcm_params_info_init;
	int ret = 0, len = 0;

	LCM_DRIVER_INFO("enter\n");
	if (lcm_params_info_init) {
		/*Only the first init is need*/
		return 0;
	}
	lcm_params_info_init = 1;
	memset(&mipi_init, 0, sizeof(mipi_init));
	memset(&mipi_suspend, 0, sizeof(mipi_suspend));
	memset(&lvds_init, 0, sizeof(lvds_init));
	memset(&power_on_timing, 0, sizeof(power_on_timing));
	memset(&power_off_timing, 0, sizeof(power_off_timing));
	memset(&panel_info, 0, sizeof(panel_info));

	lcm_panel_info_init(&panel_info);

	if (INTERFACE_TYPE_MIPI == panel_info.interface_type) {
		len = get_specific_part_extended_panel_info(&mipi_init_table_text,
							    MIPI_INIT_S, MIPI_INIT_E);
		if (NULL != mipi_init_table_text) {
			ret = mipi_setting_info_init(&mipi_init, mipi_init_table_text, len);
			if (ret) {
				LCM_DRIVER_FATAL("mipi_init fail:%d\n", ret);
				goto error;
			} else {
				LCM_DRIVER_INFO("mipi_init success:cnt=%d\n", mipi_init.cnt);
			}
		} else {
			/*this is must  have valid params*/
			LCM_DRIVER_FATAL("mipi_init is no valid para!\n");
			ret = -EINVAL;
			goto error;
		}

		len = get_specific_part_extended_panel_info(&mipi_suspend_table_text,
							    MIPI_SUSPEND_S, MIPI_SUSPEND_E);
		if (NULL != mipi_suspend_table_text) {
			ret = mipi_setting_info_init(&mipi_suspend, mipi_suspend_table_text, len);
			if (ret) {
				LCM_DRIVER_FATAL("mipi_suspend fail:%d\n", ret);
				goto error;
			} else {
				LCM_DRIVER_INFO("mipi_suspend success:cnt=%d\n", mipi_suspend.cnt);
			}
		} else {
			/*this is  option have valid params*/
			LCM_DRIVER_INFO("mipi_suspend is no valid para!\n");
		}

		len = get_specific_part_extended_panel_info(&power_on_timing_text,
							    POWER_ON_SEQUENCE_S, POWER_ON_SEQUENCE_E);
		if (NULL != power_on_timing_text) {
			ret = lcm_timing_info_init(&power_on_timing, power_on_timing_text, len);
			if (ret) {
				LCM_DRIVER_FATAL("power_on_timing fail:%d\n", ret);
				goto error;
			} else {
				LCM_DRIVER_INFO("power_on_timing success:cnt=%d\n", power_on_timing.cnt);
			}
		} else {
			/*this is  option have valid params*/
			LCM_DRIVER_INFO("power_on_timing is no valid para!\n");
		}

		len = get_specific_part_extended_panel_info(&power_off_timing_text,
							    POWER_OFF_SEQUENCE_S, POWER_OFF_SEQUENCE_E);
		if (NULL != power_off_timing_text) {
			ret = lcm_timing_info_init(&power_off_timing, power_off_timing_text, len);
			if (ret) {
				LCM_DRIVER_FATAL("power_off_timing fail:%d\n", ret);
				goto error;
			} else {
				LCM_DRIVER_INFO("power_off_timing success:cnt=%d\n", power_off_timing.cnt);
			}
		} else {
			/*this is  option have valid params*/
			LCM_DRIVER_INFO("power_off_timing is no valid para!\n");
		}
	} else {
		if ((1 == g_secondary_panel_enable)
			&& (secondary_width && (UINT32_INVALD_FLAG != secondary_width))
			&& (secondary_height && (UINT32_INVALD_FLAG != secondary_height))) {
			len = get_specific_part_extended_panel_info(&lvds_init_table_text,
				LVDS_INIT_SECONDARY_PANEL_ENABLE_S, LVDS_INIT_SECONDARY_PANEL_ENABLE_E);
		} else {
			len = get_specific_part_extended_panel_info(&lvds_init_table_text, LVDS_INIT_S, LVDS_INIT_E);
		}

		if (NULL != lvds_init_table_text) {
			ret = lvds_setting_info_init(&lvds_init, lvds_init_table_text, len);
			if (ret) {
				LCM_DRIVER_FATAL("lvds_init fail:%d\n", ret);
				goto error;
			} else {
				LCM_DRIVER_INFO("lvds_init success:cnt=%d\n", lvds_init.cnt);
			}
		} else {
			/*this is must  have valid params*/
			LCM_DRIVER_FATAL("lvds_init is no valid para!\n");
			ret = -EINVAL;
			goto error;
		}
	}

	LCM_DRIVER_INFO("leave\n");

	return 0;
error:
	LCM_DRIVER_FATAL("fail, ret=%d.\n", ret);

	return ret;
}

void dump_panel_info(void)
{
	int i = 0;

	LCM_DRIVER_PRINTF("dump struct panel_info:\n"\
					  "size:%d\n"\
					  "pixel_width:%d\n"\
					  "pixel_height:%d\n"\
					  "phy_width:%d\n"\
					  "phy_height:%d\n"\
					  "clock:%d\n"\
					  "interface_type:%d\n"\
					  "bit_format:%d\n"\
					  "htotal:%d\n"\
					  "vtotal:%d\n"\
					  "h_sync_width:%d\n"\
					  "h_front_porch:%d\n"\
					  "h_back_porch:%d\n"\
					  "v_sync_width:%d\n"\
					  "v_front_porch:%d\n"\
					  "v_back_porch:%d\n"\
					  "ssc_disable:%d\n"\
					  "ssc_range:%d\n"\
					  "lvds_mode:%d\n"\
					  "lvds_link_mode:%d\n"\
					  "lvds_data_format:%d\n"\
					  "lvds_del_pol:%d\n"\
					  "lvds_clk_pol:%d\n"\
					  "lvds_vsync_pol:%d\n"\
					  "lvds_hsync_pol:%d\n"\
					  "mipi_mode:%d\n"\
					  "mipi_lane_num:%d\n"\
					  "mipi_esd_check_enable:%d\n"\
					  "mipi_esd_check_table:\n",
					  panel_info.size,
					  panel_info.pixel_width,
					  panel_info.pixel_height,
					  panel_info.phy_width,
					  panel_info.phy_height,
					  panel_info.clock,
					  panel_info.interface_type,
					  panel_info.bit_format,
					  panel_info.htotal,
					  panel_info.vtotal,
					  panel_info.h_sync_width,
					  panel_info.h_front_porch,
					  panel_info.h_back_porch,
					  panel_info.v_sync_width,
					  panel_info.v_front_porch,
					  panel_info.v_back_porch,
					  panel_info.ssc_disable,
					  panel_info.ssc_range,
					  panel_info.lvds_mode,
					  panel_info.lvds_link_mode,
					  panel_info.lvds_data_format,
					  panel_info.lvds_del_pol,
					  panel_info.lvds_clk_pol,
					  panel_info.lvds_vsync_pol,
					  panel_info.lvds_hsync_pol,
					  panel_info.mipi_mode,
					  panel_info.mipi_lane_num,
					  panel_info.mipi_esd_check_enable);
	for (i = 0; i < MAX_ESD_CHECK_TABLE_LEN; i++) {
		LCM_DRIVER_PRINTF("0x%02x, \n", panel_info.mipi_esd_check_table[i]);
	}
	LCM_DRIVER_PRINTF("\n");
}

void dump_extended_panel_info_text(void)
{
	int i = 0, sum_len = 0, tmp_len = 0;
	char buf[DUMP_DATA_LEN+1] = {0};

	/*LCM_DRIVER_PRINTF("dump raw data that extended panel info text(len=%d):\n%s\n",*/
	/*strlen(extended_panel_info), extended_panel_info);*/
	LCM_DRIVER_PRINTF("dump processed data that extended panel info text:\n");
	/*mipi*/
	LCM_DRIVER_PRINTF("[Mipi_Init_S]\n");
	if (NULL != mipi_init_table_text) {
		sum_len = strlen(mipi_init_table_text);

		for (i = 0; i <= sum_len/DUMP_DATA_LEN; i++) {
			if (i == sum_len/DUMP_DATA_LEN) {
				tmp_len = sum_len%DUMP_DATA_LEN;
			} else {
				tmp_len = DUMP_DATA_LEN;
			}
			if (!tmp_len) {
				break;
			}
			memset(buf, 0, sizeof(buf));
			memcpy(buf, mipi_init_table_text+i*DUMP_DATA_LEN, tmp_len);
			LCM_DRIVER_PRINTF("[%d]:%s\n", i+1, buf);
		}
	}
	LCM_DRIVER_PRINTF("[Mipi_Init_E]\n");

	LCM_DRIVER_PRINTF("[Mipi_Suspend_S]\n");
	if (NULL != mipi_suspend_table_text) {
		LCM_DRIVER_PRINTF("%s\n", mipi_suspend_table_text);
	}
	LCM_DRIVER_PRINTF("[Mipi_Suspend_E]\n");

	LCM_DRIVER_PRINTF("[Power_On_Sequence_S]\n");
	if (NULL != power_on_timing_text) {
		LCM_DRIVER_PRINTF("%s\n", power_on_timing_text);
	}
	LCM_DRIVER_PRINTF("[Power_On_Sequence_E]\n");

	LCM_DRIVER_PRINTF("[Power_Off_Sequence_S]\n");
	if (NULL != power_off_timing_text) {
		LCM_DRIVER_PRINTF("%s\n", power_off_timing_text);
	}
	LCM_DRIVER_PRINTF("[Power_Off_Sequence_E]\n");

	/*lvds*/
	LCM_DRIVER_PRINTF("[Lvds_Init_S]\n");
	if (NULL != lvds_init_table_text) {
		sum_len = strlen(lvds_init_table_text);
		for (i = 0; i <= sum_len/DUMP_DATA_LEN; i++) {
			if (i == sum_len/DUMP_DATA_LEN) {
				tmp_len = sum_len%DUMP_DATA_LEN;
			} else {
				tmp_len = DUMP_DATA_LEN;
			}
			if (!tmp_len) {
				break;
			}
			memset(buf, 0, sizeof(buf));
			memcpy(buf, lvds_init_table_text+i*DUMP_DATA_LEN, tmp_len);
			LCM_DRIVER_PRINTF("[%d]:%s\n", i+1, buf);
		}
	}
	LCM_DRIVER_PRINTF("[Lvds_Init_E]\n");
}

void dump_mipi_init(void)
{
	unsigned int i = 0, j = 0, cnt = 0;
	struct mipi_setting_table_list *tmp_ptr = NULL;

	tmp_ptr = mipi_init.head_list.next;
	LCM_DRIVER_PRINTF("dump mipi_init:\n");
	LCM_DRIVER_PRINTF("{\n");
	for (i = 0; NULL != tmp_ptr; tmp_ptr = tmp_ptr->next, i++) {
	LCM_DRIVER_PRINTF("{0x%02x, 0x%02x, {\n", tmp_ptr->setting_table.cmd,
						  tmp_ptr->setting_table.cnt);
		cnt = tmp_ptr->setting_table.cnt;
		if ((MIPI_REGFLAG_MDELAY == tmp_ptr->setting_table.cmd) ||
		    (MIPI_REGFLAG_UDELAY == tmp_ptr->setting_table.cmd) ||
		    (MIPI_REGFLAG_END_OF_TABLE == tmp_ptr->setting_table.cmd)) {
			cnt = 0;
		}
		j = 0;
		while (j < cnt) {
			LCM_DRIVER_PRINTF("0x%02x, \n", tmp_ptr->setting_table.para_list[j++]);
		}
		LCM_DRIVER_PRINTF("}},\n");
	}
	LCM_DRIVER_PRINTF("}\n");

	if (mipi_init.cnt != i) {
		LCM_DRIVER_FATAL("mipi_init.cnt(%d) != nodeNum (%d)!\n", mipi_init.cnt, i);
	}
}

void dump_mipi_suspend(void)
{
	unsigned int i = 0, j = 0, cnt = 0;
	struct mipi_setting_table_list *tmp_ptr = NULL;

	tmp_ptr = mipi_suspend.head_list.next;
	LCM_DRIVER_PRINTF("dump mipi_suspend:\n");
	LCM_DRIVER_PRINTF("{\n");
	for (i = 0; NULL != tmp_ptr; tmp_ptr = tmp_ptr->next, i++) {
		LCM_DRIVER_PRINTF("{0x%02x, 0x%02x, {\n", tmp_ptr->setting_table.cmd,
							  tmp_ptr->setting_table.cnt);
		cnt = tmp_ptr->setting_table.cnt;
		if ((MIPI_REGFLAG_MDELAY == tmp_ptr->setting_table.cmd) ||
		    (MIPI_REGFLAG_UDELAY == tmp_ptr->setting_table.cmd) ||
		    (MIPI_REGFLAG_END_OF_TABLE == tmp_ptr->setting_table.cmd)) {
			cnt = 0;
		}
		j = 0;
		while (j < cnt) {
			LCM_DRIVER_PRINTF("0x%02x, \n", tmp_ptr->setting_table.para_list[j++]);
		}
		LCM_DRIVER_PRINTF("}},\n");
	}
	LCM_DRIVER_PRINTF("}\n");

	if (mipi_suspend.cnt != i) {
		LCM_DRIVER_FATAL("mipi_suspend.cnt(%d) != nodeNum (%d)!\n", mipi_suspend.cnt, i);
	}
}

void dump_lvds_init(void)
{
	unsigned int i = 0;
	struct lvds_setting_table_list *tmp_ptr = NULL;

	tmp_ptr = lvds_init.head_list.next;
	LCM_DRIVER_PRINTF("dump lvds_init:\n");
	LCM_DRIVER_PRINTF("{\n");
	for (i = 0; NULL != tmp_ptr; tmp_ptr = tmp_ptr->next, i++) {
		LCM_DRIVER_PRINTF("{0x%02x, 0x%02x},\n", tmp_ptr->setting_table.cmd,
							 tmp_ptr->setting_table.data);
	}
	LCM_DRIVER_PRINTF("}\n");

	if (lvds_init.cnt != i) {
		LCM_DRIVER_FATAL("lvds_init.cnt(%d) != nodeNum (%d)!\n", lvds_init.cnt, i);
	}
}

void dump_power_on_timing(void)
{
	unsigned int i = 0;
	struct gpio_ctl_cmd_list *tmp_ptr = NULL;

	tmp_ptr = power_on_timing.head_list.next;
	LCM_DRIVER_PRINTF("dump power_on_timing:\n");
	LCM_DRIVER_PRINTF("{\n");
	for (i = 0; NULL != tmp_ptr; tmp_ptr = tmp_ptr->next, i++) {
		LCM_DRIVER_PRINTF("{0x%02x, 0x%02x},\n", tmp_ptr->cmd.level,
							 tmp_ptr->cmd.duration);
	}
	LCM_DRIVER_PRINTF("}\n");

	if (power_on_timing.cnt != i) {
		LCM_DRIVER_FATAL("power_on_timing.cnt(%d) != nodeNum (%d)!\n", power_on_timing.cnt, i);
	}
}

void dump_power_off_timing(void)
{
	unsigned int i = 0;
	struct gpio_ctl_cmd_list *tmp_ptr = NULL;

	tmp_ptr = power_off_timing.head_list.next;
	LCM_DRIVER_PRINTF("dump power_off_timing:\n");
	LCM_DRIVER_PRINTF("{\n");
	for (i = 0; NULL != tmp_ptr; tmp_ptr = tmp_ptr->next, i++) {
		LCM_DRIVER_PRINTF("{0x%02x, 0x%02x},\n", tmp_ptr->cmd.level,
							 tmp_ptr->cmd.duration);
	}
	LCM_DRIVER_PRINTF("}\n");

	if (power_off_timing.cnt != i) {
		LCM_DRIVER_FATAL("power_off_timing.cnt(%d) != nodeNum (%d)!\n", power_off_timing.cnt, i);
	}
}

void lcm_params_info_dump(void)
{
	LCM_DRIVER_INFO("enter\n");

	dump_panel_info();
	dump_extended_panel_info_text();
	dump_mipi_init();
	dump_mipi_suspend();
	dump_power_on_timing();
	dump_power_off_timing();
	dump_lvds_init();

	LCM_DRIVER_INFO("leave\n");
}
/*EXPORT_SYMBOL(lcm_params_info_dump);*/

/*this func is used for customizing LCM on power on sequence*/
static void lcm_valid_data_start_early(void)
{
	LCM_DRIVER_INFO("enter\n");
	if (jac_lcm_enabled) {
		/* the panel is powered and initialized by jac_lcm_init() */
		LCM_DRIVER_INFO("leave\n");

		return;
	}
  	if (GAOWIN_BRIDGE_IC != lcm_bridge_ic_detect(LCM_ID_PRIMARY)) {
 		if ((INTERFACE_TYPE_LVDS == panel_info.interface_type) && (lvds_init.cnt)) {
 			/*init brige IC*/
			lcm_gpio_output(LVDS_BRIDGE_IC_RESET_PIN, 0);
			MDELAY(20);
			lcm_gpio_output(LVDS_BRIDGE_IC_RESET_PIN, 1);
			MDELAY(20);
			lvds_push_table(LCM_ID_PRIMARY, &lvds_init);
			MDELAY(5);
			/* LCM power on before LCM input valid data begin, ask for LCM (BP101wx1-206) spec 9.0 T2*/
			lcm_gpio_output(LCM_POWER_EN_PIN, 1);
		}
	}
	LCM_DRIVER_INFO("leave\n");
}

/*this func is used for customizing LCM on power on sequence*/
static void lcm_valid_data_start_late(void)
{
#ifdef BRIDGE_IC_IRQ_ENABLE
	unsigned char data = 0;
#endif
	LCM_DRIVER_INFO("enter\n");

	if (GAOWIN_BRIDGE_IC == lcm_bridge_ic_detect(LCM_ID_PRIMARY)) {
		if ((INTERFACE_TYPE_LVDS == panel_info.interface_type) && (lvds_init.cnt)) {
		/*init brige IC*/

			lcm_gpio_output(LVDS_BRIDGE_IC_RESET_PIN, 0);
			MDELAY(20);
			lcm_gpio_output(LVDS_BRIDGE_IC_RESET_PIN, 1);
			MDELAY(20);

			lvds_push_table(LCM_ID_PRIMARY, &lvds_init);
			if (BRIDGE_IC_DETECT_SUCCESS == lcm_bridge_ic_detect_ds90ub947(LCM_ID_SECONDARY)) {
				lvds_push_table(LCM_ID_SECONDARY, &lvds_init);
				MDELAY(5);
			}

			/* LCM power on before LCM input valid data begin, ask for LCM (BP101wx1-206) spec 9.0 T2*/
			lcm_gpio_output(LCM_POWER_EN_PIN, 1);
		}
	}

#ifdef BRIDGE_IC_IRQ_ENABLE
	if ((!jac_lcm_enabled) && (INTERFACE_TYPE_LVDS == panel_info.interface_type) && (lvds_init.cnt)) {
		interface_write_byte(LCM_ID_PRIMARY, LVDS_REGFLAG_BRIGE_IC_ADDR, ERROR_STATUS_REG, 0xff);
		data = 0;
		interface_read_byte(LCM_ID_PRIMARY, LVDS_REGFLAG_BRIGE_IC_ADDR, ERROR_STATUS_REG, &data);
		LCM_DRIVER_INFO("after addr:%x, value:%x\n", ERROR_STATUS_REG, data);
		interface_write_byte(LCM_ID_PRIMARY, LVDS_REGFLAG_BRIGE_IC_ADDR, IRQ_EN_REG, 0x01);
		interface_write_byte(LCM_ID_PRIMARY, LVDS_REGFLAG_BRIGE_IC_ADDR, IRQ_EN_MASK_REG, 0xff);
		if (g_lcm_irq) {
			enable_irq(g_lcm_irq);
		}
	}
#endif
	LCM_DRIVER_INFO("leave\n");
}

/*this func is used for customizing LCM on power off sequence*/
static void lcm_valid_data_stop_early(void)
{
	LCM_DRIVER_INFO("enter\n");

#ifdef BRIDGE_IC_IRQ_ENABLE
	if (g_lcm_irq) {
		disable_irq(g_lcm_irq);
	}
#endif
	if ((INTERFACE_TYPE_LVDS == panel_info.interface_type) && (lvds_init.cnt)) {
		/* 200ms delay from backlight off to LCM valid data off, ask for LCM (BP101wx1-206) spec 9.0 T6*/
		MDELAY(200);
	}
	LCM_DRIVER_INFO("leave\n");
}

/*this func is used for customizing LCM on power off sequence*/
static void lcm_valid_data_stop_late(void)
{
	LCM_DRIVER_INFO("enter\n");

	if ((INTERFACE_TYPE_LVDS == panel_info.interface_type) && (lvds_init.cnt)) {
		lcm_gpio_output(LVDS_BRIDGE_IC_RESET_PIN, 0);
		MDELAY(5);
		/* LCM power off after LCM input valid data off, ask for LCM (BP101wx1-206) spec 9.0 T2*/
		if (!jac_lcm_enabled) {
			/* jac_set_lcd_power_kthread() powers the panel off */
			lcm_gpio_output(LCM_POWER_EN_PIN, 0);
		}
	}
	LCM_DRIVER_INFO("leave\n");
}

/*this is used for customizing LCM backlight*/
static void lcm_set_backlight(struct LCM_PARAMS *params)
{
	extern unsigned int g_backlight_mdelay;
	extern unsigned int g_backlight_remap;
	if (NULL == params)  {
		LCM_DRIVER_ERROR("invalid parameters\n");
		return;
	}
#ifdef LVDS_BRIGE_IC_DS90UB941_DS90UB948_ENABLE
	if (DS90UB941_BRIDGE_IC == lcm_bridge_ic_detect(LCM_ID_PRIMARY)) {
		if ((1024 == params->primary_width) && (600 == params->primary_height)) {
			/*remap for multidisplay 1024*600 panel*/
			g_backlight_remap = g_backlight_remap | 0x1;
			return;
		}
	}
#endif
	if ((INTERFACE_TYPE_LVDS == panel_info.interface_type) && (lvds_init.cnt)) {
		/* 200ms delay from LCM valid data begin to backlight on, ask for LCM (BP101wx1-206) spec 9.0 T5*/
		g_backlight_mdelay = 200;
	}
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_matezone(void)
{
	unsigned int ret = 0;
	unsigned int value = 0;

	ret = MetaZone_Read(METAZONE_OPEN_EXT_DISP_INDEX, &value);
	if (MZ_SUCCESS != ret) {
		LCM_DRIVER_FATAL("metazone read index failed:%d!\n", ret);
	}

	if ((MZ_SUCCESS == ret) && (value == 1)) {
		g_secondary_panel_enable = 1;
	} else {
		/*default secondary panel is disable*/
		g_secondary_panel_enable = 0;
	}
	LCM_DRIVER_INFO("MetaZone_Read value:%x, g_secondary_panel_enable=%d\n",
		value, g_secondary_panel_enable);
}

#ifdef LVDS_BRIGE_IC_GAOWIN_DRAM_ENABLE
static int get_split_type(unsigned int primary_width, unsigned int primary_height,
			unsigned int secondary_width, unsigned int secondary_height)
{
	int width_h = 0;
	int height_h = 0;
	int width_v = 0;
	int height_v = 0;

	width_h = primary_width + secondary_width;
	height_h = (primary_height > secondary_height) ?
			primary_height : secondary_height;

	width_v = (primary_width > secondary_width) ?
			primary_width : secondary_width;
	height_v = primary_height + secondary_height;

	if ( (width_v * height_v) < (width_h * height_h) ) {
		return VERTICAL_SPLIT; //vertical split
	}

	return HORIZONTAL_SPLIT; //horizontal split
}
#endif


static void lcm_get_params(struct LCM_PARAMS *params)
{
	int ret = 0;
	int i = 0;
	 struct lvds_setting_table_list *tmp_list = NULL;

	LCM_DRIVER_INFO("enter\n");
	if(NULL == params) {
		LCM_DRIVER_FATAL("invalid parameters!\n");

		return;
	}
	memset(params, 0, sizeof(*params));
#if defined(K61V1_64_BSP)
	/* Default LCM params for interface of MIPI(MIPI LCM:"NT35521_HD_DSI_VDO_TRULY_RT5081)*/
	params->type = LCM_TYPE_DSI;
	params->width = 720;
	params->height = 1280;
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 6;
	params->dsi.vertical_frontporch = 85;
	params->dsi.vertical_active_line = 1280;
	params->dsi.horizontal_sync_active = 50;
	params->dsi.horizontal_backporch = 50;
	params->dsi.horizontal_frontporch = 50;
	params->dsi.horizontal_active_pixel = 720;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.ssc_disable = 1;
	params->dsi.PLL_CLOCK = 224;
	params->dsi.esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
#else
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
#endif
	lcm_get_matezone();
	ret = lcm_params_info_init();
	if (ret) {
		LCM_DRIVER_FATAL("lcm_params_info_init fail!\n");
		lcm_params_info_dump();
		goto exit;
	}

	if (!crc_check_flag) {
		goto exit;
	}

	tmp_list = lvds_init.head_list.next;
	if ( (NULL != tmp_list) && (LVDS_REGFLAG_ADDR_SELECT == tmp_list->setting_table.cmd) ) {
		lvds_bridge_ic_addr = tmp_list->setting_table.data;
	}
	LCM_DRIVER_INFO("lvds_bridge_ic_addr = 0x%x\n", lvds_bridge_ic_addr);

	if (UINT32_INVALD_FLAG != panel_info.pixel_width) {
		params->primary_width = panel_info.pixel_width;
	} else {
		LCM_DRIVER_INFO("primary_width: invalid value 0x%x!\n", panel_info.pixel_width);
	}

	if (UINT32_INVALD_FLAG != panel_info.pixel_height) {
		params->primary_height = panel_info.pixel_height;
	} else {
		LCM_DRIVER_INFO("primary_height: invalid value 0x%x!\n", panel_info.pixel_height);
	}

	if (secondary_width && (UINT32_INVALD_FLAG != secondary_width)) {
		params->secondary_width = secondary_width;
	} else {
		LCM_DRIVER_INFO("secondary_width: invalid value %d!\n", secondary_width);
	}

	if (secondary_height && (UINT32_INVALD_FLAG != secondary_height)) {
		params->secondary_height = secondary_height;
	} else {
		LCM_DRIVER_INFO("secondary_height: invalid value %d!\n", secondary_height);
	}

	if ( (1 == g_secondary_panel_enable) && (params->secondary_width) && (params->secondary_height)) {
		if ( (GAOWIN_BRIDGE_IC == lcm_bridge_ic_detect(LCM_ID_PRIMARY)) ) {
#ifdef LVDS_BRIGE_IC_GAOWIN_DRAM_ENABLE
			ret = get_split_type(params->primary_width, params->primary_height,
						params->secondary_width, params->secondary_height);
			if (VERTICAL_SPLIT == ret) {
				params->width = (params->primary_width > params->secondary_width) ?
						params->primary_width : params->secondary_width;

				params->height = params->primary_height + params->secondary_height;
				params->split_type = VERTICAL_SPLIT;
			}
			else {
				params->width = params->primary_width + params->secondary_width;

				params->height = (params->primary_height > params->secondary_height) ?
						params->primary_height : params->secondary_height;
				params->split_type = HORIZONTAL_SPLIT;
			}
#else
			params->width = params->primary_width + params->secondary_width;

			params->height = (params->primary_height > params->secondary_height) ?
					params->primary_height : params->secondary_height;
			params->split_type = HORIZONTAL_SPLIT;
#endif
		} else {
			params->width = ((params->primary_width > params->secondary_width) ?
					params->primary_width : params->secondary_width) * 2;
			params->height = (params->primary_height > params->secondary_height) ?
					params->primary_height : params->secondary_height;
			params->split_type = HORIZONTAL_SPLIT;
		}
	} else {
		params->width = params->primary_width;
		params->height = params->primary_height;
	}

	if (UINT32_INVALD_FLAG != panel_info.clock) {
		if ((0 == g_secondary_panel_enable) && (params->secondary_width)) {
			params->dsi.PLL_CLOCK = panel_info.clock / 2;
		} else {
			params->dsi.PLL_CLOCK = panel_info.clock;
		}
	} else {
		LCM_DRIVER_INFO("dsi.PLL_CLOCK: not set value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.mipi_mode) {
		if (MIPI_MODE_CMD == panel_info.mipi_mode) {
			params->dsi.mode = CMD_MODE;
		} else if (MIPI_MODE_ESYNC_PULSE_VDO == panel_info.mipi_mode) {
			params->dsi.mode = SYNC_PULSE_VDO_MODE;
		} else if (MIPI_MODE_SYNC_EVENT_VDO == panel_info.mipi_mode) {
			params->dsi.mode = SYNC_EVENT_VDO_MODE;
		} else if (MIPI_MODE_BURST_VDO_MODE == panel_info.mipi_mode) {
			params->dsi.mode = BURST_VDO_MODE;
		} else {
			LCM_DRIVER_INFO("dsi.mode:invalid value[%d], use default!\n", panel_info.mipi_mode);
		}
	} else {
		LCM_DRIVER_INFO("dsi.mode: not set value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.bit_format) {
		if (BIT_FORMAT_6 == panel_info.bit_format) {
			params->dsi.data_format.format = LCM_DSI_FORMAT_RGB666;
			params->dsi.PS = LCM_PACKED_PS_18BIT_RGB666;
		} else if (BIT_FORMAT_8 == panel_info.bit_format) {
			params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
			params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
		} else {
			LCM_DRIVER_INFO("dsi.data_format.format:invalid value[%d], use default!\n",
					panel_info.bit_format);
		}
	} else {
		LCM_DRIVER_INFO("dsi.data_format.format: not set value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.ssc_disable) {
		params->dsi.ssc_disable = panel_info.ssc_disable;
		if (!params->dsi.ssc_disable) {
			if (panel_info.ssc_range && (panel_info.ssc_range <= 8)) {
				params->dsi.ssc_range = panel_info.ssc_range;
			} else {
				LCM_DRIVER_INFO("dsi.ssc_range:invalid value[%d], use default!\n",
						panel_info.ssc_range);
			}
		}
	} else {
		LCM_DRIVER_INFO("dsi.ssc_disable: not set value, use default!\n");
	}

	if ((UINT16_INVALD_FLAG != panel_info.mipi_esd_check_enable) &&
	    ((0 == panel_info.mipi_esd_check_enable) || (1 == panel_info.mipi_esd_check_enable))) {
		params->dsi.esd_check_enable = panel_info.mipi_esd_check_enable;
		if (panel_info.mipi_esd_check_enable) {
			if ((UINT8_INVALD_FLAG != panel_info.mipi_esd_check_table[1]) &&
			    panel_info.mipi_esd_check_table[1] &&
			    (panel_info.mipi_esd_check_table[1] <= MAX_ESD_CHECK_TABLE_LEN)) {
				/*byte0:reg addr, byte1:cmd cnt, byte2 ~ byte2+cnt-1: para list*/
				params->dsi.lcm_esd_check_table[0].cmd = panel_info.mipi_esd_check_table[0];
				params->dsi.lcm_esd_check_table[0].count = panel_info.mipi_esd_check_table[1];

				memcpy(params->dsi.lcm_esd_check_table[0].para_list,
					   &panel_info.mipi_esd_check_table[2],
					   panel_info.mipi_esd_check_table[1]);
				params->dsi.esd_check_enable = 1;
			} else {
				LCM_DRIVER_INFO("mipi_esd_check_table[1]: not set value "\
						"or invalid value, use default!\n");
			}
		}
	} else {
		LCM_DRIVER_INFO("dsi.esd_check_enable: not set value or invalid value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.mipi_lane_num) {
		params->dsi.LANE_NUM = panel_info.mipi_lane_num;
	} else {
		LCM_DRIVER_INFO("dsi.LANE_NUM: not set value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.v_sync_width) {
		params->dsi.vertical_sync_active = panel_info.v_sync_width;
	} else {
		LCM_DRIVER_INFO("dsi.vertical_sync_active: not set value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.v_back_porch) {
		params->dsi.vertical_backporch = panel_info.v_back_porch;
	} else {
		LCM_DRIVER_INFO("dsi.vertical_backporch: not set value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.v_front_porch) {
		params->dsi.vertical_frontporch = panel_info.v_front_porch;
	} else {
		LCM_DRIVER_INFO("dsi.vertical_frontporch: not set value, use default!\n");
	}

	if (UINT32_INVALD_FLAG != params->height) {
		params->dsi.vertical_active_line = params->height;
	} else {
		LCM_DRIVER_INFO("dsi.vertical_active_line: not set value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.h_sync_width) {
		params->dsi.horizontal_sync_active = panel_info.h_sync_width;
	} else {
		LCM_DRIVER_INFO("dsi.horizontal_sync_active: not set value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.h_back_porch) {
		params->dsi.horizontal_backporch = panel_info.h_back_porch;
	} else {
		LCM_DRIVER_INFO("dsi.horizontal_backporch: not set value, use default!\n");
	}

	if (UINT16_INVALD_FLAG != panel_info.h_front_porch) {
		params->dsi.horizontal_frontporch = panel_info.h_front_porch;
	} else {
		LCM_DRIVER_INFO("dsi.horizontal_frontporch: not set value, use default!\n");
	}

	if (UINT32_INVALD_FLAG != params->width) {
		params->dsi.horizontal_active_pixel = params->width;
	} else {
		LCM_DRIVER_INFO("dsi.horizontal_active_pixel: not set value, use default!\n");
	}

	if ( (UINT32_INVALD_FLAG != panel_info.phy_width) && (UINT32_INVALD_FLAG != panel_info.phy_height) ) {
		params->physical_width = panel_info.phy_width;
		params->physical_height = panel_info.phy_height;
	} else {
		LCM_DRIVER_INFO("panel phy_width, panel phy_heigh: not set value, use default!\n");
	}

	jac_lcm_get_mipi_params_form_MetaZone(params);

exit:

	install_lcm_valid_data_start_early_func(lcm_valid_data_start_early);
	install_lcm_valid_data_start_late_func(lcm_valid_data_start_late);
	install_lcm_valid_data_stop_early_func(lcm_valid_data_stop_early);
	install_lcm_valid_data_stop_late_func(lcm_valid_data_stop_late);

	lcm_set_backlight(params);
#ifdef BRIDGE_IC_IRQ_ENABLE
	lcm_init_eint();
#endif
	LCM_DRIVER_INFO("**********dump lcm params**********\n");
	LCM_DRIVER_INFO("params->physical_width:%d, params->physical_height:%d\n",
			params->physical_width, params->physical_height);
	LCM_DRIVER_INFO("type:%d\n", params->type);
	LCM_DRIVER_INFO("width:%d\n", params->width);
	LCM_DRIVER_INFO("height:%d\n", params->height);
	LCM_DRIVER_INFO("primary_width:%d\n", params->primary_width);
	LCM_DRIVER_INFO("primary_height:%d\n", params->primary_height);
	LCM_DRIVER_INFO("secondary_width:%d\n", params->secondary_width);
	LCM_DRIVER_INFO("secondary_height:%d\n", params->secondary_height);
	LCM_DRIVER_INFO("mode:%d\n", params->dsi.mode);
	LCM_DRIVER_INFO("LANE_NUM:%d\n", params->dsi.LANE_NUM);
	LCM_DRIVER_INFO("format:%d\n", params->dsi.data_format.format);
	LCM_DRIVER_INFO("ssc_disable:%d\n", params->dsi.ssc_disable);
	LCM_DRIVER_INFO("ssc_range:%d\n", params->dsi.ssc_range);
	LCM_DRIVER_INFO("vertical_sync_active:%d\n", params->dsi.vertical_sync_active);
	LCM_DRIVER_INFO("vertical_backporch:%d\n", params->dsi.vertical_backporch);
	LCM_DRIVER_INFO("vertical_frontporch:%d\n", params->dsi.vertical_frontporch);
	LCM_DRIVER_INFO("vertical_active_line:%d\n", params->dsi.vertical_active_line);
	LCM_DRIVER_INFO("horizontal_sync_active:%d\n", params->dsi.horizontal_sync_active);
	LCM_DRIVER_INFO("horizontal_backporch:%d\n", params->dsi.horizontal_backporch);
	LCM_DRIVER_INFO("horizontal_frontporch:%d\n", params->dsi.horizontal_frontporch);
	LCM_DRIVER_INFO("horizontal_active_pixel:%d\n", params->dsi.horizontal_active_pixel);
	LCM_DRIVER_INFO("PS:%d\n", params->dsi.PS);
	LCM_DRIVER_INFO("PLL_CLOCK:%d\n", params->dsi.PLL_CLOCK);
	LCM_DRIVER_INFO("esd_check_enable:%d\n", params->dsi.esd_check_enable);
	LCM_DRIVER_INFO("lcm_esd_check_table.cmd:0x%02x\n", params->dsi.lcm_esd_check_table[0].cmd);
	LCM_DRIVER_INFO("lcm_esd_check_table.count:%d\n", params->dsi.lcm_esd_check_table[0].count);
	LCM_DRIVER_INFO("lcm_esd_check_table.para_list:\n");
	for (i = 0; i < params->dsi.lcm_esd_check_table[0].count; i++) {
		LCM_DRIVER_INFO("0x%02x\n", params->dsi.lcm_esd_check_table[0].para_list[i]);
	}

	LCM_DRIVER_INFO("leave\n");
	return ;
}

static void lcm_init(void)
{
	LCM_DRIVER_INFO("enter\n");
	if (g_mobilelog) {
		lcm_params_info_dump();
	}

	if (jac_lcm_enabled) {
		jac_lcm_init();
		LCM_DRIVER_INFO("leave\n");

		return;
	}

	if (INTERFACE_TYPE_MIPI == panel_info.interface_type) {
		if (power_on_timing.cnt) {
			lcm_timing_ctl(&power_on_timing);
		} else {
			LCM_DRIVER_DEBUG("power_on_timing.cnt:0, use default timing.\n");
			SET_RESET_PIN(0);
			MDELAY(15);
			SET_RESET_PIN(1);
			MDELAY(1);
			SET_RESET_PIN(0);
			MDELAY(10);
			SET_RESET_PIN(1);
			MDELAY(10);
		}

		if (mipi_init.cnt) {
			push_table(&mipi_init);
		} else {
			LCM_DRIVER_FATAL("mipi_init.cnt:0\n");
		}
	}
   LCM_DRIVER_INFO("leave\n");

   return;
}

/**
*calculate checksum: checksum = cmdid+data[x]+ ext_len + 1
*@param buf - the buffer to be calculated
*@param size - buffer size
*@param offset - start offset of buffer
*@param length - calculate  length
**/
static unsigned char lcm_calculate_checksum(char * buf, int size, int offset, int length)
{
    unsigned char result = 0;
    int position = 0;

    for (position = 0; position < length; position ++) {
        result += (unsigned char)*(buf + offset + position);
    }
    result = result + 1;

    LCM_DRIVER_INFO("checksum = 0x%x\n",result);
    return result;
}

void modules_lcm_backlight_ctrl(unsigned char i2c_id, int mode, int backlight_value)
{
        int i = 0;
        int ret = 0;
	/* map range below can be changed according to screen spec. */
	const int v_start = 10;	 /* value for darkest brightness in module screen (use bridge IC) */
	const int v_end   = 255; /* value for brightest brightness in module screen (use bridge IC) */
	int LIGHTS_MAX = 1023;
	int LIGHTS_MIN = 0;
	int v_map;

        unsigned char LCM_config[][7] = {
                {0x80, 0x01, 0, 0, 0, 0, 0},            /*set start up status*/
                {0x90, 0x01, 0xfe, 0xff, 0, 0, 0}       /*setScreenMode backlight value 65535*/
        };

	/*Detect bridge IC*/
	if (BRIDGE_IC_DETECT_SUCCESS != lcm_bridge_ic_detect_ds90ub941(i2c_id) &&
		BRIDGE_IC_DETECT_SUCCESS != lcm_bridge_ic_detect_ds90ub947(i2c_id)) {
		return;
	} else {
		v_map = ((backlight_value - LIGHTS_MIN) * (v_end - v_start)) /
			(LIGHTS_MAX - LIGHTS_MIN) + v_start;
		LCM_DRIVER_INFO("map backlight_value:%d to bridge ic value:%d\n",
				backlight_value, v_map);
	}

	LCM_config[1][1] = mode;
	LCM_config[1][2] = (unsigned char)(v_map >> 8)&0xff;
	LCM_config[1][3] = (unsigned char)v_map&0xff;
	LCM_config[0][6] = lcm_calculate_checksum((char *)&LCM_config[0][0],
						   sizeof(LCM_config[0]), 0, 6);
	LCM_config[1][6] = lcm_calculate_checksum((char *)&LCM_config[1][0],
						   sizeof(LCM_config[0]), 0, 6);

        for (i = 0; i < sizeof(LCM_config)/sizeof(LCM_config[0]); i++) {
                ret = interface_write_buffer(i2c_id, LCM_ADDR, (char *)&LCM_config[i][0],
						sizeof(LCM_config[0]));
                if (ret < 0) {
                        LCM_DRIVER_ERROR("I2C write fail(%d), addr:0x%x, i2c_id:%d\n", ret, LCM_ADDR, i2c_id);
                        return;
                } else {
                        LCM_DRIVER_INFO("I2C write success(%d)\n", ret);
                }
                if(0 == i)
                        MDELAY(100);
	}

#ifdef LVDS_BRIGE_IC_GAOWIN_ENABLE
	if (SCREEN_MODE_ON == mode) {
		/*These two GPIOs need to be turned on for the gowin FPGA board*/
		//lcm_gpio_output(GOWIN_BRIDGE_IC_POWER_EN_PIN0, 1);
		lcm_gpio_output(GOWIN_BRIDGE_IC_POWER_EN_PIN1, 1);
	} else if (SCREEN_MODE_START_POWER_OFF == mode) {
		lcm_gpio_output(GOWIN_BRIDGE_IC_POWER_EN_PIN1, 0);
	}
#endif
}

static void lcm_suspend(void)
{
	LCM_DRIVER_INFO("enter\n");

	if (INTERFACE_TYPE_MIPI == panel_info.interface_type) {
		if (mipi_suspend.cnt) {
			push_table(&mipi_suspend);
		} else {
			LCM_DRIVER_DEBUG("mipi_suspend.cnt:0\n");
		}

		if (power_off_timing.cnt) {
			lcm_timing_ctl(&power_off_timing);
		} else {
			LCM_DRIVER_DEBUG("power_off_timing.cnt:0\n");
			SET_RESET_PIN(0);
		}
	}

	if (jac_lcm_enabled) {
		lcm_gpio_output(JAC_LCD_RESET_PIN, 0);
		lcm_gpio_output(LVDS_BRIDGE_IC_RESET_PIN, 0);
		kthread_run(jac_set_lcd_power_kthread, NULL, "set_lcd_power_kthread");
	}

	LCM_DRIVER_INFO("leave\n");

	return;
}

static void lcm_resume(void)
{
	LCM_DRIVER_INFO("enter\n");
	lcm_init();
	LCM_DRIVER_INFO("leave\n");
}

static unsigned int lcm_compare_id(void)
{
	return 1;

}

void lcm_dump_reg_table(struct lvds_setting_info *setting_info)
{
	unsigned int i = 0;
	unsigned char cmd = 0, data = 0, out = 0;
	struct lvds_setting_table_list *tmp_list = NULL;
	LCM_DRIVER_INFO("enter\n");
	if (NULL == setting_info) {
		LCM_DRIVER_FATAL("invalid param!\n");
		return;
	}
	tmp_list = setting_info->head_list.next;
	for (; NULL != tmp_list; tmp_list = tmp_list->next) {
		cmd = tmp_list->setting_table.cmd;
		data = tmp_list->setting_table.data;
		switch (cmd) {
		case LVDS_REGFLAG_MDELAY:
			break;
		case LVDS_REGFLAG_END_OF_TABLE:
			break;
		default:
			out = 0;
			interface_read_byte(LCM_ID_PRIMARY, LVDS_REGFLAG_BRIGE_IC_ADDR, cmd, &out);
			if (out != data) {
				LCM_DRIVER_ERROR("dump write != read, addr:0x%02x , write:0x%02x, read:0x%02x!\n",
					cmd, data, out);
			}
		}
		i++;
	}
	if ((setting_info->cnt != i) || (LVDS_REGFLAG_END_OF_TABLE != cmd)) {
		LCM_DRIVER_ERROR("cmd send incomplete, cnt= %d, send = %d, end cmd=%X!\n", setting_info->cnt, i, cmd);
	}
	LCM_DRIVER_INFO("leave\n");
}

#ifdef LVDS_BRIGE_IC_DS90UB941_DS90UB948_ENABLE
void lcm_dump_ti941_ti948(unsigned char i2c_id)
{
	unsigned char i = 0, data = 0;
	#define TI948_DUMP_ENABLE	1
	#define TI941_DUMP_DSI0_ENABLE	1
	LCM_DRIVER_INFO("**********************TI941_TX Port0*****0x1E=0x01******************\n");
	interface_write_byte(i2c_id, lvds_bridge_ic_addr, 0x1E, 0x01);
	for (i = 0; i < 0x7F; i++) {
		interface_read_byte(i2c_id, lvds_bridge_ic_addr, i, &data);
		LCM_DRIVER_INFO("0x%02x:%02x\n", i, data);
	}
	LCM_DRIVER_INFO("**********************TI941_TX Port1*****0x1E=0x02******************\n");
	interface_write_byte(i2c_id, lvds_bridge_ic_addr, 0x1E, 0x02);
	for (i = 0; i < 0x7F; i++) {
		interface_read_byte(i2c_id, lvds_bridge_ic_addr, i, &data);
		LCM_DRIVER_INFO("0x%02x:%02x\n", i, data);
	}
#if TI948_DUMP_ENABLE
	LCM_DRIVER_INFO("********************TI941_TX Port0***TI948*****************************\n");
	interface_write_byte(i2c_id, lvds_bridge_ic_addr, 0x1E, 0x01);
	for (i = 0; i < 0xF0; i++) {
		interface_read_byte(i2c_id, LVDS_REGFLAG_TI948_ADDR, i, &data);
		LCM_DRIVER_INFO("0x%02x:%02x\n", i, data);
	}

	LCM_DRIVER_INFO("********************TI941_TX Port1***TI948*****************************\n");
	interface_write_byte(i2c_id, lvds_bridge_ic_addr, 0x1E, 0x04);
	for (i = 0; i < 0xF0; i++) {
		interface_read_byte(i2c_id, LVDS_REGFLAG_TI948_ADDR, i, &data);
		LCM_DRIVER_INFO("0x%02x:%02x\n", i, data);
	}
#endif

#if TI941_DUMP_DSI0_ENABLE
	LCM_DRIVER_INFO("***************************TI941_DSI0**********************************\n");
	interface_write_byte(i2c_id, lvds_bridge_ic_addr, 0x40, 0x05);
	for (i = 0; i < 0x3C; i++) {
		interface_write_byte(i2c_id, lvds_bridge_ic_addr, 0x41, i);
		data = 0;
		interface_read_byte(i2c_id, lvds_bridge_ic_addr, 0x42, &data);
		LCM_DRIVER_INFO("0x%02x:%02x\n", i, data);
	}
#endif
}
#endif

void lcm_dump_info(unsigned int index, unsigned char i2c_id)
{
#define  BRIGE_IC_SN65DSI83_INDEX		1
#define  BRIGE_IC_DS90UB941_DS90UB948_INDEX	2

#ifdef BRIDGE_IC_IRQ_ENABLE
	unsigned char data = 0;
#endif
	LCM_DRIVER_INFO("enter\n");
	if (BRIGE_IC_SN65DSI83_INDEX == index) {
		lcm_dump_reg_table(&lvds_init);
#ifdef BRIDGE_IC_IRQ_ENABLE
		interface_read_byte(i2c_id, LVDS_REGFLAG_BRIGE_IC_ADDR, ERROR_STATUS_REG, &data);
		LCM_DRIVER_INFO("before addr:%x, value:%x\n", ERROR_STATUS_REG, data);
		/*write 0xff to clear all bits error status*/
		interface_write_byte(i2c_id, LVDS_REGFLAG_BRIGE_IC_ADDR, ERROR_STATUS_REG, 0xff);
		MDELAY(10);
		data = 0;
		interface_read_byte(i2c_id, LVDS_REGFLAG_BRIGE_IC_ADDR, ERROR_STATUS_REG, &data);
		LCM_DRIVER_INFO("after addr:%x, value:%x\n", ERROR_STATUS_REG, data);
#endif
	} else if (BRIGE_IC_DS90UB941_DS90UB948_INDEX == index) {
#ifdef LVDS_BRIGE_IC_DS90UB941_DS90UB948_ENABLE
		lcm_dump_ti941_ti948(i2c_id);
#endif
	} else {
		LCM_DRIVER_ERROR("Invalid index\n");
	}
	LCM_DRIVER_INFO("leave\n");
}
void lcm_debug_suspend(void)
{
	LCM_DRIVER_INFO("enter\n");
	lcm_valid_data_stop_early();
	lcm_valid_data_stop_late();
	lcm_suspend();
	LCM_DRIVER_INFO("leave\n");
}
void lcm_debug_resume(void)
{
	LCM_DRIVER_INFO("enter\n");
	lcm_resume();
	lcm_valid_data_start_early();
	lcm_valid_data_start_late();
	LCM_DRIVER_INFO("leave\n");
}
void lcm_read_reg(unsigned char i2c_id, unsigned char slave_addr, unsigned char reg_addr)
{
	unsigned char data = 0;
	LCM_DRIVER_INFO("enter\n");
	interface_read_byte(i2c_id, slave_addr, reg_addr, &data);
	LCM_DRIVER_INFO("read slave_addr:%x, reg_addr:%x, value:%x\n", slave_addr, reg_addr, data);
	LCM_DRIVER_INFO("leave\n");
}
void lcm_write_reg(unsigned char i2c_id, unsigned char slave_addr, unsigned char reg_addr, unsigned char value)
{
	LCM_DRIVER_INFO("enter\n");
	interface_write_byte(i2c_id, slave_addr, reg_addr, value);
	LCM_DRIVER_INFO("write slave_addr:%x, reg_addr:%x, value:%x\n", slave_addr, reg_addr, value);
	LCM_DRIVER_INFO("leave\n");
}

struct LCM_DRIVER lcm_driver_common_lcm_drv = {
	.name = "lcm_driver_common",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
};

