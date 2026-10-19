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
*   N4_yuv_Sensor.c
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

#include "n4_yuv_sensor.h"
#include "metazone/metazone.h"
#include "metazone/metazone_ioctl.h"

#define METAZONE_FAST_AVM_MODE             0x10048

#define METAZONE_FAST_AVM_FMT              0x10046

/****************************Modify Following Strings for Debug****************************/
#define PFX "[N4]"


/****************************   Modify end    *******************************************/
#define LOG_DBG(format, args...)    pr_debug(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    pr_info(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...)    pr_err(PFX " %s " format, __FUNCTION__, ##args)

static kal_uint16 n4_i2c_write_id = 0x60;
static int n4_i2c_speed = 400;
//static VIDEO_FORMAT source_format = FORMAT_MAX;
kal_uint32 n4_isp_master_clock;
static int n4_is_avm = 1;
static int interlace_flag[4];
static int ch_fhd_mode[4]={0,0,0,0};
static int signal_lost_cnt = 0;
static int channel = 0;
static int pq_value = 0;

static kal_uint32 input_source_width = N4_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height = N4_IMAGE_SENSOR_HD_HEIGHT;

static kal_uint32 input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;

static kal_uint32 input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;

static kal_uint32 input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;

static kal_uint32 input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;

MSDK_SENSOR_CONFIG_STRUCT N4SensorConfigData;


static kal_uint16 n4_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[1] = { (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 1, (u8 *) &get_byte, 1, n4_i2c_write_id);

	return get_byte;
}

static int n4_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[2] = { (char)(addr & 0xFF), (char)(para & 0xFF) };

	return iWriteRegI2CTiming(
		pu_send_cmd, 2, n4_i2c_write_id, n4_i2c_speed);
}

static void n4_init_ch(int ch, int vfmt)
{
	LOG_INF("int ch = %d,vfmt = 0x%x\n",ch,vfmt);
	n4_write_cmos_sensor(0xFF, 0x00);
	n4_write_cmos_sensor(0x00+ch, 0x10);
	if ((CH_VALUE_HD25 == vfmt) || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25 == vfmt) || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x04+ch, 0x00);
	} else if (CH_VALUE_PAL==vfmt) {
		n4_write_cmos_sensor(0x04+ch, 0x05);
	} else if (CH_VALUE_NTSC==vfmt) {
		n4_write_cmos_sensor(0x04+ch, 0x04);
	}
	if (CH_VALUE_HD25 == vfmt) {
		n4_write_cmos_sensor(0x08+ch, 0x0D);
	} else if (CH_VALUE_HD30==vfmt) {
		n4_write_cmos_sensor(0x08+ch, 0x0C);
	} else if (CH_VALUE_FHD25 == vfmt) {
		n4_write_cmos_sensor(0x08+ch, 0x03);
	} else if (CH_VALUE_FHD30==vfmt) {
		n4_write_cmos_sensor(0x08+ch, 0x02);
	} else {
		n4_write_cmos_sensor(0x08+ch, 0x00);
	}

	n4_write_cmos_sensor(0x0c+ch, 0x00);
	if ((CH_VALUE_HD25== vfmt) || (CH_VALUE_HD30 == vfmt)) {
		n4_write_cmos_sensor(0x10+ch, 0x20);
	} else if (CH_VALUE_PAL == vfmt) {
		n4_write_cmos_sensor(0x10+ch, 0xdd);
	} else if (CH_VALUE_NTSC== vfmt) {
		n4_write_cmos_sensor(0x10+ch, 0xa0);
	}
	n4_write_cmos_sensor(0x14+ch, 0x00);
	n4_write_cmos_sensor(0x18+ch, 0x11);
	n4_write_cmos_sensor(0x1c+ch, 0x18);
	n4_write_cmos_sensor(0x20+ch, 0x00);
	if ((CH_VALUE_HD25 == vfmt) || (CH_VALUE_HD30 == vfmt)){
		n4_write_cmos_sensor(0x24+ch, 0x88);
		n4_write_cmos_sensor(0x28+ch, 0x84);
		n4_write_cmos_sensor(0x30+ch, 0x03);
		n4_write_cmos_sensor(0x34+ch, 0x0f);
	} else if ((CH_VALUE_FHD25 == vfmt) || (CH_VALUE_FHD30 == vfmt)){
		n4_write_cmos_sensor(0x24+ch, 0x86);
		n4_write_cmos_sensor(0x28+ch, 0x80);
		n4_write_cmos_sensor(0x30+ch, 0x00);
		n4_write_cmos_sensor(0x34+ch, 0x00);
	} else {
		n4_write_cmos_sensor(0x24+ch, 0x90);
		n4_write_cmos_sensor(0x28+ch, 0x90);
		n4_write_cmos_sensor(0x30+ch, 0x00);
		n4_write_cmos_sensor(0x34+ch, 0x00);
	}
	n4_write_cmos_sensor(0x40+ch, 0x00);
	n4_write_cmos_sensor(0x44+ch, 0x00);
	n4_write_cmos_sensor(0x48+ch, 0x00);
	n4_write_cmos_sensor(0x4c+ch, 0x00);
	n4_write_cmos_sensor(0x50+ch, 0x00);
	n4_write_cmos_sensor(0x58+ch, 0x80);
	n4_write_cmos_sensor(0x5c+ch, 0x82);
	n4_write_cmos_sensor(0x60+ch, 0x10);
	if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x64+ch, 0x05);
	} else if (CH_VALUE_PAL == vfmt) {
		n4_write_cmos_sensor(0x64+ch, 0x0a);
	} else if (CH_VALUE_NTSC== vfmt) {
		n4_write_cmos_sensor(0x64+ch, 0x1c);
	}
	if ((CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x68+ch, 0x48);
	} else if (CH_VALUE_HD25 == vfmt) {
		n4_write_cmos_sensor(0x68+ch, 0x38);
	} else if (CH_VALUE_HD30== vfmt) {
		n4_write_cmos_sensor(0x68+ch, 0x3e);
	} else if (CH_VALUE_PAL == vfmt) {
		n4_write_cmos_sensor(0x68+ch, 0xd8);
	} else if (CH_VALUE_NTSC == vfmt) {
		n4_write_cmos_sensor(0x68+ch, 0xd4);
	}
	n4_write_cmos_sensor(0x6c+ch, 0x00);
	n4_write_cmos_sensor(0x78+ch, 0x21);
	if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x7c+ch, 0x00);
	} else {
		n4_write_cmos_sensor(0x7c+ch, 0x8f);
	}
	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x7C, 0x00);
	if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x84+ch, 0x04);
		n4_write_cmos_sensor(0x88+ch, 0x01);
		n4_write_cmos_sensor(0x8c+ch, 0x02);
	//	n4_write_cmos_sensor(0xa0+ch, 0x00);
	} else {
		n4_write_cmos_sensor(0x84+ch, 0x06);
		n4_write_cmos_sensor(0x88+ch, 0x07);
		n4_write_cmos_sensor(0x8c+ch, 0x01);
		n4_write_cmos_sensor(0xa0+ch, 0x10);
	}
	n4_write_cmos_sensor(0xEC+ch, 0x00);

	n4_write_cmos_sensor(0xFF, 0x05+ch);
	n4_write_cmos_sensor(0x00, 0xd0);
	n4_write_cmos_sensor(0x01, 0x2c);
	n4_write_cmos_sensor(0x05, 0x24);
	n4_write_cmos_sensor(0x1d, 0x0c);
	n4_write_cmos_sensor(0x24, 0x2a);
	if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x25, 0xdc);
	} else if (CH_VALUE_PAL == vfmt) {
		n4_write_cmos_sensor(0x25, 0xcc);
	} else if (CH_VALUE_NTSC == vfmt) {
		n4_write_cmos_sensor(0x25, 0xdc);
	}
	n4_write_cmos_sensor(0x26, 0x40);
	n4_write_cmos_sensor(0x27, 0x57);
	n4_write_cmos_sensor(0x28, 0x80);
	n4_write_cmos_sensor(0x2b, 0xa8);
	if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x31, 0x82);
	} else if (CH_VALUE_PAL == vfmt) {
		n4_write_cmos_sensor(0x31, 0x02);
	} else if (CH_VALUE_NTSC == vfmt) {
		n4_write_cmos_sensor(0x31, 0x82);
	}
	n4_write_cmos_sensor(0x32, 0x10);
	if ((CH_VALUE_FHD25 == vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x38, 0x13);
		n4_write_cmos_sensor(0x47, 0xEE);
		n4_write_cmos_sensor(0x50, 0xc6);
	} else if ((CH_VALUE_HD25 == vfmt)  || (CH_VALUE_HD30 == vfmt)) {
		n4_write_cmos_sensor(0x38, 0x10);
		n4_write_cmos_sensor(0x47, 0xEE);
		n4_write_cmos_sensor(0x50, 0xc6);
	} else if (CH_VALUE_PAL == vfmt) {
		n4_write_cmos_sensor(0x38, 0x1f);
		n4_write_cmos_sensor(0x47, 0x04);
		n4_write_cmos_sensor(0x50, 0x84);
	} else {
		n4_write_cmos_sensor(0x38, 0x1d);
		n4_write_cmos_sensor(0x47, 0x04);
		n4_write_cmos_sensor(0x50, 0x84);
	}
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x57, 0x00);
	if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x58, 0x77);
	} else {
		n4_write_cmos_sensor(0x58, 0x70);
	}
	n4_write_cmos_sensor(0x59, 0x00);
	n4_write_cmos_sensor(0x5C, 0x78);
	n4_write_cmos_sensor(0x5F, 0x00);
	n4_write_cmos_sensor(0x62, 0x20);
	if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x64, 0x00);
	} else {
		n4_write_cmos_sensor(0x64, 0x01);
	}
	n4_write_cmos_sensor(0x65, 0x00);
	n4_write_cmos_sensor(0x69, 0x00);
	n4_write_cmos_sensor(0x82, 0x00);  //important!!!!
	if ((CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x6E, 0x00);
		n4_write_cmos_sensor(0x6F, 0x00);
		n4_write_cmos_sensor(0x90, 0x01);
	} else if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt)) {
		n4_write_cmos_sensor(0x6E, 0x10);
		n4_write_cmos_sensor(0x6F, 0x1C);
		n4_write_cmos_sensor(0x90, 0x01);
	} else {
		n4_write_cmos_sensor(0x6E, 0x00);
		n4_write_cmos_sensor(0x6F, 0x00);
		if ( CH_VALUE_PAL== vfmt) {
			n4_write_cmos_sensor(0x90, 0x0d);
		} else {
			n4_write_cmos_sensor(0x90, 0x01);
		}
	}
	n4_write_cmos_sensor(0x92, 0x00);
	n4_write_cmos_sensor(0x94, 0x00);
	n4_write_cmos_sensor(0x95, 0x00);
	if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0xa9, 0x00);
		n4_write_cmos_sensor(0xb5, 0x80);
	} else if (CH_VALUE_PAL == vfmt) {
		n4_write_cmos_sensor(0xa9, 0x0a);
		n4_write_cmos_sensor(0xb5, 0x00);
	} else {
		n4_write_cmos_sensor(0xa9, 0x1c);
		n4_write_cmos_sensor(0xb5, 0x00);
	}
	n4_write_cmos_sensor(0xb7, 0xfc);
	if ((CH_VALUE_NTSC== vfmt) || (CH_VALUE_PAL == vfmt)) {
		n4_write_cmos_sensor(0xb8, 0xb9);
	} else {
		n4_write_cmos_sensor(0xb8, 0x39);
	}
	n4_write_cmos_sensor(0xb9, 0x72);
	n4_write_cmos_sensor(0xbb, 0x0f);
	n4_write_cmos_sensor(0xd1, 0x30);
	n4_write_cmos_sensor(0xd5, 0x80);

	n4_write_cmos_sensor(0xFF, 0x09);
	if ((CH_VALUE_HD25== vfmt)  || (CH_VALUE_HD30 == vfmt) || (CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		n4_write_cmos_sensor(0x96+ch*0x20, 0x00);
		n4_write_cmos_sensor(0x97+ch*0x20, 0x00);
	} else {
		n4_write_cmos_sensor(0x96+ch*0x20, 0x10);
		n4_write_cmos_sensor(0x97+ch*0x20, 0x10);
	}
	if ((CH_VALUE_FHD25== vfmt)  || (CH_VALUE_FHD30 == vfmt)) {
		ch_fhd_mode[ch] = 1;
	} else {
		ch_fhd_mode[ch] = 0;
	}

}


void n4_mipi_outputs(int mipi_ch_en)
{
	int ch;
	unsigned char reg_value;
	LOG_INF("n4_mipi_outputs mipi_ch_en = 0x%x\n",mipi_ch_en);
	n4_write_cmos_sensor(0xFF, 0x20);
	reg_value = n4_read_cmos_sensor(0x01);
	for(ch=0;ch<4;ch++){
		if( 1 == ch_fhd_mode[ch] ) {
			reg_value &= (~(0x03<<(ch*2)));
		} else {
			reg_value |= (0x01<<(ch*2));
		}
	}
	n4_write_cmos_sensor(0x01, reg_value); //0x55
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x40, 0x01);
	n4_write_cmos_sensor(0x0F, 0x00);
	n4_write_cmos_sensor(0x0D, 0x01);  //4lane mode
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0xff, 0x20);
	n4_write_cmos_sensor(0x00, mipi_ch_en);  //ch1/2/3/4 enabled FF
}


static void n4_sensor_ntsc_init(void)
{
	LOG_INF("sensor_init in");
	n4_write_cmos_sensor(0xff, 0x04);
	n4_write_cmos_sensor(0xa0, 0x24);
	n4_write_cmos_sensor(0xa1, 0x24);
	n4_write_cmos_sensor(0xa2, 0x24);
	n4_write_cmos_sensor(0xa3, 0x24);
	n4_write_cmos_sensor(0xa4, 0x24);
	n4_write_cmos_sensor(0xa5, 0x24);
	n4_write_cmos_sensor(0xa6, 0x24);
	n4_write_cmos_sensor(0xa7, 0x24);
	n4_write_cmos_sensor(0xa8, 0x24);
	n4_write_cmos_sensor(0xa9, 0x24);
	n4_write_cmos_sensor(0xaa, 0x24);
	n4_write_cmos_sensor(0xab, 0x24);
	n4_write_cmos_sensor(0xac, 0x24);
	n4_write_cmos_sensor(0xad, 0x24);
	n4_write_cmos_sensor(0xae, 0x24);
	n4_write_cmos_sensor(0xaf, 0x24);
	n4_write_cmos_sensor(0xb0, 0x24);
	n4_write_cmos_sensor(0xb1, 0x24);
	n4_write_cmos_sensor(0xb2, 0x24);
	n4_write_cmos_sensor(0xb3, 0x24);
	n4_write_cmos_sensor(0xb4, 0x24);
	n4_write_cmos_sensor(0xb5, 0x24);
	n4_write_cmos_sensor(0xb6, 0x24);
	n4_write_cmos_sensor(0xb7, 0x24);
	n4_write_cmos_sensor(0xb8, 0x24);
	n4_write_cmos_sensor(0xb9, 0x24);
	n4_write_cmos_sensor(0xba, 0x24);
	n4_write_cmos_sensor(0xbb, 0x24);
	n4_write_cmos_sensor(0xbc, 0x24);
	n4_write_cmos_sensor(0xbd, 0x24);
	n4_write_cmos_sensor(0xbe, 0x24);
	n4_write_cmos_sensor(0xbf, 0x24);
	n4_write_cmos_sensor(0xc0, 0x24);
	n4_write_cmos_sensor(0xc1, 0x24);
	n4_write_cmos_sensor(0xc2, 0x24);
	n4_write_cmos_sensor(0xc3, 0x24);
	n4_write_cmos_sensor(0xff, 0x21);
	n4_write_cmos_sensor(0x07, 0x80);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0xff, 0x0A);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);
	n4_write_cmos_sensor(0xff, 0x0B);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x40, 0xAC);
	n4_write_cmos_sensor(0x41, 0x10);
	n4_write_cmos_sensor(0x42, 0x03);
	n4_write_cmos_sensor(0x43, 0x43);
	n4_write_cmos_sensor(0x11, 0x04);
	n4_write_cmos_sensor(0x10, 0x0A);
	n4_write_cmos_sensor(0x12, 0x06);
	n4_write_cmos_sensor(0x13, 0x09);
	n4_write_cmos_sensor(0x17, 0x01);
	n4_write_cmos_sensor(0x18, 0x0D);
	n4_write_cmos_sensor(0x15, 0x04);
	n4_write_cmos_sensor(0x14, 0x16);
	n4_write_cmos_sensor(0x16, 0x05);
	n4_write_cmos_sensor(0x19, 0x05);
	n4_write_cmos_sensor(0x1A, 0x0A);
	n4_write_cmos_sensor(0x1B, 0x08);
	n4_write_cmos_sensor(0x1C, 0x07);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x49, 0xF3);
	n4_write_cmos_sensor(0x49, 0xF0);
	n4_write_cmos_sensor(0x44, 0x02);
	n4_write_cmos_sensor(0x08, 0x40);
	n4_write_cmos_sensor(0x0F, 0x01);
	n4_write_cmos_sensor(0x38, 0x1E);
	n4_write_cmos_sensor(0x39, 0x1E);
	n4_write_cmos_sensor(0x3A, 0x1E);
	n4_write_cmos_sensor(0x3B, 0x1E);
	n4_write_cmos_sensor(0x07, 0x0F);
	n4_write_cmos_sensor(0x2D, 0x01);
	n4_write_cmos_sensor(0x45, 0x02);
	n4_write_cmos_sensor(0xFF, 0x13);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);

	n4_write_cmos_sensor(0xFF, 0x00);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x01, 0x00);
	n4_write_cmos_sensor(0x02, 0x00);
	n4_write_cmos_sensor(0x03, 0x00);
	n4_write_cmos_sensor(0x04, 0x04);
	n4_write_cmos_sensor(0x05, 0x04);
	n4_write_cmos_sensor(0x06, 0x04);
	n4_write_cmos_sensor(0x07, 0x04);
	n4_write_cmos_sensor(0x08, 0x00);
	n4_write_cmos_sensor(0x09, 0x00);
	n4_write_cmos_sensor(0x0a, 0x00);
	n4_write_cmos_sensor(0x0b, 0x00);
	n4_write_cmos_sensor(0x0c, 0x00);
	n4_write_cmos_sensor(0x0d, 0x00);
	n4_write_cmos_sensor(0x0e, 0x00);
	n4_write_cmos_sensor(0x0f, 0x00);
	n4_write_cmos_sensor(0x10, 0xa0);
	n4_write_cmos_sensor(0x11, 0xa0);
	n4_write_cmos_sensor(0x12, 0xa0);
	n4_write_cmos_sensor(0x13, 0xa0);
	n4_write_cmos_sensor(0x14, 0x00);
	n4_write_cmos_sensor(0x15, 0x01);
	n4_write_cmos_sensor(0x16, 0x02);
	n4_write_cmos_sensor(0x17, 0x03);
	n4_write_cmos_sensor(0x18, 0x11);
	n4_write_cmos_sensor(0x19, 0x11);
	n4_write_cmos_sensor(0x1a, 0x11);
	n4_write_cmos_sensor(0x1b, 0x11);
	n4_write_cmos_sensor(0x1c, 0x18);
	n4_write_cmos_sensor(0x1d, 0x18);
	n4_write_cmos_sensor(0x1e, 0x18);
	n4_write_cmos_sensor(0x1f, 0x18);
	n4_write_cmos_sensor(0x20, 0x00);
	n4_write_cmos_sensor(0x21, 0x00);
	n4_write_cmos_sensor(0x22, 0x00);
	n4_write_cmos_sensor(0x23, 0x00);
	n4_write_cmos_sensor(0x24, 0x90);
	n4_write_cmos_sensor(0x25, 0x90);
	n4_write_cmos_sensor(0x26, 0x90);
	n4_write_cmos_sensor(0x27, 0x90);
	n4_write_cmos_sensor(0x28, 0x90);
	n4_write_cmos_sensor(0x29, 0x90);
	n4_write_cmos_sensor(0x2a, 0x90);
	n4_write_cmos_sensor(0x2b, 0x90);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);
	n4_write_cmos_sensor(0x33, 0x00);
	n4_write_cmos_sensor(0x34, 0x00);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0x00);
	n4_write_cmos_sensor(0x37, 0x00);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x48, 0x00);
	n4_write_cmos_sensor(0x49, 0x00);
	n4_write_cmos_sensor(0x4a, 0x00);
	n4_write_cmos_sensor(0x4b, 0x00);
	n4_write_cmos_sensor(0x4c, 0x00);
	n4_write_cmos_sensor(0x4d, 0x00);
	n4_write_cmos_sensor(0x4e, 0x00);
	n4_write_cmos_sensor(0x4f, 0x00);
	n4_write_cmos_sensor(0x50, 0x00);
	n4_write_cmos_sensor(0x51, 0x00);
	n4_write_cmos_sensor(0x52, 0x00);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x58, 0x80);
	n4_write_cmos_sensor(0x59, 0x80);
	n4_write_cmos_sensor(0x5a, 0x80);
	n4_write_cmos_sensor(0x5b, 0x80);
	n4_write_cmos_sensor(0x5c, 0x82);
	n4_write_cmos_sensor(0x5d, 0x82);
	n4_write_cmos_sensor(0x5e, 0x82);
	n4_write_cmos_sensor(0x5f, 0x82);
	n4_write_cmos_sensor(0x60, 0x10);
	n4_write_cmos_sensor(0x61, 0x10);
	n4_write_cmos_sensor(0x62, 0x10);
	n4_write_cmos_sensor(0x63, 0x10);
	n4_write_cmos_sensor(0x64, 0x1c);
	n4_write_cmos_sensor(0x65, 0x1c);
	n4_write_cmos_sensor(0x66, 0x1c);
	n4_write_cmos_sensor(0x67, 0x1c);
	n4_write_cmos_sensor(0x68, 0xd4);
	n4_write_cmos_sensor(0x69, 0xd4);
	n4_write_cmos_sensor(0x6a, 0xd4);
	n4_write_cmos_sensor(0x6b, 0xd4);
	n4_write_cmos_sensor(0x6c, 0x00);
	n4_write_cmos_sensor(0x6d, 0x00);
	n4_write_cmos_sensor(0x6e, 0x00);
	n4_write_cmos_sensor(0x6f, 0x00);
	n4_write_cmos_sensor(0x78, 0xc0);
	n4_write_cmos_sensor(0x79, 0xc0);
	n4_write_cmos_sensor(0x7a, 0xc0);
	n4_write_cmos_sensor(0x7b, 0xc0);
	n4_write_cmos_sensor(0x7c, 0x8f);
	n4_write_cmos_sensor(0x7d, 0x8f);
	n4_write_cmos_sensor(0x7e, 0x8f);
	n4_write_cmos_sensor(0x7f, 0x8f);
	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x7C, 0x00);
	n4_write_cmos_sensor(0x84, 0x06);
	n4_write_cmos_sensor(0x85, 0x06);
	n4_write_cmos_sensor(0x86, 0x06);
	n4_write_cmos_sensor(0x87, 0x06);
	n4_write_cmos_sensor(0x88, 0x07);
	n4_write_cmos_sensor(0x89, 0x07);
	n4_write_cmos_sensor(0x8a, 0x07);
	n4_write_cmos_sensor(0x8b, 0x07);
	n4_write_cmos_sensor(0x8c, 0x01);
	n4_write_cmos_sensor(0x8d, 0x01);
	n4_write_cmos_sensor(0x8e, 0x01);
	n4_write_cmos_sensor(0x8f, 0x01);
	n4_write_cmos_sensor(0xa0, 0x10); //when 720P,this value is 0x00
	n4_write_cmos_sensor(0xa1, 0x10);
	n4_write_cmos_sensor(0xa2, 0x10);
	n4_write_cmos_sensor(0xa3, 0x10);
	n4_write_cmos_sensor(0xEC, 0x00);
	n4_write_cmos_sensor(0xED, 0x00);
	n4_write_cmos_sensor(0xEE, 0x00);
	n4_write_cmos_sensor(0xEF, 0x00);

	n4_write_cmos_sensor(0xFF, 0x05);
	n4_write_cmos_sensor(0x00, 0xd0);
	n4_write_cmos_sensor(0x01, 0x2c);
	n4_write_cmos_sensor(0x05, 0x20);
	n4_write_cmos_sensor(0x1d, 0x0c);
	n4_write_cmos_sensor(0x21, 0x24);
	n4_write_cmos_sensor(0x24, 0x2a);
	n4_write_cmos_sensor(0x25, 0xdc);
	n4_write_cmos_sensor(0x26, 0x40);
	n4_write_cmos_sensor(0x27, 0x57);
	n4_write_cmos_sensor(0x28, 0x80);
	n4_write_cmos_sensor(0x2b, 0xa8);
	n4_write_cmos_sensor(0x31, 0x82);
	n4_write_cmos_sensor(0x32, 0x10);
	n4_write_cmos_sensor(0x38, 0x1d);
	n4_write_cmos_sensor(0x47, 0x04);
	n4_write_cmos_sensor(0x50, 0x80);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x57, 0x00);
	n4_write_cmos_sensor(0x58, 0x70);
	n4_write_cmos_sensor(0x59, 0x00);
	n4_write_cmos_sensor(0x5C, 0x78);
	n4_write_cmos_sensor(0x5F, 0x00);
	n4_write_cmos_sensor(0x62, 0x20);
	n4_write_cmos_sensor(0x64, 0x01);
	n4_write_cmos_sensor(0x65, 0x00);
	n4_write_cmos_sensor(0x69, 0x00);
	n4_write_cmos_sensor(0x6E, 0x00);
	n4_write_cmos_sensor(0x6F, 0x00);
	n4_write_cmos_sensor(0x90, 0x01);
	n4_write_cmos_sensor(0x92, 0x00);
	n4_write_cmos_sensor(0x94, 0x00);
	n4_write_cmos_sensor(0x95, 0x00);
	n4_write_cmos_sensor(0xa9, 0x1c);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb7, 0xfc);
	n4_write_cmos_sensor(0xb8, 0xb9);
	n4_write_cmos_sensor(0xb9, 0x72);
	n4_write_cmos_sensor(0xbb, 0x0f);
	n4_write_cmos_sensor(0xd1, 0x30);
	n4_write_cmos_sensor(0xd5, 0x80);

	n4_write_cmos_sensor(0xFF, 0x09);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x50, 0x30);
	n4_write_cmos_sensor(0x51, 0x6f);
	n4_write_cmos_sensor(0x52, 0x67);
	n4_write_cmos_sensor(0x53, 0x48);
	n4_write_cmos_sensor(0x54, 0x30);
	n4_write_cmos_sensor(0x55, 0x6f);
	n4_write_cmos_sensor(0x56, 0x67);
	n4_write_cmos_sensor(0x57, 0x48);
	n4_write_cmos_sensor(0x58, 0x30);
	n4_write_cmos_sensor(0x59, 0x6f);
	n4_write_cmos_sensor(0x5a, 0x67);
	n4_write_cmos_sensor(0x5b, 0x48);
	n4_write_cmos_sensor(0x5c, 0x30);
	n4_write_cmos_sensor(0x5d, 0x6f);
	n4_write_cmos_sensor(0x5e, 0x67);
	n4_write_cmos_sensor(0x5f, 0x48);
	n4_write_cmos_sensor(0x96, 0x10);
	n4_write_cmos_sensor(0x97, 0x10);
	n4_write_cmos_sensor(0x98, 0x00);
	n4_write_cmos_sensor(0x99, 0x00);
	n4_write_cmos_sensor(0x9a, 0x00);
	n4_write_cmos_sensor(0x9b, 0x00);
	n4_write_cmos_sensor(0x9c, 0x00);
	n4_write_cmos_sensor(0x9d, 0x00);
	n4_write_cmos_sensor(0x9e, 0x00);
	n4_write_cmos_sensor(0xb6, 0x10);
	n4_write_cmos_sensor(0xb7, 0x10);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x00);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbe, 0x00);
	n4_write_cmos_sensor(0xd6, 0x10);
	n4_write_cmos_sensor(0xd7, 0x10);
	n4_write_cmos_sensor(0xd8, 0x00);
	n4_write_cmos_sensor(0xd9, 0x00);
	n4_write_cmos_sensor(0xda, 0x00);
	n4_write_cmos_sensor(0xdb, 0x00);
	n4_write_cmos_sensor(0xdc, 0x00);
	n4_write_cmos_sensor(0xdd, 0x00);
	n4_write_cmos_sensor(0xde, 0x00);
	n4_write_cmos_sensor(0xf6, 0x10);
	n4_write_cmos_sensor(0xf7, 0x10);
	n4_write_cmos_sensor(0xf8, 0x00);
	n4_write_cmos_sensor(0xf9, 0x00);
	n4_write_cmos_sensor(0xfa, 0x00);
	n4_write_cmos_sensor(0xfb, 0x00);
	n4_write_cmos_sensor(0xfc, 0x00);
	n4_write_cmos_sensor(0xfd, 0x00);
	n4_write_cmos_sensor(0xfe, 0x00);

	n4_write_cmos_sensor(0xff, 0x0a);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xff, 0x0b);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x3E, 0x00);
	n4_write_cmos_sensor(0x3F, 0x00);
	n4_write_cmos_sensor(0xFF, 0x20);
	n4_write_cmos_sensor(0x01, 0x55);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x0F, 0x00);
	n4_write_cmos_sensor(0x0D, 0x01);
	n4_write_cmos_sensor(0x00, 0xFF);
	n4_write_cmos_sensor(0x40, 0x01);
	n4_write_cmos_sensor(0x40, 0x00);//mipi reset

	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0xC8, 0x00);
	n4_write_cmos_sensor(0xC9, 0x00);
	n4_write_cmos_sensor(0xCA, 0x00);
	n4_write_cmos_sensor(0xCB, 0x00);

}

static void n4_sensor_pal_init(void)
{
	n4_write_cmos_sensor(0xff, 0x04);
	n4_write_cmos_sensor(0xa0, 0x24);
	n4_write_cmos_sensor(0xa1, 0x24);
	n4_write_cmos_sensor(0xa2, 0x24);
	n4_write_cmos_sensor(0xa3, 0x24);
	n4_write_cmos_sensor(0xa4, 0x24);
	n4_write_cmos_sensor(0xa5, 0x24);
	n4_write_cmos_sensor(0xa6, 0x24);
	n4_write_cmos_sensor(0xa7, 0x24);
	n4_write_cmos_sensor(0xa8, 0x24);
	n4_write_cmos_sensor(0xa9, 0x24);
	n4_write_cmos_sensor(0xaa, 0x24);
	n4_write_cmos_sensor(0xab, 0x24);
	n4_write_cmos_sensor(0xac, 0x24);
	n4_write_cmos_sensor(0xad, 0x24);
	n4_write_cmos_sensor(0xae, 0x24);
	n4_write_cmos_sensor(0xaf, 0x24);
	n4_write_cmos_sensor(0xb0, 0x24);
	n4_write_cmos_sensor(0xb1, 0x24);
	n4_write_cmos_sensor(0xb2, 0x24);
	n4_write_cmos_sensor(0xb3, 0x24);
	n4_write_cmos_sensor(0xb4, 0x24);
	n4_write_cmos_sensor(0xb5, 0x24);
	n4_write_cmos_sensor(0xb6, 0x24);
	n4_write_cmos_sensor(0xb7, 0x24);
	n4_write_cmos_sensor(0xb8, 0x24);
	n4_write_cmos_sensor(0xb9, 0x24);
	n4_write_cmos_sensor(0xba, 0x24);
	n4_write_cmos_sensor(0xbb, 0x24);
	n4_write_cmos_sensor(0xbc, 0x24);
	n4_write_cmos_sensor(0xbd, 0x24);
	n4_write_cmos_sensor(0xbe, 0x24);
	n4_write_cmos_sensor(0xbf, 0x24);
	n4_write_cmos_sensor(0xc0, 0x24);
	n4_write_cmos_sensor(0xc1, 0x24);
	n4_write_cmos_sensor(0xc2, 0x24);
	n4_write_cmos_sensor(0xc3, 0x24);
	n4_write_cmos_sensor(0xff, 0x21);
	n4_write_cmos_sensor(0x07, 0x80);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0xff, 0x0A);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);
	n4_write_cmos_sensor(0xff, 0x0B);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x40, 0xAC);
	n4_write_cmos_sensor(0x41, 0x10);
	n4_write_cmos_sensor(0x42, 0x03);
	n4_write_cmos_sensor(0x43, 0x43);
	n4_write_cmos_sensor(0x11, 0x04);
	n4_write_cmos_sensor(0x10, 0x0A);
	n4_write_cmos_sensor(0x12, 0x06);
	n4_write_cmos_sensor(0x13, 0x09);
	n4_write_cmos_sensor(0x17, 0x01);
	n4_write_cmos_sensor(0x18, 0x0D);
	n4_write_cmos_sensor(0x15, 0x04);
	n4_write_cmos_sensor(0x14, 0x16);
	n4_write_cmos_sensor(0x16, 0x05);
	n4_write_cmos_sensor(0x19, 0x05);
	n4_write_cmos_sensor(0x1A, 0x0A);
	n4_write_cmos_sensor(0x1B, 0x08);
	n4_write_cmos_sensor(0x1C, 0x07);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x49, 0xF3);
	n4_write_cmos_sensor(0x49, 0xF0);
	n4_write_cmos_sensor(0x44, 0x02);
	n4_write_cmos_sensor(0x08, 0x40);
	n4_write_cmos_sensor(0x0F, 0x01);
	n4_write_cmos_sensor(0x38, 0x1E);
	n4_write_cmos_sensor(0x39, 0x1E);
	n4_write_cmos_sensor(0x3A, 0x1E);
	n4_write_cmos_sensor(0x3B, 0x1E);
	n4_write_cmos_sensor(0x07, 0x0F);
	n4_write_cmos_sensor(0x2D, 0x01);
	n4_write_cmos_sensor(0x45, 0x02);
	n4_write_cmos_sensor(0xFF, 0x13);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);

	n4_write_cmos_sensor(0xFF, 0x00);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x01, 0x00);
	n4_write_cmos_sensor(0x02, 0x00);
	n4_write_cmos_sensor(0x03, 0x00);
	n4_write_cmos_sensor(0x04, 0x05);
	n4_write_cmos_sensor(0x05, 0x05);
	n4_write_cmos_sensor(0x06, 0x05);
	n4_write_cmos_sensor(0x07, 0x05);
	n4_write_cmos_sensor(0x08, 0x00);
	n4_write_cmos_sensor(0x09, 0x00);
	n4_write_cmos_sensor(0x0a, 0x00);
	n4_write_cmos_sensor(0x0b, 0x00);
	n4_write_cmos_sensor(0x0c, 0x00);
	n4_write_cmos_sensor(0x0d, 0x00);
	n4_write_cmos_sensor(0x0e, 0x00);
	n4_write_cmos_sensor(0x0f, 0x00);
	n4_write_cmos_sensor(0x10, 0xdd);
	n4_write_cmos_sensor(0x11, 0xdd);
	n4_write_cmos_sensor(0x12, 0xdd);
	n4_write_cmos_sensor(0x13, 0xdd);
	n4_write_cmos_sensor(0x14, 0x00);
	n4_write_cmos_sensor(0x15, 0x01);
	n4_write_cmos_sensor(0x16, 0x02);
	n4_write_cmos_sensor(0x17, 0x03);
	n4_write_cmos_sensor(0x18, 0x11);
	n4_write_cmos_sensor(0x19, 0x11);
	n4_write_cmos_sensor(0x1a, 0x11);
	n4_write_cmos_sensor(0x1b, 0x11);
	n4_write_cmos_sensor(0x1c, 0x18);
	n4_write_cmos_sensor(0x1d, 0x18);
	n4_write_cmos_sensor(0x1e, 0x18);
	n4_write_cmos_sensor(0x1f, 0x18);
	n4_write_cmos_sensor(0x20, 0x00);
	n4_write_cmos_sensor(0x21, 0x00);
	n4_write_cmos_sensor(0x22, 0x00);
	n4_write_cmos_sensor(0x23, 0x00);
	n4_write_cmos_sensor(0x24, 0x90);
	n4_write_cmos_sensor(0x25, 0x90);
	n4_write_cmos_sensor(0x26, 0x90);
	n4_write_cmos_sensor(0x27, 0x90);
	n4_write_cmos_sensor(0x28, 0x90);
	n4_write_cmos_sensor(0x29, 0x90);
	n4_write_cmos_sensor(0x2a, 0x90);
	n4_write_cmos_sensor(0x2b, 0x90);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);
	n4_write_cmos_sensor(0x33, 0x00);
	n4_write_cmos_sensor(0x34, 0x00);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0x00);
	n4_write_cmos_sensor(0x37, 0x00);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x48, 0x00);
	n4_write_cmos_sensor(0x49, 0x00);
	n4_write_cmos_sensor(0x4a, 0x00);
	n4_write_cmos_sensor(0x4b, 0x00);
	n4_write_cmos_sensor(0x4c, 0x00);
	n4_write_cmos_sensor(0x4d, 0x00);
	n4_write_cmos_sensor(0x4e, 0x00);
	n4_write_cmos_sensor(0x4f, 0x00);
	n4_write_cmos_sensor(0x50, 0x00);
	n4_write_cmos_sensor(0x51, 0x00);
	n4_write_cmos_sensor(0x52, 0x00);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x58, 0x80);
	n4_write_cmos_sensor(0x59, 0x80);
	n4_write_cmos_sensor(0x5a, 0x80);
	n4_write_cmos_sensor(0x5b, 0x80);
	n4_write_cmos_sensor(0x5c, 0x82);
	n4_write_cmos_sensor(0x5d, 0x82);
	n4_write_cmos_sensor(0x5e, 0x82);
	n4_write_cmos_sensor(0x5f, 0x82);
	n4_write_cmos_sensor(0x60, 0x10);
	n4_write_cmos_sensor(0x61, 0x10);
	n4_write_cmos_sensor(0x62, 0x10);
	n4_write_cmos_sensor(0x63, 0x10);
	n4_write_cmos_sensor(0x64, 0x0a);
	n4_write_cmos_sensor(0x65, 0x0a);
	n4_write_cmos_sensor(0x66, 0x0a);
	n4_write_cmos_sensor(0x67, 0x0a);
	n4_write_cmos_sensor(0x68, 0xd8);
	n4_write_cmos_sensor(0x69, 0xd8);
	n4_write_cmos_sensor(0x6a, 0xd8);
	n4_write_cmos_sensor(0x6b, 0xd8);
	n4_write_cmos_sensor(0x6c, 0x00);
	n4_write_cmos_sensor(0x6d, 0x00);
	n4_write_cmos_sensor(0x6e, 0x00);
	n4_write_cmos_sensor(0x6f, 0x00);

	n4_write_cmos_sensor(0x78, 0x21);
	n4_write_cmos_sensor(0x79, 0x21);
	n4_write_cmos_sensor(0x7a, 0x21);
	n4_write_cmos_sensor(0x7b, 0x21);
	n4_write_cmos_sensor(0x7c, 0x8f);
	n4_write_cmos_sensor(0x7d, 0x8f);
	n4_write_cmos_sensor(0x7e, 0x8f);
	n4_write_cmos_sensor(0x7f, 0x8f);
	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x7C, 0x00);
	n4_write_cmos_sensor(0x84, 0x06);
	n4_write_cmos_sensor(0x85, 0x06);
	n4_write_cmos_sensor(0x86, 0x06);
	n4_write_cmos_sensor(0x87, 0x06);
	n4_write_cmos_sensor(0x88, 0x07);
	n4_write_cmos_sensor(0x89, 0x07);
	n4_write_cmos_sensor(0x8a, 0x07);
	n4_write_cmos_sensor(0x8b, 0x07);
	n4_write_cmos_sensor(0x8c, 0x01);
	n4_write_cmos_sensor(0x8d, 0x01);
	n4_write_cmos_sensor(0x8e, 0x01);
	n4_write_cmos_sensor(0x8f, 0x01);
	n4_write_cmos_sensor(0xa0, 0x10); //when 720P,this value is 0x00
	n4_write_cmos_sensor(0xa1, 0x10);
	n4_write_cmos_sensor(0xa2, 0x10);
	n4_write_cmos_sensor(0xa3, 0x10);
	n4_write_cmos_sensor(0xEC, 0x00);
	n4_write_cmos_sensor(0xED, 0x00);
	n4_write_cmos_sensor(0xEE, 0x00);
	n4_write_cmos_sensor(0xEF, 0x00);

	n4_write_cmos_sensor(0xFF, 0x05);
	n4_write_cmos_sensor(0x00, 0xd0);
	n4_write_cmos_sensor(0x01, 0x2c);
	n4_write_cmos_sensor(0x05, 0x20);
	n4_write_cmos_sensor(0x1d, 0x0c);
	n4_write_cmos_sensor(0x21, 0x24);
	n4_write_cmos_sensor(0x24, 0x2a);
	n4_write_cmos_sensor(0x25, 0xcc);
	n4_write_cmos_sensor(0x26, 0x40);
	n4_write_cmos_sensor(0x27, 0x57);
	n4_write_cmos_sensor(0x28, 0x80);
	n4_write_cmos_sensor(0x2b, 0xa8);
	n4_write_cmos_sensor(0x31, 0x02);
	n4_write_cmos_sensor(0x32, 0x10);
	n4_write_cmos_sensor(0x38, 0x1f);
	n4_write_cmos_sensor(0x47, 0x04);
	n4_write_cmos_sensor(0x50, 0x80);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x57, 0x00);
	n4_write_cmos_sensor(0x58, 0x70);
	n4_write_cmos_sensor(0x59, 0x00);
	n4_write_cmos_sensor(0x5C, 0x78);
	n4_write_cmos_sensor(0x5F, 0x00);
	n4_write_cmos_sensor(0x62, 0x20);
	n4_write_cmos_sensor(0x64, 0x01);
	n4_write_cmos_sensor(0x65, 0x00);
	n4_write_cmos_sensor(0x69, 0x00);
	n4_write_cmos_sensor(0x6E, 0x00);
	n4_write_cmos_sensor(0x6F, 0x00);
	n4_write_cmos_sensor(0x90, 0x0d);
	n4_write_cmos_sensor(0x92, 0x00);
	n4_write_cmos_sensor(0x94, 0x00);
	n4_write_cmos_sensor(0x95, 0x00);
	n4_write_cmos_sensor(0xa9, 0x0a);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb7, 0xfc);
	n4_write_cmos_sensor(0xb8, 0xb9);
	n4_write_cmos_sensor(0xb9, 0x72);
	n4_write_cmos_sensor(0xbb, 0x0f);
	n4_write_cmos_sensor(0xd1, 0x30);
	n4_write_cmos_sensor(0xd5, 0x80);

	n4_write_cmos_sensor(0xFF, 0x09);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x50, 0x30);
	n4_write_cmos_sensor(0x51, 0x6f);
	n4_write_cmos_sensor(0x52, 0x67);
	n4_write_cmos_sensor(0x53, 0x48);
	n4_write_cmos_sensor(0x54, 0x30);
	n4_write_cmos_sensor(0x55, 0x6f);
	n4_write_cmos_sensor(0x56, 0x67);
	n4_write_cmos_sensor(0x57, 0x48);
	n4_write_cmos_sensor(0x58, 0x30);
	n4_write_cmos_sensor(0x59, 0x6f);
	n4_write_cmos_sensor(0x5a, 0x67);
	n4_write_cmos_sensor(0x5b, 0x48);
	n4_write_cmos_sensor(0x5c, 0x30);
	n4_write_cmos_sensor(0x5d, 0x6f);
	n4_write_cmos_sensor(0x5e, 0x67);
	n4_write_cmos_sensor(0x5f, 0x48);
	n4_write_cmos_sensor(0x96, 0x10);
	n4_write_cmos_sensor(0x97, 0x10);
	n4_write_cmos_sensor(0x98, 0x00);
	n4_write_cmos_sensor(0x99, 0x00);
	n4_write_cmos_sensor(0x9a, 0x00);
	n4_write_cmos_sensor(0x9b, 0x00);
	n4_write_cmos_sensor(0x9c, 0x00);
	n4_write_cmos_sensor(0x9d, 0x00);
	n4_write_cmos_sensor(0x9e, 0x00);
	n4_write_cmos_sensor(0xb6, 0x10);
	n4_write_cmos_sensor(0xb7, 0x10);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x00);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbe, 0x00);
	n4_write_cmos_sensor(0xd6, 0x10);
	n4_write_cmos_sensor(0xd7, 0x10);
	n4_write_cmos_sensor(0xd8, 0x00);
	n4_write_cmos_sensor(0xd9, 0x00);
	n4_write_cmos_sensor(0xda, 0x00);
	n4_write_cmos_sensor(0xdb, 0x00);
	n4_write_cmos_sensor(0xdc, 0x00);
	n4_write_cmos_sensor(0xdd, 0x00);
	n4_write_cmos_sensor(0xde, 0x00);
	n4_write_cmos_sensor(0xf6, 0x10);
	n4_write_cmos_sensor(0xf7, 0x10);
	n4_write_cmos_sensor(0xf8, 0x00);
	n4_write_cmos_sensor(0xf9, 0x00);
	n4_write_cmos_sensor(0xfa, 0x00);
	n4_write_cmos_sensor(0xfb, 0x00);
	n4_write_cmos_sensor(0xfc, 0x00);
	n4_write_cmos_sensor(0xfd, 0x00);
	n4_write_cmos_sensor(0xfe, 0x00);

	n4_write_cmos_sensor(0xff, 0x0a);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xff, 0x0b);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x3E, 0x00);
	n4_write_cmos_sensor(0x3F, 0x00);
	n4_write_cmos_sensor(0xFF, 0x20);
	n4_write_cmos_sensor(0x01, 0x55);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x0F, 0x00);
	n4_write_cmos_sensor(0x0D, 0x01);
	n4_write_cmos_sensor(0x00, 0xFF);
	n4_write_cmos_sensor(0x40, 0x01);
	n4_write_cmos_sensor(0x40, 0x00);//mipi reset

	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0xC8, 0x00);
	n4_write_cmos_sensor(0xC9, 0x00);
	n4_write_cmos_sensor(0xCA, 0x00);
	n4_write_cmos_sensor(0xCB, 0x00);
}

static void n4_sensor_hd25_init(void)
{
	LOG_INF("sensor_init in");
	n4_write_cmos_sensor(0xff, 0x04);
	n4_write_cmos_sensor(0xa0, 0x24);
	n4_write_cmos_sensor(0xa1, 0x24);
	n4_write_cmos_sensor(0xa2, 0x24);
	n4_write_cmos_sensor(0xa3, 0x24);
	n4_write_cmos_sensor(0xa4, 0x24);
	n4_write_cmos_sensor(0xa5, 0x24);
	n4_write_cmos_sensor(0xa6, 0x24);
	n4_write_cmos_sensor(0xa7, 0x24);
	n4_write_cmos_sensor(0xa8, 0x24);
	n4_write_cmos_sensor(0xa9, 0x24);
	n4_write_cmos_sensor(0xaa, 0x24);
	n4_write_cmos_sensor(0xab, 0x24);
	n4_write_cmos_sensor(0xac, 0x24);
	n4_write_cmos_sensor(0xad, 0x24);
	n4_write_cmos_sensor(0xae, 0x24);
	n4_write_cmos_sensor(0xaf, 0x24);
	n4_write_cmos_sensor(0xb0, 0x24);
	n4_write_cmos_sensor(0xb1, 0x24);
	n4_write_cmos_sensor(0xb2, 0x24);
	n4_write_cmos_sensor(0xb3, 0x24);
	n4_write_cmos_sensor(0xb4, 0x24);
	n4_write_cmos_sensor(0xb5, 0x24);
	n4_write_cmos_sensor(0xb6, 0x24);
	n4_write_cmos_sensor(0xb7, 0x24);
	n4_write_cmos_sensor(0xb8, 0x24);
	n4_write_cmos_sensor(0xb9, 0x24);
	n4_write_cmos_sensor(0xba, 0x24);
	n4_write_cmos_sensor(0xbb, 0x24);
	n4_write_cmos_sensor(0xbc, 0x24);
	n4_write_cmos_sensor(0xbd, 0x24);
	n4_write_cmos_sensor(0xbe, 0x24);
	n4_write_cmos_sensor(0xbf, 0x24);
	n4_write_cmos_sensor(0xc0, 0x24);
	n4_write_cmos_sensor(0xc1, 0x24);
	n4_write_cmos_sensor(0xc2, 0x24);
	n4_write_cmos_sensor(0xc3, 0x24);
	n4_write_cmos_sensor(0xff, 0x21);
	n4_write_cmos_sensor(0x07, 0x80);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0xff, 0x0A);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);
	n4_write_cmos_sensor(0xff, 0x0B);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x40, 0xAC);
	n4_write_cmos_sensor(0x41, 0x10);
	n4_write_cmos_sensor(0x42, 0x03);
	n4_write_cmos_sensor(0x43, 0x43);
	n4_write_cmos_sensor(0x11, 0x04);
	n4_write_cmos_sensor(0x10, 0x0A);
	n4_write_cmos_sensor(0x12, 0x06);
	n4_write_cmos_sensor(0x13, 0x09);
	n4_write_cmos_sensor(0x17, 0x01);
	n4_write_cmos_sensor(0x18, 0x0D);
	n4_write_cmos_sensor(0x15, 0x04);
	n4_write_cmos_sensor(0x14, 0x16);
	n4_write_cmos_sensor(0x16, 0x05);
	n4_write_cmos_sensor(0x19, 0x05);
	n4_write_cmos_sensor(0x1A, 0x0A);
	n4_write_cmos_sensor(0x1B, 0x08);
	n4_write_cmos_sensor(0x1C, 0x07);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x49, 0xF3);
	n4_write_cmos_sensor(0x49, 0xF0);
	n4_write_cmos_sensor(0x44, 0x02);
	n4_write_cmos_sensor(0x08, 0x40);
	n4_write_cmos_sensor(0x0F, 0x01);
	n4_write_cmos_sensor(0x38, 0x1E);
	n4_write_cmos_sensor(0x39, 0x1E);
	n4_write_cmos_sensor(0x3A, 0x1E);
	n4_write_cmos_sensor(0x3B, 0x1E);
	n4_write_cmos_sensor(0x07, 0x0F);
	n4_write_cmos_sensor(0x2D, 0x01);
	n4_write_cmos_sensor(0x45, 0x02);
	n4_write_cmos_sensor(0xFF, 0x13);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);

	n4_write_cmos_sensor(0xFF, 0x00);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x01, 0x00);
	n4_write_cmos_sensor(0x02, 0x00);
	n4_write_cmos_sensor(0x03, 0x00);
	n4_write_cmos_sensor(0x04, 0x00);
	n4_write_cmos_sensor(0x05, 0x00);
	n4_write_cmos_sensor(0x06, 0x00);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0x08, 0x0D);
	n4_write_cmos_sensor(0x09, 0x0D);
	n4_write_cmos_sensor(0x0a, 0x0D);
	n4_write_cmos_sensor(0x0b, 0x0D);
	n4_write_cmos_sensor(0x0c, 0x00);
	n4_write_cmos_sensor(0x0d, 0x00);
	n4_write_cmos_sensor(0x0e, 0x00);
	n4_write_cmos_sensor(0x0f, 0x00);
	n4_write_cmos_sensor(0x10, 0x20);
	n4_write_cmos_sensor(0x11, 0x20);
	n4_write_cmos_sensor(0x12, 0x20);
	n4_write_cmos_sensor(0x13, 0x20);
	n4_write_cmos_sensor(0x14, 0x00);
	n4_write_cmos_sensor(0x15, 0x00);
	n4_write_cmos_sensor(0x16, 0x00);
	n4_write_cmos_sensor(0x17, 0x00);
	n4_write_cmos_sensor(0x18, 0x11);//0x13
	n4_write_cmos_sensor(0x19, 0x11);//0x13
	n4_write_cmos_sensor(0x1a, 0x11);//0x13
	n4_write_cmos_sensor(0x1b, 0x11);//0x13
	n4_write_cmos_sensor(0x1c, 0x18);//enable black pattern
	n4_write_cmos_sensor(0x1d, 0x18);//enable black pattern
	n4_write_cmos_sensor(0x1e, 0x18);//enable black pattern
	n4_write_cmos_sensor(0x1f, 0x18);//enable black pattern
	n4_write_cmos_sensor(0x20, 0x00);
	n4_write_cmos_sensor(0x21, 0x00);
	n4_write_cmos_sensor(0x22, 0x00);
	n4_write_cmos_sensor(0x23, 0x00);
	n4_write_cmos_sensor(0x24, 0x88);
	n4_write_cmos_sensor(0x25, 0x88);
	n4_write_cmos_sensor(0x26, 0x88);
	n4_write_cmos_sensor(0x27, 0x88);
	n4_write_cmos_sensor(0x28, 0x84);
	n4_write_cmos_sensor(0x29, 0x84);
	n4_write_cmos_sensor(0x2a, 0x84);
	n4_write_cmos_sensor(0x2b, 0x84);
	n4_write_cmos_sensor(0x30, 0x03);
	n4_write_cmos_sensor(0x31, 0x03);
	n4_write_cmos_sensor(0x32, 0x03);
	n4_write_cmos_sensor(0x33, 0x03);
	n4_write_cmos_sensor(0x34, 0x0f);
	n4_write_cmos_sensor(0x35, 0x0f);
	n4_write_cmos_sensor(0x36, 0x0f);
	n4_write_cmos_sensor(0x37, 0x0f);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x48, 0x00);
	n4_write_cmos_sensor(0x49, 0x00);
	n4_write_cmos_sensor(0x4a, 0x00);
	n4_write_cmos_sensor(0x4b, 0x00);
	n4_write_cmos_sensor(0x4c, 0x00);
	n4_write_cmos_sensor(0x4d, 0x00);
	n4_write_cmos_sensor(0x4e, 0x00);
	n4_write_cmos_sensor(0x4f, 0x00);
	n4_write_cmos_sensor(0x50, 0x00);
	n4_write_cmos_sensor(0x51, 0x00);
	n4_write_cmos_sensor(0x52, 0x00);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x58, 0x80);
	n4_write_cmos_sensor(0x59, 0x80);
	n4_write_cmos_sensor(0x5a, 0x80);
	n4_write_cmos_sensor(0x5b, 0x80);
	n4_write_cmos_sensor(0x5c, 0x82);
	n4_write_cmos_sensor(0x5d, 0x82);
	n4_write_cmos_sensor(0x5e, 0x82);
	n4_write_cmos_sensor(0x5f, 0x82);
	n4_write_cmos_sensor(0x60, 0x10);
	n4_write_cmos_sensor(0x61, 0x10);
	n4_write_cmos_sensor(0x62, 0x10);
	n4_write_cmos_sensor(0x63, 0x10);
	n4_write_cmos_sensor(0x64, 0x05);
	n4_write_cmos_sensor(0x65, 0x05);
	n4_write_cmos_sensor(0x66, 0x05);
	n4_write_cmos_sensor(0x67, 0x05);
	n4_write_cmos_sensor(0x68, 0x38);
	n4_write_cmos_sensor(0x69, 0x38);
	n4_write_cmos_sensor(0x6a, 0x38);
	n4_write_cmos_sensor(0x6b, 0x38);
	n4_write_cmos_sensor(0x6c, 0x00);
	n4_write_cmos_sensor(0x6d, 0x00);
	n4_write_cmos_sensor(0x6e, 0x00);
	n4_write_cmos_sensor(0x6f, 0x00);
	n4_write_cmos_sensor(0x78, 0x21);
	n4_write_cmos_sensor(0x79, 0x21);
	n4_write_cmos_sensor(0x7a, 0x21);
	n4_write_cmos_sensor(0x7b, 0x21);


	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x7C, 0x00);
	n4_write_cmos_sensor(0x84, 0x04);
	n4_write_cmos_sensor(0x85, 0x04);
	n4_write_cmos_sensor(0x86, 0x04);
	n4_write_cmos_sensor(0x87, 0x04);
	n4_write_cmos_sensor(0x88, 0x01);
	n4_write_cmos_sensor(0x89, 0x01);
	n4_write_cmos_sensor(0x8a, 0x01);
	n4_write_cmos_sensor(0x8b, 0x01);
	n4_write_cmos_sensor(0x8c, 0x02);
	n4_write_cmos_sensor(0x8d, 0x02);
	n4_write_cmos_sensor(0x8e, 0x02);
	n4_write_cmos_sensor(0x8f, 0x02);
	n4_write_cmos_sensor(0xEC, 0x00);
	n4_write_cmos_sensor(0xED, 0x00);
	n4_write_cmos_sensor(0xEE, 0x00);
	n4_write_cmos_sensor(0xEF, 0x00);

	n4_write_cmos_sensor(0xFF, 0x05);
	n4_write_cmos_sensor(0x00, 0xd0);
	n4_write_cmos_sensor(0x01, 0x2c);
	n4_write_cmos_sensor(0x05, 0x24);
	n4_write_cmos_sensor(0x1d, 0x0c);
	n4_write_cmos_sensor(0x24, 0x2a);
	n4_write_cmos_sensor(0x25, 0xdc);
	n4_write_cmos_sensor(0x26, 0x40);
	n4_write_cmos_sensor(0x27, 0x57);
	n4_write_cmos_sensor(0x28, 0x80);
	n4_write_cmos_sensor(0x2b, 0xa8);
	n4_write_cmos_sensor(0x31, 0x82);
	n4_write_cmos_sensor(0x32, 0x10);
	n4_write_cmos_sensor(0x38, 0x10);
	n4_write_cmos_sensor(0x47, 0xEE);
	n4_write_cmos_sensor(0x50, 0xc6);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x57, 0x00);
	n4_write_cmos_sensor(0x58, 0x77);
	n4_write_cmos_sensor(0x59, 0x00);
	n4_write_cmos_sensor(0x5C, 0x78);
	n4_write_cmos_sensor(0x5F, 0x00);
	n4_write_cmos_sensor(0x62, 0x20);
	n4_write_cmos_sensor(0x64, 0x00);
	n4_write_cmos_sensor(0x65, 0x00);
	n4_write_cmos_sensor(0x69, 0x00);
	n4_write_cmos_sensor(0x6E, 0x10);
	n4_write_cmos_sensor(0x6F, 0x1C);
	n4_write_cmos_sensor(0x90, 0x01);
	n4_write_cmos_sensor(0x92, 0x00);
	n4_write_cmos_sensor(0x94, 0x00);
	n4_write_cmos_sensor(0x95, 0x00);
	n4_write_cmos_sensor(0xa9, 0x00);
	n4_write_cmos_sensor(0xb5, 0x80);
	n4_write_cmos_sensor(0xb7, 0xfc);
	n4_write_cmos_sensor(0xb8, 0x39);
	n4_write_cmos_sensor(0xb9, 0x72);
	n4_write_cmos_sensor(0xbb, 0x0f);
	n4_write_cmos_sensor(0xd1, 0x30);
	n4_write_cmos_sensor(0xd5, 0x80);

	n4_write_cmos_sensor(0xFF, 0x09);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x50, 0x30);
	n4_write_cmos_sensor(0x51, 0x6f);
	n4_write_cmos_sensor(0x52, 0x67);
	n4_write_cmos_sensor(0x53, 0x48);
	n4_write_cmos_sensor(0x54, 0x30);
	n4_write_cmos_sensor(0x55, 0x6f);
	n4_write_cmos_sensor(0x56, 0x67);
	n4_write_cmos_sensor(0x57, 0x48);
	n4_write_cmos_sensor(0x58, 0x30);
	n4_write_cmos_sensor(0x59, 0x6f);
	n4_write_cmos_sensor(0x5a, 0x67);
	n4_write_cmos_sensor(0x5b, 0x48);
	n4_write_cmos_sensor(0x5c, 0x30);
	n4_write_cmos_sensor(0x5d, 0x6f);
	n4_write_cmos_sensor(0x5e, 0x67);
	n4_write_cmos_sensor(0x5f, 0x48);
	n4_write_cmos_sensor(0x96, 0x00);
	n4_write_cmos_sensor(0x97, 0x00);
	n4_write_cmos_sensor(0x98, 0x00);
	n4_write_cmos_sensor(0x99, 0x00);
	n4_write_cmos_sensor(0x9a, 0x00);
	n4_write_cmos_sensor(0x9b, 0x00);
	n4_write_cmos_sensor(0x9c, 0x00);
	n4_write_cmos_sensor(0x9d, 0x00);
	n4_write_cmos_sensor(0x9e, 0x00);
	n4_write_cmos_sensor(0xb6, 0x00);
	n4_write_cmos_sensor(0xb7, 0x00);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x00);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbe, 0x00);
	n4_write_cmos_sensor(0xd6, 0x00);
	n4_write_cmos_sensor(0xd7, 0x00);
	n4_write_cmos_sensor(0xd8, 0x00);
	n4_write_cmos_sensor(0xd9, 0x00);
	n4_write_cmos_sensor(0xda, 0x00);
	n4_write_cmos_sensor(0xdb, 0x00);
	n4_write_cmos_sensor(0xdc, 0x00);
	n4_write_cmos_sensor(0xdd, 0x00);
	n4_write_cmos_sensor(0xde, 0x00);
	n4_write_cmos_sensor(0xf6, 0x00);
	n4_write_cmos_sensor(0xf7, 0x00);
	n4_write_cmos_sensor(0xf8, 0x00);
	n4_write_cmos_sensor(0xf9, 0x00);
	n4_write_cmos_sensor(0xfa, 0x00);
	n4_write_cmos_sensor(0xfb, 0x00);
	n4_write_cmos_sensor(0xfc, 0x00);
	n4_write_cmos_sensor(0xfd, 0x00);
	n4_write_cmos_sensor(0xfe, 0x00);

	n4_write_cmos_sensor(0xff, 0x0a);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xff, 0x0b);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x97, 0x00);
	n4_write_cmos_sensor(0x97, 0x0F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x3E, 0x00);
	n4_write_cmos_sensor(0x3F, 0x3F);

	n4_write_cmos_sensor(0xFF, 0x20);
	n4_write_cmos_sensor(0x01, 0x55);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x0F, 0x00);
	n4_write_cmos_sensor(0x0D, 0x01);  //4lane mode
	n4_write_cmos_sensor(0x00, 0xFF);  //ch1/2/3/4 enabled
	n4_write_cmos_sensor(0x40, 0x01);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0xFF, 0x01);

	n4_write_cmos_sensor(0xC8, 0x00);
	n4_write_cmos_sensor(0xC9, 0x00);
	n4_write_cmos_sensor(0xCA, 0x00);
	n4_write_cmos_sensor(0xCB, 0x00);
}

static void n4_sensor_hd30_init(void)
{
	LOG_INF("sensor_init in");
	n4_write_cmos_sensor(0xff, 0x04);
	n4_write_cmos_sensor(0xa0, 0x24);
	n4_write_cmos_sensor(0xa1, 0x24);
	n4_write_cmos_sensor(0xa2, 0x24);
	n4_write_cmos_sensor(0xa3, 0x24);
	n4_write_cmos_sensor(0xa4, 0x24);
	n4_write_cmos_sensor(0xa5, 0x24);
	n4_write_cmos_sensor(0xa6, 0x24);
	n4_write_cmos_sensor(0xa7, 0x24);
	n4_write_cmos_sensor(0xa8, 0x24);
	n4_write_cmos_sensor(0xa9, 0x24);
	n4_write_cmos_sensor(0xaa, 0x24);
	n4_write_cmos_sensor(0xab, 0x24);
	n4_write_cmos_sensor(0xac, 0x24);
	n4_write_cmos_sensor(0xad, 0x24);
	n4_write_cmos_sensor(0xae, 0x24);
	n4_write_cmos_sensor(0xaf, 0x24);
	n4_write_cmos_sensor(0xb0, 0x24);
	n4_write_cmos_sensor(0xb1, 0x24);
	n4_write_cmos_sensor(0xb2, 0x24);
	n4_write_cmos_sensor(0xb3, 0x24);
	n4_write_cmos_sensor(0xb4, 0x24);
	n4_write_cmos_sensor(0xb5, 0x24);
	n4_write_cmos_sensor(0xb6, 0x24);
	n4_write_cmos_sensor(0xb7, 0x24);
	n4_write_cmos_sensor(0xb8, 0x24);
	n4_write_cmos_sensor(0xb9, 0x24);
	n4_write_cmos_sensor(0xba, 0x24);
	n4_write_cmos_sensor(0xbb, 0x24);
	n4_write_cmos_sensor(0xbc, 0x24);
	n4_write_cmos_sensor(0xbd, 0x24);
	n4_write_cmos_sensor(0xbe, 0x24);
	n4_write_cmos_sensor(0xbf, 0x24);
	n4_write_cmos_sensor(0xc0, 0x24);
	n4_write_cmos_sensor(0xc1, 0x24);
	n4_write_cmos_sensor(0xc2, 0x24);
	n4_write_cmos_sensor(0xc3, 0x24);
	n4_write_cmos_sensor(0xff, 0x21);
	n4_write_cmos_sensor(0x07, 0x80);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0xff, 0x0A);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);
	n4_write_cmos_sensor(0xff, 0x0B);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x40, 0xAC);
	n4_write_cmos_sensor(0x41, 0x10);
	n4_write_cmos_sensor(0x42, 0x03);
	n4_write_cmos_sensor(0x43, 0x43);
	n4_write_cmos_sensor(0x11, 0x04);
	n4_write_cmos_sensor(0x10, 0x0A);
	n4_write_cmos_sensor(0x12, 0x06);
	n4_write_cmos_sensor(0x13, 0x09);
	n4_write_cmos_sensor(0x17, 0x01);
	n4_write_cmos_sensor(0x18, 0x0D);
	n4_write_cmos_sensor(0x15, 0x04);
	n4_write_cmos_sensor(0x14, 0x16);
	n4_write_cmos_sensor(0x16, 0x05);
	n4_write_cmos_sensor(0x19, 0x05);
	n4_write_cmos_sensor(0x1A, 0x0A);
	n4_write_cmos_sensor(0x1B, 0x08);
	n4_write_cmos_sensor(0x1C, 0x07);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x49, 0xF3);
	n4_write_cmos_sensor(0x49, 0xF0);
	n4_write_cmos_sensor(0x44, 0x02);
	n4_write_cmos_sensor(0x08, 0x40);
	n4_write_cmos_sensor(0x0F, 0x01);
	n4_write_cmos_sensor(0x38, 0x1E);
	n4_write_cmos_sensor(0x39, 0x1E);
	n4_write_cmos_sensor(0x3A, 0x1E);
	n4_write_cmos_sensor(0x3B, 0x1E);
	n4_write_cmos_sensor(0x07, 0x0F);
	n4_write_cmos_sensor(0x2D, 0x01);
	n4_write_cmos_sensor(0x45, 0x02);
	n4_write_cmos_sensor(0xFF, 0x13);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);

	n4_write_cmos_sensor(0xFF, 0x00);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x01, 0x00);
	n4_write_cmos_sensor(0x02, 0x00);
	n4_write_cmos_sensor(0x03, 0x00);
	n4_write_cmos_sensor(0x04, 0x00);
	n4_write_cmos_sensor(0x05, 0x00);
	n4_write_cmos_sensor(0x06, 0x00);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0x08, 0x0C);
	n4_write_cmos_sensor(0x09, 0x0C);
	n4_write_cmos_sensor(0x0a, 0x0C);
	n4_write_cmos_sensor(0x0b, 0x0C);
	n4_write_cmos_sensor(0x0c, 0x00);
	n4_write_cmos_sensor(0x0d, 0x00);
	n4_write_cmos_sensor(0x0e, 0x00);
	n4_write_cmos_sensor(0x0f, 0x00);
	n4_write_cmos_sensor(0x10, 0x20);
	n4_write_cmos_sensor(0x11, 0x20);
	n4_write_cmos_sensor(0x12, 0x20);
	n4_write_cmos_sensor(0x13, 0x20);
	n4_write_cmos_sensor(0x14, 0x00);
	n4_write_cmos_sensor(0x15, 0x00);
	n4_write_cmos_sensor(0x16, 0x00);
	n4_write_cmos_sensor(0x17, 0x00);
	n4_write_cmos_sensor(0x18, 0x11);//0x13 for signal detect easy
	n4_write_cmos_sensor(0x19, 0x11);//0x13
	n4_write_cmos_sensor(0x1a, 0x11);//0x13
	n4_write_cmos_sensor(0x1b, 0x11);//0x13
	n4_write_cmos_sensor(0x1c, 0x18);//for black data
	n4_write_cmos_sensor(0x1d, 0x18);//for black data
	n4_write_cmos_sensor(0x1e, 0x18);//for black data
	n4_write_cmos_sensor(0x1f, 0x18);//for black data
	n4_write_cmos_sensor(0x20, 0x00);
	n4_write_cmos_sensor(0x21, 0x00);
	n4_write_cmos_sensor(0x22, 0x00);
	n4_write_cmos_sensor(0x23, 0x00);
	n4_write_cmos_sensor(0x24, 0x88);
	n4_write_cmos_sensor(0x25, 0x88);
	n4_write_cmos_sensor(0x26, 0x88);
	n4_write_cmos_sensor(0x27, 0x88);
	n4_write_cmos_sensor(0x28, 0x84);
	n4_write_cmos_sensor(0x29, 0x84);
	n4_write_cmos_sensor(0x2a, 0x84);
	n4_write_cmos_sensor(0x2b, 0x84);
	n4_write_cmos_sensor(0x30, 0x03);
	n4_write_cmos_sensor(0x31, 0x03);
	n4_write_cmos_sensor(0x32, 0x03);
	n4_write_cmos_sensor(0x33, 0x03);
	n4_write_cmos_sensor(0x34, 0x0f);
	n4_write_cmos_sensor(0x35, 0x0f);
	n4_write_cmos_sensor(0x36, 0x0f);
	n4_write_cmos_sensor(0x37, 0x0f);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x48, 0x00);
	n4_write_cmos_sensor(0x49, 0x00);
	n4_write_cmos_sensor(0x4a, 0x00);
	n4_write_cmos_sensor(0x4b, 0x00);
	n4_write_cmos_sensor(0x4c, 0x00);
	n4_write_cmos_sensor(0x4d, 0x00);
	n4_write_cmos_sensor(0x4e, 0x00);
	n4_write_cmos_sensor(0x4f, 0x00);
	n4_write_cmos_sensor(0x50, 0x00);
	n4_write_cmos_sensor(0x51, 0x00);
	n4_write_cmos_sensor(0x52, 0x00);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x58, 0x80);
	n4_write_cmos_sensor(0x59, 0x80);
	n4_write_cmos_sensor(0x5a, 0x80);
	n4_write_cmos_sensor(0x5b, 0x80);
	n4_write_cmos_sensor(0x5c, 0x82);
	n4_write_cmos_sensor(0x5d, 0x82);
	n4_write_cmos_sensor(0x5e, 0x82);
	n4_write_cmos_sensor(0x5f, 0x82);
	n4_write_cmos_sensor(0x60, 0x10);
	n4_write_cmos_sensor(0x61, 0x10);
	n4_write_cmos_sensor(0x62, 0x10);
	n4_write_cmos_sensor(0x63, 0x10);
	n4_write_cmos_sensor(0x64, 0x05);
	n4_write_cmos_sensor(0x65, 0x05);
	n4_write_cmos_sensor(0x66, 0x05);
	n4_write_cmos_sensor(0x67, 0x05);
	n4_write_cmos_sensor(0x68, 0x3E);
	n4_write_cmos_sensor(0x69, 0x3E);
	n4_write_cmos_sensor(0x6a, 0x3E);
	n4_write_cmos_sensor(0x6b, 0x3E);
	n4_write_cmos_sensor(0x6c, 0x00);
	n4_write_cmos_sensor(0x6d, 0x00);
	n4_write_cmos_sensor(0x6e, 0x00);
	n4_write_cmos_sensor(0x6f, 0x00);
	n4_write_cmos_sensor(0x78, 0x21);
	n4_write_cmos_sensor(0x79, 0x21);
	n4_write_cmos_sensor(0x7a, 0x21);
	n4_write_cmos_sensor(0x7b, 0x21);


	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x7C, 0x00);
	n4_write_cmos_sensor(0x84, 0x04);
	n4_write_cmos_sensor(0x85, 0x04);
	n4_write_cmos_sensor(0x86, 0x04);
	n4_write_cmos_sensor(0x87, 0x04);
	n4_write_cmos_sensor(0x88, 0x01);
	n4_write_cmos_sensor(0x89, 0x01);
	n4_write_cmos_sensor(0x8a, 0x01);
	n4_write_cmos_sensor(0x8b, 0x01);
	n4_write_cmos_sensor(0x8c, 0x02);
	n4_write_cmos_sensor(0x8d, 0x02);
	n4_write_cmos_sensor(0x8e, 0x02);
	n4_write_cmos_sensor(0x8f, 0x02);
	n4_write_cmos_sensor(0xEC, 0x00);
	n4_write_cmos_sensor(0xED, 0x00);
	n4_write_cmos_sensor(0xEE, 0x00);
	n4_write_cmos_sensor(0xEF, 0x00);

	n4_write_cmos_sensor(0xFF, 0x05);
	n4_write_cmos_sensor(0x00, 0xd0);
	n4_write_cmos_sensor(0x01, 0x2c);
	n4_write_cmos_sensor(0x05, 0x24);
	n4_write_cmos_sensor(0x1d, 0x0c);
	n4_write_cmos_sensor(0x24, 0x2a);
	n4_write_cmos_sensor(0x25, 0xdc);
	n4_write_cmos_sensor(0x26, 0x40);
	n4_write_cmos_sensor(0x27, 0x57);
	n4_write_cmos_sensor(0x28, 0x80);
	n4_write_cmos_sensor(0x2b, 0xa8);
	n4_write_cmos_sensor(0x31, 0x82);
	n4_write_cmos_sensor(0x32, 0x10);
	n4_write_cmos_sensor(0x38, 0x10);
	n4_write_cmos_sensor(0x47, 0xEE);
	n4_write_cmos_sensor(0x50, 0xc6);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x57, 0x00);
	n4_write_cmos_sensor(0x58, 0x77);
	n4_write_cmos_sensor(0x59, 0x00);
	n4_write_cmos_sensor(0x5C, 0x78);
	n4_write_cmos_sensor(0x5F, 0x00);
	n4_write_cmos_sensor(0x62, 0x20);
	n4_write_cmos_sensor(0x64, 0x00);
	n4_write_cmos_sensor(0x65, 0x00);
	n4_write_cmos_sensor(0x69, 0x00);
	n4_write_cmos_sensor(0x6E, 0x10);
	n4_write_cmos_sensor(0x6F, 0x1C);
	n4_write_cmos_sensor(0x90, 0x01);
	n4_write_cmos_sensor(0x92, 0x00);
	n4_write_cmos_sensor(0x94, 0x00);
	n4_write_cmos_sensor(0x95, 0x00);
	n4_write_cmos_sensor(0xa9, 0x00);
	n4_write_cmos_sensor(0xb5, 0x80);
	n4_write_cmos_sensor(0xb7, 0xfc);
	n4_write_cmos_sensor(0xb8, 0x39);
	n4_write_cmos_sensor(0xb9, 0x72);
	n4_write_cmos_sensor(0xbb, 0x0f);
	n4_write_cmos_sensor(0xd1, 0x30);
	n4_write_cmos_sensor(0xd5, 0x80);

	n4_write_cmos_sensor(0xFF, 0x09);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x50, 0x30);
	n4_write_cmos_sensor(0x51, 0x6f);
	n4_write_cmos_sensor(0x52, 0x67);
	n4_write_cmos_sensor(0x53, 0x48);
	n4_write_cmos_sensor(0x54, 0x30);
	n4_write_cmos_sensor(0x55, 0x6f);
	n4_write_cmos_sensor(0x56, 0x67);
	n4_write_cmos_sensor(0x57, 0x48);
	n4_write_cmos_sensor(0x58, 0x30);
	n4_write_cmos_sensor(0x59, 0x6f);
	n4_write_cmos_sensor(0x5a, 0x67);
	n4_write_cmos_sensor(0x5b, 0x48);
	n4_write_cmos_sensor(0x5c, 0x30);
	n4_write_cmos_sensor(0x5d, 0x6f);
	n4_write_cmos_sensor(0x5e, 0x67);
	n4_write_cmos_sensor(0x5f, 0x48);
	n4_write_cmos_sensor(0x96, 0x00);
	n4_write_cmos_sensor(0x97, 0x00);
	n4_write_cmos_sensor(0x98, 0x00);
	n4_write_cmos_sensor(0x99, 0x00);
	n4_write_cmos_sensor(0x9a, 0x00);
	n4_write_cmos_sensor(0x9b, 0x00);
	n4_write_cmos_sensor(0x9c, 0x00);
	n4_write_cmos_sensor(0x9d, 0x00);
	n4_write_cmos_sensor(0x9e, 0x00);
	n4_write_cmos_sensor(0xb6, 0x00);
	n4_write_cmos_sensor(0xb7, 0x00);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x00);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbe, 0x00);
	n4_write_cmos_sensor(0xd6, 0x00);
	n4_write_cmos_sensor(0xd7, 0x00);
	n4_write_cmos_sensor(0xd8, 0x00);
	n4_write_cmos_sensor(0xd9, 0x00);
	n4_write_cmos_sensor(0xda, 0x00);
	n4_write_cmos_sensor(0xdb, 0x00);
	n4_write_cmos_sensor(0xdc, 0x00);
	n4_write_cmos_sensor(0xdd, 0x00);
	n4_write_cmos_sensor(0xde, 0x00);
	n4_write_cmos_sensor(0xf6, 0x00);
	n4_write_cmos_sensor(0xf7, 0x00);
	n4_write_cmos_sensor(0xf8, 0x00);
	n4_write_cmos_sensor(0xf9, 0x00);
	n4_write_cmos_sensor(0xfa, 0x00);
	n4_write_cmos_sensor(0xfb, 0x00);
	n4_write_cmos_sensor(0xfc, 0x00);
	n4_write_cmos_sensor(0xfd, 0x00);
	n4_write_cmos_sensor(0xfe, 0x00);

	n4_write_cmos_sensor(0xff, 0x0a);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xff, 0x0b);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x97, 0x00);
	n4_write_cmos_sensor(0x97, 0x0F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x3E, 0x00);
	n4_write_cmos_sensor(0x3F, 0x3F);

	n4_write_cmos_sensor(0xFF, 0x20);
	n4_write_cmos_sensor(0x01, 0x55);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x0F, 0x00);
	n4_write_cmos_sensor(0x0D, 0x01);  //4lane mode
	n4_write_cmos_sensor(0x00, 0xFF);  //ch1/2/3/4 enabled
	n4_write_cmos_sensor(0x40, 0x01);
	n4_write_cmos_sensor(0x40, 0x00);

	n4_write_cmos_sensor(0xFF, 0x01);

	n4_write_cmos_sensor(0xC8, 0x00);
	n4_write_cmos_sensor(0xC9, 0x00);
	n4_write_cmos_sensor(0xCA, 0x00);
	n4_write_cmos_sensor(0xCB, 0x00);
}

static void n4_sensor_fhd25_init(void)
{
	LOG_INF("sensor_init in");
	n4_write_cmos_sensor(0xff, 0x04);
	n4_write_cmos_sensor(0xa0, 0x24);
	n4_write_cmos_sensor(0xa1, 0x24);
	n4_write_cmos_sensor(0xa2, 0x24);
	n4_write_cmos_sensor(0xa3, 0x24);
	n4_write_cmos_sensor(0xa4, 0x24);
	n4_write_cmos_sensor(0xa5, 0x24);
	n4_write_cmos_sensor(0xa6, 0x24);
	n4_write_cmos_sensor(0xa7, 0x24);
	n4_write_cmos_sensor(0xa8, 0x24);
	n4_write_cmos_sensor(0xa9, 0x24);
	n4_write_cmos_sensor(0xaa, 0x24);
	n4_write_cmos_sensor(0xab, 0x24);
	n4_write_cmos_sensor(0xac, 0x24);
	n4_write_cmos_sensor(0xad, 0x24);
	n4_write_cmos_sensor(0xae, 0x24);
	n4_write_cmos_sensor(0xaf, 0x24);
	n4_write_cmos_sensor(0xb0, 0x24);
	n4_write_cmos_sensor(0xb1, 0x24);
	n4_write_cmos_sensor(0xb2, 0x24);
	n4_write_cmos_sensor(0xb3, 0x24);
	n4_write_cmos_sensor(0xb4, 0x24);
	n4_write_cmos_sensor(0xb5, 0x24);
	n4_write_cmos_sensor(0xb6, 0x24);
	n4_write_cmos_sensor(0xb7, 0x24);
	n4_write_cmos_sensor(0xb8, 0x24);
	n4_write_cmos_sensor(0xb9, 0x24);
	n4_write_cmos_sensor(0xba, 0x24);
	n4_write_cmos_sensor(0xbb, 0x24);
	n4_write_cmos_sensor(0xbc, 0x24);
	n4_write_cmos_sensor(0xbd, 0x24);
	n4_write_cmos_sensor(0xbe, 0x24);
	n4_write_cmos_sensor(0xbf, 0x24);
	n4_write_cmos_sensor(0xc0, 0x24);
	n4_write_cmos_sensor(0xc1, 0x24);
	n4_write_cmos_sensor(0xc2, 0x24);
	n4_write_cmos_sensor(0xc3, 0x24);
	n4_write_cmos_sensor(0xff, 0x21);
	n4_write_cmos_sensor(0x07, 0x80);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0xff, 0x0A);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);
	n4_write_cmos_sensor(0xff, 0x0B);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x40, 0xB4);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x03);
	n4_write_cmos_sensor(0x43, 0x43);
	n4_write_cmos_sensor(0x11, 0x08);
	n4_write_cmos_sensor(0x10, 0x13);
	n4_write_cmos_sensor(0x12, 0x0B);
	n4_write_cmos_sensor(0x13, 0x12);
	n4_write_cmos_sensor(0x17, 0x02);
	n4_write_cmos_sensor(0x18, 0x12);
	n4_write_cmos_sensor(0x15, 0x07);
	n4_write_cmos_sensor(0x14, 0x2D);
	n4_write_cmos_sensor(0x16, 0x0B);
	n4_write_cmos_sensor(0x19, 0x09);
	n4_write_cmos_sensor(0x1A, 0x15);
	n4_write_cmos_sensor(0x1B, 0x11);
	n4_write_cmos_sensor(0x1C, 0x0E);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x49, 0xF3);
	n4_write_cmos_sensor(0x49, 0xF0);
	n4_write_cmos_sensor(0x44, 0x02);
	n4_write_cmos_sensor(0x08, 0x40);
	n4_write_cmos_sensor(0x0F, 0x01);
	n4_write_cmos_sensor(0x38, 0x1E);
	n4_write_cmos_sensor(0x39, 0x1E);
	n4_write_cmos_sensor(0x3A, 0x1E);
	n4_write_cmos_sensor(0x3B, 0x1E);
	n4_write_cmos_sensor(0x07, 0x0F);
	n4_write_cmos_sensor(0x2D, 0x01);
	n4_write_cmos_sensor(0x45, 0x02);
	n4_write_cmos_sensor(0xFF, 0x13);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);

	n4_write_cmos_sensor(0xFF, 0x00);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x01, 0x00);
	n4_write_cmos_sensor(0x02, 0x00);
	n4_write_cmos_sensor(0x03, 0x00);
	n4_write_cmos_sensor(0x04, 0x00);
	n4_write_cmos_sensor(0x05, 0x00);
	n4_write_cmos_sensor(0x06, 0x00);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0x08, 0x03);
	n4_write_cmos_sensor(0x09, 0x03);
	n4_write_cmos_sensor(0x0a, 0x03);
	n4_write_cmos_sensor(0x0b, 0x03);
	n4_write_cmos_sensor(0x0c, 0x00);
	n4_write_cmos_sensor(0x0d, 0x00);
	n4_write_cmos_sensor(0x0e, 0x00);
	n4_write_cmos_sensor(0x0f, 0x00);
	n4_write_cmos_sensor(0x10, 0x20);
	n4_write_cmos_sensor(0x11, 0x20);
	n4_write_cmos_sensor(0x12, 0x20);
	n4_write_cmos_sensor(0x13, 0x20);
	n4_write_cmos_sensor(0x14, 0x00);
	n4_write_cmos_sensor(0x15, 0x00);
	n4_write_cmos_sensor(0x16, 0x00);
	n4_write_cmos_sensor(0x17, 0x00);
	n4_write_cmos_sensor(0x18, 0x11);//for signal detect stable
	n4_write_cmos_sensor(0x19, 0x11);
	n4_write_cmos_sensor(0x1a, 0x11);
	n4_write_cmos_sensor(0x1b, 0x11);
	n4_write_cmos_sensor(0x1c, 0x18);//enable black pattern
	n4_write_cmos_sensor(0x1d, 0x18);
	n4_write_cmos_sensor(0x1e, 0x18);
	n4_write_cmos_sensor(0x1f, 0x18);
	n4_write_cmos_sensor(0x20, 0x00);
	n4_write_cmos_sensor(0x21, 0x00);
	n4_write_cmos_sensor(0x22, 0x00);
	n4_write_cmos_sensor(0x23, 0x00);
	n4_write_cmos_sensor(0x24, 0x86);
	n4_write_cmos_sensor(0x25, 0x86);
	n4_write_cmos_sensor(0x26, 0x86);
	n4_write_cmos_sensor(0x27, 0x86);
	n4_write_cmos_sensor(0x28, 0x80);
	n4_write_cmos_sensor(0x29, 0x80);
	n4_write_cmos_sensor(0x2a, 0x80);
	n4_write_cmos_sensor(0x2b, 0x80);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);
	n4_write_cmos_sensor(0x33, 0x00);
	n4_write_cmos_sensor(0x34, 0x00);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0x00);
	n4_write_cmos_sensor(0x37, 0x00);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x48, 0x00);
	n4_write_cmos_sensor(0x49, 0x00);
	n4_write_cmos_sensor(0x4a, 0x00);
	n4_write_cmos_sensor(0x4b, 0x00);
	n4_write_cmos_sensor(0x4c, 0xfe);
	n4_write_cmos_sensor(0x4d, 0xfe);
	n4_write_cmos_sensor(0x4e, 0xfe);
	n4_write_cmos_sensor(0x4f, 0xfe);
	n4_write_cmos_sensor(0x50, 0xfb);
	n4_write_cmos_sensor(0x51, 0xfb);
	n4_write_cmos_sensor(0x52, 0xfb);
	n4_write_cmos_sensor(0x53, 0xfb);
	n4_write_cmos_sensor(0x58, 0x80);
	n4_write_cmos_sensor(0x59, 0x80);
	n4_write_cmos_sensor(0x5a, 0x80);
	n4_write_cmos_sensor(0x5b, 0x80);
	n4_write_cmos_sensor(0x5c, 0x82);
	n4_write_cmos_sensor(0x5d, 0x82);
	n4_write_cmos_sensor(0x5e, 0x82);
	n4_write_cmos_sensor(0x5f, 0x82);
	n4_write_cmos_sensor(0x60, 0x10);
	n4_write_cmos_sensor(0x61, 0x10);
	n4_write_cmos_sensor(0x62, 0x10);
	n4_write_cmos_sensor(0x63, 0x10);
	n4_write_cmos_sensor(0x64, 0x05);
	n4_write_cmos_sensor(0x65, 0x05);
	n4_write_cmos_sensor(0x66, 0x05);
	n4_write_cmos_sensor(0x67, 0x05);
	n4_write_cmos_sensor(0x68, 0x48);
	n4_write_cmos_sensor(0x69, 0x48);
	n4_write_cmos_sensor(0x6a, 0x48);
	n4_write_cmos_sensor(0x6b, 0x48);
	n4_write_cmos_sensor(0x6c, 0x00);
	n4_write_cmos_sensor(0x6d, 0x00);
	n4_write_cmos_sensor(0x6e, 0x00);
	n4_write_cmos_sensor(0x6f, 0x00);
	n4_write_cmos_sensor(0x78, 0x21);
	n4_write_cmos_sensor(0x79, 0x21);
	n4_write_cmos_sensor(0x7a, 0x21);
	n4_write_cmos_sensor(0x7b, 0x21);


	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x7C, 0x00);
	n4_write_cmos_sensor(0x84, 0x04);
	n4_write_cmos_sensor(0x85, 0x04);
	n4_write_cmos_sensor(0x86, 0x04);
	n4_write_cmos_sensor(0x87, 0x04);
	n4_write_cmos_sensor(0x88, 0x01);
	n4_write_cmos_sensor(0x89, 0x01);
	n4_write_cmos_sensor(0x8a, 0x01);
	n4_write_cmos_sensor(0x8b, 0x01);
	n4_write_cmos_sensor(0x8c, 0x02);
	n4_write_cmos_sensor(0x8d, 0x02);
	n4_write_cmos_sensor(0x8e, 0x02);
	n4_write_cmos_sensor(0x8f, 0x02);
	n4_write_cmos_sensor(0xEC, 0x00);
	n4_write_cmos_sensor(0xED, 0x00);
	n4_write_cmos_sensor(0xEE, 0x00);
	n4_write_cmos_sensor(0xEF, 0x00);

	n4_write_cmos_sensor(0xFF, 0x05);
	n4_write_cmos_sensor(0x00, 0xd0);
	n4_write_cmos_sensor(0x01, 0x2c);
	n4_write_cmos_sensor(0x05, 0x24);
	n4_write_cmos_sensor(0x1d, 0x0c);
	n4_write_cmos_sensor(0x24, 0x2a);
	n4_write_cmos_sensor(0x25, 0xdc);
	n4_write_cmos_sensor(0x26, 0x40);
	n4_write_cmos_sensor(0x27, 0x57);
	n4_write_cmos_sensor(0x28, 0x80);
	n4_write_cmos_sensor(0x2b, 0xa8);
	n4_write_cmos_sensor(0x31, 0x82);
	n4_write_cmos_sensor(0x32, 0x10);
	n4_write_cmos_sensor(0x38, 0x13);
	n4_write_cmos_sensor(0x47, 0xEE);
	n4_write_cmos_sensor(0x50, 0xc6);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x57, 0x00);
	n4_write_cmos_sensor(0x58, 0x77);
	n4_write_cmos_sensor(0x59, 0x00);
	n4_write_cmos_sensor(0x5C, 0x78);
	n4_write_cmos_sensor(0x5F, 0x00);
	n4_write_cmos_sensor(0x62, 0x20);
	n4_write_cmos_sensor(0x64, 0x00);
	n4_write_cmos_sensor(0x65, 0x00);
	n4_write_cmos_sensor(0x69, 0x00);
	n4_write_cmos_sensor(0x6E, 0x00);
	n4_write_cmos_sensor(0x6F, 0x00);
	n4_write_cmos_sensor(0x90, 0x01);
	n4_write_cmos_sensor(0x92, 0x00);
	n4_write_cmos_sensor(0x94, 0x00);
	n4_write_cmos_sensor(0x95, 0x00);
	n4_write_cmos_sensor(0xa9, 0x00);
	n4_write_cmos_sensor(0xb5, 0x80);
	n4_write_cmos_sensor(0xb7, 0xfc);
	n4_write_cmos_sensor(0xb8, 0x39);
	n4_write_cmos_sensor(0xb9, 0x72);
	n4_write_cmos_sensor(0xbb, 0x0f);
	n4_write_cmos_sensor(0xd1, 0x30);
	n4_write_cmos_sensor(0xd5, 0x80);

	n4_write_cmos_sensor(0xFF, 0x09);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x50, 0x30);
	n4_write_cmos_sensor(0x51, 0x6f);
	n4_write_cmos_sensor(0x52, 0x67);
	n4_write_cmos_sensor(0x53, 0x48);
	n4_write_cmos_sensor(0x54, 0x30);
	n4_write_cmos_sensor(0x55, 0x6f);
	n4_write_cmos_sensor(0x56, 0x67);
	n4_write_cmos_sensor(0x57, 0x48);
	n4_write_cmos_sensor(0x58, 0x30);
	n4_write_cmos_sensor(0x59, 0x6f);
	n4_write_cmos_sensor(0x5a, 0x67);
	n4_write_cmos_sensor(0x5b, 0x48);
	n4_write_cmos_sensor(0x5c, 0x30);
	n4_write_cmos_sensor(0x5d, 0x6f);
	n4_write_cmos_sensor(0x5e, 0x67);
	n4_write_cmos_sensor(0x5f, 0x48);
	n4_write_cmos_sensor(0x96, 0x00);
	n4_write_cmos_sensor(0x97, 0x00);
	n4_write_cmos_sensor(0x98, 0x00);
	n4_write_cmos_sensor(0x99, 0x00);
	n4_write_cmos_sensor(0x9a, 0x00);
	n4_write_cmos_sensor(0x9b, 0x00);
	n4_write_cmos_sensor(0x9c, 0x00);
	n4_write_cmos_sensor(0x9d, 0x00);
	n4_write_cmos_sensor(0x9e, 0x00);
	n4_write_cmos_sensor(0xb6, 0x00);
	n4_write_cmos_sensor(0xb7, 0x00);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x00);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbe, 0x00);
	n4_write_cmos_sensor(0xd6, 0x00);
	n4_write_cmos_sensor(0xd7, 0x00);
	n4_write_cmos_sensor(0xd8, 0x00);
	n4_write_cmos_sensor(0xd9, 0x00);
	n4_write_cmos_sensor(0xda, 0x00);
	n4_write_cmos_sensor(0xdb, 0x00);
	n4_write_cmos_sensor(0xdc, 0x00);
	n4_write_cmos_sensor(0xdd, 0x00);
	n4_write_cmos_sensor(0xde, 0x00);
	n4_write_cmos_sensor(0xf6, 0x00);
	n4_write_cmos_sensor(0xf7, 0x00);
	n4_write_cmos_sensor(0xf8, 0x00);
	n4_write_cmos_sensor(0xf9, 0x00);
	n4_write_cmos_sensor(0xfa, 0x00);
	n4_write_cmos_sensor(0xfb, 0x00);
	n4_write_cmos_sensor(0xfc, 0x00);
	n4_write_cmos_sensor(0xfd, 0x00);
	n4_write_cmos_sensor(0xfe, 0x00);

	n4_write_cmos_sensor(0xff, 0x0a);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xff, 0x0b);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x97, 0x00);
	n4_write_cmos_sensor(0x97, 0x0F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x3E, 0x00);
	n4_write_cmos_sensor(0x3F, 0x3F);

	n4_write_cmos_sensor(0xFF, 0x20);
	n4_write_cmos_sensor(0x01, 0x00);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x0F, 0x00);
	n4_write_cmos_sensor(0x0D, 0x01);//4lane mode
	n4_write_cmos_sensor(0x00, 0xFF);//ch1/2/3/4 enabled
	n4_write_cmos_sensor(0x40, 0x01);
	n4_write_cmos_sensor(0x40, 0x00);

	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0xC8, 0x00);
	n4_write_cmos_sensor(0xC9, 0x00);
	n4_write_cmos_sensor(0xCA, 0x00);
	n4_write_cmos_sensor(0xCB, 0x00);
}

static void n4_sensor_fhd30_init(void)
{
	LOG_INF("sensor_init in");
	n4_write_cmos_sensor(0xff, 0x04);
	n4_write_cmos_sensor(0xa0, 0x24);
	n4_write_cmos_sensor(0xa1, 0x24);
	n4_write_cmos_sensor(0xa2, 0x24);
	n4_write_cmos_sensor(0xa3, 0x24);
	n4_write_cmos_sensor(0xa4, 0x24);
	n4_write_cmos_sensor(0xa5, 0x24);
	n4_write_cmos_sensor(0xa6, 0x24);
	n4_write_cmos_sensor(0xa7, 0x24);
	n4_write_cmos_sensor(0xa8, 0x24);
	n4_write_cmos_sensor(0xa9, 0x24);
	n4_write_cmos_sensor(0xaa, 0x24);
	n4_write_cmos_sensor(0xab, 0x24);
	n4_write_cmos_sensor(0xac, 0x24);
	n4_write_cmos_sensor(0xad, 0x24);
	n4_write_cmos_sensor(0xae, 0x24);
	n4_write_cmos_sensor(0xaf, 0x24);
	n4_write_cmos_sensor(0xb0, 0x24);
	n4_write_cmos_sensor(0xb1, 0x24);
	n4_write_cmos_sensor(0xb2, 0x24);
	n4_write_cmos_sensor(0xb3, 0x24);
	n4_write_cmos_sensor(0xb4, 0x24);
	n4_write_cmos_sensor(0xb5, 0x24);
	n4_write_cmos_sensor(0xb6, 0x24);
	n4_write_cmos_sensor(0xb7, 0x24);
	n4_write_cmos_sensor(0xb8, 0x24);
	n4_write_cmos_sensor(0xb9, 0x24);
	n4_write_cmos_sensor(0xba, 0x24);
	n4_write_cmos_sensor(0xbb, 0x24);
	n4_write_cmos_sensor(0xbc, 0x24);
	n4_write_cmos_sensor(0xbd, 0x24);
	n4_write_cmos_sensor(0xbe, 0x24);
	n4_write_cmos_sensor(0xbf, 0x24);
	n4_write_cmos_sensor(0xc0, 0x24);
	n4_write_cmos_sensor(0xc1, 0x24);
	n4_write_cmos_sensor(0xc2, 0x24);
	n4_write_cmos_sensor(0xc3, 0x24);
	n4_write_cmos_sensor(0xff, 0x21);
	n4_write_cmos_sensor(0x07, 0x80);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0xff, 0x0A);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);
	n4_write_cmos_sensor(0xff, 0x0B);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x40, 0xB4);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x03);
	n4_write_cmos_sensor(0x43, 0x43);
	n4_write_cmos_sensor(0x11, 0x08);
	n4_write_cmos_sensor(0x10, 0x13);
	n4_write_cmos_sensor(0x12, 0x0B);
	n4_write_cmos_sensor(0x13, 0x12);
	n4_write_cmos_sensor(0x17, 0x02);
	n4_write_cmos_sensor(0x18, 0x12);
	n4_write_cmos_sensor(0x15, 0x07);
	n4_write_cmos_sensor(0x14, 0x2D);
	n4_write_cmos_sensor(0x16, 0x0B);
	n4_write_cmos_sensor(0x19, 0x09);
	n4_write_cmos_sensor(0x1A, 0x15);
	n4_write_cmos_sensor(0x1B, 0x11);
	n4_write_cmos_sensor(0x1C, 0x0E);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x49, 0xF3);
	n4_write_cmos_sensor(0x49, 0xF0);
	n4_write_cmos_sensor(0x44, 0x02);
	n4_write_cmos_sensor(0x08, 0x40);
	n4_write_cmos_sensor(0x0F, 0x01);
	n4_write_cmos_sensor(0x38, 0x1E);
	n4_write_cmos_sensor(0x39, 0x1E);
	n4_write_cmos_sensor(0x3A, 0x1E);
	n4_write_cmos_sensor(0x3B, 0x1E);
	n4_write_cmos_sensor(0x07, 0x0F);
	n4_write_cmos_sensor(0x2D, 0x01);
	n4_write_cmos_sensor(0x45, 0x02);
	n4_write_cmos_sensor(0xFF, 0x13);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);

	n4_write_cmos_sensor(0xFF, 0x00);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x01, 0x00);
	n4_write_cmos_sensor(0x02, 0x00);
	n4_write_cmos_sensor(0x03, 0x00);
	n4_write_cmos_sensor(0x04, 0x00);
	n4_write_cmos_sensor(0x05, 0x00);
	n4_write_cmos_sensor(0x06, 0x00);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0x08, 0x02);
	n4_write_cmos_sensor(0x09, 0x02);
	n4_write_cmos_sensor(0x0a, 0x02);
	n4_write_cmos_sensor(0x0b, 0x02);
	n4_write_cmos_sensor(0x0c, 0x00);
	n4_write_cmos_sensor(0x0d, 0x00);
	n4_write_cmos_sensor(0x0e, 0x00);
	n4_write_cmos_sensor(0x0f, 0x00);
	n4_write_cmos_sensor(0x10, 0x20);
	n4_write_cmos_sensor(0x11, 0x20);
	n4_write_cmos_sensor(0x12, 0x20);
	n4_write_cmos_sensor(0x13, 0x20);
	n4_write_cmos_sensor(0x14, 0x00);
	n4_write_cmos_sensor(0x15, 0x00);
	n4_write_cmos_sensor(0x16, 0x00);
	n4_write_cmos_sensor(0x17, 0x00);
	n4_write_cmos_sensor(0x18, 0x11);//for signal detect stable
	n4_write_cmos_sensor(0x19, 0x11);
	n4_write_cmos_sensor(0x1a, 0x11);
	n4_write_cmos_sensor(0x1b, 0x11);
	n4_write_cmos_sensor(0x1c, 0x18);//enable black pattern
	n4_write_cmos_sensor(0x1d, 0x18);
	n4_write_cmos_sensor(0x1e, 0x18);
	n4_write_cmos_sensor(0x1f, 0x18);
	n4_write_cmos_sensor(0x20, 0x00);
	n4_write_cmos_sensor(0x21, 0x00);
	n4_write_cmos_sensor(0x22, 0x00);
	n4_write_cmos_sensor(0x23, 0x00);
	n4_write_cmos_sensor(0x24, 0x86);
	n4_write_cmos_sensor(0x25, 0x86);
	n4_write_cmos_sensor(0x26, 0x86);
	n4_write_cmos_sensor(0x27, 0x86);
	n4_write_cmos_sensor(0x28, 0x80);
	n4_write_cmos_sensor(0x29, 0x80);
	n4_write_cmos_sensor(0x2a, 0x80);
	n4_write_cmos_sensor(0x2b, 0x80);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);
	n4_write_cmos_sensor(0x33, 0x00);
	n4_write_cmos_sensor(0x34, 0x00);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0x00);
	n4_write_cmos_sensor(0x37, 0x00);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x48, 0x00);
	n4_write_cmos_sensor(0x49, 0x00);
	n4_write_cmos_sensor(0x4a, 0x00);
	n4_write_cmos_sensor(0x4b, 0x00);
	n4_write_cmos_sensor(0x4c, 0xfe);
	n4_write_cmos_sensor(0x4d, 0xfe);
	n4_write_cmos_sensor(0x4e, 0xfe);
	n4_write_cmos_sensor(0x4f, 0xfe);
	n4_write_cmos_sensor(0x50, 0xfb);
	n4_write_cmos_sensor(0x51, 0xfb);
	n4_write_cmos_sensor(0x52, 0xfb);
	n4_write_cmos_sensor(0x53, 0xfb);
	n4_write_cmos_sensor(0x58, 0x80);
	n4_write_cmos_sensor(0x59, 0x80);
	n4_write_cmos_sensor(0x5a, 0x80);
	n4_write_cmos_sensor(0x5b, 0x80);
	n4_write_cmos_sensor(0x5c, 0x82);
	n4_write_cmos_sensor(0x5d, 0x82);
	n4_write_cmos_sensor(0x5e, 0x82);
	n4_write_cmos_sensor(0x5f, 0x82);
	n4_write_cmos_sensor(0x60, 0x10);
	n4_write_cmos_sensor(0x61, 0x10);
	n4_write_cmos_sensor(0x62, 0x10);
	n4_write_cmos_sensor(0x63, 0x10);
	n4_write_cmos_sensor(0x64, 0x05);
	n4_write_cmos_sensor(0x65, 0x05);
	n4_write_cmos_sensor(0x66, 0x05);
	n4_write_cmos_sensor(0x67, 0x05);
	n4_write_cmos_sensor(0x68, 0x48);
	n4_write_cmos_sensor(0x69, 0x48);
	n4_write_cmos_sensor(0x6a, 0x48);
	n4_write_cmos_sensor(0x6b, 0x48);
	n4_write_cmos_sensor(0x6c, 0x00);
	n4_write_cmos_sensor(0x6d, 0x00);
	n4_write_cmos_sensor(0x6e, 0x00);
	n4_write_cmos_sensor(0x6f, 0x00);
	n4_write_cmos_sensor(0x78, 0x21);
	n4_write_cmos_sensor(0x79, 0x21);
	n4_write_cmos_sensor(0x7a, 0x21);
	n4_write_cmos_sensor(0x7b, 0x21);


	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x7C, 0x00);
	n4_write_cmos_sensor(0x84, 0x04);
	n4_write_cmos_sensor(0x85, 0x04);
	n4_write_cmos_sensor(0x86, 0x04);
	n4_write_cmos_sensor(0x87, 0x04);
	n4_write_cmos_sensor(0x88, 0x01);
	n4_write_cmos_sensor(0x89, 0x01);
	n4_write_cmos_sensor(0x8a, 0x01);
	n4_write_cmos_sensor(0x8b, 0x01);
	n4_write_cmos_sensor(0x8c, 0x02);
	n4_write_cmos_sensor(0x8d, 0x02);
	n4_write_cmos_sensor(0x8e, 0x02);
	n4_write_cmos_sensor(0x8f, 0x02);
	n4_write_cmos_sensor(0xEC, 0x00);
	n4_write_cmos_sensor(0xED, 0x00);
	n4_write_cmos_sensor(0xEE, 0x00);
	n4_write_cmos_sensor(0xEF, 0x00);

	n4_write_cmos_sensor(0xFF, 0x05);
	n4_write_cmos_sensor(0x00, 0xd0);
	n4_write_cmos_sensor(0x01, 0x2c);
	n4_write_cmos_sensor(0x05, 0x24);
	n4_write_cmos_sensor(0x1d, 0x0c);
	n4_write_cmos_sensor(0x24, 0x2a);
	n4_write_cmos_sensor(0x25, 0xdc);
	n4_write_cmos_sensor(0x26, 0x40);
	n4_write_cmos_sensor(0x27, 0x57);
	n4_write_cmos_sensor(0x28, 0x80);
	n4_write_cmos_sensor(0x2b, 0xa8);
	n4_write_cmos_sensor(0x31, 0x82);
	n4_write_cmos_sensor(0x32, 0x10);
	n4_write_cmos_sensor(0x38, 0x13);
	n4_write_cmos_sensor(0x47, 0xEE);
	n4_write_cmos_sensor(0x50, 0xc6);
	n4_write_cmos_sensor(0x53, 0x00);
	n4_write_cmos_sensor(0x57, 0x00);
	n4_write_cmos_sensor(0x58, 0x77);
	n4_write_cmos_sensor(0x59, 0x00);
	n4_write_cmos_sensor(0x5C, 0x78);
	n4_write_cmos_sensor(0x5F, 0x00);
	n4_write_cmos_sensor(0x62, 0x20);
	n4_write_cmos_sensor(0x64, 0x00);
	n4_write_cmos_sensor(0x65, 0x00);
	n4_write_cmos_sensor(0x69, 0x00);
	n4_write_cmos_sensor(0x6E, 0x00);
	n4_write_cmos_sensor(0x6F, 0x00);
	n4_write_cmos_sensor(0x90, 0x01);
	n4_write_cmos_sensor(0x92, 0x00);
	n4_write_cmos_sensor(0x94, 0x00);
	n4_write_cmos_sensor(0x95, 0x00);
	n4_write_cmos_sensor(0xa9, 0x00);
	n4_write_cmos_sensor(0xb5, 0x80);
	n4_write_cmos_sensor(0xb7, 0xfc);
	n4_write_cmos_sensor(0xb8, 0x39);
	n4_write_cmos_sensor(0xb9, 0x72);
	n4_write_cmos_sensor(0xbb, 0x0f);
	n4_write_cmos_sensor(0xd1, 0x30);
	n4_write_cmos_sensor(0xd5, 0x80);

	n4_write_cmos_sensor(0xFF, 0x09);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x50, 0x30);
	n4_write_cmos_sensor(0x51, 0x6f);
	n4_write_cmos_sensor(0x52, 0x67);
	n4_write_cmos_sensor(0x53, 0x48);
	n4_write_cmos_sensor(0x54, 0x30);
	n4_write_cmos_sensor(0x55, 0x6f);
	n4_write_cmos_sensor(0x56, 0x67);
	n4_write_cmos_sensor(0x57, 0x48);
	n4_write_cmos_sensor(0x58, 0x30);
	n4_write_cmos_sensor(0x59, 0x6f);
	n4_write_cmos_sensor(0x5a, 0x67);
	n4_write_cmos_sensor(0x5b, 0x48);
	n4_write_cmos_sensor(0x5c, 0x30);
	n4_write_cmos_sensor(0x5d, 0x6f);
	n4_write_cmos_sensor(0x5e, 0x67);
	n4_write_cmos_sensor(0x5f, 0x48);
	n4_write_cmos_sensor(0x96, 0x00);
	n4_write_cmos_sensor(0x97, 0x00);
	n4_write_cmos_sensor(0x98, 0x00);
	n4_write_cmos_sensor(0x99, 0x00);
	n4_write_cmos_sensor(0x9a, 0x00);
	n4_write_cmos_sensor(0x9b, 0x00);
	n4_write_cmos_sensor(0x9c, 0x00);
	n4_write_cmos_sensor(0x9d, 0x00);
	n4_write_cmos_sensor(0x9e, 0x00);
	n4_write_cmos_sensor(0xb6, 0x00);
	n4_write_cmos_sensor(0xb7, 0x00);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x00);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbe, 0x00);
	n4_write_cmos_sensor(0xd6, 0x00);
	n4_write_cmos_sensor(0xd7, 0x00);
	n4_write_cmos_sensor(0xd8, 0x00);
	n4_write_cmos_sensor(0xd9, 0x00);
	n4_write_cmos_sensor(0xda, 0x00);
	n4_write_cmos_sensor(0xdb, 0x00);
	n4_write_cmos_sensor(0xdc, 0x00);
	n4_write_cmos_sensor(0xdd, 0x00);
	n4_write_cmos_sensor(0xde, 0x00);
	n4_write_cmos_sensor(0xf6, 0x00);
	n4_write_cmos_sensor(0xf7, 0x00);
	n4_write_cmos_sensor(0xf8, 0x00);
	n4_write_cmos_sensor(0xf9, 0x00);
	n4_write_cmos_sensor(0xfa, 0x00);
	n4_write_cmos_sensor(0xfb, 0x00);
	n4_write_cmos_sensor(0xfc, 0x00);
	n4_write_cmos_sensor(0xfd, 0x00);
	n4_write_cmos_sensor(0xfe, 0x00);

	n4_write_cmos_sensor(0xff, 0x0a);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xff, 0x0b);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);

	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0x97, 0x00);
	n4_write_cmos_sensor(0x97, 0x0F);

	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x3E, 0x00);
	n4_write_cmos_sensor(0x3F, 0x3F);

	n4_write_cmos_sensor(0xFF, 0x20);
	n4_write_cmos_sensor(0x01, 0x00);
	n4_write_cmos_sensor(0x00, 0x00);
	n4_write_cmos_sensor(0x0F, 0x00);
	n4_write_cmos_sensor(0x0D, 0x01);//4lane mode
	n4_write_cmos_sensor(0x00, 0xFF);//ch1/2/3/4 enabled
	n4_write_cmos_sensor(0x40, 0x01);
	n4_write_cmos_sensor(0x40, 0x00);

	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0xC8, 0x00);
	n4_write_cmos_sensor(0xC9, 0x00);
	n4_write_cmos_sensor(0xCA, 0x00);
	n4_write_cmos_sensor(0xCB, 0x00);

}

static void n4_common_settings(void)
{
	LOG_INF("n4_common_settings ENTER\n");
	n4_write_cmos_sensor(0xff, 0x04);
	n4_write_cmos_sensor(0xa0, 0x24);
	n4_write_cmos_sensor(0xa1, 0x24);
	n4_write_cmos_sensor(0xa2, 0x24);
	n4_write_cmos_sensor(0xa3, 0x24);
	n4_write_cmos_sensor(0xa4, 0x24);
	n4_write_cmos_sensor(0xa5, 0x24);
	n4_write_cmos_sensor(0xa6, 0x24);
	n4_write_cmos_sensor(0xa7, 0x24);
	n4_write_cmos_sensor(0xa8, 0x24);
	n4_write_cmos_sensor(0xa9, 0x24);
	n4_write_cmos_sensor(0xaa, 0x24);
	n4_write_cmos_sensor(0xab, 0x24);
	n4_write_cmos_sensor(0xac, 0x24);
	n4_write_cmos_sensor(0xad, 0x24);
	n4_write_cmos_sensor(0xae, 0x24);
	n4_write_cmos_sensor(0xaf, 0x24);
	n4_write_cmos_sensor(0xb0, 0x24);
	n4_write_cmos_sensor(0xb1, 0x24);
	n4_write_cmos_sensor(0xb2, 0x24);
	n4_write_cmos_sensor(0xb3, 0x24);
	n4_write_cmos_sensor(0xb4, 0x24);
	n4_write_cmos_sensor(0xb5, 0x24);
	n4_write_cmos_sensor(0xb6, 0x24);
	n4_write_cmos_sensor(0xb7, 0x24);
	n4_write_cmos_sensor(0xb8, 0x24);
	n4_write_cmos_sensor(0xb9, 0x24);
	n4_write_cmos_sensor(0xba, 0x24);
	n4_write_cmos_sensor(0xbb, 0x24);
	n4_write_cmos_sensor(0xbc, 0x24);
	n4_write_cmos_sensor(0xbd, 0x24);
	n4_write_cmos_sensor(0xbe, 0x24);
	n4_write_cmos_sensor(0xbf, 0x24);
	n4_write_cmos_sensor(0xc0, 0x24);
	n4_write_cmos_sensor(0xc1, 0x24);
	n4_write_cmos_sensor(0xc2, 0x24);
	n4_write_cmos_sensor(0xc3, 0x24);
	n4_write_cmos_sensor(0xff, 0x21);
	n4_write_cmos_sensor(0x07, 0x80);
	n4_write_cmos_sensor(0x07, 0x00);
	n4_write_cmos_sensor(0xff, 0x0A);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);
	n4_write_cmos_sensor(0xff, 0x0B);
	n4_write_cmos_sensor(0x77, 0x8F);
	n4_write_cmos_sensor(0xF7, 0x8F);
	n4_write_cmos_sensor(0xFF, 0x01);
	n4_write_cmos_sensor(0xC8, 0x00);
	n4_write_cmos_sensor(0xC9, 0x00);
	n4_write_cmos_sensor(0xCA, 0x00);
	n4_write_cmos_sensor(0xCB, 0x00);
	n4_write_cmos_sensor(0xFF, 0x09);
	n4_write_cmos_sensor(0x40, 0x00);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x00);
	n4_write_cmos_sensor(0x43, 0x00);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x45, 0x00);
	n4_write_cmos_sensor(0x46, 0x00);
	n4_write_cmos_sensor(0x47, 0x00);
	n4_write_cmos_sensor(0x50, 0x30);
	n4_write_cmos_sensor(0x51, 0x6f);
	n4_write_cmos_sensor(0x52, 0x67);
	n4_write_cmos_sensor(0x53, 0x48);
	n4_write_cmos_sensor(0x54, 0x30);
	n4_write_cmos_sensor(0x55, 0x6f);
	n4_write_cmos_sensor(0x56, 0x67);
	n4_write_cmos_sensor(0x57, 0x48);
	n4_write_cmos_sensor(0x58, 0x30);
	n4_write_cmos_sensor(0x59, 0x6f);
	n4_write_cmos_sensor(0x5a, 0x67);
	n4_write_cmos_sensor(0x5b, 0x48);
	n4_write_cmos_sensor(0x5c, 0x30);
	n4_write_cmos_sensor(0x5d, 0x6f);
	n4_write_cmos_sensor(0x5e, 0x67);
	n4_write_cmos_sensor(0x5f, 0x48);
	n4_write_cmos_sensor(0x96, 0x00);
	n4_write_cmos_sensor(0x97, 0x00);
	n4_write_cmos_sensor(0x98, 0x00);
	n4_write_cmos_sensor(0x99, 0x00);
	n4_write_cmos_sensor(0x9a, 0x00);
	n4_write_cmos_sensor(0x9b, 0x00);
	n4_write_cmos_sensor(0x9c, 0x00);
	n4_write_cmos_sensor(0x9d, 0x00);
	n4_write_cmos_sensor(0x9e, 0x00);
	n4_write_cmos_sensor(0xb6, 0x00);
	n4_write_cmos_sensor(0xb7, 0x00);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x00);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbe, 0x00);
	n4_write_cmos_sensor(0xd6, 0x00);
	n4_write_cmos_sensor(0xd7, 0x00);
	n4_write_cmos_sensor(0xd8, 0x00);
	n4_write_cmos_sensor(0xd9, 0x00);
	n4_write_cmos_sensor(0xda, 0x00);
	n4_write_cmos_sensor(0xdb, 0x00);
	n4_write_cmos_sensor(0xdc, 0x00);
	n4_write_cmos_sensor(0xdd, 0x00);
	n4_write_cmos_sensor(0xde, 0x00);
	n4_write_cmos_sensor(0xf6, 0x00);
	n4_write_cmos_sensor(0xf7, 0x00);
	n4_write_cmos_sensor(0xf8, 0x00);
	n4_write_cmos_sensor(0xf9, 0x00);
	n4_write_cmos_sensor(0xfa, 0x00);
	n4_write_cmos_sensor(0xfb, 0x00);
	n4_write_cmos_sensor(0xfc, 0x00);
	n4_write_cmos_sensor(0xfd, 0x00);
	n4_write_cmos_sensor(0xfe, 0x00);
	n4_write_cmos_sensor(0xff, 0x0a);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);
	n4_write_cmos_sensor(0xff, 0x0b);
	n4_write_cmos_sensor(0x3d, 0x00);
	n4_write_cmos_sensor(0x3c, 0x00);
	n4_write_cmos_sensor(0x30, 0xac);
	n4_write_cmos_sensor(0x31, 0x78);
	n4_write_cmos_sensor(0x32, 0x17);
	n4_write_cmos_sensor(0x33, 0xc1);
	n4_write_cmos_sensor(0x34, 0x40);
	n4_write_cmos_sensor(0x35, 0x00);
	n4_write_cmos_sensor(0x36, 0xc3);
	n4_write_cmos_sensor(0x37, 0x0a);
	n4_write_cmos_sensor(0x38, 0x00);
	n4_write_cmos_sensor(0x39, 0x02);
	n4_write_cmos_sensor(0x3a, 0x00);
	n4_write_cmos_sensor(0x3b, 0xb2);
	n4_write_cmos_sensor(0x25, 0x10);
	n4_write_cmos_sensor(0x27, 0x1e);
	n4_write_cmos_sensor(0xbd, 0x00);
	n4_write_cmos_sensor(0xbc, 0x00);
	n4_write_cmos_sensor(0xb0, 0xac);
	n4_write_cmos_sensor(0xb1, 0x78);
	n4_write_cmos_sensor(0xb2, 0x17);
	n4_write_cmos_sensor(0xb3, 0xc1);
	n4_write_cmos_sensor(0xb4, 0x40);
	n4_write_cmos_sensor(0xb5, 0x00);
	n4_write_cmos_sensor(0xb6, 0xc3);
	n4_write_cmos_sensor(0xb7, 0x0a);
	n4_write_cmos_sensor(0xb8, 0x00);
	n4_write_cmos_sensor(0xb9, 0x02);
	n4_write_cmos_sensor(0xba, 0x00);
	n4_write_cmos_sensor(0xbb, 0xb2);
	n4_write_cmos_sensor(0xa5, 0x10);
	n4_write_cmos_sensor(0xa7, 0x1e);
	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x40, 0xB4);
	n4_write_cmos_sensor(0x41, 0x00);
	n4_write_cmos_sensor(0x42, 0x03);
	n4_write_cmos_sensor(0x43, 0x43);
	n4_write_cmos_sensor(0x11, 0x08);
	n4_write_cmos_sensor(0x10, 0x13);
	n4_write_cmos_sensor(0x12, 0x0B);
	n4_write_cmos_sensor(0x13, 0x12);
	n4_write_cmos_sensor(0x17, 0x02);
	n4_write_cmos_sensor(0x18, 0x12);
	n4_write_cmos_sensor(0x15, 0x07);
	n4_write_cmos_sensor(0x14, 0x2D);
	n4_write_cmos_sensor(0x16, 0x0B);
	n4_write_cmos_sensor(0x19, 0x09);
	n4_write_cmos_sensor(0x1A, 0x15);
	n4_write_cmos_sensor(0x1B, 0x11);
	n4_write_cmos_sensor(0x1C, 0x0E);
	n4_write_cmos_sensor(0x44, 0x00);
	n4_write_cmos_sensor(0x49, 0xF3);
	n4_write_cmos_sensor(0x49, 0xF0);
	n4_write_cmos_sensor(0x44, 0x02);
	n4_write_cmos_sensor(0x08, 0x40);
	n4_write_cmos_sensor(0x0F, 0x01);
	n4_write_cmos_sensor(0x38, 0x1E);
	n4_write_cmos_sensor(0x39, 0x1E);
	n4_write_cmos_sensor(0x3A, 0x1E);
	n4_write_cmos_sensor(0x3B, 0x1E);
	n4_write_cmos_sensor(0x07, 0x0F);
	n4_write_cmos_sensor(0x2D, 0x01);
	n4_write_cmos_sensor(0x45, 0x02);
	n4_write_cmos_sensor(0xFF, 0x21);
	n4_write_cmos_sensor(0x3E, 0x00);
	n4_write_cmos_sensor(0x3F, 0x3F);
	n4_write_cmos_sensor(0xFF, 0x13);
	n4_write_cmos_sensor(0x30, 0x00);
	n4_write_cmos_sensor(0x31, 0x00);
	n4_write_cmos_sensor(0x32, 0x00);
}






static UINT32 n4_getsensorid(uint32_t *sensorId)
{
	int retry = 3;
	do {
		n4_write_cmos_sensor(0xff,0x00);
		*sensorId = ((n4_read_cmos_sensor(0xf4) << 8) | n4_read_cmos_sensor(0xf5));
		LOG_INF("N4_GetSensorID: sensor id = 0x%x\n", *sensorId);
		if (*sensorId == SENSOR_ID)
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

static UINT32 n4_preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&N4SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

static UINT32 n4_capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&N4SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	LOG_INF("Leave\n");
	return ERROR_NONE;
}

static UINT32 n4_getresolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
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
	LOG_DBG("input_source_width = %d,input_source_height = %d\n",input_source_width,input_source_height);
	return ERROR_NONE;
}

SIGNAL_STATUS n4_get_signalstatus(int ch)
{
	int value = 0xff;
	SIGNAL_STATUS signal_status = SIGNAL_IDLE;

	n4_write_cmos_sensor(0xff, 0x00);
	value = n4_read_cmos_sensor(0xa0);
	if ((value & (0x01 << ch)) == 0x00 )
	{
		signal_status = SIGNAL_READY;
	}
	else
	{
		signal_lost_cnt++;
		if (4 < signal_lost_cnt) {
			signal_status = SIGNAL_LOST;
		}
	}

	//LOG_INF("get signal status leave and current ch is %d,signal status is %d,value = 0x%x\n",ch,signal_status,value);

	return signal_status;

}

static int n4_detect_format_avm(int ch)
{
	int temp_value = 0;
	int retry = 20;
	n4_write_cmos_sensor(0xff,0x00);
	n4_write_cmos_sensor(0x18+(ch%4),0x13);
	n4_write_cmos_sensor(0xff,0x05 + (ch%4));
	n4_write_cmos_sensor(0x82,0xFF);
	n4_write_cmos_sensor(0x32,0x13);
	n4_write_cmos_sensor(0x00,0xF0);
	n4_write_cmos_sensor(0x01,0x2c);
	n4_write_cmos_sensor(0x5c,0x7f);
	n4_write_cmos_sensor(0x58,0x47);
	n4_write_cmos_sensor(0xB8,0xB8);

	n4_write_cmos_sensor(0xFF,0x01);
	do {
		temp_value = n4_read_cmos_sensor(0x10 + (0x20 * ch));
		LOG_INF("ch = %d,value = 0x%x\n",ch,temp_value);
		if (0xf != (temp_value & 0xf)) {
			LOG_INF("N4 Channel = %d format is 0x%x,retry = %d\n",ch,temp_value,(20-retry));
			break;
		}
		retry --;
		mdelay(40);
	} while (retry > 0);
	n4_write_cmos_sensor(0xff,0x05 + (ch%4));
	n4_write_cmos_sensor(0x82,0x00);//must  be set 00,or will get error signal status
	n4_write_cmos_sensor(0x32,0x10);

	switch (temp_value) {
	case CH_VALUE_NTSC:
		input_source_width_ch0 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		break;
	case CH_VALUE_PAL:
		input_source_width_ch0 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		break;
	case CH_VALUE_HD25:
		input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
		break;
	case CH_VALUE_HD30:
		input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
		break;
	case CH_VALUE_FHD25:
		input_source_width_ch0 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		break;
	case CH_VALUE_FHD30:
		input_source_width_ch0 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		break;
	default:
		input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
		break;
	}

	return temp_value;
}

static int n4_detect_format(int ch)
{
	int temp_value = 0;
	int retry = 80;
	n4_write_cmos_sensor(0xff,0x00);
	n4_write_cmos_sensor(0x18+(ch%4),0x11);
	n4_write_cmos_sensor(0xff,0x05 + (ch%4));
	n4_write_cmos_sensor(0x00,0xD0);
	n4_write_cmos_sensor(0x01,0x2C);
	n4_write_cmos_sensor(0x58,0x77);
	n4_write_cmos_sensor(0x82,0xFF);
	n4_write_cmos_sensor(0xFF,0x13);
	n4_write_cmos_sensor(0x30,0xFF);
	n4_write_cmos_sensor(0x31,0xFF);
	n4_write_cmos_sensor(0x32,0xFF);
	n4_write_cmos_sensor(0xFF,0x01);
	do {
		temp_value = n4_read_cmos_sensor(0x10 + (0x20 * ch));
		LOG_INF("ch = %d,value = 0x%x\n",ch,temp_value);
		if (0xf != (temp_value & 0xf)) {
			LOG_INF("N4 Channel = %d format is 0x%x,retry = %d\n",ch,temp_value,retry);
			break;
		}
		retry --;
		mdelay(10);
	} while (retry > 0);
	n4_write_cmos_sensor(0xff,0x05 + (ch%4));
	n4_write_cmos_sensor(0x82,0x00);//must  be set 00,or will get error signal status

	switch (ch) {
		case 0:
			switch (temp_value) {
				case CH_VALUE_NTSC:
					input_source_width_ch0 = N4_IMAGE_SENSOR_NTSC_WIDTH;
					input_source_height_ch0 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
					break;
				case CH_VALUE_PAL:
					input_source_width_ch0 = N4_IMAGE_SENSOR_PAL_WIDTH;
					input_source_height_ch0 = N4_IMAGE_SENSOR_PAL_HEIGHT;
					break;
				case CH_VALUE_HD25:
					input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
				case CH_VALUE_HD30:
					input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
				case CH_VALUE_FHD25:
					input_source_width_ch0 = N4_IMAGE_SENSOR_FHD_WIDTH;
					input_source_height_ch0 = N4_IMAGE_SENSOR_FHD_HEIGHT;
					break;
				case CH_VALUE_FHD30:
					input_source_width_ch0 = N4_IMAGE_SENSOR_FHD_WIDTH;
					input_source_height_ch0 = N4_IMAGE_SENSOR_FHD_HEIGHT;
					break;
				default:
					input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
			}
			break;
		case 1:
			switch (temp_value) {
				case CH_VALUE_NTSC:
					input_source_width_ch1 = N4_IMAGE_SENSOR_NTSC_WIDTH;
					input_source_height_ch1 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
					break;
				case CH_VALUE_PAL:
					input_source_width_ch1 = N4_IMAGE_SENSOR_PAL_WIDTH;
					input_source_height_ch1 = N4_IMAGE_SENSOR_PAL_HEIGHT;
					break;
				case CH_VALUE_HD25:
					input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
				case CH_VALUE_HD30:
					input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
				case CH_VALUE_FHD25:
					input_source_width_ch1 = N4_IMAGE_SENSOR_FHD_WIDTH;
					input_source_height_ch1 = N4_IMAGE_SENSOR_FHD_HEIGHT;
					break;
				case CH_VALUE_FHD30:
					input_source_width_ch1 = N4_IMAGE_SENSOR_FHD_WIDTH;
					input_source_height_ch1 = N4_IMAGE_SENSOR_FHD_HEIGHT;
					break;
				default:
					input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
			}
			break;
		case 2:
			switch (temp_value) {
				case CH_VALUE_NTSC:
					input_source_width_ch2 = N4_IMAGE_SENSOR_NTSC_WIDTH;
					input_source_height_ch2 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
					break;
				case CH_VALUE_PAL:
					input_source_width_ch2 = N4_IMAGE_SENSOR_PAL_WIDTH;
					input_source_height_ch2 = N4_IMAGE_SENSOR_PAL_HEIGHT;
					break;
				case CH_VALUE_HD25:
					input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
				case CH_VALUE_HD30:
					input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
				case CH_VALUE_FHD25:
					input_source_width_ch2 = N4_IMAGE_SENSOR_FHD_WIDTH;
					input_source_height_ch2 = N4_IMAGE_SENSOR_FHD_HEIGHT;
					break;
				case CH_VALUE_FHD30:
					input_source_width_ch2 = N4_IMAGE_SENSOR_FHD_WIDTH;
					input_source_height_ch2 = N4_IMAGE_SENSOR_FHD_HEIGHT;
					break;
				default:
					input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
			}
			break;
		case 3:
			switch (temp_value) {
				case CH_VALUE_NTSC:
					input_source_width_ch3 = N4_IMAGE_SENSOR_NTSC_WIDTH;
					input_source_height_ch3 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
					break;
				case CH_VALUE_PAL:
					input_source_width_ch3 = N4_IMAGE_SENSOR_PAL_WIDTH;
					input_source_height_ch3 = N4_IMAGE_SENSOR_PAL_HEIGHT;
					break;
				case CH_VALUE_HD25:
					input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
				case CH_VALUE_HD30:
					input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
				case CH_VALUE_FHD25:
					input_source_width_ch3 = N4_IMAGE_SENSOR_FHD_WIDTH;
					input_source_height_ch3 = N4_IMAGE_SENSOR_FHD_HEIGHT;
					break;
				case CH_VALUE_FHD30:
					input_source_width_ch3 = N4_IMAGE_SENSOR_FHD_WIDTH;
					input_source_height_ch3 = N4_IMAGE_SENSOR_FHD_HEIGHT;
					break;
				default:
					input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
					input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
					break;
			}
			break;
		default:
			break;
	}
	return temp_value;
}

static int n4_fastavm_metazone_handle(unsigned int value)
{
	LOG_INF("enter\n");
	switch (value) {
	case FORMAT_NTSC:
		n4_common_settings();
		n4_sensor_ntsc_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		interlace_flag[0] = 1;
		interlace_flag[1] = 1;
		interlace_flag[2] = 1;
		interlace_flag[3] = 1;
		break;
	case FORMAT_PAL:
		n4_common_settings();
		n4_sensor_pal_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		interlace_flag[0] = 1;
		interlace_flag[1] = 1;
		interlace_flag[2] = 1;
		interlace_flag[3] = 1;
		break;
	case FORMAT_HD25:
		n4_common_settings();
		n4_sensor_hd25_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
		interlace_flag[0] = 0;
		interlace_flag[1] = 0;
		interlace_flag[2] = 0;
		interlace_flag[3] = 0;
		break;
	case FORMAT_HD30:
		n4_common_settings();
		n4_sensor_hd30_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
		interlace_flag[0] = 0;
		interlace_flag[1] = 0;
		interlace_flag[2] = 0;
		interlace_flag[3] = 0;
		break;
	case FORMAT_FHD25:
		n4_common_settings();
		n4_sensor_fhd25_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		interlace_flag[0] = 0;
		interlace_flag[1] = 0;
		interlace_flag[2] = 0;
		interlace_flag[3] = 0;
		break;
	case FORMAT_FHD30:
		n4_common_settings();
		n4_sensor_fhd30_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		interlace_flag[0] = 0;
		interlace_flag[1] = 0;
		interlace_flag[2] = 0;
		interlace_flag[3] = 0;
		break;
	default:
		return -1;
	}
	return ERROR_NONE;
}


static void n4_fastavm_metazone_invalid_handle(void)
{
	int n4_avm_fmt = CH_VALUE_HD30;

	LOG_INF("enter\n");
	n4_common_settings();
	n4_avm_fmt = n4_detect_format_avm(0);

	switch (n4_avm_fmt) {
	case CH_VALUE_NTSC:
		n4_sensor_ntsc_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_NTSC_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_NTSC_HEIGHT;
		interlace_flag[0] = 1;
		interlace_flag[1] = 1;
		interlace_flag[2] = 1;
		interlace_flag[3] = 1;
		break;
	case CH_VALUE_PAL:
		n4_sensor_pal_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_PAL_HEIGHT;
		interlace_flag[0] = 1;
		interlace_flag[1] = 1;
		interlace_flag[2] = 1;
		interlace_flag[3] = 1;
		break;
	case CH_VALUE_HD25:
		n4_sensor_hd25_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
		interlace_flag[0] = 0;
		interlace_flag[1] = 0;
		interlace_flag[2] = 0;
		interlace_flag[3] = 0;
		break;
	case CH_VALUE_HD30:
		n4_sensor_hd30_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
		interlace_flag[0] = 0;
		interlace_flag[1] = 0;
		interlace_flag[2] = 0;
		interlace_flag[3] = 0;
		break;
	case CH_VALUE_FHD25:
		n4_sensor_fhd25_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		interlace_flag[0] = 0;
		interlace_flag[1] = 0;
		interlace_flag[2] = 0;
		interlace_flag[3] = 0;
		break;
	case CH_VALUE_FHD30:
		n4_sensor_fhd30_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_FHD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_FHD_HEIGHT;
		interlace_flag[0] = 0;
		interlace_flag[1] = 0;
		interlace_flag[2] = 0;
		interlace_flag[3] = 0;
		break;
	default:
		LOG_ERR("can not detect valid format,uesd HD_30\n");
		n4_sensor_hd30_init();
		input_source_width_ch0 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch0 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch1 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch1 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch2 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch2 = N4_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width_ch3 = N4_IMAGE_SENSOR_HD_WIDTH;
		input_source_height_ch3 = N4_IMAGE_SENSOR_HD_HEIGHT;
		interlace_flag[0] = 0;
		interlace_flag[1] = 0;
		interlace_flag[2] = 0;
		interlace_flag[3] = 0;
		break;
	}
}

UINT32 n4_open(void)
{

	UINT32 sensor_id = 0;
	int n4_format_ch0 = CH_VALUE_HD30;
	int n4_format_ch1 = CH_VALUE_HD30;
	int n4_format_ch2 = CH_VALUE_HD30;
	int n4_format_ch3 = CH_VALUE_HD30;
	int mipi_ch_en = 0xff;
	int ret = 0;
	unsigned int value = 0;

	LOG_INF("N4Open Enter\n");

	n4_getsensorid(&sensor_id);
	if (sensor_id != SENSOR_ID)
	{
		LOG_ERR("N4 YUV read sensor id failed\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	if (1 == n4_is_avm) {
		if (MZ_SUCCESS != MetaZone_Read(METAZONE_FAST_AVM_MODE, &value)) {
			LOG_INF("MetaZone_Read(METAZONE_FAST_AVM_MODE) fail\n");
		} else {
			LOG_INF("MetaZone_Read success and METAZONE_FAST_AVM_MODE = %d \n", value);
		}

		if (MZ_SUCCESS != MetaZone_Read(METAZONE_FAST_AVM_FMT, &value)) {
			LOG_ERR("MetaZone_Read(METAZONE_FAST_AVM_FMT) fail\n");
		} else {
			LOG_INF("MetaZone_Read success and METAZONE_FAST_AVM_FMT = %d \n", value);
			ret = n4_fastavm_metazone_handle(value);
			if (ret < 0) {
				n4_fastavm_metazone_invalid_handle();
			}
		}
	}

	if (0 == n4_is_avm) {
		n4_format_ch0 = n4_detect_format(0);
		if (0xff == n4_format_ch0) {
			n4_format_ch0 = 0x20;
		}
		mipi_ch_en = mipi_ch_en |0x11;

		n4_format_ch1 = n4_detect_format(1);
		if (0xff == n4_format_ch1) {
			n4_format_ch1 = 0x20;
		}
		mipi_ch_en = mipi_ch_en |0x22;

		n4_format_ch2 = n4_detect_format(2);
		if (0xff == n4_format_ch2) {
			n4_format_ch2 = 0x20;
		}
		mipi_ch_en = mipi_ch_en | 0x44;

		n4_format_ch3 = n4_detect_format(3);
		if (0xff == n4_format_ch3) {
			n4_format_ch3 = 0x20;
		}
		mipi_ch_en = mipi_ch_en | 0x88;

		if ((CH_VALUE_NTSC == n4_format_ch0) || (CH_VALUE_PAL == n4_format_ch0)) {
			interlace_flag[0] = 1;
		} else {
			interlace_flag[0] = 0;
		}

		if ((CH_VALUE_NTSC == n4_format_ch1) || (CH_VALUE_PAL == n4_format_ch1)) {
			interlace_flag[1] = 1;
		} else {
			interlace_flag[1] = 0;
		}

		if ((CH_VALUE_NTSC == n4_format_ch2) || (CH_VALUE_PAL == n4_format_ch2)) {
			interlace_flag[2] = 1;
		} else {
			interlace_flag[2]= 0;
		}

		if ((CH_VALUE_NTSC == n4_format_ch3) || (CH_VALUE_PAL == n4_format_ch3)) {
			interlace_flag[3] = 1;
		} else {
			interlace_flag[3] = 0;
		}

		LOG_INF("4 channel format is ch0 = 0x%x,ch1 = 0x%x,ch2 = 0x%x,ch3 = 0x%x\n",n4_format_ch0,n4_format_ch1,n4_format_ch2,n4_format_ch3);
		n4_common_settings();

		n4_init_ch(0,n4_format_ch0);
		n4_init_ch(1,n4_format_ch1);
		n4_init_ch(2,n4_format_ch2);
		n4_init_ch(3,n4_format_ch3);

		n4_mipi_outputs(mipi_ch_en);
	}

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 n4_close(void)
{
	LOG_INF("N4Close");
	n4_is_avm = 1;
	signal_lost_cnt = 0;
	return ERROR_NONE;
}

UINT32 n4_getinfo(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
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

		memcpy(pSensorConfigData, &N4SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

		return ERROR_NONE;

}

UINT32 n4_control(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	LOG_INF("Enter, ScenarioId = %d\n", ScenarioId);
	switch (ScenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			n4_preview(pImageWindow, pSensorConfigData);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			n4_capture(pImageWindow, pSensorConfigData);
			break;
		default:
			break;
	}

	LOG_INF("Leave\n");

	return ERROR_NONE;
}

UINT32 n4_featurecontrol(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
			n4_isp_master_clock = *pFeatureData32;
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			n4_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = n4_read_cmos_sensor(pSensorRegData->RegAddr);
			break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &N4SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
			n4_getsensorid(pFeatureData32);
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
			*pFeatureData32 = n4_get_signalstatus(0);
			break;
		case SENSOR_FEATURE_GET_CH1_SIGNAL_STATUS:
			*pFeatureData32 = n4_get_signalstatus(1);
			break;
		case SENSOR_FEATURE_GET_CH2_SIGNAL_STATUS:
			*pFeatureData32 = n4_get_signalstatus(2);
			break;
		case SENSOR_FEATURE_GET_CH3_SIGNAL_STATUS:
			*pFeatureData32 = n4_get_signalstatus(3);
			break;
		case SENSOR_FEATURE_SET_IS_AVM:
			n4_is_avm = *pFeatureData32;
			break;
		case SENSOR_FEATURE_GET_CH0_INTERLACE_FLAG:
			if (1 == interlace_flag[0]) {
				*pFeatureData32 = 1;
			} else {
				*pFeatureData32 = 0;
			}
			break;
		case SENSOR_FEATURE_GET_CH1_INTERLACE_FLAG:
			if (1 == interlace_flag[1]) {
				*pFeatureData32 = 1;
			} else {
				*pFeatureData32 = 0;
			}
			break;
		case SENSOR_FEATURE_GET_CH2_INTERLACE_FLAG:
			if (1 == interlace_flag[2]) {
				*pFeatureData32 = 1;
			} else {
				*pFeatureData32 = 0;
			}
			break;
		case SENSOR_FEATURE_GET_CH3_INTERLACE_FLAG:
			if (1 == interlace_flag[3]) {
				*pFeatureData32 = 1;
			} else {
				*pFeatureData32 = 0;
			}
			break;
	case SENSOR_FEATURE_SET_BRIGHTNESS_VALUE:
		channel = *feature_data;
		pq_value = *(feature_data + 1);
		LOG_INF("Channel is %d, brightness value is %d\n", channel, pq_value);
		switch (channel) {
		case 0:
			n4_write_cmos_sensor(0x20, pq_value);//default is 0, -128~127
			break;
		case 1:
			n4_write_cmos_sensor(0x21, pq_value);//default is 0, -128~127
			break;
		case 2:
			n4_write_cmos_sensor(0x22, pq_value);//default is 0, -128~127
			break;
		case 3:
			n4_write_cmos_sensor(0x23, pq_value);//default is 0, -128~127
			break;
		}
		break;
	case SENSOR_FEATURE_SET_CONTRAST_VALUE:
		channel = *feature_data;
		pq_value = *(feature_data + 1);
		LOG_INF("Channel is %d, contrast value is %d\n", channel, pq_value);
		switch (channel) {
		case 0:
			n4_write_cmos_sensor(0x24, pq_value);//default is 128, 0~255
			break;
		case 1:
			n4_write_cmos_sensor(0x25, pq_value);//default is 128, 0~255
			break;
		case 2:
			n4_write_cmos_sensor(0x26, pq_value);//default is 128, 0~255
			break;
		case 3:
			n4_write_cmos_sensor(0x27, pq_value);//default is 128, 0~255
			break;
		}
		break;

	case SENSOR_FEATURE_SET_SATURATION_VALUE:
		channel = *feature_data;
		pq_value = *(feature_data + 1);
		LOG_INF("Channel is %d, saturation value is %d\n", channel, pq_value);
		switch (channel) {
		case 0:
			n4_write_cmos_sensor(0x58, pq_value);//default is 128, 0~255
			break;
		case 1:
			n4_write_cmos_sensor(0x59, pq_value);//default is 128, 0~255
			break;
		case 2:
			n4_write_cmos_sensor(0x5A, pq_value);//default is 128, 0~255
			break;
		case 3:
			n4_write_cmos_sensor(0x5B, pq_value);//default is 128, 0~255
			break;
		}
		break;
	case SENSOR_FEATURE_SET_HUE_VALUE:
		channel = *feature_data;
		pq_value = *(feature_data + 1);
		LOG_INF("Channel is %d, hue value is %d\n", channel, pq_value);
		switch (channel) {
		case 0:
			n4_write_cmos_sensor(0x40, pq_value);//default is 0, -128~127
			break;
		case 1:
			n4_write_cmos_sensor(0x41, pq_value);//default is 0, -128~127
			break;
		case 2:
			n4_write_cmos_sensor(0x42, pq_value);//default is 0, -128~127
			break;
		case 3:
			n4_write_cmos_sensor(0x43, pq_value);//default is 0, -128~127
			break;
		}
		break;
	default:
		break;
	}
	return ERROR_NONE;
}


static struct SENSOR_FUNCTION_STRUCT sensor_func =
{
	n4_open,
	n4_getinfo,
	n4_getresolution,
	n4_featurecontrol,
	n4_control,
	n4_close,
};

UINT32 N4_YUV_SensorInit(
			struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL)
	{
		*pfFunc = &sensor_func;
	}
	return ERROR_NONE;
}






