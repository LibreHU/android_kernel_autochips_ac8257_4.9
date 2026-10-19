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
*   tp2854_yuv_Sensor.c
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

#include "tp2854_yuv_sensor.h"

static kal_uint16 tp2854_i2c_write_id = 0x88;
static int tp2854_i2c_speed = 400;
//static VIDEO_FORMAT source_format = FORMAT_MAX;

kal_uint32 TP2854_isp_master_clock;


static kal_uint32 input_source_width = TP2854_IMAGE_SENSOR_HD25_WIDTH;
static kal_uint32 input_source_height = TP2854_IMAGE_SENSOR_HD25_HEIGHT;

MSDK_SENSOR_CONFIG_STRUCT TP2854SensorConfigData;

/****************************Modify Following Strings for Debug****************************/
#define PFX "[TP2854]"


/****************************   Modify end    *******************************************/
#define LOG_DBG(format, args...)    pr_debug(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    pr_info(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...)    pr_err(PFX " %s " format, __FUNCTION__, ##args)

static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] = {
	/* Preview mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x1E, 0x02d0, 0x0120,/*VC0*/
	 0x01, 0x1E, 0x02d0, 0x0120,/*VC1*/
	 0x02, 0x1E, 0x02d0, 0x0120,/*VC2 LPD+RPD*/
	 0x03, 0x1E, 0x02d0, 0x0120},/*VC3*/
	/* Capture mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x1E, 0x02d0, 0x0120,/*VC0*/
	 0x01, 0x1E, 0x02d0, 0x0120,/*VC1*/
	 0x02, 0x1E, 0x02d0, 0x0120,/*VC2 LPD+RPD*/
	 0x03, 0x1E, 0x02d0, 0x0120},/*VC3*/
	/* Video mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x1E, 0x02d0, 0x0120,
	 0x01, 0x1E, 0x02d0, 0x0120,
	 0x02, 0x1E, 0x02d0, 0x0120,
	 0x03, 0x1E, 0x02d0, 0x0120}
};

static kal_uint16 TP2854_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[1] = { (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 1, (u8 *) &get_byte, 1, tp2854_i2c_write_id);
	return get_byte;
}

static int TP2854_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[2] = { (char)(addr & 0xFF), (char)(para & 0xFF) };

	return iWriteRegI2CTiming(
		pu_send_cmd, 2, tp2854_i2c_write_id, tp2854_i2c_speed);
}

void TP2854_Sensor_Init(void)
{
	//video setting
	LOG_INF("TP2854_Sensor_Init enter!\n");
	TP2854_write_cmos_sensor(0x40, 0x04); //video page, write all

#if 0
	//NTSC
	TP2854_write_cmos_sensor(0x02, 0xc7);
	TP2854_write_cmos_sensor(0x0c, 0x13);
	TP2854_write_cmos_sensor(0x0d, 0x50);

	TP2854_write_cmos_sensor(0x15, 0x13);
	TP2854_write_cmos_sensor(0x16, 0x60);
	TP2854_write_cmos_sensor(0x17, 0x80);
	TP2854_write_cmos_sensor(0x18, 0x12);
	TP2854_write_cmos_sensor(0x19, 0xf0);
	TP2854_write_cmos_sensor(0x1a, 0x07);
	TP2854_write_cmos_sensor(0x1c, 0x09);
	TP2854_write_cmos_sensor(0x1d, 0x38);

	TP2854_write_cmos_sensor(0x20, 0x40);
	TP2854_write_cmos_sensor(0x21, 0x84);
	TP2854_write_cmos_sensor(0x22, 0x36);
	TP2854_write_cmos_sensor(0x23, 0x3c);

	TP2854_write_cmos_sensor(0x2b, 0x70);
	TP2854_write_cmos_sensor(0x2c, 0x2a);
	TP2854_write_cmos_sensor(0x2d, 0x68);
	TP2854_write_cmos_sensor(0x2e, 0x57);

	TP2854_write_cmos_sensor(0x30, 0x62);
	TP2854_write_cmos_sensor(0x31, 0xbb);
	TP2854_write_cmos_sensor(0x32, 0x96);
	TP2854_write_cmos_sensor(0x33, 0xc0);

#endif

#if 0
	//PAL
	TP2854_write_cmos_sensor(0x02, 0xc7);
	TP2854_write_cmos_sensor(0x0c, 0x13);
	TP2854_write_cmos_sensor(0x0d, 0x51);

	TP2854_write_cmos_sensor(0x15, 0x13);
	TP2854_write_cmos_sensor(0x16, 0x76);
	TP2854_write_cmos_sensor(0x17, 0x80);
	TP2854_write_cmos_sensor(0x18, 0x17);
	TP2854_write_cmos_sensor(0x19, 0x20);
	TP2854_write_cmos_sensor(0x1a, 0x17);
	TP2854_write_cmos_sensor(0x1c, 0x09);
	TP2854_write_cmos_sensor(0x1d, 0x48);

	TP2854_write_cmos_sensor(0x20, 0x48);
	TP2854_write_cmos_sensor(0x21, 0x84);
	TP2854_write_cmos_sensor(0x22, 0x37);
	TP2854_write_cmos_sensor(0x23, 0x3f);

	TP2854_write_cmos_sensor(0x2b, 0x70);
	TP2854_write_cmos_sensor(0x2c, 0x2a);
	TP2854_write_cmos_sensor(0x2d, 0x64);
	TP2854_write_cmos_sensor(0x2e, 0x56);

	TP2854_write_cmos_sensor(0x30, 0x7a);
	TP2854_write_cmos_sensor(0x31, 0x4a);
	TP2854_write_cmos_sensor(0x32, 0x4d);
	TP2854_write_cmos_sensor(0x33, 0xf0);

#endif
#if 1
	//AHD 720P@30fps
	TP2854_write_cmos_sensor(0x02, 0xc2);
	TP2854_write_cmos_sensor(0x07, 0xc0);
	TP2854_write_cmos_sensor(0x0b, 0xc0);
	TP2854_write_cmos_sensor(0x0c, 0x13);
	TP2854_write_cmos_sensor(0x0d, 0x50);

	TP2854_write_cmos_sensor(0x15, 0x13);
	TP2854_write_cmos_sensor(0x16, 0x15);
	TP2854_write_cmos_sensor(0x17, 0x00);
	TP2854_write_cmos_sensor(0x18, 0x19);
	TP2854_write_cmos_sensor(0x19, 0xd0);
	TP2854_write_cmos_sensor(0x1a, 0x25);
	//TP2854_write_cmos_sensor(0x1c, 0x07);//0x07 for 720p@25fps
	//TP2854_write_cmos_sensor(0x1d, 0xbc);//0xbc for 720p@25fps
	TP2854_write_cmos_sensor(0x1c, 0x06);//0x06 for 720p@30fps
	TP2854_write_cmos_sensor(0x1d, 0x72);//0x72 for 720p@30fps

	TP2854_write_cmos_sensor(0x20, 0x30);
	TP2854_write_cmos_sensor(0x21, 0x84);
	TP2854_write_cmos_sensor(0x22, 0x36);
	TP2854_write_cmos_sensor(0x23, 0x3c);

	TP2854_write_cmos_sensor(0x2b, 0x60);
	TP2854_write_cmos_sensor(0x2c, 0x0a);
	TP2854_write_cmos_sensor(0x2d, 0x30);
	TP2854_write_cmos_sensor(0x2e, 0x70);

	TP2854_write_cmos_sensor(0x30, 0x48);
	TP2854_write_cmos_sensor(0x31, 0xbb);
	TP2854_write_cmos_sensor(0x32, 0x2e);
	TP2854_write_cmos_sensor(0x33, 0x90);

	TP2854_write_cmos_sensor(0x35, 0x25);
	TP2854_write_cmos_sensor(0x38, 0x00);
	TP2854_write_cmos_sensor(0x39, 0x18);

	TP2854_write_cmos_sensor(0x4e, 0x00);
	TP2854_write_cmos_sensor(0xf5, 0xf0);

	TP2854_write_cmos_sensor(0x02, 0xc6);

	//TP2854_write_cmos_sensor(0x0d, 0x71);//25fps
	TP2854_write_cmos_sensor(0x0d, 0x70);

	TP2854_write_cmos_sensor(0x20, 0x40);
	TP2854_write_cmos_sensor(0x21, 0x46);

	TP2854_write_cmos_sensor(0x25, 0xfe);
	TP2854_write_cmos_sensor(0x26, 0x01);

	TP2854_write_cmos_sensor(0x2c, 0x3a);
	TP2854_write_cmos_sensor(0x2d, 0x5a);
	TP2854_write_cmos_sensor(0x2e, 0x40);

	//TP2854_write_cmos_sensor(0x30, 0x9e);//25fps
	//TP2854_write_cmos_sensor(0x31, 0x20);//25fps
	//TP2854_write_cmos_sensor(0x32, 0x10);//25fps
	//TP2854_write_cmos_sensor(0x33, 0x90);//25fps
	TP2854_write_cmos_sensor(0x30, 0x9d);
	TP2854_write_cmos_sensor(0x31, 0xc8);//ca
	TP2854_write_cmos_sensor(0x32, 0x01);
	TP2854_write_cmos_sensor(0x33, 0xd0);
#endif
	//mipi setting
	TP2854_write_cmos_sensor(0x40, 0x08);//MIPI page
	TP2854_write_cmos_sensor(0x01, 0xf8);
	TP2854_write_cmos_sensor(0x02, 0x01);
	TP2854_write_cmos_sensor(0x08, 0x0f);
	TP2854_write_cmos_sensor(0x13, 0x24);
	TP2854_write_cmos_sensor(0x14, 0x04);
	TP2854_write_cmos_sensor(0x15, 0x00);
	TP2854_write_cmos_sensor(0x20, 0x44);
	TP2854_write_cmos_sensor(0x34, 0x1b);
	TP2854_write_cmos_sensor(0x25, 0x0a);
	TP2854_write_cmos_sensor(0x26, 0x06);
	TP2854_write_cmos_sensor(0x27, 0x03);

	/* disable MIPI CSI2 output */
	TP2854_write_cmos_sensor(0x23, 0x02);

	/* enable MIPI CSI2 output */
	TP2854_write_cmos_sensor(0x23, 0x00);

	//set decoder page
	TP2854_write_cmos_sensor(0x40, 0x04);
}

static UINT32 TP2854ForceUpdateFormat(void)
{
#if 0
	UINT8 reg_value_00 = 0;
	UINT8 Vdet=0;
	UINT8 F30=0;

	mdelay(500);
	TP2854_write_cmos_sensor(0x40,0x00);
	reg_value_00=TP2854_read_cmos_sensor(0x03);
	LOG_INF("tp2854 TP2854ForceUpdateFormat 00 0x03= 0x%x\n",reg_value_00);
	LOG_INF("tp2854 TP2854ForceUpdateFormat 00 0x01= 0x%x\n",TP2854_read_cmos_sensor(0x01));
	mdelay(500);
	TP2854_write_cmos_sensor(0x40,0x01);
	reg_value_00=TP2854_read_cmos_sensor(0x03);
	LOG_INF("tp2854 TP2854ForceUpdateFormat 01 0x03= 0x%x\n",reg_value_00);
	LOG_INF("tp2854 TP2854ForceUpdateFormat 01 0x01= 0x%x\n",TP2854_read_cmos_sensor(0x01));
	mdelay(500);
	TP2854_write_cmos_sensor(0x40,0x02);
	reg_value_00=TP2854_read_cmos_sensor(0x03);
	LOG_INF("tp2854 TP2854ForceUpdateFormat 02 0x03= 0x%x\n",reg_value_00);
	LOG_INF("tp2854 TP2854ForceUpdateFormat 02 0x01= 0x%x\n",TP2854_read_cmos_sensor(0x01));
	mdelay(500);
	TP2854_write_cmos_sensor(0x40,0x03);
	reg_value_00=TP2854_read_cmos_sensor(0x03);
	LOG_INF("tp2854 TP2854ForceUpdateFormat 03 0x03= 0x%x\n",reg_value_00);
	LOG_INF("tp2854 TP2854ForceUpdateFormat 03 0x01= 0x%x\n",TP2854_read_cmos_sensor(0x01));
	if (reg_value_00 & (1<<4)){
		LOG_INF("NO video input is detected");
		return ERROR_NONE;
	}
	Vdet = (reg_value_00>>5);
	F30 = (reg_value_00&1);
	if ((Vdet == 0x00)&&(F30 == 0x00)){
		input_source_width = TP2854_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height = TP2854_IMAGE_SENSOR_PAL_HEIGHT;
		source_format = FORMAT_PAL;
	}
	else if ((Vdet == 0x00)&&(F30 == 0x01)){
		input_source_width = TP2854_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height = TP2854_IMAGE_SENSOR_PAL_HEIGHT;
		source_format = FORMAT_NTSC;
	}
	else if ((Vdet == 0x01)&&(F30 == 0x00)){
		input_source_width = TP2854_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height = TP2854_IMAGE_SENSOR_PAL_HEIGHT;
		source_format = FORMAT_HD25;
	}
	else if ((Vdet == 0x01)&&(F30 == 0x01)){
		input_source_width = TP2854_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height = TP2854_IMAGE_SENSOR_PAL_HEIGHT;
		source_format = FORMAT_HD30;
	}
	else if ((Vdet == 0x02)&&(F30 == 0x00)){
		input_source_width = TP2854_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height = TP2854_IMAGE_SENSOR_PAL_HEIGHT;
		source_format = FORMAT_FHD25;
	}
	else if ((Vdet == 0x02)&&(F30 == 0x01)){
		input_source_width = TP2854_IMAGE_SENSOR_PAL_WIDTH;
		input_source_height = TP2854_IMAGE_SENSOR_PAL_HEIGHT;
		source_format = FORMAT_FHD30;
	}
#endif
	return ERROR_NONE;

}

static UINT32 TP2854_GetSensorID(uint32_t *sensorId)
{
	int retry = 3;
	do {
		*sensorId = ((TP2854_read_cmos_sensor(0xfe) << 8) | TP2854_read_cmos_sensor(0xff));
		LOG_INF("sensor id = 0x%x", *sensorId);
		if (*sensorId == SENSOR_ID)
			break;
		retry--;
	} while (retry > 0);

	if (*sensorId != SENSOR_ID){
		*sensorId = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	return ERROR_NONE;
}

UINT32 TP2854Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&TP2854SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 TP2854Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&TP2854SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	LOG_INF("Leave\n");
	return ERROR_NONE;
}


UINT32 TP2854GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth = input_source_width;
	pSensorResolution->SensorFullHeight = input_source_height;
	pSensorResolution->SensorPreviewWidth = input_source_width;
	pSensorResolution->SensorPreviewHeight = input_source_height;
	pSensorResolution->SensorVideoWidth = input_source_width;
	pSensorResolution->SensorVideoHeight = input_source_height;
	pSensorResolution->SensorCh0Width = input_source_width;
	pSensorResolution->SensorCh0Height = input_source_height;
	pSensorResolution->SensorCh1Width = input_source_width;
	pSensorResolution->SensorCh1Height = input_source_height;
	pSensorResolution->SensorCh2Width = input_source_width;
	pSensorResolution->SensorCh2Height = input_source_height;
	pSensorResolution->SensorCh3Width = input_source_width;
	pSensorResolution->SensorCh3Height = input_source_height;

	return ERROR_NONE;
}

UINT32 TP2854Open(void)
{
	volatile signed char i;
	UINT32 sensor_id = 0;
	LOG_INF("TP2854Open Enter\n");

	for (i = 0; i < 3; i++){
		TP2854_GetSensorID(&sensor_id);
		if (sensor_id != SENSOR_ID){
			LOG_INF("TP2854 YUV read sensor id failed\n");
			return ERROR_SENSOR_CONNECT_FAIL;
		}
	}

	TP2854_Sensor_Init();
	TP2854ForceUpdateFormat();
	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 TP2854Close(void)
{
	LOG_INF("TP2854Close");
	return ERROR_NONE;
}

SIGNAL_STATUS TP2854_Get_SignalStatus(void)
{
	UINT32 value = 0xff;
	SIGNAL_STATUS signal_status = SIGNAL_IDLE;

	LOG_INF("get signal status enter\n");
	value = TP2854_read_cmos_sensor(0x00);
	if ((value & 0x10) == 0x00 ){
		signal_status = SIGNAL_READY;
	}else{
		signal_status = SIGNAL_LOST;
	}
	LOG_INF("get signal status leave and current signal status is %d\n", signal_status);
	return signal_status;
}

UINT32 TP2854GetInfo(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX = input_source_width;
	pSensorInfo->SensorPreviewResolutionY = input_source_height;
	pSensorInfo->SensorFullResolutionX = input_source_width;
	pSensorInfo->SensorFullResolutionY = input_source_height;
	//LOG_INF("tp2854 TP2854GetInfo\n");
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
	pSensorInfo->PreviewDelayFrame = 13;
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

	memcpy(pSensorConfigData, &TP2854SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	return ERROR_NONE;

}

UINT32 TP2854Control(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	LOG_INF("Enter, ScenarioId = %d\n", ScenarioId);
	switch (ScenarioId) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		TP2854Preview(pImageWindow, pSensorConfigData);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		TP2854Capture(pImageWindow, pSensorConfigData);
		break;


	default:
		break;
	}

	LOG_INF("Leave\n");

	return ERROR_NONE;
}

UINT32 TP2854FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
		UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16 = (UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32 = (UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32 = (UINT32 *) pFeaturePara;
	UINT32 *feature_data_32 = (UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData = (MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData = (MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;
	unsigned long long *feature_data = (unsigned long long *)pFeaturePara;
	//LOG_INF("tp2854 TP2854FeatureControl FeatureId = %d\n",FeatureId);
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
		TP2854_isp_master_clock = *pFeatureData32;
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		TP2854_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		pSensorRegData->RegData = TP2854_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
	case SENSOR_FEATURE_GET_CONFIG_PARA:
		memcpy(pSensorConfigData, &TP2854SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
		TP2854_GetSensorID(pFeatureData32);
		break;
	case SENSOR_FEATURE_GET_VC_INFO:
	LOG_INF("TP2854 SENSOR_FEATURE_GET_VC_INFO %d\n",
		(UINT16) *feature_data);

		pvcinfo =
		(struct SENSOR_VC_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
			sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[2],
			sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
				memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
			}
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
	TP2854Open,
	TP2854GetInfo,
	TP2854GetResolution,
	TP2854FeatureControl,
	TP2854Control,
	TP2854Close,
	TP2854_Get_SignalStatus
};

UINT32 TP2854_YUV_SensorInit(
			struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL){
		*pfFunc = &sensor_func;
	}
	return ERROR_NONE;
}







