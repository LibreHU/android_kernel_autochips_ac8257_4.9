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
*  Video Plane: Interface
*****************************************************************************/

#ifndef _DRV_VDP_H_
#define _DRV_VDP_H_

#include <media/atc/x_vid_plane.h>
#include <media/atc/x_vid_dec.h>
#include "drv_common.h"
#include "drv_config.h"
#include "chip_ver.h"

#define MODEL_DEVELOPE  1

#ifdef MODEL_CLRQAM
#ifndef CC_VDP_MT5371_API
#define CC_VDP_MT5371_API
#endif
#endif

#ifdef MODEL_DCR
#ifndef CC_VDP_MT5371_API
#define CC_VDP_MT5371_API
#endif
#endif

#ifdef MODEL_DEVELOPE
#ifndef CC_VDP_MT5371_API
#define CC_VDP_MT5371_API
#endif
#endif

/* Maximum number of video plane*/
#define VDP_MAX_NS				3
#define VDP_NS					3
#define VDP_HW_NS       2
#define VDP_1					0	/* VDP1*/
#define VDP_2					1	/* VDP2*/
#define VDP_3					2	/* OSD1, Virtual VDP*/
#define VDP_4					3	/* OSD2, Virtual VDP*/
#define VDP_5					4	/* OSD3, Virtual VDP*/
#define VDP_6                   5
#define VDP_7                   6
#define VDP_8                   7

/* Maximum number of input port for each video plane*/

/* Video plane configuration return value*/
#define VDP_SET_ERROR			0
#define VDP_SET_OK				1

/* Video plane status*/
/*#define VDP_STATUS_NOSIGNAL         0
#define VDP_STATUS_NOSUPPORT        1
#define VDP_STATUS_UNKNOWN          2
#define VDP_STATUS_STABLE           3
*/
/* Video plane argument*/
#define VDP_ARG_BLENDING            0
#define VDP_ARG_BRIGHTNESS          1
#define VDP_ARG_CONTRAST            2
#define VDP_ARG_HUE                 3
#define VDP_ARG_SATURATION          4
#define VDP_ARG_BLE                 5
#define VDP_ARG_CTI                 6
#define VDP_ARG_ETI                 7
#define VDP_ARG_SHARPNESS_ON_OFF    8
#define VDP_ARG_SHARPNESS           9
#define VDP_ARG_NR                  10
#define VDP_ARG_COLOR_SUPPRESS      11
#define VDP_ARG_DEINT_FILM          12
#define VDP_ARG_DEINT_DEINT         13
#define VDP_ARG_DEINT_EDGE          14
#define VDP_ARG_ADAPTIVE_LUMA       15
#define VDP_ARG_SCE                 16
#define VDP_ARG_R_GAIN				17
#define VDP_ARG_G_GAIN				18
#define VDP_ARG_B_GAIN				19
#define VDP_ARG_R_OFFSET			20
#define VDP_ARG_G_OFFSET			21
#define VDP_ARG_B_OFFSET			22
#define VDP_ARG_GAMMA				23
#define VDP_ARG_WHITE_PEAK_LMT		24
#define VDP_ARG_BACK_LIGHT_LVL      25
#define VDP_ARG_ADAPTIVE_BACK_LIGHT 26
#define VDP_ARG_3D_NR               27
#define VDP_ARG_NS                  28
#define VDP_ARG_SOFT_BWS            29    /*SoftBWS*/

/*                                     // Video plane mode
#define VDP_MODE_NORMAL			0
#define VDP_MODE_BG				1
#define VDP_MODE_DEINT			3
#define VDP_MODE_UNKNOWN		4
*/
/* Video plane format*/
/*#define VDP_FMT_NORMAL			0
#define VDP_FMT_LETTERBOX		1
#define VDP_FMT_PAN_SCAN		2
#define VDP_FMT_USER			3
#define VDP_FMT_UNKNOWN			4
*/
/* Video plane enhance mode*/
/*#define VDP_ENC_NONE			0
#define VDP_ENC_ON				1
#define VDP_ENC_BLUR			2
#define VDP_ENC_UNKNOWN			3
*/
/* Video plane capability*/
#define VDP_CAP_OUTPUT_AUX		(1 << 0)
#define VDP_CAP_QV				(1 << 1)
#define VDP_CAP_DEINT			(1 << 2)
#define VDP_CAP_ENHANCE			(1 << 3)
#define VDP_CAP_420_CM			(1 << 4)
#define VDP_CAP_32BIT_CM		(1 << 5)

/* Aspect Ratio*/
#define VDP_UNIT_ASPECT_RATIO	0x10000
#define VDP_4_3_ASPECT_RATIO	((VDP_UNIT_ASPECT_RATIO * 4) / 3)
#define VDP_16_9_ASPECT_RATIO	((VDP_UNIT_ASPECT_RATIO * 16) / 9)
#define VDP_221_1_ASPECT_RATIO	((VDP_UNIT_ASPECT_RATIO * 221) / 100)

/*
typedef enum
{
    VDP_REGION_TYPE_UNKNOWN = 0,
    VDP_REGION_TYPE_PIXEL,
    VDP_REGION_TYPE_PERMILLE,
} VDP_REGION_TYPE_T;
*/
/* Video plane region (source/output)*/
typedef struct _VDP_REGION_T {
	VID_PLA_REGION_TYPE_T   eRegionType;
	u32			            X;
	u32			            Y;
	u32			            Width;
	u32			            Height;
	u8                   bIsFullRegion;
} VDP_REGION_T;

/* Video Plane Call Back Error*/
#define VDP_CB_ERR				(0x80000000)

typedef enum _EVDP_VIDEO_INPUT_SOURCE_T {
	EVDP_VIS_TUNER_DIGITAL = 0,

	EVDP_VIS_MAX
} EVDP_VIDEO_INPUT_SOURCE_T;

/** Brief of EVDP_UI_VIDEO_QUALITY_ITEM_T
 *  Ui Video Quality Item (UVQI) type
 */
typedef enum _EVDP_UI_VIDEO_QUALITY_ITEM_T {
	EVDP_UVQI_BRIGHTNESS = 0,
	EVDP_UVQI_CONTRAST,
	EVDP_UVQI_HUE,
	EVDP_UVQI_SATURATION,
	EVDP_UVQI_CTI,
	EVDP_UVQI_ETI,
	EVDP_UVQI_SHARPNESS,
	EVDP_UVQI_COLOR_GAIN,
	EVDP_UVQI_COLOR_OFFSET,
	EVDP_UVQI_COLOR_SUPPRESS,
	EVDP_UVQI_NR,
	EVDP_UVQI_BLACK_LVL_EXT,
	EVDP_UVQI_WHITE_PEAK_LMT,
	EVDP_UVQI_FLESH_TONE,       /* SCE: second color enhance*/
	EVDP_UVQI_LUMA,             /* adaptive luma*/
	EVDP_UVQI_BACK_LIGHT_LVL,
	EVDP_UVQI_ADAPTIVE_BACK_LIGHT,
	EVDP_UVQI_3D_NR,

	EVDP_UVQI_MAX
} EVDP_UI_VIDEO_QUALITY_ITEM_T;

typedef void (*PFN_VDP_ERR_NOTIFY)(u32 u4Arg1, u32 u4Arg2, u32 u4Arg3, u32 u4Arg4);
typedef void (*PTS_CB_FUNC)(u32 u4Pts, u32 u4Arg);

/** Brief of VDP_CONF_T
 *  min, max, default values of UI video quality item.
 */
typedef struct _VDP_UI_VQ_MIN_MAX_DFT_T {
	s32 i4Min;
	s32 i4Max;
	s32 i4Dft;
} VDP_UI_VQ_MIN_MAX_DFT_T;
/*

typedef enum
{
    VDP_NFY_COND_ERROR = -1,
    VDP_NFY_COND_CTRL_DONE,
    VDP_NFY_INBAND_CMD_DONE,
    VID_NFY_COND_AUTO_PAUSE_DONE,
    VID_NFY_COND_INJECT_DONE
} VDP_NFY_COND_T;

*/

#if !UNIFORM_DRV_CALLBACK
typedef void (*PFN_VDP_NFY_FCT)(
	void               *pvNfyTag,
	VID_PLA_COND_T      eNfyCond,
	u32              u4Data1,
	u32              u4Data2
);


typedef struct _VDP_NFY_INFO_T {
	void *pvTag;
	PFN_VDP_NFY_FCT pfNfyFct;
} VDP_NFY_INFO_T;
#endif

typedef enum {
	CAPTURE_CURRENT_PICTURE,
	CAPTURE_NTH_PICTURE,
	CAPTURE_PICTURE_WITH_PTS,
} VDP_CMD_CAPTURE_MODE_T;
typedef struct {
	VDP_CMD_CAPTURE_MODE_T eCaptureType;
	union {
		u64 Pts;
		u32 FrameNum;
	} u;
	u32 AddrY;
	u32 AddrC;
	u32 Format;
	u16 FrameWidth;
	u16 FrameHeight;
	u16 Width;
	u16 Height;
} VDP_CMD_CAPTURE_T;
/* VID_PLA_GET_TYPE_PICTURE_INFO **********************************************/
/*typedef enum
{
    VDP_PIC_INFO_TYPE_UNKNOWN = 0,
    VDP_PIC_INFO_TYPE_CURR_DISPLAY,
    VDP_PIC_INFO_TYPE_LATEST_DISPLAY_I,
} VDP_PIC_INFO_TYPE_T;
*/
typedef struct _VDP_PICTURE_INFO_T {
	VID_PLA_PIC_INFO_TYPE_T   Type;
	u64                    u8Offset;
	u64                    u8Pts;
	PBINF_V                   rPbInf;               /*/< Playback information*/
} VDP_PICTURE_INFO_T;

typedef enum {
	VDP_DISABLED = 0,
	VDP_HIDDEN,
	VDP_ENABLED
} VDP_ENABLE_T;
/* defines.*/
/* Picture Coding Type*/
#define MPEG_PIC_TYPE_I						1
#define MPEG_PIC_TYPE_P						2
#define MPEG_PIC_TYPE_B						3

/* Picture Structure*/
#define MPEG_PIC_TOP_FIELD					1
#define MPEG_PIC_BOTTOM_FIELD				2
#define MPEG_PIC_FRAME						3

/* Frame Rate Code*/
#define MPEG_FRAME_RATE_24_					1	/* minus*/
#define MPEG_FRAME_RATE_24					2
#define MPEG_FRAME_RATE_25					3
#define MPEG_FRAME_RATE_30_					4	/* minus*/
#define MPEG_FRAME_RATE_30					5
#define MPEG_FRAME_RATE_50					6
#define MPEG_FRAME_RATE_60_					7	/* minus*/
#define MPEG_FRAME_RATE_60					8

/* Aspect Ratio*/
#define MPEG_ASPECT_RATIO_1_1				1	/* SAR, 1:1*/
#define MPEG_ASPECT_RATIO_4_3				2	/* DAR, 4:3*/
#define MPEG_ASPECT_RATIO_16_9				3	/* DAR, 16:9*/
#define MPEG_ASPECT_RATIO_221_1				4	/* DAR, 2.21:1*/

/* Colour Primary*/
#define MPEG_COLOR_PRIMARY_709				1

#define VDP_FRAME_RATE_UNKNOWN				0xFF
#define VDP_FRAME_RATE_24					24
#define VDP_FRAME_RATE_25					25
#define VDP_FRAME_RATE_30					30
#define VDP_FRAME_RATE_50					50
#define VDP_FRAME_RATE_60					60


#define VDP_BACKGROUND    2

/* log variables*/
typedef struct VDP_DBG_FRAME_LOG_T {
	u32  u4Pts;
	u32  u4Stc;
	u32  u4SoftPts;
	u8   ucVsyncNs;
	u8   VdpId;
} VDP_DBG_FRAME_LOG_T;

#define MAX_FRAME_LOG_COUNT         512

typedef struct {
	VID_PLA_PAUSE_MODE_T  ePauseMode;
} VDP_GLOBAL_SET_T;

extern VDP_DBG_FRAME_LOG_T   FrameLog[MAX_FRAME_LOG_COUNT];

/******************************************************************************
* VDP API
******************************************************************************/
#ifdef __linux__
/*TODO: use the newest vdp driver*/
extern void VDP_Init(void);
extern void VDP_Reset(u8 ucVdpId);
#else
extern void VDP_Init(bool fgHwReset);
extern void VDP_Reset(u8 ucVdpId, bool fgHwReset);
#endif
extern s32 VDP_Uninit(u32 u4Case);
extern void VDP_CheckFbgReady(void);
extern void VDP_FrcResetPort(u8 ucVdpId);

extern u32 VDP_SetEnable(u8 ucVdpId, VDP_ENABLE_T eEnable);
extern u32 VDP_SetMode(u8 ucVdpId, VID_PLA_MODE_T eMode);
extern u32 VDP_SetFmt(u8 ucVdpId, VID_PLA_DISP_FMT_T Fmt);
extern u32 VDP_SetEnhance(u8 ucVdpId, VID_PLA_ENHANCE_MODE_T Enhance, u8 ucEnhanceLevel);
extern u32 VDP_SetAlpha(u8 ucVdpId, u8 ucAlphaValue);
extern u32 VDP_SetInput(u8 ucVdpId, u8 ucEsId);
extern u32 VDP_SetOutput(u8 ucVdpId, u8 ucPmxId);
extern u32 VDP_SetRegion(u8 ucVdpId, u8 ucSrcFullRegion, VDP_REGION_T rSrcRegion
	, u8 ucOutFullRegion, VDP_REGION_T rOutRegion);
extern u32 VDP_SetSrcRegion(u8 ucVdpId, u8 bIsDelayed, VDP_REGION_T rSrcRegion);
extern u32 VDP_SetOutRegion(u8 ucVdpId, u8 bIsDelayed, VDP_REGION_T rOutRegion);
extern u32 VDP_SetBgRegion(u8 ucVdpId, VDP_REGION_T rBgRegion, SOURCE_ASPECT_RATIO_T Asp);
extern u32 VDP_SetBg(u8 ucVdpId, u32 u4BgColor);
extern u32 VDP_SetColorMode(u8 ucVdpId, DRV_PMX_COLOR_SPACE_T eColorMode);
extern u32 VDP_SetAddr(u8 ucVdpId, u32 u4AddrY, u32 u4AddrC);
extern u32 VDP_SetSrcSize(u8 ucVdpId, u32 u4SrcWidth, u32 u4SrcHeight);		/* for CLI test*/
extern u32 VDP_SetAcsMode(u8 ucVdpId, u8 ucAcsFrame, u8 ucSrcFrame
	, u8 ucAcsAuto, u8 ucPhyTop, u8 ucAcsTop);
extern u32 VDP_GetEnable(u8 ucVdpId, VDP_ENABLE_T *pEnable);
extern u32 VDP_GetMode(u8 ucVdpId, VID_PLA_MODE_T *peMode);
extern u32 VDP_GetModeWithProtection(u8 ucVdpId, u8 *pucMode);
extern u32 VDP_GetFmt(u8 ucVdpId, u8 *pucFmt);
extern u32 VDP_GetEnhance(u8 ucVdpId, VID_PLA_ENHANCE_MODE_T *pEnhance, u8 *pucEnhanceLevel);
extern u32 VDP_GetAlpha(u8 ucVdpId, u8 *pucAlphaValue);
extern u32 VDP_GetOutput(u8 ucVdpId, u8 *pucPmxId);
extern u32 VDP_GetSrcRegion(u8 ucVdpId, VDP_REGION_T *prSrcRegion);
extern u32 VDP_GetOutRegion(u8 ucVdpId, VDP_REGION_T *prOutRegion);
extern u32 VDP_SetFullScreen(u8 ucVdpId, bool b_full_screen_on);
extern u32 VDP_GetBgRegion(u8 ucVdpId, VDP_REGION_T *prRegion);
extern u32 VDP_GetBg(u8 ucVdpId, u32 *pu4BgColor);
extern u32 VDP_GetColorMode(u8 ucVdpId, DRV_PMX_COLOR_SPACE_T *eColorMode);
extern u32 VDP_GetAddr(u8 ucVdpId, u32 *pu4AddrY, u32 *pu4AddrC);
extern u32 VDP_GetSrcSize(u8 ucVdpId, u32 *pu4SrcWidth, u32 *pu4SrcHeight);
extern u32 VDP_Get_Sample_Aspect_Ratio(u8 ucVdpId, u32 *pu4ARW, u32 *pu4ARH);
extern u32 VDP_GetAcsMode(u8 ucVdpId, u8 *pucAcsFrame, u8 *pucSrcFrame
	, u8 *pucAcsAuto, u8 *pucPhyTop, u8 *pucAcsTop);
extern u32 VDP_GetInput(u8 ucVdpId, u8 *pucFbgId);
extern u32 VDP_IsPopPlane(u8 ucVdpId, u32 *pu4PopPlane, u32 *pu4ColourPrimary);

extern u32 VDP_SetPtsCb(u32 u4EsId, PTS_CB_FUNC pfCbPtsFun, u32 u4CbPts);

extern void VDP_SetZYXW(u32 u4Value);

#if UNIFORM_DRV_CALLBACK
extern s32 VDP_SetNotifyFunction(u8 ucVdpId, DRV_CB_REG_INFO_T *prNfyInfo);
#else
extern __s32 VDP_SetNotifyFunction(u8 ucVdpId, VDP_NFY_INFO_T *prNfyInfo);
#endif

extern void VDP_GetPicInfo(__u8 ucVdpId, VDP_PICTURE_INFO_T *prPicInfo);
extern void VDP_GetPBInfo(u8 ucVdpId, VID_PLA_PICTURE_INFO_T *prPicInfo);
extern u64 VDP_GetPts(u8 ucVdpId);
extern u32 VDP_CreateDispQueue(u8 ucVdpId, u16 u2CompType, u16 u2CompId);
extern u32 VDP_DestroyDispQueue(u8 ucVdpId);

extern s32 VDP_IsInitialized(void);
extern s32 i4VDP_Simp_Connect(u16 u2VdecCompType,
				u16 u2VdecCompId,
				u16 u2VdpCompType,
				u16 u2VdpCompId);

extern s32 i4VDP_Simp_Disconnect(u16 u2VdecCompType,
				   u16 u2VdecCompId,
				   u16 u2VdpCompType,
				   u16 u2VdpCompId);

extern u32 VDP_FlushFrameBuffer(u8 ucVdpId);
#ifdef ENUM_SRC_ASPECT_RATIO
extern u32 VDP_InjectPicture(u8 ucVdpId, u32 pAddrY, __u32 pAddrC
	, u16 Width, u16 Height, SOURCE_ASPECT_RATIO_T eAspectRatio);
#else
extern u32 VDP_InjectPicture(u8 ucVdpId, u32 pAddrY, __u32 pAddrC
	, u16 Width, u16 Height, __u8 AspectRatio);
#endif
extern u32 VDP_GetUiVqItemMinMaxDftCur(u8 u1VidPath, __u8 ucArgType
	, INT16 * pi2Min, INT16 * pi2Max, INT16 * pi2Dft, u8 *pucCur);
extern u32 VDP_SetLumaKey(s32 bIsOn, VID_PLA_LUMA_T Key);
extern u32 VDP_SetTvFormat(PMX_RESOLUTION_MODE_T eFmt, u8 bIsResetHw);
extern u32 VDP_EnableCC(s32 bIsOn);
extern u32 VDP_GetMainVdpId(void);
extern void VDP_GlobalOutputChange(void);
extern void VDP_CheckBufferInvalid(u8 FBGId, u8 FBId);
extern u32 VDP_GetCurrPictureInfo(u8 ucVdpId, u64 *pPts, u8 *pbIsOpenB);
extern u32 VDP_CapturePicture(u8 ucVdpId, VDP_CMD_CAPTURE_T *pCapInfo);
extern u32 VDP_CaptureAbort(u8 ucVdpId);
extern u8 VDP_GetHwEnable(u8 ucVdpId);
#if (CONFIG_DRV_NR_SUPPORT)
extern u32 VDP_SetNRStrength(u32 u4Strength);
extern __u32 VDP_SetNRDBGLevel(u32 u4DBGLevel);
extern u32 VDP_SetNRMode(u32 u4NRMode);
extern u32 VDP_SetSharpStrength(u32 u4Strength);
#endif


u32 VDP_GetDispQueue(u32 ucVdpId);
u32 VDP_GetUnderRunNum(u32 ucVdpId);
extern u32 VDP_SetPauseMode(VID_PLA_PAUSE_MODE_T ePauseMode);
extern u32 VDP_IgnoreCPSIBCmd(bool fgVdpIgnoreCPSIBCmd);
extern u32 VDP_ClearCPSInfo(bool fgIsScreenSaver);

extern u32 VDP_SetVideoBlack(u32 ucVdpId, u8 VideoY, u8 VideoCb, u8 VideoCr);

#endif /* _DRV_VDP_H_ */



