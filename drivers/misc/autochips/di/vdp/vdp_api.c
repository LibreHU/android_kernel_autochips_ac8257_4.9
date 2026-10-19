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


#ifndef ATC_ARM2_LK_SUPPORT
#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/videodev2.h>
#include <linux/mm.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include "display_inc.h"
#include "pmx_hal.h"
#include "wch_if.h"

//#include <media/atc/drv_av_d.h>
#include "display.h"
#include "display_inc.h"
#include <ac8257_addr_utils.h>
#include "di_hal.h"
//#include "windows.h"
//#include "imgresz_hal_if.h"
//#include "drv_imgresz.h"

#else
#include "platform/mt_typedefs.h"
#include "sys/types.h"
#include "di_types.h"
#include <kernel/thread.h>
#include <arch/ops.h>

#include "display_inc.h"
#include "pmx_hal.h"
#include "display.h"
#include "errno-base.h"
#endif

#include "drv_pmx.h"
#include "vdp_hal.h"

#include "vdp.h"
#include "log.h"

#include "bringup.h"

#ifdef CONFIG_ATC_OS_linux
#define ENABLE_MM_DEINTERLACE     1
#define ENABLE_PD_DEINTERLACE     1
#define ENABLE_HD_DEINTERLACE     1
#define LINUX_FUN                 1
#else
#define ENABLE_MM_DEINTERLACE     0
#define ENABLE_PD_DEINTERLACE     0
#define ENABLE_HD_DEINTERLACE     0
#define LINUX_FUN                 0
#endif

#define IMG_RESZ_DST_BUFF_WIDTH   (_u4LCDWidth)      /* Max panel width*/
#ifdef CONFIG_ATC_OS_linux
#define IMG_RESZ_DST_BUFF_HEIGHT  (_u4LCDHeight << 1)  /* Max panel height * 2 for HD deinterlace source*/
#else
#define IMG_RESZ_DST_BUFF_HEIGHT  (_u4LCDHeight)  /* Max panel height * 2 for HD deinterlace source*/
#endif
#define IMG_RESZ_SD_BUFF_WIDTH    (720)      /* Max panel width*/
#define IMG_RESZ_SD_BUFF_HEIGHT   (480)      /* Max panel height * 2 for HD deinterlace source*/

#define VDP_BACKUP 2   /* Back up front video*/

#define VDP_ASSERT(argu) ASSERT(argu)

#define SIZE_ALIGN(x, n)  ((x + n - 1) / n * n)

#define ROUND_UP_COUNT(Count, Pow2)          (((Count)+(Pow2)-1) & (~(((LONG)(Pow2))-1)))

#define VALID_BUFF_COUNT(Cur, Disp, Count)   (((Cur - Disp + MAX_BUFF_CNT) % MAX_BUFF_CNT) > Count)

#define INVALID_BUFF_COUNT(Cur, Disp)        (((Disp + 1) % MAX_BUFF_CNT) == Cur)

#define CALCULATE_VSYNC_CNT(Duration, Vsync) ((Duration + Vsync / 2) / Vsync)

#define MONITOR_QBUF_TIME 0

#define SPIN_LOCK_SRC_ISRF   0
#define SPIN_LOCK_SRC_ISRR   1
#define SPIN_LOCK_SRC_IOCTL  2
#define SPIN_LOCK_SRC_RESIZE 3

#ifndef ATC_ARM2_LK_SUPPORT
static spinlock_t _vdp_lock;
static u32 u4VdpLockCnt;
static bool fgLockInit = FALSE;

#endif

static bool _fgInitVdpParam[2] = {FALSE, FALSE};
static bool _fgBypassFstVysnc[2] = {TRUE, TRUE};
VDP_PARAM rData[3] = {{0}, {0}, {0} };


#if MONITOR_VDO_FPS
u32 vdo_fps_en[2] = {0, 0};
u32 vdo_flip_cnt[2] = {0, 0};
#endif

#if MONITOR_QBUF_TIME
struct timeval start_t[2];
static u32 u4TmpCnt;
static bool fgStartCnt = FALSE;
#endif

/*static char vdp_name[100];
#define vdp_aee_print(string, args...) do {\
	snprintf(vdp_name, 100, "[DI][VDP]"string, ##args); \
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_MMPROFILE_BUFFER, vdp_name, "[DI][VDP] error:"string, ##args);  \
} while (0)*/

u32 fr_follow = 0;
bool vdo_fr_on = FALSE;
const char *_pcVideoLogLevel[] = {
	"[VDO OFF]",
	"[VDO ERR]",
	"[VDO WARN]",
	"[VDO INFO]",
	"[VDO HAL]",
	"[VDO DBG]",
	"[VDO IRQ]",
	"[VDO REGRW]",
};

void SpinLock(u32 u4LockSrc)
{
#ifndef ATC_ARM2_LK_SUPPORT
	spin_lock(&_vdp_lock);
	u4VdpLockCnt++;

	VDO_LOG(VDO_LOG_LVL_IRQ, "VDP Spinlock  SRC =%d, count=%d \n", (int)u4LockSrc, (int)u4VdpLockCnt);

#else
	enter_critical_section();
#endif
}

void SpinUnlock(u32 u4LockSrc)
{
#ifndef ATC_ARM2_LK_SUPPORT
	u4VdpLockCnt--;
	spin_unlock(&_vdp_lock);

	VDO_LOG(VDO_LOG_LVL_IRQ, "VDP SpinUnlock SRC =%d, count=%d \n", (int)u4LockSrc, (int)u4VdpLockCnt);

#else
	exit_critical_section();
#endif
}

bool fgResetParam(VDP_PARAM *prParam)
{

	memset(prParam, 0, sizeof(VDP_PARAM));

	prParam->u4ClearIdx = 0xFF;
	prParam->fgFirstField = TRUE;
	prParam->fgTopFiledFirst = TRUE;
	prParam->fgRepeatFirstField = TRUE;
#if MONITOR_QBUF_TIME
	u4TmpCnt = 0;
	fgStartCnt = FALSE;
#endif
	return TRUE;
}
u32 startIdx = 1, endIdx = 0;

bool fgSetBufState(VDP_PARAM *prParam, u32 u4StartIdx, u32 u4EndIdx)
{
	u32 u4Idx = 0;
	AVIN_BUFFER_T *clearBufferinfo = NULL;

	if (u4StartIdx > u4EndIdx) {
		return FALSE;
	}
	for (u4Idx = u4StartIdx; u4Idx <= u4EndIdx; u4Idx++) {
		if (prParam->u4FrmBuffY[u4Idx] & prParam->u4FrmBuffC[u4Idx]) {
			clearBufferinfo = (AVIN_BUFFER_T *)prParam->u4BufferInfor[u4Idx];
#ifndef ATC_ARM2_LK_SUPPORT
			if(clearBufferinfo != NULL) {
				atomic_set(&clearBufferinfo->status, AVIN_BUF_FREE);
			}
#else
			if(clearBufferinfo != NULL) {
				VDO_LOG(VDO_LOG_LVL_DBG, "%s line %d,buffer index is %d\r\n", __func__, __LINE__, (int)clearBufferinfo->idex);
				atomic_swap(&clearBufferinfo->status, AVIN_BUF_FREE);
			}
#endif
		}
	}
	return TRUE;
}

bool fgClearFrmBuff(VDP_PARAM *prParam, u32 u4StartIdx, u32 u4EndIdx)
{
	u32 u4Idx = 0;
	if (u4StartIdx > u4EndIdx) {
		return FALSE;
	}

	for (u4Idx = u4StartIdx; u4Idx <= u4EndIdx; u4Idx++) {
		if (prParam->u4FrmBuffY[u4Idx] & prParam->u4FrmBuffC[u4Idx]) {
			/* clear buffer*/
			/*VDO_LOG(VDO_LOG_LVL_INFO, "%s Y/C 0x%x 0x%x\r\n", __func__
				, prParam->u4FrmBuffY[u4Idx], prParam->u4FrmBuffC[u4Idx]);*/
			prParam->u4FrmBuffY[u4Idx] = 0;
			prParam->u4FrmBuffC[u4Idx] = 0;
			prParam->u4Duration[u4Idx] = 0;
		}
	}

	return TRUE;
}
#if 0
#ifndef ATC_ARM2_LK_SUPPORT
#define MAX_ISR_CNT    10
static struct atc_dispc_isr_data isr_data[MAX_ISR_CNT];
static bool register_isr[MAX_ISR_CNT];
int atc_dispc_register_isr(atc_dispc_isr_t isr, void *arg)
{
	u32 i = 0;

	if ((isr == NULL) || (arg == NULL)) {
		VDO_LOG(VDO_LOG_LVL_ERR, "atc_dispc_register_isr return EINVAL\r\n");

		return -EINVAL;
	}

	for (i = 0; i < MAX_ISR_CNT; i++) {
		if (isr_data[i].isr == NULL) {
			isr_data[i].isr = isr;
			isr_data[i].arg = arg;
			register_isr[i] = TRUE;
			break;
		} else if ((isr_data[i].isr == isr) && (isr_data[i].arg == arg)) {
			register_isr[i] = TRUE;
			break;
		}
	}

	if (i == MAX_ISR_CNT) {
		VDO_LOG(VDO_LOG_LVL_ERR, "atc_dispc_register_isr no free isr %d\r\n", i);

		return -EBUSY;
	}
	VDO_LOG(VDO_LOG_LVL_INFO, "atc_dispc_register_isr %d\r\n", i);

	return 0;
}
EXPORT_SYMBOL(atc_dispc_register_isr);

int atc_dispc_unregister_isr(atc_dispc_isr_t isr, void *arg)
{
	u32 i = 0;

	if ((isr == NULL) || (arg == NULL)) {
		VDO_LOG(VDO_LOG_LVL_ERR, "atc_dispc_unregister_isr return EINVAL\r\n");

		return -EINVAL;
	}

	for (i = 0; i < MAX_ISR_CNT; i++) {
		if ((isr_data[i].isr == isr) && (isr_data[i].arg == arg)) {
			isr_data[i].isr = NULL;
			isr_data[i].arg = NULL;
			register_isr[i] = FALSE;
			VDO_LOG(VDO_LOG_LVL_INFO, "atc_dispc_unregister_isr %d\r\n", i);

			return 0;
		}
	}
	VDO_LOG(VDO_LOG_LVL_ERR, "atc_dispc_unregister_isr no match isr %d\r\n", i);

	return -EBUSY;
}
EXPORT_SYMBOL(atc_dispc_unregister_isr);
#endif
#endif
void vDumpFrameBuffer(VDP_PARAM *prParam, bool fgFstField, u32 *u4FBufferY)
{
	VDO_LOG(VDO_LOG_LVL_TRACE, "[VDP]vDumpFrameBuffer: first field %d, id %d %d %d, buffer %x, %x, %x, %x\r\n"
		, (int)fgFstField, (int)prParam->u4PrevIdx, (int)prParam->u4DispIdx, (int)prParam->u4CurrIdx
		, (unsigned int)u4FBufferY[0], (unsigned int)u4FBufferY[1]
		, (unsigned int)u4FBufferY[2], (unsigned int)u4FBufferY[3]);
}

bool fgUpdateHwAddr(u32 u4VdpIdx)
{
	VDP_PARAM *prParam = &rData[u4VdpIdx];
	u32 u4PhyAddrY[4];
	u32 u4PhyAddrC[4];
	u32 u4Prev = prParam->u4PrevIdx;
	u32 u4Disp = prParam->u4DispIdx;
	u32 u4Next = (u4Disp + 1) % MAX_BUFF_CNT;

#ifdef MONITOR_VDO_FPS

	if (vdo_fps_en[u4VdpIdx]) {
		vdo_flip_cnt[u4VdpIdx]++;
	}

#endif

	if (prParam->u4DeintMode < VDP_DI_MA4F_MODE) {
		if (prParam->u4FrmBuffY[u4Disp] && prParam->u4FrmBuffC[u4Disp]) {
			vVdpHalSetYBufPtr(u4VdpIdx, prParam->u4FrmBuffY[u4Disp], prParam->u4FrmBuffC[u4Disp]);
			/*VDO_LOG(VDO_LOG_LVL_INFO, "%s: hw id %d, disp %d, cur %d, address %x, %x duration %d\r\n"
				, __func__, u4VdpIdx , u4Disp, prParam->u4CurrIdx, prParam->u4FrmBuffY[u4Disp]
				, prParam->u4FrmBuffC[u4Disp], prParam->u4Duration[u4Disp]);*/
		} else {
			VDO_LOG(VDO_LOG_LVL_ERR, "[VDP]fgUpdateHwAddr: error id %d, disp %d, address %x, %x\r\n"
				, (int)u4VdpIdx, (int)u4Disp, (unsigned int)prParam->u4FrmBuffY[u4Disp]
				, (unsigned int)prParam->u4FrmBuffC[u4Disp]);
			return FALSE;
		}
	} else {
		if (prParam->fgFirstField) {
			u4PhyAddrY[0] = (prParam->u4FrmBuffY[u4Prev]);
			u4PhyAddrY[1] = (prParam->u4FrmBuffY[u4Prev]);
			u4PhyAddrY[2] = (prParam->u4FrmBuffY[u4Disp]);
			u4PhyAddrY[3] = (prParam->u4FrmBuffY[u4Disp]);

			u4PhyAddrC[0] = (prParam->u4FrmBuffC[u4Prev]);
			u4PhyAddrC[1] = (prParam->u4FrmBuffC[u4Prev]);
			u4PhyAddrC[2] = (prParam->u4FrmBuffC[u4Disp]);
			u4PhyAddrC[3] = (prParam->u4FrmBuffC[u4Disp]);
		} else {
			u4PhyAddrY[0] = (prParam->u4FrmBuffY[u4Prev]);
			u4PhyAddrY[1] = (prParam->u4FrmBuffY[u4Disp]);
			u4PhyAddrY[2] = (prParam->u4FrmBuffY[u4Disp]);
			u4PhyAddrY[3] = (prParam->u4FrmBuffY[u4Next]);

			u4PhyAddrC[0] = (prParam->u4FrmBuffC[u4Prev]);
			u4PhyAddrC[1] = (prParam->u4FrmBuffC[u4Disp]);
			u4PhyAddrC[2] = (prParam->u4FrmBuffC[u4Disp]);
			u4PhyAddrC[3] = (prParam->u4FrmBuffC[u4Next]);
		}

		if (u4PhyAddrY[0] && u4PhyAddrY[1] && u4PhyAddrY[2] && u4PhyAddrY[3] &&
		    u4PhyAddrC[0] && u4PhyAddrC[1] && u4PhyAddrC[2] && u4PhyAddrC[3]) {
			/*vDumpFrameBuffer(prParam, prParam->fgFirstField, u4PhyAddrY);*/
			vVdpHalSetDeintWXYZ(u4VdpIdx, u4PhyAddrY, u4PhyAddrC);
		} else {
			vDumpFrameBuffer(prParam, prParam->fgFirstField, u4PhyAddrY);
			return FALSE;
		}
	}

	return TRUE;
}

void vUpdateDispIdx(u32 u4VdpIdx)
{
	VDP_PARAM *prParam = &rData[u4VdpIdx];

	if (prParam->u4PrevIdx == prParam->u4DispIdx) {
		prParam->u4DispIdx = (prParam->u4DispIdx + 1) % MAX_BUFF_CNT;
		prParam->u4ClearIdx = 0xFF;
	} else {
		prParam->u4ClearIdx = prParam->u4PrevIdx;
        startIdx = prParam->u4ClearIdx;
        endIdx = prParam->u4ClearIdx;
		prParam->u4PrevIdx = (prParam->u4PrevIdx + 1) % MAX_BUFF_CNT;
		prParam->u4DispIdx = (prParam->u4DispIdx + 1) % MAX_BUFF_CNT;
	}
}
int buffernum =0;
void vVdpIsr(u32 u4VdpIdx)
{
	VDP_PARAM *prParam = NULL;
	u32 u4DispIdx = 0;
	/*VDO_LOG(VDO_LOG_LVL_ERR, "vVdpIsr  %d\r\n", u4VdpIdx);*/
	if (u4VdpIdx > VDP_2) {
		VDO_LOG(VDO_LOG_LVL_ERR, "vdpisr not support hw id %d\r\n", u4VdpIdx);
		return;
	}

#ifndef ATC_ARM2_LK_SUPPORT
	if (!fgLockInit) {
		spin_lock_init(&_vdp_lock);
		fgLockInit = TRUE;
		VDO_LOG(VDO_LOG_LVL_DBG, "vdpisr spin lock init %x\r\n", (unsigned int)&_vdp_lock);
	}
#endif
	SpinLock(u4VdpIdx);
	prParam = &rData[u4VdpIdx];


	if ((prParam->u4NeedClrBuff & VDP_CLR_ONCE_BUF) && (prParam->u4ClearIdx != 0xFF)) {
		if (prParam->u4ClearIdx >= prParam->u4CurrIdx) {
			fgClearFrmBuff(prParam, prParam->u4CurrIdx, prParam->u4ClearIdx);
		} else {
			fgClearFrmBuff(prParam, prParam->u4CurrIdx, MAX_BUFF_CNT - 1);
			fgClearFrmBuff(prParam, 0, prParam->u4ClearIdx);
		}

		prParam->u4ClearIdx = 0xFF;
		prParam->u4NeedClrBuff &= ~VDP_CLR_ONCE_BUF;
	}

	if ((prParam->u4NeedClrBuff & VDP_CLR_REGU_BUF) && (prParam->u4ClearIdx != 0xFF))
    {
		fgClearFrmBuff(prParam, prParam->u4ClearIdx, prParam->u4ClearIdx);
		prParam->u4ClearIdx = 0xFF;
	}

	if ((prParam->u4DeintMode < VDP_DI_MA4F_MODE) || (prParam->u4VdpStatus == VDP_STATUS_BLACK)
		|| (prParam->u4VdpStatus == VDP_STATUS_SEEK)) {
	}

	if (prParam->u4DeintMode == VDP_DI_MA4F_MODE) {
		switch (prParam->u4VdpStatus) {
		case VDP_STATUS_PREPARE:
		case VDP_STATUS_SEEK:
			if (VALID_BUFF_COUNT(prParam->u4CurrIdx, prParam->u4DispIdx, 1)) {
				VDO_LOG(VDO_LOG_LVL_INFO, "%s hw %d ready to show %d, %d\r\n", __func__, u4VdpIdx
					, (int)prParam->u4DispIdx, (int)prParam->u4CurrIdx);
				prParam->u4VdpStatus = VDP_STATUS_SHOW;
				_fgBypassFstVysnc[u4VdpIdx] = TRUE;
			} else {
				VDO_LOG(VDO_LOG_LVL_INFO, "%s hw %d VDP_STATUS_PREPARE %d, %d\r\n", __func__, u4VdpIdx
					, (int)prParam->u4DispIdx, (int)prParam->u4CurrIdx);

			}
			break;

		case VDP_STATUS_SHOW: {
			if (INVALID_BUFF_COUNT(prParam->u4CurrIdx, prParam->u4DispIdx)) {
				/* buffer not enough to show*/
				/*VDO_LOG(VDO_LOG_LVL_WARN, "VDP", "vVdpIsr no buffer %d, %d topfieldfirst %d\r\n"
				, (int)prParam->u4DispIdx, (int)prParam->u4CurrIdx
				, (int)prParam->fgTopFiledFirst);*/
			} else {
				if (_fgBypassFstVysnc[u4VdpIdx]) {
					/* First vsync not update address and motion and comb is invalid*/
					_fgBypassFstVysnc[u4VdpIdx] = FALSE;
				}

				u4DispIdx = prParam->u4DispIdx;

				if ((prParam->u4VsyncCnt[u4DispIdx] ==
					CALCULATE_VSYNC_CNT(prParam->u4Duration[u4DispIdx], VSYNC_PER_FIELD)) ||
					(prParam->u4VsyncCnt[u4DispIdx] ==
					CALCULATE_VSYNC_CNT(prParam->u4Duration[u4DispIdx], VSYNC_PER_FRAME))) {
						fgUpdateHwAddr(u4VdpIdx);
						vVdpHalSetFieldInfo(u4VdpIdx, prParam->fgTopFiledFirst, prParam->fgFirstField);
						prParam->fgFirstField = !prParam->fgFirstField;

						if (!vPmxHalGetFmtEn(u4VdpIdx)) {
							/* Enable FMT to show video*/
							vPmxHalEnableFmt(u4VdpIdx);
							VDO_LOG(VDO_LOG_LVL_INFO, "%s hw %d enable fmt\r\n", __func__, u4VdpIdx);
						}


					}


					if (prParam->u4VsyncCnt[u4DispIdx] != 0) {
						prParam->u4VsyncCnt[u4DispIdx]--;
					}
#ifndef ATC_ARM2_LK_SUPPORT
					//set_di_outbuf();
#endif
					if (prParam->u4VsyncCnt[u4DispIdx] == 0) {
						vUpdateDispIdx(u4VdpIdx);
						//update buffer index
					}
			}

			break;
		}

		case VDP_STATUS_FFRW: { /* fast forward or rew*/

			if (fgUpdateHwAddr(u4VdpIdx)) {
				vUpdateDispIdx(u4VdpIdx);
			}
			break;
		}

		default: {
			break;
		}
		}
	} /*else if (prParam->u4DeintMode == VDP_DI_HD_MODE) {
		ToDo: Swap top / bottom filed in isr
	}*/
		buffernum--;

	SpinUnlock(u4VdpIdx);
	fgSetBufState(prParam,startIdx,endIdx);
}

void vDumpVdpParam(int index)
{
	VDP_PARAM *prParam = &rData[index];

	VDO_LOG(VDO_LOG_LVL_INFO, "##############VDP Param Data Start Vdp %d##############\r\n", index);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4SrcWidth: %d\r\n", (int)prParam->u4SrcWidth);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4SrcHeight: %d\r\n", (int)prParam->u4SrcHeight);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->rSrcRect: %d %d %d %d\r\n", (int)prParam->rSrcRect.left,
		(int)prParam->rSrcRect.top, (int)prParam->rSrcRect.right, (int)prParam->rSrcRect.bottom);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->rDstRect: %d %d %d %d\r\n", (int)prParam->rDstRect.left,
		(int)prParam->rDstRect.top, (int)prParam->rDstRect.right, (int)prParam->rDstRect.bottom);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4SrcType: %d\r\n", (int)prParam->u4SrcType);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4Flags: 0x%x\r\n", (unsigned int)prParam->u4Flags);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4FrmBuffY: 0x%x\r\n"
		, (unsigned int)prParam->u4FrmBuffY[prParam->u4DispIdx]);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4FrmBuffC: 0x%x\r\n"
		, (unsigned int)prParam->u4FrmBuffC[prParam->u4DispIdx]);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4Duration: %d\r\n", (int)prParam->u4Duration[prParam->u4DispIdx]);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4VsyncCnt: %d\r\n", (int)prParam->u4VsyncCnt[prParam->u4DispIdx]);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4VdpStatus: %d\r\n", (int)prParam->u4VdpStatus);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4VdpMode: 0x%x\r\n", (unsigned int)prParam->u4VdpMode);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4DeintMode: %d\r\n", (int)prParam->u4DeintMode);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4PullDownMode: %d\r\n", (int)prParam->u4PullDownMode);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4CurrIdx: %d\r\n", (int)prParam->u4CurrIdx);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4DispIdx: %d\r\n", (int)prParam->u4DispIdx);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4PrevIdx: %d\r\n", (int)prParam->u4PrevIdx);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->u4ClearIdx: %d\r\n", (int)prParam->u4ClearIdx);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->fgNeedClrBuff: %d\r\n", (int)prParam->u4NeedClrBuff);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->fgFirstField: %d\r\n", (int)prParam->fgFirstField);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->fgNeedResizer: %d\r\n", (int)prParam->fgNeedResizer);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->fgProgSrc: %d\r\n", (int)prParam->fgProgSrc);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->fgTopFiledFirst: %d\r\n", (int)prParam->fgTopFiledFirst);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->fgRepeatFirstField: %d\r\n", (int)prParam->fgRepeatFirstField);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->fgProgSeq: %d\r\n", (int)prParam->fgProgSeq);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->fgPullDownFlagValid: %d\r\n", (int)prParam->fgPullDownFlagValid);
	VDO_LOG(VDO_LOG_LVL_INFO, "##############VDP Param Data End##############\r\n");
}

void vDumpParam(VDP_PARAM *prParam)
{
	VDO_LOG(VDO_LOG_LVL_DBG, "##############VDP Param Data Start Vdp 0x%x##############\r\n"
		, (unsigned int)prParam);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4SrcWidth: %d\r\n", (int)prParam->u4SrcWidth);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4SrcHeight: %d\r\n", (int)prParam->u4SrcHeight);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->rSrcRect: %d %d %d %d\r\n", (int)prParam->rSrcRect.left,
		(int)prParam->rSrcRect.top, (int)prParam->rSrcRect.right, (int)prParam->rSrcRect.bottom);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->rDstRect: %d %d %d %d\r\n", (int)prParam->rDstRect.left,
		(int)prParam->rDstRect.top, (int)prParam->rDstRect.right, (int)prParam->rDstRect.bottom);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4SrcType: %d\r\n", (int)prParam->u4SrcType);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4Flags: 0x%x\r\n", (unsigned int)prParam->u4Flags);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4FrmBuffY: 0x%x\r\n"
		, (unsigned int)prParam->u4FrmBuffY[prParam->u4DispIdx]);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4FrmBuffC: 0x%x\r\n"
		, (unsigned int)prParam->u4FrmBuffC[prParam->u4DispIdx]);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4Duration: %d\r\n", (int)prParam->u4Duration[prParam->u4DispIdx]);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4VsyncCnt: %d\r\n", (int)prParam->u4VsyncCnt[prParam->u4DispIdx]);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4VdpStatus: %d\r\n", (int)prParam->u4VdpStatus);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4VdpMode: 0x%x\r\n", (unsigned int)prParam->u4VdpMode);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4DeintMode: %d\r\n", (int)prParam->u4DeintMode);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4PullDownMode: %d\r\n", (int)prParam->u4PullDownMode);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4CurrIdx: %d\r\n", (int)prParam->u4CurrIdx);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4DispIdx: %d\r\n", (int)prParam->u4DispIdx);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4PrevIdx: %d\r\n", (int)prParam->u4PrevIdx);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->u4ClearIdx: %d\r\n", (int)prParam->u4ClearIdx);
	VDO_LOG(VDO_LOG_LVL_INFO, "prParam->fgNeedClrBuff: %d\r\n", (int)prParam->u4NeedClrBuff);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->fgFirstField: %d\r\n", (int)prParam->fgFirstField);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->fgNeedResizer: %d\r\n", (int)prParam->fgNeedResizer);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->fgProgSrc: %d\r\n", (int)prParam->fgProgSrc);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->fgTopFiledFirst: %d\r\n", (int)prParam->fgTopFiledFirst);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->fgRepeatFirstField: %d\r\n", (int)prParam->fgRepeatFirstField);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->fgProgSeq: %d\r\n", (int)prParam->fgProgSeq);
	VDO_LOG(VDO_LOG_LVL_DBG, "prParam->fgPullDownFlagValid: %d\r\n", (int)prParam->fgPullDownFlagValid);
	VDO_LOG(VDO_LOG_LVL_DBG, "##############VDP Param Data End##############\r\n");
}

bool fgAddFrmBuff(u32 u4VdpIdx, u32 u4YAddr, u32 u4CAddr, uintptr_t u4Bufferinfor)
{
	VDP_PARAM *prParam = &rData[u4VdpIdx];
	u32 u4Duration = 0;
	u32 u4Idx = prParam->u4CurrIdx;
#if MONITOR_QBUF_TIME
	struct timeval end_t = {0, 0};
	long usec = 0;
#endif

	if ((u4YAddr & 0x7F) || (u4CAddr & 0x7F)) {
		VDO_LOG(VDO_LOG_LVL_ERR, "[VDP]fgAddFrmBuff: error buffer is not 128 bytes alignment %x, %x, %d\r\n",
			(unsigned int)u4YAddr, (unsigned int)u4CAddr, (int)u4Duration);
		return FALSE;
	}

	if ((u4YAddr == prParam->u4FrmBuffY[(u4Idx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT]) ||
		(u4CAddr == prParam->u4FrmBuffY[(u4Idx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT])) {
		/*vdp_aee_print("fgAddFrmBuff: error buffer addr is the same as before ");*/
		VDO_LOG(VDO_LOG_LVL_ERR, "[VDP]fgAddFrmBuff: error buffer addr is the same as before %x, %x, %d\r\n",
			(unsigned int)u4YAddr, (unsigned int)u4CAddr, (int)u4Duration);
		return FALSE;
	}

	prParam->u4FrmBuffY[u4Idx] = u4YAddr;
	prParam->u4FrmBuffC[u4Idx] = u4CAddr;
	prParam->u4BufferInfor[u4Idx] = u4Bufferinfor;
	prParam->u4Duration[u4Idx] = VSYNC_PER_FRAME;
	prParam->u4VsyncCnt[u4Idx] = CALCULATE_VSYNC_CNT(prParam->u4Duration[u4Idx], VSYNC_PER_FIELD);

#if MONITOR_QBUF_TIME
#if 1

	if ((u4TmpCnt == 0) && (fgStartCnt == FALSE)) {
		do_gettimeofday(&start_t[u4VdpIdx]);
		fgStartCnt = TRUE;
	}

	if (u4TmpCnt == 30) {
		do_gettimeofday(&end_t);
		usec = (end_t.tv_sec - start_t[u4VdpIdx].tv_sec) * 1000000
			+ (end_t.tv_usec - start_t[u4VdpIdx].tv_usec);
		VDO_LOG(VDO_LOG_LVL_INFO, "[VDP]fgAddFrmBuff: 30 FPS id = %d usec = %d\r\n", (int)u4VdpIdx, (int)usec);
		do_gettimeofday(&start_t[u4VdpIdx]);
		u4TmpCnt = 0;
	} else {
		u4TmpCnt++;
	}

#else

	if (fgStartCnt == FALSE) {
		do_gettimeofday(&start_t[u4VdpIdx]);
		fgStartCnt = TRUE;
	}

	do_gettimeofday(&end_t);
	usec = (end_t.tv_sec - start_t[u4VdpIdx].tv_sec) * 1000000 + (end_t.tv_usec - start_t[u4VdpIdx].tv_usec);
	VDO_LOG(VDO_LOG_LVL_INFO, "[VDP]fgAddFrmBuff: id = %d usec = %d~~~~~~~~\r\n", (int)u4VdpIdx, (int)usec);
	do_gettimeofday(&start_t[u4VdpIdx]);
#endif
#endif

	if (prParam->u4DeintMode > VDP_DI_MA4F_MODE) {
		VDO_LOG(VDO_LOG_LVL_ERR, "[VDP]fgAddFrmBuff: Duration error idx %d, %x, %x, %d, %d, DI %d\r\n",
			(int)u4Idx, (unsigned int)u4YAddr, (unsigned int)u4CAddr, (int)u4Duration,
			(int)prParam->u4VsyncCnt[u4Idx], (int)prParam->u4DeintMode);
		return FALSE;
	}

	prParam->u4CurrIdx++;

	if (prParam->u4CurrIdx == MAX_BUFF_CNT) {
		prParam->u4CurrIdx = 0;
	}

	return TRUE;
}

u32 RemapVdpRect(RECT *prDst, RECT *prObj, u32 u4SrcWidth, u32 u4SrcHeight
	, u32 u4DstWidth, u32 u4DstHeight)
{
	prObj->left = prDst->left * u4SrcWidth / u4DstWidth;
	prObj->right = prDst->right * u4SrcWidth / u4DstWidth;
	prObj->top = prDst->top * u4SrcHeight / u4DstHeight;
	prObj->bottom = prDst->bottom * u4SrcHeight / u4DstHeight;

	return TRUE;
}

u32 remap_rear_rect(RECT *prBefore)
{
	return 0;
}




bool fgSetVideoInfo(u32 u4VdpIdx, struct OVERLAY_PARAM *pOverlay, bool fgFlag)
{
	VDP_PARAM *prParam = &rData[u4VdpIdx];

#ifdef CONFIG_ATC_OS_linux
	/*If set src crop, src crop < src width and height*/
	if (((pOverlay->rSrcRect.right - pOverlay->rSrcRect.left) > pOverlay->u4SrcWidth) ||
		((pOverlay->rSrcRect.bottom - pOverlay->rSrcRect.top) > pOverlay->u4SrcHeight))
#else
	if (((pOverlay->rSrcRect.right - pOverlay->rSrcRect.left) != pOverlay->u4SrcWidth) ||
		((pOverlay->rSrcRect.bottom - pOverlay->rSrcRect.top) != pOverlay->u4SrcHeight))
#endif
	{
		prParam->u4SrcWidth = SIZE_ALIGN(pOverlay->rSrcRect.right - pOverlay->rSrcRect.left, 16);
		prParam->u4SrcHeight = SIZE_ALIGN(pOverlay->rSrcRect.bottom - pOverlay->rSrcRect.top, 32);
#ifdef CONFIG_ATC_OS_linux
		VDO_LOG(VDO_LOG_LVL_WARN, "reconfig dev %d src width/height %d, %d\r\n", u4VdpIdx
			, prParam->u4SrcWidth, prParam->u4SrcHeight);
#endif
	} else {
		prParam->u4SrcWidth = pOverlay->u4SrcWidth;
		prParam->u4SrcHeight = pOverlay->u4SrcHeight;
	}

	prParam->rSrcRect.left = pOverlay->rSrcRect.left;
	prParam->rSrcRect.top = pOverlay->rSrcRect.top;
	prParam->rSrcRect.right = pOverlay->rSrcRect.right;
	prParam->rSrcRect.bottom = pOverlay->rSrcRect.bottom;

	if (u4VdpIdx == VDP_1) {
		prParam->rDstRect.left = pOverlay->rDstRect.left;
		prParam->rDstRect.top = pOverlay->rDstRect.top;
		prParam->rDstRect.right = pOverlay->rDstRect.right;
		prParam->rDstRect.bottom = pOverlay->rDstRect.bottom;
	} else if (u4VdpIdx == VDP_2) {
		if ((pOverlay->rDstRect.right > 720) || (pOverlay->rDstRect.bottom > 576) ||
		    (pOverlay->rDstRect.right == 0) || (pOverlay->rDstRect.bottom == 0)) {
			if (prParam->u4VdpMode == RES_480P) {
				prParam->rDstRect.bottom = 480;
			} else {
				prParam->rDstRect.bottom = 576;
			}

			prParam->rDstRect.left = 0;
			prParam->rDstRect.top = 0;
			prParam->rDstRect.right = 720;
			VDO_LOG(VDO_LOG_LVL_WARN, "Rear param error and reconfig to SD %d, %d, %d, %d vdp mode %d\r\n"
				, (int)pOverlay->rDstRect.top, (int)pOverlay->rDstRect.bottom
				, (int)pOverlay->rDstRect.left, (int)pOverlay->rDstRect.right
				, (int)prParam->u4VdpMode);
		} else {
			prParam->rDstRect.left = pOverlay->rDstRect.left;
			prParam->rDstRect.top = pOverlay->rDstRect.top;
			prParam->rDstRect.right = pOverlay->rDstRect.right;
			prParam->rDstRect.bottom = pOverlay->rDstRect.bottom;
		}
	}

	prParam->u4Flags = pOverlay->u4Flags;
	prParam->u4SrcType = pOverlay->device_name;
	prParam->fgProgSrc = pOverlay->fgProgSrc;

#if !LINUX_FUN
	if (prParam->u4SrcType == USB) {
#endif
		prParam->fgTopFiledFirst = pOverlay->fgTopFiledFirst;
#if !LINUX_FUN
	} else {
#if 1
		if(prParam->u4SrcType == 0x11)
			prParam->fgTopFiledFirst = TRUE;
		else
			prParam->fgTopFiledFirst = FALSE;
#else
		prParam->fgTopFiledFirst = TRUE;
		if(prParam->rSrcRect.top & 1) {
			prParam->fgFirstField = FALSE;
		}
		else {
			prParam->fgFirstField = TRUE;
		}
#endif
	}
#endif

	prParam->fgRepeatFirstField = pOverlay->fgRepeatFirstField;
	prParam->fgProgSeq = pOverlay->fgProgSeq;
	prParam->fgPullDownFlagValid = pOverlay->fgPullDownFlagValid;

	return TRUE;
}

bool vGetWidthHeight(u32 u4Mode, u32 *prWidth, u32 *prHeight)
{
	switch (u4Mode) {
	case RES_480P:
	case RES_480I:
		*prWidth = 720;
		*prHeight = 480;
		break;

	case RES_576P:
	case RES_576I:
		*prWidth = 720;
		*prHeight = 576;
		break;

	case RES_480P_800:
	case RES_480P_800_50HZ:
		*prWidth = 800;
		*prHeight = 480;
		break;

	case RES_600P_800:
	case RES_600P_800_50HZ:
		*prWidth = 800;
		*prHeight = 600;
		break;

	case RES_600P_1024:
	case RES_600P_1024_50HZ:
		*prWidth = 1024;
		*prHeight = 600;
		break;

	case RES_768P_1024:
		*prWidth = 1024;
		*prHeight = 768;
		break;

	case RES_800P_1280:
	case RES_800P_1280_50HZ:
		*prWidth = 1280;
		*prHeight = 800;
		break;

	default:
		*prWidth = 720;
		*prHeight = 480;
		return FALSE;
	}

	return TRUE;
}

void vSclMasterSwitchSrc(u32 u4VdpMode, u32 u4LcdWidth, u32 u4LcdHeight)
{
#if 0
	u32 u4SclInput = 0, u4SclOutput = 0;

	switch (u4VdpMode) {
	case RES_480P:
		u4SclInput = SCL_IN_480P;
		break;

	case RES_480P_800:
	case RES_480P_800_50HZ:
		u4SclInput = SCL_IN_480P_800;
		break;

	case RES_600P_800:
	case RES_600P_800_50HZ:
		u4SclInput = SCL_IN_600P_800;
		break;

	case RES_600P_1024:
	case RES_600P_1024_50HZ:
		u4SclInput = SCL_IN_600P_1024;
		break;

	case RES_720P_1280:
	case RES_720P_1280_50HZ:
		u4SclInput = SCL_IN_720P_1280;
		break;

	case RES_800P_1280:
	case RES_800P_1280_50HZ:
		u4SclInput = SCL_IN_800P_1280;
		break;

	case RES_768P_1024:
		u4SclInput = SCL_IN_768P_1024;
		break;
	}

	if ((u4LcdWidth == 800) && (u4LcdHeight == 480)) {
		u4SclOutput = SCL_OUT_800_480;
	} else if ((u4LcdWidth == 1024) && (u4LcdHeight == 600)) {
		u4SclOutput = SCL_OUT_1024_600;
	} else if ((u4LcdWidth == 1280) && (u4LcdHeight == 720)) {
		u4SclOutput = SCL_OUT_1280_720;
	} else if ((u4LcdWidth == 1280) && (u4LcdHeight == 800)) {
		u4SclOutput = SCL_OUT_1280_800;
	} else if ((u4LcdWidth == 1024) && (u4LcdHeight == 768)) {
		u4SclOutput = SCL_OUT_1024_768;
	}

	VDO_LOG(VDO_LOG_LVL_DBG, "vSclMasterSwitchSrc: u4SclInput %d, u4SclOutput %d\r\n"
		, (int)u4SclInput, (int)u4SclOutput);
	vSclHalSetMasterSrc(u4SclInput, u4SclOutput);
#endif
}

static bool fgConfigVdpHw(u32 u4VdpIdx)
{
	VDP_PARAM *prParam = &rData[u4VdpIdx];
	u32 u4CurIdx;
	PRECT pRect = &prParam->rDstRect;

	//VDO_LOG(VDO_LOG_LVL_INFO, "[xzr]%s\r\n", __func__);
	//vDumpVdpParam(u4VdpIdx);

	prParam->u4DispIdx = (prParam->u4CurrIdx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;

	vVdpHalSetOutRegion(u4VdpIdx, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top);

	vVdpHalSetSrcSize(u4VdpIdx, prParam->u4SrcWidth, prParam->u4SrcHeight);
	pRect = &prParam->rSrcRect;
	vVdpHalSetSrcRegion(u4VdpIdx, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top);
	vVdpHalSetYuv422(u4VdpIdx, FALSE); /* default is YUV420 mode*/
	vVdpHalSetScanLine(u4VdpIdx, prParam->u4Flags & VDP_SCANLINE_MODE);
	//vPmxHalDispFmtHFilter(u4VdpIdx, 3, 2);
	//vPmxHalSetMode(u4VdpIdx, RES_480P_800); //now ,fixed in panel size:800*480
	vVdpHalSetMode(u4VdpIdx, RES_480P_800);
	vPmxHalSetFullRange(u4VdpIdx, TRUE, TRUE); /* video normal range and convert to full range*/
	vVdpHalSetFifo(u4VdpIdx);


	if (prParam->u4VdpStatus == VDP_STATUS_SHOW) {
		u4CurIdx = prParam->u4CurrIdx;
		/* Updateoverlay more than once, and need clear unused buffer if sd source*/
		if (!prParam->fgNeedResizer) {
			prParam->u4NeedClrBuff |= VDP_CLR_REGU_BUF;
			/* cur buffer is ready to write, cur - 1 is ready to show and cur - 2 is unsused buffer*/
			prParam->u4ClearIdx = (u4CurIdx - 2 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
		}

		prParam->u4PrevIdx = (u4CurIdx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
		prParam->u4DispIdx = (u4CurIdx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
#if 0
		if(prParam->rSrcRect.top & 1) {
			prParam->fgFirstField = FALSE;
		}
		else {
			prParam->fgFirstField = TRUE;
		}
#else
		prParam->fgFirstField = TRUE;
#endif
		prParam->u4NeedShowBuff = 0;
		VDO_LOG(VDO_LOG_LVL_DBG,"fgConfigVdpHw: u4VdpIdx = %d, Curr = %d, Disp = %d\r\n"
		    , (int)u4VdpIdx, (int)u4CurIdx, (int)prParam->u4DispIdx);
	}

	prParam->u4VdpStatus = VDP_STATUS_PREPARE;

	if (prParam->u4DeintMode < VDP_DI_MA4F_MODE) {
		vVdpHalSetFieldInfo(u4VdpIdx, prParam->fgTopFiledFirst, prParam->fgFirstField);       //should config filedinfo?
		if (fgUpdateHwAddr(u4VdpIdx)) {
			vUpdateDispIdx(u4VdpIdx);
			if (!vPmxHalGetFmtEn(u4VdpIdx)) {
				/* Enable FMT to show video*/
				vPmxHalEnableFmt(u4VdpIdx);
				vVdpHalReset(u4VdpIdx);
				VDO_LOG(VDO_LOG_LVL_INFO, "%s hw %d enable fmt\r\n", __func__, u4VdpIdx);
			}
			if (u4VdpIdx == VDP_1){ //((u4VdpIdx == VDP_1) && !fgPmxHalMixPlane(PMX_1, PMX_HW_PLANE_1)) {
				/* Front need set mix layer and rear setting in tve module*/
				vPmxMixPlane(PMX_1, PMX_HW_PLANE_1);
				VDO_LOG(VDO_LOG_LVL_INFO, "enable video layer\r\n");
			}
			prParam->u4VdpStatus = VDP_STATUS_SHOW;
		}
	}
	return TRUE;
}

bool fgHDSource(VDP_PARAM *prParam)
{
	bool ret = ((prParam->u4SrcWidth > 720) || (prParam->u4SrcHeight > 576)) ? TRUE : FALSE;
	return ret;
}


bool fgNeedDeint(VDP_PARAM *prParam)
{
	bool fgResult = TRUE;
	if (prParam->fgProgSrc || !(prParam->u4Flags & VDP_ENABLE_DEINT) || fgHDSource(prParam)) {
			fgResult = FALSE;
	}

#if !ENABLE_MM_DEINTERLACE
	else if (prParam->u4SrcType == USB) {
		fgResult = FALSE;
	}
#endif

	return fgResult;
}

#define RECT_WIDTH(r)         ((r)->right - (r)->left)
#define RECT_HEIGHT(r)         ((r)->bottom - (r)->top)

void vSetVdpPath(u32 u4VdpIdx, VDP_PARAM *prParam)
{
	if (u4VdpIdx == VDP_1) {
		if (fgHDSource(prParam)) {
			//prParam->u4VdpMode = _u4DispMode;
		} else {
			prParam->u4VdpMode = RES_480P;
		}
	} else {
		if ((!prParam->fgNeedResizer) && (prParam->u4SrcHeight > 480)) { /* 720x480 < src < 720x576*/
			prParam->u4VdpMode = RES_576P;
		} else { /* (src < 720x480) or (src > 720x576 need to resizer to 720x480)*/
			prParam->u4VdpMode = RES_480P;
		}
	}
}

void vSetDeintMode(u32 u4VdpIdx, VDP_PARAM *prParam)
{
	u32 u4OldDeintMode = prParam->u4DeintMode;

	if (fgNeedDeint(prParam)) {
		prParam->u4DeintMode = VDP_DI_MA4F_MODE;
	}
	#if !LINUX_FUN
	else {
		if(!(prParam->u4Flags & VDP_TOP_LEVEL)) {
			if(prParam->u4SrcType == 0x11) {
				prParam->u4DeintMode = VDP_DI_FIELD_MODE;
			} else {
				prParam->u4DeintMode = VDP_DI_FRAME_MODE;
			}
		}
	}
	#endif

	if ((prParam->u4DeintMode != u4OldDeintMode) || (prParam->u4Flags & VDP_UPDATE_OVERLAY)) {
		VDO_LOG(VDO_LOG_LVL_INFO, "%s: idx %d, old %d, new %d, vdp mode %d\r\n", __func__, (int)u4VdpIdx,
			(int)u4OldDeintMode, (int)prParam->u4DeintMode, (int)prParam->u4VdpMode);

		if (prParam->u4DeintMode == VDP_DI_MA4F_MODE) {
			prParam->u4NeedClrBuff |= VDP_CLR_REGU_BUF;
			prParam->u4ClearIdx = (prParam->u4DispIdx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT; /* unused idx*/
			prParam->u4PrevIdx = prParam->u4DispIdx;
			VDO_LOG(VDO_LOG_LVL_INFO,"vSetDeintMode: idx %d, prev %d, disp %d, status %d\r\n"
				, (int)u4VdpIdx, (int)prParam->u4PrevIdx
				, (int)prParam->u4DispIdx, (int)prParam->u4VdpStatus);
		} else {

			prParam->u4NeedClrBuff |= VDP_CLR_ONCE_BUF;
			prParam->u4ClearIdx = prParam->u4PrevIdx; /* unused buffer idx*/
		}

		vVdpHalSetDeintMode(u4VdpIdx, prParam->u4DeintMode);
	}
}

bool fgCheckVdpFlags(u32 u4VdpIdx, VDP_PARAM *prParam)
{
	u32 u4Idx = prParam->u4CurrIdx;

	if ((prParam->u4Flags & VDP_PLAY_FF_RW) && (prParam->u4VdpStatus != VDP_STATUS_FFRW)) { /* enter fast forward*/
		if (prParam->u4DeintMode >= VDP_DI_MA4F_MODE) {
			prParam->u4DeintMode = VDP_DI_FIELD_MODE; /* fast forward only used field mode*/
			prParam->u4NeedClrBuff |= VDP_CLR_ONCE_BUF;
			/* cur buffer is ready to write, cur - 1 is ready to show and cur - 2 is unsused buffer*/
			prParam->u4ClearIdx = (u4Idx - 2 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
			vVdpHalSetDeintMode(u4VdpIdx, VDP_DI_FIELD_MODE);
		}

		prParam->u4VdpStatus = VDP_STATUS_FFRW;
		VDO_LOG(VDO_LOG_LVL_INFO, "fgCheckVdpFlags: enter fast forward flags %x, status %d, di %d\r\n",
			(unsigned int)prParam->u4Flags, (int)prParam->u4VdpStatus, (int)prParam->u4DeintMode);
	} else if (!(prParam->u4Flags & VDP_PLAY_FF_RW) && (prParam->u4VdpStatus == VDP_STATUS_FFRW)) { /*exit FF*/
		vSetVdpPath(u4VdpIdx, prParam);

		if (prParam->u4DeintMode >= VDP_DI_MA4F_MODE) {
			prParam->u4NeedClrBuff |= VDP_CLR_ONCE_BUF;
			/* cur buffer is ready to write, cur - 1 is ready to show and cur - 2 is unsused buffer*/
			prParam->u4ClearIdx = (u4Idx - 2 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
			prParam->u4PrevIdx = (u4Idx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
			prParam->u4DispIdx = (u4Idx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
			prParam->fgFirstField = TRUE;
			prParam->u4NeedShowBuff = 0;
			prParam->u4VdpStatus = VDP_STATUS_PREPARE;
		} else {
			prParam->u4VdpStatus = VDP_STATUS_SHOW;
		}

		VDO_LOG(VDO_LOG_LVL_INFO, "fgCheckVdpFlags: exit fast forward flags %x, status %d, di %d\r\n",
			(unsigned int)prParam->u4Flags, (int)prParam->u4VdpStatus, (int)prParam->u4DeintMode);
	} else if (prParam->u4Flags & VDP_SEEK_LOCATE) {
		if (prParam->u4DeintMode >= VDP_DI_MA4F_MODE) {
			prParam->u4NeedClrBuff |= VDP_CLR_ONCE_BUF;
			/* cur buffer is ready to write, cur - 1 is ready to show and cur - 2 is unsused buffer*/
			prParam->u4ClearIdx = (u4Idx - 2 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
			prParam->u4PrevIdx = (u4Idx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
			prParam->u4DispIdx = (u4Idx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
			prParam->fgFirstField = TRUE;
			prParam->u4NeedShowBuff = 0;
		}

		prParam->u4VdpStatus = VDP_STATUS_SEEK;
		VDO_LOG(VDO_LOG_LVL_INFO, "fgCheckVdpFlags: seek flags %x, status %d, di %d\r\n",
			(unsigned int)prParam->u4Flags, (int)prParam->u4VdpStatus, (int)prParam->u4DeintMode);
	}

	return TRUE;
}

bool fgClrSetHDFrmBuff(VDP_PARAM *prParam, u32 u4BufIdx, u32 u4YAddr, u32 u4CAddr)
{
	/* Release buffer and set new buffer address*/
	fgClearFrmBuff(prParam, u4BufIdx, u4BufIdx);

	if ((u4YAddr & 0x7F) || (u4CAddr & 0x7F)) {
		VDO_LOG(VDO_LOG_LVL_ERR, "fgClrSetHDFrmBuff: error buffer is not 128 bytes alignment %x, %x\r\n",
			(unsigned int)u4YAddr, (unsigned int)u4CAddr);
		return FALSE;
	}

	prParam->u4FrmBuffY[u4BufIdx] = u4YAddr;
	prParam->u4FrmBuffC[u4BufIdx] = u4CAddr;

	if (prParam->u4NeedClrBuff != VDP_CLR_NONE_BUF) {
		VDO_LOG(VDO_LOG_LVL_DBG, "fgClrSetHDFrmBuff: u4NeedClrBuff error id %d, flga %x\r\n",
			(int)u4BufIdx, (unsigned int)prParam->u4NeedClrBuff);
		prParam->u4NeedClrBuff = VDP_CLR_NONE_BUF;
	}

	return TRUE;
}

extern void vPmxNotMixPlane(unsigned char ucPmxId, unsigned int u4Plane);

bool fgHideVdp(u32 u4VdpIdx)
{
	VDP_PARAM *prParam = &rData[u4VdpIdx];

	if (u4VdpIdx == VDP_1) {
		vPmxNotMixPlane(PMX_1, PMX_HW_PLANE_1);
		VDO_LOG(VDO_LOG_LVL_INFO, "%s vPmxHalNotMixPlane video plane\r\n", __func__);
	}

	vPmxHalDisableFmt(u4VdpIdx);

	if (prParam->u4NeedClrBuff == VDP_CLR_NONE_BUF) { /* MA4F need 4 buffer to show 1 frame*/
		fgResetParam(prParam);
	} else {
		fgClearFrmBuff(prParam, 0, MAX_BUFF_CNT - 1);
		fgResetParam(prParam);
	}
	VDO_LOG(VDO_LOG_LVL_INFO, "%s dev %d status %d\r\n", __func__, u4VdpIdx, prParam->u4VdpStatus);

	return TRUE;
}

static bool Hide_Video(u32 u4VdpIdx)
{
	fgHideVdp(u4VdpIdx);
	return TRUE;
}
#if 0
static bool fgResizerBuffer(VDP_PARAM *prParam, PMTKSurface pDstSurf)
{

#ifndef ATC_ARM2_LK_SUPPORT
	IMGRESZ_HAL_IMG_INFO_T rSrcBufInfo;
	IMGRESZ_HAL_IMG_INFO_T rDstBufInfo;
	u32 u4BufIdx = (prParam->u4CurrIdx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
	__u64 u4YAddr = pDstSurf->PhysicalAddress(pDstSurf);
	__u64 u4CAddr = pDstSurf->PhysicalAddress(pDstSurf) +
			 ROUND_UP_COUNT(pDstSurf->Width(pDstSurf), 16) * ROUND_UP_COUNT(pDstSurf->Height(pDstSurf), 32);
	bool fg8Tap = FALSE;

	memset(&rSrcBufInfo, 0, sizeof(rSrcBufInfo));
	rSrcBufInfo.rBufferFormat.eBufferMainFormat = IMGRESZ_HAL_IMG_BUF_MAIN_FORMAT_Y_C_BUFFER;
	rSrcBufInfo.rBufferFormat.eYUVFormat = IMGRESZ_HAL_IMG_YUV_FORMAT_420;
	rSrcBufInfo.rBufferFormat.fgBlockMode = !(prParam->u4Flags & VDP_SCANLINE_MODE);
	rSrcBufInfo.rBufferFormat.fgAddrSwap = FALSE;//TRUE;
#ifdef CONFIG_ATC_OS_android
	if(!prParam->fgNeedRotate)
	{
		rSrcBufInfo.u4BufWidth = ROUND_UP_COUNT(prParam->u4SrcWidth, 16);
		rSrcBufInfo.u4BufHeight = ROUND_UP_COUNT(prParam->u4SrcHeight, 32);
		rSrcBufInfo.u4ImgWidth = prParam->rSrcRect.right;
		rSrcBufInfo.u4ImgHeight = prParam->rSrcRect.bottom;
		rSrcBufInfo.u4ImgXOff = 0;
		rSrcBufInfo.u4ImgYOff = 0;
	}
	else
	{
		rSrcBufInfo.u4BufWidth = prParam->u4SrcWidth;
		rSrcBufInfo.u4BufHeight = prParam->u4SrcHeight;
		rSrcBufInfo.u4ImgWidth = prParam->rSrcRect.right - prParam->rSrcRect.left;
		rSrcBufInfo.u4ImgHeight = prParam->rSrcRect.bottom - prParam->rSrcRect.top;
		rSrcBufInfo.u4ImgXOff = prParam->rSrcRect.left;
		rSrcBufInfo.u4ImgYOff = prParam->rSrcRect.top;
	}
#else
	rSrcBufInfo.u4BufWidth = ROUND_UP_COUNT(prParam->u4SrcWidth, 16);
	rSrcBufInfo.u4BufHeight = ROUND_UP_COUNT(prParam->u4SrcHeight, 32);
	rSrcBufInfo.u4ImgWidth = prParam->rSrcRect.right;

	rSrcBufInfo.u4ImgXOff = 0;
	rSrcBufInfo.u4ImgYOff = 0;
#endif
	rSrcBufInfo.u4BufSA1 = (unsigned long)prParam->u4FrmBuffY[u4BufIdx];
	rSrcBufInfo.u4BufSA2 = (unsigned long)prParam->u4FrmBuffC[u4BufIdx];

	/* Set target buffer info*/
	memset(&rDstBufInfo, 0, sizeof(rSrcBufInfo));
	rDstBufInfo.rBufferFormat.eBufferMainFormat = IMGRESZ_HAL_IMG_BUF_MAIN_FORMAT_Y_C_BUFFER;
	rDstBufInfo.rBufferFormat.eYUVFormat = IMGRESZ_HAL_IMG_YUV_FORMAT_420;
	rDstBufInfo.rBufferFormat.fgBlockMode = !(prParam->u4Flags & VDP_SCANLINE_MODE);
	rDstBufInfo.rBufferFormat.fgAddrSwap = FALSE;//TRUE;
#ifdef CONFIG_ATC_OS_android
	if(!prParam->fgNeedRotate)
	{
			rDstBufInfo.u4BufWidth =  ROUND_UP_COUNT(pDstSurf->Width(pDstSurf), 16);
			rDstBufInfo.u4BufHeight = ROUND_UP_COUNT(pDstSurf->Height(pDstSurf), 32);
	}
	else
	{
			rDstBufInfo.u4BufWidth =  ROUND_UP_COUNT(pDstSurf->Width(pDstSurf), 16);
			rDstBufInfo.u4BufHeight = pDstSurf->Height(pDstSurf);
	}
#else
	rDstBufInfo.u4BufWidth =  ROUND_UP_COUNT(pDstSurf->Width(pDstSurf), 16);
	rDstBufInfo.u4BufHeight = ROUND_UP_COUNT(pDstSurf->Height(pDstSurf), 32);
#endif

	rDstBufInfo.u4ImgWidth = pDstSurf->Width(pDstSurf);//prParam->rSrcRect.right;

	rDstBufInfo.u4ImgXOff = 0;
	rDstBufInfo.u4ImgYOff = 0;
	rDstBufInfo.u4BufSA1 = u4YAddr;
	rDstBufInfo.u4BufSA2 = u4CAddr;

#if ENABLE_HD_DEINTERLACE
	if (prParam->u4DeintMode == VDP_DI_HD_MODE) {
		rSrcBufInfo.rBufferFormat.fgProgressiveFrame = FALSE;
		rSrcBufInfo.rBufferFormat.fgTopField = TRUE;
		rSrcBufInfo.u4ImgHeight = prParam->u4SrcHeight >> 1;

		rDstBufInfo.rBufferFormat.fgProgressiveFrame = FALSE;
		rDstBufInfo.rBufferFormat.fgTopField = TRUE;
		rDstBufInfo.u4ImgHeight = pDstSurf->Height(pDstSurf) >> 1;
	} else
#endif
	{
		rSrcBufInfo.rBufferFormat.fgProgressiveFrame = TRUE;
	#ifdef CONFIG_ATC_OS_linux
		rSrcBufInfo.u4ImgHeight = prParam->rSrcRect.bottom;
	#endif

		rDstBufInfo.rBufferFormat.fgProgressiveFrame = TRUE;
		rDstBufInfo.u4ImgHeight = pDstSurf->Height(pDstSurf);//prParam->rSrcRect.bottom;
	}

	SpinUnlock(SPIN_LOCK_SRC_RESIZE); /* Can not sleep in spin lock case*/

        #if 0
	/*if wch srctype ,both front and rear now, imgresz use H-8Tap method when > 720p*/
	if ((prParam->u4SrcType != USB) && (rData[0].fgNeedResizer && rData[1].fgNeedResizer)) {
		if (rSrcBufInfo.u4ImgWidth > 1280 || rSrcBufInfo.u4ImgHeight > 720) {
			fg8Tap = TRUE;
		}
	}
        #endif

	TS_DirectScale(1, fg8Tap, &rSrcBufInfo, &rDstBufInfo);
	/*VDO_LOG(VDO_LOG_LVL_DBG, "fgResizerBuffer: 1 in src buf %d, %d, img %d, %d, dst buf %d, %d, img %d, %d\n"
		, (int)rSrcBufInfo.u4BufWidth, (int)rSrcBufInfo.u4BufHeight, (int)rSrcBufInfo.u4ImgWidth
		, (int)rSrcBufInfo.u4ImgHeight, (int)rDstBufInfo.u4BufWidth, (int)rDstBufInfo.u4BufHeight
		, (int)rDstBufInfo.u4ImgWidth, (int)rDstBufInfo.u4ImgHeight);*/

#if ENABLE_HD_DEINTERLACE
	if (prParam->u4DeintMode == VDP_DI_HD_MODE) {
		rSrcBufInfo.rBufferFormat.fgTopField = FALSE;
		rDstBufInfo.rBufferFormat.fgTopField = FALSE;
		TS_DirectScale(1, fg8Tap, &rSrcBufInfo, &rDstBufInfo);
		/*VDO_LOG(VDO_LOG_LVL_DBG, "fgResizerBuffer: 2 src %d, %d, img %d, %d, dst %d, %d, img %d, %d\n"
			, (int)rSrcBufInfo.u4BufWidth, (int)rSrcBufInfo.u4BufHeight
			, (int)rSrcBufInfo.u4ImgWidth, (int)rSrcBufInfo.u4ImgHeight
			, (int)rDstBufInfo.u4BufWidth, (int)rDstBufInfo.u4BufHeight
			, (int)rDstBufInfo.u4ImgWidth, (int)rDstBufInfo.u4ImgHeight);*/
	}
#endif
	SpinLock(SPIN_LOCK_SRC_RESIZE);

	if (!fgClrSetHDFrmBuff(prParam, u4BufIdx, (unsigned int)u4YAddr, (unsigned int)u4CAddr)) {
		return FALSE;
	}
#endif
	return TRUE;

}
#endif

static bool Show_Video(u32 u4VdpIdx)
{
	VDP_PARAM *prParam = &rData[u4VdpIdx];

	/*if ((fgNeedResizer != prParam->fgNeedResizer) && !(prParam->u4Flags & VDP_UPDATE_OVERLAY)) {

	}*/

	vSetDeintMode(u4VdpIdx, prParam);
	fgCheckVdpFlags(u4VdpIdx, prParam);

	//Remove Image Resize

	/*VDO_LOG(VDO_LOG_LVL_DBG, "[VDP]Show_Video prParam->u4Flags 0x%x\n", (unsigned int)prParam->u4Flags);*/
	if (prParam->u4Flags & VDP_UPDATE_OVERLAY) {
		fgConfigVdpHw(u4VdpIdx);
	} else if (prParam->u4DeintMode < VDP_DI_MA4F_MODE) {
		prParam->u4NeedShowBuff++;
		if (fgUpdateHwAddr(u4VdpIdx)) {
			vUpdateDispIdx(u4VdpIdx);
		}
	}

	return TRUE;
}

int VDP_IOControl(u32 dwCode, void *pBufIn,  void *pBufOut)
{
	u32 u4Ret = 0, u4VdpIdx = 0, i, loop_cnt;
	bool ret = FALSE;
	struct OVERLAY_PARAM rear_follow_with_front_vdp_param;
	struct OVERLAY_PARAM *pOverlay = NULL;
	VDP_PARAM *prParam = NULL;
#ifndef ATC_ARM2_LK_SUPPORT
	if (!fgLockInit) {
		spin_lock_init(&_vdp_lock);
		fgLockInit = TRUE;
		VDO_LOG(VDO_LOG_LVL_DBG, "vdpisr spin lock init %x\r\n", (unsigned int)&_vdp_lock);
	}
#endif
	if (!pBufIn) {
		VDO_LOG(VDO_LOG_LVL_ERR, "VDP_IOControl: input buffer is null code = 0x%x\n", dwCode);
		return -EINVAL;
	}

	loop_cnt = (fr_follow & FR_FOLLOW_VIDEO) ? 2 : 1;

	switch (dwCode) {
	case VIDIOC_STREAMOFF: {
		/*if (!access_ok(VERIFY_READ, pBufIn, sizeof(struct OVERLAY_PARAM))) {
			u4Ret  = -EINVAL;
			VDO_LOG(VDO_LOG_LVL_ERR, "VIDIOC_STREAMOFF return %d\r\n", u4Ret);
			break;
		}*/
		pOverlay = (struct OVERLAY_PARAM *)pBufIn;
		u4VdpIdx = pOverlay->u4Idx;
		prParam = &rData[u4VdpIdx];
		VDO_LOG(VDO_LOG_LVL_INFO, "VDP_IOControl: VIDIOC_STREAMOFF cmd 0x%x id %d\n", dwCode, pOverlay->u4Idx);

		fgSetBufState(prParam,0,MAX_BUFF_CNT-1);

		for (i = 0; i < loop_cnt; i++) {
			SpinLock(SPIN_LOCK_SRC_IOCTL);
			if ((i == VDP_2) && (fr_follow & FR_FOLLOW_VIDEO)) {
				if (vdo_fr_on) {
					VDO_LOG(VDO_LOG_LVL_INFO, "%s id from %d to %d\n", __func__, pOverlay->u4Idx, i);
					pOverlay->u4Idx = i;
					vdo_fr_on = FALSE;
				} else {
					VDO_LOG(VDO_LOG_LVL_INFO, "%s vdo fr off and do nothing\n", __func__);
				}
			}

			u4VdpIdx = pOverlay->u4Idx;
			Hide_Video(u4VdpIdx);
			SpinUnlock(SPIN_LOCK_SRC_IOCTL);
		}
		break;
	}

	case VIDIOC_QBUF: {
		/*if (!access_ok(VERIFY_READ, pBufIn, sizeof(struct OVERLAY_PARAM))) {
			u4Ret  = -EINVAL;
			VDO_LOG(VDO_LOG_LVL_ERR, "VIDIOC_QBUF return %d\r\n", u4Ret);
			break;
		}*/

		pOverlay = (struct OVERLAY_PARAM *)pBufIn;
		//VDO_LOG(VDO_LOG_LVL_INFO, "[xzr]VIDIOC_QBUF Addr=%x,%x\r\n", pOverlay->u4PhysicalAddressY, pOverlay->u4PhysicalAddressC);

		if ((loop_cnt == 1) && vdo_fr_on) {
			VDO_LOG(VDO_LOG_LVL_INFO, "%s fr_follow %d vdo fr %d and hide rear\n", __func__
				, fr_follow, (u32)vdo_fr_on);
			Hide_Video(VDP_2);
			vdo_fr_on = FALSE;
		}
		for (i = 0; i < loop_cnt ; i++) {
			if ((i == VDP_2) && (fr_follow & FR_FOLLOW_VIDEO)) {
				pOverlay  = &rear_follow_with_front_vdp_param;
				memcpy(pOverlay, pBufIn, sizeof(struct OVERLAY_PARAM));
				remap_rear_rect(&pOverlay->rDstRect);
				if (!vdo_fr_on) {
					VDO_LOG(VDO_LOG_LVL_INFO, "%s id from %d to %d vdo fr %d\n", __func__
						, pOverlay->u4Idx, i, (u32)vdo_fr_on);
					pOverlay->u4Flags |= VDP_UPDATE_OVERLAY;
					vdo_fr_on = TRUE;
				}
				pOverlay->u4Idx = i;
			}
			u4VdpIdx = pOverlay->u4Idx;
			prParam = &rData[u4VdpIdx];
			VDO_LOG(VDO_LOG_LVL_DBG, "VIDIOC_QBUF  Addr=%x,%x\r\n"
								, (unsigned int)pOverlay->u4PhysicalAddressY
								, (unsigned int)pOverlay->u4PhysicalAddressC);

			VDO_LOG(VDO_LOG_LVL_DBG, "VDP_IOControl: VIDIOC_QBUF cmd 0x%x\n", dwCode);
			if (pOverlay->u4Flags & VDP_UPDATE_OVERLAY) {
				VDO_LOG(VDO_LOG_LVL_INFO, "VIDIOC_QBUF param 0x%x id=%d, src %d,%d, {%d,%d,%d,%d}\r\n"
					, (unsigned int)pBufIn, (int)pOverlay->u4Idx
					, (int)pOverlay->u4SrcWidth, (int)pOverlay->u4SrcHeight
					, (int)pOverlay->rSrcRect.left, (int)pOverlay->rSrcRect.top
					, (int)pOverlay->rSrcRect.right, (int)pOverlay->rSrcRect.bottom);
				VDO_LOG(VDO_LOG_LVL_INFO, "VIDIOC_QBUF dst {%d,%d,%d,%d} Addr=%x,%x, stutas=%d, flags=%x\r\n"
					, (int)pOverlay->rDstRect.left, (int)pOverlay->rDstRect.top
					, (int)pOverlay->rDstRect.right, (int)pOverlay->rDstRect.bottom
					, (unsigned int)pOverlay->u4PhysicalAddressY
					, (unsigned int)pOverlay->u4PhysicalAddressC
					, (int)pOverlay->u4Status, (unsigned int)pOverlay->u4Flags);
			}

			if (!_fgInitVdpParam[u4VdpIdx]) {
				fgResetParam(prParam);
				_fgInitVdpParam[u4VdpIdx] = TRUE;
			}

			SpinLock(SPIN_LOCK_SRC_IOCTL);

			if (pOverlay->u4Flags & VDP_TOP_LEVEL) {
				if (fgPmxHalMixPlane(PMX_1, PRIMARY_SURF_PLANE)) {
					vPmxNotMixPlane(PMX_1, PRIMARY_SURF_PLANE);
					VDO_LOG(VDO_LOG_LVL_INFO, "vdp not mix primary surface plane %d\r\n", PRIMARY_SURF_PLANE);
				}
			}

			ret = fgAddFrmBuff(u4VdpIdx, pOverlay->u4PhysicalAddressY, pOverlay->u4PhysicalAddressC,pOverlay->u4BufferInfor);
			if (ret) {
				fgSetVideoInfo(u4VdpIdx, pOverlay, TRUE);
				Show_Video(u4VdpIdx);
			} else {
				u4Ret = -EIO;
			}
			if(buffernum < 1) {
				buffernum = 2;
			} else {
				buffernum = buffernum+2;
			}
			SpinUnlock(SPIN_LOCK_SRC_IOCTL);
		}
		break;
	}
#ifndef ATC_ARM2_LK_SUPPORT
	case STIOC_SET_VDP_PARAM: {
		/*if (!access_ok(VERIFY_READ, pBufIn, sizeof(struct OVERLAY_PARAM))) {
			u4Ret  = -EINVAL;
			VDO_LOG(VDO_LOG_LVL_ERR, "STIOC_SET_VDP_PARAM return %d\r\n", u4Ret);
			break;
		}*/
		pOverlay = (struct OVERLAY_PARAM *)pBufIn;
		for (i = 0; i < loop_cnt ; i++) {
			if (i == VDP_2) {
				pOverlay  = &rear_follow_with_front_vdp_param;
				memcpy(pOverlay, pBufIn, sizeof(struct OVERLAY_PARAM));
				remap_rear_rect(&pOverlay->rDstRect);
			}
			SpinLock(SPIN_LOCK_SRC_IOCTL);
			if (fr_follow & FR_FOLLOW_VIDEO) {
				pOverlay->u4Idx = i;
			}
			u4VdpIdx = pOverlay->u4Idx;
			VDO_LOG(VDO_LOG_LVL_DBG, "VDP_IOControl: STIOC_SET_VDP_PARAM cmd 0x%x\n", dwCode);
			VDO_LOG(VDO_LOG_LVL_INFO, "STIOC_SET_VDP_PARAM param 0x%x id=%d, src %d,%d, {%d,%d,%d,%d}\r\n"
				, (unsigned int)pBufIn, (int)pOverlay->u4Idx
				, (int)pOverlay->u4SrcWidth, (int)pOverlay->u4SrcHeight
				, (int)pOverlay->rSrcRect.left, (int)pOverlay->rSrcRect.top
				, (int)pOverlay->rSrcRect.right, (int)pOverlay->rSrcRect.bottom);
			VDO_LOG(VDO_LOG_LVL_INFO, "STIOC_SET_VDP_PARAM dst {%d,%d,%d,%d} Addr=%x,%x, stutas=%d, flags=%x\r\n"
				, (int)pOverlay->rDstRect.left, (int)pOverlay->rDstRect.top
				, (int)pOverlay->rDstRect.right, (int)pOverlay->rDstRect.bottom
				, (unsigned int)pOverlay->u4PhysicalAddressY
				, (unsigned int)pOverlay->u4PhysicalAddressC
				, (int)pOverlay->u4Status, (unsigned int)pOverlay->u4Flags);
			/* Other video is show which need back up param and release vdp hw for backcar*/

			prParam = &rData[u4VdpIdx];

			if ((pOverlay->device_name == BACKCAR) && (pOverlay->device_name != prParam->u4SrcType)) {
				Hide_Video(u4VdpIdx);
			}
			fgSetVideoInfo(u4VdpIdx, pOverlay, TRUE);
			SpinUnlock(SPIN_LOCK_SRC_IOCTL);
		}
		break;
	}

	case STIOC_SET_FMT_BLACK: {
		struct FMT_BG_PARAM *pFmtBG = NULL;

		/*if (!access_ok(VERIFY_READ, pBufIn, sizeof(struct FMT_BG_PARAM))) {
			u4Ret  = -EINVAL;
			VDO_LOG(VDO_LOG_LVL_ERR, "STIOC_SET_FMT_BLACK return %d\r\n", u4Ret);
			break;
		}*/
		pFmtBG = (struct FMT_BG_PARAM *)pBufIn;
		prParam = &rData[pFmtBG->u4Idx];
		SpinLock(SPIN_LOCK_SRC_IOCTL);
		if (pFmtBG->fgEnable) {
			//vPmxHalSetBg(pFmtBG->u4Idx, pFmtBG->u4Color);
			vPmxHalDisableFmt(pFmtBG->u4Idx);
			prParam->u4VdpStatus = VDP_STATUS_BLACK;
		} else {
			u32 u4Idx = prParam->u4CurrIdx;

			vPmxHalEnableFmt(pFmtBG->u4Idx);
			if (prParam->u4DeintMode >= VDP_DI_MA4F_MODE) {
				prParam->u4NeedClrBuff |= VDP_CLR_ONCE_BUF;
				/* cur buffer is ready to write, cur - 1 is ready to show and cur - 2 is unsused buffer*/
				prParam->u4ClearIdx = (u4Idx - 2 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
				prParam->u4PrevIdx = (u4Idx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
				prParam->u4DispIdx = (u4Idx - 1 + MAX_BUFF_CNT) % MAX_BUFF_CNT;
				prParam->fgFirstField = TRUE;
				prParam->u4NeedShowBuff = 0;
				prParam->u4VdpStatus = VDP_STATUS_PREPARE;
			} else {
				prParam->u4VdpStatus = VDP_STATUS_SHOW;
			}
		}
		VDO_LOG(VDO_LOG_LVL_INFO, "VDP_IOControl: set black %d status %d and video layer %d\n", pFmtBG->fgEnable
			, prParam->u4VdpStatus, fgPmxHalMixPlane(0, PMX_HW_PLANE_1));
		SpinUnlock(SPIN_LOCK_SRC_IOCTL);
		break;
	}
#endif
	default:
		VDO_LOG(VDO_LOG_LVL_DBG, "VDP_IOControl: Unsupported cmd 0x%x\n", dwCode);
		break;
	}

	return u4Ret;
}
#ifndef ATC_ARM2_LK_SUPPORT
EXPORT_SYMBOL(VDP_IOControl);
#endif
