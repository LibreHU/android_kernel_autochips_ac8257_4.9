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
 *   tp9963_yuv_sensor.h
 *
 * Project:
 * --------
 *   Android P on AC8257
 *
 * Description:
 * ------------
 *   Techpoint TP9963 (2-channel HD-TVI / AHD / CVI / CVBS to MIPI CSI-2 decoder) driver.
 *   The source of this driver is missing from the AutoChips GPL package: rebuilt from the
 *   stock UJC201 kernel (Jancar, 4.9.117+ #25), same structure as tp2850_yuv.
 *
 ******************************************************************************/
#ifndef _TP9963_YUV_SENSOR_H
#define _TP9963_YUV_SENSOR_H

typedef enum {
	FORMAT_NTSC = 1,
	FORMAT_PAL,
	FORMAT_HD_25,
	FORMAT_HD_30,
	FORMAT_FHD_25,
	FORMAT_FHD_30,
	FORMAT_MAX
} VIDEO_FORMAT;

typedef enum {
	SENSOR_FORMAT_UYVY = 0,
	SENSOR_FORMAT_YUYV,
	SENSOR_FORMAT_BGR888,
	SENSOR_FORMAT_MAX
} SENSOR_FORMAT;

#define SENSOR_ID				0x2863
#define SENSOR_ID_ERROR				0xFFFFFFFF

/* The stock driver reports 1280 x 720 to the camera HAL whatever the input format. */
#define TP9963_IMAGE_SENSOR_WIDTH		1280
#define TP9963_IMAGE_SENSOR_HEIGHT		720

#define TP9963_IMAGE_SENSOR_HD_WIDTH		1280
#define TP9963_IMAGE_SENSOR_HD_HEIGHT		720
#define TP9963_IMAGE_SENSOR_FHD_WIDTH		1920
#define TP9963_IMAGE_SENSOR_FHD_HEIGHT		1080

#define TP9963_FPS				250

extern int iReadRegI2C(
	u8 *a_pSendData,
	u16 a_sizeSendData,
	u8 *a_pRecvData,
	u16 a_sizeRecvData,
	u16 i2cId);

extern int iWriteRegI2CTiming(
	u8 *a_pSendData,
	u16 a_sizeSendData,
	u16 i2cId,
	u16 timing);

#endif
