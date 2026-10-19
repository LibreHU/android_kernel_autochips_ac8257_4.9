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
*   tp2850_yuv_Sensor.c
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
#include "metazone/metazone.h"
#include "metazone/metazone_ioctl.h"


#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "tp2850_yuv_sensor.h"

/****************************Modify Following Strings for Debug****************************/
#define PFX "[TP2850]"

/****************************   Modify end    *******************************************/
#define LOG_DBG(format, args...)    pr_debug(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    pr_info(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...)    pr_err(PFX " %s " format, __FUNCTION__, ##args)

#define METAZONE_AHD_FORMAT_DEFAULT             0x10045

static kal_uint16 tp2850_i2c_write_id = 0x88;
static int tp2850_i2c_speed = 400;
static VIDEO_FORMAT source_format = FORMAT_MAX;
kal_uint32 TP2850_isp_master_clock;


static kal_uint32 input_source_width = TP2850_IMAGE_SENSOR_HD30_WIDTH;
static kal_uint32 input_source_height = TP2850_IMAGE_SENSOR_HD30_HEIGHT;
static kal_uint32 input_source_fps = TP2850_FPS_30;

MSDK_SENSOR_CONFIG_STRUCT TP2850SensorConfigData;


static kal_uint16 TP2850_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[1] = { (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 1, (u8 *) &get_byte, 1, tp2850_i2c_write_id);

	return get_byte;
}

static int TP2850_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[2] = { (char)(addr & 0xFF), (char)(para & 0xFF) };

	return iWriteRegI2CTiming(
		pu_send_cmd, 2, tp2850_i2c_write_id, tp2850_i2c_speed);
}

void TP2850_Sensor_PreInit(void)
{
	LOG_INF("sensor_preinit in\n");
	TP2850_write_cmos_sensor(0x40, 0x00);
	TP2850_write_cmos_sensor(0x02, 0x44);
	TP2850_write_cmos_sensor(0x07, 0x80);
	TP2850_write_cmos_sensor(0x0B, 0xC0);
	TP2850_write_cmos_sensor(0x0C, 0x03);
	TP2850_write_cmos_sensor(0x0D, 0x72);
	TP2850_write_cmos_sensor(0x0E, 0x00);
	TP2850_write_cmos_sensor(0x0F, 0x00);
	TP2850_write_cmos_sensor(0x10, 0x00);
	TP2850_write_cmos_sensor(0x11, 0x40);
	TP2850_write_cmos_sensor(0x12, 0x40);
	TP2850_write_cmos_sensor(0x13, 0x00);
	TP2850_write_cmos_sensor(0x14, 0x00);
	TP2850_write_cmos_sensor(0x15, 0x01);
	TP2850_write_cmos_sensor(0x16, 0xF0);
	TP2850_write_cmos_sensor(0x17, 0x80);
	TP2850_write_cmos_sensor(0x18, 0x29);
	TP2850_write_cmos_sensor(0x19, 0x38);
	TP2850_write_cmos_sensor(0x1A, 0x47);
	TP2850_write_cmos_sensor(0x1B, 0x01);
	TP2850_write_cmos_sensor(0x1C, 0x08);
	TP2850_write_cmos_sensor(0x1D, 0x98);
	TP2850_write_cmos_sensor(0x20, 0x38);
	TP2850_write_cmos_sensor(0x21, 0x46);
	TP2850_write_cmos_sensor(0x22, 0x36);
	TP2850_write_cmos_sensor(0x23, 0x3C);
	TP2850_write_cmos_sensor(0x24, 0x04);
	TP2850_write_cmos_sensor(0x25, 0xFE);
	TP2850_write_cmos_sensor(0x26, 0x0D);
	TP2850_write_cmos_sensor(0x27, 0x2D);
	TP2850_write_cmos_sensor(0x2A, 0x30);
	TP2850_write_cmos_sensor(0x2B, 0x60);
	TP2850_write_cmos_sensor(0x2C, 0x3A);
	TP2850_write_cmos_sensor(0x2D, 0x54);
	TP2850_write_cmos_sensor(0x2E, 0x40);
	TP2850_write_cmos_sensor(0x30, 0xA5);
	TP2850_write_cmos_sensor(0x31, 0x95);
	TP2850_write_cmos_sensor(0x32, 0xE0);
	TP2850_write_cmos_sensor(0x33, 0x60);
	TP2850_write_cmos_sensor(0x35, 0x05);
	TP2850_write_cmos_sensor(0x36, 0xDC);
	TP2850_write_cmos_sensor(0x38, 0x40);
	TP2850_write_cmos_sensor(0x39, 0x1C);
	TP2850_write_cmos_sensor(0x3A, 0x32);
	TP2850_write_cmos_sensor(0x3B, 0x26);
	TP2850_write_cmos_sensor(0x41, 0x00);
	TP2850_write_cmos_sensor(0x4E, 0x00);
	TP2850_write_cmos_sensor(0x4F, 0x00);
	TP2850_write_cmos_sensor(0xFD, 0x80);
	TP2850_write_cmos_sensor(0x06, 0xB2);
	TP2850_write_cmos_sensor(0x40, 0x08);
	TP2850_write_cmos_sensor(0x10, 0x88);
	TP2850_write_cmos_sensor(0x10, 0x08);
	TP2850_write_cmos_sensor(0x01, 0xf8);
	TP2850_write_cmos_sensor(0x02, 0x01);
	TP2850_write_cmos_sensor(0x08, 0x0f);
	TP2850_write_cmos_sensor(0x13, 0x04);
	TP2850_write_cmos_sensor(0x14, 0x46);
	TP2850_write_cmos_sensor(0x15, 0x08);
	TP2850_write_cmos_sensor(0x20, 0x12);
	TP2850_write_cmos_sensor(0x34, 0x1b);
	TP2850_write_cmos_sensor(0x23, 0x02);
	TP2850_write_cmos_sensor(0x25, 0x02);
	TP2850_write_cmos_sensor(0x26, 0x01);
	TP2850_write_cmos_sensor(0x27, 0x08);
	TP2850_write_cmos_sensor(0x40, 0x00);
}

#if 0
static void TP2850_Sensor_Ntsc_Init(void)
{
	TP2850_write_cmos_sensor(0x02, 0x47);
	TP2850_write_cmos_sensor(0x05, 0x00);
	TP2850_write_cmos_sensor(0x06, 0x32);
	TP2850_write_cmos_sensor(0x07, 0xC0);
	TP2850_write_cmos_sensor(0x08, 0x00);
	TP2850_write_cmos_sensor(0x09, 0x24);
	TP2850_write_cmos_sensor(0x0A, 0x48);
	TP2850_write_cmos_sensor(0x0B, 0xC0);
	TP2850_write_cmos_sensor(0x0C, 0x13);
	TP2850_write_cmos_sensor(0x0D, 0x50);
	TP2850_write_cmos_sensor(0x0E, 0x00);
	TP2850_write_cmos_sensor(0x0F, 0x00);
	TP2850_write_cmos_sensor(0x10, 0xF8);
	TP2850_write_cmos_sensor(0x11, 0x40);
	TP2850_write_cmos_sensor(0x12, 0x60);
	TP2850_write_cmos_sensor(0x13, 0x00);
	TP2850_write_cmos_sensor(0x14, 0x00);
	TP2850_write_cmos_sensor(0x15, 0x13);
	TP2850_write_cmos_sensor(0x16, 0x4A);
	TP2850_write_cmos_sensor(0x17, 0xBC);
	TP2850_write_cmos_sensor(0x18, 0x12);
	TP2850_write_cmos_sensor(0x19, 0xF0);
	TP2850_write_cmos_sensor(0x1A, 0x07);
	TP2850_write_cmos_sensor(0x1B, 0x00);
	TP2850_write_cmos_sensor(0x1C, 0x09);
	TP2850_write_cmos_sensor(0x1D, 0x38);
	TP2850_write_cmos_sensor(0x1E, 0x80);
	TP2850_write_cmos_sensor(0x1F, 0x80);
	TP2850_write_cmos_sensor(0x20, 0x40);
	TP2850_write_cmos_sensor(0x21, 0x84);
	TP2850_write_cmos_sensor(0x22, 0x36);
	TP2850_write_cmos_sensor(0x23, 0x3C);
	TP2850_write_cmos_sensor(0x24, 0x04);
	TP2850_write_cmos_sensor(0x25, 0xFF);
	TP2850_write_cmos_sensor(0x26, 0x05);
	TP2850_write_cmos_sensor(0x27, 0x2D);
	TP2850_write_cmos_sensor(0x28, 0x00);
	TP2850_write_cmos_sensor(0x29, 0x48);
	TP2850_write_cmos_sensor(0x2A, 0x30);
	TP2850_write_cmos_sensor(0x2B, 0x70);
	TP2850_write_cmos_sensor(0x2C, 0x2A);
	TP2850_write_cmos_sensor(0x2D, 0x68);
	TP2850_write_cmos_sensor(0x2E, 0x57);
	TP2850_write_cmos_sensor(0x2F, 0x00);
	TP2850_write_cmos_sensor(0x30, 0x62);
	TP2850_write_cmos_sensor(0x31, 0xBB);
	TP2850_write_cmos_sensor(0x32, 0x96);
	TP2850_write_cmos_sensor(0x33, 0xC0);
	TP2850_write_cmos_sensor(0x34, 0x00);
	TP2850_write_cmos_sensor(0x35, 0x25);
	TP2850_write_cmos_sensor(0x36, 0xDC);
	TP2850_write_cmos_sensor(0x37, 0x00);
	TP2850_write_cmos_sensor(0x38, 0x00);
	TP2850_write_cmos_sensor(0x39, 0x04);
	TP2850_write_cmos_sensor(0x3A, 0x32);
	TP2850_write_cmos_sensor(0x3B, 0x26);
	TP2850_write_cmos_sensor(0x3C, 0x00);
	TP2850_write_cmos_sensor(0x3D, 0x60);
	TP2850_write_cmos_sensor(0x3E, 0x00);
	TP2850_write_cmos_sensor(0x3F, 0x00);
	TP2850_write_cmos_sensor(0x40, 0x00);
	TP2850_write_cmos_sensor(0x41, 0x00);
	TP2850_write_cmos_sensor(0x42, 0x00);
	TP2850_write_cmos_sensor(0x43, 0x00);
	TP2850_write_cmos_sensor(0x44, 0x00);
	TP2850_write_cmos_sensor(0x45, 0x00);
	TP2850_write_cmos_sensor(0x46, 0x00);
	TP2850_write_cmos_sensor(0x47, 0x00);
	TP2850_write_cmos_sensor(0x48, 0x00);
	TP2850_write_cmos_sensor(0x49, 0x00);
	TP2850_write_cmos_sensor(0x4A, 0x00);
	TP2850_write_cmos_sensor(0x4B, 0x00);
	TP2850_write_cmos_sensor(0x4C, 0x40);
	TP2850_write_cmos_sensor(0x4D, 0x00);
	TP2850_write_cmos_sensor(0x4E, 0x00);
	TP2850_write_cmos_sensor(0x4F, 0x00);
	TP2850_write_cmos_sensor(0x50, 0x00);
	TP2850_write_cmos_sensor(0x51, 0x00);
	TP2850_write_cmos_sensor(0x52, 0x00);
	TP2850_write_cmos_sensor(0x53, 0x00);
	TP2850_write_cmos_sensor(0x54, 0x00);

	TP2850_write_cmos_sensor(0xB3, 0xFA);
	TP2850_write_cmos_sensor(0xB4, 0x00);
	TP2850_write_cmos_sensor(0xB5, 0x00);
	TP2850_write_cmos_sensor(0xB6, 0x00);
	TP2850_write_cmos_sensor(0xB7, 0x00);
	TP2850_write_cmos_sensor(0xB8, 0x00);
	TP2850_write_cmos_sensor(0xB9, 0x00);
	TP2850_write_cmos_sensor(0xBA, 0x00);
	TP2850_write_cmos_sensor(0xBB, 0x00);
	TP2850_write_cmos_sensor(0xBC, 0x00);
	TP2850_write_cmos_sensor(0xBD, 0x00);
	TP2850_write_cmos_sensor(0xBE, 0x00);
	TP2850_write_cmos_sensor(0xBF, 0x00);
	TP2850_write_cmos_sensor(0xC0, 0x00);
	TP2850_write_cmos_sensor(0xC1, 0x00);
	TP2850_write_cmos_sensor(0xC2, 0x0B);
	TP2850_write_cmos_sensor(0xC3, 0x0C);
	TP2850_write_cmos_sensor(0xC4, 0x00);
	TP2850_write_cmos_sensor(0xC5, 0x00);
	TP2850_write_cmos_sensor(0xC6, 0x1F);
	TP2850_write_cmos_sensor(0xC7, 0x78);
	TP2850_write_cmos_sensor(0xC8, 0x27);
	TP2850_write_cmos_sensor(0xC9, 0x00);
	TP2850_write_cmos_sensor(0xCA, 0x00);
	TP2850_write_cmos_sensor(0xCB, 0x07);
	TP2850_write_cmos_sensor(0xCC, 0x08);
	TP2850_write_cmos_sensor(0xCD, 0x00);
	TP2850_write_cmos_sensor(0xCE, 0x00);
	TP2850_write_cmos_sensor(0xCF, 0x04);
	TP2850_write_cmos_sensor(0xD0, 0x00);
	TP2850_write_cmos_sensor(0xD1, 0x00);
	TP2850_write_cmos_sensor(0xD2, 0x60);
	TP2850_write_cmos_sensor(0xD3, 0x10);
	TP2850_write_cmos_sensor(0xD4, 0x06);
	TP2850_write_cmos_sensor(0xD5, 0xBE);
	TP2850_write_cmos_sensor(0xD6, 0x39);
	TP2850_write_cmos_sensor(0xD7, 0x27);
	TP2850_write_cmos_sensor(0xD8, 0x00);
	TP2850_write_cmos_sensor(0xD9, 0x00);
	TP2850_write_cmos_sensor(0xDA, 0x00);
	TP2850_write_cmos_sensor(0xDB, 0x00);
	TP2850_write_cmos_sensor(0xDC, 0x00);
	TP2850_write_cmos_sensor(0xDD, 0x00);
	TP2850_write_cmos_sensor(0xDE, 0x00);
	TP2850_write_cmos_sensor(0xDF, 0x00);
	TP2850_write_cmos_sensor(0xE0, 0x00);
	TP2850_write_cmos_sensor(0xE1, 0x00);
	TP2850_write_cmos_sensor(0xE2, 0x00);
	TP2850_write_cmos_sensor(0xE3, 0x00);
	TP2850_write_cmos_sensor(0xE4, 0x00);
	TP2850_write_cmos_sensor(0xE5, 0x00);
	TP2850_write_cmos_sensor(0xE6, 0x00);
	TP2850_write_cmos_sensor(0xE7, 0x13);
	TP2850_write_cmos_sensor(0xE8, 0x03);
	TP2850_write_cmos_sensor(0xE9, 0x00);
	TP2850_write_cmos_sensor(0xEA, 0x00);
	TP2850_write_cmos_sensor(0xEB, 0x00);
	TP2850_write_cmos_sensor(0xEC, 0x00);
	TP2850_write_cmos_sensor(0xED, 0x00);
	TP2850_write_cmos_sensor(0xEE, 0x00);
	TP2850_write_cmos_sensor(0xEF, 0x00);
	TP2850_write_cmos_sensor(0xF0, 0x00);
	TP2850_write_cmos_sensor(0xF1, 0x00);
	TP2850_write_cmos_sensor(0xF2, 0x00);
	TP2850_write_cmos_sensor(0xF3, 0x00);
	TP2850_write_cmos_sensor(0xF4, 0x20);
	TP2850_write_cmos_sensor(0xF5, 0x10);
	TP2850_write_cmos_sensor(0xF6, 0x00);
	TP2850_write_cmos_sensor(0xF7, 0x00);
	TP2850_write_cmos_sensor(0xF8, 0x00);
	TP2850_write_cmos_sensor(0xF9, 0x00);
	TP2850_write_cmos_sensor(0xFA, 0x88);
	TP2850_write_cmos_sensor(0xFB, 0x00);
	TP2850_write_cmos_sensor(0xFC, 0x00);

	TP2850_write_cmos_sensor(0x40, 0x08);
	TP2850_write_cmos_sensor(0x01, 0xf8);
	TP2850_write_cmos_sensor(0x02, 0x01);
	TP2850_write_cmos_sensor(0x08, 0x0f);
	TP2850_write_cmos_sensor(0x13, 0x24);
	TP2850_write_cmos_sensor(0x14, 0x46);
	TP2850_write_cmos_sensor(0x15, 0x08);
	TP2850_write_cmos_sensor(0x20, 0x12);
	TP2850_write_cmos_sensor(0x34, 0x1b);
	TP2850_write_cmos_sensor(0x23, 0x02);
	TP2850_write_cmos_sensor(0x23, 0x00);
	TP2850_write_cmos_sensor(0x25, 0x02);
	TP2850_write_cmos_sensor(0x26, 0x01);
	TP2850_write_cmos_sensor(0x27, 0x08);
	TP2850_write_cmos_sensor(0x40, 0x00);

}

static void TP2850_Sensor_Pal_Init(void)
{

	TP2850_write_cmos_sensor(0x02, 0x47);
	TP2850_write_cmos_sensor(0x05, 0x00);
	TP2850_write_cmos_sensor(0x06, 0x32);
	TP2850_write_cmos_sensor(0x07, 0xC0);
	TP2850_write_cmos_sensor(0x08, 0x00);
	TP2850_write_cmos_sensor(0x09, 0x24);
	TP2850_write_cmos_sensor(0x0A, 0x48);
	TP2850_write_cmos_sensor(0x0B, 0xC0);
	TP2850_write_cmos_sensor(0x0C, 0x13);
	TP2850_write_cmos_sensor(0x0D, 0x51);
	TP2850_write_cmos_sensor(0x0E, 0x00);
	TP2850_write_cmos_sensor(0x0F, 0x00);
	TP2850_write_cmos_sensor(0x10, 0xF8);
	TP2850_write_cmos_sensor(0x11, 0x40);
	TP2850_write_cmos_sensor(0x12, 0x60);
	TP2850_write_cmos_sensor(0x13, 0x00);
	TP2850_write_cmos_sensor(0x14, 0x00);
	TP2850_write_cmos_sensor(0x15, 0x13);
	TP2850_write_cmos_sensor(0x16, 0x70);
	TP2850_write_cmos_sensor(0x17, 0xBC);
	TP2850_write_cmos_sensor(0x18, 0x17);
	TP2850_write_cmos_sensor(0x19, 0x20);
	TP2850_write_cmos_sensor(0x1A, 0x17);
	TP2850_write_cmos_sensor(0x1B, 0x00);
	TP2850_write_cmos_sensor(0x1C, 0x09);
	TP2850_write_cmos_sensor(0x1D, 0x48);
	TP2850_write_cmos_sensor(0x1E, 0x80);
	TP2850_write_cmos_sensor(0x1F, 0x80);
	TP2850_write_cmos_sensor(0x20, 0x48);
	TP2850_write_cmos_sensor(0x21, 0x84);
	TP2850_write_cmos_sensor(0x22, 0x37);
	TP2850_write_cmos_sensor(0x23, 0x3F);
	TP2850_write_cmos_sensor(0x24, 0x04);
	TP2850_write_cmos_sensor(0x25, 0xFF);
	TP2850_write_cmos_sensor(0x26, 0x05);
	TP2850_write_cmos_sensor(0x27, 0x2D);
	TP2850_write_cmos_sensor(0x28, 0x00);
	TP2850_write_cmos_sensor(0x29, 0x48);
	TP2850_write_cmos_sensor(0x2A, 0x30);
	TP2850_write_cmos_sensor(0x2B, 0x70);
	TP2850_write_cmos_sensor(0x2C, 0x2A);
	TP2850_write_cmos_sensor(0x2D, 0x64);
	TP2850_write_cmos_sensor(0x2E, 0x56);
	TP2850_write_cmos_sensor(0x2F, 0x00);
	TP2850_write_cmos_sensor(0x30, 0x7A);
	TP2850_write_cmos_sensor(0x31, 0x4A);
	TP2850_write_cmos_sensor(0x32, 0x4D);
	TP2850_write_cmos_sensor(0x33, 0xF0);
	TP2850_write_cmos_sensor(0x34, 0x00);
	TP2850_write_cmos_sensor(0x35, 0x25);
	TP2850_write_cmos_sensor(0x36, 0xDC);
	TP2850_write_cmos_sensor(0x37, 0x00);
	TP2850_write_cmos_sensor(0x38, 0x00);
	TP2850_write_cmos_sensor(0x39, 0x04);
	TP2850_write_cmos_sensor(0x3A, 0x32);
	TP2850_write_cmos_sensor(0x3B, 0x26);
	TP2850_write_cmos_sensor(0x3C, 0x00);
	TP2850_write_cmos_sensor(0x3D, 0x60);
	TP2850_write_cmos_sensor(0x3E, 0x00);
	TP2850_write_cmos_sensor(0x3F, 0x00);
	TP2850_write_cmos_sensor(0x40, 0x00);
	TP2850_write_cmos_sensor(0x41, 0x00);
	TP2850_write_cmos_sensor(0x42, 0x00);
	TP2850_write_cmos_sensor(0x43, 0x00);
	TP2850_write_cmos_sensor(0x44, 0x00);
	TP2850_write_cmos_sensor(0x45, 0x00);
	TP2850_write_cmos_sensor(0x46, 0x00);
	TP2850_write_cmos_sensor(0x47, 0x00);
	TP2850_write_cmos_sensor(0x48, 0x00);
	TP2850_write_cmos_sensor(0x49, 0x00);
	TP2850_write_cmos_sensor(0x4A, 0x00);
	TP2850_write_cmos_sensor(0x4B, 0x00);
	TP2850_write_cmos_sensor(0x4C, 0x40);
	TP2850_write_cmos_sensor(0x4D, 0x00);
	TP2850_write_cmos_sensor(0x4E, 0x00);
	TP2850_write_cmos_sensor(0x4F, 0x00);
	TP2850_write_cmos_sensor(0x50, 0x00);
	TP2850_write_cmos_sensor(0x51, 0x00);
	TP2850_write_cmos_sensor(0x52, 0x00);
	TP2850_write_cmos_sensor(0x53, 0x00);
	TP2850_write_cmos_sensor(0x54, 0x00);

	TP2850_write_cmos_sensor(0xB3, 0xFA);
	TP2850_write_cmos_sensor(0xB4, 0x00);
	TP2850_write_cmos_sensor(0xB5, 0x00);
	TP2850_write_cmos_sensor(0xB6, 0x00);
	TP2850_write_cmos_sensor(0xB7, 0x00);
	TP2850_write_cmos_sensor(0xB8, 0x00);
	TP2850_write_cmos_sensor(0xB9, 0x00);
	TP2850_write_cmos_sensor(0xBA, 0x00);
	TP2850_write_cmos_sensor(0xBB, 0x00);
	TP2850_write_cmos_sensor(0xBC, 0x00);
	TP2850_write_cmos_sensor(0xBD, 0x00);
	TP2850_write_cmos_sensor(0xBE, 0x00);
	TP2850_write_cmos_sensor(0xBF, 0x00);
	TP2850_write_cmos_sensor(0xC0, 0x00);
	TP2850_write_cmos_sensor(0xC1, 0x00);
	TP2850_write_cmos_sensor(0xC2, 0x0B);
	TP2850_write_cmos_sensor(0xC3, 0x0C);
	TP2850_write_cmos_sensor(0xC4, 0x00);
	TP2850_write_cmos_sensor(0xC5, 0x00);
	TP2850_write_cmos_sensor(0xC6, 0x1F);
	TP2850_write_cmos_sensor(0xC7, 0x78);
	TP2850_write_cmos_sensor(0xC8, 0x27);
	TP2850_write_cmos_sensor(0xC9, 0x00);
	TP2850_write_cmos_sensor(0xCA, 0x00);
	TP2850_write_cmos_sensor(0xCB, 0x07);
	TP2850_write_cmos_sensor(0xCC, 0x08);
	TP2850_write_cmos_sensor(0xCD, 0x00);
	TP2850_write_cmos_sensor(0xCE, 0x00);
	TP2850_write_cmos_sensor(0xCF, 0x04);
	TP2850_write_cmos_sensor(0xD0, 0x00);
	TP2850_write_cmos_sensor(0xD1, 0x00);
	TP2850_write_cmos_sensor(0xD2, 0x60);
	TP2850_write_cmos_sensor(0xD3, 0x10);
	TP2850_write_cmos_sensor(0xD4, 0x06);
	TP2850_write_cmos_sensor(0xD5, 0xBE);
	TP2850_write_cmos_sensor(0xD6, 0x39);
	TP2850_write_cmos_sensor(0xD7, 0x27);
	TP2850_write_cmos_sensor(0xD8, 0x00);
	TP2850_write_cmos_sensor(0xD9, 0x00);
	TP2850_write_cmos_sensor(0xDA, 0x00);
	TP2850_write_cmos_sensor(0xDB, 0x00);
	TP2850_write_cmos_sensor(0xDC, 0x00);
	TP2850_write_cmos_sensor(0xDD, 0x00);
	TP2850_write_cmos_sensor(0xDE, 0x00);
	TP2850_write_cmos_sensor(0xDF, 0x00);
	TP2850_write_cmos_sensor(0xE0, 0x00);
	TP2850_write_cmos_sensor(0xE1, 0x00);
	TP2850_write_cmos_sensor(0xE2, 0x00);
	TP2850_write_cmos_sensor(0xE3, 0x00);
	TP2850_write_cmos_sensor(0xE4, 0x00);
	TP2850_write_cmos_sensor(0xE5, 0x00);
	TP2850_write_cmos_sensor(0xE6, 0x00);
	TP2850_write_cmos_sensor(0xE7, 0x13);
	TP2850_write_cmos_sensor(0xE8, 0x03);
	TP2850_write_cmos_sensor(0xE9, 0x00);
	TP2850_write_cmos_sensor(0xEA, 0x00);
	TP2850_write_cmos_sensor(0xEB, 0x00);
	TP2850_write_cmos_sensor(0xEC, 0x00);
	TP2850_write_cmos_sensor(0xED, 0x00);
	TP2850_write_cmos_sensor(0xEE, 0x00);
	TP2850_write_cmos_sensor(0xEF, 0x00);
	TP2850_write_cmos_sensor(0xF0, 0x00);
	TP2850_write_cmos_sensor(0xF1, 0x00);
	TP2850_write_cmos_sensor(0xF2, 0x00);
	TP2850_write_cmos_sensor(0xF3, 0x00);
	TP2850_write_cmos_sensor(0xF4, 0x20);
	TP2850_write_cmos_sensor(0xF5, 0x10);
	TP2850_write_cmos_sensor(0xF6, 0x00);
	TP2850_write_cmos_sensor(0xF7, 0x00);
	TP2850_write_cmos_sensor(0xF8, 0x00);
	TP2850_write_cmos_sensor(0xF9, 0x00);
	TP2850_write_cmos_sensor(0xFA, 0x88);
	TP2850_write_cmos_sensor(0xFB, 0x00);
	TP2850_write_cmos_sensor(0xFC, 0x00);

	TP2850_write_cmos_sensor(0x40, 0x08);
	TP2850_write_cmos_sensor(0x01, 0xf8);
	TP2850_write_cmos_sensor(0x02, 0x01);
	TP2850_write_cmos_sensor(0x08, 0x0f);
	TP2850_write_cmos_sensor(0x13, 0x24);
	TP2850_write_cmos_sensor(0x14, 0x46);
	TP2850_write_cmos_sensor(0x15, 0x08);
	TP2850_write_cmos_sensor(0x20, 0x12);
	TP2850_write_cmos_sensor(0x34, 0x1b);
	TP2850_write_cmos_sensor(0x23, 0x02);
	TP2850_write_cmos_sensor(0x23, 0x00);
	TP2850_write_cmos_sensor(0x25, 0x02);
	TP2850_write_cmos_sensor(0x26, 0x01);
	TP2850_write_cmos_sensor(0x27, 0x08);
	TP2850_write_cmos_sensor(0x40, 0x00);

}
#endif
static void TP2850_Sensor_Init(VIDEO_FORMAT source_format)
{
	LOG_INF("TP2850 sensor init enter source_format = %d \n",source_format);
	if (FORMAT_HD_25== source_format) {
		TP2850_write_cmos_sensor(0x40, 0x00);
		TP2850_write_cmos_sensor(0x02, 0x46);
		TP2850_write_cmos_sensor(0x07, 0x80);
		TP2850_write_cmos_sensor(0x0B, 0xC0);
		TP2850_write_cmos_sensor(0x0C, 0x03);
		TP2850_write_cmos_sensor(0x0D, 0x71);
		TP2850_write_cmos_sensor(0x0E, 0x00);
		TP2850_write_cmos_sensor(0x0F, 0x00);
		TP2850_write_cmos_sensor(0x10, 0x00);
		TP2850_write_cmos_sensor(0x11, 0x40);
		TP2850_write_cmos_sensor(0x12, 0x40);
		TP2850_write_cmos_sensor(0x13, 0x00);
		TP2850_write_cmos_sensor(0x14, 0x00);
		TP2850_write_cmos_sensor(0x15, 0x13);
		TP2850_write_cmos_sensor(0x16, 0x16);
		TP2850_write_cmos_sensor(0x17, 0x00);
		TP2850_write_cmos_sensor(0x18, 0x19);
		TP2850_write_cmos_sensor(0x19, 0xD0);
		TP2850_write_cmos_sensor(0x1A, 0x25);
		TP2850_write_cmos_sensor(0x1B, 0x01);
		TP2850_write_cmos_sensor(0x1C, 0x07);
		TP2850_write_cmos_sensor(0x1D, 0xBC);
		TP2850_write_cmos_sensor(0x20, 0x40);
		TP2850_write_cmos_sensor(0x21, 0x46);
		TP2850_write_cmos_sensor(0x22, 0x36);
		TP2850_write_cmos_sensor(0x23, 0x3C);
		TP2850_write_cmos_sensor(0x24, 0x04);
		TP2850_write_cmos_sensor(0x25, 0xFE);
		TP2850_write_cmos_sensor(0x26, 0x01);
		TP2850_write_cmos_sensor(0x27, 0x2D);
		TP2850_write_cmos_sensor(0x2A, 0x30);
		TP2850_write_cmos_sensor(0x2B, 0x60);
		TP2850_write_cmos_sensor(0x2C, 0x3A);
		TP2850_write_cmos_sensor(0x2D, 0x5A);
		TP2850_write_cmos_sensor(0x2E, 0x40);
		TP2850_write_cmos_sensor(0x30, 0x9E);
		TP2850_write_cmos_sensor(0x31, 0x20);
		TP2850_write_cmos_sensor(0x32, 0x01);
		TP2850_write_cmos_sensor(0x33, 0x90);
		TP2850_write_cmos_sensor(0x35, 0x25);
		TP2850_write_cmos_sensor(0x36, 0xDC);
		TP2850_write_cmos_sensor(0x38, 0x40);
		TP2850_write_cmos_sensor(0x39, 0x18);
		TP2850_write_cmos_sensor(0x3A, 0x32);
		TP2850_write_cmos_sensor(0x3B, 0x26);
		TP2850_write_cmos_sensor(0x41, 0x00);
		TP2850_write_cmos_sensor(0x4E, 0x00);
		TP2850_write_cmos_sensor(0x4F, 0x00);
		TP2850_write_cmos_sensor(0xFD, 0x80);
		TP2850_write_cmos_sensor(0x06, 0xB2);

		TP2850_write_cmos_sensor(0x40, 0x08);
		TP2850_write_cmos_sensor(0x01, 0xf8);
		TP2850_write_cmos_sensor(0x02, 0x01);
		TP2850_write_cmos_sensor(0x08, 0x0f);
		TP2850_write_cmos_sensor(0x13, 0x24);
		TP2850_write_cmos_sensor(0x14, 0x46);
		TP2850_write_cmos_sensor(0x15, 0x08);
		TP2850_write_cmos_sensor(0x20, 0x12);
		TP2850_write_cmos_sensor(0x34, 0x1b);
		TP2850_write_cmos_sensor(0x25, 0x02);
		TP2850_write_cmos_sensor(0x26, 0x01);
		TP2850_write_cmos_sensor(0x27, 0x08);
		TP2850_write_cmos_sensor(0x10, 0x88);
		TP2850_write_cmos_sensor(0x10, 0x08);
		TP2850_write_cmos_sensor(0x23, 0x02);
		TP2850_write_cmos_sensor(0x40, 0x00);
	} else if (FORMAT_HD_30 == source_format) {
		TP2850_write_cmos_sensor(0x40, 0x00);
		TP2850_write_cmos_sensor(0x02, 0x46);
		TP2850_write_cmos_sensor(0x07, 0x80);
		TP2850_write_cmos_sensor(0x0B, 0xC0);
		TP2850_write_cmos_sensor(0x0C, 0x03);
		TP2850_write_cmos_sensor(0x0D, 0x70);
		TP2850_write_cmos_sensor(0x0E, 0x00);
		TP2850_write_cmos_sensor(0x0F, 0x00);
		TP2850_write_cmos_sensor(0x10, 0x00);
		TP2850_write_cmos_sensor(0x11, 0x40);
		TP2850_write_cmos_sensor(0x12, 0x40);
		TP2850_write_cmos_sensor(0x13, 0x00);
		TP2850_write_cmos_sensor(0x14, 0x00);
		TP2850_write_cmos_sensor(0x15, 0x13);
		TP2850_write_cmos_sensor(0x16, 0x16);
		TP2850_write_cmos_sensor(0x17, 0x00);
		TP2850_write_cmos_sensor(0x18, 0x19);
		TP2850_write_cmos_sensor(0x19, 0xD0);
		TP2850_write_cmos_sensor(0x1A, 0x25);
		TP2850_write_cmos_sensor(0x1B, 0x01);
		TP2850_write_cmos_sensor(0x1C, 0x06);
		TP2850_write_cmos_sensor(0x1D, 0x72);
		TP2850_write_cmos_sensor(0x20, 0x40);
		TP2850_write_cmos_sensor(0x21, 0x46);
		TP2850_write_cmos_sensor(0x22, 0x36);
		TP2850_write_cmos_sensor(0x23, 0x3C);
		TP2850_write_cmos_sensor(0x24, 0x04);
		TP2850_write_cmos_sensor(0x25, 0xFE);
		TP2850_write_cmos_sensor(0x26, 0x01);
		TP2850_write_cmos_sensor(0x27, 0x2D);
		TP2850_write_cmos_sensor(0x2A, 0x30);
		TP2850_write_cmos_sensor(0x2B, 0x60);
		TP2850_write_cmos_sensor(0x2C, 0x3A);
		TP2850_write_cmos_sensor(0x2D, 0x5A);
		TP2850_write_cmos_sensor(0x2E, 0x40);
		TP2850_write_cmos_sensor(0x30, 0x9D);
		TP2850_write_cmos_sensor(0x31, 0xCA);
		TP2850_write_cmos_sensor(0x32, 0x01);
		TP2850_write_cmos_sensor(0x33, 0xD0);
		TP2850_write_cmos_sensor(0x35, 0x25);
		TP2850_write_cmos_sensor(0x36, 0xDC);
		TP2850_write_cmos_sensor(0x38, 0x40);
		TP2850_write_cmos_sensor(0x39, 0x18);
		TP2850_write_cmos_sensor(0x3A, 0x32);
		TP2850_write_cmos_sensor(0x3B, 0x26);
		TP2850_write_cmos_sensor(0x41, 0x00);
		TP2850_write_cmos_sensor(0x4E, 0x00);
		TP2850_write_cmos_sensor(0x4F, 0x00);
		TP2850_write_cmos_sensor(0xFD, 0x80);
		TP2850_write_cmos_sensor(0x06, 0xB2);

		TP2850_write_cmos_sensor(0x40, 0x08);
		TP2850_write_cmos_sensor(0x01, 0xf8);
		TP2850_write_cmos_sensor(0x02, 0x01);
		TP2850_write_cmos_sensor(0x08, 0x0f);
		TP2850_write_cmos_sensor(0x13, 0x24);
		TP2850_write_cmos_sensor(0x14, 0x46);
		TP2850_write_cmos_sensor(0x15, 0x08);
		TP2850_write_cmos_sensor(0x20, 0x12);
		TP2850_write_cmos_sensor(0x34, 0x1b);
		TP2850_write_cmos_sensor(0x25, 0x02);
		TP2850_write_cmos_sensor(0x26, 0x01);
		TP2850_write_cmos_sensor(0x27, 0x08);
		TP2850_write_cmos_sensor(0x10, 0x88);
		TP2850_write_cmos_sensor(0x10, 0x08);
		TP2850_write_cmos_sensor(0x23, 0x02);
		TP2850_write_cmos_sensor(0x40, 0x00);
	}else if (FORMAT_FHD_25 == source_format) {
		TP2850_write_cmos_sensor(0x40, 0x00);
		TP2850_write_cmos_sensor(0x02, 0x44);
		TP2850_write_cmos_sensor(0x07, 0x80);
		TP2850_write_cmos_sensor(0x0B, 0xC0);
		TP2850_write_cmos_sensor(0x0C, 0x03);
		TP2850_write_cmos_sensor(0x0D, 0x73);
		TP2850_write_cmos_sensor(0x0E, 0x00);
		TP2850_write_cmos_sensor(0x0F, 0x00);
		TP2850_write_cmos_sensor(0x10, 0x00);
		TP2850_write_cmos_sensor(0x11, 0x40);
		TP2850_write_cmos_sensor(0x12, 0x40);
		TP2850_write_cmos_sensor(0x13, 0x00);
		TP2850_write_cmos_sensor(0x14, 0x00);
		TP2850_write_cmos_sensor(0x15, 0x01);
		TP2850_write_cmos_sensor(0x16, 0xF0);
		TP2850_write_cmos_sensor(0x17, 0x80);
		TP2850_write_cmos_sensor(0x18, 0x29);
		TP2850_write_cmos_sensor(0x19, 0x38);
		TP2850_write_cmos_sensor(0x1A, 0x47);
		TP2850_write_cmos_sensor(0x1B, 0x01);
		TP2850_write_cmos_sensor(0x1C, 0x0A);
		TP2850_write_cmos_sensor(0x1D, 0x50);
		TP2850_write_cmos_sensor(0x20, 0x3C);
		TP2850_write_cmos_sensor(0x21, 0x46);
		TP2850_write_cmos_sensor(0x22, 0x36);
		TP2850_write_cmos_sensor(0x23, 0x3C);
		TP2850_write_cmos_sensor(0x24, 0x04);
		TP2850_write_cmos_sensor(0x25, 0xFE);
		TP2850_write_cmos_sensor(0x26, 0x0D);
		TP2850_write_cmos_sensor(0x27, 0x2D);
		TP2850_write_cmos_sensor(0x2A, 0x30);
		TP2850_write_cmos_sensor(0x2B, 0x60);
		TP2850_write_cmos_sensor(0x2C, 0x1A);
		TP2850_write_cmos_sensor(0x2D, 0x54);
		TP2850_write_cmos_sensor(0x2E, 0x40);
		TP2850_write_cmos_sensor(0x30, 0xA5);
		TP2850_write_cmos_sensor(0x31, 0x86);
		TP2850_write_cmos_sensor(0x32, 0xFB);
		TP2850_write_cmos_sensor(0x33, 0x60);
		TP2850_write_cmos_sensor(0x35, 0x05);
		TP2850_write_cmos_sensor(0x36, 0xDC);
		TP2850_write_cmos_sensor(0x38, 0x40);
		TP2850_write_cmos_sensor(0x39, 0x1C);
		TP2850_write_cmos_sensor(0x3A, 0x32);
		TP2850_write_cmos_sensor(0x3B, 0x26);
		TP2850_write_cmos_sensor(0x41, 0x00);
		TP2850_write_cmos_sensor(0x4E, 0x00);
		TP2850_write_cmos_sensor(0x4F, 0x00);
		TP2850_write_cmos_sensor(0xFD, 0x80);
		TP2850_write_cmos_sensor(0x06, 0xB2);

		TP2850_write_cmos_sensor(0x40, 0x08);
		TP2850_write_cmos_sensor(0x01, 0xf8);
		TP2850_write_cmos_sensor(0x02, 0x01);
		TP2850_write_cmos_sensor(0x08, 0x0f);
		TP2850_write_cmos_sensor(0x13, 0x04);
		TP2850_write_cmos_sensor(0x14, 0x46);
		TP2850_write_cmos_sensor(0x15, 0x08);
		TP2850_write_cmos_sensor(0x20, 0x12);
		TP2850_write_cmos_sensor(0x34, 0x1b);
		TP2850_write_cmos_sensor(0x25, 0x02);
		TP2850_write_cmos_sensor(0x26, 0x01);
		TP2850_write_cmos_sensor(0x27, 0x08);
		TP2850_write_cmos_sensor(0x10, 0x88);
		TP2850_write_cmos_sensor(0x10, 0x08);
		TP2850_write_cmos_sensor(0x23, 0x02);
		TP2850_write_cmos_sensor(0x40, 0x00);

	}else if (FORMAT_FHD_30 == source_format) {
		TP2850_write_cmos_sensor(0x40, 0x00);
		TP2850_write_cmos_sensor(0x02, 0x44);
		TP2850_write_cmos_sensor(0x07, 0x80);
		TP2850_write_cmos_sensor(0x0B, 0xC0);
		TP2850_write_cmos_sensor(0x0C, 0x03);
		TP2850_write_cmos_sensor(0x0D, 0x72);
		TP2850_write_cmos_sensor(0x0E, 0x00);
		TP2850_write_cmos_sensor(0x0F, 0x00);
		TP2850_write_cmos_sensor(0x10, 0x00);
		TP2850_write_cmos_sensor(0x11, 0x40);
		TP2850_write_cmos_sensor(0x12, 0x40);
		TP2850_write_cmos_sensor(0x13, 0x00);
		TP2850_write_cmos_sensor(0x14, 0x00);
		TP2850_write_cmos_sensor(0x15, 0x01);
		TP2850_write_cmos_sensor(0x16, 0xF0);
		TP2850_write_cmos_sensor(0x17, 0x80);
		TP2850_write_cmos_sensor(0x18, 0x29);
		TP2850_write_cmos_sensor(0x19, 0x38);
		TP2850_write_cmos_sensor(0x1A, 0x47);
		TP2850_write_cmos_sensor(0x1B, 0x01);
		TP2850_write_cmos_sensor(0x1C, 0x08);
		TP2850_write_cmos_sensor(0x1D, 0x98);
		TP2850_write_cmos_sensor(0x20, 0x38);
		TP2850_write_cmos_sensor(0x21, 0x46);
		TP2850_write_cmos_sensor(0x22, 0x36);
		TP2850_write_cmos_sensor(0x23, 0x3C);
		TP2850_write_cmos_sensor(0x24, 0x04);
		TP2850_write_cmos_sensor(0x25, 0xFE);
		TP2850_write_cmos_sensor(0x26, 0x0D);
		TP2850_write_cmos_sensor(0x27, 0x2D);
		TP2850_write_cmos_sensor(0x2A, 0x30);
		TP2850_write_cmos_sensor(0x2B, 0x60);
		TP2850_write_cmos_sensor(0x2C, 0x3A);
		TP2850_write_cmos_sensor(0x2D, 0x54);
		TP2850_write_cmos_sensor(0x2E, 0x40);
		TP2850_write_cmos_sensor(0x30, 0xA5);
		TP2850_write_cmos_sensor(0x31, 0x95);
		TP2850_write_cmos_sensor(0x32, 0xE0);
		TP2850_write_cmos_sensor(0x33, 0x60);
		TP2850_write_cmos_sensor(0x35, 0x05);
		TP2850_write_cmos_sensor(0x36, 0xDC);
		TP2850_write_cmos_sensor(0x38, 0x40);
		TP2850_write_cmos_sensor(0x39, 0x1C);
		TP2850_write_cmos_sensor(0x3A, 0x32);
		TP2850_write_cmos_sensor(0x3B, 0x26);
		TP2850_write_cmos_sensor(0x41, 0x00);
		TP2850_write_cmos_sensor(0x4E, 0x00);
		TP2850_write_cmos_sensor(0x4F, 0x00);
		TP2850_write_cmos_sensor(0xFD, 0x80);
		TP2850_write_cmos_sensor(0x06, 0xB2);

		TP2850_write_cmos_sensor(0x40, 0x08);
		TP2850_write_cmos_sensor(0x01, 0xf8);
		TP2850_write_cmos_sensor(0x02, 0x01);
		TP2850_write_cmos_sensor(0x08, 0x0f);
		TP2850_write_cmos_sensor(0x13, 0x04);
		TP2850_write_cmos_sensor(0x14, 0x46);
		TP2850_write_cmos_sensor(0x15, 0x08);
		TP2850_write_cmos_sensor(0x20, 0x12);
		TP2850_write_cmos_sensor(0x34, 0x1b);
		TP2850_write_cmos_sensor(0x25, 0x02);
		TP2850_write_cmos_sensor(0x26, 0x01);
		TP2850_write_cmos_sensor(0x27, 0x08);
		TP2850_write_cmos_sensor(0x10, 0x88);
		TP2850_write_cmos_sensor(0x10, 0x08);
		TP2850_write_cmos_sensor(0x23, 0x02);
		TP2850_write_cmos_sensor(0x40, 0x00);
	}

}

static void TP2850_Clk_Enable(void)
{
	TP2850_write_cmos_sensor(0x40,0x08);
	TP2850_write_cmos_sensor(0x23,0x02);
	TP2850_write_cmos_sensor(0x23,0x00);
	TP2850_write_cmos_sensor(0x40,0x00);
}

static void TP2850_Hvlock_Detect(void)
{
	int retry = 100;
	int hv_status = 0;

	TP2850_write_cmos_sensor(0x40, 0x00);
	while (retry--) {
		hv_status = TP2850_read_cmos_sensor(0x01);
		if (0x7a == hv_status) {
			LOG_INF("HVlocked  = %d,status = 0x%x\n", (100-retry),hv_status);
			break;
		}
		mdelay(10);
	}

}
static VIDEO_FORMAT _TP2850_Format_Detect(void)
{
	uint32_t value = 0x00;
	VIDEO_FORMAT format = FORMAT_MAX;

	value = TP2850_read_cmos_sensor(0x03);
	value &= 0x07; //save bit0~2

	if (0x02 == value) {
		format = FORMAT_FHD_30;
	} else if (0x03 == value) {
		format = FORMAT_FHD_25;
	} else if (0x04 == value) {
		format = FORMAT_HD_30;
	} else if (0x05 == value) {
		format = FORMAT_HD_25;
	} else {
		format = FORMAT_MAX;
	}
	LOG_INF("the tmp format is %d, value is 0x%x\n", format, value);
	return format;
}


static UINT32 TP2850_GetSensorID(uint32_t *sensorId)
{
	int retry = 3;
	do {
		*sensorId = ((TP2850_read_cmos_sensor(0xfe) << 8) | TP2850_read_cmos_sensor(0xff));
		LOG_INF("sensor id = 0x%x\n", *sensorId);
		if (*sensorId == SENSOR_ID) {
			break;
		}
		retry--;
	} while (retry > 0);

	if (*sensorId != SENSOR_ID) {
		*sensorId = SENSOR_ID_ERROR;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	return ERROR_NONE;
}

UINT32 TP2850Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&TP2850SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 TP2850Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&TP2850SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 TP2850GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth = input_source_width;
	pSensorResolution->SensorFullHeight = input_source_height;
	pSensorResolution->SensorPreviewWidth = input_source_width;
	pSensorResolution->SensorPreviewHeight = input_source_height;
	pSensorResolution->SensorVideoWidth = input_source_width;
	pSensorResolution->SensorVideoHeight = input_source_height;
	pSensorResolution->SensorFrameRate = input_source_fps;
	pSensorResolution->SensorFormatInfo = SENSOR_FORMAT_UYVY;

	return ERROR_NONE;
}

SIGNAL_STATUS TP2850_SignalStatus_Get(void)
{
	UINT16 value = 0;
	SIGNAL_STATUS signal_status = SIGNAL_IDLE;

	value = TP2850_read_cmos_sensor(0x01);
	if ((value & 0x80) == 0x00 ) {
		signal_status = SIGNAL_READY;
	} else {
		signal_status = SIGNAL_LOST;
	}
	//LOG_INF("signal is change! pre_signal = %d.value = %d\n",signal_status,value);
	return signal_status;
}

static void TP2850_Param_Set(VIDEO_FORMAT source_format)
{
	switch (source_format){
	case FORMAT_HD_25:
		input_source_height = TP2850_IMAGE_SENSOR_HD25_HEIGHT;
		input_source_width = TP2850_IMAGE_SENSOR_HD25_WIDTH;
		input_source_fps = TP2850_FPS_25;
		break;
	case FORMAT_HD_30:
		input_source_height = TP2850_IMAGE_SENSOR_HD30_HEIGHT;
		input_source_width = TP2850_IMAGE_SENSOR_HD30_WIDTH;
		input_source_fps = TP2850_FPS_30;
		break;
	case FORMAT_FHD_25:
		input_source_height = TP2850_IMAGE_SENSOR_FHD25_HEIGHT;
		input_source_width = TP2850_IMAGE_SENSOR_FHD25_WIDTH;
		input_source_fps = TP2850_FPS_25;
		break;
	case FORMAT_FHD_30:
		input_source_height = TP2850_IMAGE_SENSOR_FHD30_HEIGHT;
		input_source_width = TP2850_IMAGE_SENSOR_FHD30_WIDTH;
		input_source_fps = TP2850_FPS_30;
		break;
	default:
		LOG_ERR("this may be impossible\n");
		break;
	}
}

static VIDEO_FORMAT TP2850_Format_Detect(void)
{
	int retry = 12;
	VIDEO_FORMAT pre_format = FORMAT_MAX ;
	VIDEO_FORMAT cur_format = FORMAT_MAX ;

	do {
		cur_format = _TP2850_Format_Detect();
		if ((pre_format == cur_format) && ( FORMAT_MAX != cur_format)) {
			TP2850_Param_Set(cur_format);
			break;
		}
		pre_format = cur_format;
		mdelay(50);
		retry--;
	} while (retry > 0);

	LOG_INF("detect format have try %d times\n", ((12 - retry) + 1));

	if(0 == retry) {
		input_source_height = TP2850_IMAGE_SENSOR_HD30_HEIGHT;
		input_source_width = TP2850_IMAGE_SENSOR_HD30_WIDTH;
		input_source_fps = TP2850_FPS_30;
		cur_format = FORMAT_MAX;
		LOG_ERR("format detect fail,use default param!\n");
	}
	LOG_INF("source_height = %d, source_width =%d, source_format =%d\n",input_source_height,input_source_width,cur_format);
	return cur_format;
}



UINT32 TP2850Open(void)
{
	UINT32 sensor_id = 0;
	UINT32 ret = 0;
	unsigned int value = 0;

	LOG_INF("TP2850Open Enter\n");

	ret = TP2850_GetSensorID(&sensor_id);
	if (ERROR_NONE != ret) {
		LOG_ERR("TP2850 YUV read sensor id failed\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}
#if 0
	if (MZ_SUCCESS != MetaZone_Read(METAZONE_AHD_FORMAT_DEFAULT, &value)) {
		LOG_ERR("TP2850 YUV metazone read fail\n");
	} else {
		LOG_INF("metazone value = %d\n",value);
	}
#endif
	if ((FORMAT_HD_25 <= value) && (FORMAT_FHD_30 >= value)) {
		source_format = value;
		TP2850_Param_Set(source_format);
	} else {
		TP2850_Sensor_PreInit();
		mdelay(100);
		source_format = TP2850_Format_Detect();
	}

	if (FORMAT_MAX != source_format) {
		TP2850_Sensor_Init(source_format);
		TP2850_Hvlock_Detect();
	}
	TP2850_Clk_Enable();
	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 TP2850Close(void)
{
	LOG_INF("TP2850Close");
	input_source_width = TP2850_IMAGE_SENSOR_HD30_WIDTH;
	input_source_height = TP2850_IMAGE_SENSOR_HD30_HEIGHT;
	input_source_fps = TP2850_FPS_30;
	return ERROR_NONE;
}


UINT32 TP2850GetInfo(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
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

	pSensorInfo->SensorOutputDataFormat = SENSOR_OUTPUT_FORMAT_UYVY;

	pSensorInfo->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; //inverse
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

	pSensorInfo->SensorInterruptDelayLines = 1;
	pSensorInfo->CaptureDelayFrame = 5;
	pSensorInfo->PreviewDelayFrame = 4;
	pSensorInfo->VideoDelayFrame = 5;
	pSensorInfo->YUVAwbDelayFrame = 0;
	pSensorInfo->YUVEffectDelayFrame = 0;
	pSensorInfo->AESensorGainDelayFrame = 0;
	pSensorInfo->AEISPGainDelayFrame = 2;
	pSensorInfo->SensorModeNum = 5;

	pSensorInfo->SensorMasterClockSwitch = 0;
	pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;
	pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_MIPI;
	pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;

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

	memcpy(pSensorConfigData, &TP2850SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	return ERROR_NONE;

}

UINT32 TP2850Control(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	LOG_INF("Enter, ScenarioId = %d\n", ScenarioId);
	switch (ScenarioId) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		TP2850Preview(pImageWindow, pSensorConfigData);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		TP2850Capture(pImageWindow, pSensorConfigData);
		break;

	default:
		break;
	}

	LOG_INF("Leave\n");

	return ERROR_NONE;
}

UINT32 TP2850FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
		TP2850_isp_master_clock = *pFeatureData32;
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		TP2850_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		pSensorRegData->RegData = TP2850_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
	case SENSOR_FEATURE_GET_CONFIG_PARA:
		memcpy(pSensorConfigData, &TP2850SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
		TP2850_GetSensorID(pFeatureData32);
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
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = input_source_fps;
		break;
	case SENSOR_FEATURE_GET_AHD_SIGNAL_STATUS:
		*pFeatureData32 = TP2850_SignalStatus_Get();
		break;
	case SENSOR_FEATURE_SET_BRIGHTNESS_VALUE:
		LOG_INF("brigntness value is %llu .\n", *(feature_data + 1));
		TP2850_write_cmos_sensor(0x10, *(feature_data + 1) >> 1);//default is 0, -64~63
		break;
	case SENSOR_FEATURE_SET_CONTRAST_VALUE:
		LOG_INF("contrast value is %llu .\n", *(feature_data + 1));
		TP2850_write_cmos_sensor(0x11, *(feature_data + 1) >> 1);//default is 64, 0~127
		break;
	case SENSOR_FEATURE_SET_SATURATION_VALUE:
		LOG_INF("saturation value is %llu .\n", *(feature_data + 1));
		TP2850_write_cmos_sensor(0x12, *(feature_data + 1) >> 1);//default is 64, 0~127
		break;
	case SENSOR_FEATURE_SET_HUE_VALUE:
		LOG_INF("hue value is %llu .\n", *(feature_data + 1));
		TP2850_write_cmos_sensor(0x13, *(feature_data + 1));//default is 0, -128~127
		break;
	default:
		break;
	}
	return ERROR_NONE;
}


static struct SENSOR_FUNCTION_STRUCT sensor_func =
{
	TP2850Open,
	TP2850GetInfo,
	TP2850GetResolution,
	TP2850FeatureControl,
	TP2850Control,
	TP2850Close,
	TP2850_SignalStatus_Get
};

UINT32 TP2850_YUV_SensorInit(
			struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL) {
		*pfFunc = &sensor_func;
	}
	return ERROR_NONE;
}






