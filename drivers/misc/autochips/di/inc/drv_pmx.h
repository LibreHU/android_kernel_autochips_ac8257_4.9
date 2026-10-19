/*
* Copyright (c) 2016 AutoChips Inc.
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
*  Plane mixer: Interface
*****************************************************************************/

#ifndef _DRV_PMX_H_
#define _DRV_PMX_H_

#ifndef ATC_ARM2_LK_SUPPORT
//#include "x_plane_mxr.h"
#include "types.h"

#else
//#include "x_plane_mxr.h"
#include "platform/mt_typedefs.h"
#include "sys/types.h"
#include "di_types.h"

#endif
#include "drv_config.h"


#define MODEL_DEVELOPE  1

#ifdef MODEL_CLRQAM
#ifndef CC_PMX_MT5371_API
#define CC_PMX_MT5371_API
#endif
#endif

#ifdef MODEL_DCR
#ifndef CC_PMX_MT5371_API
#define CC_PMX_MT5371_API
#endif
#endif

#ifdef MODEL_DEVELOPE
#ifndef CC_PMX_MT5371_API
#define CC_PMX_MT5371_API
#endif
#endif


/* Maximum number of input plane for each mixer*/
#define PMX_MAX_INPORT_NS		8


/* Plane mixer capability*/
#define PMX_CAP_480I			(1 << 0)
#define PMX_CAP_576I			(1 << 1)
#define PMX_CAP_480P			(1 << 2)
#define PMX_CAP_576P			(1 << 3)
#define PMX_CAP_720P60HZ		(1 << 4)
#define PMX_CAP_720P50HZ		(1 << 5)
#define PMX_CAP_1080I60HZ		(1 << 6)
#define PMX_CAP_1080I50HZ		(1 << 7)
#define PMX_CAP_1080P60HZ		(1 << 8)
#define PMX_CAP_1080P50HZ		(1 << 9)
#define PMX_CAP_1080P30HZ		(1 << 10)
#define PMX_CAP_1080P25HZ		(1 << 11)
#define PMX_CAP_1080P24HZ		(1 << 12)
#define PMX_CAP_1080P23_976HZ		(1 << 13)
#define PMX_CAP_1080P29_97HZ		(1 << 14)
#define PMX_CAP_3D_RES			(1 << 15)
#define PMX_CAP_PANEL_RES		(1 << 16)

#define PMX_CAP_REORDER			(1 << 19)
#define PMX_CAP_GAMMA			(1 << 20)
#define PMX_CAP_BRIGHTNESS		(1 << 21)
#define PMX_CAP_CONTRAST		(1 << 22)
#define PMX_CAP_HUE			(1 << 23)
#define PMX_CAP_SATURATION		(1 << 24)
#define PMX_CAP_DIGITAL_OUT		(1 << 25)
#define PMX_CAP_HDMI_DEEP_COLOR		(1 << 26)
#define PMX_CAP_SHARPNESS		(1 << 27)
#define PMX_CAP_HDCP_ON_OFF		(1 << 28)
#define PMX_CAP_CLOSED_CAPTION_ON_OFF	(1 << 29)
#define PMX_CAP_BLACK_LV		(1 << 30)

#define PMX_SWAP_1_2                  1
#define PMX_SWAP_3_4                  2
#define PMX_SWAP_6_7                  3
#define PMX_SWAP_4_7                  4
#define PMX_SWAP_1_3                  5
#define PMX_SWAP_5_8                  6

#define PMX_VDP1_ON             (0x75643021)
#define PMX_VDP1_OFF            (0x75643210)
/******************************************************************************
* PMX API
******************************************************************************/


/************************/
/* PMX native interface */
/************************/
extern void PMX_Init(bool fgHwReset);
extern void PMX_Uninit(void);
extern s32 i4PMX_Uninit(u32 u4Case);
extern void PMX_Stop(void);
extern void PMX_Reset(u8 ucPmxId, bool fgHwReset);
extern void PMX_QueryStatus(void);
extern u32 PMX_GetCap(u8 ucPmxId, u32 *pu4Cap);
extern u32 PMX_SetEnable(u8 ucPmxId, u8 ucEnable);
extern u32 PMX_SetForceCGMSEnable(u8 ucPmxId, u8 ucEnable);
extern u32 PMX_SetCBEnable(u8 ucPmxId, u8 ucEnable);
extern u32 PMX_SetMuteEnable(u8 ucPmxId, u8 ucEnable);
extern u32 PMX_SetTvType(u8 ucPmxId, u8 ucTvType);
extern u32 PMX_SetBlackLv(u8 ucPmxId, u8 ucCAVEn, u8 ucCVBSEn);
extern u32 PMX_GetBlackLv(u8 ucPmxId, u8 *pucCAVEn, u8 *pucCVBSEn);
extern u32 PMX_SetGamma(u8 ucPmxId, u8 ucGamma);
extern u32 PMX_SetBrightness(u8 ucPmxId, u8 ucBrightness, u8 bSetTVE);
extern u32 PMX_SetContrast(u8 ucPmxId, u8 ucContrast, u8 bSetTVE);
extern u32 PMX_SetHue(u8 ucPmxId, u8 ucHue, u8 bSetTVE);
extern u32 PMX_SetSaturation(u8 ucPmxId, u8 ucSaturation, u8 bSetTVE);
extern u32 PMX_SetSharpness(u8 ucPmxId, u8 ucSharpness, u8 bSetTVE);
extern u32 PMX_SetBg(u8 ucPmxId, u32 u4BgColor);
extern u32 PMX_SetPlaneOrder(u8 ucPmxId, u8 ucVdpId, u8 ucPlaneOrder);
extern u32 PMX_SetDigitalOutEnable(u8 ucPmxId, u8 ucEnable);
extern u32 PMX_SetDigitalOutPara(u8 ucPmxId, u8 ucDigitalBit, u8 ucMtkMode);
extern u32 PMX_SetPlaneOrderArray(u8 ucPmxId, const u32 *pu4PlaneOrder);
extern u32 PMX_SetDelay(u8 ucVdoId, u8 ucADJ_F, u16 u2HDelay, u16 u2VDelay);
extern u32 PMX_SetTVSystem(u8 ucPmxId, u16 u2VSyncFreq,  bool  fgTvSysAuto);
extern u32 PMX_GetEnable(u8 ucPmxId, u8 *pucEnable);
extern u32 PMX_GetForceCGMSEnable(u8 ucPmxId, u8 *pucEnable);
extern u32 PMX_GetCBEnable(u8 ucPmxId, u8 *pucEnable);
extern u32 PMX_GetMuteEnable(u8 ucPmxId, u8 *pucEnable);
extern u32 PMX_GetTvType(u8 ucPmxId, u8 *pucTvType);
extern u32 PMX_GetGamma(u8 ucPmxId, u8 *pucGamma);
extern u32 PMX_GetGammaNs(u8 ucPmxId, u8 *pucGammaNs);
extern u32 PMX_GetBrightness(u8 ucPmxId, u8 *pucBrightness);
extern u32 PMX_GetContrast(u8 ucPmxId, u8 *pucContrast);
extern u32 PMX_GetHue(u8 ucPmxId, u8 *pucHue);
extern u32 PMX_GetSaturation(u8 ucPmxId, u8 *pucSaturation);
extern u32 PMX_GetSharpness(u8 ucPmxId, u8 *pucSharpness);
extern u32 PMX_GetPlaneOrder(u8 ucPmxId, u8 ucVdpId, u8 *pucPlaneOrder);
extern u32 PMX_SetSwapEnable(u8 ucPmxId, u8 ucEnable, u8 ucSwapMode);
extern u32 PMX_GetSwapEnable(u8 ucPmxId, u8 *pucEnable, u8 ucSwapMode);
extern u32 PMX_GetDigitalOutEnable(u8 ucPmxId, u8 *pucEnable);
extern u8 PMX_ProgressiveOut(u8 ucPmxId);
//extern __u32 PMX_SetExternalChipSetting(PLA_MXR_EXTERNAL_CHIP_SETTING_E eSetType, __s32 i4Value);
extern u32 PMX_SetAspectRatio(u8 ucPmxId, u32 u4AspectRatio);
extern u32 PMX_SetAspectRatioEx(u8 ucPmxId, u32 u4AspectRatio, bool fgApplyOnNext);
extern u32 PMX_SetAspectRatioNext(u8 ucPmxId);
extern u32 PMX_SetDigitalOutPhase(u8 ucPmxId, u32 u4Phase);
extern u32 PMX_SetDigitalOutDrv(u32 u4Drving);
extern u32 PMX_SetDigitalOutRgb(u8 ucPmxId, u8 ucRgb);
extern u32 PMX_GetDigitalOutRgb(u8 ucPmxId, u8 *pucRgb);
extern u32 i4Pmx_Connect(u16 u2ThisCompType, u16 u2ThisCompId
	, u16 u2UpstreamCompType, u16 u2UpstreamCompId);
extern u32 i4Pmx_Disconnect(u16 u2ThisCompType, u16 u2ThisCompId
	, u16 u2UpstreamCompType, u16 u2UpstreamCompId);
extern u32 PMX_ChkSupportedRes(u8 ucPmxId, u8 ucChkRes, u16 u2VSyncFreq
	,  u8 *ucSupportedAutoRes, u16 *ucSupportedAutoVsyncFreg);
extern u32 PMX_SetHdmiDeepColor(u8 ucPmxId, u8 ucDeepColor);
extern u32 PMX_SetHdcpOnOff(u8 ucPmxId, u8 ucHdcpOnOff);
//extern __u32 PMX_SetClosedCaptionOnOff(__u8 ucPmxId, PLA_MXR_CLOSED_CAPTION_ON_OFF_T eClosedCaptionOnOff);
extern void PMX_RecordUiResAuto(bool fgIsAuto);
extern u32 PMX_SetDigestAspectRatioEx(u8 ucPmxId, u32 u4AspectRatio, bool fgDigestEnable);

extern u32 u4PmxCompId2LocalId(u16 u2CompType, u16 u2CompId);
extern u32 PMX_SetMediaType(u8 ucPmxId, u8 ucMediaType);
extern u32 PMX_SetDGOSource(bool fgBeforeMixer);
extern u32 PMX_SetCVBSSetup(bool fgEnable);
#if CONFIG_DRV_3D_SUPPORT
extern PMX_VSYNC_MODE_T PMX_GetCurrVSyncMode(u8 ucPmxId);
extern PMX_VSYNC_MODE_T PMX_GetNextVSyncMode(__u8 ucPmxId);
extern void PMX_Set3DMode(PMX_3D_MODE_T ePmx3DMode);
extern PMX_3D_MODE_T PMX_Get3DMode(void);
#endif

extern u32 PMX_SetArbPlaneOrder(u8 ucPmxId, u32 u4PlaneOrder);
extern u32 PMX_GetArbPlaneOrder(u8 ucPmxId, u32 *pu4PlaneOrder);

/***************/
/* MW Temp API */
/***************/
extern u32 d_PMX_SetOrderVdpComp0(u8 ucPmxId, u32 u4OnTop); /* 1: VDP 0 is on top; 0: VDP 0 is on bottom.*/
extern u32 d_PMX_GetOrderVdpComp0(u8 ucPmxId, u32 *pu4OnTop);


/***********/
/* OLD API */
/***********/
#ifdef CC_DIGITAL_VDO_LOOPBACK
extern u32 PMX_SetTveEnable(u8 ucTveId);
extern u32 PMX_SetTveDisable(__u8 ucTveId);
extern u32 PMX_GetLoopback(u8 ucPmxId, u32 *pu4Loopbacking, u32 *pu4LbWidth, u32 *pu4LbHeight);
extern u8 PMX_GetLoopbackPmx(void);
#endif


#endif /* _DRV_PMX_H_ */



