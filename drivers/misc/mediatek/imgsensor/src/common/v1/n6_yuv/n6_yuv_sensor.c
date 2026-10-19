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

/*****************************************************************************
*
* Filename:
* ---------
*   N6_yuv_Sensor.c
*
* Project:
* --------
*   Android P on AC8257
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *
 * Author:
 * -------
 *   Atc0168
 *
 ******************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "n6_yuv_sensor.h"
#include "metazone/metazone.h"
#include "metazone/metazone_ioctl.h"


/****************************Modify Following Strings for Debug****************************/
#define PFX "[N6]"


/****************************   Modify end    *******************************************/
#define LOG_DBG(format, args...)    pr_debug(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    pr_info(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...)    pr_err(PFX " %s " format, __FUNCTION__, ##args)

static kal_uint16 n6_i2c_write_id = 0x62;
static int n6_i2c_speed = 400;
//static VIDEO_FORMAT source_format = FORMAT_MAX;
kal_uint32 n6_isp_master_clock;
static int n6_is_avm = 1;
static bool fastavm = false;

static kal_uint32 input_source_width = N6_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height = N6_IMAGE_SENSOR_HD_HEIGHT;

static kal_uint32 input_source_width_ch0 = N6_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height_ch0 = N6_IMAGE_SENSOR_HD_HEIGHT;

static kal_uint32 input_source_width_ch1 = N6_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height_ch1 = N6_IMAGE_SENSOR_HD_HEIGHT;

static kal_uint32 input_source_width_ch2 = N6_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height_ch2 = N6_IMAGE_SENSOR_HD_HEIGHT;

static kal_uint32 input_source_width_ch3 = N6_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height_ch3 = N6_IMAGE_SENSOR_HD_HEIGHT;

MSDK_SENSOR_CONFIG_STRUCT n6SensorConfigData;


static kal_uint16 n6_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[1] = { (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 1, (u8 *) &get_byte, 1, n6_i2c_write_id);

	return get_byte;
}

static int n6_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[2] = { (char)(addr & 0xFF), (char)(para & 0xFF) };
	LOG_INF("addr = 0x%x, value = 0x%x",addr,para);
	return iWriteRegI2CTiming(
		pu_send_cmd, 2, n6_i2c_write_id, n6_i2c_speed);
}

#define _MIPI_PORT_0_
#ifdef _MIPI_PORT_0_
#define _MAR_BANK_ 0x20
#define _MTX_BANK_ 0x23
#else
#define _MAR_BANK_ 0x30
#define _MTX_BANK_ 0x33
#endif
static void n6_sensor_hd30_init(void)
{
	LOG_INF("sensor_init hd30 in111");
	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x80, 0x00);  //4ch the same video setting,relatives to bank5/6/7/8
	n6_write_cmos_sensor(0x00, 0x10);
	n6_write_cmos_sensor(0x01, 0x10);
	n6_write_cmos_sensor(0x02, 0x10);
	n6_write_cmos_sensor(0x03, 0x10);
	n6_write_cmos_sensor(0x08, 0x00);
	n6_write_cmos_sensor(0x09, 0x00);
	n6_write_cmos_sensor(0x0a, 0x00);
	n6_write_cmos_sensor(0x0b, 0x00);
	n6_write_cmos_sensor(0x18, 0x3f);
	n6_write_cmos_sensor(0x19, 0x3f);
	n6_write_cmos_sensor(0x1a, 0x3f);
	n6_write_cmos_sensor(0x1b, 0x3f);
	n6_write_cmos_sensor(0x22, 0x0b);
	n6_write_cmos_sensor(0x23, 0x41);
	n6_write_cmos_sensor(0x26, 0x0b);
	n6_write_cmos_sensor(0x27, 0x41);
	n6_write_cmos_sensor(0x2a, 0x0b);
	n6_write_cmos_sensor(0x2b, 0x41);
	n6_write_cmos_sensor(0x2e, 0x0b);
	n6_write_cmos_sensor(0x2f, 0x41);
	n6_write_cmos_sensor(0x30, 0x12);
	n6_write_cmos_sensor(0x31, 0x12);
	n6_write_cmos_sensor(0x32, 0x12);
	n6_write_cmos_sensor(0x33, 0x12);
	n6_write_cmos_sensor(0x34, 0x00);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0x00);
	n6_write_cmos_sensor(0x37, 0x00);
	n6_write_cmos_sensor(0x58, 0x80);
	n6_write_cmos_sensor(0x59, 0x80);
	n6_write_cmos_sensor(0x5a, 0x80);
	n6_write_cmos_sensor(0x5b, 0x80);
	n6_write_cmos_sensor(0x5c, 0x00);
	n6_write_cmos_sensor(0x5d, 0x00);
	n6_write_cmos_sensor(0x5e, 0x00);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x64, 0x01);
	n6_write_cmos_sensor(0x65, 0x01);
	n6_write_cmos_sensor(0x66, 0x01);
	n6_write_cmos_sensor(0x67, 0x01);
	n6_write_cmos_sensor(0x81, 0x0C);
	n6_write_cmos_sensor(0x82, 0x0C);
	n6_write_cmos_sensor(0x83, 0x0C);
	n6_write_cmos_sensor(0x84, 0x0C);

	n6_write_cmos_sensor(0x85, 0x00);
	n6_write_cmos_sensor(0x86, 0x00);
	n6_write_cmos_sensor(0x87, 0x00);
	n6_write_cmos_sensor(0x88, 0x00);
	n6_write_cmos_sensor(0x89, 0x00);
	n6_write_cmos_sensor(0x8a, 0x00);
	n6_write_cmos_sensor(0x8b, 0x00);
	n6_write_cmos_sensor(0x8c, 0x00);
	n6_write_cmos_sensor(0x8e, 0x00);
	n6_write_cmos_sensor(0x8f, 0x00);
	n6_write_cmos_sensor(0x90, 0x00);
	n6_write_cmos_sensor(0x91, 0x00);
	n6_write_cmos_sensor(0xa0, 0x05);
	n6_write_cmos_sensor(0xa1, 0x05);
	n6_write_cmos_sensor(0xa2, 0x05);
	n6_write_cmos_sensor(0xa3, 0x05);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x84, 0x02);
	n6_write_cmos_sensor(0x85, 0x02);
	n6_write_cmos_sensor(0x86, 0x02);
	n6_write_cmos_sensor(0x87, 0x02);
	n6_write_cmos_sensor(0x88, 0x00);
	n6_write_cmos_sensor(0x89, 0x00);
	n6_write_cmos_sensor(0x8a, 0x00);
	n6_write_cmos_sensor(0x8b, 0x00);
	n6_write_cmos_sensor(0x8c, 0x40);
	n6_write_cmos_sensor(0x8d, 0x40);
	n6_write_cmos_sensor(0x8e, 0x40);
	n6_write_cmos_sensor(0x8f, 0x40);
	n6_write_cmos_sensor(0xa0, 0x20);
	n6_write_cmos_sensor(0xa1, 0x20);
	n6_write_cmos_sensor(0xa2, 0x20);
	n6_write_cmos_sensor(0xa3, 0x20);
	n6_write_cmos_sensor(0xed, 0x00);

	n6_write_cmos_sensor(0xff, 0x05);
	n6_write_cmos_sensor(0x00, 0xd0);
	n6_write_cmos_sensor(0x01, 0x22);
	n6_write_cmos_sensor(0x05, 0x04);
	n6_write_cmos_sensor(0x08, 0x55);
	n6_write_cmos_sensor(0x25, 0xdc);
	n6_write_cmos_sensor(0x28, 0x80);
	n6_write_cmos_sensor(0x2f, 0x00);
	n6_write_cmos_sensor(0x30, 0xe0);
	n6_write_cmos_sensor(0x31, 0x43);
	n6_write_cmos_sensor(0x32, 0xa2);
	n6_write_cmos_sensor(0x47, 0xee);
	n6_write_cmos_sensor(0x50, 0xc6);
	n6_write_cmos_sensor(0x57, 0x00);
	n6_write_cmos_sensor(0x58, 0x77);
	n6_write_cmos_sensor(0x5b, 0x41);
	n6_write_cmos_sensor(0x5c, 0x7C);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x7b, 0x11);
	n6_write_cmos_sensor(0x7c, 0x01);
	n6_write_cmos_sensor(0x7d, 0x80);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x90, 0x01);
	n6_write_cmos_sensor(0xa9, 0x00);
	n6_write_cmos_sensor(0xb8, 0x39);
	n6_write_cmos_sensor(0xb9, 0x72);
	n6_write_cmos_sensor(0xd1, 0x00);
	n6_write_cmos_sensor(0xd5, 0x80);

	n6_write_cmos_sensor(0xff, 0x09);
	n6_write_cmos_sensor(0x50, 0x30);
	n6_write_cmos_sensor(0x51, 0x6f);
	n6_write_cmos_sensor(0x52, 0x67);
	n6_write_cmos_sensor(0x53, 0x48);
	n6_write_cmos_sensor(0x54, 0x30);
	n6_write_cmos_sensor(0x55, 0x6f);
	n6_write_cmos_sensor(0x56, 0x67);
	n6_write_cmos_sensor(0x57, 0x48);
	n6_write_cmos_sensor(0x58, 0x30);
	n6_write_cmos_sensor(0x59, 0x6f);
	n6_write_cmos_sensor(0x5a, 0x67);
	n6_write_cmos_sensor(0x5b, 0x48);
	n6_write_cmos_sensor(0x5c, 0x30);
	n6_write_cmos_sensor(0x5d, 0x6f);
	n6_write_cmos_sensor(0x5e, 0x67);
	n6_write_cmos_sensor(0x5f, 0x48);
	n6_write_cmos_sensor(0x96, 0x00);
	n6_write_cmos_sensor(0x98, 0x00);
	n6_write_cmos_sensor(0x9e, 0x00);
	n6_write_cmos_sensor(0xb6, 0x00);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xbe, 0x00);
	n6_write_cmos_sensor(0xd6, 0x00);
	n6_write_cmos_sensor(0xd8, 0x00);
	n6_write_cmos_sensor(0xde, 0x00);
	n6_write_cmos_sensor(0xf6, 0x00);
	n6_write_cmos_sensor(0xf8, 0x00);
	n6_write_cmos_sensor(0xfe, 0x00);

	n6_write_cmos_sensor(0xff, 0x0a);
	n6_write_cmos_sensor(0x25, 0x10);
	n6_write_cmos_sensor(0x27, 0x1e);
	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);
	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);
	n6_write_cmos_sensor(0x34, 0x40);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);
	n6_write_cmos_sensor(0xa5, 0x10);
	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);
	n6_write_cmos_sensor(0xff, 0x0b);
	n6_write_cmos_sensor(0x25, 0x10);
	n6_write_cmos_sensor(0x27, 0x1e);
	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);
	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);
	n6_write_cmos_sensor(0x34, 0x40);


	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);
	n6_write_cmos_sensor(0xa5, 0x10);
	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);

	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);

	n6_write_cmos_sensor(0xff, 0x13);
	n6_write_cmos_sensor(0x05, 0xa0);
	n6_write_cmos_sensor(0x31, 0xff);
	n6_write_cmos_sensor(0x07, 0x47);
	n6_write_cmos_sensor(0x12, 0x04);
	n6_write_cmos_sensor(0x1e, 0x1f);
	n6_write_cmos_sensor(0x1f, 0x27);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	n6_write_cmos_sensor(0x01, 0x55);
	n6_write_cmos_sensor(0x10, 0xff);
	n6_write_cmos_sensor(0x11, 0xff);
	n6_write_cmos_sensor(0x12, 0x80);
	n6_write_cmos_sensor(0x13, 0x02);
	n6_write_cmos_sensor(0x14, 0x80);
	n6_write_cmos_sensor(0x15, 0x02);
	n6_write_cmos_sensor(0x16, 0x80);
	n6_write_cmos_sensor(0x17, 0x02);
	n6_write_cmos_sensor(0x18, 0x80);
	n6_write_cmos_sensor(0x19, 0x02);
	n6_write_cmos_sensor(0x30, 0x0f);
	n6_write_cmos_sensor(0x32, 0x92);
	n6_write_cmos_sensor(0x34, 0xcd);
	n6_write_cmos_sensor(0x36, 0x04);
	n6_write_cmos_sensor(0x38, 0x58);
	n6_write_cmos_sensor(0x3c, 0x01);
	n6_write_cmos_sensor(0x3d, 0x11);
	n6_write_cmos_sensor(0x3e, 0x11);
	n6_write_cmos_sensor(0x45, 0x60);
	n6_write_cmos_sensor(0x46, 0x49);

	n6_write_cmos_sensor(0xff, _MTX_BANK_);
	n6_write_cmos_sensor(0x01, 0xe4);
	n6_write_cmos_sensor(0x00, 0x7d);
	n6_write_cmos_sensor(0x01, 0xe0);
	n6_write_cmos_sensor(0x02, 0xa0);
	n6_write_cmos_sensor(0x20, 0x1e);
	n6_write_cmos_sensor(0x20, 0x1f);
	n6_write_cmos_sensor(0x03, 0x02);
	n6_write_cmos_sensor(0x04, 0x0c); //162M test,0x38 756M
	n6_write_cmos_sensor(0x45, 0xc4);
	n6_write_cmos_sensor(0x46, 0x01);
	n6_write_cmos_sensor(0x47, 0x1b);
	n6_write_cmos_sensor(0x48, 0x08);
	n6_write_cmos_sensor(0x65, 0xc4);
	n6_write_cmos_sensor(0x66, 0x01);
	n6_write_cmos_sensor(0x67, 0x1b);
	n6_write_cmos_sensor(0x68, 0x08);
	n6_write_cmos_sensor(0x85, 0xc4);
	n6_write_cmos_sensor(0x86, 0x01);
	n6_write_cmos_sensor(0x87, 0x1b);
	n6_write_cmos_sensor(0x88, 0x08);
	n6_write_cmos_sensor(0xa5, 0xc4);
	n6_write_cmos_sensor(0xa6, 0x01);
	n6_write_cmos_sensor(0xa7, 0x1b);
	n6_write_cmos_sensor(0xa8, 0x08);
	n6_write_cmos_sensor(0xc5, 0xc4);
	n6_write_cmos_sensor(0xc6, 0x01);
	n6_write_cmos_sensor(0xc7, 0x1b);
	n6_write_cmos_sensor(0xc8, 0x08);
	n6_write_cmos_sensor(0xe1, 0x02);
	n6_write_cmos_sensor(0xe9, 0x03); //4lane,=01 2lane, =00 1lane
	n6_write_cmos_sensor(0xeb, 0x8d);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x97, 0x00);
	n6_write_cmos_sensor(0x97, 0x0f);
	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	n6_write_cmos_sensor(0x00, 0x11);
	n6_write_cmos_sensor(0x40, 0x01);
	n6_write_cmos_sensor(0x40, 0x00);

	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x78, 0xab);
	n6_write_cmos_sensor(0x79, 0xca);
	n6_write_cmos_sensor(0xff, 0x05);
	n6_write_cmos_sensor(0x2c, 0x08);
	n6_write_cmos_sensor(0x6a, 0x80);
}

static void n6_sensor_hd25_init(void)
{
	LOG_INF("sensor_init hd25 in");
	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x80, 0x00);  //4ch the same video setting,relatives to bank5/6/7/8
	n6_write_cmos_sensor(0x00, 0x10);
	n6_write_cmos_sensor(0x01, 0x10);
	n6_write_cmos_sensor(0x02, 0x10);
	n6_write_cmos_sensor(0x03, 0x10);
	n6_write_cmos_sensor(0x08, 0x00);
	n6_write_cmos_sensor(0x09, 0x00);
	n6_write_cmos_sensor(0x0a, 0x00);

	n6_write_cmos_sensor(0x0b, 0x00);
	n6_write_cmos_sensor(0x18, 0x3f);
	n6_write_cmos_sensor(0x19, 0x3f);
	n6_write_cmos_sensor(0x1a, 0x3f);
	n6_write_cmos_sensor(0x1b, 0x3f);
	n6_write_cmos_sensor(0x22, 0x0b);
	n6_write_cmos_sensor(0x23, 0x41);
	n6_write_cmos_sensor(0x26, 0x0b);
	n6_write_cmos_sensor(0x27, 0x41);
	n6_write_cmos_sensor(0x2a, 0x0b);
	n6_write_cmos_sensor(0x2b, 0x41);
	n6_write_cmos_sensor(0x2e, 0x0b);
	n6_write_cmos_sensor(0x2f, 0x41);
	n6_write_cmos_sensor(0x30, 0x12);
	n6_write_cmos_sensor(0x31, 0x12);
	n6_write_cmos_sensor(0x32, 0x12);
	n6_write_cmos_sensor(0x33, 0x12);
	n6_write_cmos_sensor(0x34, 0x00);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0x00);
	n6_write_cmos_sensor(0x37, 0x00);
	n6_write_cmos_sensor(0x58, 0x80);
	n6_write_cmos_sensor(0x59, 0x80);
	n6_write_cmos_sensor(0x5a, 0x80);
	n6_write_cmos_sensor(0x5b, 0x80);
	n6_write_cmos_sensor(0x5c, 0x00);
	n6_write_cmos_sensor(0x5d, 0x00);
	n6_write_cmos_sensor(0x5e, 0x00);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x64, 0x01);
	n6_write_cmos_sensor(0x65, 0x01);
	n6_write_cmos_sensor(0x66, 0x01);
	n6_write_cmos_sensor(0x67, 0x01);

	n6_write_cmos_sensor(0x81, 0x0d);
	n6_write_cmos_sensor(0x82, 0x0d);
	n6_write_cmos_sensor(0x83, 0x0d);
	n6_write_cmos_sensor(0x84, 0x0d);
	n6_write_cmos_sensor(0x85, 0x00);
	n6_write_cmos_sensor(0x86, 0x00);
	n6_write_cmos_sensor(0x87, 0x00);
	n6_write_cmos_sensor(0x88, 0x00);
	n6_write_cmos_sensor(0x89, 0x00);
	n6_write_cmos_sensor(0x8a, 0x00);
	n6_write_cmos_sensor(0x8b, 0x00);
	n6_write_cmos_sensor(0x8c, 0x00);
	n6_write_cmos_sensor(0x8e, 0x00);
	n6_write_cmos_sensor(0x8f, 0x00);
	n6_write_cmos_sensor(0x90, 0x00);
	n6_write_cmos_sensor(0x91, 0x00);
	n6_write_cmos_sensor(0xa0, 0x05);
	n6_write_cmos_sensor(0xa1, 0x05);
	n6_write_cmos_sensor(0xa2, 0x05);
	n6_write_cmos_sensor(0xa3, 0x05);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x84, 0x02);
	n6_write_cmos_sensor(0x85, 0x02);
	n6_write_cmos_sensor(0x86, 0x02);
	n6_write_cmos_sensor(0x87, 0x02);
	n6_write_cmos_sensor(0x88, 0x00);
	n6_write_cmos_sensor(0x89, 0x00);
	n6_write_cmos_sensor(0x8a, 0x00);
	n6_write_cmos_sensor(0x8b, 0x00);
	n6_write_cmos_sensor(0x8c, 0x40);
	n6_write_cmos_sensor(0x8d, 0x40);
	n6_write_cmos_sensor(0x8e, 0x40);

	n6_write_cmos_sensor(0x8f, 0x40);
	n6_write_cmos_sensor(0xa0, 0x20);
	n6_write_cmos_sensor(0xa1, 0x20);

	n6_write_cmos_sensor(0xa2, 0x20);
	n6_write_cmos_sensor(0xa3, 0x20);
	n6_write_cmos_sensor(0xed, 0x00);

	n6_write_cmos_sensor(0xff, 0x05);
	n6_write_cmos_sensor(0x00, 0xd0);
	n6_write_cmos_sensor(0x01, 0x22);
	n6_write_cmos_sensor(0x05, 0x04);
	n6_write_cmos_sensor(0x08, 0x55);
	n6_write_cmos_sensor(0x25, 0xdc);
	n6_write_cmos_sensor(0x28, 0x80);
	n6_write_cmos_sensor(0x2f, 0x00);

	n6_write_cmos_sensor(0x30, 0xe0);
	n6_write_cmos_sensor(0x31, 0x43);
	n6_write_cmos_sensor(0x32, 0xa2);
	n6_write_cmos_sensor(0x47, 0xee);

	n6_write_cmos_sensor(0x50, 0xc6);
	n6_write_cmos_sensor(0x57, 0x00);
	n6_write_cmos_sensor(0x58, 0x77);
	n6_write_cmos_sensor(0x5b, 0x41);
	n6_write_cmos_sensor(0x5c, 0x7C);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x7b, 0x11);
	n6_write_cmos_sensor(0x7c, 0x01);
	n6_write_cmos_sensor(0x7d, 0x80);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x90, 0x01);
	n6_write_cmos_sensor(0xa9, 0x00);
	n6_write_cmos_sensor(0xb8, 0x39);
	n6_write_cmos_sensor(0xb9, 0x72);
	n6_write_cmos_sensor(0xd1, 0x00);
	n6_write_cmos_sensor(0xd5, 0x80);
	n6_write_cmos_sensor(0xff, 0x09);
	n6_write_cmos_sensor(0x50, 0x30);
	n6_write_cmos_sensor(0x51, 0x6f);
	n6_write_cmos_sensor(0x52, 0x67);
	n6_write_cmos_sensor(0x53, 0x48);
	n6_write_cmos_sensor(0x54, 0x30);
	n6_write_cmos_sensor(0x55, 0x6f);
	n6_write_cmos_sensor(0x56, 0x67);
	n6_write_cmos_sensor(0x57, 0x48);
	n6_write_cmos_sensor(0x58, 0x30);
	n6_write_cmos_sensor(0x59, 0x6f);
	n6_write_cmos_sensor(0x5a, 0x67);
	n6_write_cmos_sensor(0x5b, 0x48);
	n6_write_cmos_sensor(0x5c, 0x30);
	n6_write_cmos_sensor(0x5d, 0x6f);
	n6_write_cmos_sensor(0x5e, 0x67);
	n6_write_cmos_sensor(0x5f, 0x48);
	n6_write_cmos_sensor(0x96, 0x00);
	n6_write_cmos_sensor(0x98, 0x00);
	n6_write_cmos_sensor(0x9e, 0x00);
	n6_write_cmos_sensor(0xb6, 0x00);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xbe, 0x00);
	n6_write_cmos_sensor(0xd6, 0x00);
	n6_write_cmos_sensor(0xd8, 0x00);
	n6_write_cmos_sensor(0xde, 0x00);
	n6_write_cmos_sensor(0xf6, 0x00);
	n6_write_cmos_sensor(0xf8, 0x00);
	n6_write_cmos_sensor(0xfe, 0x00);

	n6_write_cmos_sensor(0xff, 0x0a);
	n6_write_cmos_sensor(0x25, 0x10);
	n6_write_cmos_sensor(0x27, 0x1e);
	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);
	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);
	n6_write_cmos_sensor(0x34, 0x40);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);
	n6_write_cmos_sensor(0xa5, 0x10);
	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);
	n6_write_cmos_sensor(0xff, 0x0b);
	n6_write_cmos_sensor(0x25, 0x10);
	n6_write_cmos_sensor(0x27, 0x1e);

	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);
	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);
	n6_write_cmos_sensor(0x34, 0x40);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);
	n6_write_cmos_sensor(0xa5, 0x10);
	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);

	n6_write_cmos_sensor(0xff, 0x13);
	n6_write_cmos_sensor(0x05, 0xa0);
	n6_write_cmos_sensor(0x31, 0xff);
	n6_write_cmos_sensor(0x07, 0x47);
	n6_write_cmos_sensor(0x12, 0x04);
	n6_write_cmos_sensor(0x1e, 0x1f);
	n6_write_cmos_sensor(0x1f, 0x27);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	n6_write_cmos_sensor(0x01, 0x55);
	n6_write_cmos_sensor(0x10, 0xff);
	n6_write_cmos_sensor(0x11, 0xff);
	n6_write_cmos_sensor(0x12, 0x80);
	n6_write_cmos_sensor(0x13, 0x02);
	n6_write_cmos_sensor(0x14, 0x80);
	n6_write_cmos_sensor(0x15, 0x02);
	n6_write_cmos_sensor(0x16, 0x80);
	n6_write_cmos_sensor(0x17, 0x02);
	n6_write_cmos_sensor(0x18, 0x80);
	n6_write_cmos_sensor(0x19, 0x02);
	n6_write_cmos_sensor(0x30, 0x0f);
	n6_write_cmos_sensor(0x34, 0xcd);
	n6_write_cmos_sensor(0x38, 0xff);
	n6_write_cmos_sensor(0x3c, 0x01);
	n6_write_cmos_sensor(0x3d, 0x11);
	n6_write_cmos_sensor(0x3e, 0x11);
	n6_write_cmos_sensor(0x45, 0x60);
	n6_write_cmos_sensor(0x46, 0x49);

	n6_write_cmos_sensor(0xff, _MTX_BANK_);
	n6_write_cmos_sensor(0x01, 0xe4);
	n6_write_cmos_sensor(0x00, 0x7d);
	n6_write_cmos_sensor(0x01, 0xe0);
	n6_write_cmos_sensor(0x02, 0xa0);
	n6_write_cmos_sensor(0x20, 0x1e);
	n6_write_cmos_sensor(0x20, 0x1f);
	n6_write_cmos_sensor(0x03, 0x02);
	n6_write_cmos_sensor(0x04, 0x6c);
	n6_write_cmos_sensor(0x45, 0xcd);
	n6_write_cmos_sensor(0x46, 0x42);
	n6_write_cmos_sensor(0x47, 0x36);
	n6_write_cmos_sensor(0x65, 0xcd);
	n6_write_cmos_sensor(0x66, 0x42);
	n6_write_cmos_sensor(0x67, 0x0e);
	n6_write_cmos_sensor(0x85, 0xcd);
	n6_write_cmos_sensor(0x86, 0x42);
	n6_write_cmos_sensor(0x87, 0x0e);
	n6_write_cmos_sensor(0xa5, 0xcd);
	n6_write_cmos_sensor(0xa6, 0x42);
	n6_write_cmos_sensor(0xa7, 0x0e);
	n6_write_cmos_sensor(0xc5, 0xcd);
	n6_write_cmos_sensor(0xc6, 0x42);
	n6_write_cmos_sensor(0xc7, 0x0e);
	n6_write_cmos_sensor(0xe9, 0x03); //4lane,=01 2lane, =00 1lane
	n6_write_cmos_sensor(0xeb, 0x8d);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x97, 0x00);
	n6_write_cmos_sensor(0x97, 0x0f);
	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	n6_write_cmos_sensor(0x00, 0xff);
	n6_write_cmos_sensor(0x40, 0x01);
	n6_write_cmos_sensor(0x40, 0x00);

	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x78, 0xab);
	n6_write_cmos_sensor(0x79, 0xca);
	n6_write_cmos_sensor(0xff, 0x05);
	n6_write_cmos_sensor(0x2c, 0x08);
	n6_write_cmos_sensor(0x6a, 0x80);
}

static void n6_sensor_fhd30_init(void)
{
	LOG_INF("sensor_init fhd30 in1111");
	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x00, 0x10);
	n6_write_cmos_sensor(0x01, 0x10);
	n6_write_cmos_sensor(0x02, 0x10);
	n6_write_cmos_sensor(0x03, 0x10);
	n6_write_cmos_sensor(0x08, 0x00);
	n6_write_cmos_sensor(0x09, 0x00);
	n6_write_cmos_sensor(0x0a, 0x00);
	n6_write_cmos_sensor(0x0b, 0x00);
	n6_write_cmos_sensor(0x18, 0x3f);
	n6_write_cmos_sensor(0x19, 0x3f);
	n6_write_cmos_sensor(0x1a, 0x3f);
	n6_write_cmos_sensor(0x1b, 0x3f);
	n6_write_cmos_sensor(0x22, 0x0b);

	n6_write_cmos_sensor(0x23, 0x41);
	n6_write_cmos_sensor(0x26, 0x0b);
	n6_write_cmos_sensor(0x27, 0x41);
	n6_write_cmos_sensor(0x2a, 0x0b);
	n6_write_cmos_sensor(0x2b, 0x41);
	n6_write_cmos_sensor(0x2e, 0x0b);
	n6_write_cmos_sensor(0x2f, 0x41);
	n6_write_cmos_sensor(0x30, 0x12);
	n6_write_cmos_sensor(0x31, 0x12);
	n6_write_cmos_sensor(0x32, 0x12);
	n6_write_cmos_sensor(0x33, 0x12);
	n6_write_cmos_sensor(0x34, 0x00);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0x00);
	n6_write_cmos_sensor(0x37, 0x00);
	n6_write_cmos_sensor(0x58, 0x80);
	n6_write_cmos_sensor(0x59, 0x80);
	n6_write_cmos_sensor(0x5a, 0x80);
	n6_write_cmos_sensor(0x5b, 0x80);
	n6_write_cmos_sensor(0x5c, 0x00);
	n6_write_cmos_sensor(0x5d, 0x00);
	n6_write_cmos_sensor(0x5e, 0x00);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x64, 0x01);
	n6_write_cmos_sensor(0x65, 0x01);
	n6_write_cmos_sensor(0x66, 0x01);
	n6_write_cmos_sensor(0x67, 0x01);
	n6_write_cmos_sensor(0x81, 0x02);
	n6_write_cmos_sensor(0x82, 0x02);
	n6_write_cmos_sensor(0x83, 0x02);
	n6_write_cmos_sensor(0x84, 0x02);
	n6_write_cmos_sensor(0x85, 0x00);
	n6_write_cmos_sensor(0x86, 0x00);
	n6_write_cmos_sensor(0x87, 0x00);
	n6_write_cmos_sensor(0x88, 0x00);
	n6_write_cmos_sensor(0x89, 0x10);
	n6_write_cmos_sensor(0x8a, 0x10);

	n6_write_cmos_sensor(0x8b, 0x10);
	n6_write_cmos_sensor(0x8c, 0x10);
	n6_write_cmos_sensor(0x8e, 0x00);
	n6_write_cmos_sensor(0x8f, 0x00);
	n6_write_cmos_sensor(0x90, 0x00);
	n6_write_cmos_sensor(0x91, 0x00);
	n6_write_cmos_sensor(0xa0, 0x05);
	n6_write_cmos_sensor(0xa1, 0x05);
	n6_write_cmos_sensor(0xa2, 0x05);
	n6_write_cmos_sensor(0xa3, 0x05);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x84, 0x80);
	n6_write_cmos_sensor(0x85, 0x80);
	n6_write_cmos_sensor(0x86, 0x80);
	n6_write_cmos_sensor(0x87, 0x80);
	n6_write_cmos_sensor(0x88, 0x00);
	n6_write_cmos_sensor(0x89, 0x00);
	n6_write_cmos_sensor(0x8a, 0x00);
	n6_write_cmos_sensor(0x8b, 0x00);
	n6_write_cmos_sensor(0x8c, 0x40);
	n6_write_cmos_sensor(0x8d, 0x40);
	n6_write_cmos_sensor(0x8e, 0x40);
	n6_write_cmos_sensor(0x8f, 0x40);
	n6_write_cmos_sensor(0xa0, 0x20);
	n6_write_cmos_sensor(0xa1, 0x20);
	n6_write_cmos_sensor(0xa2, 0x20);
	n6_write_cmos_sensor(0xa3, 0x20);
	n6_write_cmos_sensor(0xed, 0x00);

	n6_write_cmos_sensor(0xff, 0x05);
	n6_write_cmos_sensor(0x00, 0xd0);
	n6_write_cmos_sensor(0x01, 0x62);
	n6_write_cmos_sensor(0x05, 0x04);
	n6_write_cmos_sensor(0x08, 0x55);
	n6_write_cmos_sensor(0x1b, 0x08);
	n6_write_cmos_sensor(0x25, 0xdc);
	n6_write_cmos_sensor(0x28, 0x80);
	n6_write_cmos_sensor(0x2f, 0x00);
	n6_write_cmos_sensor(0x30, 0xe0);
	n6_write_cmos_sensor(0x31, 0x43);
	n6_write_cmos_sensor(0x32, 0xa2);
	n6_write_cmos_sensor(0x47, 0xee);
	n6_write_cmos_sensor(0x50, 0xc6);
	n6_write_cmos_sensor(0x57, 0x00);
	n6_write_cmos_sensor(0x58, 0x77);
	n6_write_cmos_sensor(0x5b, 0x41);
	n6_write_cmos_sensor(0x5c, 0x7C);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x7b, 0x11);
	n6_write_cmos_sensor(0x7c, 0x01);
	n6_write_cmos_sensor(0x7d, 0x80);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x90, 0x01);
	n6_write_cmos_sensor(0xa9, 0x00);
	n6_write_cmos_sensor(0xb8, 0x39);
	n6_write_cmos_sensor(0xb9, 0x72);
	n6_write_cmos_sensor(0xd1, 0x00);
	n6_write_cmos_sensor(0xd5, 0x80);

	n6_write_cmos_sensor(0xff, 0x09);
	n6_write_cmos_sensor(0x50, 0x30);
	n6_write_cmos_sensor(0x51, 0x6f);
	n6_write_cmos_sensor(0x52, 0x67);
	n6_write_cmos_sensor(0x53, 0x48);
	n6_write_cmos_sensor(0x54, 0x30);
	n6_write_cmos_sensor(0x55, 0x6f);
	n6_write_cmos_sensor(0x56, 0x67);
	n6_write_cmos_sensor(0x57, 0x48);
	n6_write_cmos_sensor(0x58, 0x30);
	n6_write_cmos_sensor(0x59, 0x6f);
	n6_write_cmos_sensor(0x5a, 0x67);
	n6_write_cmos_sensor(0x5b, 0x48);
	n6_write_cmos_sensor(0x5c, 0x30);
	n6_write_cmos_sensor(0x5d, 0x6f);
	n6_write_cmos_sensor(0x5e, 0x67);
	n6_write_cmos_sensor(0x5f, 0x48);
	n6_write_cmos_sensor(0x96, 0x00);
	n6_write_cmos_sensor(0x98, 0x00);
	n6_write_cmos_sensor(0x9e, 0x00);
	n6_write_cmos_sensor(0xb6, 0x00);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xbe, 0x00);
	n6_write_cmos_sensor(0xd6, 0x00);
	n6_write_cmos_sensor(0xd8, 0x00);
	n6_write_cmos_sensor(0xde, 0x00);
	n6_write_cmos_sensor(0xf6, 0x00);
	n6_write_cmos_sensor(0xf8, 0x00);
	n6_write_cmos_sensor(0xfe, 0x00);

	n6_write_cmos_sensor(0xff, 0x0a);
	n6_write_cmos_sensor(0x25, 0x10);
	n6_write_cmos_sensor(0x27, 0x1e);
	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);
	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);
	n6_write_cmos_sensor(0x34, 0x40);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);
	n6_write_cmos_sensor(0xa5, 0x10);
	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);
	n6_write_cmos_sensor(0xff, 0x0b);
	n6_write_cmos_sensor(0x25, 0x10);

	n6_write_cmos_sensor(0x27, 0x1e);
	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);

	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);

	n6_write_cmos_sensor(0x34, 0x40);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);

	n6_write_cmos_sensor(0xa5, 0x10);


	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);

	n6_write_cmos_sensor(0xff, 0x13);
	n6_write_cmos_sensor(0x05, 0xa0);
	n6_write_cmos_sensor(0x31, 0xff);
	n6_write_cmos_sensor(0x07, 0x47);
	n6_write_cmos_sensor(0x12, 0x04);
	n6_write_cmos_sensor(0x1e, 0x1f);
	n6_write_cmos_sensor(0x1f, 0x27);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	n6_write_cmos_sensor(0x01, 0x00);
	n6_write_cmos_sensor(0x10, 0xff);
	n6_write_cmos_sensor(0x11, 0xff);
	n6_write_cmos_sensor(0x12, 0xc0);
	n6_write_cmos_sensor(0x13, 0x03);
	n6_write_cmos_sensor(0x14, 0xc0);
	n6_write_cmos_sensor(0x15, 0x03);
	n6_write_cmos_sensor(0x16, 0xc0);
	n6_write_cmos_sensor(0x17, 0x03);
	n6_write_cmos_sensor(0x18, 0xc0);
	n6_write_cmos_sensor(0x19, 0x03);
	n6_write_cmos_sensor(0x30, 0x0f);
	n6_write_cmos_sensor(0x34, 0xcd);
	n6_write_cmos_sensor(0x38, 0xff);
	n6_write_cmos_sensor(0x3c, 0x01);
	n6_write_cmos_sensor(0x3d, 0x11);
	n6_write_cmos_sensor(0x3e, 0x11);
	n6_write_cmos_sensor(0x45, 0x60);
	n6_write_cmos_sensor(0x46, 0x49);

	n6_write_cmos_sensor(0xff, _MTX_BANK_);
	n6_write_cmos_sensor(0x01, 0xe4);
	n6_write_cmos_sensor(0x00, 0x7d);
	n6_write_cmos_sensor(0x01, 0xe0);
	n6_write_cmos_sensor(0x02, 0xa0);
	n6_write_cmos_sensor(0x20, 0x1e);
	n6_write_cmos_sensor(0x20, 0x1f);
	n6_write_cmos_sensor(0x03, 0x02);
	n6_write_cmos_sensor(0x04, 0x6c);
	n6_write_cmos_sensor(0x45, 0xcd);
	n6_write_cmos_sensor(0x46, 0x42);
	n6_write_cmos_sensor(0x47, 0x36);
	n6_write_cmos_sensor(0x65, 0xcd);
	n6_write_cmos_sensor(0x66, 0x42);
	n6_write_cmos_sensor(0x67, 0x0e);
	n6_write_cmos_sensor(0x85, 0xcd);
	n6_write_cmos_sensor(0x86, 0x42);
	n6_write_cmos_sensor(0x87, 0x0e);
	n6_write_cmos_sensor(0xa5, 0xcd);
	n6_write_cmos_sensor(0xa6, 0x42);
	n6_write_cmos_sensor(0xa7, 0x0e);
	n6_write_cmos_sensor(0xc5, 0xcd);
	n6_write_cmos_sensor(0xc6, 0x42);
	n6_write_cmos_sensor(0xc7, 0x0e);
	n6_write_cmos_sensor(0xe9, 0x03);
	n6_write_cmos_sensor(0xeb, 0x8d);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x97, 0x00);
	n6_write_cmos_sensor(0x97, 0x0f);
	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	n6_write_cmos_sensor(0x00, 0xff);
	n6_write_cmos_sensor(0x40, 0x01);
	n6_write_cmos_sensor(0x40, 0x00);

	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x78, 0xab);
	n6_write_cmos_sensor(0x79, 0xca);
	n6_write_cmos_sensor(0xff, 0x05);
	n6_write_cmos_sensor(0x2c, 0x08);
	n6_write_cmos_sensor(0x6a, 0x80);
}

static void n6_sensor_fhd25_init(void)
{
	LOG_INF("sensor_init fhd25 in");
	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x00, 0x10);
	n6_write_cmos_sensor(0x01, 0x10);
	n6_write_cmos_sensor(0x02, 0x10);
	n6_write_cmos_sensor(0x03, 0x10);
	n6_write_cmos_sensor(0x08, 0x00);
	n6_write_cmos_sensor(0x09, 0x00);
	n6_write_cmos_sensor(0x0a, 0x00);
	n6_write_cmos_sensor(0x0b, 0x00);
	n6_write_cmos_sensor(0x18, 0x3f);
	n6_write_cmos_sensor(0x19, 0x3f);
	n6_write_cmos_sensor(0x1a, 0x3f);
	n6_write_cmos_sensor(0x1b, 0x3f);
	n6_write_cmos_sensor(0x22, 0x0b);
	n6_write_cmos_sensor(0x23, 0x41);
	n6_write_cmos_sensor(0x26, 0x0b);
	n6_write_cmos_sensor(0x27, 0x41);
	n6_write_cmos_sensor(0x2a, 0x0b);
	n6_write_cmos_sensor(0x2b, 0x41);
	n6_write_cmos_sensor(0x2e, 0x0b);
	n6_write_cmos_sensor(0x2f, 0x41);
	n6_write_cmos_sensor(0x30, 0x12);
	n6_write_cmos_sensor(0x31, 0x12);
	n6_write_cmos_sensor(0x32, 0x12);
	n6_write_cmos_sensor(0x33, 0x12);
	n6_write_cmos_sensor(0x34, 0x00);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0x00);
	n6_write_cmos_sensor(0x37, 0x00);
	n6_write_cmos_sensor(0x58, 0x80);
	n6_write_cmos_sensor(0x59, 0x80);
	n6_write_cmos_sensor(0x5a, 0x80);
	n6_write_cmos_sensor(0x5b, 0x80);
	n6_write_cmos_sensor(0x5c, 0x00);
	n6_write_cmos_sensor(0x5d, 0x00);
	n6_write_cmos_sensor(0x5e, 0x00);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x64, 0x01);
	n6_write_cmos_sensor(0x65, 0x01);
	n6_write_cmos_sensor(0x66, 0x01);
	n6_write_cmos_sensor(0x67, 0x01);
	n6_write_cmos_sensor(0x81, 0x03);
	n6_write_cmos_sensor(0x82, 0x03);
	n6_write_cmos_sensor(0x83, 0x03);
	n6_write_cmos_sensor(0x84, 0x03);
	n6_write_cmos_sensor(0x85, 0x00);
	n6_write_cmos_sensor(0x86, 0x00);
	n6_write_cmos_sensor(0x87, 0x00);
	n6_write_cmos_sensor(0x88, 0x00);
	n6_write_cmos_sensor(0x89, 0x10);
	n6_write_cmos_sensor(0x8a, 0x10);
	n6_write_cmos_sensor(0x8b, 0x10);
	n6_write_cmos_sensor(0x8c, 0x10);
	n6_write_cmos_sensor(0x8e, 0x00);
	n6_write_cmos_sensor(0x8f, 0x00);
	n6_write_cmos_sensor(0x90, 0x00);
	n6_write_cmos_sensor(0x91, 0x00);
	n6_write_cmos_sensor(0xa0, 0x05);
	n6_write_cmos_sensor(0xa1, 0x05);
	n6_write_cmos_sensor(0xa2, 0x05);
	n6_write_cmos_sensor(0xa3, 0x05);


	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x84, 0x02);
	n6_write_cmos_sensor(0x85, 0x02);
	n6_write_cmos_sensor(0x86, 0x02);
	n6_write_cmos_sensor(0x87, 0x02);
	n6_write_cmos_sensor(0x88, 0x00);
	n6_write_cmos_sensor(0x89, 0x00);
	n6_write_cmos_sensor(0x8a, 0x00);
	n6_write_cmos_sensor(0x8b, 0x00);
	n6_write_cmos_sensor(0x8c, 0x40);
	n6_write_cmos_sensor(0x8d, 0x40);
	n6_write_cmos_sensor(0x8e, 0x40);
	n6_write_cmos_sensor(0x8f, 0x40);
	n6_write_cmos_sensor(0xa0, 0x20);
	n6_write_cmos_sensor(0xa1, 0x20);
	n6_write_cmos_sensor(0xa2, 0x20);
	n6_write_cmos_sensor(0xa3, 0x20);
	n6_write_cmos_sensor(0xed, 0x00);

	n6_write_cmos_sensor(0xff, 0x05);
	n6_write_cmos_sensor(0x00, 0xd0);
	n6_write_cmos_sensor(0x01, 0x62);
	n6_write_cmos_sensor(0x05, 0x04);
	n6_write_cmos_sensor(0x08, 0x55);
	n6_write_cmos_sensor(0x1b, 0x08);
	n6_write_cmos_sensor(0x25, 0xdc);
	n6_write_cmos_sensor(0x28, 0x80);
	n6_write_cmos_sensor(0x2f, 0x00);
	n6_write_cmos_sensor(0x30, 0xe0);
	n6_write_cmos_sensor(0x31, 0x43);
	n6_write_cmos_sensor(0x32, 0xa2);
	n6_write_cmos_sensor(0x47, 0xee);
	n6_write_cmos_sensor(0x50, 0xc6);
	n6_write_cmos_sensor(0x57, 0x00);
	n6_write_cmos_sensor(0x58, 0x77);
	n6_write_cmos_sensor(0x5b, 0x41);
	n6_write_cmos_sensor(0x5c, 0x7C);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x7b, 0x11);
	n6_write_cmos_sensor(0x7c, 0x01);
	n6_write_cmos_sensor(0x7d, 0x80);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x90, 0x01);
	n6_write_cmos_sensor(0xa9, 0x00);
	n6_write_cmos_sensor(0xb8, 0x39);
	n6_write_cmos_sensor(0xb9, 0x72);
	n6_write_cmos_sensor(0xd1, 0x00);
	n6_write_cmos_sensor(0xd5, 0x80);

	n6_write_cmos_sensor(0xff, 0x09);
	n6_write_cmos_sensor(0x50, 0x30);
	n6_write_cmos_sensor(0x51, 0x6f);
	n6_write_cmos_sensor(0x52, 0x67);
	n6_write_cmos_sensor(0x53, 0x48);
	n6_write_cmos_sensor(0x54, 0x30);
	n6_write_cmos_sensor(0x55, 0x6f);
	n6_write_cmos_sensor(0x56, 0x67);
	n6_write_cmos_sensor(0x57, 0x48);
	n6_write_cmos_sensor(0x58, 0x30);
	n6_write_cmos_sensor(0x59, 0x6f);
	n6_write_cmos_sensor(0x5a, 0x67);
	n6_write_cmos_sensor(0x5b, 0x48);
	n6_write_cmos_sensor(0x5c, 0x30);
	n6_write_cmos_sensor(0x5d, 0x6f);
	n6_write_cmos_sensor(0x5e, 0x67);
	n6_write_cmos_sensor(0x5f, 0x48);
	n6_write_cmos_sensor(0x96, 0x00);
	n6_write_cmos_sensor(0x98, 0x00);
	n6_write_cmos_sensor(0x9e, 0x00);
	n6_write_cmos_sensor(0xb6, 0x00);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xbe, 0x00);
	n6_write_cmos_sensor(0xd6, 0x00);
	n6_write_cmos_sensor(0xd8, 0x00);
	n6_write_cmos_sensor(0xde, 0x00);
	n6_write_cmos_sensor(0xf6, 0x00);
	n6_write_cmos_sensor(0xf8, 0x00);
	n6_write_cmos_sensor(0xfe, 0x00);

	n6_write_cmos_sensor(0xff, 0x0a);
	n6_write_cmos_sensor(0x25, 0x10);
	n6_write_cmos_sensor(0x27, 0x1e);
	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);
	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);
	n6_write_cmos_sensor(0x34, 0x40);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);
	n6_write_cmos_sensor(0xa5, 0x10);
	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);

	n6_write_cmos_sensor(0xff, 0x0b);
	n6_write_cmos_sensor(0x25, 0x10);
	n6_write_cmos_sensor(0x27, 0x1e);
	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);
	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);
	n6_write_cmos_sensor(0x34, 0x40);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);
	n6_write_cmos_sensor(0xa5, 0x10);
	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);

	n6_write_cmos_sensor(0xff, 0x13);
	n6_write_cmos_sensor(0x05, 0xa0);
	n6_write_cmos_sensor(0x31, 0xff);
	n6_write_cmos_sensor(0x07, 0x47);
	n6_write_cmos_sensor(0x12, 0x04);
	n6_write_cmos_sensor(0x1e, 0x1f);
	n6_write_cmos_sensor(0x1f, 0x27);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	n6_write_cmos_sensor(0x01, 0x00);
	n6_write_cmos_sensor(0x10, 0xff);
	n6_write_cmos_sensor(0x11, 0xff);
	n6_write_cmos_sensor(0x12, 0xc0);
	n6_write_cmos_sensor(0x13, 0x03);
	n6_write_cmos_sensor(0x14, 0xc0);
	n6_write_cmos_sensor(0x15, 0x03);

	n6_write_cmos_sensor(0x16, 0xc0);
	n6_write_cmos_sensor(0x17, 0x03);
	n6_write_cmos_sensor(0x18, 0xc0);
	n6_write_cmos_sensor(0x19, 0x03);
	n6_write_cmos_sensor(0x30, 0x0f);

	n6_write_cmos_sensor(0x34, 0xcd);
	n6_write_cmos_sensor(0x38, 0xff);
	n6_write_cmos_sensor(0x3c, 0x01);
	n6_write_cmos_sensor(0x3d, 0x11);
	n6_write_cmos_sensor(0x3e, 0x11);
	n6_write_cmos_sensor(0x45, 0x60);
	n6_write_cmos_sensor(0x46, 0x49);

	n6_write_cmos_sensor(0xff, _MTX_BANK_);
	n6_write_cmos_sensor(0x01, 0xe4);
	n6_write_cmos_sensor(0x00, 0x7d);
	n6_write_cmos_sensor(0x01, 0xe0);
	n6_write_cmos_sensor(0x02, 0xa0);
	n6_write_cmos_sensor(0x20, 0x1e);
	n6_write_cmos_sensor(0x20, 0x1f);
	n6_write_cmos_sensor(0x03, 0x02);
	n6_write_cmos_sensor(0x04, 0x6c);
	n6_write_cmos_sensor(0x45, 0xcd);
	n6_write_cmos_sensor(0x46, 0x42);
	n6_write_cmos_sensor(0x47, 0x36);
	n6_write_cmos_sensor(0x65, 0xcd);
	n6_write_cmos_sensor(0x66, 0x42);
	n6_write_cmos_sensor(0x67, 0x0e);
	n6_write_cmos_sensor(0x85, 0xcd);
	n6_write_cmos_sensor(0x86, 0x42);
	n6_write_cmos_sensor(0x87, 0x0e);
	n6_write_cmos_sensor(0xa5, 0xcd);
	n6_write_cmos_sensor(0xa6, 0x42);
	n6_write_cmos_sensor(0xa7, 0x0e);
	n6_write_cmos_sensor(0xc5, 0xcd);
	n6_write_cmos_sensor(0xc6, 0x42);
	n6_write_cmos_sensor(0xc7, 0x0e);
	n6_write_cmos_sensor(0xe9, 0x03);
	n6_write_cmos_sensor(0xeb, 0x8d);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x97, 0x00);
	n6_write_cmos_sensor(0x97, 0x0f);
	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	n6_write_cmos_sensor(0x00, 0xff);
	n6_write_cmos_sensor(0x40, 0x01);
	n6_write_cmos_sensor(0x40, 0x00);

	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x78, 0xab);
	n6_write_cmos_sensor(0x79, 0xca);
	n6_write_cmos_sensor(0xff, 0x05);
	n6_write_cmos_sensor(0x2c, 0x08);
	n6_write_cmos_sensor(0x6a, 0x80);
}

static void n6_common_settings(void)
{
	unsigned char ch = 0;;
	LOG_INF("n6_common_settings ENTER\n");
	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x80, 0x0f);
	n6_write_cmos_sensor(0x00, 0x10);
	n6_write_cmos_sensor(0x01, 0x10);
	n6_write_cmos_sensor(0x02, 0x10);
	n6_write_cmos_sensor(0x03, 0x10);
	n6_write_cmos_sensor(0x22, 0x0b);
	n6_write_cmos_sensor(0x23, 0x41);
	n6_write_cmos_sensor(0x26, 0x0b);
	n6_write_cmos_sensor(0x27, 0x41);
	n6_write_cmos_sensor(0x2a, 0x0b);
	n6_write_cmos_sensor(0x2b, 0x41);
	n6_write_cmos_sensor(0x2e, 0x0b);
	n6_write_cmos_sensor(0x2f, 0x41);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x98, 0x30);
	n6_write_cmos_sensor(0xed, 0x00);

	for(ch=0;ch<4;ch++)
	{
		n6_write_cmos_sensor(0xff, 0x05+ch);
		n6_write_cmos_sensor(0x00, 0xd0);
		n6_write_cmos_sensor(0x01, 0x22);
		n6_write_cmos_sensor(0x47, 0xee);
		n6_write_cmos_sensor(0x50, 0xc6);
		n6_write_cmos_sensor(0x57, 0x00);
		n6_write_cmos_sensor(0x58, 0x77);
		n6_write_cmos_sensor(0x5b, 0x41);
		n6_write_cmos_sensor(0x5c, 0x7C);
		n6_write_cmos_sensor(0xB8, 0xB8);
	}

	n6_write_cmos_sensor(0xff, 0x09);
	n6_write_cmos_sensor(0x50, 0x30);
	n6_write_cmos_sensor(0x51, 0x6f);
	n6_write_cmos_sensor(0x52, 0x67);
	n6_write_cmos_sensor(0x53, 0x48);
	n6_write_cmos_sensor(0x54, 0x30);
	n6_write_cmos_sensor(0x55, 0x6f);
	n6_write_cmos_sensor(0x56, 0x67);
	n6_write_cmos_sensor(0x57, 0x48);
	n6_write_cmos_sensor(0x58, 0x30);
	n6_write_cmos_sensor(0x59, 0x6f);
	n6_write_cmos_sensor(0x5a, 0x67);
	n6_write_cmos_sensor(0x5b, 0x48);
	n6_write_cmos_sensor(0x5c, 0x30);
	n6_write_cmos_sensor(0x5d, 0x6f);
	n6_write_cmos_sensor(0x5e, 0x67);
	n6_write_cmos_sensor(0x5f, 0x48);

	n6_write_cmos_sensor(0xff, 0x0a);
	n6_write_cmos_sensor(0x25, 0x10);
	n6_write_cmos_sensor(0x27, 0x1e);
	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);
	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);
	n6_write_cmos_sensor(0x34, 0x40);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);
	n6_write_cmos_sensor(0xa5, 0x10);
	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);
	n6_write_cmos_sensor(0xff, 0x0b);
	n6_write_cmos_sensor(0x25, 0x10);
	n6_write_cmos_sensor(0x27, 0x1e);
	n6_write_cmos_sensor(0x30, 0xac);
	n6_write_cmos_sensor(0x31, 0x78);
	n6_write_cmos_sensor(0x32, 0x17);
	n6_write_cmos_sensor(0x33, 0xc1);
	n6_write_cmos_sensor(0x34, 0x40);
	n6_write_cmos_sensor(0x35, 0x00);
	n6_write_cmos_sensor(0x36, 0xc3);
	n6_write_cmos_sensor(0x37, 0x0a);
	n6_write_cmos_sensor(0x38, 0x00);
	n6_write_cmos_sensor(0x39, 0x02);
	n6_write_cmos_sensor(0x3a, 0x00);
	n6_write_cmos_sensor(0x3b, 0xb2);
	n6_write_cmos_sensor(0xa5, 0x10);
	n6_write_cmos_sensor(0xa7, 0x1e);
	n6_write_cmos_sensor(0xb0, 0xac);
	n6_write_cmos_sensor(0xb1, 0x78);
	n6_write_cmos_sensor(0xb2, 0x17);
	n6_write_cmos_sensor(0xb3, 0xc1);
	n6_write_cmos_sensor(0xb4, 0x40);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb6, 0xc3);
	n6_write_cmos_sensor(0xb7, 0x0a);
	n6_write_cmos_sensor(0xb8, 0x00);
	n6_write_cmos_sensor(0xb9, 0x02);
	n6_write_cmos_sensor(0xba, 0x00);
	n6_write_cmos_sensor(0xbb, 0xb2);


	n6_write_cmos_sensor(0xff, 0x13);
	n6_write_cmos_sensor(0x05, 0xa0);
	n6_write_cmos_sensor(0x07, 0x47);
	n6_write_cmos_sensor(0x12, 0x04);
	n6_write_cmos_sensor(0x1e, 0x1f);
	n6_write_cmos_sensor(0x1f, 0x27);
	n6_write_cmos_sensor(0x2e, 0x10);
	n6_write_cmos_sensor(0x2f, 0xc8);
	n6_write_cmos_sensor(0x31, 0xff);
	n6_write_cmos_sensor(0x32, 0x00);
	n6_write_cmos_sensor(0x33, 0x00);
	n6_write_cmos_sensor(0x72, 0x05);
	n6_write_cmos_sensor(0x7a, 0xf0);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	n6_write_cmos_sensor(0x10, 0xff);
	n6_write_cmos_sensor(0x11, 0xff);
	n6_write_cmos_sensor(0x30, 0x0f);
	n6_write_cmos_sensor(0x32, 0xff);
	n6_write_cmos_sensor(0x34, 0xcd);
	n6_write_cmos_sensor(0x36, 0x04);
	n6_write_cmos_sensor(0x38, 0xff);

	n6_write_cmos_sensor(0x3c, 0x01);
	n6_write_cmos_sensor(0x3d, 0x11);
	n6_write_cmos_sensor(0x3e, 0x11);
	n6_write_cmos_sensor(0x45, 0x60);
	n6_write_cmos_sensor(0x46, 0x49);

	n6_write_cmos_sensor(0xff, _MTX_BANK_);
	n6_write_cmos_sensor(0xe9, 0x03);
	n6_write_cmos_sensor(0x03, 0x02);
	n6_write_cmos_sensor(0x01, 0xe4);
	n6_write_cmos_sensor(0x00, 0x7d);
	n6_write_cmos_sensor(0x01, 0xe0);
	n6_write_cmos_sensor(0x02, 0xa0);
	n6_write_cmos_sensor(0x20, 0x1e);
	n6_write_cmos_sensor(0x20, 0x1f);
	n6_write_cmos_sensor(0x04, 0x6c);
	n6_write_cmos_sensor(0x45, 0xcd);
	n6_write_cmos_sensor(0x46, 0x42);
	n6_write_cmos_sensor(0x47, 0x36);
	n6_write_cmos_sensor(0x48, 0x0f);
	n6_write_cmos_sensor(0x65, 0xcd);
	n6_write_cmos_sensor(0x66, 0x42);
	n6_write_cmos_sensor(0x67, 0x0e);
	n6_write_cmos_sensor(0x68, 0x0f);
	n6_write_cmos_sensor(0x85, 0xcd);
	n6_write_cmos_sensor(0x86, 0x42);
	n6_write_cmos_sensor(0x87, 0x0e);
	n6_write_cmos_sensor(0x88, 0x0f);
	n6_write_cmos_sensor(0xa5, 0xcd);
	n6_write_cmos_sensor(0xa6, 0x42);
	n6_write_cmos_sensor(0xa7, 0x0e);
	n6_write_cmos_sensor(0xa8, 0x0f);
	n6_write_cmos_sensor(0xc5, 0xcd);
	n6_write_cmos_sensor(0xc6, 0x42);
	n6_write_cmos_sensor(0xc7, 0x0e);
	n6_write_cmos_sensor(0xc8, 0x0f);
	n6_write_cmos_sensor(0xeb, 0x8d);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	//n6_write_cmos_sensor(0x00, 0xff);
	n6_write_cmos_sensor(0x00, 0x00);  //first turn off mipi outputs
	n6_write_cmos_sensor(0x40, 0x01);
	n6_write_cmos_sensor(0x40, 0x00);
	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x97, 0x00);
	n6_write_cmos_sensor(0x97, 0x0f);

	n6_write_cmos_sensor(0xff, 0x00);  //test pattern
	n6_write_cmos_sensor(0x78, 0xba);
	n6_write_cmos_sensor(0x79, 0xac);
	n6_write_cmos_sensor(0xff, 0x05);
	n6_write_cmos_sensor(0x2c, 0x08);
	n6_write_cmos_sensor(0x6a, 0x80);
	n6_write_cmos_sensor(0xff, 0x06);
	n6_write_cmos_sensor(0x2c, 0x08);
	n6_write_cmos_sensor(0x6a, 0x80);
	n6_write_cmos_sensor(0xff, 0x07);
	n6_write_cmos_sensor(0x2c, 0x08);
	n6_write_cmos_sensor(0x6a, 0x80);
	n6_write_cmos_sensor(0xff, 0x08);
	n6_write_cmos_sensor(0x2c, 0x08);
	n6_write_cmos_sensor(0x6a, 0x80);
}

void n6_set_chn_960h(unsigned char ch, unsigned char ntpal)
{
	unsigned char val_0x54, val_20x01;
	printk("[%s::%d]\n", __func__, __LINE__);
	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x08+ch, ntpal?0xdd:0xa0);
	n6_write_cmos_sensor(0x18+ch, 0x08);
	n6_write_cmos_sensor(0x22+ch*4, 0x0b);
	n6_write_cmos_sensor(0x23+ch*4, 0x41);
	n6_write_cmos_sensor(0x30+ch, 0x12);
	n6_write_cmos_sensor(0x34+ch, 0x01);
	val_0x54 = n6_read_cmos_sensor(0x54);
	if(ntpal)
		val_0x54 &= ~(0x10<<ch);
	else
		val_0x54 |= (0x10<<ch);
	n6_write_cmos_sensor(0x54, val_0x54);
	n6_write_cmos_sensor(0x58+ch, ntpal?0x80:0x90);
	n6_write_cmos_sensor(0x5c+ch, ntpal?0xbe:0xbc);
	n6_write_cmos_sensor(0x64+ch, ntpal?0xa0:0x81);
	n6_write_cmos_sensor(0x81+ch, ntpal?0xf0:0xe0);
	n6_write_cmos_sensor(0x85+ch, 0x00);
	n6_write_cmos_sensor(0x89+ch, 0x00);
	n6_write_cmos_sensor(ch+0x8e, 0x00);
	n6_write_cmos_sensor(0xa0+ch, 0x05);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x84+ch, 0x02);
	n6_write_cmos_sensor(0x88+ch, 0x00);
	n6_write_cmos_sensor(0x8c+ch, 0x40);
	n6_write_cmos_sensor(0xa0+ch, 0x20);
	n6_write_cmos_sensor(0xed, 0x00);

	n6_write_cmos_sensor(0xff, 0x05+ch);
	n6_write_cmos_sensor(0x01, 0x22);
	n6_write_cmos_sensor(0x05, 0x00);
	n6_write_cmos_sensor(0x08, 0x55);
	n6_write_cmos_sensor(0x25, 0xdc);
	n6_write_cmos_sensor(0x28, 0x80);
	n6_write_cmos_sensor(0x2f, 0x00);
	n6_write_cmos_sensor(0x30, 0xe0);
	n6_write_cmos_sensor(0x31, 0x43);
	n6_write_cmos_sensor(0x32, 0xa2);
	n6_write_cmos_sensor(0x47, 0x04);
	n6_write_cmos_sensor(0x50, 0x84);
	n6_write_cmos_sensor(0x57, 0x00);
	n6_write_cmos_sensor(0x58, 0x77);
	n6_write_cmos_sensor(0x5b, 0x43);
	n6_write_cmos_sensor(0x5c, 0x78);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x62, 0x20);
	n6_write_cmos_sensor(0x7b, 0x00);
	n6_write_cmos_sensor(0x7c, 0x01);
	n6_write_cmos_sensor(0x7d, 0x80);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x90, 0x01);
	n6_write_cmos_sensor(0xa9, 0x00);
	n6_write_cmos_sensor(0xb5, 0x00);
	n6_write_cmos_sensor(0xb8, 0xb9);
	n6_write_cmos_sensor(0xb9, 0x72);
	n6_write_cmos_sensor(0xd1, 0x00);
	n6_write_cmos_sensor(0xd5, 0x80);

	n6_write_cmos_sensor(0xff, 0x09);
	n6_write_cmos_sensor(0x96+ch*0x20, 0x10);
	n6_write_cmos_sensor(0x98+ch*0x20, ntpal?0xc0:0xe0);

	n6_write_cmos_sensor(ch*0x20+0x9e, 0x00);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	val_20x01 = n6_read_cmos_sensor(0x01);
	val_20x01 &=(~(0x03<<(ch*2)));
	val_20x01 |=(0x02<<(ch*2));
	n6_write_cmos_sensor(0x01, val_20x01);
	n6_write_cmos_sensor(0x12+ch*2, 0xe0);
	n6_write_cmos_sensor(0x13+ch*2, 0x01);


}

void n6_set_chn_720p(unsigned char ch, unsigned char ntpal)
{
	unsigned char val_0x54, val_20x01;
	printk("[%s::%d]\n", __func__, __LINE__);
	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x08+ch, 0x00);
	n6_write_cmos_sensor(0x18+ch, 0x3f);
	n6_write_cmos_sensor(0x30+ch, 0x12);
	n6_write_cmos_sensor(0x34+ch, 0x00);
	val_0x54 = n6_read_cmos_sensor(0x54);
	val_0x54 &= ~(0x10<<ch);
	n6_write_cmos_sensor(0x54, val_0x54);
	n6_write_cmos_sensor(0x58+ch, ntpal?0x80:0x80);
	n6_write_cmos_sensor(0x5c+ch, ntpal?0x00:0x00);
	n6_write_cmos_sensor(0x64+ch, ntpal?0x01:0x01);
	n6_write_cmos_sensor(0x81+ch, ntpal?0x0d:0x0c);
	n6_write_cmos_sensor(0x85+ch, 0x00);
	n6_write_cmos_sensor(0x89+ch, 0x00);
	n6_write_cmos_sensor(ch+0x8e, 0x00);
	n6_write_cmos_sensor(0xa0+ch, 0x05);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x84+ch, 0x02);
	n6_write_cmos_sensor(0x88+ch, 0x00);
	n6_write_cmos_sensor(0x8c+ch, 0x40);
	n6_write_cmos_sensor(0xa0+ch, 0x20);

	n6_write_cmos_sensor(0xff, 0x05+ch);
	n6_write_cmos_sensor(0x01, 0x22);
	n6_write_cmos_sensor(0x05, 0x04);
	n6_write_cmos_sensor(0x08, 0x55);
	n6_write_cmos_sensor(0x25, 0xdc);
	n6_write_cmos_sensor(0x28, 0x80);
	n6_write_cmos_sensor(0x2f, 0x00);
	n6_write_cmos_sensor(0x30, 0xe0);
	n6_write_cmos_sensor(0x31, 0x43);
	n6_write_cmos_sensor(0x32, 0xa2);
	n6_write_cmos_sensor(0x47, 0xee);
	n6_write_cmos_sensor(0x50, 0xc6);
	n6_write_cmos_sensor(0x57, 0x00);
	n6_write_cmos_sensor(0x58, 0x77);
	n6_write_cmos_sensor(0x5b, 0x41);
	n6_write_cmos_sensor(0x5c, 0x7C);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x62, 0x20);
	n6_write_cmos_sensor(0x7b, 0x11);
	n6_write_cmos_sensor(0x7c, 0x01);
	n6_write_cmos_sensor(0x7d, 0x80);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x90, 0x01);
	n6_write_cmos_sensor(0xa9, 0x00);
	n6_write_cmos_sensor(0xb5, 0x40);
	n6_write_cmos_sensor(0xb8, 0x39);
	n6_write_cmos_sensor(0xb9, 0x72);
	n6_write_cmos_sensor(0xd1, 0x00);
	n6_write_cmos_sensor(0xd5, 0x80);

	n6_write_cmos_sensor(0xff, 0x09);
	n6_write_cmos_sensor(0x96+ch*0x20, 0x00);
	n6_write_cmos_sensor(0x98+ch*0x20, 0x00);
	n6_write_cmos_sensor(ch*0x20+0x9e, 0x00);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	val_20x01 = n6_read_cmos_sensor(0x01);
	val_20x01 &=(~(0x03<<(ch*2)));
	val_20x01 |=(0x01<<(ch*2));
	n6_write_cmos_sensor(0x01, val_20x01);
	n6_write_cmos_sensor(0x12+ch*2, 0x80);
	n6_write_cmos_sensor(0x13+ch*2, 0x02);

}

void n6_set_chn_960p(unsigned char ch, unsigned char ntpal)
{
	unsigned char val_0x54, val_20x01;
	printk("[%s::%d]\n", __func__, __LINE__);
	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x08+ch, 0x00);
	n6_write_cmos_sensor(0x18+ch, 0x0f);
	n6_write_cmos_sensor(0x30+ch, 0x12);
	n6_write_cmos_sensor(0x34+ch, 0x00);
	val_0x54 = n6_read_cmos_sensor(0x54);
	val_0x54 &= ~(0x10<<ch);
	n6_write_cmos_sensor(0x54, val_0x54);
	n6_write_cmos_sensor(0x58+ch, ntpal?0x40:0x48);
	n6_write_cmos_sensor(0x5c+ch, ntpal?0x80:0x80);
	n6_write_cmos_sensor(0x64+ch, ntpal?0x28:0x28);
	n6_write_cmos_sensor(0x81+ch, ntpal?0x07:0x06);
	n6_write_cmos_sensor(0x85+ch, 0x0b);
	n6_write_cmos_sensor(0x89+ch, 0x00);
	n6_write_cmos_sensor(ch+0x8e, 0x00);
	n6_write_cmos_sensor(0xa0+ch, 0x05);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x84+ch, 0x02);
	n6_write_cmos_sensor(0x88+ch, 0x00);
	n6_write_cmos_sensor(0x8c+ch, 0x40);
	n6_write_cmos_sensor(0xa0+ch, 0x20);

	n6_write_cmos_sensor(0xff, 0x05+ch);
	n6_write_cmos_sensor(0x01, 0x22);
	n6_write_cmos_sensor(0x05, 0x04);
	n6_write_cmos_sensor(0x08, 0x55);
	n6_write_cmos_sensor(0x25, 0xdc);
	n6_write_cmos_sensor(0x28, 0x80);
	n6_write_cmos_sensor(0x2f, 0x00);
	n6_write_cmos_sensor(0x30, 0xe0);
	n6_write_cmos_sensor(0x31, 0x43);
	n6_write_cmos_sensor(0x32, 0xa2);
	n6_write_cmos_sensor(0x47, 0xee);
	n6_write_cmos_sensor(0x50, 0xc6);
	n6_write_cmos_sensor(0x57, 0x00);
	n6_write_cmos_sensor(0x58, 0x77);
	n6_write_cmos_sensor(0x5b, 0x41);
	n6_write_cmos_sensor(0x5c, 0x7C);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x62, 0x20);
	n6_write_cmos_sensor(0x7b, 0x11);
	n6_write_cmos_sensor(0x7c, 0x01);
	n6_write_cmos_sensor(0x7d, 0x80);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x90, 0x01);
	n6_write_cmos_sensor(0xa9, 0x00);
	n6_write_cmos_sensor(0xb5, 0x40);
	n6_write_cmos_sensor(0xb8, 0x39);
	n6_write_cmos_sensor(0xb9, 0x72);
	n6_write_cmos_sensor(0xd1, 0x00);
	n6_write_cmos_sensor(0xd5, 0x80);

	n6_write_cmos_sensor(0xff, 0x09);
	n6_write_cmos_sensor(0x96+ch*0x20, 0x00);
	n6_write_cmos_sensor(0x98+ch*0x20, 0x00);
	n6_write_cmos_sensor(ch*0x20+0x9e, 0x00);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	val_20x01 = n6_read_cmos_sensor(0x01);
	val_20x01 &=(~(0x03<<(ch*2)));
	n6_write_cmos_sensor(0x01, val_20x01);
	n6_write_cmos_sensor(0x12+ch*2, 0x80);
	n6_write_cmos_sensor(0x13+ch*2, 0x02);

}

void n6_set_chn_1080p(unsigned char ch, unsigned char ntpal)
{
	unsigned char val_0x54, val_20x01;
	printk("[%s::%d]\n", __func__, __LINE__);
	n6_write_cmos_sensor(0xff, 0x00);
	n6_write_cmos_sensor(0x08+ch, 0x00);
	n6_write_cmos_sensor(0x18+ch, 0x3f);
	n6_write_cmos_sensor(0x30+ch, 0x12);
	n6_write_cmos_sensor(0x34+ch, 0x00);
	val_0x54 = n6_read_cmos_sensor(0x54);
	val_0x54 &= ~(0x10<<ch);
	n6_write_cmos_sensor(0x54, val_0x54);
	n6_write_cmos_sensor(0x58+ch, ntpal?0x80:0x80);
	n6_write_cmos_sensor(0x5c+ch, ntpal?0x00:0x00);
	n6_write_cmos_sensor(0x64+ch, ntpal?0x01:0x01);
	n6_write_cmos_sensor(0x81+ch, ntpal?0x03:0x02);
	n6_write_cmos_sensor(0x85+ch, 0x00);
	n6_write_cmos_sensor(0x89+ch, 0x10);
	n6_write_cmos_sensor(ch+0x8e, 0x00);
	n6_write_cmos_sensor(0xa0+ch, 0x05);

	n6_write_cmos_sensor(0xff, 0x01);
	n6_write_cmos_sensor(0x84+ch, 0x02);
	n6_write_cmos_sensor(0x88+ch, 0x00);
	n6_write_cmos_sensor(0x8c+ch, 0x40);
	n6_write_cmos_sensor(0xa0+ch, 0x20);

	n6_write_cmos_sensor(0xff, 0x05+ch);
	n6_write_cmos_sensor(0x01, 0x22);
	n6_write_cmos_sensor(0x05, 0x04);
	n6_write_cmos_sensor(0x08, 0x55);
	n6_write_cmos_sensor(0x25, 0xdc);
	n6_write_cmos_sensor(0x28, 0x80);
	n6_write_cmos_sensor(0x2f, 0x00);
	n6_write_cmos_sensor(0x30, 0xe0);
	n6_write_cmos_sensor(0x31, 0x41);
	n6_write_cmos_sensor(0x32, 0xa2);
	n6_write_cmos_sensor(0x47, 0xee);
	n6_write_cmos_sensor(0x50, 0xc6);
	n6_write_cmos_sensor(0x57, 0x00);
	n6_write_cmos_sensor(0x58, 0x77);
	n6_write_cmos_sensor(0x5b, 0x41);
	n6_write_cmos_sensor(0x5c, 0x7C);
	n6_write_cmos_sensor(0x5f, 0x00);
	n6_write_cmos_sensor(0x62, 0x20);
	n6_write_cmos_sensor(0x7b, 0x11);
	n6_write_cmos_sensor(0x7c, 0x01);
	n6_write_cmos_sensor(0x7d, 0x80);
	n6_write_cmos_sensor(0x80, 0x00);
	n6_write_cmos_sensor(0x90, 0x01);
	n6_write_cmos_sensor(0xa9, 0x00);
	n6_write_cmos_sensor(0xb5, 0x40);
	n6_write_cmos_sensor(0xb8, 0x39);
	n6_write_cmos_sensor(0xb9, 0x72);
	n6_write_cmos_sensor(0xd1, 0x00);
	n6_write_cmos_sensor(0xd5, 0x80);

	n6_write_cmos_sensor(0xff, 0x09);
	n6_write_cmos_sensor(0x96+ch*0x20, 0x00);
	n6_write_cmos_sensor(0x98+ch*0x20, 0x00);
	n6_write_cmos_sensor(ch*0x20+0x9e, 0x00);

	n6_write_cmos_sensor(0xff, _MAR_BANK_);
	val_20x01 = n6_read_cmos_sensor(0x01);
	val_20x01 &=(~(0x03<<(ch*2)));
	n6_write_cmos_sensor(0x01, val_20x01);
	n6_write_cmos_sensor(0x12+ch*2, 0xc0);
	n6_write_cmos_sensor(0x13+ch*2, 0x03);
}

static void n6_init_ch(int ch, int vfmt)
{
	LOG_INF("int ch = %d,vfmt = 0x%x\n",ch,vfmt);
	switch (vfmt) {
		case CH_VALUE_NTSC:
			n6_set_chn_960h(ch, 0);
		break;
		case CH_VALUE_PAL:
			n6_set_chn_960h(ch, 1);
		break;
		case CH_VALUE_HD30:
			n6_set_chn_720p(ch, 0);
		break;
		case CH_VALUE_HD25:
			n6_set_chn_720p(ch, 1);
		break;
		case CH_VALUE_FHD30:
			n6_set_chn_1080p(ch, 0);
		break;
		case CH_VALUE_FHD25:
			n6_set_chn_1080p(ch, 1);
		break;
		default:
			LOG_INF("err!!!ch:%d not support vfmt=%d yet\n",ch,vfmt);
		break;
	//	n6_write_cmos_sensor(0xa0+ch, 0x00);
	}

	}
void n6_mipi_outputs(int mipi_ch_en)
{
	LOG_INF("n6_mipi_outputs mipi_ch_en = 0x%x\n",mipi_ch_en);
	n6_write_cmos_sensor(0xFF, 0x01);
	n6_write_cmos_sensor(0x97, 0x00);
	n6_write_cmos_sensor(0x97, 0x0f);
	n6_write_cmos_sensor(0xFF, _MAR_BANK_);
	n6_write_cmos_sensor(0x00, mipi_ch_en);  //ch1/2/3/4 enabled FF
	n6_write_cmos_sensor(0x40, 0x01);
	n6_write_cmos_sensor(0x40, 0x00);

	}

static int n6_detect_format_avm(int ch)
{
	int temp_value = 0;
	int retry = 20;
	return 0x20;
	n6_write_cmos_sensor(0xff,0x13);
	n6_write_cmos_sensor(0x30,0x7f);
	n6_write_cmos_sensor(0x31,0xFF);
	n6_write_cmos_sensor(0x32,0x00);
	n6_write_cmos_sensor(0x33,0x00);
	n6_write_cmos_sensor(0x70,0xf0);
	n6_write_cmos_sensor(0xff,0x00);
	n6_write_cmos_sensor(0x00+(ch%4),0x18);
	n6_write_cmos_sensor(0x00+(ch%4),0x10);
	n6_write_cmos_sensor(0x23+(ch%4)*4,0x41);
	n6_write_cmos_sensor(0xff,0x01);
	n6_write_cmos_sensor(0x8c+(ch%4),0x40);
	n6_write_cmos_sensor(0xff,0x05+(ch%4));
	n6_write_cmos_sensor(0x00,0xF0);
	n6_write_cmos_sensor(0x01,0x22);
	n6_write_cmos_sensor(0x57,0x00);
	n6_write_cmos_sensor(0x58,0x47);
	n6_write_cmos_sensor(0x5c,0x7f);
	n6_write_cmos_sensor(0xB8,0xB8);

	n6_write_cmos_sensor(0xFF,0x05+ch%4);
	do {
		temp_value = n6_read_cmos_sensor(0xF0);
		LOG_INF("ch = %d,value = 0x%x\n",ch,temp_value);
		if (0xf != (temp_value & 0xf)) {
			LOG_INF("n6 Channel = %d format is 0x%x,retry = %d\n",ch,temp_value,retry);
			break;
}
		retry --;
		mdelay(40);
	} while (retry > 0);
	n6_write_cmos_sensor(0xff,0x13);
	n6_write_cmos_sensor(0x30,0x00);
	n6_write_cmos_sensor(0x70,0x00);
	return temp_value;
}

static UINT32 n6_getsensorid(uint32_t *sensorId)
{
	int retry = 3;
	do {
		n6_write_cmos_sensor(0xff,0x00);
		*sensorId = ((n6_read_cmos_sensor(0xf4) << 8) | n6_read_cmos_sensor(0xf5));
		LOG_INF("n6_GetSensorID: sensor id = 0x%x\n", *sensorId);
		if (*sensorId == SENSOR_ID)  //N6??id ?? 0xD300
			break;
		retry--;
	} while (retry > 0);

	if (*sensorId != SENSOR_ID)
	{
		*sensorId = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	return ERROR_NONE;
}

static UINT32 n6_preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&n6SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

static UINT32 n6_capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&n6SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	LOG_INF("Leave\n");
	return ERROR_NONE;
}

static UINT32 n6_getresolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	LOG_INF("w = %d,h=%d\n",input_source_width,input_source_height);
	pSensorResolution->SensorFullWidth = input_source_width;
	pSensorResolution->SensorFullHeight = input_source_height;
	pSensorResolution->SensorPreviewWidth = input_source_width;
	pSensorResolution->SensorPreviewHeight = input_source_height;
	pSensorResolution->SensorVideoWidth = input_source_width;
	pSensorResolution->SensorVideoHeight = input_source_height;
	pSensorResolution->SensorCh0Width = input_source_width_ch0;
	pSensorResolution->SensorCh0Height = input_source_height_ch0;
	pSensorResolution->SensorCh1Width = input_source_width_ch1;
	pSensorResolution->SensorCh1Height = input_source_height_ch1;
	pSensorResolution->SensorCh2Width = input_source_width_ch2;
	pSensorResolution->SensorCh2Height = input_source_height_ch2;
	pSensorResolution->SensorCh3Width = input_source_width_ch3;
	pSensorResolution->SensorCh3Height = input_source_height_ch3;

	return ERROR_NONE;
}

SIGNAL_STATUS n6_get_signalstatus(int ch)
{
	int value = 0xff;
	SIGNAL_STATUS signal_status = SIGNAL_IDLE;
	return SIGNAL_READY;

	n6_write_cmos_sensor(0xff, 0x00);

	value = n6_read_cmos_sensor(0xa8);

	if ((value & (0x01 << ch)) == 0x00 )
	{
		signal_status = SIGNAL_READY;
	}
	else
	{
		signal_status = SIGNAL_READY;
	}

	//LOG_INF("get signal status leave and current ch is %d,signal status is %d,value = 0x%x\n",ch,signal_status,value);

	return signal_status;

}

static int n6_detect_format(int ch)
{
	int temp_value = CH_VALUE_NTSC;
#if 0
	int retry = 50;
	n6_write_cmos_sensor(0xff,0x13);
	n6_write_cmos_sensor(0x30,0x7f);
	n6_write_cmos_sensor(0x31,0xff);
	n6_write_cmos_sensor(0x32,0x00);
	n6_write_cmos_sensor(0x33,0x00);
	n6_write_cmos_sensor(0x70,0xf0);
	n6_write_cmos_sensor(0xff,0x00);

	n6_write_cmos_sensor(0x00+(ch%4),0x18);
	n6_write_cmos_sensor(0x00+(ch%4),0x10);
	n6_write_cmos_sensor(0x23+(ch%4)*4,0x41);



	n6_write_cmos_sensor(0xff,0x01);
	n6_write_cmos_sensor(0x8c+(ch%4),0x40);
	n6_write_cmos_sensor(0xff,0x05+(ch%4));
	n6_write_cmos_sensor(0x00,0xF0);
	n6_write_cmos_sensor(0x01,0x22);
	n6_write_cmos_sensor(0x57,0x00);
	n6_write_cmos_sensor(0x58,0x47);
	n6_write_cmos_sensor(0x5c,0x7f);
	n6_write_cmos_sensor(0xB8,0xB8);

	n6_write_cmos_sensor(0xFF,0x05+ch%4);
	do {
		temp_value = n6_read_cmos_sensor(0xF0);
		LOG_INF("ch = %d,value = 0x%x\n",ch,temp_value);
		if (0xf != (temp_value & 0xf)) {
			LOG_INF("n6 Channel = %d format is 0x%x,retry = %d\n",ch,temp_value,retry);
			break;
		}
		retry --;
		mdelay(40);
	} while (retry > 0);
	n6_write_cmos_sensor(0xff,0x13);
	n6_write_cmos_sensor(0x30,0x00);
	n6_write_cmos_sensor(0x70,0x00);
#endif
	switch (ch) {
	case 0:
		switch (temp_value) {
		case CH_VALUE_NTSC:
			input_source_width_ch0 = N6_IMAGE_SENSOR_NTSC_WIDTH;
			input_source_height_ch0 = N6_IMAGE_SENSOR_NTSC_HEIGHT;
			break;
		case CH_VALUE_PAL:
			input_source_width_ch0 = N6_IMAGE_SENSOR_PAL_WIDTH;
			input_source_height_ch0 = N6_IMAGE_SENSOR_PAL_HEIGHT;
			break;
		case CH_VALUE_HD25:
			input_source_width_ch0 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch0 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		case CH_VALUE_HD30:
			input_source_width_ch0 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch0 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		case CH_VALUE_FHD25:
			input_source_width_ch0 = N6_IMAGE_SENSOR_FHD_WIDTH;
			input_source_height_ch0 = N6_IMAGE_SENSOR_FHD_HEIGHT;
			break;
		case CH_VALUE_FHD30:
			input_source_width_ch0 = N6_IMAGE_SENSOR_FHD_WIDTH;
			input_source_height_ch0 = N6_IMAGE_SENSOR_FHD_HEIGHT;
			break;
		default:
			input_source_width_ch0 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch0 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		}
		break;
	case 1:
		switch (temp_value) {
		case CH_VALUE_NTSC:
			input_source_width_ch1 = N6_IMAGE_SENSOR_NTSC_WIDTH;
			input_source_height_ch1 = N6_IMAGE_SENSOR_NTSC_HEIGHT;
			break;
		case CH_VALUE_PAL:
			input_source_width_ch1 = N6_IMAGE_SENSOR_PAL_WIDTH;
			input_source_height_ch1 = N6_IMAGE_SENSOR_PAL_HEIGHT;
			break;
		case CH_VALUE_HD25:
			input_source_width_ch1 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch1 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		case CH_VALUE_HD30:
			input_source_width_ch1 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch1 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		case CH_VALUE_FHD25:
			input_source_width_ch1 = N6_IMAGE_SENSOR_FHD_WIDTH;
			input_source_height_ch1 = N6_IMAGE_SENSOR_FHD_HEIGHT;
			break;
		case CH_VALUE_FHD30:
			input_source_width_ch1 = N6_IMAGE_SENSOR_FHD_WIDTH;
			input_source_height_ch1 = N6_IMAGE_SENSOR_FHD_HEIGHT;
			break;
		default:
			input_source_width_ch1 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch1 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		}
		break;
	case 2:
		switch (temp_value) {
		case CH_VALUE_NTSC:
			input_source_width_ch2 = N6_IMAGE_SENSOR_NTSC_WIDTH;
			input_source_height_ch2 = N6_IMAGE_SENSOR_NTSC_HEIGHT;
			break;
		case CH_VALUE_PAL:
			input_source_width_ch2 = N6_IMAGE_SENSOR_PAL_WIDTH;
			input_source_height_ch2 = N6_IMAGE_SENSOR_PAL_HEIGHT;
			break;
		case CH_VALUE_HD25:
			input_source_width_ch2 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch2 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		case CH_VALUE_HD30:
			input_source_width_ch2 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch2 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		case CH_VALUE_FHD25:
			input_source_width_ch2 = N6_IMAGE_SENSOR_FHD_WIDTH;
			input_source_height_ch2 = N6_IMAGE_SENSOR_FHD_HEIGHT;
			break;
		case CH_VALUE_FHD30:
			input_source_width_ch2 = N6_IMAGE_SENSOR_FHD_WIDTH;
			input_source_height_ch2 = N6_IMAGE_SENSOR_FHD_HEIGHT;
			break;
		default:
			input_source_width_ch2 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch2 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		}
		break;
	case 3:
		switch (temp_value) {
		case CH_VALUE_NTSC:
			input_source_width_ch3 = N6_IMAGE_SENSOR_NTSC_WIDTH;
			input_source_height_ch3 = N6_IMAGE_SENSOR_NTSC_HEIGHT;
			break;
		case CH_VALUE_PAL:
			input_source_width_ch3 = N6_IMAGE_SENSOR_PAL_WIDTH;
			input_source_height_ch3 = N6_IMAGE_SENSOR_PAL_HEIGHT;
			break;
		case CH_VALUE_HD25:
			input_source_width_ch3 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch3 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		case CH_VALUE_HD30:
			input_source_width_ch3 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch3 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		case CH_VALUE_FHD25:
			input_source_width_ch3 = N6_IMAGE_SENSOR_FHD_WIDTH;
			input_source_height_ch3 = N6_IMAGE_SENSOR_FHD_HEIGHT;
			break;
		case CH_VALUE_FHD30:
			input_source_width_ch3 = N6_IMAGE_SENSOR_FHD_WIDTH;
			input_source_height_ch3 = N6_IMAGE_SENSOR_FHD_HEIGHT;
			break;
		default:
			input_source_width_ch3 = N6_IMAGE_SENSOR_HD_WIDTH;
			input_source_height_ch3 = N6_IMAGE_SENSOR_HD_HEIGHT;
			break;
		}
		break;
	default:
		break;
	}
	return temp_value;
}
#if 0
static int n6_fastavm_metazone_handle(unsigned int value)
{
	LOG_INF("enter\n");
	switch (value) {

	case FORMAT_HD25:
		//n6_common_settings();
		n6_sensor_hd25_init();
		input_source_width = N6_IMAGE_SENSOR_HD_WIDTH;
		input_source_height = N6_IMAGE_SENSOR_HD_HEIGHT;
		break;
	case FORMAT_HD30:
		//n6_common_settings();
		n6_sensor_hd30_init();
		input_source_width = N6_IMAGE_SENSOR_HD_WIDTH;
		input_source_height = N6_IMAGE_SENSOR_HD_HEIGHT;
		break;
	case FORMAT_FHD25:
		//n6_common_settings();
		n6_sensor_fhd25_init();
		input_source_width = N6_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height = N6_IMAGE_SENSOR_FHD_HEIGHT;
		break;
	case FORMAT_FHD30:
		//n6_common_settings();
		n6_sensor_fhd30_init();
		input_source_width = N6_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height = N6_IMAGE_SENSOR_FHD_HEIGHT;
		break;
	default:
		return -1;
	}
	return ERROR_NONE;
}

#endif
static void n6_fastavm_metazone_invalid_handle(void)
{
	int n6_avm_fmt = CH_VALUE_HD30;

	LOG_INF("enter\n");
	//n6_common_settings();
	n6_avm_fmt = n6_detect_format_avm(0);

	switch (n6_avm_fmt) {
	case CH_VALUE_HD25:
		n6_sensor_hd25_init();
		input_source_width = N6_IMAGE_SENSOR_HD_WIDTH;
		input_source_height = N6_IMAGE_SENSOR_HD_HEIGHT;
		break;
	case CH_VALUE_HD30:
		n6_sensor_hd30_init();
		input_source_width = N6_IMAGE_SENSOR_HD_WIDTH;
		input_source_height = N6_IMAGE_SENSOR_HD_HEIGHT;
		break;
	case CH_VALUE_FHD25:
		n6_sensor_fhd25_init();
		input_source_width = N6_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height = N6_IMAGE_SENSOR_FHD_HEIGHT;
		break;
	case CH_VALUE_FHD30:
		n6_sensor_fhd30_init();
		input_source_width = N6_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height = N6_IMAGE_SENSOR_FHD_HEIGHT;
		break;
	default:
		LOG_ERR("can not detect valid format,uesd HD_30\n");
		n6_sensor_hd30_init();
		input_source_width = N6_IMAGE_SENSOR_HD_WIDTH;
		input_source_height = N6_IMAGE_SENSOR_HD_HEIGHT;
		break;
	}

}

UINT32 n6_open(void)
{

	UINT32 sensor_id = 0;
	int n6_format_ch0 = CH_VALUE_HD30;
	int n6_format_ch1 = CH_VALUE_HD30;
	int n6_format_ch2 = CH_VALUE_HD30;
	int n6_format_ch3 = CH_VALUE_HD30;
	int mipi_ch_en = 0xff;


	LOG_INF("n6Open 222 Enter\n");

	n6_getsensorid(&sensor_id);
	if (sensor_id != SENSOR_ID)
	{
		LOG_ERR("n6 YUV read sensor id failed\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}


	if (1 == n6_is_avm) {
		LOG_INF("not fast avm,default avm mode");
		n6_fastavm_metazone_invalid_handle();
		return ERROR_NONE;
	}

	if (0 == n6_is_avm) {
		n6_format_ch0 = n6_detect_format(0);
		if (0xff == n6_format_ch0) {
			n6_format_ch0 = 0x20;
		}
		mipi_ch_en = mipi_ch_en |0x11;

		n6_format_ch1 = n6_detect_format(1);
		if (0xff == n6_format_ch1) {
			n6_format_ch1 = 0x20;
		}
		mipi_ch_en = mipi_ch_en |0x22;

		n6_format_ch2 = n6_detect_format(2);
		if (0xff == n6_format_ch2) {
			n6_format_ch2 = 0x20;
		}
		mipi_ch_en = mipi_ch_en | 0x44;

		n6_format_ch3 = n6_detect_format(3);
		if (0xff == n6_format_ch3) {
			n6_format_ch3 = 0x20;
		}
		mipi_ch_en = mipi_ch_en | 0x88;





		LOG_INF("4 channel format is ch0 = 0x%x,ch1 = 0x%x,ch2 = 0x%x,ch3 = 0x%x\n",n6_format_ch0,n6_format_ch1,n6_format_ch2,n6_format_ch3);
		n6_common_settings();

		n6_init_ch(0,n6_format_ch0);
		n6_init_ch(1,n6_format_ch1);
		n6_init_ch(2,n6_format_ch2);
		n6_init_ch(3,n6_format_ch3);

		n6_mipi_outputs(mipi_ch_en);
	}

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 n6_close(void)
{
	LOG_INF("n6Close");
	n6_is_avm = 1;
	fastavm = false;
	return ERROR_NONE;
}

UINT32 n6_getinfo(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX = input_source_width;
	pSensorInfo->SensorPreviewResolutionY = input_source_height;
	pSensorInfo->SensorFullResolutionX = input_source_width;
	pSensorInfo->SensorFullResolutionY = input_source_height;

	pSensorInfo->SensorCameraPreviewFrameRate = 30;
	pSensorInfo->SensorVideoFrameRate = 30;
	pSensorInfo->SensorStillCaptureFrameRate = 10;
	pSensorInfo->SensorWebCamCaptureFrameRate = 15;

	pSensorInfo->SensorResetActiveHigh = FALSE;
	pSensorInfo->SensorResetDelayCount = 1;

	pSensorInfo->SensorOutputDataFormat = SENSOR_OUTPUT_FORMAT_YUYV;

	pSensorInfo->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; //inverse
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

	pSensorInfo->SensorInterruptDelayLines = 1;
	pSensorInfo->CaptureDelayFrame = 5;
	pSensorInfo->PreviewDelayFrame = 5;
	pSensorInfo->VideoDelayFrame = 5;
	pSensorInfo->YUVAwbDelayFrame = 0;
	pSensorInfo->YUVEffectDelayFrame = 0;
	pSensorInfo->AESensorGainDelayFrame = 0;
	pSensorInfo->AEISPGainDelayFrame = 0;
	pSensorInfo->SensorModeNum = 5;

	pSensorInfo->SensorMasterClockSwitch = 0;
	pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;
	pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_MIPI;
	pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;

	pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
	pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
	pSensorInfo->SensorPacketECCOrder = 1;
	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 30;

	pSensorInfo->SettleDelayMode = MIPI_SETTLEDELAY_MANUAL;
	pSensorInfo->MIPIsensorType = MIPI_OPHY_NCSI2;

	switch (ScenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pSensorInfo->SensorClockFreq = 24;
			pSensorInfo->SensorClockDividCount = 3;
			pSensorInfo->SensorClockRisingCount = 0;
			pSensorInfo->SensorClockFallingCount = 2;
			pSensorInfo->SensorPixelClockCount = 3;
			pSensorInfo->SensorDataLatchCount = 2;
			pSensorInfo->SensorGrabStartX = 0;
			pSensorInfo->SensorGrabStartY = 0;
			break;

		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			pSensorInfo->SensorClockFreq = 24;
			pSensorInfo->SensorClockDividCount = 3;
			pSensorInfo->SensorClockRisingCount = 0;
			pSensorInfo->SensorClockFallingCount = 2;
			pSensorInfo->SensorPixelClockCount = 3;
			pSensorInfo->SensorDataLatchCount = 2;
			pSensorInfo->SensorGrabStartX = 0;
			pSensorInfo->SensorGrabStartY = 0;
			break;

		default:
			pSensorInfo->SensorClockFreq = 24;
			pSensorInfo->SensorClockDividCount = 3;
			pSensorInfo->SensorClockRisingCount = 0;
			pSensorInfo->SensorClockFallingCount = 2;
			pSensorInfo->SensorPixelClockCount = 3;
			pSensorInfo->SensorDataLatchCount = 2;
			pSensorInfo->SensorGrabStartX = 0;
			pSensorInfo->SensorGrabStartY = 0;
			break;
	}

		memcpy(pSensorConfigData, &n6SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

		return ERROR_NONE;

}

UINT32 n6_control(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	LOG_INF("Enter, ScenarioId = %d\n", ScenarioId);
	switch (ScenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			n6_preview(pImageWindow, pSensorConfigData);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			n6_capture(pImageWindow, pSensorConfigData);
			break;

		#ifdef ATC_KERN_SUPPORT_CVBS_FORMAT_CHANGE
		case MSDK_UPDATE_CVBS_FORMAT:
			//N6ForceUpdateFormat();
			break;
		#endif

		default:
			break;
	}

	LOG_INF("Leave\n");

	return ERROR_NONE;
}

UINT32 n6_featurecontrol(MSDK_SENSOR_FEATURE_ENUM FeatureId,
		UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16 = (UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32 = (UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32 = (UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData = (MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData = (MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

	unsigned long long *feature_data = (unsigned long long *)pFeaturePara;
	switch (FeatureId) {
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++ = input_source_width;
			*pFeatureReturnPara16 = input_source_height;
			*pFeatureParaLen = 4;
			break;
		case SENSOR_FEATURE_GET_PERIOD:
			*pFeatureReturnPara16++ = input_source_width;
			*pFeatureReturnPara16 = input_source_height;
			*pFeatureParaLen = 4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = 648000000;
			*pFeatureParaLen = 4;
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			break;
		case SENSOR_FEATURE_SET_GAIN:
			break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			n6_isp_master_clock = *pFeatureData32;
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			n6_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = n6_read_cmos_sensor(pSensorRegData->RegAddr);
			break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &n6SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			*pFeatureParaLen = sizeof(MSDK_SENSOR_CONFIG_STRUCT);
			break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
		case SENSOR_FEATURE_GET_GROUP_COUNT:
			*pFeatureReturnPara32++ = 0;
			*pFeatureParaLen = 4;
			break;
		case SENSOR_FEATURE_GET_GROUP_INFO:
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			*pFeatureReturnPara32 = LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen = 4;
			break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			n6_getsensorid(pFeatureData32);
			break;
		case SENSOR_FEATURE_GET_DELAY_INFO:
			break;
		case SENSOR_FEATURE_GET_PIXEL_RATE:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 364500000;
			break;
		case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 648000000;
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 250;
			break;
		case SENSOR_FEATURE_GET_CH0_SIGNAL_STATUS:
			*pFeatureData32 = n6_get_signalstatus(0);
			break;
		case SENSOR_FEATURE_GET_CH1_SIGNAL_STATUS:
			*pFeatureData32 = n6_get_signalstatus(1);
			break;
		case SENSOR_FEATURE_GET_CH2_SIGNAL_STATUS:
			*pFeatureData32 = n6_get_signalstatus(2);
			break;
		case SENSOR_FEATURE_GET_CH3_SIGNAL_STATUS:
			*pFeatureData32 = n6_get_signalstatus(3);
			break;
		case SENSOR_FEATURE_SET_IS_AVM:
			n6_is_avm = *pFeatureData32;
			break;
		default:
			break;
	}
	return ERROR_NONE;
}


static struct SENSOR_FUNCTION_STRUCT sensor_func =
{
	n6_open,
	n6_getinfo,
	n6_getresolution,
	n6_featurecontrol,
	n6_control,
	n6_close,
};

UINT32 N6_YUV_SensorInit(
			struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL)
	{
		*pfFunc = &sensor_func;
	}
	return ERROR_NONE;
}






