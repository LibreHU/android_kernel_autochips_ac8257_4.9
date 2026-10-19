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
*   tp2855_yuv_Sensor.h
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
#ifndef _TP2855_YUV_SENSOR_H
#define _TP2855_YUV_SENSOR_H

typedef enum {
	NTSC,
	PAL,
	HD_25,
	HD_30,
	FHD_25,
	FHD_30,
	FORMAT_MAX
} VIDEO_FORMAT;

#define FORMAT_NTSC				0x0
#define FORMAT_PAL				0x1
#define FORMAT_HD25				0x2
#define FORMAT_HD30				0x3
#define FORMAT_FHD25				0x4
#define FORMAT_FHD30				0x5
#define SENSOR_ID				0x2855

#define TP2855_IMAGE_SENSOR_FHD25_WIDTH				1920
#define TP2855_IMAGE_SENSOR_FHD25_HEIGHT			1080
#define TP2855_IMAGE_SENSOR_HD25_WIDTH				1280
#define TP2855_IMAGE_SENSOR_HD25_HEIGHT				720
#define TP2855_IMAGE_SENSOR_NTSC_WIDTH				720
#define TP2855_IMAGE_SENSOR_NTSC_HEIGHT				240
#define TP2855_IMAGE_SENSOR_PAL_WIDTH				720
#define TP2855_IMAGE_SENSOR_PAL_HEIGHT				288


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

