/*
 * Copyright (c) 2019 AutoChips Inc.
 * Copyright (C) 2026 LibreHU
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
 *   tp9963_yuv_sensor.c
 *
 * Project:
 * --------
 *   Android P on AC8257
 *
 * Description:
 * ------------
 *   Techpoint TP9963 decoder, MAIN2 camera of the Jancar UJC201 (AHD rear camera, "camera 8"
 *   of the stock camera HAL). Rebuilt from the stock kernel 4.9.117+ #25: same I2C address,
 *   speed, register sequences, metazone keys, sensor ID and answers to the camera HAL.
 *   Only the log texts may differ.
 *   Cross-checked against the GPL TP9963 drivers of OpenIPC (openingenic, Ingenic T41) and of the
 *   Allwinner Tina V821 BSP: same chip ID 0x2863 (regs 0xfe / 0xff), 1080p sequences of Techpoint's
 *   reference (without its 0x18 = 0x2a), MIPI output = Techpoint's MIPI_2CH2LANE_594M.
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

#include "tp9963_yuv_sensor.h"

#define PFX "[TP9963]"

#define LOG_INF(format, args...)    pr_info(PFX " %s " format, __func__, ##args)
#define LOG_ERR(format, args...)    pr_err(PFX " %s " format, __func__, ##args)

/* Format forced by ATC Settings: 3..6 = FORMAT_HD_25..FORMAT_FHD_30, else detection. */
#define METAZONE_AHD_FORMAT_DEFAULT             0x10045
/* Set to 1 once a TP9963 has answered. */
#define METAZONE_AHD_SENSOR_ID                  0x1019d
/* Jancar's ADDR_AHD_SENSOR_VALUE (camera format of its settings): 0 720p25, 1 1080p25, 2 720p30, 3 1080p30. */
#define METAZONE_AHD_SENSOR_VALUE               0x101e2

static kal_uint16 tp9963_i2c_write_id = 0x8a;
static int tp9963_i2c_speed = 200;
static VIDEO_FORMAT source_format = FORMAT_MAX;
static kal_uint32 TP9963_isp_master_clock;

static kal_uint32 input_source_width = TP9963_IMAGE_SENSOR_HD_WIDTH;
static kal_uint32 input_source_height = TP9963_IMAGE_SENSOR_HD_HEIGHT;

static MSDK_SENSOR_CONFIG_STRUCT TP9963SensorConfigData;

struct tp9963_reg {
	u8 reg;
	u8 val;
};

static kal_uint16 TP9963_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[1] = { (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 1, (u8 *) &get_byte, 1, tp9963_i2c_write_id);

	return get_byte;
}

static int TP9963_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[2] = { (char)(addr & 0xFF), (char)(para & 0xFF) };

	return iWriteRegI2CTiming(
		pu_send_cmd, 2, tp9963_i2c_write_id, tp9963_i2c_speed);
}

static void TP9963_write_table(const struct tp9963_reg *tbl, int n)
{
	int i;

	for (i = 0; i < n; i++)
		TP9963_write_cmos_sensor(tbl[i].reg, tbl[i].val);
}

/* Decoder settings per input format (stock order, rewrites included); 1080p = Techpoint's STD_HDA (AHD). */
static const struct tp9963_reg tp9963_ntsc[] = {
	{0x02, 0x47}, {0x07, 0x80}, {0x0b, 0x80}, {0x0c, 0x13},
	{0x0d, 0x50}, {0x15, 0x03}, {0x16, 0xd6}, {0x17, 0xa0},
	{0x18, 0x12}, {0x19, 0xf0}, {0x1a, 0x05}, {0x1c, 0x06},
	{0x1d, 0xb4}, {0x20, 0x40}, {0x21, 0x84}, {0x22, 0x36},
	{0x23, 0x3c}, {0x2b, 0x70}, {0x2c, 0x2a}, {0x2d, 0x4b},
	{0x2e, 0x57}, {0x30, 0x62}, {0x31, 0xbb}, {0x32, 0x96},
	{0x33, 0xcb}, {0x35, 0x65}, {0x39, 0x04},
};

static const struct tp9963_reg tp9963_pal[] = {
	{0x06, 0x32}, {0x02, 0x47}, {0x07, 0x80}, {0x0b, 0x80},
	{0x0c, 0x13}, {0x0d, 0x51}, {0x15, 0x03}, {0x16, 0xf0},
	{0x17, 0xa0}, {0x18, 0x17}, {0x19, 0x20}, {0x1a, 0x15},
	{0x1c, 0x06}, {0x1d, 0xc0}, {0x20, 0x48}, {0x21, 0x84},
	{0x22, 0x37}, {0x23, 0x3f}, {0x2b, 0x70}, {0x2c, 0x2a},
	{0x2d, 0x4b}, {0x2e, 0x56}, {0x30, 0x7a}, {0x31, 0x4a},
	{0x32, 0x4d}, {0x33, 0xfb}, {0x35, 0x65}, {0x39, 0x04},
};

static const struct tp9963_reg tp9963_hd_25[] = {
	{0x02, 0x42}, {0x07, 0xc0}, {0x0b, 0xc0}, {0x0c, 0x13},
	{0x0d, 0x50}, {0x15, 0x13}, {0x16, 0x15}, {0x17, 0x00},
	{0x18, 0x19}, {0x19, 0xd0}, {0x1a, 0x25}, {0x1c, 0x07},
	{0x1d, 0xbc}, {0x20, 0x30}, {0x21, 0x84}, {0x22, 0x36},
	{0x23, 0x3c}, {0x2b, 0x60}, {0x2c, 0x0a}, {0x2d, 0x30},
	{0x2e, 0x70}, {0x30, 0x48}, {0x31, 0xbb}, {0x32, 0x2e},
	{0x33, 0x90}, {0x35, 0x25}, {0x39, 0x08}, {0x02, 0x46},
	{0x0d, 0x71}, {0x20, 0x40}, {0x21, 0x46}, {0x25, 0xfe},
	{0x26, 0x01}, {0x2c, 0x3a}, {0x2d, 0x5a}, {0x2e, 0x40},
	{0x30, 0x9e}, {0x31, 0x20}, {0x32, 0x10}, {0x33, 0x90},
};

static const struct tp9963_reg tp9963_hd_30[] = {
	{0x02, 0x42}, {0x07, 0xc0}, {0x0b, 0xc0}, {0x0c, 0x13},
	{0x0d, 0x50}, {0x15, 0x13}, {0x16, 0x15}, {0x17, 0x00},
	{0x18, 0x19}, {0x19, 0xd0}, {0x1a, 0x25}, {0x1c, 0x06},
	{0x1d, 0x72}, {0x20, 0x30}, {0x21, 0x84}, {0x22, 0x36},
	{0x23, 0x3c}, {0x2b, 0x60}, {0x2c, 0x0a}, {0x2d, 0x30},
	{0x2e, 0x70}, {0x30, 0x48}, {0x31, 0xbb}, {0x32, 0x2e},
	{0x33, 0x90}, {0x35, 0x25}, {0x39, 0x08}, {0x02, 0x46},
	{0x0d, 0x70}, {0x20, 0x40}, {0x21, 0x46}, {0x25, 0xfe},
	{0x26, 0x01}, {0x2c, 0x3a}, {0x2d, 0x5a}, {0x2e, 0x40},
	{0x30, 0x9d}, {0x31, 0xca}, {0x32, 0x01}, {0x33, 0xd0},
};

static const struct tp9963_reg tp9963_fhd_25[] = {
	{0x02, 0x40}, {0x07, 0xc0}, {0x0b, 0xc0}, {0x0c, 0x03},
	{0x0d, 0x50}, {0x15, 0x03}, {0x16, 0xd2}, {0x17, 0x80},
	{0x18, 0x29}, {0x19, 0x38}, {0x1a, 0x47}, {0x1c, 0x0a},
	{0x1d, 0x50}, {0x20, 0x30}, {0x21, 0x84}, {0x22, 0x36},
	{0x23, 0x3c}, {0x2b, 0x60}, {0x2c, 0x0a}, {0x2d, 0x30},
	{0x2e, 0x70}, {0x30, 0x48}, {0x31, 0xbb}, {0x32, 0x2e},
	{0x33, 0x90}, {0x35, 0x05}, {0x39, 0x0c}, {0x02, 0x44},
	{0x0d, 0x73}, {0x15, 0x01}, {0x16, 0xf0}, {0x20, 0x3c},
	{0x21, 0x46}, {0x25, 0xfe}, {0x26, 0x0d}, {0x2c, 0x3a},
	{0x2d, 0x54}, {0x2e, 0x40}, {0x30, 0xa5}, {0x31, 0x86},
	{0x32, 0xfb}, {0x33, 0x60},
};

static const struct tp9963_reg tp9963_fhd_30[] = {
	{0x02, 0x40}, {0x07, 0xc0}, {0x0b, 0xc0}, {0x0c, 0x03},
	{0x0d, 0x50}, {0x15, 0x03}, {0x16, 0xd2}, {0x17, 0x80},
	{0x18, 0x29}, {0x19, 0x38}, {0x1a, 0x47}, {0x1c, 0x08},
	{0x1d, 0x98}, {0x20, 0x30}, {0x21, 0x84}, {0x22, 0x36},
	{0x23, 0x3c}, {0x2b, 0x60}, {0x2c, 0x0a}, {0x2d, 0x30},
	{0x2e, 0x70}, {0x30, 0x48}, {0x31, 0xbb}, {0x32, 0x2e},
	{0x33, 0x90}, {0x35, 0x05}, {0x39, 0x0c}, {0x02, 0x44},
	{0x0d, 0x72}, {0x15, 0x01}, {0x16, 0xf0}, {0x20, 0x38},
	{0x21, 0x46}, {0x25, 0xfe}, {0x26, 0x0d}, {0x2c, 0x3a},
	{0x2d, 0x54}, {0x2e, 0x40}, {0x30, 0xa5}, {0x31, 0x95},
	{0x32, 0xe0}, {0x33, 0x60},
};

/* MIPI page (0x40 = 0x08), Techpoint's MIPI_2CH2LANE_594M (2 channels, 2 lanes, up to 2 x 1080p30). */
static const struct tp9963_reg tp9963_mipi_out[] = {
	{0x40, 0x08}, {0x02, 0x78}, {0x03, 0x70}, {0x04, 0x70},
	{0x05, 0x70}, {0x06, 0x70}, {0x13, 0xef}, {0x20, 0x00},
	{0x21, 0x22}, {0x22, 0x20}, {0x23, 0x9e}, {0x21, 0x22},
	{0x14, 0x00}, {0x15, 0x01}, {0x2a, 0x08}, {0x2b, 0x08},
	{0x2c, 0x10}, {0x2e, 0x0a}, {0x10, 0xa0}, {0x10, 0x20},
	{0x28, 0x02}, {0x28, 0x00},
};

static void TP9963_decoder_init(VIDEO_FORMAT format)
{
	kal_uint16 value;

	TP9963_write_cmos_sensor(0x40, 0x04);	/* both channels (CH_ALL) */
	TP9963_write_cmos_sensor(0x06, 0x12);
	TP9963_write_cmos_sensor(0x50, 0x00);
	TP9963_write_cmos_sensor(0x51, 0x00);
	TP9963_write_cmos_sensor(0x54, 0x03);

	if (format < FORMAT_NTSC || format > FORMAT_FHD_30)
		return;

	value = TP9963_read_cmos_sensor(0x42);
	if (format >= FORMAT_FHD_25)
		TP9963_write_cmos_sensor(0x42, value & 0xfc);
	else
		TP9963_write_cmos_sensor(0x42, value | 0x03);
	value = TP9963_read_cmos_sensor(0x43);
	TP9963_write_cmos_sensor(0x43, value & 0xfc);

	switch (format) {
	case FORMAT_NTSC:
		TP9963_write_table(tp9963_ntsc, ARRAY_SIZE(tp9963_ntsc));
		break;
	case FORMAT_PAL:
		TP9963_write_table(tp9963_pal, ARRAY_SIZE(tp9963_pal));
		break;
	case FORMAT_HD_25:
		TP9963_write_table(tp9963_hd_25, ARRAY_SIZE(tp9963_hd_25));
		break;
	case FORMAT_HD_30:
		TP9963_write_table(tp9963_hd_30, ARRAY_SIZE(tp9963_hd_30));
		break;
	case FORMAT_FHD_25:
		TP9963_write_table(tp9963_fhd_25, ARRAY_SIZE(tp9963_fhd_25));
		break;
	case FORMAT_FHD_30:
		TP9963_write_table(tp9963_fhd_30, ARRAY_SIZE(tp9963_fhd_30));
		break;
	default:
		break;
	}
}

static void TP9963_Sensor_Init(VIDEO_FORMAT format)
{
	LOG_INF("TP9963 sensor init enter source_format = %d\n", format);
	TP9963_decoder_init(format);
	TP9963_write_table(tp9963_mipi_out, ARRAY_SIZE(tp9963_mipi_out));
}

static VIDEO_FORMAT _TP9963_Format_Detect(void)
{
	uint32_t value;
	VIDEO_FORMAT format;

	value = TP9963_read_cmos_sensor(0x03);
	value &= 0x07; //save bit0~2

	if (0x02 == value)
		format = FORMAT_FHD_30;
	else if (0x03 == value)
		format = FORMAT_FHD_25;
	else if (0x04 == value)
		format = FORMAT_HD_30;
	else if (0x05 == value)
		format = FORMAT_HD_25;
	else
		format = FORMAT_MAX;
	LOG_INF("the tmp format is %d, value is 0x%x\n", format, value);
	return format;
}

static void TP9963_Param_Set(VIDEO_FORMAT format)
{
	switch (format) {
	case FORMAT_HD_25:
	case FORMAT_HD_30:
		input_source_height = TP9963_IMAGE_SENSOR_HD_HEIGHT;
		input_source_width = TP9963_IMAGE_SENSOR_HD_WIDTH;
		break;
	case FORMAT_FHD_25:
	case FORMAT_FHD_30:
		input_source_height = TP9963_IMAGE_SENSOR_FHD_HEIGHT;
		input_source_width = TP9963_IMAGE_SENSOR_FHD_WIDTH;
		break;
	default:
		LOG_ERR("this may be impossible\n");
		break;
	}
}

static VIDEO_FORMAT TP9963_Format_Detect(void)
{
	int retry = 12;
	VIDEO_FORMAT pre_format = FORMAT_MAX;
	VIDEO_FORMAT cur_format = FORMAT_MAX;

	do {
		cur_format = _TP9963_Format_Detect();
		if ((pre_format == cur_format) && (FORMAT_MAX != cur_format)) {
			TP9963_Param_Set(cur_format);
			break;
		}
		pre_format = cur_format;
		msleep(50);
		retry--;
	} while (retry > 0);

	LOG_INF("detect format have try %d times\n", ((12 - retry) + 1));

	if (0 == retry) {
		cur_format = FORMAT_MAX;
		LOG_ERR("format detect fail,use default param!\n");
	}
	LOG_INF("source_height = %d, source_width =%d, source_format =%d\n",
		input_source_height, input_source_width, cur_format);
	return cur_format;
}

/* Jancar: the format chosen in its settings wins over the detection. */
static VIDEO_FORMAT jac_detect_format_from_metazone(void)
{
	unsigned int value = 0;
	VIDEO_FORMAT format;

	if (MZ_SUCCESS != MetaZone_Read(METAZONE_AHD_SENSOR_VALUE, &value))
		LOG_ERR("TP9963 YUV metazone read fail\n");
	else
		LOG_INF("metazone value = %d\n", value);

	switch (value) {
	case 1:
		format = FORMAT_FHD_25;
		break;
	case 2:
		format = FORMAT_HD_30;
		break;
	case 3:
		format = FORMAT_FHD_30;
		break;
	default:
		format = FORMAT_HD_25;
		break;
	}
	TP9963_Param_Set(format);
	return format;
}

static UINT32 TP9963_GetSensorID(uint32_t *sensorId)
{
	int retry = 3;
	unsigned int value = 0;

	do {
		*sensorId = ((TP9963_read_cmos_sensor(0xfe) << 8) | TP9963_read_cmos_sensor(0xff));
		LOG_INF("sensor id = 0x%x\n", *sensorId);
		if (*sensorId == SENSOR_ID)
			break;
		retry--;
	} while (retry > 0);

	if (*sensorId != SENSOR_ID) {
		*sensorId = SENSOR_ID_ERROR;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	if (MZ_SUCCESS != MetaZone_Read(METAZONE_AHD_SENSOR_ID, &value)) {
		LOG_ERR("TP9963 YUV metazone read fail\n");
		MetaZone_Write(METAZONE_AHD_SENSOR_ID, 1);
		MetaZone_Flush(1);
	} else {
		if (value != 1) {
			MetaZone_Write(METAZONE_AHD_SENSOR_ID, 1);
			MetaZone_Flush(1);
		}
		LOG_INF("metazone_sensor_id_value = %d\n", value);
	}

	return ERROR_NONE;
}

static SIGNAL_STATUS TP9963_Get_SignalStatus(unsigned int ch)
{
	UINT16 value;

	LOG_INF("get signal status enter:%d\n", ch);
	if (ch > 3)
		ch = 0;
	TP9963_write_cmos_sensor(0x40, ch);
	value = TP9963_read_cmos_sensor(0x01);
	LOG_INF("TP9963 ch:%d,signal status is %d\n", ch, value);

	return (0x7a == value) ? SIGNAL_READY : SIGNAL_LOST;
}

static SIGNAL_STATUS TP9963_SignalStatus_Get(void)
{
	return TP9963_Get_SignalStatus(0);
}

static void TP9963_Set_Picture(unsigned int ch, kal_uint32 reg, unsigned int value)
{
	TP9963_write_cmos_sensor(0x40, ch);
	TP9963_write_cmos_sensor(reg, value);
}

UINT32 TP9963Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = TP9963_IMAGE_SENSOR_WIDTH;
	image_window->ExposureWindowHeight = TP9963_IMAGE_SENSOR_HEIGHT;

	memcpy(&TP9963SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 TP9963Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Enter\n");

	image_window->GrabStartX = 0;
	image_window->GrabStartY = 0;
	image_window->ExposureWindowWidth = TP9963_IMAGE_SENSOR_WIDTH;
	image_window->ExposureWindowHeight = TP9963_IMAGE_SENSOR_HEIGHT;

	memcpy(&TP9963SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 TP9963GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorPreviewWidth = TP9963_IMAGE_SENSOR_WIDTH;
	pSensorResolution->SensorPreviewHeight = TP9963_IMAGE_SENSOR_HEIGHT;
	pSensorResolution->SensorFullWidth = TP9963_IMAGE_SENSOR_WIDTH;
	pSensorResolution->SensorFullHeight = TP9963_IMAGE_SENSOR_HEIGHT;
	pSensorResolution->SensorVideoWidth = TP9963_IMAGE_SENSOR_WIDTH;
	pSensorResolution->SensorVideoHeight = TP9963_IMAGE_SENSOR_HEIGHT;
	pSensorResolution->SensorCh0Width = input_source_width;
	pSensorResolution->SensorCh0Height = input_source_height;
	pSensorResolution->SensorCh1Width = input_source_width;
	pSensorResolution->SensorCh1Height = input_source_height;

	return ERROR_NONE;
}

UINT32 TP9963Open(void)
{
	UINT32 sensor_id = 0;
	UINT32 ret = 0;
	unsigned int value = 0;

	LOG_INF("TP9963Open Enter\n");

	ret = TP9963_GetSensorID(&sensor_id);
	if (ERROR_NONE != ret) {
		LOG_ERR("TP9963 YUV read sensor id failed\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	if (MZ_SUCCESS != MetaZone_Read(METAZONE_AHD_FORMAT_DEFAULT, &value))
		LOG_ERR("TP9963 YUV metazone read fail\n");
	else
		LOG_INF("metazone value = %d\n", value);

	if ((FORMAT_HD_25 <= value) && (FORMAT_FHD_30 >= value)) {
		source_format = value;
		TP9963_Param_Set(source_format);
	} else {
		TP9963_decoder_init(FORMAT_HD_25);
		msleep(100);
		source_format = TP9963_Format_Detect();
	}

	source_format = jac_detect_format_from_metazone();
	LOG_INF("source_format:%d\n", source_format);

	if (FORMAT_MAX != source_format)
		TP9963_Sensor_Init(source_format);

	LOG_INF("Leave\n");
	return ERROR_NONE;
}

UINT32 TP9963Close(void)
{
	LOG_INF("TP9963Close");
	return ERROR_NONE;
}


UINT32 TP9963GetInfo(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX = TP9963_IMAGE_SENSOR_WIDTH;
	pSensorInfo->SensorPreviewResolutionY = TP9963_IMAGE_SENSOR_HEIGHT;
	pSensorInfo->SensorFullResolutionX = TP9963_IMAGE_SENSOR_WIDTH;
	pSensorInfo->SensorFullResolutionY = TP9963_IMAGE_SENSOR_HEIGHT;

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

	/* Same timing for every scenario. */
	pSensorInfo->SensorClockFreq = 24;
	pSensorInfo->SensorClockDividCount = 3;
	pSensorInfo->SensorClockRisingCount = 0;
	pSensorInfo->SensorClockFallingCount = 2;
	pSensorInfo->SensorPixelClockCount = 3;
	pSensorInfo->SensorDataLatchCount = 2;
	pSensorInfo->SensorGrabStartX = 0;
	pSensorInfo->SensorGrabStartY = 0;

	memcpy(pSensorConfigData, &TP9963SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	return ERROR_NONE;
}

UINT32 TP9963Control(enum MSDK_SCENARIO_ID_ENUM ScenarioId,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
		MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	LOG_INF("Enter, ScenarioId = %d\n", ScenarioId);
	switch (ScenarioId) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		TP9963Preview(pImageWindow, pSensorConfigData);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		TP9963Capture(pImageWindow, pSensorConfigData);
		break;
	default:
		break;
	}

	LOG_INF("Leave\n");

	return ERROR_NONE;
}

UINT32 TP9963FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
		UINT8 *pFeaturePara, UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16 = (UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32 = (UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32 = (UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData = (MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData = (MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	unsigned long long *feature_data = (unsigned long long *)pFeaturePara;
	unsigned int ch, value;

	switch (FeatureId) {
	case SENSOR_FEATURE_GET_RESOLUTION:
	case SENSOR_FEATURE_GET_PERIOD:
		*pFeatureReturnPara16++ = TP9963_IMAGE_SENSOR_WIDTH;
		*pFeatureReturnPara16 = TP9963_IMAGE_SENSOR_HEIGHT;
		*pFeatureParaLen = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*pFeatureReturnPara32 = 648000000;
		*pFeatureParaLen = 4;
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		TP9963_isp_master_clock = *pFeatureData32;
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		TP9963_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		pSensorRegData->RegData = TP9963_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
	case SENSOR_FEATURE_GET_CONFIG_PARA:
		memcpy(pSensorConfigData, &TP9963SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
		*pFeatureParaLen = sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		break;
	case SENSOR_FEATURE_SET_CCT_REGISTER:
	case SENSOR_FEATURE_SET_ENG_REGISTER:
	case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
	case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
	case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
	case SENSOR_FEATURE_GET_GROUP_COUNT:
		*pFeatureReturnPara32 = 0;
		*pFeatureParaLen = 4;
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		*pFeatureReturnPara32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*pFeatureParaLen = 4;
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		TP9963_GetSensorID(pFeatureData32);
		break;
	case SENSOR_FEATURE_GET_PIXEL_RATE:
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 364500000;
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 648000000;
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = TP9963_FPS;
		break;
	case SENSOR_FEATURE_GET_CH0_SIGNAL_STATUS:
	case SENSOR_FEATURE_GET_AHD_SIGNAL_STATUS:
		*pFeatureData32 = TP9963_Get_SignalStatus(0);
		break;
	case SENSOR_FEATURE_GET_CH1_SIGNAL_STATUS:
		*pFeatureData32 = TP9963_Get_SignalStatus(1);
		break;
	case SENSOR_FEATURE_GET_CH0_INTERLACE_FLAG:
	case SENSOR_FEATURE_GET_CH1_INTERLACE_FLAG:
		*pFeatureData32 = 0;
		break;
	case SENSOR_FEATURE_SET_BRIGHTNESS_VALUE:
		ch = (unsigned int)feature_data[0];
		value = (unsigned int)feature_data[1];
		LOG_INF("Channel is %d, brightness value is %d\n", ch, value);
		TP9963_Set_Picture(ch, 0x10, value);
		break;
	case SENSOR_FEATURE_SET_CONTRAST_VALUE:
		ch = (unsigned int)feature_data[0];
		value = (unsigned int)feature_data[1];
		LOG_INF("Channel is %d, contrast value is %d\n", ch, value);
		TP9963_Set_Picture(ch, 0x11, value);
		break;
	case SENSOR_FEATURE_SET_SATURATION_VALUE:
		ch = (unsigned int)feature_data[0];
		value = (unsigned int)feature_data[1];
		LOG_INF("Channel is %d, saturation value is %d\n", ch, value);
		TP9963_Set_Picture(ch, 0x12, value);
		break;
	case SENSOR_FEATURE_SET_HUE_VALUE:
		ch = (unsigned int)feature_data[0];
		value = (unsigned int)feature_data[1];
		LOG_INF("Channel is %d, hue value is %d\n", ch, value);
		TP9963_Set_Picture(ch, 0x13, value);
		break;
	default:
		break;
	}
	return ERROR_NONE;
}


static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	TP9963Open,
	TP9963GetInfo,
	TP9963GetResolution,
	TP9963FeatureControl,
	TP9963Control,
	TP9963Close,
	TP9963_SignalStatus_Get
};

UINT32 TP9963_YUV_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}
