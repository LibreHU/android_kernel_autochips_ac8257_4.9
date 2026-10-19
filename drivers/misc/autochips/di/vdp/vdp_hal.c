/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors. Without
 * the prior written permission of AutoChips inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of AutoChips Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * AutoChips Inc. (C) 2018. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AUTOCHIPS SOFTWARE")
 * RECEIVED FROM AUTOCHIPS AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. AUTOCHIPS EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES AUTOCHIPS PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE AUTOCHIPS SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN AUTOCHIPS
 * SOFTWARE. AUTOCHIPS SHALL ALSO NOT BE RESPONSIBLE FOR ANY AUTOCHIPS SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AUTOCHIPS'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE AUTOCHIPS SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT AUTOCHIPS'S OPTION, TO REVISE OR REPLACE THE
 * AUTOCHIPS SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO AUTOCHIPS FOR SUCH AUTOCHIPS SOFTWARE AT ISSUE.
 */


/*****************************************************************************
*  Video Plane: Interface
*****************************************************************************/

#ifndef _VDP_HAL_C_
#define _VDP_HAL_C_

/*#include "strict_warning.h"*/
#ifndef ATC_ARM2_LK_SUPPORT
#include <linux/module.h>
#include "pmx_hal.h"
#include "display_inc.h"

#else
#include "platform/mt_typedefs.h"
#include "sys/types.h"
#include "di_types.h"

#include "assert.h"
#include "pmx_hal.h"
#include "display_inc.h"
#endif
#include "vdp_hw.h"
#include "vdp_hal.h"
#include "log.h"
#include "pmx_hw.h"

//#include "x_lint.h"
//#include "x_hal_ic.h"

#include "drv_common.h"
#include "drv_config.h"
#include "chip_ver.h"
#include "bringup.h"
//#include "x_bim.h"
//#include "x_os.h"
//#include "x_assert.h"
#include <ac8257_addr_utils.h>

/******************************************************************************
* Local variable
******************************************************************************/
#define DEINTERLACE_DEFAULT_START_ADDRESS	0x50
#define DEINTERLACE_DEFAULT_SIZE			28

u32 _u4Tm070ddhg;


extern unsigned int fmtr_irq;
#ifndef ATC_ARM2_LK_SUPPORT
extern void __iomem* IO_BASE_BRINGUP;
#else
extern unsigned long IO_BASE_BRINGUP;
#endif

u32 deinterlace_default[] = { /* start from 0x70042150*/
		0x00010000, 0x000007ff, 0x10000000, 0x10000000,
		0x00000000, 0x80018700, 0x00000c0f, 0x05110000,
		0x00001000, 0x000001f1, 0x00010037, 0x08000000,
		0x0006ba80, 0x00091a00, 0x4a0186a0, 0x1b011940,
		0x0000c350, 0x00012003, 0x3202a02a, 0x1600c002,
		0x6401c01c, 0x32051004, 0x01436004, 0x02002604,
		0x0332323c, 0x00320814, 0x00320432, 0x00005537,
};

HAL_VDO_UNION_T _rVdp1SwReg;
HAL_VDO_UNION_T _rVdp2SwReg;
u8 _rVdp1RegMode[VDP_HAL_VDO_REG_NUM];
u8 _rVdp2RegMode[VDP_HAL_VDO_REG_NUM];


HAL_VDO_APQ_UNION_T _rVdp1ApqSwReg;
HAL_VDO_APQ_UNION_T _rVdp2ApqSwReg;
u8 _rVdp1ApqRegMode[HAL_VDO_APQ_REG_NUM];
u8 _rVdp2ApqRegMode[HAL_VDO_APQ_REG_NUM];

static VDP_REGION_T _rVdp1SrcRegion = { 0, 0, 2, 1 };
static VDP_REGION_T _rVdp2SrcRegion = { 0, 0, 2, 1 };
static VDP_REGION_T _rVdp1OutRegion = { 0, 0, 2, 1 };
static VDP_REGION_T _rVdp2OutRegion = { 0, 0, 2, 1 };

static bool _fgVdp1ResetInVSync = FALSE;
static bool _fgVdp2ResetInVSync = FALSE;


#if 0
/*not use*/
static VDP_ACT_START_POS _tVdp1ActPos_TM[] = { /*for TM070ddhg*/
	{RES_480P,			 0x068, 0x035, 0x035},/*verified*/
	{RES_576P,			 0x074, 0x030, 0x030},
	{RES_480P_800,		 0x066, 0x02C, 0x02C},
	{RES_600P_800,		 0x064, 0x02C, 0x02C},
	{RES_600P_1024, 	 0x09d, 0x024, 0x024},/*verified*/
	{RES_768P_1024, 	 0x063, 0x01A, 0x01A},
	{RES_720P_1280, 	 0x050, 0x013, 0x013},
	{RES_800P_1280, 	 0x090, 0x01C, 0x01C},
};
#endif
static VDP_ACT_START_POS _tVdp1ActPos[] = {
	{RES_480P,			 0x068, 0x02C, 0x02C},
	{RES_576P,			 0x074, 0x030, 0x030},
	{RES_480P_800,		 0x066, 0x02C, 0x02C},
	{RES_600P_800,		 0x064, 0x02C, 0x02C},
	{RES_600P_1024, 	 0x105, 0x018, 0x018},
	{RES_768P_1024, 	 0x063, 0x01A, 0x01A},
	{RES_720P_1280, 	 0x050, 0x013, 0x013},
	{RES_800P_1280, 	 0x090, 0x01C, 0x01C},
};

/*static VDP_ACT_START_POS _tVdp2ActPos[] = {
	{RES_480P,			0x06D, 0x02C, 0x02C},
	{RES_576P,			0x079, 0x032, 0x032},
};*/

void vVdpHalReset(u8 ucVdpId)
{
	if (ucVdpId == VDP_1) {
		_fgVdp1ResetInVSync = TRUE;
	} else {
		_fgVdp2ResetInVSync = TRUE;
	}
}

void vVdpHalInit(u8 ucVdpId, bool fgHwReset)
{

}
#ifndef ATC_ARM2_LK_SUPPORT
EXPORT_SYMBOL(vVdpHalInit);
#endif

void vVdpHalSetSrcSize(u8 ucVdpId, u32 Width, u32 Height)
{
	/*HAL_VDO_UNION_T *prVdpSwReg;
	u8  *prVdpRegMode;
	u8  *prPmxDispRegMode;
	PMX_DISP_REG_UNION_PTR_T prPmxDispReg;

	GET_VDP_PTR(ucVdpId, prVdpSwReg, prVdpRegMode);
	GET_PMX_DISP_PTR(ucVdpId, prPmxDispReg, prPmxDispRegMode);*/

	//prVdpSwReg->rField.u4Hblock = ((Width + 7) >> 3);
	VDPWriteREGMsk(0x10, (Width + 7) >> 3, 0xFF);
	//prVdpSwReg->rField.u4Pic_Height = Height;
	VDPWriteREGMsk(0x10, (Height << 16), 0x7FF0000);

	//prPmxDispReg.pDispMain->rField.u2PXLLEN = Width;
	FmtWriteREGMsk(0x9C, Width, 0x1FFF);
	/*VDO_LOG(VDO_LOG_LVL_REGRW, "vVdpHalSetSrcSize: ucVdpId %d, 0x10 %x\r\n"
		, (int)ucVdpId, (unsigned int)prVdpSwReg->au4Reg[(0x10 / 4)]);

	prVdpRegMode[(0x10 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prVdpRegMode[(0xE0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0x9C / 4)] |= PMX_HAL_REG_MODE_WRITE;*/
}

void vVdpHalSetSrcRegion(u8 ucVdpId, u32 X, u32 Y, u32 Width, u32 Height)
{
	if (ucVdpId == VDP_1) {
		_rVdp1SrcRegion.X = X;
		_rVdp1SrcRegion.Y = Y;
		_rVdp1SrcRegion.Width = Width;
		_rVdp1SrcRegion.Height = Height;
	}

	if (ucVdpId == VDP_2) {
		_rVdp2SrcRegion.X = X;
		_rVdp2SrcRegion.Y = Y;
		_rVdp2SrcRegion.Width = Width;
		_rVdp2SrcRegion.Height = Height;
	}
}

void vVdpHalSetOutRegion(u8 ucVdpId, u32 X, u32 Y, u32 Width, u32 Height)
{
	if (ucVdpId == VDP_1) {
		_rVdp1OutRegion.X = X;
		_rVdp1OutRegion.Y = Y;
		_rVdp1OutRegion.Width = Width;
		_rVdp1OutRegion.Height = Height;
	}

	if (ucVdpId == VDP_2) {
		_rVdp2OutRegion.X = X;
		_rVdp2OutRegion.Y = Y;
		_rVdp2OutRegion.Width = Width;
		_rVdp2OutRegion.Height = Height;
	}
}
void vVdpHalSetInterlaceSourceEn(bool val)
{
#ifndef ATC_ARM2_LK_SUPPORT
	/*support  DI  input interlace source*/
	VDO8TAPWriteREG(0xA0, val << 31);
#endif
}

void vVdpHalSetMode(u8 ucVdpId, u32 u4Mode)
{

	HAL_VDO_UNION_T *prVdpSwReg;
	u8	*prVdpRegMode;
	PMX_DISP_REG_UNION_PTR_T prPmxDispReg;
	u8	*prPmxDispRegMode;
	VDP_REGION_T	*prSrcRegion;
	VDP_REGION_T	*prOutRegion;
	u32	u4Cnt, u4PosX = 0, u4PosY = 0, u4PosEY = 0;
	u32	u4VdpOutWidth, u4VdpOutHeight, u4HFactor;
	unsigned int SrcWidth, OutWidth;
	unsigned int u4HFactor_inverse = 0, adj_hact_vdo = 0, adj_hact_hd = 0;

	GET_VDP_PTR_REGION(ucVdpId, prVdpSwReg, prVdpRegMode, prSrcRegion, prOutRegion);
	GET_PMX_DISP_PTR(ucVdpId, prPmxDispReg, prPmxDispRegMode);
	(void)prPmxDispRegMode;
	(void)prVdpSwReg;


	for (u4Cnt = 0; u4Cnt < sizeof(_tVdp1ActPos) / sizeof(_tVdp1ActPos[0]); u4Cnt++) {
		if (_tVdp1ActPos[u4Cnt].u4Mode == u4Mode) {
			u4PosX = _tVdp1ActPos[u4Cnt].u4PosX;
			u4PosY = _tVdp1ActPos[u4Cnt].u4PosY;
			u4PosEY = _tVdp1ActPos[u4Cnt].u4PosEY;
			break;
		}
	}



	VDO_LOG(VDO_LOG_LVL_REGRW, "vVdpHalSetMode: ucVdpId %d, u4PosX %d, u4PosY %d, u4PosEY %d u4Cnt %d\r\n",
		(int)ucVdpId, (int)u4PosX, (int)u4PosY, (int)u4PosEY, (int)u4Cnt);

	switch (u4Mode) {
		case RES_600P_1024:
		case RES_600P_1024_50HZ:
			u4VdpOutWidth = 1024;
			u4VdpOutHeight = 600;
			break;

		default:
			u4VdpOutWidth = 720;
			u4VdpOutHeight = 480;
	}

	//prVdpSwReg->rField.fgVdoEn = 1;
	//prVdpSwReg->rField.u4P_Skip = prSrcRegion->X & ~0x1;
	VDPWriteREGMsk(0x1C, 1, 0x1);
	VDPWriteREGMsk(0x14, ((prSrcRegion->X & ~0x1) << 16), 0x7FF0000);
	//prVdpSwReg->rField.u4Total_Mbr = 0x1E;
	VDPWriteREGMsk(0x18, (0x1E << 24), 0x7F000000);
	//prVdpSwReg->rField.u4Max_Mbr = 0x1D;
	VDPWriteREGMsk(0x18, (0x1E << 16), 0x7F0000);

	//prVdpSwReg->rField.fgSP_LD = 1;
	VDPWriteREGMsk(0x64, (1 << 15), 0x8000);

	//u4Vscale indicates the vertical scale factor to display a picture.The scale ratio is 4096/u4Vscale in interlace mode,
	//or 2048/u4Vscale in progressive mode.
	//prVdpSwReg->rField.u4Vscale = (2048 * (prSrcRegion->Height - 1) + (prOutRegion->Height - 1) / 2) / (prOutRegion->Height - 1);
	VDPWriteREGMsk(0x14, (2048 * (prSrcRegion->Height - 1) + (prOutRegion->Height - 1) / 2) / (prOutRegion->Height - 1), 0xFFFF);

	VDO_LOG(VDO_LOG_LVL_REGRW, "vVdpHalSetMode: ucVdpId %d, u4VdpOutWidth %d, u4VdpOutHeight %d\r\n"
		, (int)ucVdpId, (int)u4VdpOutWidth, (int)u4VdpOutHeight);
	//VDO_LOG(VDO_LOG_LVL_REGRW, "vVdpHalSetMode: ucVdpId %d, _u4LCDWidth %d, _u4LCDHeight %d\r\n"
		//, (int)ucVdpId, (int)_u4LCDWidth, (int)_u4LCDHeight);
	VDO_LOG(VDO_LOG_LVL_REGRW, "vVdpHalSetMode: ucVdpId %d, prOutRegion->Width %d, prOutRegion->Height %d\r\n",
		(int)ucVdpId, (int)prOutRegion->Width, (int)prOutRegion->Height);
	VDO_LOG(VDO_LOG_LVL_REGRW, "vVdpHalSetMode: ucVdpId %d, prSrcRegion->Width %d, prSrcRegion->Height %d\r\n",
		(int)ucVdpId, (int)prSrcRegion->Width, (int)prSrcRegion->Height);

#if 0
	if (prSrcRegion->Width > 720) {
		/*Set start line and start sub-line*/
		prVdpSwReg->au4Reg[0x20 / 4] = 0x00000000;
		prVdpSwReg->au4Reg[0x24 / 4] = 0x00000000;
		prVdpSwReg->au4Reg[0x28 / 4] = 0x00000000;
		prVdpSwReg->au4Reg[0x2C / 4] = 0x00000000;
		prVdpSwReg->au4Reg[0x50 / 4] = 0x00000000;
		prVdpSwReg->au4Reg[0x54 / 4] = 0x00000000;
	} else {
#endif
	if (prPmxDispReg.pDispMain->rField.fgPRGS) {
		/*Set start line and start sub-line*/
		unsigned int u4YTTPos, u4YBTPos, u4YTBPos, u4YBBPos;
		unsigned int u4CTTPos, u4CBTPos, u4CTBPos, u4CBBPos;
		unsigned int u4YSLTTini, u4YSLBTini;
		unsigned int u4YSSLTTini, u4YSSLBTini;
		unsigned int u4CSLTTini, u4CSLBTini;
		unsigned int u4CSSLTTini, u4CSSLBTini;
		//bool fgYUV422 = prVdpSwReg->rField.fgYUV422;
		bool fgYUV422 = VDPReadREGFEILD(0x30, 21, 0x200000);
		//unsigned int u4VScaleHW = prVdpSwReg->rField.u4Vscale;
		unsigned int u4VScaleHW = VDPReadREGFEILD(0x14, 0, 0xFFFF);
		unsigned int SrcY = prSrcRegion->Y;
		// fld without filter
		u4YSLTTini = 0x7FA;
		u4YSSLTTini = 0;
		u4YSLBTini = 0x7F9;
		u4YSSLBTini = 0x80;
		u4CSLTTini = fgYUV422? u4YSLTTini : 0x7FC;
		u4CSSLTTini = fgYUV422? u4YSSLTTini : 0xE0;
		u4CSLBTini = fgYUV422? u4YSLBTini : 0x7FC;
		u4CSSLBTini = fgYUV422? u4YSSLBTini : 0x60;

		u4YTTPos = SrcY * 128 + u4YSSLTTini;
		u4YBTPos = SrcY * 128 + u4YSSLBTini;
		u4YTBPos = u4YTTPos + ((u4VScaleHW) / 16);
		u4YBBPos = u4YBTPos + ((u4VScaleHW) / 16);

		//prVdpSwReg->rField.u4YSLTT = u4YSLTTini + ((u4YTTPos / 256) * 2);
		VDPWriteREGMsk(0x20, u4YSLTTini + ((u4YTTPos / 256) * 2), 0x7FF);
		VDPWriteREGMsk(0x28, (u4YTTPos % 256), 0xFF);
		VDPWriteREGMsk(0x50, u4YSLBTini, 0x7FF);
		VDPWriteREGMsk(0x28, ((u4YBTPos % 256) << 16), 0xFF0000);
		//prVdpSwReg->rField.u4YSSLTT = (u4YTTPos % 256);
		//prVdpSwReg->rField.u4YSLBT = u4YSLBTini /*+ ((u4YBTPos / 256) * 2)*/;
		//prVdpSwReg->rField.u4YSSLBT = (u4YBTPos % 256);

		VDPWriteREGMsk(0x50, ((u4YSLTTini + ((u4YTBPos / 256) * 2)) << 16), 0x7FF0000);
		VDPWriteREGMsk(0x28, ((u4YTBPos % 256) << 24), 0xFF000000);
		VDPWriteREGMsk(0x20, ((u4YSLBTini + ((u4YBBPos / 256) * 2)) << 16), 0x7FF0000);
		VDPWriteREGMsk(0x28, ((u4YBBPos % 256) << 8), 0xFF00);

		/*prVdpSwReg->rField.u4YSLTB = u4YSLTTini + ((u4YTBPos / 256) * 2);
		prVdpSwReg->rField.u4YSSLTB = (u4YTBPos % 256);
		prVdpSwReg->rField.u4YSLBB = u4YSLBTini + ((u4YBBPos / 256) * 2);
		prVdpSwReg->rField.u4YSSLBB = (u4YBBPos % 256);*/

		if (fgYUV422) //422
		{
			u4CTTPos = (SrcY * 128) + u4CSSLTTini;
			u4CBTPos = (SrcY * 128) + u4CSSLBTini;
			u4CTBPos = u4CTTPos + ((u4VScaleHW) / 16);
			u4CBBPos = u4CBTPos + ((u4VScaleHW) / 16);
		}
		else
		{//420
			u4CTTPos = (SrcY * 128 / 2) + u4CSSLTTini;
			u4CBTPos = (SrcY * 128 / 2) + u4CSSLBTini;
			u4CTBPos = u4CTTPos + ((u4VScaleHW) / 32);
			u4CBBPos = u4CBTPos + ((u4VScaleHW) / 32);
		}
		VDPWriteREGMsk(0x24, u4CSLTTini + ((u4CTTPos / 256) * 2), 0x7FF);
		VDPWriteREGMsk(0x2C, (u4CTTPos % 256), 0xFF);
		VDPWriteREGMsk(0x54, u4CSLBTini, 0x7FF);
		VDPWriteREGMsk(0x2C, ((u4CBTPos % 256) << 16), 0xFF0000);

		VDPWriteREGMsk(0x54, ((u4CSLTTini + ((u4CTBPos / 256) * 2)) << 16), 0x7FF0000);
		VDPWriteREGMsk(0x2C, ((u4CTBPos % 256) << 24), 0xFF000000);
		VDPWriteREGMsk(0x24, ((u4CSLBTini + ((u4CBBPos / 256) * 2)) << 16), 0x7FF0000);
		VDPWriteREGMsk(0x2C, ((u4CBBPos % 256) << 8), 0xFF00);
		#if 0
		prVdpSwReg->rField.u4CSLTT = u4CSLTTini + ((u4CTTPos / 256) * 2);
		prVdpSwReg->rField.u4CSSLTT = (u4CTTPos % 256);
		prVdpSwReg->rField.u4CSLBT = u4CSLBTini /*+ ((u4CBTPos / 256) * 2)*/;
		prVdpSwReg->rField.u4CSSLBT = (u4CBTPos % 256);

		prVdpSwReg->rField.u4CSLTB = u4CSLTTini + ((u4CTBPos / 256) * 2);
		prVdpSwReg->rField.u4CSSLTB = (u4CTBPos % 256);
		prVdpSwReg->rField.u4CSLBB = u4CSLBTini + ((u4CBBPos / 256) * 2);
		prVdpSwReg->rField.u4CSSLBB = (u4CBBPos % 256);
		#endif
		} else {
			/*Set start line and start sub-line for interlace mode*/
			//todo
		}
		#if 0
	}
		#endif

		//calclate HSFACTOR
		SrcWidth = prSrcRegion->Width;
		OutWidth = prOutRegion->Width;

		if(SrcWidth > OutWidth) {
			//prVdpSwReg->rField.fgXHalf = 1;
			VDPWriteREGMsk(0x1C, (1 << 11), 0x800);
			SrcWidth >>= 1;
		}
		else {
			//prVdpSwReg->rField.fgXHalf = 0;
			VDPWriteREGMsk(0x1C, (0 << 11), 0x800);
		}
		u4HFactor = (HSCALE_UNIT * (SrcWidth - 1) + (OutWidth - 1) / 2) / (OutWidth - 1);

		#if 0
		if (prPmxDispReg.pDispMain->rField.u2HSFACTOR == HSCALE_UNIT)		 // 1:1 scaler, turn on linear filter
		{
			prPmxDispReg.pDispMain->rField.fgHSLR = 1; // use linear filter
			prPmxDispReg.pDispMain->rField.fgHD_C_FIR_EN = 0; // use linear filter
		}
		else
		{
			prPmxDispReg.pDispMain->rField.fgHSLR = 0; // use fir
			prPmxDispReg.pDispMain->rField.fgHD_C_FIR_EN = 1; // use fir
		}
		#endif

		prPmxDispReg.pDispMain->rField.u2HACTBGN = u4PosX + prOutRegion->X;
		prPmxDispReg.pDispMain->rField.u2HACTEND = u4PosX + prOutRegion->X + prOutRegion->Width - 1;
		prPmxDispReg.pDispMain->rField.u2VOACTBGN = u4PosY + prOutRegion->Y;
		prPmxDispReg.pDispMain->rField.u2VOACTEND = u4PosY + prOutRegion->Y + prOutRegion->Height - 1;
		prPmxDispReg.pDispMain->rField.u2VEACTBGN = u4PosEY + prOutRegion->Y;
		prPmxDispReg.pDispMain->rField.u2VEACTEND = u4PosEY + prOutRegion->Y + prOutRegion->Height - 1;

		prPmxDispReg.pDispMain->rField.fgHSON = 1;

		//setting fmt 6c~7c, c0, e8, ec,c0
		/*
		prPmxDispReg.pDispMain->au4Reg[(0x6C / 4)] = 0x10000820; //hard code--why?
		prPmxDispReg.pDispMain->rField.u2HDWN_HBGN = u4PosX + 5; //70
		prPmxDispReg.pDispMain->rField.u2HDWN_HEND = prPmxDispReg.pDispMain->rField.u2HDWN_HBGN + u4VdpOutWidth; //70
		prPmxDispReg.pDispMain->rField.u2HDWN_VOBGN = u4PosY; //74
		prPmxDispReg.pDispMain->rField.u2HDWN_VOEND = prPmxDispReg.pDispMain->rField.u2HDWN_VOBGN + u4VdpOutHeight + 1; //74
		prPmxDispReg.pDispMain->rField.u2HDWN_VEBGN = prPmxDispReg.pDispMain->rField.u2HDWN_VOBGN; //78
		prPmxDispReg.pDispMain->rField.u2HDWN_VEEND = prPmxDispReg.pDispMain->rField.u2HDWN_VOEND; //78
		//hardcode for 1024*600 panel
	   prPmxDispReg.pDispMain->au4Reg[(0x7C / 4)] = 0x01402940;  //hard code--why?*/
#if 0//move to arm2 for pp&vcp
		prPmxDispReg.pDispMain->au4Reg[(0xE8 / 4)] = 0x00040a7c;  //h/v sync delay
		prPmxDispReg.pDispMain->au4Reg[(0xEC / 4)] = 0x40020536;
#endif
		prPmxDispReg.pDispMain->au4Reg[(0xC0 / 4)] = 0x00b104b0; //hard code--why?


		if(u4HFactor > 0x1000){
			prPmxDispReg.pDispMain->rField.u2HSFACTOR = HSCALE_UNIT;
			prPmxDispReg.pDispMain->rField.fgHSON = 1;
			prPmxDispReg.pDispMain->au4Reg[(0x6C / 4)] = ((u4HFactor << 16)| 0x821);

			//use acc.
			u4HFactor_inverse = (u4HFactor*1000)/0x1000;
			u4HFactor_inverse = (0x1000 * 1000) / u4HFactor_inverse;
			prPmxDispReg.pDispMain->rField.fgDownScaleMode = 1;
			prPmxDispReg.pDispMain->rField.u4ScalerFactor = u4HFactor_inverse;

			adj_hact_vdo = prPmxDispReg.pDispMain->rField.u2HACTBGN;
			adj_hact_hd = prPmxDispReg.pDispMain->rField.u2HDWN_HBGN;

			adj_hact_vdo = SrcWidth +adj_hact_vdo +1;
			adj_hact_hd = SrcWidth + adj_hact_hd +1;

			prPmxDispReg.pDispMain->rField.u2HACTEND = adj_hact_vdo;
			prPmxDispReg.pDispMain->rField.u2HDWN_HEND = adj_hact_hd;

		}else{
			prPmxDispReg.pDispMain->rField.u2HSFACTOR = u4HFactor;
			//prVdpSwReg->rField.fgXHalf = 0;
			prPmxDispReg.pDispMain->rField.fgHSON = 1;

			if(u4HFactor == 0x1000){ //fixme;
				//use acc.
				u4HFactor_inverse = (u4HFactor*1000)/0x1000;
				u4HFactor_inverse = (0x1000 * 1000) / u4HFactor_inverse;
				prPmxDispReg.pDispMain->rField.fgDownScaleMode = 1;
				prPmxDispReg.pDispMain->rField.u4ScalerFactor = u4HFactor_inverse;
			}

		}
		//setting fmt 48 : H Filter
		//todo


		if (prPmxDispReg.pDispMain->rField.u2HSFACTOR == HSCALE_UNIT)		 // 1:1 scaler, turn on linear filter
		{
				prPmxDispReg.pDispMain->rField.fgHSLR = 1; // use linear filter
				prPmxDispReg.pDispMain->rField.fgHD_C_FIR_EN = 0; // use linear filter
		}
		else
		{
				prPmxDispReg.pDispMain->rField.fgHSLR = 0; // use fir
				prPmxDispReg.pDispMain->rField.fgHD_C_FIR_EN = 1; // use fir
		}

#if 0 //should config fgFrm??
		//tmp solution:all be frame mode,no deinterlace
		prVdpSwReg->rField.fgYFrm = 1;
		prVdpSwReg->rField.fgCFrm = 1;
		prVdpRegMode[(0x30 / 4)] |= PMX_HAL_REG_MODE_WRITE;
#endif

	/*prPmxDispRegMode[(0x6C / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0x70 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0x74 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0x78 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0x7C / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0xA0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0xA4 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0xA8 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0xB0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0xC0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0xCC / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0xE4 / 4)] |= PMX_HAL_REG_MODE_WRITE;*/
#if 0//move to arm2 for pp&vcp
	prPmxDispRegMode[(0xE8 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prPmxDispRegMode[(0xEC / 4)] |= PMX_HAL_REG_MODE_WRITE;
#endif
	/*prVdpRegMode[(0x14 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prVdpRegMode[(0x18 / 4)] |= PMX_HAL_REG_MODE_WRITE;*/
	prVdpRegMode[(0x1C / 4)] |= PMX_HAL_REG_MODE_WRITE;
/*	prVdpRegMode[(0x20 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prVdpRegMode[(0x24 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prVdpRegMode[(0x28 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prVdpRegMode[(0x2C / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prVdpRegMode[(0x34 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prVdpRegMode[(0x50 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prVdpRegMode[(0x54 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	prVdpRegMode[(0x64 / 4)] |= PMX_HAL_REG_MODE_WRITE;*/
}

void vVdpHalSetFifo(u8 ucVdpId)
{
	HAL_VDO_UNION_T *prVdpSwReg;
	u8 *prVdpRegMode;

	GET_VDP_PTR(ucVdpId, prVdpSwReg, prVdpRegMode);
	(void)prVdpSwReg;

	//if (prVdpSwReg->rField.fgHD_EN) {
	if (VDPReadREGFEILD(0xE0, 24, 0x1000000)) {
		//prVdpSwReg->au4Reg[0x34 / 4] = 0x3cc83c3c;
		VDPWriteREGMsk(0x34, 0x3cc83c3c, 0xFFFFFFFF);
	} else {
		//prVdpSwReg->au4Reg[0x34 / 4] = 0x20D82020;
		VDPWriteREGMsk(0x34, 0x20D82020, 0xFFFFFFFF);
	}

	prVdpRegMode[(0x34 / 4)] |= PMX_HAL_REG_MODE_WRITE;
}

void vVdpHalSetYuv422(u8 ucVdpId, bool fgYUV422)
{
	HAL_VDO_UNION_T *prVdpSwReg;
	u8 *prVdpRegMode;

	GET_VDP_PTR(ucVdpId, prVdpSwReg, prVdpRegMode);
	(void)prVdpSwReg;

	//prVdpSwReg->rField.fgYUV422 = fgYUV422;
	VDPWriteREGMsk(0x30, fgYUV422 << 21, 0x200000);
	prVdpRegMode[(0x30 / 4)] |= PMX_HAL_REG_MODE_WRITE;
}

void vVdpHalSetScanLine(u8 ucVdpId, bool fgScLineEnable)
{
	HAL_VDO_UNION_T *prVdpSwReg;
	u8 *prVdpRegMode;

	GET_VDP_PTR(ucVdpId, prVdpSwReg, prVdpRegMode);
	(void)prVdpSwReg;

	//prVdpSwReg->rField.fgSLE = fgScLineEnable;
	VDPWriteREGMsk(0xE0, fgScLineEnable << 23, 0x800000);
	prVdpRegMode[(0xE0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
}

void vVdpHalSetHFilterMode(u8 ucVdpId, bool fgFIRMode)
{

}

void vVdpHalSetYBufPtr(u8 ucVdpId, u32 u4AddrY, u32 u4AddrC)
{
	if (0)
	{
		VDO_LOG(VDO_LOG_LVL_ERR, "vVdpHalSetVdoPtr: address not align, u4AddrY = 0x%x, u4AddrC = 0x%x\r\n",
			(unsigned int)u4AddrY, (unsigned int)u4AddrC);
		//ASSERT(0);
	} else {
		HAL_VDO_UNION_T *prVdpSwReg;
		u8 *prVdpRegMode;

		GET_VDP_PTR(ucVdpId, prVdpSwReg, prVdpRegMode);
		(void)prVdpSwReg;
		//u4AddrY = (0x18000000);
		//u4AddrC=	(0x18100000);
		VDPWriteREGMsk(0x00, u4AddrY >> 2, 0x3FFFFFFF);
		VDPWriteREGMsk(0x08, u4AddrY >> 2, 0x3FFFFFFF);
		VDPWriteREGMsk(0x80, u4AddrY >> 2, 0x3FFFFFFF);
		VDPWriteREGMsk(0x84, u4AddrY >> 2, 0x3FFFFFFF);
		VDPWriteREGMsk(0xEC, u4AddrC >> 2, 0x3FFFFFFF);
		VDPWriteREGMsk(0x04, u4AddrC >> 2, 0x3FFFFFFF);
		VDPWriteREGMsk(0x0C, u4AddrC >> 2, 0x3FFFFFFF);
		VDPWriteREGMsk(0xFC, u4AddrC >> 2, 0x3FFFFFFF);
		/*prVdpSwReg->rField.u4YAddrY = u4AddrY >> 2;
		prVdpSwReg->rField.u4XAddrY = u4AddrY >> 2;
		prVdpSwReg->rField.u4WAddrY = u4AddrY >> 2;
		prVdpSwReg->rField.u4ZAddrY = u4AddrY >> 2;

		prVdpSwReg->rField.u4PTR_AF_Y = u4AddrC >> 2;
		prVdpSwReg->rField.u4YAddrC = u4AddrC >> 2;
		prVdpSwReg->rField.u4XAddrC = u4AddrC >> 2;
		prVdpSwReg->rField.u4ZAddrC = u4AddrC >> 2;*/
		prVdpRegMode[(0x00 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x08 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x80 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x84 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0xEC / 4)] |= PMX_HAL_REG_MODE_WRITE;

		prVdpRegMode[(0x04 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x0C / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0xFC / 4)] |= PMX_HAL_REG_MODE_WRITE;
	}
}
#ifndef ATC_ARM2_LK_SUPPORT
#if 0
extern char _stext[];
extern char _etext[];

static void checkAddr (u32 addr)
{
	u32 start_addr = virt_to_phys(_stext);
	u32 end_addr = virt_to_phys(_etext);

	if((addr >= start_addr) && (addr <= end_addr)) {
		VDO_LOG(VDO_LOG_LVL_ERR, "checkAddr: Addr = 0x%x\r\n",
			(unsigned int)addr);
		//TODO:
		panic("[MEM_INVALID]di write address di_error ");
	}
}

static void cmpAddr(u32 vdpHwAddr, u32 vdpSwAddr)
{
	if (vdpHwAddr != vdpSwAddr) {
		VDO_LOG(VDO_LOG_LVL_ERR, "vdpHwAddr: vdpHwAddr = 0x%x, vdpSwAddr = 0x%x\r\n",
			(unsigned int)vdpHwAddr, (unsigned int)vdpSwAddr);
		//TODO:
		panic("[MEM_INVALID]di write address di_error");
	}
}
#endif
#endif
void vVdpHalSetDeintWXYZ(u8 ucVdpId, const u32 *pu4AddrY, const u32 *pu4AddrC)
{
	if ((pu4AddrY) && (pu4AddrC)) {
		//HAL_VDO_UNION_T *prVdpSwReg;
		//u8 *prVdpRegMode;
		volatile HAL_VDO_UNION_T *prVdpHwReg;
		//u8 *pVdpRegMode;

		//GET_VDP_PTR(ucVdpId, prVdpSwReg, pVdpRegMode);
		GET_VDP_HW_PTR(ucVdpId, prVdpHwReg);

		prVdpHwReg->rField.u4WAddrY = (u32)(pu4AddrY[0] >> 2);
		prVdpHwReg->rField.u4XAddrY = (u32)(pu4AddrY[1] >> 2);
		prVdpHwReg->rField.u4YAddrY = (u32)(pu4AddrY[2] >> 2);
		prVdpHwReg->rField.u4ZAddrY = (u32)(pu4AddrY[3] >> 2);

		prVdpHwReg->rField.u4PTR_AF_Y = (u32)(pu4AddrC[0] >> 2);
		prVdpHwReg->rField.u4XAddrC = (u32)(pu4AddrC[1] >> 2);
		prVdpHwReg->rField.u4YAddrC = (u32)(pu4AddrC[2] >> 2);
		prVdpHwReg->rField.u4ZAddrC = (u32)(pu4AddrC[3] >> 2);

#ifndef ATC_ARM2_LK_SUPPORT
#if 0
		u32 wAddr = prVdpHwReg->rField.u4WAddrY << 2;
		checkAddr(wAddr);
		cmpAddr(wAddr,pu4AddrY[0]);

		u32 xAddr = prVdpHwReg->rField.u4XAddrY << 2;
		checkAddr(xAddr);
		cmpAddr(xAddr,pu4AddrY[1]);

		u32 yAddr = prVdpHwReg->rField.u4YAddrY << 2;
		checkAddr(yAddr);
		cmpAddr(yAddr,pu4AddrY[2]);

		u32 zAddr = prVdpHwReg->rField.u4ZAddrY << 2;
		checkAddr(zAddr);
		cmpAddr(zAddr,pu4AddrY[3]);

		u32 wCAddr = prVdpHwReg->rField.u4PTR_AF_Y << 2;
		checkAddr(wCAddr);
		cmpAddr(wCAddr,pu4AddrC[0]);

		u32 xCAddr = prVdpHwReg->rField.u4XAddrC << 2;
		checkAddr(xCAddr);
		cmpAddr(xCAddr,pu4AddrC[1]);

		u32 yCAddr = prVdpHwReg->rField.u4YAddrC << 2;
		checkAddr(yCAddr);
		cmpAddr(yCAddr,pu4AddrC[2]);


		u32 zCAddr = prVdpHwReg->rField.u4ZAddrC << 2;
		checkAddr(zCAddr);
		cmpAddr(zCAddr,pu4AddrC[3]);
#endif
#endif
	} else {
		//ASSERT(0);
	}
}

void vVdpHalSetFieldInfo(u8 ucVdpId, bool fgTopFldFirst, bool fgFirstFld)
{
	HAL_VDO_UNION_T *prVdpSwReg;
	u8 *prVdpRegMode;
	volatile HAL_VDO_UNION_T *prVdpHwReg;

	GET_VDP_HW_PTR(ucVdpId, prVdpHwReg);

	GET_VDP_PTR(ucVdpId, prVdpSwReg, prVdpRegMode);
	(void)prVdpSwReg;
	(void)prVdpRegMode;

	if (fgTopFldFirst) {
		prVdpHwReg->rField.fgPFld = !fgFirstFld;	  /*Field Setting 0 for top, 1 for bottom.*/
		prVdpHwReg->rField.fgAFld = !fgFirstFld;	  /*Field Setting 0 for top, 1 for bottom.*/
	} else {
		prVdpHwReg->rField.fgPFld = fgFirstFld;		  /*Field Setting 0 for top, 1 for bottom.*/
		prVdpHwReg->rField.fgAFld = fgFirstFld;		  /*Field Setting 0 for top, 1 for bottom.*/
	}
}

void vVdpHalGetFieldInfo(u32 u4VdpId, u32 *prFldInfo)
{

}

void vVdpHalSetHDMode(u8 ucVdpId, u8 ucDiMode, u32 Width)
{

	if (ucDiMode == VDP_DI_MA4F_MODE) {
		if (Width > 1280) {
			VDPWriteREGMsk(0xE0, ((Width + 3) >> 2), 0x1FF);
			VDPWriteREGMsk(0xE0, 0, 0x1600000);
			VDPWriteREGMsk(0xE0, (1 << 21), 0x200000);
		} else if (Width > 720) {
			VDPWriteREGMsk(0xE0, ((Width + 3) >> 2), 0x1FF);
			VDPWriteREGMsk(0xE0, 0, 0x1600000);
			VDPWriteREGMsk(0xE0, (1 << 22), 0x400000);
		} else {
			VDPWriteREGMsk(0xE0, 0, 0x1600000);
		}
	} else {
		 if (Width > 720) {
			VDPWriteREGMsk(0xE0, ((Width + 3) >> 2), 0x1FF);
			VDPWriteREGMsk(0xE0, 0, 0x1600000);
			VDPWriteREGMsk(0xE0, (1 << 24), 0x1000000);
		} else {
			VDPWriteREGMsk(0xE0, 0, 0x1600000);
		}
	}

}


unsigned int MOTION_THRESHOLD = 0x10;
static unsigned int COMB_THRESHOLD = 0x03;
static unsigned int CT_THRESHOLD = 0x20;
void vVdpHalSetDeintMode(u8 ucVdpId, u8 ucDiMode)
{
	HAL_VDO_UNION_T *prVdpSwReg;
	u8 *prVdpRegMode;
	HAL_VDO_APQ_UNION_T   *prVdpApqSwReg;
	u8 *prVdpApqRegMode;

	GET_VDP_PTR(ucVdpId, prVdpSwReg, prVdpRegMode);
	GET_VDP_APQ_PTR(ucVdpId, prVdpApqSwReg, prVdpApqRegMode);
	(void)prVdpSwReg;
	//0x1C 24-31bit = 0x10, enable auto_reset? //xzr
	//prVdpSwReg->rField.u4TST = 0x10;
	//prVdpRegMode[(0x1C / 4)] |= PMX_HAL_REG_MODE_WRITE;

	//0x38 bit u4CTHRD 24-30bit Comb threshold
	//prVdpSwReg->rField.u4CTHRD = 0x6;
	VDPWriteREGMsk(0x38, 0x6 << 24, 0x7F000000);

	//prVdpRegMode[(0x38 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	if (ucDiMode == VDP_DI_MA4F_MODE) {
#if 1
		prVdpApqSwReg->rField.u4blend_Exp_Off_Sel  = 1; 				//Apq 0xF4
		prVdpApqSwReg->rField.u4Jaggy_smooth_enable = 0;				//Apq 0xF4
		prVdpApqSwReg->rField.u4Pre_Fetch_Line_Still_Dis = 0;			//Apq 0xF4

		/*prVdpSwReg->rField.fgRace = 0;					//0x1c
		prVdpSwReg->rField.fgPD32 = 0;	//verify is 1?											 //0x1C
		//force every pxl as motion point in motion regions
		prVdpSwReg->rField.fgF_CMB = 0; 		*/						//0x1C
		VDPWriteREGMsk(0x1C, (0x0 << 1) | (0x0 << 21) | (0x0 << 30), 0x40200002);

		/*prVdpSwReg->rField.fgYFrm = 0;									//0x30
		prVdpSwReg->rField.fgCFrm = 0;									//0x30
		prVdpSwReg->rField.fgCR80 = 1;*/
		// skip the first line to avoid chroma motion crash.
		//prVdpSwReg->rField.fgNF5S_1ST = 1;				//0x30
		VDPWriteREGMsk(0x30, (0x0 << 0) | (0x0 << 1) | (0x1 << 9), 0x203);

		//prVdpSwReg->rField.u4CT_THRD = CT_THRESHOLD;					//0x38
		//prVdpSwReg->rField.u4MTHRD = MOTION_THRESHOLD;					//0x38
		VDPWriteREGMsk(0x38, (CT_THRESHOLD << 8) | (MOTION_THRESHOLD << 16), 0xFFFF00);

		//prVdpSwReg->rField.u4FLD_DYN_8F_CNT = 0x2;			//0x40
		VDPWriteREGMsk(0x40, (0x2 << 16), 0xFF0000);

		prVdpApqSwReg->rField.u4SelNewMd = 0;							//0x4C
		prVdpApqSwReg->rField.u4SelNewPixDif = 0;
		prVdpApqSwReg->rField.u4EnVEdgeMag = 0;
		prVdpApqSwReg->rField.u4SelBigVEdge = 0;
		prVdpApqSwReg->rField.u4FrameVEdge = 0;
		prVdpApqSwReg->rField.u4MagSawOnly = 0;
		prVdpApqSwReg->rField.u4NoSpMoveChk = 0;
		prVdpApqSwReg->rField.u4UseUpDownClamp = 0;
		prVdpApqSwReg->rField.u4ClampNoMagnify = 0;

		VDPWriteREGMsk(0x70, (0x1 << 28), 0x10000000);

		//prVdpSwReg->rField.fgEHF = 1;	//disable edge horizontal filter				   //0x70

		/*prVdpSwReg->rField.fgCSP_DIS = 0; // 20080617 by YL 					//0x78
		prVdpSwReg->rField.fgBE_4LINE_FORCE = 0;						//0x78
		prVdpSwReg->rField.fgBE_4LINE_DIS = 0;							//0x78
		prVdpSwReg->rField.fgINTRA_EDGEPMODE = 0;						//0x78
		prVdpSwReg->rField.fgAYS4L_DIS = 0; // 0x42478[21]		*/			 //0x78
		VDPWriteREGMsk(0x78, (0x0 << 17) | (0 << 13) | (0 << 14) | (0 << 11) | (0 << 21), 0x226800);

		//prVdpSwReg->rField.u4Video_Opt20 = 1; //?												//0x7C
		//prVdpSwReg->rField.u4Video_Opt21 = 1; //?												//0x7C
		VDPWriteREGMsk(0x7C, (0x1 << 20) | (0x1 << 21), 0x30000);

		//prVdpSwReg->rField.fgMA4F = 1;								//0x88
		//prVdpSwReg->rField.u4FDIFF_CTRL = 1;							//0x88
		VDPWriteREGMsk(0x88, (0x1 << 24) | (0x1 << 25), 0x3000000);

		//prVdpSwReg->rField.u4MA_Video_Mode5 = 0;						//0x8C
		//prVdpSwReg->rField.u4MA_Video_Mode1 = 1;						//0x8C

		//prVdpSwReg->rField.u4MA_HW_Option3 = 1; 			//0x90
		VDPWriteREGMsk(0x8C, (0x0 << 29) | (0x1 << 25), 0x22000000);

		//prVdpSwReg->rField.u4EDGE_P_TH = 0x7c;				//0xB0
		//prVdpSwReg->rField.u4MA_EDGE_MODE0 = 1; 			//0xB0
		//prVdpSwReg->rField.u4MA_EDGE_MODE1 = 1; 			//0xB0

		/*prVdpSwReg->rField.fgBLEND_EXP_OFF = 0; 						//0xC0
		prVdpSwReg->rField.fgGET5F = 1; 							//0xC0
		prVdpSwReg->rField.fgMA5F = 0;								//0xC0
		prVdpSwReg->rField.fgWA_NA24   = 1; 		*/					//0xC0
		VDPWriteREGMsk(0xC0, (0x0 << 3) | (0x1 << 5) | (0x1 << 4) | (0x0 << 23), 0x800038);

		/*prVdpSwReg->rField.u4PD_COMB_TH = COMB_THRESHOLD;				//0xC4
		prVdpSwReg->rField.fgVDO_32_PD_EN = 0; //verify is 1?					//0xC4
		prVdpSwReg->rField.fgVDO_32_PD_MODE = 0;						//0xC4*/
		VDPWriteREGMsk(0xC4, (COMB_THRESHOLD << 16) | (0x0 << 24) | (0x0 << 26), 0x57F0000);

		//Chroma 3-field motion detection
		/*prVdpSwReg->rField.fgCRM_3FMD = 1;				//0xD0
		prVdpSwReg->rField.fgC_VT_BLEND = 1; //sd											   //0xD0
		prVdpSwReg->rField.fgC_VT_121 = 1;
		prVdpSwReg->rField.u4CRM_MOTION_CNT_TH = 16;
		prVdpSwReg->rField.fgC_INTER_X	  = 0;							//0xD0*/

		VDPWriteREG(0x00D0, 0x10001433);

		//prVdpSwReg->rField.fgF_PRGS = 1;                                //0xE0
		//prVdpSwReg->rField.fgF_L_SEL = 1; //First/Last fetching selection	       //0xE0

		//skip config fusion
#else
		prVdpSwReg->rField.fgMA4F = 1;
		//prVdpSwReg->rField.fgMDDI_CLK_ON = 1;

		prVdpSwReg->rField.u4Video_Opt20 = 1;
		prVdpSwReg->rField.u4Video_Opt21 = 1;

		prVdpSwReg->au4Reg[0xE0 / 4] |= 0xA0000000;

		prVdpSwReg->rField.fgYFrm = 0;
		prVdpSwReg->rField.fgCFrm = 0;
#endif

#if 1
		prVdpRegMode[(0x1C / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x30 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x38 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x40 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x4C / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x70 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x78 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x7C / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x88 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x8C / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x90 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0xB0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0xC0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0xC4 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0xD0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0xE0 / 4)] |= PMX_HAL_REG_MODE_WRITE;

		prVdpApqRegMode[(0xF4 / 4)] |= PMX_HAL_REG_MODE_WRITE;
#else
		prVdpRegMode[(0x30 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x7C / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0x88 / 4)] |= PMX_HAL_REG_MODE_WRITE;
		prVdpRegMode[(0xE0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
#endif
	} else {
		//prVdpSwReg->rField.fgMA4F = 0;
		//prVdpSwReg->rField.fgMDDI_CLK_ON = 0;
		VDPWriteREGMsk(0x88, (0x0 << 24), 0x1000000);

		//prVdpSwReg->rField.u4Video_Opt20 = 0;
		//prVdpSwReg->rField.u4Video_Opt21 = 0;
		VDPWriteREGMsk(0x7C, (0x0 << 20) | (0x0 << 21), 0x30000);

		//prVdpSwReg->au4Reg[0xE0 / 4] &= 0x5FFFFFFF;
		VDPWriteREGMsk(0xE0, (0x0 << 30) | (0x0 << 28), 0x5FFFFFFF);

		if (ucDiMode == VDP_DI_FRAME_MODE) {
			//prVdpSwReg->rField.fgYFrm = 1;
			//prVdpSwReg->rField.fgCFrm = 1;
			VDPWriteREGMsk(0x30, (0x1) | (0x1 << 1), 0x3);
		} else {
			//prVdpSwReg->rField.fgYFrm = 0;
			//prVdpSwReg->rField.fgCFrm = 0;
			//prVdpSwReg->rField.fgPFld = 0;
			VDPWriteREGMsk(0x30, (0x0) | (0x0 << 1) | (0x0 << 2), 0x7);
		}

			prVdpRegMode[(0x30 / 4)] |= PMX_HAL_REG_MODE_WRITE;
			prVdpRegMode[(0x7C / 4)] |= PMX_HAL_REG_MODE_WRITE;
			prVdpRegMode[(0x88 / 4)] |= PMX_HAL_REG_MODE_WRITE;
			prVdpRegMode[(0xE0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	}

}

void vVdpHalSetPdDetect(u8 ucVdpId, bool fgEnable)
{

}

void vVdpHalSetMergeZ(u32 u4VDP, bool fgMergeZ)
{

}


void vVdpHalDisMergeZ(u32 u4VDP)
{

}

void vVdpHalResume(u8 ucVdpId)
{

}

void vVdpHalGetMotionComb(u32 u4VdpId, u32 *prXYComb, u32 *prWXComb, u32 *prWYMotion, u32 *prXZMotion)
{

}
#endif




