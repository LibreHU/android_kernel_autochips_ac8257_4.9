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
*   rn6864m_yuv_Sensor.h
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
#ifndef _RN6864M_YUV_SENSOR_H
#define _RN6864M_YUV_SENSOR_H

typedef enum {
	NTSC,
	PAL,
	HD_25,
	HD_30,
	FHD_25,
	FHD_30,
	FORMAT_MAX
} VIDEO_FORMAT;

#define FORMAT_NTSC				0x1
#define FORMAT_PAL				0x2
#define FORMAT_HD25				0x3
#define FORMAT_HD30				0x4
#define FORMAT_FHD25			0x5
#define FORMAT_FHD30			0x6
#define SENSOR_ID				0x0510

#define RN6864M_IMAGE_SENSOR_HD25_WIDTH				1280
#define RN6864M_IMAGE_SENSOR_HD25_HEIGHT			720


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

