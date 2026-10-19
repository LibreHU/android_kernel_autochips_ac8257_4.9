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
*   atcdemo_yuv_Sensor.c
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

#include "atcdemo_yuv_sensor.h"

/****************************Modify Following Strings for Debug****************************/
#define PFX "[ATCDEMO]"

/****************************   Modify end    *******************************************/
#define LOG_DBG(format, args...)    pr_debug(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    pr_info(PFX " %s " format, __FUNCTION__, ##args)
#define LOG_ERR(format, args...)    pr_err(PFX " %s " format, __FUNCTION__, ##args)

static int atcdemo_i2c_speed = 400;
static kal_uint16 atcdemo_i2c_write_id = 0x88; //Must Fix
static VIDEO_FORMAT source_format = FORMAT_MAX;
static MSDK_SENSOR_CONFIG_STRUCT atcdemo_sensorconfigdata;

static kal_uint32 input_source_fps = ATCDEMO_FPS_30;
static kal_uint32 input_source_width = ATCDEMO_IMAGE_SENSOR_HD30_WIDTH;
static kal_uint32 input_source_height = ATCDEMO_IMAGE_SENSOR_HD30_HEIGHT;


static kal_uint16 atcdemo_i2c_read(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[1] = { (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 1, (u8 *) &get_byte, 1, atcdemo_i2c_write_id);

	return get_byte;
}

static int atcdemo_i2c_write(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[2] = { (char)(addr & 0xFF), (char)(para & 0xFF) };

	return iWriteRegI2CTiming(
		pu_send_cmd, 2, atcdemo_i2c_write_id, atcdemo_i2c_speed);
}


static void atcdemo_sensor_init(VIDEO_FORMAT source_format)
{
	LOG_INF("ATCDEMO sensor init enter source_format = %d \n",source_format);
	if (FORMAT_HD_25== source_format) {
		atcdemo_i2c_write(0x40, 0x00);
	} else if (FORMAT_HD_30 == source_format) {
		atcdemo_i2c_write(0x40, 0x00);
	}else if (FORMAT_FHD_25 == source_format) {
		atcdemo_i2c_write(0x40, 0x00);
	}else if (FORMAT_FHD_30 == source_format) {
		atcdemo_i2c_write(0x40, 0x00);
	}

}


static UINT32 atcdemo_getsensorid(uint32_t *sensorId)
{
	int retry = 3;

	do {
		*sensorId = ((atcdemo_i2c_read(0xfe) << 8) | atcdemo_i2c_read(0xff));
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

UINT32 atcdemo_preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&atcdemo_sensorconfigdata, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	return ERROR_NONE;
}

UINT32 atcdemo_capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = input_source_width;
	image_window->ExposureWindowHeight = input_source_height;

	memcpy(&atcdemo_sensorconfigdata, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	return ERROR_NONE;
}

UINT32 atcdemo_resoluton_get(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

	pSensorResolution->SensorFullWidth = input_source_width;
	pSensorResolution->SensorFullHeight = input_source_height;
	pSensorResolution->SensorPreviewWidth = input_source_width;
	pSensorResolution->SensorPreviewHeight = input_source_height;
	pSensorResolution->SensorVideoWidth = input_source_width;
	pSensorResolution->SensorVideoHeight = input_source_height;
	pSensorResolution->SensorFrameRate = input_source_fps;
	pSensorResolution->SensorFormatInfo = SENSOR_FORMAT_YUV422;

	return ERROR_NONE;
}

SIGNAL_STATUS atcdemo_signalstatus_get(void)
{
	SIGNAL_STATUS signal_status = SIGNAL_READY;

	return signal_status;
}

static void atcdemo_parm_set(VIDEO_FORMAT source_format)
{

	switch (source_format){
	case FORMAT_HD_25:
		input_source_height = ATCDEMO_IMAGE_SENSOR_HD25_HEIGHT;
		input_source_width = ATCDEMO_IMAGE_SENSOR_HD25_WIDTH;
		input_source_fps = ATCDEMO_FPS_25;
		break;
	case FORMAT_HD_30:
		input_source_height = ATCDEMO_IMAGE_SENSOR_HD30_HEIGHT;
		input_source_width = ATCDEMO_IMAGE_SENSOR_HD30_WIDTH;
		input_source_fps = ATCDEMO_FPS_30;
		break;
	case FORMAT_FHD_25:
		input_source_height = ATCDEMO_IMAGE_SENSOR_FHD25_HEIGHT;
		input_source_width = ATCDEMO_IMAGE_SENSOR_FHD25_WIDTH;
		input_source_fps = ATCDEMO_FPS_25;
		break;
	case FORMAT_FHD_30:
		input_source_height = ATCDEMO_IMAGE_SENSOR_FHD30_HEIGHT;
		input_source_width = ATCDEMO_IMAGE_SENSOR_FHD30_WIDTH;
		input_source_fps = ATCDEMO_FPS_30;
		break;
	default:
		LOG_ERR("this may be impossible\n");
		break;
	}
}

static VIDEO_FORMAT atcdemo_format_detect(void)
{
	VIDEO_FORMAT cur_format = FORMAT_HD_30 ;

	return cur_format;
}



UINT32 atcdemo_open(void)
{
	UINT32 sensor_id = 0;
	UINT32 ret = 0;

	ret = atcdemo_getsensorid(&sensor_id);
	if (ERROR_NONE != ret) {
		LOG_ERR("read sensor id failed\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	source_format = atcdemo_format_detect();
	atcdemo_parm_set(source_format);
	atcdemo_sensor_init(source_format);

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 atcdemo_close(void)
{

	input_source_width = ATCDEMO_IMAGE_SENSOR_HD30_WIDTH;
	input_source_height = ATCDEMO_IMAGE_SENSOR_HD30_HEIGHT;
	input_source_fps = ATCDEMO_FPS_30;
	return ERROR_NONE;
}


UINT32 atcdemo_getinfo(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
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
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
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

	memcpy(pSensorConfigData, &atcdemo_sensorconfigdata, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	return ERROR_NONE;

}

UINT32 atcdemo_control(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		atcdemo_preview(pImageWindow, pSensorConfigData);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		atcdemo_capture(pImageWindow, pSensorConfigData);
		break;

	default:
		break;
	}

	return ERROR_NONE;
}

UINT32 atcdemo_feature_control(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		atcdemo_i2c_write(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		pSensorRegData->RegData = atcdemo_i2c_read(pSensorRegData->RegAddr);
		break;
	case SENSOR_FEATURE_GET_CONFIG_PARA:
		memcpy(pSensorConfigData, &atcdemo_sensorconfigdata, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
		atcdemo_getsensorid(pFeatureData32);
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
		*pFeatureData32 = atcdemo_signalstatus_get();
		break;
	default:
		break;
	}
	return ERROR_NONE;
}


static struct SENSOR_FUNCTION_STRUCT sensor_func =
{
	atcdemo_open,
	atcdemo_getinfo,
	atcdemo_resoluton_get,
	atcdemo_feature_control,
	atcdemo_control,
	atcdemo_close,
	atcdemo_signalstatus_get
};

UINT32 ATCDEMO_YUV_SensorInit(
			struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL) {
		*pfFunc = &sensor_func;
	}
	return ERROR_NONE;
}






