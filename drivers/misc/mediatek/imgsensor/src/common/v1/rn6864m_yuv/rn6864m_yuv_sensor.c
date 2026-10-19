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
*   RN6864M_yuv_Sensor.c
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

#include "rn6864m_yuv_sensor.h"

/****************************Modify Following Strings for Debug****************************/
#define PFX "[RN6864]"


/****************************   Modify end    *******************************************/
#define LOG_DBG(format, args...)    pr_debug(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    pr_info(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...)    pr_err(PFX " %s " format, __FUNCTION__, ##args)

static kal_uint16 RN6864M_i2c_write_id = 0x58;
static int RN6864M_i2c_speed = 400;
//static VIDEO_FORMAT source_format = FORMAT_MAX;
static bool RN6864M_Inputsource_Format_Detected = false;
kal_bool RN6864M_MPEG4_encode_mode = KAL_FALSE;
kal_uint32 RN6864M_isp_master_clock;


static kal_uint32 input_source_width = RN6864M_IMAGE_SENSOR_HD25_WIDTH;
static kal_uint32 input_source_height = RN6864M_IMAGE_SENSOR_HD25_HEIGHT;

MSDK_SENSOR_CONFIG_STRUCT RN6864MSensorConfigData;


static kal_uint16 RN6864M_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[1] = { (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 1, (u8 *) &get_byte, 1, RN6864M_i2c_write_id);

	return get_byte;
}

static int RN6864M_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[2] = { (char)(addr & 0xFF), (char)(para & 0xFF) };

	return iWriteRegI2CTiming(
		pu_send_cmd, 2, RN6864M_i2c_write_id, RN6864M_i2c_speed);
}

void RN6864M_Pre_Init(void)
{
	char value1, value2, value3, value4, value5, value6;

	LOG_INF("RN6864M pre init in\n");

	RN6864M_write_cmos_sensor(0xE1, 0x80);
	RN6864M_write_cmos_sensor(0xFA, 0x81);

	value1 = RN6864M_read_cmos_sensor(0xFB);
	value2 = RN6864M_read_cmos_sensor(0xFB);
	value3 = RN6864M_read_cmos_sensor(0xFB);
	value4 = RN6864M_read_cmos_sensor(0xFB);
	value5 = RN6864M_read_cmos_sensor(0xFB);
	value6 = RN6864M_read_cmos_sensor(0xFB);

	//config decoder according to value
	if ((value6 == 0x00) && (value5 == 0x00)) {
		RN6864M_write_cmos_sensor(0xEF, 0xAA);
		RN6864M_write_cmos_sensor(0xE7, 0xFF);
		RN6864M_write_cmos_sensor(0xFF, 0x09);
		RN6864M_write_cmos_sensor(0x03, 0x0C);
		RN6864M_write_cmos_sensor(0xFF, 0x0B);
		RN6864M_write_cmos_sensor(0x03, 0x0C);
	} else if (((value6 == 0x34) && (value5 == 0xA9)) || ((value6 == 0x2C) && (value5 == 0xA8)) ){
		RN6864M_write_cmos_sensor(0xEF, 0xAA);
		RN6864M_write_cmos_sensor(0xE7, 0xFF);
		RN6864M_write_cmos_sensor(0xFC, 0x60);
		RN6864M_write_cmos_sensor(0xFF, 0x09);
		RN6864M_write_cmos_sensor(0x03, 0x18);
		RN6864M_write_cmos_sensor(0xFF, 0x0B);
		RN6864M_write_cmos_sensor(0x03, 0x18);
	} else {
		RN6864M_write_cmos_sensor(0xEF, 0xAA);
		RN6864M_write_cmos_sensor(0xFC, 0x60);
		RN6864M_write_cmos_sensor(0xFF, 0x09);
		RN6864M_write_cmos_sensor(0x03, 0x18);
		RN6864M_write_cmos_sensor(0xFF, 0x0B);
		RN6864M_write_cmos_sensor(0x03, 0x18);
	}
}
void RN6864M_Sensor_Init(void)
{
	LOG_INF("sensor_init in");
	// 720P@25 with mipi 2 data lanes + 1 clock lane out
	// pin24/23 data lane0, pin18/17 data lane1
	// pin16/15 data lane2, pin12/11 data lane3
	// pin14/13 clock lane
	// Slave address is 0x58
	// Register, data
	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked fir
	//RN6864M_write_cmos_sensor(0xD2, 0x85)// disable auto clock detect
	//RN6864M_write_cmos_sensor(0xD6, 0x37) // 27MHz default
	//RN6864M_write_cmos_sensor(0xD8, 0x18) // switch to 26MHz clock
	//delay(100), // delay 100ms
	RN6864M_write_cmos_sensor(0x81, 0x0F); // turn on video decoder
	RN6864M_write_cmos_sensor(0xDF, 0xF0); // enable HD format
	
	RN6864M_write_cmos_sensor(0x88, 0x00); // turn on video decoder
	RN6864M_write_cmos_sensor(0xF6, 0x00); // enable HD format

	// ch0
	RN6864M_write_cmos_sensor(0xFF, 0x00); // switch to ch0 (default; optional)
	RN6864M_write_cmos_sensor(0x00, 0x20); // internal use*
	RN6864M_write_cmos_sensor(0x06, 0x08); // internal use*
	RN6864M_write_cmos_sensor(0x07, 0x63); // HD format
	RN6864M_write_cmos_sensor(0x2A, 0x01); // filter control
	RN6864M_write_cmos_sensor(0x3A, 0x20); // Insert Channel ID in SAV/EAV code
	RN6864M_write_cmos_sensor(0x3F, 0x10); // channel ID
	RN6864M_write_cmos_sensor(0x4C, 0x37); // equalizer
	RN6864M_write_cmos_sensor(0x4F, 0x03); // sync control
	RN6864M_write_cmos_sensor(0x50, 0x02); // 720p resolution
	RN6864M_write_cmos_sensor(0x56, 0x01); // BT 72M mode
	RN6864M_write_cmos_sensor(0x5F, 0x40); // blank level
	RN6864M_write_cmos_sensor(0x63, 0xF5); // filter control
	RN6864M_write_cmos_sensor(0x59, 0x00); // extended register access
	RN6864M_write_cmos_sensor(0x5A, 0x42); // data for extended register
	RN6864M_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6864M_write_cmos_sensor(0x59, 0x33); // extended register access
	RN6864M_write_cmos_sensor(0x5A, 0x23); // data for extended register
	RN6864M_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6864M_write_cmos_sensor(0x51, 0xE1); // scale factor1
	RN6864M_write_cmos_sensor(0x52, 0x88); // scale factor2
	RN6864M_write_cmos_sensor(0x53, 0x12); // scale factor3
	RN6864M_write_cmos_sensor(0x5B, 0x07); // H-scaling control
	RN6864M_write_cmos_sensor(0x5E, 0x0B); // enable H-scaling control
	RN6864M_write_cmos_sensor(0x6A, 0x82); // H-scaling control
	RN6864M_write_cmos_sensor(0x28, 0x92); // cropping
	RN6864M_write_cmos_sensor(0x03, 0x80); // saturation
	RN6864M_write_cmos_sensor(0x04, 0x80); // hue
	RN6864M_write_cmos_sensor(0x05, 0x04); // sharpness
	RN6864M_write_cmos_sensor(0x57, 0x23); // black/white stretch
	RN6864M_write_cmos_sensor(0x68, 0x32); // coring
	RN6864M_write_cmos_sensor(0x37, 0x33);
	RN6864M_write_cmos_sensor(0x61, 0x6C); 

	// ch1
	RN6864M_write_cmos_sensor(0xFF, 0x01); // switch to ch0 (default; optional)
	RN6864M_write_cmos_sensor(0x00, 0x20); // internal use*
	RN6864M_write_cmos_sensor(0x06, 0x08); // internal use*
	RN6864M_write_cmos_sensor(0x07, 0x63); // HD format
	RN6864M_write_cmos_sensor(0x2A, 0x01); // filter control
	RN6864M_write_cmos_sensor(0x3A, 0x20); // Insert Channel ID in SAV/EAV code
	RN6864M_write_cmos_sensor(0x3F, 0x11); // channel ID
	RN6864M_write_cmos_sensor(0x4C, 0x37); // equalizer
	RN6864M_write_cmos_sensor(0x4F, 0x03); // sync control
	RN6864M_write_cmos_sensor(0x50, 0x02); // 720p resolution
	RN6864M_write_cmos_sensor(0x56, 0x01); // BT 72M mode
	RN6864M_write_cmos_sensor(0x5F, 0x40); // blank level
	RN6864M_write_cmos_sensor(0x63, 0xF5); // filter control
	RN6864M_write_cmos_sensor(0x59, 0x00); // extended register access
	RN6864M_write_cmos_sensor(0x5A, 0x42); // data for extended register
	RN6864M_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6864M_write_cmos_sensor(0x59, 0x33); // extended register access
	RN6864M_write_cmos_sensor(0x5A, 0x23); // data for extended register
	RN6864M_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6864M_write_cmos_sensor(0x51, 0xE1); // scale factor1
	RN6864M_write_cmos_sensor(0x52, 0x88); // scale factor2
	RN6864M_write_cmos_sensor(0x53, 0x12); // scale factor3
	RN6864M_write_cmos_sensor(0x5B, 0x07); // H-scaling control
	RN6864M_write_cmos_sensor(0x5E, 0x0B); // enable H-scaling control
	RN6864M_write_cmos_sensor(0x6A, 0x82); // H-scaling control
	RN6864M_write_cmos_sensor(0x28, 0x92); // cropping
	RN6864M_write_cmos_sensor(0x03, 0x80); // saturation
	RN6864M_write_cmos_sensor(0x04, 0x80); // hue
	RN6864M_write_cmos_sensor(0x05, 0x04); // sharpness
	RN6864M_write_cmos_sensor(0x57, 0x23); // black/white stretch
	RN6864M_write_cmos_sensor(0x68, 0x32); // coring
	RN6864M_write_cmos_sensor(0x37, 0x33);
	RN6864M_write_cmos_sensor(0x61, 0x6C); 

	// ch2
	RN6864M_write_cmos_sensor(0xFF, 0x02); // switch to ch0 (default; optional)
	RN6864M_write_cmos_sensor(0x00, 0x20); // internal use*
	RN6864M_write_cmos_sensor(0x06, 0x08); // internal use*
	RN6864M_write_cmos_sensor(0x07, 0x63); // HD format
	RN6864M_write_cmos_sensor(0x2A, 0x01); // filter control
	RN6864M_write_cmos_sensor(0x3A, 0x20); // Insert Channel ID in SAV/EAV code
	RN6864M_write_cmos_sensor(0x3F, 0x12); // channel ID
	RN6864M_write_cmos_sensor(0x4C, 0x37); // equalizer
	RN6864M_write_cmos_sensor(0x4F, 0x03); // sync control
	RN6864M_write_cmos_sensor(0x50, 0x02); // 720p resolution
	RN6864M_write_cmos_sensor(0x56, 0x01); // BT 72M mode
	RN6864M_write_cmos_sensor(0x5F, 0x40); // blank level
	RN6864M_write_cmos_sensor(0x63, 0xF5); // filter control
	RN6864M_write_cmos_sensor(0x59, 0x00); // extended register access
	RN6864M_write_cmos_sensor(0x5A, 0x42); // data for extended register
	RN6864M_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6864M_write_cmos_sensor(0x59, 0x33); // extended register access
	RN6864M_write_cmos_sensor(0x5A, 0x23); // data for extended register
	RN6864M_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6864M_write_cmos_sensor(0x51, 0xE1); // scale factor1
	RN6864M_write_cmos_sensor(0x52, 0x88); // scale factor2
	RN6864M_write_cmos_sensor(0x53, 0x12); // scale factor3
	RN6864M_write_cmos_sensor(0x5B, 0x07); // H-scaling control
	RN6864M_write_cmos_sensor(0x5E, 0x0B); // enable H-scaling control
	RN6864M_write_cmos_sensor(0x6A, 0x82); // H-scaling control
	RN6864M_write_cmos_sensor(0x28, 0x92); // cropping
	RN6864M_write_cmos_sensor(0x03, 0x80); // saturation
	RN6864M_write_cmos_sensor(0x04, 0x80); // hue
	RN6864M_write_cmos_sensor(0x05, 0x04); // sharpness
	RN6864M_write_cmos_sensor(0x57, 0x23); // black/white stretch
	RN6864M_write_cmos_sensor(0x68, 0x32); // coring
	RN6864M_write_cmos_sensor(0x37, 0x33);
	RN6864M_write_cmos_sensor(0x61, 0x6C); 

	// ch3
	RN6864M_write_cmos_sensor(0xFF, 0x03); // switch to ch0 (default; optional)
	RN6864M_write_cmos_sensor(0x00, 0x20); // internal use*
	RN6864M_write_cmos_sensor(0x06, 0x08); // internal use*
	RN6864M_write_cmos_sensor(0x07, 0x63); // HD format
	RN6864M_write_cmos_sensor(0x2A, 0x01); // filter control
	RN6864M_write_cmos_sensor(0x3A, 0x20); // Insert Channel ID in SAV/EAV code
	RN6864M_write_cmos_sensor(0x3F, 0x13); // channel ID
	RN6864M_write_cmos_sensor(0x4C, 0x37); // equalizer
	RN6864M_write_cmos_sensor(0x4F, 0x03); // sync control
	RN6864M_write_cmos_sensor(0x50, 0x02); // 720p resolution
	RN6864M_write_cmos_sensor(0x56, 0x01); // BT 72M mode
	RN6864M_write_cmos_sensor(0x5F, 0x40); // blank level
	RN6864M_write_cmos_sensor(0x63, 0xF5); // filter control
	RN6864M_write_cmos_sensor(0x59, 0x00); // extended register access
	RN6864M_write_cmos_sensor(0x5A, 0x42); // data for extended register
	RN6864M_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6864M_write_cmos_sensor(0x59, 0x33); // extended register access
	RN6864M_write_cmos_sensor(0x5A, 0x23); // data for extended register
	RN6864M_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6864M_write_cmos_sensor(0x51, 0xE1); // scale factor1
	RN6864M_write_cmos_sensor(0x52, 0x88); // scale factor2
	RN6864M_write_cmos_sensor(0x53, 0x12); // scale factor3
	RN6864M_write_cmos_sensor(0x5B, 0x07); // H-scaling control
	RN6864M_write_cmos_sensor(0x5E, 0x0B); // enable H-scaling control
	RN6864M_write_cmos_sensor(0x6A, 0x82); // H-scaling control
	RN6864M_write_cmos_sensor(0x28, 0x92); // cropping
	RN6864M_write_cmos_sensor(0x03, 0x80); // saturation
	RN6864M_write_cmos_sensor(0x04, 0x80); // hue
	RN6864M_write_cmos_sensor(0x05, 0x04); // sharpness
	RN6864M_write_cmos_sensor(0x57, 0x23); // black/white stretch
	RN6864M_write_cmos_sensor(0x68, 0x32); // coring
	RN6864M_write_cmos_sensor(0x37, 0x33);
	RN6864M_write_cmos_sensor(0x61, 0x6C); 

	// mipi lik1
	RN6864M_write_cmos_sensor(0xFF, 0x09); // switch to mipi tx1
	RN6864M_write_cmos_sensor(0x00, 0x03); // enable bias
	RN6864M_write_cmos_sensor(0xFF, 0x08); // switch to mipi csi1
	RN6864M_write_cmos_sensor(0x04, 0x03); // csi1 and tx1 reset
	RN6864M_write_cmos_sensor(0x6C, 0x1F); // disable ch output; turn on ch0
	RN6864M_write_cmos_sensor(0x06, 0x7C); // 4 lanes
	RN6864M_write_cmos_sensor(0x21, 0x01); // enable hs clock
	RN6864M_write_cmos_sensor(0x78, 0x80); // Y/C counts for ch0
	RN6864M_write_cmos_sensor(0x79, 0x02); // Y/C counts for ch0
	RN6864M_write_cmos_sensor(0x7A, 0x80); // Y/C counts for ch1
	RN6864M_write_cmos_sensor(0x7B, 0x02); // Y/C counts for ch1
	RN6864M_write_cmos_sensor(0x7C, 0x80); // Y/C counts for ch2
	RN6864M_write_cmos_sensor(0x7D, 0x02); // Y/C counts for ch2
	RN6864M_write_cmos_sensor(0x7E, 0x80); // Y/C counts for ch3
	RN6864M_write_cmos_sensor(0x7F, 0x02); // Y/C counts for ch3
	RN6864M_write_cmos_sensor(0x6C, 0x0F); // enable ch output
	RN6864M_write_cmos_sensor(0x04, 0x00); // csi1 and tx1 reset finish
	RN6864M_write_cmos_sensor(0x07, 0x05); //enable non-continuous clock
	//RN6864M_write_cmos_sensor(0x20, 0xAA); // invert hs clock
	// mipi link3
	RN6864M_write_cmos_sensor(0xFF, 0x0A); // switch to mipi csi3
	RN6864M_write_cmos_sensor(0x6C, 0x10); // disable ch output; turn off ch0~3
	RN6864M_write_cmos_sensor(0xFF, 0x00); // atc0154 add
}

static UINT32 RN6864MForceUpdateFormat(void)
{
#if 0
	UINT8 reg_value_00 = 0;
	UINT8 Vdet=0;
	UINT8 F30=0;
	RN6864M_write_cmos_sensor(0x80,0x31);
	//mdely(500);
	RN6864M_write_cmos_sensor(0x80,0x30);

	reg_value_00 = RN6864M_read_cmos_sensor(0x00);

	if (reg_value_00 & (1<<4))
	{
		printk("NO video input is detected");
		return ERROR_NONE;
	}
	Vdet = (reg_value_00>>5);
	F30 = (reg_value_00&1);
	if ((Vdet == 0x00)&&(F30 == 0x00))
	{
		input_source_width = RN6864M_IMAGE_SENSOR_HD25_WIDTH;
		input_source_height = RN6864M_IMAGE_SENSOR_HD25_HEIGHT;
		source_format = FORMAT_PAL;
	}
	else if ((Vdet == 0x00)&&(F30 == 0x01))
	{
		input_source_width = RN6864M_IMAGE_SENSOR_HD25_WIDTH;
		input_source_height = RN6864M_IMAGE_SENSOR_HD25_HEIGHT;
		source_format = FORMAT_NTSC;
	}
	else if ((Vdet == 0x01)&&(F30 == 0x00))
	{
		input_source_width = RN6864M_IMAGE_SENSOR_HD25_WIDTH;
		input_source_height = RN6864M_IMAGE_SENSOR_HD25_HEIGHT;
		source_format = FORMAT_HD25;
	}
	else if ((Vdet == 0x01)&&(F30 == 0x01))
	{
		input_source_width = RN6864M_IMAGE_SENSOR_HD25_WIDTH;
		input_source_height = RN6864M_IMAGE_SENSOR_HD25_HEIGHT;
		source_format = FORMAT_HD30;
	}
	else if ((Vdet == 0x02)&&(F30 == 0x00))
	{
		input_source_width = RN6864M_IMAGE_SENSOR_HD25_WIDTH;
		input_source_height = RN6864M_IMAGE_SENSOR_HD25_HEIGHT;
		source_format = FORMAT_FHD25;
	}
	else if ((Vdet == 0x02)&&(F30 == 0x01))
	{
		input_source_width = RN6864M_IMAGE_SENSOR_HD25_WIDTH;
		input_source_height = RN6864M_IMAGE_SENSOR_HD25_HEIGHT;
		source_format = FORMAT_FHD30;
	}
#endif
	return ERROR_NONE;

}

static UINT32 RN6864M_GetSensorID(uint32_t *sensorId)
{
	int retry = 3;
	do {
		*sensorId = ((RN6864M_read_cmos_sensor(0xfe) << 8) | RN6864M_read_cmos_sensor(0xfd));
		//*sensorId = 0x0510;
		LOG_INF("WFH: sensor id = 0x%x", *sensorId);
		if (*sensorId == SENSOR_ID)
			break;
		retry--;
	} while (retry > 0);

	if (*sensorId != SENSOR_ID)
	{
		*sensorId = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	if (false == RN6864M_Inputsource_Format_Detected)
	{
		RN6864MForceUpdateFormat();
		RN6864M_Inputsource_Format_Detected = true;
	}
	return ERROR_NONE;
}

UINT32 RN6864MPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&RN6864MSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 RN6864MCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&RN6864MSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 RN6864MGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth = input_source_width;
	pSensorResolution->SensorFullHeight = input_source_height;
	pSensorResolution->SensorPreviewWidth = input_source_width;
	pSensorResolution->SensorPreviewHeight = input_source_height;
	pSensorResolution->SensorVideoWidth = input_source_width;
	pSensorResolution->SensorVideoHeight = input_source_height;

	return ERROR_NONE;
}

UINT32 RN6864MOpen(void)
{

	UINT32 sensor_id = 0;
	LOG_INF("RN6864MOpen Enter\n");

	RN6864M_GetSensorID(&sensor_id);
	if (sensor_id != SENSOR_ID)
	{
		LOG_ERR("RN6864 YUV read sensor id failed\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	RN6864M_Pre_Init();
	RN6864M_Sensor_Init();

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 RN6864MClose(void)
{
	LOG_INF("RN6864MClose");
	RN6864M_Inputsource_Format_Detected=false;
	return ERROR_NONE;
}

SIGNAL_STATUS RN6864M_Get_SignalStatus_Ch0(void)
{
	UINT32 value = 0xff;
	SIGNAL_STATUS signal_status = SIGNAL_IDLE;

	//Channel 0 signal status
	RN6864M_write_cmos_sensor(0xff, 0x00);
	value = RN6864M_read_cmos_sensor(0x00);
	if ((value & 0x10) == 0x00 )
	{
		signal_status = SIGNAL_READY;
	}
	else
	{
		signal_status = SIGNAL_LOST;
	}

	return signal_status;
}

SIGNAL_STATUS RN6864M_Get_SignalStatus_Ch1(void)
{
	UINT32 value = 0xff;
	SIGNAL_STATUS signal_status = SIGNAL_IDLE;

	//Channel 1 signal status
	RN6864M_write_cmos_sensor(0xff, 0x01);
	value = RN6864M_read_cmos_sensor(0x00);
	if ((value & 0x10) == 0x00 )
	{
		signal_status = SIGNAL_READY;
	}
	else
	{
		signal_status = SIGNAL_LOST;
	}

	return signal_status;
}

SIGNAL_STATUS RN6864M_Get_SignalStatus_Ch2(void)
{
	UINT32 value = 0xff;
	SIGNAL_STATUS signal_status = SIGNAL_IDLE;

	//Channel 2 siganl status
	RN6864M_write_cmos_sensor(0xff, 0x02);
	value = RN6864M_read_cmos_sensor(0x00);
	if ((value & 0x10) == 0x00 )
	{
		signal_status = SIGNAL_READY;
	}
	else
	{
		signal_status = SIGNAL_LOST;
	}

	return signal_status;
}

SIGNAL_STATUS RN6864M_Get_SignalStatus_Ch3(void)
{
	UINT32 value = 0xff;
	SIGNAL_STATUS signal_status = SIGNAL_IDLE;

	//Channel 3 signal status
	RN6864M_write_cmos_sensor(0xff, 0x03);
	value = RN6864M_read_cmos_sensor(0x00);
	if ((value & 0x10) == 0x00 )
	{
		signal_status = SIGNAL_READY;
	}
	else
	{
		signal_status = SIGNAL_LOST;
	}

	return signal_status;
}

UINT32 RN6864MGetInfo(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
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

		memcpy(pSensorConfigData, &RN6864MSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

		return ERROR_NONE;

}

UINT32 RN6864MControl(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	LOG_INF("Enter, ScenarioId = %d\n", ScenarioId);
	switch (ScenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			RN6864MPreview(pImageWindow, pSensorConfigData);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			RN6864MCapture(pImageWindow, pSensorConfigData);
			break;

		#ifdef ATC_KERN_SUPPORT_CVBS_FORMAT_CHANGE
		case MSDK_UPDATE_CVBS_FORMAT:
			RN6864MForceUpdateFormat();
			break;
		#endif

		default:
			break;
	}

	LOG_INF("Leave\n");

	return ERROR_NONE;
}

UINT32 RN6864MFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
			RN6864M_isp_master_clock = *pFeatureData32;
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			RN6864M_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = RN6864M_read_cmos_sensor(pSensorRegData->RegAddr);
			break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &RN6864MSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
			RN6864M_GetSensorID(pFeatureData32);
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
			*pFeatureData32 = RN6864M_Get_SignalStatus_Ch0();
			break;
		case SENSOR_FEATURE_GET_CH1_SIGNAL_STATUS:
			*pFeatureData32 = RN6864M_Get_SignalStatus_Ch1();
			break;
		case SENSOR_FEATURE_GET_CH2_SIGNAL_STATUS:
			*pFeatureData32 = RN6864M_Get_SignalStatus_Ch2();
			break;
		case SENSOR_FEATURE_GET_CH3_SIGNAL_STATUS:
			*pFeatureData32 = RN6864M_Get_SignalStatus_Ch3();
			break;

		default:
			break;
	}
	return ERROR_NONE;
}


static struct SENSOR_FUNCTION_STRUCT sensor_func =
{
	RN6864MOpen,
	RN6864MGetInfo,
	RN6864MGetResolution,
	RN6864MFeatureControl,
	RN6864MControl,
	RN6864MClose,
};

UINT32 RN6864M_YUV_SensorInit(
			struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL)
	{
		*pfFunc = &sensor_func;
	}
	return ERROR_NONE;
}






