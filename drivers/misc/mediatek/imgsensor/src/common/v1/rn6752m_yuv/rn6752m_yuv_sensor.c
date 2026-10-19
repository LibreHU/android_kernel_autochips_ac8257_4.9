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
*   rn6752m_yuv_Sensor.c
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

#include "rn6752m_yuv_sensor.h"

/****************************Modify Following Strings for Debug****************************/
#define PFX "[RN6752M]"


/****************************   Modify end    *******************************************/
#define LOG_DBG(format, args...)    pr_debug(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    pr_info(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...)    pr_err(PFX " %s " format, __FUNCTION__, ##args)

static kal_uint16 rn6752m_i2c_write_id = 0x58;
static int rn6752m_i2c_speed = 400;
//static VIDEO_FORMAT source_format = FORMAT_MAX;
static bool RN6752m_Inputsource_Format_Detected = false;
kal_uint32 RN6752m_isp_master_clock;


static kal_uint32 input_source_width = RN6752m_IMAGE_SENSOR_HD25_WIDTH;
static kal_uint32 input_source_height = RN6752m_IMAGE_SENSOR_HD25_HEIGHT;

MSDK_SENSOR_CONFIG_STRUCT RN6752mSensorConfigData;


static kal_uint16 RN6752m_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[1] = { (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 1, (u8 *) &get_byte, 1, rn6752m_i2c_write_id);

	return get_byte;
}

static int RN6752m_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[2] = { (char)(addr & 0xFF), (char)(para & 0xFF) };

	return iWriteRegI2CTiming(
		pu_send_cmd, 2, rn6752m_i2c_write_id, rn6752m_i2c_speed);
}

void RN6752M_Pre_Init(void)
{
	char value1, value2, value3, value4, value5, value6;

	LOG_INF("rn6752m pre init in\n");

	RN6752m_write_cmos_sensor(0xE1, 0x80);
	RN6752m_write_cmos_sensor(0xFA, 0x81);

	value1 = RN6752m_read_cmos_sensor(0xFB);
	value2 = RN6752m_read_cmos_sensor(0xFB);
	value3 = RN6752m_read_cmos_sensor(0xFB);
	value4 = RN6752m_read_cmos_sensor(0xFB);
	value5 = RN6752m_read_cmos_sensor(0xFB);
	value6 = RN6752m_read_cmos_sensor(0xFB);

	//config decoder according to value
	if ((value6 == 0x00) && (value5 == 0x00)) {
		RN6752m_write_cmos_sensor(0xEF, 0xAA);
		RN6752m_write_cmos_sensor(0xE7, 0xFF);
		RN6752m_write_cmos_sensor(0xFF, 0x09);
		RN6752m_write_cmos_sensor(0x03, 0x0C);
		RN6752m_write_cmos_sensor(0xFF, 0x0B);
		RN6752m_write_cmos_sensor(0x03, 0x0C);
	} else if (((value6 == 0x34) && (value5 == 0xA9)) || ((value6 == 0x2C) && (value5 == 0xA8)) ){
		RN6752m_write_cmos_sensor(0xEF, 0xAA);
		RN6752m_write_cmos_sensor(0xE7, 0xFF);
		RN6752m_write_cmos_sensor(0xFC, 0x60);
		RN6752m_write_cmos_sensor(0xFF, 0x09);
		RN6752m_write_cmos_sensor(0x03, 0x18);
		RN6752m_write_cmos_sensor(0xFF, 0x0B);
		RN6752m_write_cmos_sensor(0x03, 0x18);
	} else {
		RN6752m_write_cmos_sensor(0xEF, 0xAA);
		RN6752m_write_cmos_sensor(0xFC, 0x60);
		RN6752m_write_cmos_sensor(0xFF, 0x09);
		RN6752m_write_cmos_sensor(0x03, 0x18);
		RN6752m_write_cmos_sensor(0xFF, 0x0B);
		RN6752m_write_cmos_sensor(0x03, 0x18);
	}
}
#if 1 //for HD 30fps
void RN6752M_Sensor_Init(void)
{
	LOG_INF("sensor_init in");
	// 720P@25 with mipi 2 data lanes + 1 clock lane out
	// pin24/23 data lane0, pin18/17 data lane1
	// pin16/15 data lane2, pin12/11 data lane3
	// pin14/13 clock lane
	// Slave address is 0x58
	// Register, data
	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked fir
	//RN6752m_write_cmos_sensor(0xD2, 0x85)// disable auto clock detect
	//RN6752m_write_cmos_sensor(0xD6, 0x37) // 27MHz default
	//RN6752m_write_cmos_sensor(0xD8, 0x18) // switch to 26MHz clock
	//delay(100), // delay 100ms
	RN6752m_write_cmos_sensor(0x81, 0x01); // turn on video decoder
	RN6752m_write_cmos_sensor(0xDF, 0xFE); // enable HD format
	// ch0
	RN6752m_write_cmos_sensor(0xFF, 0x00); // switch to ch0 (default; optional)
	RN6752m_write_cmos_sensor(0x00, 0x20); // internal use*
	RN6752m_write_cmos_sensor(0x06, 0x08); // internal use*
	RN6752m_write_cmos_sensor(0x07, 0x63); // HD format
	RN6752m_write_cmos_sensor(0x1A, 0x03); // Black screen
	RN6752m_write_cmos_sensor(0x3B, 0x10); // Black screen
	RN6752m_write_cmos_sensor(0x3C, 0x80); // Black screen
	RN6752m_write_cmos_sensor(0x3D, 0x80); // Black screen

	RN6752m_write_cmos_sensor(0x2A, 0x01); // filter control
	RN6752m_write_cmos_sensor(0x3A, 0x28); // Insert Channel ID in SAV/EAV code
	RN6752m_write_cmos_sensor(0x3F, 0x10); // channel ID
	RN6752m_write_cmos_sensor(0x4C, 0x37); // equalizer
	RN6752m_write_cmos_sensor(0x4F, 0x03); // sync control
	RN6752m_write_cmos_sensor(0x50, 0x02); // 720p resolution
	RN6752m_write_cmos_sensor(0x56, 0x01); // BT 72M mode
	RN6752m_write_cmos_sensor(0x5F, 0x40); // blank level
	RN6752m_write_cmos_sensor(0x63, 0xF5); // filter control
	RN6752m_write_cmos_sensor(0x59, 0x00); // extended register access
	RN6752m_write_cmos_sensor(0x5A, 0x44); // data for extended register
	RN6752m_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6752m_write_cmos_sensor(0x59, 0x33); // extended register access
	RN6752m_write_cmos_sensor(0x5A, 0x23); // data for extended register
	RN6752m_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6752m_write_cmos_sensor(0x51, 0xE1); // scale factor1
	RN6752m_write_cmos_sensor(0x52, 0x88); // scale factor2
	RN6752m_write_cmos_sensor(0x53, 0x12); // scale factor3
	RN6752m_write_cmos_sensor(0x5B, 0x07); // H-scaling control
	RN6752m_write_cmos_sensor(0x5E, 0x0B); // enable H-scaling control
	RN6752m_write_cmos_sensor(0x6A, 0x82); // H-scaling control
	RN6752m_write_cmos_sensor(0x28, 0x92); // cropping
	RN6752m_write_cmos_sensor(0x03, 0x80); // saturation
	RN6752m_write_cmos_sensor(0x04, 0x80); // hue
	RN6752m_write_cmos_sensor(0x05, 0x04); // sharpness
	RN6752m_write_cmos_sensor(0x57, 0x23); // black/white stretch
	RN6752m_write_cmos_sensor(0x68, 0x32); // coring
	RN6752m_write_cmos_sensor(0x37, 0x33); // add for reduce color gradient time
	RN6752m_write_cmos_sensor(0x61, 0x6C); // add for reduce color gradient time
	RN6752m_write_cmos_sensor(0x0D, 0x20); // add for reduce color gradient time
	RN6752m_write_cmos_sensor(0x49, 0x81); // add for reduce color gradient time

	RN6752m_write_cmos_sensor(0x81, 0x01); // turn on video decoder
	// mipi lik1
	RN6752m_write_cmos_sensor(0xFF, 0x09); // switch to mipi tx1
	RN6752m_write_cmos_sensor(0x00, 0x03); // enable bias
	RN6752m_write_cmos_sensor(0xFF, 0x08); // switch to mipi csi1
	RN6752m_write_cmos_sensor(0x04, 0x03); // csi1 and tx1 reset
	RN6752m_write_cmos_sensor(0x6C, 0x11); // disable ch output; turn on ch0
	RN6752m_write_cmos_sensor(0x06, 0x4C); // 2 lanes
	RN6752m_write_cmos_sensor(0x21, 0x01); // enable hs clock
	RN6752m_write_cmos_sensor(0x78, 0x80); // Y/C counts for ch0
	RN6752m_write_cmos_sensor(0x79, 0x02); // Y/C counts for ch0
	RN6752m_write_cmos_sensor(0x6C, 0x01); // enable ch output
	RN6752m_write_cmos_sensor(0x04, 0x00); // csi1 and tx1 reset finish
	RN6752m_write_cmos_sensor(0x07, 0x05); //enable non-continuous clock
	//RN6752m_write_cmos_sensor(0x20, 0xAA); // invert hs clock
	// mipi link3
	RN6752m_write_cmos_sensor(0xFF, 0x0A); // switch to mipi csi3
	RN6752m_write_cmos_sensor(0x6C, 0x10); // disable ch output; turn off ch0~3
	RN6752m_write_cmos_sensor(0xFF, 0x00); // atc0154 add
}
#else //for FHD 30fps
void RN6752M_Sensor_Init(void)
{
	LOG_INF("sensor_init FHD30 in\n");
	// 1080P@30 with mipi 2 data lanes + 1 clock lane out
	// pin24/23 data lane0, pin18/17 data lane1
	// pin16/15 data lane2, pin12/11 data lane3
	// pin14/13 clock lane
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	RN6752m_write_cmos_sensor(0x81, 0x01); // turn on video decoder0
	RN6752m_write_cmos_sensor(0xDF, 0xFE); // enable ch0 as HD format
	RN6752m_write_cmos_sensor(0xF0, 0xC0); // 144MHz output
	// ch0
	RN6752m_write_cmos_sensor(0xFF, 0x00); // switch to ch0 (default; optional)
	RN6752m_write_cmos_sensor(0x00, 0x20); // internal use*
	RN6752m_write_cmos_sensor(0x06, 0x08); // internal use*
	RN6752m_write_cmos_sensor(0x07, 0x63); // HD format
	RN6752m_write_cmos_sensor(0x2A, 0x01); // filter control
	RN6752m_write_cmos_sensor(0x3A, 0x20); // Insert Channel ID in SAV/EAV code
	RN6752m_write_cmos_sensor(0x3F, 0x10); // channel ID
	RN6752m_write_cmos_sensor(0x4C, 0x37); // equalizer
	RN6752m_write_cmos_sensor(0x4F, 0x03); // sync control
	RN6752m_write_cmos_sensor(0x50, 0x03); // 1080p resolution
	RN6752m_write_cmos_sensor(0x56, 0x02); // BT 144M mode
	RN6752m_write_cmos_sensor(0x5F, 0x44); // blank level
	RN6752m_write_cmos_sensor(0x63, 0xF8); // filter control
	RN6752m_write_cmos_sensor(0x59, 0x00); // extended register access
	RN6752m_write_cmos_sensor(0x5A, 0x49); // data for extended register
	RN6752m_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6752m_write_cmos_sensor(0x59, 0x33); // extended register access
	RN6752m_write_cmos_sensor(0x5A, 0x23); // data for extended register
	RN6752m_write_cmos_sensor(0x58, 0x01); // enable extended register write
	RN6752m_write_cmos_sensor(0x51, 0xF4); // scale factor1
	RN6752m_write_cmos_sensor(0x52, 0x29); // scale factor2
	RN6752m_write_cmos_sensor(0x53, 0x15); // scale factor3
	RN6752m_write_cmos_sensor(0x5B, 0x01); // H-scaling control
	RN6752m_write_cmos_sensor(0x5E, 0x0F); // enable H-scaling control
	RN6752m_write_cmos_sensor(0x6A, 0x87); // H-scaling control
	RN6752m_write_cmos_sensor(0x28, 0x92); // cropping
	RN6752m_write_cmos_sensor(0x03, 0x80); // saturation
	RN6752m_write_cmos_sensor(0x04, 0x80); // hue
	RN6752m_write_cmos_sensor(0x05, 0x04); // sharpness
	RN6752m_write_cmos_sensor(0x57, 0x23); // black/white stretch
	RN6752m_write_cmos_sensor(0x68, 0x00); // coring
	RN6752m_write_cmos_sensor(0x81, 0x01); // turn on video decoder

	// mipi link1
	RN6752m_write_cmos_sensor(0xFF, 0x09); // switch to mipi tx1
	RN6752m_write_cmos_sensor(0x00, 0x03); // enable bias
	RN6752m_write_cmos_sensor(0xFF, 0x08); // switch to mipi csi1
	RN6752m_write_cmos_sensor(0x04, 0x03); // csi1 and tx1 reset
	RN6752m_write_cmos_sensor(0x6C, 0x11); // disable ch output; turn on ch0
	RN6752m_write_cmos_sensor(0x06, 0x4C); // 2 lanes
	RN6752m_write_cmos_sensor(0x21, 0x01); // enable hs clock
	RN6752m_write_cmos_sensor(0x78, 0xC0); // Y/C counts for ch0
	RN6752m_write_cmos_sensor(0x79, 0x03); // Y/C counts for ch0
	RN6752m_write_cmos_sensor(0x6C, 0x01); // enable ch output
	RN6752m_write_cmos_sensor(0x04, 0x00); // csi1 and tx1 reset finish
	RN6752m_write_cmos_sensor(0x07, 0x05);
	// 0x20, 0xAA, // invert clock phase
	// 0x07, 0x05, // enable non-clock

	// mipi
	RN6752m_write_cmos_sensor(0xFF, 0x0A); // switch to mipi csi3
	RN6752m_write_cmos_sensor(0x6C, 0x10); // disable ch output; turn off ch0~3
	RN6752m_write_cmos_sensor(0xFF, 0x00); // atc0154 add
}
#endif
static UINT32 RN6752mForceUpdateFormat(void)
{
#if 0
	kal_uint8 count_bit4 = 0, check_sig = 0, check_bit4 = 0;
	kal_uint8 rn6752m_reg_00, i;

	//RN6752M_write_cmos_sensor(0x49,0x81);
	RN6752m_write_cmos_sensor(0x19,0x07);
	for (i=0; i<100; i++ )
	{
		mdelay(1);
		check_bit4 = RN6752m_read_cmos_sensor(0x00)>>4;
		LOG_INF("check_bit4 = %x\n",(check_bit4&0x1));
		if (( check_bit4 & 0x1 ) != 0x0)
		{
			count_bit4 = 0;
			continue;
		} else {
			if(count_bit4++ > 10)
			{
			check_sig = 1;
				LOG_INF("RN6752DecoderLock signal ok,i = %d\r\n",i);
				break;
			}
		}
	}
	mdelay(500);
	if(check_sig)
	{
		rn6752m_reg_00 = RN6752m_read_cmos_sensor(0x00) & 0xf1;
		LOG_INF("rn6752m_reg_00 = 0x%x\r\n",rn6752m_reg_00);

		switch (rn6752m_reg_00)
		{
		case 0x00:
			LOG_INF("RN6752M_DecoderLock signal CVBS PAL\r\n");
			break;
		case 0x01:
			LOG_INF("RN6752M_DecoderLock signal CVBS NTSC\r\n");
			break;
		case 0x20:
			LOG_INF("RN6752M_DecoderLock signal 720P PAL\r\n");
			break;
		case 0x21:
			LOG_INF("RN6752M_DecoderLock signal 720P NTSC\r\n");
			break;
		case 0x40:
			LOG_INF("RN6752M_DecoderLock signal 1080P PAL\r\n");
			break;
		case 0x41:
			LOG_INF("RN6752M_DecoderLock signal 1080P NTSC\r\n");
			break;
		default:
			break;
		}

	}
#endif
	return ERROR_NONE;

}

static UINT32 RN6752m_GetSensorID(uint32_t *sensorId)
{
	int retry = 3;
	do {
		*sensorId = ((RN6752m_read_cmos_sensor(0xfe) << 8) | RN6752m_read_cmos_sensor(0xfd));
		LOG_INF("sensor id = 0x%x", *sensorId);
		if (*sensorId == SENSOR_ID)
			break;
		retry--;
	} while (retry > 0);

	if (*sensorId != SENSOR_ID)
	{
		*sensorId = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

		RN6752mForceUpdateFormat();

	return ERROR_NONE;
}

UINT32 RN6752mPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&RN6752mSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 RN6752mCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&RN6752mSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 RN6752mGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth = input_source_width;
	pSensorResolution->SensorFullHeight = input_source_height;
	pSensorResolution->SensorPreviewWidth = input_source_width;
	pSensorResolution->SensorPreviewHeight = input_source_height;
	pSensorResolution->SensorVideoWidth = input_source_width;
	pSensorResolution->SensorVideoHeight = input_source_height;

	return ERROR_NONE;
}

UINT32 RN6752mOpen(void)
{
	volatile signed char i;
	UINT32 sensor_id = 0;
	LOG_INF("RN6752mOpen Enter\n");

	for (i = 0; i < 3; i++)
	{
		RN6752m_GetSensorID(&sensor_id);
		if (sensor_id != SENSOR_ID)
		{
			LOG_ERR("RN6752M YUV read sensor id failed\n");
			return ERROR_SENSOR_CONNECT_FAIL;
		}
	}

	RN6752M_Pre_Init();
	RN6752M_Sensor_Init();

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 RN6752mClose(void)
{
	LOG_INF("RN6752mClose");
	RN6752m_Inputsource_Format_Detected=false;
	return ERROR_NONE;
}

SIGNAL_STATUS RN6752m_Get_SignalStatus(void)
{
	UINT32 value = 0xff;
	SIGNAL_STATUS signal_status = SIGNAL_IDLE;

	LOG_INF("get signal status enter\n");
	value = RN6752m_read_cmos_sensor(0x00);
	if ((value & 0x10) == 0x00 )
	{
		signal_status = SIGNAL_READY;
	}
	else
	{
		signal_status = SIGNAL_LOST;
	}
	//LOG_INF("get signal status leave and current signal status is %d\n", signal_status);
	return signal_status;
}

UINT32 RN6752mGetInfo(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
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
	pSensorInfo->PreviewDelayFrame = 5;
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

		memcpy(pSensorConfigData, &RN6752mSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

		return ERROR_NONE;

}

UINT32 RN6752mControl(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	LOG_INF("Enter, ScenarioId = %d\n", ScenarioId);
	switch (ScenarioId) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		RN6752mPreview(pImageWindow, pSensorConfigData);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		RN6752mCapture(pImageWindow, pSensorConfigData);
		break;

	#ifdef ATC_KERN_SUPPORT_CVBS_FORMAT_CHANGE
	case MSDK_UPDATE_CVBS_FORMAT:
		RN6752mForceUpdateFormat();
		break;
	#endif

	default:
		break;
	}

	LOG_INF("Leave\n");

	return ERROR_NONE;
}

UINT32 RN6752mFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
		RN6752m_isp_master_clock = *pFeatureData32;
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		RN6752m_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		pSensorRegData->RegData = RN6752m_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
	case SENSOR_FEATURE_GET_CONFIG_PARA:
		memcpy(pSensorConfigData, &RN6752mSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
		RN6752m_GetSensorID(pFeatureData32);
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
		break;
	default:
		break;
	}
	return ERROR_NONE;
}


static struct SENSOR_FUNCTION_STRUCT sensor_func =
{
	RN6752mOpen,
	RN6752mGetInfo,
	RN6752mGetResolution,
	RN6752mFeatureControl,
	RN6752mControl,
	RN6752mClose,
	RN6752m_Get_SignalStatus
};

UINT32 RN6752M_YUV_SensorInit(
			struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL)
	{
		*pfFunc = &sensor_func;
	}
	return ERROR_NONE;
}






