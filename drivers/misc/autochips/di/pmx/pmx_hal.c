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
*  Plane Mixer: Interface
*****************************************************************************/

#ifndef _PMX_HAL_C_
#define _PMX_HAL_C_

#ifndef ATC_ARM2_LK_SUPPORT
#include <linux/module.h>
#include <linux/interrupt.h>
#include "pmx_hal.h"
#include "display_inc.h"
#include "x_os.h"
#include <linux/kthread.h>/*kthread_create*/
#include <linux/delay.h>
#include <linux/clk.h>

#else
#include "platform/mt_typedefs.h"
#include "platform/mt_reg_base.h"
#include "sys/types.h"
#include "di_types.h"

#include "assert.h"
#include "pmx_hal.h"
#include "display_inc.h"
#endif
#include "chip_ver.h"
#include "vdp_hw.h"
#include "vdp_hal.h"
#include "vdp.h"
#include "pmx_hw.h"
#include "log.h"
#include "wch_if.h"

/*static char di_name[100];
#define di_aee_print(string, args...) do {\
	snprintf(di_name, 100, "[DI][PMX]"string, ##args); \
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_MMPROFILE_BUFFER, di_name, "[DI][PMX] error:"string, ##args);  \
} while (0)*/


#define UTIL_Printf(a,...)

volatile bool PmxVerifyDoRestInVyncFront = FALSE;
volatile bool PmxVerifyDoRestInVyncRear = FALSE;
static bool _fgPmxMainIsrInited = FALSE;


/*** sw reg ***/
HAL_PMX_DISP_MAIN_UNION_T *pDcmPmxDispMainSwReg;
HAL_PMX_DISP_MAIN_UNION_T *pDcmPmxDispAuxSwReg;



//HAL_PMX_VDOUT_MAIN_UNION_T *pDcmPmxVdoutMainSwReg;
//HAL_PMX_VDOUT_AUX_UNION_T *pDcmPmxVdoutAuxSwReg;


HAL_PMX_DISP_MAIN_C_UNION_T *pDcmPmxDispMainCSwReg;
HAL_PMX_DISP_MAIN_C_UNION_T *pDcmPmxDispAuxCSwReg;
//HAL_PMX_SCLER_MAIN_UNION_T _rPmxSclerMainSwReg;
//HAL_PMX_SCLER_AUX_UNION_T _rPmxSclerAuxSwReg;
HAL_PMX_DISP_MAIN_UNION_T _rPmxDispMainSwReg;
HAL_PMX_DISP_MAIN_UNION_T _rPmxDispAuxSwReg;
HAL_PMX_DISP_MAIN_Y_UNION_T _rPmxDispMainYSwReg;
HAL_PMX_DISP_MAIN_Y_UNION_T _rPmxDisAuxYSwReg;

//HAL_PMX_VDOUT_MAIN_UNION_T _rPmxVdoutMainSwReg;
//HAL_PMX_VDOUT_AUX_UNION_T _rPmxVdoutAuxSwReg;
HAL_PMX_DISP_MAIN_C_UNION_T _rPmxDispMainCSwReg;
HAL_PMX_DISP_MAIN_C_UNION_T _rPmxDispAuxCSwReg;
PMX_HAL_MIX_UNION_T   _rPmxHalMixSwReg;

/*** hw reg ***/
volatile HAL_PMX_DISP_MAIN_UNION_T*  _prPmxDispMainHwReg;// = (HAL_PMX_DISP_MAIN_UNION_T*)HAL_PMX_DISP_MAIN_REG;
volatile HAL_PMX_DISP_MAIN_UNION_T*  _prPmxDispAuxHwReg;// = (HAL_PMX_DISP_MAIN_UNION_T*)HAL_PMX_DISP_AUX_REG;
volatile HAL_PMX_DISP_MAIN_Y_UNION_T * _prPmxDispMainYHwReg;
volatile HAL_PMX_DISP_MAIN_Y_UNION_T * _prPmxDispAuxYHwReg;
volatile HAL_PMX_DISP_MAIN_C_UNION_T*  _prPmxDispMainCHwReg;// = (HAL_PMX_DISP_MAIN_C_UNION_T*)HAL_PMX_DISP_MAIN_C_REG;
volatile HAL_PMX_DISP_MAIN_C_UNION_T*  _prPmxDispAuxCHwReg;// = (HAL_PMX_DISP_MAIN_C_UNION_T*)HAL_PMX_DISP_AUX_C_REG;
volatile PMX_HAL_MIX_UNION_T *  _prPmxHalMixHwReg;
volatile PMX_HAL_MIX_UNION_T *  _prPmxHalMix2HwReg;// = (PMX_HAL_MIX_UNION_T *)PMX_HAL_MIX_REG;

UINT64 *pPmxSclerMainRegMode;
UINT64 *pPmxSclerAuxRegMode;
UINT64 *pPmxDispMainRegMode;
UINT64 *pPmxDispAuxRegMode;
UINT64 *pPmxVdoutMainRegMode;
UINT64 *pPmxVdoutAuxRegMode;

UINT64 *pPmxDispMainYRegMode;
UINT64 *pPmxDispMainCRegMode;


UINT64 *pPmxDispAuxCRegMode;
UCHAR _rPmxSclerMainRegMode[HAL_PMX_SCLER_MAIN_REG_NUM];
UCHAR _rPmxSclerAuxRegMode[HAL_PMX_SCLER_AUX_REG_NUM];
UCHAR _rPmxDispMainRegMode[HAL_PMX_DISP_MAIN_REG_NUM];
UCHAR _rPmxDispAuxRegMode[HAL_PMX_DISP_AUX_REG_NUM];
UCHAR _rPmxVdoutMainRegMode[HAL_PMX_VDOUT_MAIN_REG_NUM];
UCHAR _rPmxVdoutAuxRegMode[HAL_PMX_VDOUT_AUX_REG_NUM];

UCHAR _rPmxDispMainYRegMode[HAL_PMX_DISP_MAIN_Y_REG_NUM];
UCHAR _rPmxDispMainCRegMode[HAL_PMX_DISP_MAIN_C_REG_NUM];
UCHAR _rPmxDispAuxCRegMode[HAL_PMX_DISP_AUX_C_REG_NUM];
UCHAR _rPmxMixRegMode[PMX_HAL_MIX_REG_NUM];

bool setBuffer = false;

#define REG_MASK(idx)           ((UINT64)1LL << (idx))
#define IS_REG_SET(r, mask)     ((r) & (mask))
#define REG_SET(r, mask)        ((r) |= (mask))

extern struct clk *di_clk;
extern struct clk *di_parent_clk;

#ifndef ATC_ARM2_LK_SUPPORT
extern void vVdpIsr(u32 u4VdpIdx);
extern void mt33xx_mask_ack_bim_irq(unsigned int virq);

#define MONITOR_TIME 0
#if MONITOR_TIME
struct timeval start;
static u32 u4TmpCnt;
static bool fgStartCnt = FALSE;
#endif

#endif



/*struct mutex buffer_lock;*/
#ifndef ATC_ARM2_LK_SUPPORT
static spinlock_t buffer_lock;
static unsigned long buffer_flags;
#endif


extern bool data_valid;
extern int buffernum;

#define TIMERCNT (20)
int wch_buffer_cnt = 0 , di_triger_cnt = 0;
int firstBuf = true;
int fir_triger = true;

void di_thread_init(void)
{
#ifndef ATC_ARM2_LK_SUPPORT
	spin_lock_init(&buffer_lock);
#endif

	return;
}

void di_thread_deinit(void)
{

}

void vPmxDIWork(unsigned long tmp)
{
#ifndef ATC_ARM2_LK_SUPPORT
	//static int cnt = 0;
	//VDO_LOG(VDO_LOG_LVL_WARN, "vPmxDIWork step in %d\r\n",cnt);
#if 0
	if(cnt == 100) {
		cnt = 0;
	}

#endif
	static int irq_num;
	if (irq_num < 6) {
		VDO_LOG(VDO_LOG_LVL_WARN, "%s entry %d\r\n", __func__, irq_num);
		irq_num++;
	}


	if(buffernum < 1) {
		//VDO_LOG(VDO_LOG_LVL_WARN, "vPmxDIWork no buffernum\r\n");
		return ;
	}

	vVdpIsr(0);
	spin_lock_irqsave(&buffer_lock, buffer_flags);
#if 0
	if(!getBufferDone) {
		//VDO_LOG(VDO_LOG_LVL_WARN, "vPmxDIWork get no buffer done \r\n");
		cnt++;
		if(cnt == 10) {
			/*di_aee_print("WCH_DI will work 10 times and reset it\n");*/
			FmtWriteREG(0x00AC,0x08000000);
			VDO_LOG(VDO_LOG_LVL_WARN, "Set FMT Mode reset start\r\n");
			spin_unlock_irqrestore(&buffer_lock, buffer_flags);
			return;
		} else if(cnt == 12) {
			FmtWriteREG(0x00AC,0x08000003);
			VDO_LOG(VDO_LOG_LVL_WARN, "Set FMT Mode reset end\r\n");
			cnt = 0;
		}

	}
	else {
		cnt = 0;
	}
	getBufferDone= false;
	#endif
	/*mutex_unlock(&buffer_lock);*/
	spin_unlock_irqrestore(&buffer_lock, buffer_flags);
	wch_set_di_triger_status(WCH_DI);
	_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
	_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;
	//VDO_LOG(VDO_LOG_LVL_WARN, "vPmxDIWork step out %d\r\n",cnt);
#else


	if(!data_valid) {
		//VDO_LOG(VDO_LOG_LVL_WARN, "vPmxDIWork no data \r\n");
		return ;
	}
	vVdpIsr(0);
	//	WriteREGMsk(0x2000004,1,1);
	//	WriteREGMsk(0x2000004,0,1);
	wch_set_di_triger_status(WCH_DI);
	_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
	_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;
#endif
	return ;
}

void vPmxHalTrigger(void)
{
	_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
	_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;

}

/*

void getPmxHwToSw(void)
{
	int regcnt = 0;
	for (; regcnt < PMX_HAL_MIX_REG_NUM * 4; regcnt += 4) {
		//there is a overflow if use need to fix
		_rPmxHalMixSwReg.au4Reg[regcnt] = _prPmxHalMixHwReg->au4Reg[regcnt];
		VDO_LOG(VDO_LOG_LVL_DBG, "getPmxHwToSw _rPmxHalMixSwReg.au4Reg[%d]:0x%x \r\n",
			regcnt, _rPmxHalMixSwReg.au4Reg[regcnt]);
	}
}*/
#ifndef ATC_ARM2_LK_SUPPORT
//tmp solution for get hw ioreg instead of read from dts
void getpmxbaseaddr(void)
{
	int ret = 0;

	VDO_LOG(VDO_LOG_LVL_INFO, "getpmxbaseaddr\r\n");
#if 0
	/*enable di clk*/
	//CLKWriteREGMsk(0x0000, 0x80220, 0x1803E0);
	//VDO_LOG(VDO_LOG_LVL_DBG, "DI clock[0x00C0]:0x%x\r\n", CLKReadREG(0x0000));
#else
	/*enable di clk*/
	ret = clk_prepare_enable(di_parent_clk);
	if (ret) {
		VDO_LOG(VDO_LOG_LVL_ERR, "DI enable di_parent_clk error 0x%x\r\n", ret);
	}
	ret = clk_prepare_enable(di_clk);
	if (ret) {
		VDO_LOG(VDO_LOG_LVL_ERR, "DI enable di_parent_clk error 0x%x\r\n", ret);
	}
	ret = clk_set_parent(di_clk, di_parent_clk);
	if (ret) {
		VDO_LOG(VDO_LOG_LVL_ERR, "DI enable di_parent_clk error 0x%x\r\n", ret);
	}
	VDO_LOG(VDO_LOG_LVL_INFO, "DI clock enable\r\n");
#endif

	//Fmt                                                                       v0x12002400
	_prPmxDispMainHwReg = (HAL_PMX_DISP_MAIN_UNION_T *)(fmtf_reg);
	_prPmxDispAuxHwReg = (HAL_PMX_DISP_MAIN_UNION_T *)(fmtf_reg);
	VDO_LOG(VDO_LOG_LVL_INFO, "_prPmxDispMainHwReg:0x%p\r\n", _prPmxDispMainHwReg);
	VDO_LOG(VDO_LOG_LVL_INFO, "_prPmxDispAuxHwReg:0x%p\r\n", _prPmxDispAuxHwReg);

	//Y-Coef
	_prPmxDispMainYHwReg = (HAL_PMX_DISP_MAIN_Y_UNION_T *)(IO_BASE_BRINGUP + 0x1C00);
	VDO_LOG(VDO_LOG_LVL_INFO, "_prPmxDispMainYHwReg:0x%p\r\n", _prPmxDispMainYHwReg);

	//C-Coef
	_prPmxDispMainCHwReg = (HAL_PMX_DISP_MAIN_C_UNION_T*)(IO_BASE_BRINGUP + 0x1D00);
	_prPmxDispAuxCHwReg = (HAL_PMX_DISP_MAIN_C_UNION_T*)(IO_BASE_BRINGUP + 0x1D00);
	VDO_LOG(VDO_LOG_LVL_INFO, "_prPmxDispMainCHwReg:0x%p\r\n", _prPmxDispMainCHwReg);
	VDO_LOG(VDO_LOG_LVL_INFO, "_prPmxDispAuxCHwReg:0x%p\r\n", _prPmxDispAuxCHwReg);


}
#endif
void PMX_HalSetupSoftwareRegister(void)
{
	// dispfmt
	pDcmPmxDispMainSwReg = &_rPmxDispMainSwReg;
	pPmxDispMainRegMode = (UINT64 *)&_rPmxDispMainRegMode;


	// dispfmt_pip
	pDcmPmxDispAuxSwReg = &_rPmxDispAuxSwReg;
	pPmxDispAuxRegMode = (UINT64 *)&_rPmxDispAuxRegMode;

	// vdout_fmt
	//pDcmPmxVdoutMainSwReg = &_rPmxVdoutMainSwReg;
	//pPmxVdoutMainRegMode = (UINT64 *)&_rPmxVdoutMainRegMode;
	// vdout_fmt_pip
	//pDcmPmxVdoutAuxSwReg = &_rPmxVdoutAuxSwReg;
	//pPmxVdoutAuxRegMode = (UINT64 *)&_rPmxVdoutAuxRegMode;

	//timing scler
	//pDcmPmxSclerMainSwReg = &_rPmxSclerMainSwReg;
	//pPmxSclerMainRegMode = (UINT64 *)&_rPmxSclerMainRegMode;

	//pDcmPmxSclerAuxSwReg = &_rPmxSclerAuxSwReg;
	//pPmxSclerAuxRegMode = (UINT64 *)&_rPmxSclerAuxRegMode;

	//pPmxDispMainYRegMode = (UINT64 * )&_rPmxDispMainYRegMode;

	// dispfmt CS
	pDcmPmxDispMainCSwReg = &_rPmxDispMainCSwReg;
	//pPmxDispMainCRegMode = (UINT64 *)&_rPmxDispMainCRegMode;
	// dispfmt_pip CS
	pDcmPmxDispAuxCSwReg = &_rPmxDispAuxCSwReg;
	//pPmxDispAuxCRegMode = (UINT64 *)&_rPmxDispAuxCRegMode;
}


#if DI_GET_BUF
int vPmxSetWchOutBuf(void)
{
#if 0
	int ret = 0;
	//ret = wch_hw_set_di_buffer(WCH_DI,di_outbuf_list,di_buf_num);
	if (ret == WCH_SUCCESS ) {
		setBuffer = true;
	} else {
		VDO_LOG(VDO_LOG_LVL_ERR, "vPmxSetWchOutBuf set wch buffer fail\r\n");
		return -1;
	}
#endif
	return 0;
}
#endif
void vPmxHalInit(int width,int height,int inWidth,int inHeight)
{
	volatile HAL_VDO_UNION_T *prVdpHwRegs = NULL;
	int outWidth = width;
	int outHeight = height;

	GET_VDP_HW_PTR(VDP_1, prVdpHwRegs);

	#if 1

	FmtWriteREG(0x0000,0x00AC04AB);
	FmtWriteREG(0x0004,0xc302a20a);
	FmtWriteREG(0x0008,0x00B30382);
	FmtWriteREG(0x000C,0x00AC04AB);
	FmtWriteREG(0x0010,0x00000001);
	FmtWriteREG(0x0014,0x00080000);
	FmtWriteREG(0x0018,0x00000000);
	FmtWriteREG(0x001C,0x00050005);
	FmtWriteREG(0x008C,0x002B020A);
	FmtWriteREG(0x0090,0x002B020A);


	FmtWriteREG(0x00f8,0x00100000);

	FmtWriteREG(0x0080,0x00503801);
	FmtWriteREG(0x0084,0xC102A20A);
	FmtWriteREG(0x0088,0x00B30382);
	FmtWriteREG(0x008C,0x002B020A);
	FmtWriteREG(0x0090,0x002B020A);
	FmtWriteREG(0x0094,0x00008D20);
	FmtWriteREG(0x0098,0x00100000);
	FmtWriteREG(0x009C,0x000002D0);
	FmtWriteREG(0x00B4,0x00808080);
	FmtWriteREG(0x00B8,0x00302010);
	FmtWriteREG(0x00E4,0x00000000);
	FmtWriteREG(0x0008,0x00B30382);
	FmtWriteREG(0x0008,0x00B30382);
	FmtWriteREG(0x005c,0x0010f018);
	#endif
	//WriteREG(0x20010AC,0x08000003);
	_prPmxDispMainHwReg->rField.fgHOR_VALID_START = 0xAC;
	_prPmxDispMainHwReg->rField.fgHOR_VALID_END = _prPmxDispMainHwReg->rField.fgHOR_VALID_START +outWidth-1;
	//_prPmxDispMainHwReg->rField.fgHOR_NR_VALID_START = _prPmxDispMainHwReg->rField.fgHOR_VALID_START;
	//_prPmxDispMainHwReg->rField.fgHOR_NR_VALID_END =_prPmxDispMainHwReg->rField.fgHOR_VALID_END;

	_prPmxDispMainHwReg->rField.fgNR_IN_VSYNC_START = 0x2A;
	_prPmxDispMainHwReg->rField.fgNR_IN_VSYNC_END = _prPmxDispMainHwReg->rField.fgNR_IN_VSYNC_START + outHeight;
	_prPmxDispMainHwReg->rField.fgNR_IN_HOR_STAT= _prPmxDispMainHwReg->rField.fgHOR_VALID_START;
	_prPmxDispMainHwReg->rField.fgNR_IN_HOR_END =_prPmxDispMainHwReg->rField.fgHOR_VALID_END;
	_prPmxDispMainHwReg->rField.u2HACTBGN = 0xA5;
	_prPmxDispMainHwReg->rField.u2HACTEND = _prPmxDispMainHwReg->rField.u2HACTBGN+outWidth-1;

	//_prPmxDispMainHwReg->rField.fgHOR_NR_VALID_START = 0xB3;
	//_prPmxDispMainHwReg->rField.fgHOR_NR_VALID_END = _prPmxDispMainHwReg->rField.fgHOR_NR_VALID_START +outWidth-1;

	_prPmxDispMainHwReg->rField.u2VEACTBGN= 0x2B;
	_prPmxDispMainHwReg->rField.u2VEACTEND= _prPmxDispMainHwReg->rField.u2VEACTBGN+outHeight-1;

	_prPmxDispMainHwReg->rField.u2VEACTBGN = _prPmxDispMainHwReg->rField.u2VEACTBGN;
	_prPmxDispMainHwReg->rField.u2VEACTEND = _prPmxDispMainHwReg->rField.u2VEACTEND;
	_prPmxDispMainHwReg->rField.u1HIDE_EST = 0;
	_prPmxDispMainHwReg->rField.u2VOACTBGN =_prPmxDispMainHwReg->rField.u2VEACTBGN;
	_prPmxDispMainHwReg->rField.u2VOACTEND =_prPmxDispMainHwReg->rField.u2VEACTEND;
	_prPmxDispMainHwReg->rField.u1HIDE_OST = 0;
	_prPmxDispMainHwReg->rField.fgEVEN_FIR = 0;
	_prPmxDispMainHwReg->rField.fgEVEN_FIR=0;
	_prPmxDispMainHwReg->rField.fgHSLR = 1;
	_prPmxDispMainHwReg->rField.fgHSON =1;
	_prPmxDispMainHwReg->rField.u2HSFACTOR = (inWidth)*(0x100)/(outWidth);
	/*in width equal out width*/
	if(inWidth == outWidth) {
		_prPmxDispMainHwReg->rField.u2HSFACTOR = 0x100;
	}

	//different width
	if(inWidth > 1500) {
		FmtWriteREG(0x00d0,0x896004b0);
		FmtWriteREG(0x00d4,0x896004b0);
	} else {
		FmtWriteREG(0x00d0,0x85dc03e8);
		FmtWriteREG(0x00d4,0x85dc03e8);
	}
	FmtWriteREG(0x00C0,0x00000000);
	FmtWriteREG(0x00CC,0x00100000);

	_prPmxDispMainHwReg->rField.fgFMTM = 1;

	prVdpHwRegs->rField.fgGET5F = 1;
	prVdpHwRegs->rField.fgMA4F = 1;
	if(inHeight== 480) {
		prVdpHwRegs->rField.u4Vscale = (480)*(0x800)/(outHeight);

		VDPWriteREG(0x0010,0x01E0B45A);
	} else {
		prVdpHwRegs->rField.u4Vscale = (576)*(0x800)/(outHeight);
		if( inHeight> outHeight)
		{
			//out > in
			prVdpHwRegs->rField.u4Vscale = 0x999;
		}

		VDPWriteREG(0x0010,0x0240B45A);
	}
	/*in Height equal out height*/
	if(inHeight == outHeight) {
		prVdpHwRegs->rField.u4Vscale = 0x800;
	}
	VDPWriteREG(0x20,0x07FD07FC);
	VDPWriteREG(0x24,0x07FF07FE);
	VDPWriteREG(0x50,0x07FC07FB);
	VDPWriteREG(0x54,0x07FE07FD);

	VDPWriteREG(0x28,0x80800000);
	VDPWriteREG(0x2C,0xA0E02060);
	VDPWriteREG(0x6C,0x05010000);
	VDPWriteREG(0x34,0x20d82020);
	VDPWriteREG(0x38,0x06102000);
	VDPWriteREG(0x3c,0x771d1d1e);

	prVdpHwRegs->rField.fgSwap_Off = 1;

	_prPmxDispMainHwReg->rField.fgPFOFF = 1;
	prVdpHwRegs->rField.fgVdoEn = 1;
	VDPWriteREG(0x18,0x1E1E0000);
	VDPWriteREG(0x64,0x00008000);

	return;
#if 0
	/*Start line*/
	VDPWriteREG(0x20,0x07FA07F9);
	VDPWriteREG(0x24,0x07FD07FC);

	VDPWriteREG(0x50,0x07FA07F9);
	VDPWriteREG(0x54,0x07FD07FC);

	VDPWriteREG(0x0028,0x00000000);
	VDPWriteREG(0x002C,0x20602060);
	return;

	/*Start line for 4-field*/
	VDPWriteREG(0x20,0x07FD07FC);
	VDPWriteREG(0x24,0x07FF07FE);

	VDPWriteREG(0x50,0x07FC07FB);
	VDPWriteREG(0x54,0x07FE07FD);
	VDPWriteREG(0x18,0x1E1E0000);
	VDPWriteREG(0x28,0x00000000);
	VDPWriteREG(0x1C,0x00000003);

	VDPWriteREG(0x0018,0x1E1E0000);
	VDPWriteREG(0x001C,0x00000003);
	VDPWriteREG(0x0020,0x07FA07F9);
	VDPWriteREG(0x0024,0x07FD07FC);
	VDPWriteREG(0x0028,0x00000000);
	VDPWriteREG(0x002C,0x20602060);
	VDPWriteREG(0x0030,0x00000003);
	VDPWriteREG(0x0034,0x14C91414);
	VDPWriteREG(0x0038,0x06000E00);

	VDPWriteREG(0x40,0x2002FF00);
	VDPWriteREG(0x44,0x201F0000);
	VDPWriteREG(0x48,0x6A986A6C);
	VDPWriteREG(0x4C,0x90584C4A);
	VDPWriteREG(0x50,0x07FA07F9);
	VDPWriteREG(0x54,0x07FD07FC);
	VDPWriteREG(0x58,0x26800000);
	VDPWriteREG(0x5C,0x26840000);
	VDPWriteREG(0x60,0x00000000);
	VDPWriteREG(0x64,0x00008702);
	VDPWriteREG(0x68,0x00000000);
	VDPWriteREG(0x6C,0x05010000);
	VDPWriteREG(0x70,0x00001000);
	VDPWriteREG(0x74,0x00000000);
	VDPWriteREG(0x78,0x00000000);
	VDPWriteREG(0x7C,0x04000000);
	VDPWriteREG(0x88,0x4A0186A0);
	VDPWriteREG(0x8C,0x5B011940);
	VDPWriteREG(0x90,0x0000C350);
	VDPWriteREG(0x94,0x00012003);
	VDPWriteREG(0x98,0x3202A02A);
	VDPWriteREG(0x9C,0x0600C002);
	VDPWriteREG(0xA0,0x6401C01C);
	VDPWriteREG(0xA4,0x32032004);
	VDPWriteREG(0xA8,0x00036000);
	VDPWriteREG(0xAC,0x02000020);
	VDPWriteREG(0xB0,0x0332323C);
	VDPWriteREG(0xB4,0x00320814);
	VDPWriteREG(0xB8,0x00320432);
	VDPWriteREG(0xBC,0x00005537);
	VDPWriteREG(0xC0,0x02332083);
	VDPWriteREG(0xC4,0x01030510);
	VDPWriteREG(0xC8,0x04162000);
	VDPWriteREG(0xCC,0x00007601);
	VDPWriteREG(0xD0,0x10001433);
	VDPWriteREG(0xD4,0x40060008);
	VDPWriteREG(0xD8,0x26800000);
	VDPWriteREG(0xDC,0x0FFFFFE0);
	VDPWriteREG(0xE0,0x020000B4);
	VDPWriteREG(0xE4,0x00000000);
	VDPWriteREG(0xE8,0x00000000);
	VDPWriteREG(0xF0,0x00000000);
	VDPWriteREG(0xF4,0x00000000);
	VDPWriteREG(0xF8,0xFFFFFFFF);
	VDO_LOG(VDO_LOG_LVL_INFO, "vPmxHalInit start \r\n");

	return;
#endif

}

void vPmxHalRstInVSync(u8 ucPmxId)
{
	if (ucPmxId == PMX_1) {
		PmxVerifyDoRestInVyncFront = TRUE;
	} else {
		PmxVerifyDoRestInVyncRear = TRUE;
	}
}

void vPmxHalReset(u8 ucPmxId)
{
	vPmxHalRstInVSync(ucPmxId);
}

void vPmxHalSetMasterMode(bool fgEnable)
{
        //[xzr]
}

extern bool EnPmxRearNewSD144Mode;
#if 0
void PMX_HalSetMode(UCHAR ucPmxId, UCHAR ucFmt)
{
	HAL_PMX_DISP_MAIN_UNION_T *pDcmPmxSwReg;
	//HAL_PMX_DISP_MAIN_Y_UNION_T * pDcmPmxSwYCoefSwReg;
	//volatile HAL_PMX_DISP_MAIN_UNION_T*  pDcmPmxHwReg = (ucPmxId==0) ? _prPmxDispMainHwReg : _prPmxDispAuxHwReg;
	//volatile HAL_PMX_DISP_MAIN_Y_UNION_T *pDcmPmxYHwReg = (ucPmxId == 0)?_prPmxDispMainYHwReg:_prPmxDispAuxYHwReg;

	if(ucPmxId==0){
		pDcmPmxSwReg = pDcmPmxDispMainSwReg;
	}else{
		pDcmPmxSwReg = pDcmPmxDispAuxSwReg;
	}
	// special case, don't wait VSYNC, since ISR may NOT happen
	if (ucPmxId == PMX_1)  //main vdo 0
	{
		//pDcmPmxSwReg->rField.fgDownScaleMode=1;
		pDcmPmxSwReg->rField.u2C4Default = 0x10EB;
		switch(ucFmt)
		{
			case RES_480P_800:
				VDO_LOG(VDO_LOG_LVL_INFO, "PMX_HalSetMode start 11000\r\n");
				pDcmPmxSwReg->rField.fgADJ_T = 1;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 0;

				pDcmPmxSwReg->rField.fgHD_ON = 0;
				pDcmPmxSwReg->rField.fgHD_TP = 0;
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;

				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				pDcmPmxSwReg->rField.u2V_TOTAL = 0x271;
				pDcmPmxSwReg->rField.u2H_TOTAL = 0x540;

				pDcmPmxSwReg->rField.horizontal_use_2fs = 1;
				pDcmPmxSwReg->rField.new_hd= 1;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 1;

				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				break;
#if 0
				pDcmPmxSwReg->rField.fgHD_ON = 0;
				pDcmPmxSwReg->rField.fgHD_TP = 0;
				pDcmPmxSwReg->rField.fgADJ_T = 1;
				pDcmPmxSwReg->rField.u2H_TOTAL = 0x3e8;
				//pDcmPmxSwReg->rField.u2H_TOTAL_MIX = 0x3b9;

				pDcmPmxSwReg->rField.u2V_TOTAL = 0x20d;
				//pDcmPmxSwReg->rField.u2V_TOTAL_MIX=0x20d;

				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0;
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				pDcmPmxSwReg->rField.fgAverage422to444 = 1;

				pDcmPmxSwReg->rField.fgNEW_SD_MODE = 1; //new_sd_144m
				pDcmPmxSwReg->rField.fgDSDCLK_EANBLE = 1; //new_sd_144m

				pDcmPmxSwReg->rField.horizontal_use_3fs = 1;
				pDcmPmxSwReg->rField.new_hd= 0;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0;
				break;
#endif
			case RES_480I:
			case RES_480P:
				pDcmPmxSwReg->rField.fgHD_ON = 0;
				pDcmPmxSwReg->rField.fgHD_TP = 0;
				pDcmPmxSwReg->rField.fgADJ_T = 0;
				pDcmPmxSwReg->rField.u2H_TOTAL = 0x3e8;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 0;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0;
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				break;

			case RES_576I:
			case RES_576P:
				pDcmPmxSwReg->rField.fgHD_ON = 0;
				pDcmPmxSwReg->rField.fgHD_TP = 0;
				pDcmPmxSwReg->rField.fgADJ_T = 0;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 0;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0;
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				break;

			case RES_720P60HZ:
			case RES_720P50HZ:
			case RES_720P23HZ:
			case RES_720P24HZ:
			case RES_720P25HZ:
			case RES_720P30HZ:
				pDcmPmxSwReg->rField.fgADJ_T = 0;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 0;

				if ((ucFmt == RES_720P23HZ) || (ucFmt == RES_720P24HZ)||(ucFmt == RES_3D_720P23HZ_TB) || (ucFmt == RES_3D_720P23HZ_SBS_HALF)||
				(ucFmt == RES_3D_720P24HZ_TB) || (ucFmt == RES_3D_720P24HZ_SBS_HALF) || (ucFmt == RES_720P30HZ) ||
				(ucFmt == RES_3D_720P30HZ_TB) || (ucFmt == RES_3D_720P30HZ_SBS_HALF))
				{
					pDcmPmxSwReg->rField.u2H_TOTAL = 3300;
					pDcmPmxSwReg->rField.u2H_TOTAL_MIX = 3300;

					pDcmPmxSwReg->rField.u2V_TOTAL = 750;
					pDcmPmxSwReg->rField.u2V_TOTAL_MIX=750;
					pDcmPmxSwReg->rField.fgADJ_T = 1;
					pDcmPmxSwReg->rField.fgADJ_T_MIX = 1;
				}
				else if ((ucFmt == RES_720P25HZ) || (ucFmt == RES_3D_720P25HZ_TB)||(ucFmt == RES_3D_720P25HZ_SBS_HALF))
				{
					pDcmPmxSwReg->rField.u2H_TOTAL = 3960;
					pDcmPmxSwReg->rField.u2H_TOTAL_MIX = 3960;
					pDcmPmxSwReg->rField.u2V_TOTAL = 750;
					pDcmPmxSwReg->rField.u2V_TOTAL_MIX=750;
					pDcmPmxSwReg->rField.fgADJ_T = 1;
					pDcmPmxSwReg->rField.fgADJ_T_MIX = 1;
				}

				pDcmPmxSwReg->rField.fgHD_ON = 1;
				pDcmPmxSwReg->rField.fgHD_TP = 1;
				//pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0; //for 3365
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				/*new add for 3365*/
				if(ucFmt == RES_720P60HZ){
					pDcmPmxSwReg->rField.u2V_TOTAL = 0x2ee;
					pDcmPmxSwReg->rField.u2H_TOTAL = 0x672;
				}else if(ucFmt == RES_720P50HZ){
					pDcmPmxSwReg->rField.u2V_TOTAL = 0x2ee;
					pDcmPmxSwReg->rField.u2H_TOTAL = 0x7bc;
				}
				pDcmPmxSwReg->rField.horizontal_use_2fs = 1;
				pDcmPmxSwReg->rField.new_hd= 1;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 1;

				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				break;

			case RES_600P_1024:
				pDcmPmxSwReg->rField.fgADJ_T = 1;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 0;

				pDcmPmxSwReg->rField.fgHD_ON = 1;
				pDcmPmxSwReg->rField.fgHD_TP = 0;
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;

				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				pDcmPmxSwReg->rField.u2V_TOTAL = 0x271;
				pDcmPmxSwReg->rField.u2H_TOTAL = 0x540;

				pDcmPmxSwReg->rField.horizontal_use_2fs = 1;
				pDcmPmxSwReg->rField.new_hd= 1;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 1;

				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				break;

			case RES_600P_800:
				//pDcmPmxSwReg->rField.fgADJ_T = 0;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 0;
				pDcmPmxSwReg->rField.fgHD_ON = 1;
				pDcmPmxSwReg->rField.fgHD_TP = 0;
				//pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0; //for 3365
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				/*new add for 3365*/
				pDcmPmxSwReg->rField.fgADJ_T =1;

				pDcmPmxSwReg->rField.u2V_TOTAL = 0x294;
				pDcmPmxSwReg->rField.u2H_TOTAL = 0x3f2;

				pDcmPmxSwReg->rField.horizontal_use_4fs = 1;
				pDcmPmxSwReg->rField.new_hd= 1;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 1;
				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				break;


			case RES_1080P60HZ:
			case RES_1080P50HZ:
			case RES_1080I60HZ:
			case RES_1080I50HZ:
				pDcmPmxSwReg->rField.fgHD_ON = 1;
				pDcmPmxSwReg->rField.fgHD_TP = 0;
				pDcmPmxSwReg->rField.fgADJ_T = 0;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 0;
				//pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0;
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				pDcmPmxSwReg->rField.fgAverage422to444 = 1;

				/*new add for 3365*/
				if(ucFmt == RES_1080P60HZ){
					pDcmPmxSwReg->rField.u2V_TOTAL = 1125;
					pDcmPmxSwReg->rField.u2H_TOTAL = 2200;

				}else if(ucFmt == RES_1080P50HZ){
					pDcmPmxSwReg->rField.u2V_TOTAL = 1125;
					pDcmPmxSwReg->rField.u2H_TOTAL = 2640;
				}
				pDcmPmxSwReg->rField.horizontal_use_fs = 1;
				pDcmPmxSwReg->rField.new_hd= 1;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 1;

				break;

			case RES_1080P24HZ:
			case RES_1080P23_976HZ:
				pDcmPmxSwReg->rField.fgHD_ON = 1;
				pDcmPmxSwReg->rField.fgHD_TP = 1;
				pDcmPmxSwReg->rField.u2V_TOTAL = 0x465;
				pDcmPmxSwReg->rField.u2H_TOTAL = 0xABE;
				pDcmPmxSwReg->rField.fgADJ_T = 1;
				pDcmPmxSwReg->rField.u2V_TOTAL_MIX = 0x465;
				pDcmPmxSwReg->rField.u2H_TOTAL_MIX = 0xABE;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 1;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0;
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				break;

			case RES_1080P25HZ:
				pDcmPmxSwReg->rField.fgHD_ON = 1;
				pDcmPmxSwReg->rField.fgHD_TP = 1;
				pDcmPmxSwReg->rField.u2V_TOTAL = 0x465;
				pDcmPmxSwReg->rField.u2H_TOTAL = 0xA50;
				pDcmPmxSwReg->rField.fgADJ_T = 1;
				pDcmPmxSwReg->rField.u2V_TOTAL_MIX = 0x465;
				pDcmPmxSwReg->rField.u2H_TOTAL_MIX = 0xA50;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 1;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0;
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				break;

			case RES_1080P30HZ:
			case RES_1080P29_97HZ:
				pDcmPmxSwReg->rField.fgHD_ON = 1;
				pDcmPmxSwReg->rField.fgHD_TP = 1;
				pDcmPmxSwReg->rField.u2V_TOTAL = 0x465;
				pDcmPmxSwReg->rField.u2H_TOTAL = 0x898;
				pDcmPmxSwReg->rField.fgADJ_T = 1;
				pDcmPmxSwReg->rField.u2V_TOTAL_MIX = 0x465;
				pDcmPmxSwReg->rField.u2H_TOTAL_MIX = 0x898;
				pDcmPmxSwReg->rField.fgADJ_T_MIX = 1;
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0;
				pDcmPmxSwReg->rField.fgEN_FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.u1FIRST_PXL_LEAD = 0;
				pDcmPmxSwReg->rField.fgFACT_PREC = 1;
				pDcmPmxSwReg->rField.fgAverage422to444 = 1;
				break;

			//ToDo: add settings for 1080P25HZ, 1080P30HZ, 1080P24HZ
			//(1)0x30d4 bit28 ] 1, BNH_Total, V_total ](CEA-861-D spec)
			//    1080P30:  H_total= 2200, V_total=1125 => 0x30d4= 0x18980465
			//    1080P25:  H_total= 2640, V_total=1125 => 0x30d4= 0x1A500465
			//    1080P24:  H_total= 2750, V_total=1125 => 0x30d4= 0x1ABE0465
			//(2)0x30A0, 0x30A4, 0x30A8 ]1080P active region 1920x 1080
			//    0x30a0= 0x00970817,
			//    0x30a4=   0x002a0461,
			//    0x30a8=   0x002a0461,
			//(3)0x3094 , bit 13,bit 14,bit 15  ]1
			default:
				//VERIFY(0);
				VDO_LOG(VDO_LOG_LVL_ERR, "VERIFY Failed: %s, %d!\r\n", __FILE__, __LINE__);
				break;
		}

		switch(ucFmt)
		{
			case RES_1080I50HZ: // PMX mode is 1080P 50. P2I output 1080I
			case RES_1080I60HZ: // PMX mode is 1080P 60. P2I output 1080I
			case RES_1080P50HZ:
			case RES_1080P60HZ:
			case RES_720P60HZ:
			case RES_720P50HZ:
			case RES_600P_800:
			case RES_600P_1024:
				//For Front FMT, must be 1 when hd mode.
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 1;
				break;
			default:
				pDcmPmxSwReg->rField.fgNOT_PST_D2 = 0;
				break;
		}

	if(fgIsHDRes(ucFmt) && (!fgIsFullHDRes(ucFmt)))
	{
		pDcmPmxSwReg->rField.fgHD_C_FIR_EN = 1;
	}
	else
	{
		pDcmPmxSwReg->rField.fgHD_C_FIR_EN = 0;
	}

	// for pana  pq request
	// repeat mode is better for s&m tt14(setup&evlauration->chroma multiburst),2013-06-18,yongqiang.niu,mtk40418

	pDcmPmxSwReg->rField.fgAverage422to444 = 0;

#if 0 //xzr
	pDcmPmxHwReg->au4Reg[(0xd0/4)] = pDcmPmxSwReg->au4Reg[(0xd0/4)];
	pDcmPmxHwReg->au4Reg[(0xd4/4)] = pDcmPmxSwReg->au4Reg[(0xd4/4)];
	pDcmPmxHwReg->au4Reg[(0xc4/4)] = pDcmPmxSwReg->au4Reg[(0xc4/4)];
	pDcmPmxHwReg->au4Reg[(0xe4/4)] = pDcmPmxSwReg->au4Reg[(0xe4/4)];
	pDcmPmxHwReg->au4Reg[(0xf8/4)] = pDcmPmxSwReg->au4Reg[(0xf8/4)];
#endif

	_rPmxDispMainRegMode[(0xd0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	_rPmxDispMainRegMode[(0xd4 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	_rPmxDispMainRegMode[(0xc4 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	_rPmxDispMainRegMode[(0xe4 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	_rPmxDispMainRegMode[(0xf8 / 4)] |= PMX_HAL_REG_MODE_WRITE;

	if(fgIsTrueInterlaceSupport(ucFmt))
	{
		pDcmPmxSwReg->rField.fgPRGS = 0;
		pDcmPmxSwReg->rField.u1VSYNWIDTH = 8;
	}
	else
	{
		UINT8 u1VsyncWidth = 12;
		pDcmPmxSwReg->rField.fgPRGS = 1;
		pDcmPmxSwReg->rField.u1VSYNWIDTH = u1VsyncWidth;
	}
	pDcmPmxSwReg->rField.u1HSYNWIDTH = 32;
#if 0 //xzr
	pDcmPmxHwReg->au4Reg[(0x94/4)] = pDcmPmxSwReg->au4Reg[(0x94/4)];
#endif
	_rPmxDispMainRegMode[(0x94 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	pDcmPmxSwReg->rField.fgPFOFF = 1;
	//pDcmPmxSwReg->rField.fgFMTM = 1;
#if 0 //xzr
	pDcmPmxHwReg->au4Reg[(0xac/4)] = pDcmPmxSwReg->au4Reg[(0xac/4)];
#endif
	_rPmxDispMainRegMode[(0xac / 4)] |= PMX_HAL_REG_MODE_WRITE;

	}else{
	} //end if vdo 1

	VDO_LOG(VDO_LOG_LVL_INFO, " %s,no fast logo,HalResetInVSync(4)--- .\n",__FUNCTION__);
	pDcmPmxSwReg->rField.u2H_TOTAL = 0xbb7;
	//vPmxHalRstInVSync(ucPmxId);
	//  _PmxMainReset();
}
#endif
#if 0 /*no func call it*/
void PMX_HalSetTvType(UCHAR ucPmxId, UCHAR ucTvType)
{
	UINT32 u4Tvmode;
	char *szTvType[] = {
		"PMX_TV_TYPE_NTSC",
		"PMX_TV_TYPE_PAL_M",
		"PMX_TV_TYPE_PAL_N",
		"PMX_TV_TYPE_PAL",
	};

	if (ucPmxId == PMX_1)
	{
		u4Tvmode = 0;
		if(ucTvType == PMX_TV_TYPE_NTSC)
		{
			u4Tvmode = 0;
		}
		else if(ucTvType == PMX_TV_TYPE_PAL_M)
		{
			u4Tvmode = 1;
		}
		else if(ucTvType == PMX_TV_TYPE_PAL_N)
		{
			u4Tvmode = 2;
		}
		else if(ucTvType == PMX_TV_TYPE_PAL)
		{
			u4Tvmode = 3;
		}

		if (u4Tvmode <= PMX_TV_TYPE_PAL)
		{
			UTIL_Printf("[PMX] set %s\n", szTvType[u4Tvmode]);
		}

		pDcmPmxDispMainSwReg->rField.u1TVMODE = u4Tvmode;
		//pDcmPmxDispAuxSwReg->rField.u1TVMODE = u4Tvmode;
		//pDcmPmxVdoutMainSwReg->rField.u1TVMODE = u4Tvmode;

		_prPmxDispMainHwReg->au4Reg[(0x94/4)] = pDcmPmxDispMainSwReg->au4Reg[(0x94/4)];
		//_prPmxDispAuxHwReg->au4Reg[(0x94/4)] = pDcmPmxDispAuxSwReg->au4Reg[(0x94/4)];
		//_prPmxVdoutMainHwReg->au4Reg[(0x94/4)] = pDcmPmxVdoutMainSwReg->au4Reg[(0x94/4)];
	}else if(ucPmxId == PMX_2){
		u4Tvmode = 0;
		if(ucTvType == PMX_TV_TYPE_NTSC)
		{
			u4Tvmode = 0;
		}
		else if(ucTvType == PMX_TV_TYPE_PAL_M)
		{
			u4Tvmode = 1;
		}
		else if(ucTvType == PMX_TV_TYPE_PAL_N)
		{
			u4Tvmode = 2;
		}
		else if(ucTvType == PMX_TV_TYPE_PAL)
		{
			u4Tvmode = 3;
		}

		if (u4Tvmode <= PMX_TV_TYPE_PAL)
		{
			UTIL_Printf("[PMX] set %s \n", &szTvType[u4Tvmode]);
		}


	//pDcmPmxDispMainSwReg->rField.u1TVMODE = u4Tvmode;
	pDcmPmxDispAuxSwReg->rField.u1TVMODE = u4Tvmode;
	//pDcmPmxVdoutMainSwReg->rField.u1TVMODE = u4Tvmode;
	WriteREGMsk(0x43124,u4Tvmode<<30,0x3<<30);  //scaler tv type.
	//_prPmxDispMainHwReg->au4Reg[(0x94/4)] = pDcmPmxDispMainSwReg->au4Reg[(0x94/4)];
	_prPmxDispAuxHwReg->au4Reg[(0x94/4)] = pDcmPmxDispAuxSwReg->au4Reg[(0x94/4)];
	//_prPmxVdoutMainHwReg->au4Reg[(0x94/4)] = pDcmPmxVdoutMainSwReg->au4Reg[(0x94/4)];
	}
}
#endif
void vPmxHalSetMode(u8 ucPmxId, u8 ucFmt)
{
}

void vPmxHalSetTvType(u8 ucPmxId, u8 ucTvType)
{
}


void vPmxHalSetAlpha(u8 ucPmxId, u32 ucInAlpha, u32 ucOutAlpha, bool fgEnable)
{
}

void setPmxSwToHw(void)
{
#if 0 //old
	int u4RegIdx = 0;
	for (; u4RegIdx < PMX_HAL_MIX_REG_NUM; u4RegIdx++) {
		if (_rPmxMixRegMode[u4RegIdx] & PMX_HAL_REG_MODE_WRITE)
			_prPmxHalMixHwReg->au4Reg[u4RegIdx] = _rPmxHalMixSwReg.au4Reg[u4RegIdx];
			_rPmxMixRegMode[u4RegIdx] &= ~PMX_HAL_REG_MODE_WRITE;
	}
#endif
#if 0
        __u32 u4RegIdx;
	__u32 tmp = 0;

	/* update plane mixer vdout register at vsync*/
	for (u4RegIdx = 0; u4RegIdx < PMX_HAL_MIX_REG_NUM; u4RegIdx++) {
		if (_rPmxMixRegMode[u4RegIdx] & PMX_HAL_REG_MODE_WRITE) {
			if ((u4RegIdx == 0) && (_rPmxMixRegMode[u4RegIdx] >> 2)) {
				/* Don't enable video layer and primary surface if delay write set*/
				/*tmp = (1 << PRIMARY_SURF_PLANE) | 0x3;*/
				tmp = 0x3;
				_prPmxHalMixHwReg->au4Reg[u4RegIdx] = _rPmxHalMixSwReg.au4Reg[u4RegIdx] & ~tmp;
				_rPmxMixRegMode[u4RegIdx] &= ~PMX_HAL_REG_MODE_WRITE;
			} else {
				_prPmxHalMixHwReg->au4Reg[u4RegIdx] = _rPmxHalMixSwReg.au4Reg[u4RegIdx];
				_rPmxMixRegMode[u4RegIdx] &= ~PMX_HAL_REG_MODE_WRITE;
			}
		}

		if (_rPmxMixRegMode[u4RegIdx] & PMX_HAL_REG_MODE_READ) {
			_rPmxHalMixSwReg.au4Reg[u4RegIdx] = _prPmxHalMixHwReg->au4Reg[u4RegIdx];
			_rPmxMixRegMode[u4RegIdx] &= ~PMX_HAL_REG_MODE_READ;
		}

#if !(PMX_HAL_REG_MODE_DELAY_WRITE == PMX_HAL_REG_MODE_WRITE)

		if (_rPmxMixRegMode[u4RegIdx] >= (PMX_HAL_REG_MODE_WRITE << 1)) {
			_rPmxMixRegMode[u4RegIdx] = _rPmxMixRegMode[u4RegIdx] >> 1;
			VDO_LOG(VDO_LOG_LVL_INFO, "PMX", "MIX delay write mode %d [0x%x] 0x%x -> 0x%x\r\n"
				, (int)_rPmxMixRegMode[u4RegIdx], (unsigned int)(u4RegIdx << 2)
				, (unsigned int)_rPmxHalMixSwReg.au4Reg[u4RegIdx]
				, (unsigned int)_prPmxHalMixHwReg->au4Reg[u4RegIdx]);
		}

#endif
        }
#endif
}

void vPmxMixPlane(unsigned char ucPmxId, unsigned int u4Plane)
{
	if (ucPmxId == PMX_1)
	{
		switch (u4Plane)
		{
			case PMX_HW_VIDEO_MIX:
				if (!_rPmxHalMixSwReg.rField.fgVIDEO_MIX_EN) {
					_rPmxHalMixSwReg.rField.fgVIDEO_MIX_EN = 1;
					_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_DELAY_WRITE;
				}
				//_rPmxHalMixSwReg.rField.fgVIDEO_MIX_EN = 1;
				break;

			case PMX_HW_OSD1_MIX:
				_rPmxHalMixSwReg.rField.fgOSD1_MIX_EN = 1;
				_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
				break;

			case PMX_HW_OSD2_MIX:
				_rPmxHalMixSwReg.rField.fgOSD2_MIX_EN = 1;
				_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
				break;

			case PMX_HW_OSD3_MIX:
				_rPmxHalMixSwReg.rField.fgOSD3_MIX_EN = 1;
				_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
				break;

			case PMX_HW_PLANE_6:
				//_rPmxHalMixSwReg.rField.fgOSD4_MIX_EN = 1;
				break;

			case PMX_HW_OSD4_MIX:
				_rPmxHalMixSwReg.rField.fgOSD4_MIX_EN = 1;
				_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
				break;

			case PMX_HW_PLANE_8:
			default:
				break;
		}

		//_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;

	}
}

void vPmxNotMixPlane(unsigned char ucPmxId, unsigned int u4Plane)
{
	if (ucPmxId == PMX_1)
	{
		switch (u4Plane)
		{
			case PMX_HW_VIDEO_MIX:
				if (_rPmxHalMixSwReg.rField.fgVIDEO_MIX_EN) {
						_rPmxHalMixSwReg.rField.fgVIDEO_MIX_EN = 0;
						_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
				}
				break;
			case PMX_HW_OSD1_MIX:
				_rPmxHalMixSwReg.rField.fgOSD1_MIX_EN = 0;
				_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
				break;
			case PMX_HW_OSD2_MIX:
				_rPmxHalMixSwReg.rField.fgOSD2_MIX_EN = 0;
				_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
				break;
			case PMX_HW_OSD3_MIX:
				_rPmxHalMixSwReg.rField.fgOSD3_MIX_EN = 0;
				_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
				break;
			case PMX_HW_OSD4_MIX:
				_rPmxHalMixSwReg.rField.fgOSD4_MIX_EN = 0;
				_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
				break;
			case PMX_HW_PLANE_6:
				//_rPmxHalMixSwReg.rField.fgOSD5_MIX_EN = 0;
				break;

			case PMX_HW_PLANE_8:
			default:
				break;
		}
		//_rPmxMixRegMode[(0 / 4)] |= PMX_HAL_REG_MODE_WRITE;

	}
}

void vPmxHalMixPlane(u8 ucPmxId, u32 u4Plane)
{

}

void vPmxHalNotMixPlane(u8 ucPmxId, u32 u4Plane)
{

}

bool fgPmxHalMixPlane(u8 ucPmxId, u32 u4Plane)
{
	bool fgMixPlane = FALSE;
	return fgMixPlane;
}

void vPmxHalSetFullRange(u8 ucPmxId, bool fgEnable, bool fg2352255)
{
	u32 value = FmtReadREG(0x00F8);

	if (ucPmxId == PMX_1)
	{
		if (fgEnable) {
			value |= 0x40;
		} else {
			value &= 0xFFFFFFBF;
		}

		if (fg2352255) {
			value |= 0x80;
		} else {
			value &= 0xFFFFFF7F;
		}

		FmtWriteREG(0x00F8, value);
	}
	else
	{
		pDcmPmxDispAuxSwReg->rField.fg235_TO_255_EN = fgEnable;
		pDcmPmxDispAuxSwReg->rField.fgDATA_235_255 = fg2352255;
		//REG_SET(*pPmxDispAuxRegMode, REG_MASK (0xf8 / 4));
		_rPmxDispAuxRegMode[(0xf8 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	}
}

void vPmxHalSet709To601(u8 ucPmxId, bool fgEnable, bool fg7092601)
{

}

void vPmxHalSetFmtPhase(UCHAR ucPmxId, PMX_PHASE_TYPE_E eYPhase, PMX_PHASE_TYPE_E eCPhase)
{
	if (ucPmxId == PMX_1)
	{
		if ((PMX_PAHSE_TYPE_32 == eYPhase) && (PMX_PAHSE_TYPE_32 == eCPhase))
		{
			_prPmxDispMainHwReg->rField.fgUsePhase32 = 1;
		}
		else
		{
			if (PMX_PAHSE_TYPE_16 == eYPhase)
			{
				_prPmxDispMainHwReg->rField.fgUsePhase32 = 0;
				_prPmxDispMainHwReg->rField.fgPHASE_16 = 1;
			}
			else if (PMX_PAHSE_TYPE_8 == eYPhase) // 8 phase
			{
				_prPmxDispMainHwReg->rField.fgUsePhase32 = 0;
				_prPmxDispMainHwReg->rField.fgPHASE_16 = 0;
			}
			if (PMX_PAHSE_TYPE_16 == eCPhase)
			{
				_prPmxDispMainHwReg->rField.fgUsePhase32 = 0;
				_prPmxDispMainYHwReg->rField.fgC_16_PHAEE = 1;
			}
			else if (PMX_PAHSE_TYPE_8 == eCPhase)// 8 phase
			{
				_prPmxDispMainHwReg->rField.fgUsePhase32 = 0;
				_prPmxDispMainYHwReg->rField.fgC_16_PHAEE = 0;
			}
		}
		REG_SET(*pPmxDispMainRegMode, REG_MASK (0xe4 / 4));
		REG_SET(*pPmxDispMainRegMode, REG_MASK (0xb0 / 4));
		REG_SET(*pPmxDispMainRegMode, REG_MASK (0x48 / 4));
	}
	else
	{
		if ((PMX_PAHSE_TYPE_32 == eYPhase) && (PMX_PAHSE_TYPE_32 == eCPhase))
		{
			pDcmPmxDispAuxSwReg->rField.fgUsePhase32 = 1;
		}
		else
		{
			if (PMX_PAHSE_TYPE_16 == eYPhase)
			{
				pDcmPmxDispAuxSwReg->rField.fgUsePhase32 = 0;
				pDcmPmxDispAuxSwReg->rField.fgPHASE_16 = 1;
			}
			else // 8 phase
			{
				pDcmPmxDispAuxSwReg->rField.fgUsePhase32 = 0;
				pDcmPmxDispAuxSwReg->rField.fgPHASE_16 = 0;
			}

			if (PMX_PAHSE_TYPE_16 == eCPhase)
			{
				pDcmPmxDispAuxSwReg->rField.fgUsePhase32 = 0;
#if 0
				pDcmPmxDispAuxSwReg->rField.fgC_16_PHAEE = 1;
#endif
			}
			else // 8 phase
			{
				pDcmPmxDispAuxSwReg->rField.fgUsePhase32 = 0;
#if 0
				pDcmPmxDispAuxSwReg->rField.fgC_16_PHAEE = 0;
#endif
			}
		}
		REG_SET(*pPmxDispAuxRegMode, REG_MASK (0xe4 / 4));
		REG_SET(*pPmxDispAuxRegMode, REG_MASK (0xb0 / 4));
		REG_SET(*pPmxDispAuxRegMode, REG_MASK (0x48 / 4));
	}
}

void vPmxHalSetSDMode(UCHAR ucPmxId, PMX_SD_MODE_TYPE_E eSDModeType)
{
	HAL_PMX_DISP_MAIN_UNION_T *pDcmPmxDispSwReg_temp;
	UINT64 *pPmxDispRegMode_temp;

	if (ucPmxId == PMX_1){
		pDcmPmxDispSwReg_temp = pDcmPmxDispMainSwReg;
		pPmxDispRegMode_temp = pPmxDispMainRegMode;
	}else{
		pDcmPmxDispSwReg_temp = pDcmPmxDispAuxSwReg;
		pPmxDispRegMode_temp = pPmxDispAuxRegMode;
	}
	if (PMX_SD_MODE_TYPE_NEW_16 == eSDModeType)
	{
		pDcmPmxDispSwReg_temp->rField.fgDIRECT = 1;
		pDcmPmxDispSwReg_temp->rField.fgNEW_SD_MODE = 1;
		pDcmPmxDispSwReg_temp->rField.fgHD_ON = 0;
	}
	else if (PMX_SD_MODE_TYPE_NEW_8 == eSDModeType)
	{
		pDcmPmxDispSwReg_temp->rField.fgDIRECT = 0;
		pDcmPmxDispSwReg_temp->rField.fgNEW_SD_MODE = 1;
		pDcmPmxDispSwReg_temp->rField.fgHD_ON = 0;
	}
	else if (PMX_SD_MODE_TYPE_OLD_16 == eSDModeType)
	{
		pDcmPmxDispSwReg_temp->rField.fgDIRECT = 1;
		pDcmPmxDispSwReg_temp->rField.fgNEW_SD_MODE = 0;
		pDcmPmxDispSwReg_temp->rField.fgHD_ON = 0;
	}
	else
	{
		pDcmPmxDispSwReg_temp->rField.fgDIRECT = 0;
		pDcmPmxDispSwReg_temp->rField.fgNEW_SD_MODE = 0;
		pDcmPmxDispSwReg_temp->rField.fgHD_ON = 0;
	}

	REG_SET(*pPmxDispRegMode_temp, REG_MASK (0xe4 / 4));
	REG_SET(*pPmxDispRegMode_temp, REG_MASK (0x94 / 4));

}

extern volatile BOOL g_rearFieldSwith;

/******************************************************************************
* Local Function
******************************************************************************/
extern int g_cnt;
irqreturn_t vPmxHalMainIsr(int u2Vector, void *dev_id)
{
	/*vVdpIsr(0);*/
#if 0

	if (PmxVerifyDoRestInVyncFront == TRUE) {
		_prPmxDispMainHwReg->rField.fgFTRST = 0;
		_prPmxDispMainHwReg->rField.fgFTRST = 1;
		_prPmxDispMainHwReg->rField.fgFTRST = 0;

		PmxVerifyDoRestInVyncFront = FALSE;
	}
	//VDO_LOG(VDO_LOG_LVL_INFO, " PmxVerifyDoRestInVyncFront00000\r\n");

	pDcmPmxDispMainIsrSwReg = &_rPmxDispMainSwReg;
	pDcmPmxDispMainIsrMode = (UINT64 *)&_rPmxDispMainRegMode;

	/* update plane mixer display register at vsync*/
	for (u4RegIdx = 0; u4RegIdx < HAL_PMX_DISP_MAIN_REG_NUM; u4RegIdx++) {
		if (_rPmxDispMainRegMode[u4RegIdx] & PMX_HAL_REG_MODE_WRITE) {
			_prPmxDispMainHwReg->au4Reg[u4RegIdx] = _rPmxDispMainSwReg.au4Reg[u4RegIdx];
			_rPmxDispMainRegMode[u4RegIdx] &= ~PMX_HAL_REG_MODE_WRITE;
		}

		if (_rPmxDispMainRegMode[u4RegIdx] & PMX_HAL_REG_MODE_READ) {
			_rPmxDispMainSwReg.au4Reg[u4RegIdx] = _prPmxDispMainHwReg->au4Reg[u4RegIdx];
			_rPmxDispMainRegMode[u4RegIdx] &= ~PMX_HAL_REG_MODE_READ;
		}


#if !(PMX_HAL_REG_MODE_DELAY_WRITE == PMX_HAL_REG_MODE_WRITE)
		if (_rPmxDispMainRegMode[u4RegIdx] >= (PMX_HAL_REG_MODE_WRITE << 1)) {
			_rPmxDispMainRegMode[u4RegIdx] = _rPmxDispMainRegMode[u4RegIdx] >> 1;
			VDO_LOG(VDO_LOG_LVL_INFO, "FMTF delay write mode %d [0x%x] 0x%x -> 0x%x\r\n"
				, (int)_rPmxDispMainRegMode[u4RegIdx], (unsigned int)(u4RegIdx << 2)
				, (unsigned int)_rPmxDispMainSwReg.au4Reg[u4RegIdx]
				, (unsigned int)_prPmxDispMainHwReg->au4Reg[u4RegIdx]);
		}

#endif
	}

#endif
#ifndef ATC_ARM2_LK_SUPPORT

	/*_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
	_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;*/
#else
	_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
	_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;

	//v_clear_bim_irq(u2Vector);
	WriteREGMsk(0x2000004,1,1);
	WriteREGMsk(0x2000004,0,1);

#endif

	return IRQ_HANDLED;
}
#if 0
#ifndef ATC_ARM2_LK_SUPPORT
irqreturn_t vPmxHalAuxIsr(int u2Vector, void *dev_id)
{
        unsigned int u4RegIdx;
	UINT64 regMask;
	HAL_PMX_DISP_MAIN_UNION_T *pDcmPmxDispAuxIsrSwReg;
	UINT64 *pDcmPmxDispAuxIsrMode;
	HAL_PMX_DISP_MAIN_C_UNION_T *pDcmPmxDispAuxCIsrSwReg;
	UINT64 *pDcmPmxDispAuxCIsrMode;

	if(g_rearFieldSwith){
		/* switch field*/
		volatile unsigned int temp = (~(ReadREG(0x43430)>>5)) & 0x1;
		WriteREGMsk(0x43430,1<<6,1<<6);
		WriteREGMsk(0x4341c,1<<13,1<<13);
		WriteREGMsk(0x43430,temp<<5,1<<5);

	}

	vVdpIsr(1);

	if (PmxVerifyDoRestInVyncRear == TRUE) {
		_prPmxDispAuxHwReg->rField.fgFTRST = 0;
		_prPmxDispAuxHwReg->rField.fgFTRST = 1;
		_prPmxDispAuxHwReg->rField.fgFTRST = 0;

		PmxVerifyDoRestInVyncRear = FALSE;
	}

#if 0 //verify code
	if(PmxVerifyDoRestInVyncRear==TRUE){

		WriteREGMsk(0x43430,0<<5,1<<5);
		WriteREGMsk(0x430ac,(0x1<<10),0x1<<10);

		WriteREG(0x43120,0x403);
		WriteREG(0x43120,0x003);

		WriteREG(0X4343C,0xFF);
		WriteREG(0X4343C,0x00);

	}
#endif

	// dispfmt_pip
	pDcmPmxDispAuxIsrSwReg = &_rPmxDispAuxSwReg;
	pDcmPmxDispAuxIsrMode = (UINT64 *)&_rPmxDispAuxRegMode;
	// dispfmt_pip CS
	pDcmPmxDispAuxCIsrSwReg = &_rPmxDispAuxCSwReg;
	pDcmPmxDispAuxCIsrMode = (UINT64 *)&_rPmxDispAuxCRegMode;

	for (u4RegIdx = 0, regMask = 1; u4RegIdx < HAL_PMX_DISP_AUX_REG_NUM;
	u4RegIdx++, regMask <<= 1)
	{
		if (IS_REG_SET (*pDcmPmxDispAuxIsrMode, regMask))
			_prPmxDispAuxHwReg->au4Reg[u4RegIdx] = pDcmPmxDispAuxIsrSwReg->au4Reg[u4RegIdx];
	}

	*pDcmPmxDispAuxIsrMode = 0;

	for (u4RegIdx = 0, regMask = 1; u4RegIdx < HAL_PMX_DISP_AUX_C_REG_NUM;
	u4RegIdx++, regMask <<= 1)
	{
		if (IS_REG_SET (*pDcmPmxDispAuxCIsrMode, regMask))
			_prPmxDispAuxCHwReg->au4Reg[u4RegIdx] = pDcmPmxDispAuxCIsrSwReg->au4Reg[u4RegIdx];
	}

	*pDcmPmxDispAuxCIsrMode = 0;


	// clear global isr
	//mt33xx_mask_ack_bim_irq(u2Vector);

	return IRQ_HANDLED;
}
#endif
#endif
extern unsigned int fmtf_irq;
void vPmxHalIsrInit(void)
{
	firstBuf = true;
	buffernum = 0;

	if (!_fgPmxMainIsrInited) {

		ISRWriteREG(0x0100,0x00000000);
		/* init local variable*/
		_fgPmxMainIsrInited = TRUE;
	}
	VDO_LOG(VDO_LOG_LVL_INFO, "vPmxHalIsrInit fmtf irq reigster end\r\n");
}

void vPmxHalIsrStop(u8 ucPmxId)
{
	if (_fgPmxMainIsrInited == TRUE) {
#ifndef ATC_ARM2_LK_SUPPORT
		setBuffer= false;
		ISRWriteREG(0x0100,0xFFFFFFFF);
		free_irq(fmtf_irq, NULL);
#endif
		_fgPmxMainIsrInited = FALSE;
	}
}

void vPmxHalEnableFmt(u8 ucPmxId)
{
	if (ucPmxId == PMX_1) {
		/*reset vdout fmt first*/
		_prPmxDispMainHwReg->rField.fgVDO_EN = 1;
		_prPmxDispMainHwReg->rField.fgFTRST = 1;
		/* FTRST is sw reset and write only register, so need read back after setting*/
		_rPmxDispMainRegMode[(0xac / 4)] |= PMX_HAL_REG_MODE_READ | PMX_HAL_REG_MODE_WRITE;
	} else {
		/*reset vdout fmt first*/
		_rPmxDispAuxSwReg.rField.fgVDO_EN = 1;
		_rPmxDispAuxSwReg.rField.fgFTRST = 1;
		/* FTRST is sw reset and write only register, so need read back after setting*/
		_rPmxDispAuxRegMode[(0xac / 4)] |= PMX_HAL_REG_MODE_READ | PMX_HAL_REG_MODE_WRITE;
	}
}

void vPmxHalDisableFmt(u8 ucPmxId)
{
	if (ucPmxId == PMX_1) {
		_prPmxDispMainHwReg->rField.fgVDO_EN = 0;
		_rPmxDispMainSwReg.rField.fgVDO_EN = _prPmxDispMainHwReg->rField.fgVDO_EN;
	} else {
		_prPmxDispAuxHwReg->rField.fgVDO_EN = 0;
		_rPmxDispAuxSwReg.rField.fgVDO_EN = _prPmxDispAuxHwReg->rField.fgVDO_EN;
	}
}

bool vPmxHalGetFmtEn(u8 ucPmxId)
{
	if (ucPmxId == PMX_1) {
		return _prPmxDispMainHwReg->rField.fgVDO_EN;
	} else {
		return _prPmxDispAuxHwReg->rField.fgVDO_EN;
	}
}

void vPmxHalSetPlaneOrder(u8 ucPmxId, u32 u4PlaneOrder)
{
	VDO_LOG(VDO_LOG_LVL_INFO, "vPmxHalSetPlaneOrder: id = %d, order = %x\r\n", ucPmxId, (unsigned int)u4PlaneOrder);
        //[xzr]todo
}

u32 vPmxHalGetPlaneOrder(u8 ucPmxId)
{
	u32 u4PlaneOrder = 0;
	//[xzr] todo
	VDO_LOG(VDO_LOG_LVL_INFO, "vPmxHalGetPlaneOrder: id = %d, order = %x\r\n", ucPmxId, (unsigned int)u4PlaneOrder);
	return u4PlaneOrder;
}

void vPmxHalTrigerDIModule(u8 ucPmxId)
{
		VDO_LOG(VDO_LOG_LVL_DBG, "vPmxHalTrigerDIModule start \r\n");

		_prPmxDispMainHwReg->rField.fgFTRST = 1;
		//_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
		//_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;

		_prPmxDispMainHwReg->rField.fgVDO_EN = 1;

		_prPmxDispMainHwReg->rField.fgFMTM = 0;
		_prPmxDispMainHwReg->rField.fgFMTM = 1;
		//_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
		//_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;
		return;
#if 0
		unsigned int u4RegIdx;
		static int dropNum =0;
		//unsigned int u4Udf;
		UINT64 regMask;
		HAL_PMX_DISP_MAIN_UNION_T *pDcmPmxDispMainIsrSwReg;
		UINT64 *pDcmPmxDispMainIsrMode;

		HAL_PMX_DISP_MAIN_C_UNION_T *pDcmPmxDispMainCIsrSwReg;
		UINT64 *pDcmPmxDispMainCIsrMode;
		HAL_PMX_DISP_MAIN_C_UNION_T *pDcmPmxDispMainYIsrSwReg;
		UINT64 *pDcmPmxDispMainYIsrMode;
		/*VDO_LOG(VDO_LOG_LVL_INFO, "vPmxHalTrigerDIModule start 11000\r\n");*/
		if(getBufferDone) {
			dropNum =0;
			getBufferDone = false;
			VDO_LOG(VDO_LOG_LVL_INFO, "vPmxHalTrigerDIModule dropNum is %d \r\n",dropNum);
		} else {
			dropNum++;
			_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
			_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;
			return;
		}

		vVdpIsr(0);

		VDO_LOG(VDO_LOG_LVL_INFO, " fmtf_reg = %x pa=%x\r\n",fmtf_reg, __pa(fmtf_reg));
		//_prPmxDispMainHwReg = (HAL_PMX_DISP_MAIN_UNION_T*)fmtf_reg;

		if (PmxVerifyDoRestInVyncFront == TRUE) {
			_prPmxDispMainHwReg->rField.fgFTRST = 0;
			_prPmxDispMainHwReg->rField.fgFTRST = 1;
			_prPmxDispMainHwReg->rField.fgFTRST = 0;

			PmxVerifyDoRestInVyncFront = FALSE;
		}
		//VDO_LOG(VDO_LOG_LVL_INFO, " PmxVerifyDoRestInVyncFront00000\r\n");

        #if 0 //verify code
		if(TRUE == PmxVerifyDoRestInVyncFront){
			WriteREGMsk(0x420ac,(0x1<<10),0x1<<10);
			WriteREGMsk(0xa46ac,(0x1<<10),0x1<<10);

			WriteREG(0X4243C,0xFF);
			WriteREG(0X4243C,0x00);
			WriteREG(0X4243C,0xFF);
			WriteREG(0X4243C,0x00);
			VDO_LOG(VDO_LOG_LVL_IRQ, "rest vdo 0 in vysnc isr\n");
		}
        #endif

		pDcmPmxDispMainIsrSwReg = &_rPmxDispMainSwReg;
		pDcmPmxDispMainIsrMode = (UINT64 *)&_rPmxDispMainRegMode;


	// dispfmt CS
	//  pDcmPmxDispMainCIsrSwReg = &_rPmxDispMainCSwReg;
	// pDcmPmxDispMainCIsrMode = (UINT64 *)&_rPmxDispMainCRegMode;

		//pDcmPmxDispMainYIsrSwReg = &_rPmxDispMainYSwReg;
		//pDcmPmxDispMainYIsrMode = (UINT64*)&_rPmxDispMainYRegMode;

		/* update plane mixer display register at vsync*/

		for (u4RegIdx = 0; u4RegIdx < HAL_PMX_DISP_MAIN_REG_NUM; u4RegIdx++) {

			if (_rPmxDispMainRegMode[u4RegIdx] & PMX_HAL_REG_MODE_WRITE) {
				_prPmxDispMainHwReg->au4Reg[u4RegIdx] = _rPmxDispMainSwReg.au4Reg[u4RegIdx];
				_rPmxDispMainRegMode[u4RegIdx] &= ~PMX_HAL_REG_MODE_WRITE;
			}

			if (_rPmxDispMainRegMode[u4RegIdx] & PMX_HAL_REG_MODE_READ) {
				_rPmxDispMainSwReg.au4Reg[u4RegIdx] = _prPmxDispMainHwReg->au4Reg[u4RegIdx];
				_rPmxDispMainRegMode[u4RegIdx] &= ~PMX_HAL_REG_MODE_READ;
			}


#if !(PMX_HAL_REG_MODE_DELAY_WRITE == PMX_HAL_REG_MODE_WRITE)

			if (_rPmxDispMainRegMode[u4RegIdx] >= (PMX_HAL_REG_MODE_WRITE << 1)) {
				_rPmxDispMainRegMode[u4RegIdx] = _rPmxDispMainRegMode[u4RegIdx] >> 1;
				VDO_LOG(VDO_LOG_LVL_INFO, "FMTF delay write mode %d [0x%x] 0x%x -> 0x%x\r\n"
					, (int)_rPmxDispMainRegMode[u4RegIdx], (unsigned int)(u4RegIdx << 2)
					, (unsigned int)_rPmxDispMainSwReg.au4Reg[u4RegIdx]
					, (unsigned int)_prPmxDispMainHwReg->au4Reg[u4RegIdx]);
			}

#endif
		}
		//setPmxSwToHw();

        #if 0
		for (u4RegIdx = 0, regMask = 1; u4RegIdx < HAL_PMX_DISP_MAIN_Y_REG_NUM;
		u4RegIdx++, regMask <<= 1)
		{
		    if (IS_REG_SET (*pDcmPmxDispMainYIsrMode, regMask))
			_prPmxDispMainYHwReg->au4Reg[u4RegIdx] = pDcmPmxDispMainYIsrSwReg->au4Reg[u4RegIdx];
		}

		*pDcmPmxDispMainYIsrMode = 0;
		for (u4RegIdx = 0, regMask = 1; u4RegIdx < HAL_PMX_DISP_MAIN_REG_NUM;
		u4RegIdx++, regMask <<= 1)
		{
		    if (IS_REG_SET (*pDcmPmxDispMainIsrMode, regMask))
		    {
			{
			    _prPmxDispMainHwReg->au4Reg[u4RegIdx] = pDcmPmxDispMainIsrSwReg->au4Reg[u4RegIdx];
			}
		    }
		}
		*pDcmPmxDispMainIsrMode = 0;

		for (u4RegIdx = 0, regMask = 1; u4RegIdx < HAL_PMX_DISP_MAIN_C_REG_NUM;
		u4RegIdx++, regMask <<= 1)
		{
		    if (IS_REG_SET (*pDcmPmxDispMainCIsrMode, regMask))
			_prPmxDispMainCHwReg->au4Reg[u4RegIdx] = pDcmPmxDispMainCIsrSwReg->au4Reg[u4RegIdx];
		}

		*pDcmPmxDispMainCIsrMode = 0;
        #endif

		// clear global isr
#ifndef ATC_ARM2_LK_SUPPORT
		VDO_LOG(VDO_LOG_LVL_INFO, "vPmxHalTrigerDIModule start 2222\r\n");

		//mt33xx_mask_ack_bim_irq(u2Vector);
		//ISRWriteREGMsk(0x0004,1,1);
		//ISRWriteREGMsk(0x0004,0,1);
		_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
		_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;
#else
		 _prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 1;
		_prPmxDispMainHwReg->rField.fgDI_AGENT_TRIG = 0;

		//v_clear_bim_irq(u2Vector);
		WriteREGMsk(0x2000004,1,1);
		WriteREGMsk(0x2000004,0,1);


#endif
#endif
		//VDO_LOG(VDO_LOG_LVL_INFO, "vPmxHalTrigerDIModule start 3333\r\n");

		return ;
}

void vPmxHalSetPlaneDstColorKey(u8  ucPmxId, bool fgEnable)
{

}

void vPmxHalResume(u8 ucPmxId)
{
	//[xzr]todo
	vVdpHalResume(ucPmxId);
}

void vPmxHalMixIsr(void)
{
	//[xzr]todo?
}
#if 0
void vPmxHalDispFmtHFilter(__u8 ucVdoId, __u8 ucYC, __u8 ucCoef)
	{
	unsigned int u4FmtCRegBase = 0;
	unsigned int u4FmtYRegBase = 0;
	unsigned int u4i = 0, u4Size = 0;

	if(ucVdoId == 0)
	{
		u4FmtYRegBase = 0x42000;
		u4FmtCRegBase = 0x42d00;
	}
	else
	{
		VDO_LOG(VDO_LOG_LVL_INFO, "vPmxHalDispFmtHFilter: now, vdo%d not support HFilter select\r\n", ucVdoId);
		return;
	}

	u4Size = _ArVdoHCoefSetting[ucCoef].size;

	if ((ucYC & (1<<0)) == 1)
	{
		for(u4i = 0; u4i < u4Size-1; u4i ++)
		{
			WriteREG((u4FmtYRegBase + (u4i*4)), _ArVdoHCoefSetting[ucCoef].pu4RegSetting[u4i]);
		}

		WriteREG((u4FmtYRegBase + 0x4C), _ArVdoHCoefSetting[ucCoef].pu4RegSetting[u4Size-1]);
	}
	if ((ucYC & (1<<1)) == (1<<1))
	{
		for(u4i = 0; u4i < u4Size-1; u4i ++)
		{
			WriteREG((u4FmtCRegBase + (u4i*4)), _ArVdoHCoefSetting[ucCoef].pu4RegSetting[u4i]);
		}

		WriteREG((u4FmtCRegBase + 0x4C), _ArVdoHCoefSetting[ucCoef].pu4RegSetting[u4Size-1]);
	}

	_rPmxDispMainSwReg.rField.fgHSLR = 0;
	_rPmxDispMainSwReg.rField.fgHD_C_FIR_EN = 1;

	_rPmxDispMainRegMode[(0xb0 / 4)] |= PMX_HAL_REG_MODE_WRITE;
	_rPmxDispMainRegMode[(0xe4 / 4)] |= PMX_HAL_REG_MODE_WRITE;
}
#endif

#ifndef ATC_ARM2_LK_SUPPORT
EXPORT_SYMBOL(vPmxDIWork);
EXPORT_SYMBOL(vPmxHalInit);
EXPORT_SYMBOL(vPmxHalReset);
EXPORT_SYMBOL(vPmxHalSetMasterMode);
EXPORT_SYMBOL(vPmxMixPlane);
EXPORT_SYMBOL(vPmxNotMixPlane);
EXPORT_SYMBOL(vPmxHalMixPlane);
EXPORT_SYMBOL(vPmxHalNotMixPlane);
EXPORT_SYMBOL(vPmxHalIsrInit);
EXPORT_SYMBOL(vPmxHalEnableFmt);
EXPORT_SYMBOL(vPmxHalDisableFmt);
EXPORT_SYMBOL(vPmxHalSetPlaneOrder);
EXPORT_SYMBOL(vPmxHalGetPlaneOrder);
EXPORT_SYMBOL(vPmxHalTrigerDIModule);
EXPORT_SYMBOL(di_thread_deinit);
EXPORT_SYMBOL(di_thread_init);
#endif

#endif /* _PMX_VSYNC_C_ */
