/*
* Copyright (c) 2019 AutoChips Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#ifndef __LCM_DRIVER_COMMON_H__
#define __LCM_DRIVER_COMMON_H__

#ifndef UINT32
#define UINT32 unsigned int
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef UINT8
#define UINT8 unsigned char
#endif

/*#define LCM_DRIVER_DEBUG_EN*/
#define LCM_DRIVER_FATAL(string, args...)  \
	pr_crit("[F] [DISP][LCM] [%s: %d] "string, __FUNCTION__, __LINE__, ##args)
#define LCM_DRIVER_ERROR(string, args...)  \
	pr_err("[E] [DISP][LCM] [%s: %d] "string,  __FUNCTION__, __LINE__, ##args)
#define LCM_DRIVER_INFO(string, args...)   \
	pr_info("[I] [DISP][LCM] [%s] "string, __FUNCTION__, ##args)
#define LCM_DRIVER_PRINTF(string, args...) pr_info(string, ##args)
#ifdef LCM_DRIVER_DEBUG_EN
#define LCM_DRIVER_DEBUG(string, args...)  pr_debug("[D] [DISP][LCM] [%s] "string, __FUNCTION__, ##args)
#else
#define LCM_DRIVER_DEBUG(string, args...)
#endif

#define GPIO_NUM_MAX			10
#define PARA_LIST_LEN_MAX 		64
#define MAX_ESD_CHECK_TABLE_LEN		12

#define SCREEN_MODE_ON                 0x1
#define SCREEN_MODE_START_POWER_OFF    0x2

#define BRIDGE_IC_DETECT_SUCCESS	1
#define BRIDGE_IC_DETECT_FAILED		2

enum interface_type{
	INTERFACE_TYPE_LVDS,
	INTERFACE_TYPE_MIPI,
};

enum split_type{
	HORIZONTAL_SPLIT,
	VERTICAL_SPLIT,
};

enum bridge_ic_type{
	DS90UB941_BRIDGE_IC = 1,
	GAOWIN_BRIDGE_IC,
	SN65DSI83_BRIDGE_IC,
};


enum mipi_mode_type{
	MIPI_MODE_CMD,
	MIPI_MODE_ESYNC_PULSE_VDO,
	MIPI_MODE_SYNC_EVENT_VDO,
	MIPI_MODE_BURST_VDO_MODE,
};

enum bit_format{
	BIT_FORMAT_6 = 6,
	BIT_FORMAT_8 = 8,
};


struct lcm_panel_info{
	UINT32 size;
	UINT32 offset;
	UINT32 pixel_width;
	UINT32 pixel_height;
	UINT32 phy_width;
	UINT32 phy_height;
	UINT32 clock;
	UINT16 interface_type;
	UINT16 bit_format;
	UINT16 htotal;
	UINT16 vtotal;
	UINT16 h_sync_width;
	UINT16 h_front_porch;
	UINT16 h_back_porch;
	UINT16 v_sync_width;
	UINT16 v_front_porch;
	UINT16 v_back_porch;
	UINT16 ssc_disable;
	UINT16 ssc_range;

	UINT16 lvds_mode;
	UINT16 lvds_link_mode;
	UINT16 lvds_data_format;
	UINT16 lvds_del_pol;
	UINT16 lvds_clk_pol;
	UINT16 lvds_vsync_pol;
	UINT16 lvds_hsync_pol;

	UINT16 mipi_mode;
	UINT16 mipi_lane_num;
	UINT16 mipi_esd_check_enable;
	UINT8  mipi_esd_check_table[MAX_ESD_CHECK_TABLE_LEN];
	UINT32 reserved[32];
	UINT32 crc;
} ;

struct lcm_panel_header{
	UINT8  image_file_name[8];
	UINT32 real_size;
	UINT8  panel_count;
	struct lcm_panel_info primary_panel_info;
	struct lcm_panel_info secondary_panel_info;
};

/*for lvds interface*/
struct lvds_setting_table{
	unsigned char cmd; /*cmd :0~255*/
	unsigned char data;/*data :0~255*/
};

struct lvds_setting_table_list{
	struct lvds_setting_table setting_table;
	struct lvds_setting_table_list *next;
};

struct lvds_setting_info{
	unsigned int cnt;
	struct lvds_setting_table_list head_list;
};

/*for mipi interface*/
struct mipi_setting_table{
	unsigned int cmd;		/*cmd :0~0xFFFF*/
	unsigned char cnt;		/*para cnt :0~PARA_LIST_LEN_MAX*/
	unsigned char para_list[PARA_LIST_LEN_MAX];/*PARA_LIST_LEN_MAX :64*/
};

struct mipi_setting_table_list{
	struct mipi_setting_table setting_table;
	struct mipi_setting_table_list *next;
};

struct mipi_setting_info{
	unsigned int cnt;
	struct mipi_setting_table_list head_list;
};

#if 0

/*for  power on/off timng control*/
struct gpio_ctl_cmd{
	unsigned char gpio_id;  /*gpio:0~255*/
	unsigned char level;	/*low level:0, high level:1*/
	unsigned char duration;	/*ms:0~255*/
};

struct gpio_ctl_cmd_list{
	struct gpio_ctl_cmd cmd;
	struct gpio_ctl_cmd_list *next;
};

struct lcm_timing_info{
	unsigned char gpio_cnt;
	unsigned char gpio_list[GPIO_NUM_MAX];/*GPIO_NUM_MAX :10*/
	struct gpio_ctl_cmd_list head_list;
};
#endif
/*for  power on/off timng control*/
struct gpio_ctl_cmd{
	unsigned char level;	/*low level:0, high level:1*/
	unsigned char duration;	/*ms:0~255*/
};

struct gpio_ctl_cmd_list{
	struct gpio_ctl_cmd cmd;
	struct gpio_ctl_cmd_list *next;
};

struct lcm_timing_info{
	unsigned char cnt;
	struct gpio_ctl_cmd_list head_list;
};

void modules_lcm_backlight_ctrl(unsigned char i2c_id, int mode, int backlight_value);
int lcm_bridge_ic_detect_ds90ub947(unsigned char i2c_id);
int lcm_bridge_ic_detect_gaowin(unsigned char i2c_id);
#endif
