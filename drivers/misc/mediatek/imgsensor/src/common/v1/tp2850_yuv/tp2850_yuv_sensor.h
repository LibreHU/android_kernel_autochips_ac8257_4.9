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
*   trn6752m_yuv_Sensor.h
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
#ifndef _TP2850_YUV_SENSOR_H
#define _TP2850_YUV_SENSOR_H

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

#define SENSOR_ID				0x2850
#define SENSOR_ID_ERROR				0xFFFFFFFF

#define TP2850_IMAGE_SENSOR_HD30_WIDTH 			1280
#define TP2850_IMAGE_SENSOR_HD30_HEIGHT			720
#define TP2850_IMAGE_SENSOR_HD25_WIDTH 			1280
#define TP2850_IMAGE_SENSOR_HD25_HEIGHT			720

#define TP2850_IMAGE_SENSOR_FHD30_WIDTH			1920
#define TP2850_IMAGE_SENSOR_FHD30_HEIGHT		1080
#define TP2850_IMAGE_SENSOR_FHD25_WIDTH			1920
#define TP2850_IMAGE_SENSOR_FHD25_HEIGHT		1080

#define TP2850_IMAGE_SENSOR_NTSC_WIDTH			1920
#define TP2850_IMAGE_SENSOR_NTSC_HEIGHT			240
#define TP2850_IMAGE_SENSOR_PAL_WIDTH			1920
#define TP2850_IMAGE_SENSOR_PAL_HEIGHT			288

#define TP2850_FPS_25					250
#define TP2850_FPS_30					300


extern int iReadRegI2C(
	u8 *a_pSendData,
	u16 a_sizeSendData,
	u8 *a_pRecvData,
	u16 a_sizeRecvData,
	u16 i2cId);

extern int iReadRegI2CTiming(
	u8 *a_pSendData,
	u16 a_sizeSendData,
	u8 *a_pRecvData,
	u16 a_sizeRecvData,
	u16 i2cId,
	u16 timing);

extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
extern int iWriteRegI2CTiming(
	u8 *a_pSendData,
	u16 a_sizeSendData,
	u16 i2cId,
	u16 timing);


#endif

