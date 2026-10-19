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
#include <ion.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include "mtk_sync.h"

#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/memremap.h>
#include <linux/io.h>
#include "mtk_sync.h"

#else
#include <kernel/timer.h>
#include <platform/timer.h>
#include <platform/mt_reg.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_irq.h>
#include "kernel/event.h"
#include "errno-base.h"
#include "di_drv.h"
#include "platform/mt_typedefs.h"
#include "reserved_memory.h"
#define PAL_TICKS (TIMER_TICK_RATE/50) // 20ms
#define NTSC_TICKS (TIMER_TICK_RATE/60) // 16.666ms


extern void mt_irq_unmask(unsigned int irq);
extern void mt_irq_mask(unsigned int irq);
#endif
#include "display_inc.h"
#include "display.h"
#include "pmx_hal.h"

#include "drv_pmx.h"
#include "vdp_hal.h"
#include "vdp.h"
#include "di_if.h"
#include "di_hal.h"
#include "wch_if.h"
#include "log.h"
#include <ac8257_addr_utils.h>


#define NTSC_CYCLE (16666666) /*16.666666ms*/
#define PAL_CYCLE  (20000000) /*20ms*/
#define AVM_CYCLE  (4000000) /* 4ms*/

#define DI_BUFFER_SKIP_CNT  (2)
unsigned int skip_cnt;

#ifdef	DI_TZG_OUTPUT_EN
unsigned int addr_offset[VIDEO_CHANNEL_NUM];
#endif
atc_di_device DI_INTER_PARAM;

di_internal_params di_param[DEV_MAX];
int di_buf_num = DI_BUF_MAX;

di_buf_t di_outbuf_list[DI_BUF_MAX];
unsigned int outbuf_idx;
di_buf_t *di_inbuf_list[DI_BUF_MAX] = {NULL, NULL, NULL, NULL, NULL};
static unsigned int inbuf_cnt;
unsigned int inbuf_idx;
extern unsigned int singleChannelFlag;



#ifndef ATC_ARM2_LK_SUPPORT
/* Jira:AC8257P-13459 */
//static DEFINE_SPINLOCK(di_fd_lock);
//static DEFINE_MUTEX(di_buffer_mutex);

wait_queue_head_t di_trigger_wq;
atomic_t di_trigger_event = ATOMIC_INIT(0);

struct workqueue_struct *cvbs_wq;

atomic_t getBufferDone = ATOMIC_INIT(0);
atomic_t dev_use_di = ATOMIC_INIT(DEV_MAX);



#if DI_DUMP_BUFFER_ATTR
static DI_BUF_ADDR g_DI_BUF_ADDR = {
	.di_in_buf_cnt = 0,
	.di_out_buf_cnt = 0
};
#endif


#else
atomic_t getBufferDone = 1;
atomic_t dev_use_di = DEV_MAX;

event_t di_trigger_event;

#endif

extern void SpinLock(u32 u4LockSrc);
extern void SpinUnlock(u32 u4LockSrc);

//VDPSYS Configure
#ifndef ATC_ARM2_LK_SUPPORT
extern void __iomem* IO_BASE_BRINGUP;
extern void *memremap(resource_size_t offset, size_t size, unsigned long flags);
extern void memunmap(void *addr);

#define VDPSYS_BASE          		(IO_BASE_BRINGUP)
#define VDPSYS_READREG(dAddr)    readl(VDPSYS_BASE + dAddr)
#define VDPSYS_WRITEREG(addr, val)    writel(val, VDPSYS_BASE+addr)

#else
//extern unsigned long IO_BASE_BRINGUP;
#define VDPSYS_BASE          		(ARM9_IO_BASE + 0x02000000)
#define VDPSYS_READREG(dAddr)		 (*(volatile unsigned int*)(VDPSYS_BASE + (dAddr)))
#define VDPSYS_WRITEREG(addr, val)     (*(volatile unsigned int*)(VDPSYS_BASE + (addr)) = val)
#endif

//#ifndef ATC_ARM2_LK_SUPPORT

void dispfmt_irq_handler(void)
{
	unsigned int val = 0;
	val = VDPSYS_READREG(VDPSYS_INT_STATUS);

	if (~val & 0x1) {
		/* can trigger di now*/
		atomic_set(&getBufferDone, 1);
		VDO_LOG(VDO_LOG_LVL_DBG, "%s %x\n", __func__, val);

		VDPSYS_WRITEREG(VDPSYS_DISPFMT_INT_CLR, 1);
		VDPSYS_WRITEREG(VDPSYS_DISPFMT_INT_CLR, 0);

		VDPSYS_WRITEREG(VDPSYS_VDO_INT_CLR, 1);
		VDPSYS_WRITEREG(VDPSYS_VDO_INT_CLR, 0);
	}
}
//#endif

#ifdef ATC_ARM2_LK_SUPPORT
void set_di_outbuf(void)
{
	int i = 0;
	u32 out_base = get_reserved_memory_node_addr("wch-di-reserved-memory");

	SpinLock(0);
	memset(di_outbuf_list, 0, sizeof(di_buf_t) * DI_BUF_MAX);
	for (i = 0; i < DI_BUF_MAX; i++) {
		di_outbuf_list[i].y_addr = out_base + i * DI_OUT_BUF_SIZE;
		di_outbuf_list[i].idex = i;
		/*c_addr means size for out whcih fmt is YUV422*/
		di_outbuf_list[i].c_addr = DI_OUT_BUF_SIZE;
		atomic_set(&di_outbuf_list[i].status, AVIN_BUF_FREE);
		VDO_LOG(VDO_LOG_LVL_INFO, "idx:%d status %d addr:%p\n", i,
			atomic_read(&di_outbuf_list[i].status), (void *)di_outbuf_list[i].y_addr);
	}
	SpinUnlock(0);
}
#endif

#ifdef	DI_TZG_OUTPUT_EN
/********************/
/*     -----stride----
 *     ---w-------w---
 *     |      |      |
 *  2*h|------|------|
 *     |      |      |
 *     ---------------
 */
static void di_set_tzg_offset(int dev, int w, int h, int stride, int fmt)
{
	if (dev == AVM && fmt == DI_FMT_YUV420) {
		addr_offset[0] = 0;
		addr_offset[1] = w * 2;
		addr_offset[2] = (stride * 2) * h;
		addr_offset[3] = (stride * 2) * h + w * 2;
	}
 }
static void di_clr_tzg_offset(int dev)
{
	if (dev == AVM) {
		memset(addr_offset, 0, sizeof(addr_offset));
	}
}
#endif

int di_is_cur_dev(int dev)
{
	return (atomic_read(&dev_use_di) == dev);
}

static int update_hw_addr(u32 *yin_addr, u32 *cin_addr, di_internal_params *param)
{
	u32 phy_addr_y[4] = {0};
	u32 phy_addr_c[4] = {0};

#ifndef ATC_ARM2_LK_SUPPORT
	bool interlace_source_en = param->flags & DI_INTERLACE_SOURCE_EN;
	u32 y_offset = param->src_w * param->src_h / 2;
	u32 c_offset = param->src_w * param->src_h / 2;

	if (param->first_field) {
		phy_addr_y[0] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[0]);
		phy_addr_y[2] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1]);
		phy_addr_c[0] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[0]);
		phy_addr_c[2] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1]);

		if (interlace_source_en) {
			phy_addr_y[1] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[0] + y_offset);
			phy_addr_y[3] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1] + y_offset);
			phy_addr_c[1] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[0] + c_offset);
			phy_addr_c[3] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1] + c_offset);
		} else {
			phy_addr_y[1] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[0]);
			phy_addr_y[3] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1]);
			phy_addr_c[1] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[0]);
			phy_addr_c[3] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1]);
		}
	} else {
		phy_addr_y[0] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[0]);
		phy_addr_y[2] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1]);
		phy_addr_c[0] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[0]);
		phy_addr_c[2] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1]);

		if (interlace_source_en) {
			phy_addr_y[1] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1] + y_offset);
			phy_addr_y[3] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[2] + y_offset);
			phy_addr_c[1] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1] + c_offset);
			phy_addr_c[3] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[2] + c_offset);
		} else {
			phy_addr_y[1] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1]);
			phy_addr_y[3] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[2]);
			phy_addr_c[1] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1]);
			phy_addr_c[3] = ARM1_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[2]);
		}
	}
#else
	if (param->first_field) {
		phy_addr_y[0] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[0]);
		phy_addr_y[1] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[0]);
		phy_addr_y[2] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1]);
		phy_addr_y[3] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1]);

		phy_addr_c[0] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[0]);
		phy_addr_c[1] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[0]);
		phy_addr_c[2] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1]);
		phy_addr_c[3] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1]);
	} else {
		phy_addr_y[0] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[0]);
		phy_addr_y[1] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1]);
		phy_addr_y[2] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[1]);
		phy_addr_y[3] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(yin_addr[2]);

		phy_addr_c[0] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[0]);
		phy_addr_c[1] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1]);
		phy_addr_c[2] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[1]);
		phy_addr_c[3] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(cin_addr[2]);
	}
#endif
	if (phy_addr_y[0] && phy_addr_y[1] && phy_addr_y[2] && phy_addr_y[3] &&
	    phy_addr_c[0] && phy_addr_c[1] && phy_addr_c[2] && phy_addr_c[3]) {
		vVdpHalSetDeintWXYZ(VDP_1, phy_addr_y, phy_addr_c);
		return 0;
	}

	VDO_LOG(VDO_LOG_LVL_WARN, "phy0[%x] phy1[%x] phy2[%x] phy3[%x] phc0[%x] phc1[%x] phc2[%x] phc3[%x] is null \n",
			phy_addr_y[0], phy_addr_y[1], phy_addr_y[2], phy_addr_y[3],
			phy_addr_c[0], phy_addr_c[1], phy_addr_c[2], phy_addr_c[3]);
	return -EINVAL;

}

static bool set_vdp_hal_info(di_internal_params *params, int idx)
{
	PRECT rect = &params->dst_rect;

	vPmxHalInit(params->dst_w, params->dst_h, params->src_w, params->src_h);
	vVdpHalSetOutRegion(idx, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
	vVdpHalSetSrcSize(idx, params->src_w, params->src_h);
	vVdpHalSetSrcRegion(idx, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
	vVdpHalSetYuv422(idx, FALSE); /* default is YUV420 mode*/
	vVdpHalSetScanLine(idx, params->scan_line_mode);
	vVdpHalSetMode(idx, RES_480P_800);
	vVdpHalSetInterlaceSourceEn(params->flags & DI_INTERLACE_SOURCE_EN);
	vPmxHalSetFullRange(idx, TRUE, TRUE); /* video normal range and convert to full range*/
	vVdpHalSetFifo(idx);
	vVdpHalSetDeintMode(idx, params->deinterlace_mode);
	vVdpHalSetFieldInfo(idx, params->top_field_first, params->first_field);
	vVdpHalSetHDMode(idx, params->deinterlace_mode, params->src_w);

	return TRUE;
}

void di_callback(di_buf_t *buf_info)
{
#ifdef ATC_ARM2_LK_SUPPORT
	int j = 0;

	SpinLock(0);
	for (j = 0; j < DI_BUF_MAX; j++) {
		if (di_outbuf_list[j].y_addr == buf_info->y_addr) {
			di_param[CVBS].getdibuffer(&di_outbuf_list[j]);
			VDO_LOG(VDO_LOG_LVL_DBG, "%s callback backcar  idx %d , y_addr:%p %p\n", __func__, j,
				(void *)di_outbuf_list[j].y_addr, (void *)&di_outbuf_list[j]);
			break;
		}
	}
	SpinUnlock(0);
#endif
}

void di_init_params(void)
{
	static bool inited;
#ifndef ATC_ARM2_LK_SUPPORT
	int i = 0;
#endif
	if (inited) {
		return;
	}
	memset(&di_param, 0, sizeof(di_internal_params) * DEV_MAX);
#ifndef ATC_ARM2_LK_SUPPORT
	for (i = 0; i < DEV_MAX; i++) {
		mutex_init(&di_param[i].lock);
	}
	init_waitqueue_head(&di_trigger_wq);
#endif
	inited = true;
}


#ifndef ATC_ARM2_LK_SUPPORT
irqreturn_t dispfmt_vsync_isr(int vector, void *dev_id)
{
	VDO_LOG(VDO_LOG_LVL_DBG, " irq id %d \n", vector);
	dispfmt_irq_handler();
	return IRQ_HANDLED;
}

void di_signal_trigger_event(void)
{
	atomic_set(&di_trigger_event, 1);
	wake_up_interruptible(&di_trigger_wq);
	VDO_LOG(VDO_LOG_LVL_DBG, "signal di trigger event \n");
}

int di_wait_trigger_event(void)
{
	int ret = 0;

	//wait_event_interruptible(di_trigger_wq, atomic_read(&di_trigger_event));
	ret = wait_event_interruptible_timeout(di_trigger_wq, atomic_read(&di_trigger_event), msecs_to_jiffies(40));
	if (ret < 0) {
		VDO_LOG(VDO_LOG_LVL_ERR, "wait event error %d ms \n", ret);
	} else if (ret > 0) {
		VDO_LOG(VDO_LOG_LVL_DBG, "time left %d jiffies wait %lu\n", ret, msecs_to_jiffies(40));
		atomic_set(&di_trigger_event, 0);
	} else if (ret == 0) {
		VDO_LOG(VDO_LOG_LVL_ERR, "time out\n");
	}

	return ret;
}

#endif
int di_hal_set_params(int dev, di_params *param)
{
	int ret = 0;
	ENTER();

	if (param == NULL || (dev >= DEV_MAX || dev < 0)) {
		VDO_LOG(VDO_LOG_LVL_ERR, "param is NULL \n");
		return -EINVAL;
	}
#ifndef ATC_ARM2_LK_SUPPORT
	mutex_lock(&di_param[dev].lock);
#endif

	if (di_param[dev].enable_hw == param->enable_hw) {
		VDO_LOG(VDO_LOG_LVL_WARN, "dev %d sta %d set params repeatedly, do nothing\n", dev, param->enable_hw);
		goto FAIL;
	}

	if (atomic_read(&dev_use_di) != DEV_MAX && !di_is_cur_dev(dev)) {
		VDO_LOG(VDO_LOG_LVL_WARN, "dev %d cur dev %d di hw is busy \n", dev, atomic_read(&dev_use_di));
		ret = -EBUSY;
		goto FAIL;
	}

	di_param[dev].enable_hw = param->enable_hw;
	if (param->enable_hw) {
		di_param[dev].src_w = param->src_width;
		di_param[dev].src_h = param->src_height;
		di_param[dev].dst_w = param->dst_width;
		di_param[dev].dst_h = param->dst_height;
		di_param[dev].src_rect.left = param->src_rect.left;
		di_param[dev].src_rect.top = param->src_rect.top;
		di_param[dev].src_rect.right = param->src_rect.right;
		di_param[dev].src_rect.bottom = param->src_rect.bottom;

		di_param[dev].dst_rect.left = param->dst_rect.left;
		di_param[dev].dst_rect.top = param->dst_rect.top;
		di_param[dev].dst_rect.right = param->dst_rect.right;
		di_param[dev].dst_rect.bottom = param->dst_rect.bottom;

		di_param[dev].scan_line_mode = param->scan_line_mode;
		di_param[dev].deinterlace_mode = VDP_DI_MA4F_MODE;
		di_param[dev].first_field = 1;
		di_param[dev].top_field_first = 0;
		di_param[dev].flags |= param->flag;
		di_param[dev].stride = param->stride;
		//register di callback to wch
		di_register_get_addr_fun(di_get_addr_fun);
		vPmxHalIsrInit();
#ifdef	DI_TZG_OUTPUT_EN
		di_set_tzg_offset(dev, param->dst_width, param->dst_height, param->stride, DI_FMT_YUV420);
#endif
		atomic_set(&dev_use_di, dev);
	} else {
		di_trigger_timer_deinit();
		di_register_get_addr_fun(NULL);
#ifdef	DI_TZG_OUTPUT_EN
		di_clr_tzg_offset(dev);
#endif
		atomic_set(&dev_use_di, DEV_MAX);
	}

	VDO_LOG(VDO_LOG_LVL_INFO, "dev:%d en:%d src_w:%d src_h:%d dst_w:%d dst_h:%d scan_line:%d stride:%d\n",
		dev, di_param[dev].enable_hw, di_param[dev].src_w, di_param[dev].src_h, di_param[dev].dst_w,
		di_param[dev].dst_h, di_param[dev].scan_line_mode, di_param[dev].stride);
	VDO_LOG(VDO_LOG_LVL_INFO, "flag:%x di_mode:%d f_field:%d top_ff:%d call_back:%p, cur dev:%d\n",
		di_param[dev].flags, di_param[dev].deinterlace_mode, di_param[dev].first_field,
		di_param[dev].top_field_first, di_param[dev].getdibuffer, atomic_read(&dev_use_di));

FAIL:
#ifndef ATC_ARM2_LK_SUPPORT
	mutex_unlock(&di_param[dev].lock);
#endif
	//if (!param->enable_hw)
		//memset(&di_param[dev], 0, sizeof(di_param[dev]));

	LEAVE();
	return ret;
}

int di_hal_init(int dev)
{
	u32 ret = 0;
	ENTER();

	if (dev >= DEV_MAX || dev < 0) {
		VDO_LOG(VDO_LOG_LVL_ERR, "devcie id is out of range \n");
		return -EINVAL;
	}
#ifndef ATC_ARM2_LK_SUPPORT
	mutex_lock(&di_param[dev].lock);
#endif
	if (!di_param[dev].enable_hw) {
		VDO_LOG(VDO_LOG_LVL_ERR, "hardware is not enable \n");
#ifndef ATC_ARM2_LK_SUPPORT
		mutex_unlock(&di_param[dev].lock);
#endif
		return -EPERM;
	}
	/*before start wch should make sure disable fmt*/
	vPmxHalDisableFmt(VDP_1);

	/*Enable di clock*/
	ret = wch_hw_init(WCH_DI);
	if(ret) {
		VDO_LOG(VDO_LOG_LVL_ERR, "line %d ret %x", __LINE__, ret);
	}

	di_start_wch(di_param[dev].src_w, di_param[dev].src_h, di_param[dev].dst_w, di_param[dev].dst_h, di_param[dev].stride);

	ret = wch_hw_start(WCH_DI);
	if(ret) {
		VDO_LOG(VDO_LOG_LVL_ERR, "line %d ret %x", __LINE__, ret);
	}

	di_trigger_timer_init();

#ifndef ATC_ARM2_LK_SUPPORT
	mutex_unlock(&di_param[dev].lock);
#endif

	LEAVE();
	return ret;
}

int di_hal_deinit(int dev)
{
	u32 ret = 0;
	ENTER();

	vPmxHalDisableFmt(VDP_1);

	ret = wch_hw_stop(WCH_DI);
	if(ret) {
		VDO_LOG(VDO_LOG_LVL_ERR, "line %d ret %x", __LINE__, ret);
	}
	ret = wch_hw_deinit(WCH_DI);
	if(ret) {
		VDO_LOG(VDO_LOG_LVL_ERR, "line %d ret %x", __LINE__, ret);
	}

	LEAVE();
	return ret;
}

#if 0
static int di_trigger(void *vdp_info, uintptr_t *yin_addr, uintptr_t *cin_addr)
{
	int ret = 0;
	di_internal_params *param = (di_internal_params *)vdp_info;
	ENTER();

	vPmxHalInit(param->dst_w, param->dst_h, param->src_w, param->src_h);
	vPmxHalTrigerDIModule(0);

	set_vdp_hal_info(param, VDP_1);

	if (!vPmxHalGetFmtEn(VDP_1)) {
		/* Enable FMT to show video*/
		vPmxHalEnableFmt(VDP_1);
		VDO_LOG(VDO_LOG_LVL_INFO, "%s hw %d enable fmt\r\n", __func__, VDP_1);
	}

	VDO_LOG(VDO_LOG_LVL_DBG, "%s top:%d  fst:%d\n", __func__, param->top_field_first, param->first_field);

	ret = update_hw_addr((u32 *)yin_addr, (u32 *)cin_addr, param);
	vVdpHalSetFieldInfo(VDP_1, param->top_field_first, param->first_field);
	if (param->getdibuffer) {
		param->first_field = !param->first_field;
	}

	vPmxHalTrigger();
	LEAVE();
	return ret;
}
#endif

int di_work(int dev, uintptr_t *yin_addr, uintptr_t *cin_addr, uintptr_t ycout_addr, bool top_field_fst)
{
	di_internal_params *param = NULL;
	uintptr_t y_addr = 0;
	uintptr_t c_addr = 0;
	int ret = 0;

	ENTER();
	if (dev >= DEV_MAX || dev < 0) {
		VDO_LOG(VDO_LOG_LVL_ERR, "devcie id is out of range \n");
		return -EINVAL;
	}

	if (yin_addr == NULL || cin_addr == NULL || !ycout_addr) {
		VDO_LOG(VDO_LOG_LVL_ERR, "yin_addr:%p cin_addr:%p or ycout_addr:%p is NULL\n",
				yin_addr, cin_addr, (void *)ycout_addr);
		return -EINVAL;
	}

	param = (di_internal_params *)&di_param[dev];
#ifndef ATC_ARM2_LK_SUPPORT
	mutex_lock(&param->lock);
#endif
	if (!param->enable_hw) {
		VDO_LOG(VDO_LOG_LVL_ERR, "hardware is not enable \n");
		ret = -EPERM;
		goto FAIL;
	}

	param->top_field_first = top_field_fst;

	ret = update_hw_addr((u32 *)yin_addr, (u32 *)cin_addr, param);
	if(ret) {
		VDO_LOG(VDO_LOG_LVL_ERR, "line %d ret %x", __LINE__, ret);
	}
	set_vdp_hal_info(param, VDP_1);
	VDO_LOG(VDO_LOG_LVL_DBG, "%s top:%d  fst:%d\n", __func__, param->top_field_first, param->first_field);
	if (param->getdibuffer) {
		param->first_field = !param->first_field;
	}

	/*support 420 422 output*/
	y_addr = ycout_addr;
	//c_addr = ycout_addr + param->src_width * param->src_height;

	//set out put buffer to wch
	ret = wch_hw_set_di_buffer(WCH_DI, y_addr, c_addr);
	if(ret) {
		VDO_LOG(VDO_LOG_LVL_ERR, "line %d ret %x", __LINE__, ret);
	}
	wch_set_di_triger_status(WCH_DI);

	vPmxHalEnableFmt(VDP_1);
	vPmxHalTrigger();

FAIL:
#ifndef ATC_ARM2_LK_SUPPORT
	mutex_unlock(&param->lock);
#endif

	LEAVE();
	return ret;
}

int di_set_output_buf_status(u32 buf_idex, u32 buf_status)
{
	if (buf_idex >= DI_BUF_MAX) {
		VDO_LOG(VDO_LOG_LVL_INFO, " buffer index overflow  %d\n", buf_idex);
		return -EINVAL;
	}

	SpinLock(0);
	if (atomic_read(&di_outbuf_list[buf_idex].status) != (s32)buf_status) {
		VDO_LOG(VDO_LOG_LVL_DBG, "out buf idx:%d status %d addr:%p %p\n", buf_idex, buf_status,
			(void *)di_outbuf_list[buf_idex].y_addr, &di_outbuf_list[buf_idex]);
		atomic_set(&di_outbuf_list[buf_idex].status, buf_status);
	} else {
		VDO_LOG(VDO_LOG_LVL_WARN, "set buffer status override  idx %d, status:%d\n", buf_idex, buf_status);
		SpinUnlock(0);
		return -EPERM;
	}
	SpinUnlock(0);

	return 0;//wch_hw_set_di_buf_status( WCH_DI, buf_idex, buf_status);
}

#ifndef ATC_ARM2_LK_SUPPORT
void di_work_func(struct work_struct *work)
{
	struct di_work_struct *p_work = container_of(work, struct di_work_struct, work_store_buf);
	int i = 0, j = 0;
#if DI_DUMP_BUFFER_ATTR
	static  int k;
#endif
	int ret = 0;
	uintptr_t yc_out = 0;
	uintptr_t gbg_addr = get_wch_garbage_addr();
	ENTER();

	ret = di_hal_init(p_work->type);
	if (ret < 0) {
		VDO_LOG(VDO_LOG_LVL_ERR, "hardware is not enable err:%d\n", ret);
		goto FAIL;
	}

	if (p_work->type == AVM) {
		if (singleChannelFlag) {
			yc_out = p_work->work_buf_out[0][0];
			ret = di_work(p_work->type, (void *)p_work->work_buf_inY[0], (void *)p_work->work_buf_inC[0],
						yc_out, p_work->top_filed_fst[0]);
			if (ret < 0) {
				VDO_LOG(VDO_LOG_LVL_ERR, "hardware is not enable err:%d\n", ret);
				goto FAIL;
			}

			//wait di work done
			di_wait_trigger_event();
		} else {
			for (i = 0; i < VIDEO_CHANNEL_NUM; i++) {
#if DI_DUMP_BUFFER_ATTR
				if (i == 0) {
					di_save_yc_addr(DI_OUT_BUF, k%DI_BUF_MAX_CNT, p_work->work_buf_out[i][0], DI_IN_BUF_YSIZE * 2);
					di_save_yc_addr(DI_IN_BUF, k++%DI_BUF_MAX_CNT, p_work->work_buf_inY[i][0], p_work->work_buf_inC[i][0]);
				}
#endif
#ifdef	DI_TZG_OUTPUT_EN
				yc_out = p_work->work_buf_out[0][0] +  addr_offset[i];
#else
				yc_out = p_work->work_buf_out[i][0];
#endif
				ret = di_work(p_work->type, (void *)p_work->work_buf_inY[i], (void *)p_work->work_buf_inC[i],
						yc_out, p_work->top_filed_fst[i]);
				if (ret < 0) {
					VDO_LOG(VDO_LOG_LVL_ERR, "hardware is not enable err:%d\n", ret);
					goto FAIL;
				}

				//wait di work done
				di_wait_trigger_event();
			}
		}
	} else if (p_work->type == CVBS) {
		for (i = 0; i < 2; i++) {
			yc_out = p_work->work_buf_out[0][i];

			SpinLock(0);
			for (j = 0; j < DI_BUF_MAX; j++) {
				if (di_outbuf_list[j].y_addr == yc_out) {
					break;
				}
			}
			SpinUnlock(0);

			if (j == DI_BUF_MAX) {
				VDO_LOG(VDO_LOG_LVL_ERR, "out buffer %p not find in buf list \n", (void *)yc_out);
				yc_out = gbg_addr;
			}
			if ((skip_cnt < DI_BUFFER_SKIP_CNT)
				|| ((j < DI_BUF_MAX) && atomic_read(&di_outbuf_list[j].status) == AVIN_BUF_BUSY)) {
				VDO_LOG(VDO_LOG_LVL_INFO, "skip count %d. idx %d %d, status:%d y_addr:%p \n",
					skip_cnt, i, j, atomic_read(&di_outbuf_list[j].status),
					(void *)di_outbuf_list[j].y_addr);
				yc_out = gbg_addr;
			}

			ret = di_work(p_work->type, (void *)p_work->work_buf_inY[0], (void *)p_work->work_buf_inC[0],
					yc_out, 0);
			if (ret < 0) {
				VDO_LOG(VDO_LOG_LVL_ERR, "hardware is not enable err:%d\n", ret);
				goto FAIL;
			}

			//wait di work done
			di_wait_trigger_event();

			if ((j < DI_BUF_MAX) && (skip_cnt >= DI_BUFFER_SKIP_CNT) && (yc_out != gbg_addr)) {
				di_set_output_buf_status(j, AVIN_BUF_BUSY);
				di_param[p_work->type].getdibuffer(&di_outbuf_list[j]);
				VDO_LOG(VDO_LOG_LVL_DBG, "%s callback backcar  idx %d %d , y_addr:%p \n", __func__, j, i,
					(void *)di_outbuf_list[j].y_addr);
			}
			skip_cnt++;
			//wait time
			VDO_LOG(VDO_LOG_LVL_DBG, "%s  \n", __func__);
		}
	}

	ret = di_hal_deinit(p_work->type);
	if (ret < 0) {
		VDO_LOG(VDO_LOG_LVL_ERR, "hardware is not enable err:%d\n", ret);
		goto FAIL;
	}

FAIL:
	if (p_work->type == AVM) {
		timeline_inc(p_work->timeline_in, 1);
		timeline_inc(p_work->timeline_out, 1);
	} else if (p_work->type == CVBS) {
		SpinLock(0);
		if (di_inbuf_list[p_work->in_idx] && atomic_read(&di_inbuf_list[p_work->in_idx]->status) == AVIN_BUF_BUSY) {
			atomic_set(&di_inbuf_list[p_work->in_idx]->status, AVIN_BUF_FREE);
			VDO_LOG(VDO_LOG_LVL_DBG,  "%s set idx %d  addr %p free\r\n", __func__, p_work->in_idx,
				(void *)di_inbuf_list[p_work->in_idx]->y_addr);
		}
		SpinUnlock(0);
	}

	kfree(p_work);
	LEAVE();
}
#endif

unsigned int di_hw_init(void)
{
#if 0
	memset(&DI_INTER_PARAM,0,sizeof(atc_di_device));
	/*Enable di clock*/
	wch_hw_init(WCH_DI);
	DI_INTER_PARAM.param.u4Idx = VDP_1;
#ifndef ATC_ARM2_LK_SUPPORT
	VDO_LOG(VDO_LOG_LVL_WARN, " %s set getfd false \r\n",__func__);
	spin_lock(&di_fd_lock);
	getfd = false;
	spin_unlock(&di_fd_lock);
#else
	initalDIModule();
	//timer_initialize(&di_work_timer);

	getfd = true;
#endif
#endif
	return 0;
}

unsigned int di_hw_deinit(void)
{
	ENTER();
#if 0
	/*Disable di clock*/
	VDO_LOG(VDO_LOG_LVL_WARN, " %s enter \r\n",__func__);
	VDO_LOG(VDO_LOG_LVL_WARN, " %s set getfd false \r\n",__func__);
#ifndef ATC_ARM2_LK_SUPPORT
	spin_lock(&di_fd_lock);
	getfd = false;
	spin_unlock(&di_fd_lock);
#endif
	wch_hw_deinit(WCH_DI);
	VDO_LOG(VDO_LOG_LVL_WARN, " %s leave \r\n",__func__);
#endif
	LEAVE();
	return 0;
}

#ifdef ATC_ARM2_LK_SUPPORT
void di_irq_handler(void)
{
	static int irq_num;
	uintptr_t y_addr[BUFFER_NUM] = {0};
	uintptr_t c_addr[BUFFER_NUM] = {0};
	uintptr_t out_addr = 0;
	int i = 0;
	int ret = 0;

	if (irq_num < 6) {
		VDO_LOG(VDO_LOG_LVL_WARN, "%s entry %d %d in_idx:%d\n", __func__, irq_num, inbuf_cnt, inbuf_idx);
		irq_num++;
	}

	DRV_WriteReg32(IRQCLR_REG, ARM9_TIMER2_INTERRUPT_CLEAR); //clr T2 Interrupt
	if (atomic_read(&getBufferDone) < 1) {
		VDO_LOG(VDO_LOG_LVL_DBG, "%s cnt:%d in_idx:%d\n", __func__, inbuf_cnt, inbuf_idx);
		return;
	}

	SpinLock(0);
	i = inbuf_idx;

	if (inbuf_cnt >= 3 && di_inbuf_list[i] != NULL) {
		VDO_LOG(VDO_LOG_LVL_DBG, "%s entry %d %d %d\n", __func__,
			di_inbuf_list[i]->vsync_cnt, atomic_read(&di_outbuf_list[outbuf_idx].status), outbuf_idx);
		if (di_inbuf_list[i]->vsync_cnt > 0 && atomic_read(&di_outbuf_list[outbuf_idx].status) == AVIN_BUF_FREE) {
			y_addr[0] = di_inbuf_list[i]->y_addr;
			c_addr[0] = di_inbuf_list[i]->c_addr;
			y_addr[1] = di_inbuf_list[(i+1) % DI_BUF_MAX]->y_addr;
			c_addr[1] = di_inbuf_list[(i+1) % DI_BUF_MAX]->c_addr;
			y_addr[2] = di_inbuf_list[(i+2) % DI_BUF_MAX]->y_addr;
			c_addr[2] = di_inbuf_list[(i+2) % DI_BUF_MAX]->c_addr;

			out_addr = di_outbuf_list[outbuf_idx].y_addr;
			di_set_output_buf_status(outbuf_idx, AVIN_BUF_BUSY);
			outbuf_idx = (outbuf_idx + 1) % DI_BUF_MAX;
			di_inbuf_list[i]->vsync_cnt--;

			VDO_LOG(VDO_LOG_LVL_DBG, "%s index %d %p\n", __func__, i, (void *)out_addr);

			ret = di_work(CVBS, y_addr, c_addr, out_addr, 0);

			atomic_set(&getBufferDone, 0);

			if (ret < 0) {
				SpinUnlock(0);
				VDO_LOG(VDO_LOG_LVL_ERR, "%s trigger DI error \n", __func__);
				return;
			}

		}

		if (!di_inbuf_list[i]->vsync_cnt) {
			if (di_inbuf_list[i] && atomic_read(&di_inbuf_list[i]->status) == AVIN_BUF_BUSY) {
				atomic_set(&di_inbuf_list[i]->status, AVIN_BUF_FREE);
				VDO_LOG(VDO_LOG_LVL_DBG,  "%s set idx %d  addr %p free\n", __func__, i,
					(void *)di_inbuf_list[i]->y_addr);
			}
			inbuf_idx = (inbuf_idx + 1) % DI_BUF_MAX;
			inbuf_cnt--;
		}

	}
	SpinUnlock(0);
}
#else
atomic_t timer_enable = ATOMIC_INIT(0);
static struct hrtimer di_triger_timer;
static enum hrtimer_restart di_timer_handler(struct hrtimer *timer)
{
	static int i, start_s, start_ms;
	struct timeval start = {0}, end = {0};

	if (i == 0) {
		do_gettimeofday(&start);
		start_s = start.tv_sec;
		start_ms = start.tv_usec;
	}
	if (i < 1000) {
		i++;
		if (i == 1000) {
			do_gettimeofday(&end);
			VDO_LOG(VDO_LOG_LVL_INFO, "di trigger count:1000, time:%ld.%ld s\n",
				end.tv_sec - start_s, end.tv_usec - start_ms);
			i = 0;
		}
	}

	if (atomic_read(&getBufferDone) > 0) {
		di_signal_trigger_event();
		atomic_set(&getBufferDone, 0);
	}

	if (di_param[CVBS].enable_hw) {
		if (di_param[CVBS].src_h == 576) {
			hrtimer_forward_now(timer, ns_to_ktime(PAL_CYCLE));
		} else if (di_param[CVBS].src_h == 480) {
			hrtimer_forward_now(timer, ns_to_ktime(NTSC_CYCLE));
		} else {
			VDO_LOG(VDO_LOG_LVL_ERR, "di timer not support src_h %d\n", di_param[CVBS].src_h);
			return HRTIMER_NORESTART;
		}
	} else {
		hrtimer_forward_now(timer, ns_to_ktime(AVM_CYCLE));
	}

	return HRTIMER_RESTART;
}
#endif

void di_trigger_timer_init(void)
{
	ENTER();
#ifndef ATC_ARM2_LK_SUPPORT
	if (atomic_read(&timer_enable)) {
		return;
	}
	hrtimer_init(&di_triger_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di_triger_timer.function = di_timer_handler;

	if (di_param[CVBS].enable_hw) {
		if (di_param[CVBS].src_h == 576) {
			hrtimer_start(&di_triger_timer, ns_to_ktime(PAL_CYCLE), HRTIMER_MODE_REL);
		} else if (di_param[CVBS].src_h == 480) {
			hrtimer_start(&di_triger_timer, ns_to_ktime(NTSC_CYCLE), HRTIMER_MODE_REL);
		} else {
			VDO_LOG(VDO_LOG_LVL_ERR, "not support src_h %d\n", di_param[CVBS].src_h);
		}
	} else {
		hrtimer_start(&di_triger_timer, ns_to_ktime(AVM_CYCLE), HRTIMER_MODE_REL);
	}
	atomic_set(&timer_enable, 1);
#else
	if (di_param[CVBS].src_h == 576) {
		platform_set_periodic_timer2(PAL_TICKS);
	} else if (di_param[CVBS].src_h == 480) {
		platform_set_periodic_timer2(NTSC_TICKS);
	} else {
		VDO_LOG(VDO_LOG_LVL_ERR, "not support src_h %d\n", di_param[CVBS].src_h);
	}
#endif

	LEAVE();
}

void di_trigger_timer_deinit(void)
{
	ENTER();
#ifndef ATC_ARM2_LK_SUPPORT
	if (!atomic_read(&timer_enable)) {
		return;
	}
	hrtimer_cancel(&di_triger_timer);
	atomic_set(&timer_enable, 0);
#else
	mt_irq_mask(ARM9_TIMER2_IRQ_ID); //disable T2 IRQ
#endif
	LEAVE();
}

unsigned int di_hw_start(void)
{
#if 0
	vPmxHalIsrInit();
#ifndef ATC_ARM2_LK_SUPPORT
	di_trigger_timer_init();
#else
	if(DI_INTER_PARAM.param.u4SrcHeight == 576) {
		platform_set_periodic_timer2(PAL_TICKS);
	}else if(DI_INTER_PARAM.param.u4SrcHeight == 480) {
		platform_set_periodic_timer2(NTSC_TICKS);
	}
#endif
#endif
	return 0;
}


/*Clean di param */

bool data_valid = false;
unsigned int di_hw_stop(void)
{
	di_params param = {0};
	int i = 0;
	int ret = 0;

	ENTER();
#ifndef ATC_ARM2_LK_SUPPORT
	/*delete work queue*/
	if (cvbs_wq) {
		flush_workqueue(cvbs_wq);
	}
	dev_ion_free();
#else
	di_hal_deinit(CVBS);
#endif
	SpinLock(0);
	for (i = 0; i < DI_BUF_MAX; i++) {
		if (di_inbuf_list[i]) {
			atomic_set(&di_inbuf_list[i]->status, AVIN_BUF_FREE);
			di_inbuf_list[i] = NULL;
		}
		memset(&di_outbuf_list[i], 0, sizeof(di_outbuf_list[0]));
	}
	outbuf_idx = 0;
	inbuf_cnt = 0;
	inbuf_idx = 0;
	SpinUnlock(0);
	skip_cnt = 0;

#ifndef ATC_ARM2_LK_SUPPORT
	atomic_set(&getBufferDone, 0);
	free_irq(fmtf_irq, NULL);
#else
	atomic_set(&getBufferDone, 1);
	mt_irq_mask(ARM9_DISPFMT_VSYNC_IRQ_ID); /*disable dispfmt IRQ*/
#endif
	param.enable_hw = false;
	ret = di_hal_set_params(CVBS, &param);

	LEAVE();
	return ret;
}

int di_hw_config(DI_PRM_T *param)
{
	int ret = 0;
	di_params params = {0};
	ENTER();

	if (NULL == param) {
		VDO_LOG(VDO_LOG_LVL_ERR, "param is null\n");
		return -EINVAL;
	}
#ifdef ATC_ARM2_LK_SUPPORT
	mt_irq_unmask(ARM9_DISPFMT_VSYNC_IRQ_ID); /*enable dispfmt IRQ*/

	WriteREG(VDPSYS_CG_CON0, 0x0);
	/*enable di clk*/
	//CLKWriteREGMsk(0x0000, 0x80220, 0x1803E0);
	CLKWriteREGMsk(0x0000, 0x220, 0x3E0);
	VDO_LOG(VDO_LOG_LVL_INFO, "DI clock[0x00C0]:0x%x clk:%x\r\n", CLKReadREG(0x0000), ReadREG(VDPSYS_CG_CON0));
	initalDIModule();

	di_init_params();
#else
	ret = request_irq(fmtf_irq, dispfmt_vsync_isr, IRQF_TRIGGER_LOW, "dispfmt_isr", NULL);
	if (ret < 0) {
		VDO_LOG(VDO_LOG_LVL_ERR, "di hw config register irq fali ret=%d\n", ret);
		return ret;
	}

#endif

	params.enable_hw = true;
	params.src_height = param->src_height;
	params.src_width = param->src_width;
	params.dst_width = param->dst_width;
	params.dst_height = param->dst_height;
	params.src_rect.top = param->src_rect.top;
	params.src_rect.left  = param->src_rect.left;
	params.src_rect.bottom = param->src_rect.bottom;
	params.src_rect.right = param->src_rect.right;

	params.dst_rect.top = param->dst_rect.top;
	params.dst_rect.left  = param->dst_rect.left;
	params.dst_rect.bottom = param->dst_rect.bottom;
	params.dst_rect.right = param->dst_rect.right;
	params.scan_line_mode = 0;
	params.flag = 0;
	params.stride = param->dst_width;
	ret = di_hal_set_params(CVBS, &params);
	if (ret < 0) {
		goto FAIL;
	}
#ifndef ATC_ARM2_LK_SUPPORT
	mutex_lock(&di_param[CVBS].lock);
#endif
	di_param[CVBS].getdibuffer = param->getdibuffer;
#ifndef ATC_ARM2_LK_SUPPORT
	mutex_unlock(&di_param[CVBS].lock);
#else
	set_di_outbuf();

	di_hal_init(CVBS);
#endif
FAIL:
	LEAVE();
	return ret;
}

unsigned int di_buffer_receive_sync(void *inputBuffer)
{
	AVIN_BUFFER_T *buffer_info = (AVIN_BUFFER_T *)inputBuffer;
#ifndef ATC_ARM2_LK_SUPPORT
	int i = 0;
	int idx = 0;
	struct di_work_struct *di_work = NULL;
#endif
	if (NULL == inputBuffer) {
		return -EINVAL;
	}

	if (!di_is_cur_dev(CVBS)) {
		VDO_LOG(VDO_LOG_LVL_ERR, " di hw is busy\n");
		return -EPERM;
	}

#ifndef ATC_ARM2_LK_SUPPORT

#if DI_DUMP_BUFFER_ATTR
	if (g_DI_BUF_ADDR.di_in_buf_cnt < DI_BUF_MAX_CNT) {
		di_save_yc_addr(DI_IN_BUF, g_DI_BUF_ADDR.di_in_buf_cnt, buffer_info->y_addr, buffer_info->c_addr);
	}
#endif
	if (AVIN_BUF_FREE == atomic_read(&buffer_info->status)) {
		VDO_LOG(VDO_LOG_LVL_ERR, " di reciver buffer index %u invalid\n", buffer_info->idex);
		vss_aee_print(" di reciver buffer index %u invalid\n",  buffer_info->idex);
	}

	di_work = kmalloc(sizeof(*di_work), GFP_KERNEL);
	if (!di_work) {
		VDO_LOG(VDO_LOG_LVL_ERR, " no memory\n");
		//mutex_unlock(&di_buffer_mutex);
		return -ENOMEM;
	}
#endif
	SpinLock(0);

	di_inbuf_list[buffer_info->idex] = (di_buf_t *)buffer_info;
	inbuf_cnt++;

	/*one buffer use twice*/
	di_inbuf_list[buffer_info->idex]->vsync_cnt = 2;

	VDO_LOG(VDO_LOG_LVL_DBG, "di reciver buffer index %u cnt %d y%p sta:%d\n", buffer_info->idex, inbuf_cnt,
		(void *)di_inbuf_list[buffer_info->idex]->y_addr, atomic_read(&buffer_info->status));
#ifndef ATC_ARM2_LK_SUPPORT
	if (inbuf_cnt >= 3) {
		INIT_WORK(&(di_work->work_store_buf), di_work_func);
		i = (buffer_info->idex - 2 + DI_BUF_MAX) % DI_BUF_MAX;
		//must keep buffer order
		if (!di_inbuf_list[i]) {
			VDO_LOG(VDO_LOG_LVL_ERR, "%s %d is null \n", __func__, i);
			kfree(di_work);
			goto FAIL;
		}

		//VDO_LOG(VDO_LOG_LVL_INFO, "%s index %d %p %p\n", __func__, i, );
		if (AVIN_BUF_BUSY == atomic_read(&di_inbuf_list[i]->status)) {
			di_work->work_buf_inY[0][0] = di_inbuf_list[i]->y_addr;
			di_work->work_buf_inC[0][0] = di_inbuf_list[i]->c_addr;
			di_work->work_buf_inY[0][1] = di_inbuf_list[(i+1) % DI_BUF_MAX]->y_addr;
			di_work->work_buf_inC[0][1] = di_inbuf_list[(i+1) % DI_BUF_MAX]->c_addr;
			di_work->work_buf_inY[0][2] = di_inbuf_list[(i+2) % DI_BUF_MAX]->y_addr;
			di_work->work_buf_inC[0][2] = di_inbuf_list[(i+2) % DI_BUF_MAX]->c_addr;

			idx = outbuf_idx;

			di_work->work_buf_out[0][0] = di_outbuf_list[outbuf_idx].y_addr;
			outbuf_idx = (outbuf_idx + 1) % DI_BUF_MAX;
			di_work->work_buf_out[0][1] = di_outbuf_list[outbuf_idx].y_addr;
			outbuf_idx = (outbuf_idx + 1) % DI_BUF_MAX;

			VDO_LOG(VDO_LOG_LVL_DBG, "%s index %d %x %x\n", __func__, i, di_work->work_buf_out[0][0],
				di_work->work_buf_out[0][1]);
			di_work->in_idx = i;
		}

		di_work->top_filed_fst[0] = 1;
		di_work->type = CVBS;
		queue_work(cvbs_wq, &di_work->work_store_buf);
		inbuf_cnt--;
	}
FAIL:

#endif
	SpinUnlock(0);

	return 0;
}

#ifndef ATC_ARM2_LK_SUPPORT
int di_buffer_output_receive(unsigned int *pfd,unsigned int fd_num)
{
	int bufIdx = 0;
	physical_block phy_info = {0};

	VDO_LOG(VDO_LOG_LVL_INFO, " %s start\r\n",__func__);

	if(pfd == NULL||fd_num < 1||fd_num > di_buf_num) {
		return -EINVAL;
	}
	if(fd_num < di_buf_num) {
		di_buf_num = fd_num;
	}

#if 0
	mutex_lock(&di_buffer_mutex);
	for (bufIdx = 0; bufIdx < fd_num; bufIdx++) {
		if (ion_handles[bufIdx])
			ion_free(g_ion_client, ion_handles[bufIdx]);
		ion_handles[bufIdx] = di_ion_to_phyaddr(pfd[bufIdx],&phy_info);

#if DI_DUMP_BUFFER_ATTR
		if (g_DI_BUF_ADDR.di_out_buf_cnt < DI_BUF_MAX_CNT) {
			di_save_yc_addr(DI_OUT_BUF, g_DI_BUF_ADDR.di_out_buf_cnt, phy_info.addr, phy_info.size);
		}
#endif
		if (phy_info.addr) {
			di_outbuf_list[bufIdx].y_addr = phy_info.addr;
		}
		VDO_LOG(VDO_LOG_LVL_INFO, " %s fd %d ,0x%x  ion addr\r\n",__func__,
								pfd[bufIdx],phy_info.addr);
	}
	mutex_unlock(&di_buffer_mutex);
	//di_hw_triger(DI_INTER_PARAM);

	di_start_wch((DI_INTER_PARAM.param.rDstRect.right-DI_INTER_PARAM.param.rDstRect.left),
		(DI_INTER_PARAM.param.rDstRect.bottom-DI_INTER_PARAM.param.rDstRect.top));

	wch_hw_start(WCH_DI);
	spin_lock(&di_fd_lock);
	getfd = true;
	spin_unlock(&di_fd_lock);
#else
	//mutex_lock(&di_buffer_mutex);

	for (bufIdx = 0; bufIdx < fd_num; bufIdx++) {
		di_ion_to_phyaddr(pfd[bufIdx], &phy_info);

#if DI_DUMP_BUFFER_ATTR
		if (g_DI_BUF_ADDR.di_out_buf_cnt < DI_BUF_MAX_CNT) {
			di_save_yc_addr(DI_OUT_BUF, g_DI_BUF_ADDR.di_out_buf_cnt, phy_info.addr, phy_info.size);
		}
#endif
		SpinLock(0);
		if (phy_info.addr) {
			di_outbuf_list[bufIdx].y_addr = phy_info.addr;
			di_outbuf_list[bufIdx].idex = bufIdx;
		}
		SpinUnlock(0);
		VDO_LOG(VDO_LOG_LVL_INFO, " %s fd %d ,0x%x ion addr\r\n", __func__, pfd[bufIdx], phy_info.addr);
	}
	//mutex_unlock(&di_buffer_mutex);

#endif
	return 0;
}


EXPORT_SYMBOL(di_hw_deinit);
EXPORT_SYMBOL(di_hw_init);
EXPORT_SYMBOL(di_hw_start);
EXPORT_SYMBOL(di_hw_stop);
EXPORT_SYMBOL(di_hw_config);
EXPORT_SYMBOL(di_buffer_output_receive);
EXPORT_SYMBOL(di_set_output_buf_status);
void di_read_template(uintptr_t virt_addr, const unsigned int size, const unsigned char diReadFile[])
{
	struct file *fp = NULL;
	int ret = 0;
	loff_t offset = 0;

	VDO_LOG(VDO_LOG_LVL_DBG, "%s in.\n.", __func__);
	fp = filp_open(diReadFile, O_RDONLY, 0777);
	if (IS_ERR(fp)) {
		ret = PTR_ERR(fp);
		VDO_LOG(VDO_LOG_LVL_DBG, "%s open %s error! Error Num[%d]\n", __func__, diReadFile, ret);
		return;
	}

	ret = kernel_read(fp, offset, (void *)virt_addr, size);
	VDO_LOG(VDO_LOG_LVL_INFO, "kernel read ret code is %d\n", ret);
	filp_close(fp, NULL);
	VDO_LOG(VDO_LOG_LVL_DBG, "%s out.\n.", __func__);
}

/* read file from /sdcard and write data to func param */
void di_read_file(uintptr_t file_in_Y, uintptr_t file_in_C, int i, bool is_virt_addr)
{
	unsigned char file_name[50] = "";
	uintptr_t addr_in_y = 0;
	uintptr_t addr_in_c = 0;

	VDO_LOG(VDO_LOG_LVL_DBG, "%s in.\n.", __func__);

	//for (int i = 0; i < 2; ++i) {
		sprintf(file_name, "/storage/BA0A-EBC4/di0_frame/even%dy.bin", i);
		addr_in_y = is_virt_addr ? file_in_Y :
					(uintptr_t) ioremap(file_in_Y, DI_IN_BUF_YSIZE);

		VDO_LOG(VDO_LOG_LVL_INFO, "%s addr_in_y is %p \n", __func__, (void *)addr_in_y);
#ifdef DI_INTERLACE_SOURCE_EN
		di_read_template(addr_in_y, DI_IN_BUF_YSIZE/2, file_name);
		sprintf(file_name, "/storage/BA0A-EBC4/di0_frame/odd%dy.bin", i);
		di_read_template(addr_in_y + DI_IN_BUF_YSIZE/2, DI_IN_BUF_YSIZE/2, file_name);
#else
		di_read_template(addr_in_y, DI_IN_BUF_YSIZE, file_name);
#endif
		sprintf(file_name, "/storage/BA0A-EBC4/di0_frame/even%dc.bin", i);
		addr_in_c = is_virt_addr ? file_in_C :
					(uintptr_t) ioremap(file_in_C, DI_IN_BUF_CSIZE);

		VDO_LOG(VDO_LOG_LVL_INFO, "%s addr_in_c is %p \n", __func__, (void *)addr_in_c);
#ifdef DI_INTERLACE_SOURCE_EN
		di_read_template(addr_in_c, DI_IN_BUF_CSIZE/2, file_name);
		sprintf(file_name, "/storage/BA0A-EBC4/di0_frame/odd%dc.bin", i);
		di_read_template(addr_in_c + DI_IN_BUF_CSIZE/2, DI_IN_BUF_CSIZE/2, file_name);
#else
		di_read_template(addr_in_c, DI_IN_BUF_CSIZE, file_name);
#endif
		if (!is_virt_addr) {
			iounmap((void *)addr_in_y);
			iounmap((void *)addr_in_c);
		}
	//}
	VDO_LOG(VDO_LOG_LVL_DBG, "%s out.\n.", __func__);
}


#if DI_DUMP_BUFFER_ATTR
void di_save_yc_addr(unsigned int di_io, unsigned int di_buf_idx, unsigned long y_addr, unsigned long c_addr)
{
	if (di_io >= DI_IO_NUM || di_buf_idx >= DI_BUF_MAX_CNT) {
		VDO_LOG(VDO_LOG_LVL_ERR, "di_save_yc_addr: di_io[%d], di_buf_idx[%d]\n", di_io, di_buf_idx);
		return;
	}

	if (di_io == DI_IN_BUF) {
		g_DI_BUF_ADDR.di_in_buf_cnt++;
	} else if (di_io == DI_OUT_BUF){
		g_DI_BUF_ADDR.di_out_buf_cnt++;
	}
	g_DI_BUF_ADDR.u4YAddr[di_io][di_buf_idx] = y_addr;//when out_buf mean ycAddr;
	g_DI_BUF_ADDR.u4CAddr[di_io][di_buf_idx] = c_addr;//when out_buf mean size;
}

void di_dump_register(void)
{
	unsigned *virt_addr = NULL;
	const unsigned int reg_size = DI_DUMP_REG_SIZE;//64 *4
	unsigned char diDumpFile[40] = "";

	//new add
	//const unsigned int vdpsys_reg_size = VDPSYS_REG_SIZE;
	unsigned int val = 0;
	HAL_VDO_UNION_T *prVdofReg = (HAL_VDO_UNION_T *)vdof_reg;

	// dump prPmxDisMainHwRegValue start
	sprintf(diDumpFile, "/data/di/prPmxDisMainHwRegValue.bin");
	virt_addr = (unsigned int *)&(_prPmxDispMainHwReg->au4Reg[0]);

	di_dump_template((unsigned long)virt_addr, reg_size, diDumpFile);
	// dump prPmxDisMainHwRegValue end
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] Dump Success! File:%s for DI\r\n", diDumpFile);
	sprintf(diDumpFile, "\n");//reset

	// dump prVdpHwRegs start
	sprintf(diDumpFile, "/data/di/prVdpHwRegs.bin");
	virt_addr = &(prVdofReg->au4Reg[0]);

	di_dump_template((unsigned long)virt_addr, reg_size, diDumpFile);
	// dump prVdpHwRegs end

	sprintf(diDumpFile, "\n");

	/*virt_addr = ioremap((void *)((uintptr_t)VDPSYS_BASE), vdpsys_reg_size);
	sprintf(diDumpFile, "/sdcard/di/VDPSYSConfigure.bin");
	di_dump_template((unsigned long)virt_addr, vdpsys_reg_size, diDumpFile);*/

	virt_addr = &val;
	val = VDPSYS_READREG(VDPSYS_INT_STATUS);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_INT_STATUS[0x%x] \r\n", val);
	val = VDPSYS_READREG(VDPSYS_INT_CLR);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_INT_CLR[0x%x] \r\n", val);
	val = VDPSYS_READREG(VDPSYS_DI_CONFIG);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_DI_CONFIG[0x%x] \r\n", val);
	val = VDPSYS_READREG(VDPSYS_CLK_CONFIG0);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_CLK_CONFIG0[0x%x] \r\n", val);
	val = VDPSYS_READREG(VDPSYS_CLK_CONFIG1);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_CLK_CONFIG1[0x%x] \r\n", val);
	val = VDPSYS_READREG(VDPSYS_VDO_INT_CLR);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_VDO_INT_CLR[0x%x] \r\n", val);
	val = VDPSYS_READREG(VDPSYS_DISPFMT_INT_CLR);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_DISPFMT_INT_CLR[0x%x] \r\n", val);
	val = VDPSYS_READREG(VDPSYS_WR_CHN_DI_INT_CLR);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_WR_CHN_DI_INT_CLR[0x%x] \r\n", val);
	val = VDPSYS_READREG(VDPSYS_CG_CON0);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_CG_CON0[0x%x] \r\n", val);
	val = VDPSYS_READREG(VDPSYS_SW_RST_B);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] VDPSYS_SW_RST_B[0x%x] \r\n", val);

	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_register] Dump Success! File:%s for DI\r\n", diDumpFile);
}

void di_dump_template(const unsigned long virt_addr, const unsigned int size, const unsigned char diDumpFile[])
{
	struct file *fp = NULL;
	mm_segment_t old_fs = 0;
	unsigned int ret = 0;
	loff_t pos = 0;
	fp = filp_open(diDumpFile, O_CREAT | O_RDWR, 0777);
	if (IS_ERR(fp)) {
		ret = PTR_ERR(fp);
		VDO_LOG(VDO_LOG_LVL_ERR, "[di_dump_buffer] open file %s error return %d\n", diDumpFile, ret);
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0;
	ret = vfs_write(fp, (char __user *)virt_addr, size, &pos);
	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_buffer] write file virt_addr[0x%lx],size[%d], pos[%d]\r\n",
		(unsigned long)virt_addr, (int)size, (int)pos);

	filp_close(fp, NULL);
	set_fs(old_fs);
}

unsigned int di_dump_buffer(unsigned int di_io, unsigned int idx)
{
	unsigned int buf_idx = 0;
	unsigned char diDumpFile[40] = "";
	unsigned int yc_size = 0;
	unsigned int y_size = 0;
	unsigned int c_size = 0;
	unsigned long yc_addr = 0;
	unsigned long y_addr = 0;
	unsigned long c_addr = 0;
	char *virt_addr = NULL;

	if (di_io >= DI_IO_NUM || idx < 0) {
		VDO_LOG(VDO_LOG_LVL_ERR, "di_save_yc_addr: di_io[%d], di_buf_idx[%d]\n", di_io, idx);
		return 1;
	}
	buf_idx = idx % DI_BUF_MAX_CNT;

	if (di_io == DI_IN_BUF) {
		y_size = DI_IN_BUF_YSIZE; //in 420 planar
		c_size = DI_IN_BUF_CSIZE;
		y_addr = g_DI_BUF_ADDR.u4YAddr[di_io][buf_idx];
		c_addr = g_DI_BUF_ADDR.u4CAddr[di_io][buf_idx];
		if (0 == y_addr || 0 == c_addr) { //add check null addr
			VDO_LOG(VDO_LOG_LVL_ERR, "[di_dump_buffer] %d addr %lx or size %lx null\n", __LINE__, y_addr, c_addr);
			return -ENXIO;
		}
		VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_buffer] %d y_addr %lx or c_addr %lx \n", __LINE__, y_addr, c_addr);

		/* dump y data begin */
		sprintf(diDumpFile, "/data/di/di_in-y%d.bin", idx);
		virt_addr = memremap(y_addr, y_size, MEMREMAP_WB);
		di_dump_template((unsigned long)virt_addr, y_size, diDumpFile);
		/* dump y data end */
		memunmap((void *)virt_addr);

		/* dump cbcr data begin */
		sprintf(diDumpFile, "/data/di/di_in-c%d.bin", idx);
		virt_addr = memremap(c_addr, c_size, MEMREMAP_WB);
		di_dump_template((unsigned long)virt_addr, c_size, diDumpFile);
		/* dump cbcr data end */
		memunmap((void *)virt_addr);
	} else if (di_io == DI_OUT_BUF) {
		yc_addr = g_DI_BUF_ADDR.u4YAddr[di_io][buf_idx];//out 422 packed
		yc_size = g_DI_BUF_ADDR.u4CAddr[di_io][buf_idx];
		if (0 == yc_addr || 0 == yc_size) {
			VDO_LOG(VDO_LOG_LVL_ERR, "[di_dump_buffer] %d addr %lx or size %u null\n", __LINE__, yc_addr, yc_size);
			return -ENXIO;
		}
		VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_buffer] %d addr %lx or size %u \n", __LINE__, yc_addr, yc_size);

		/* dump yc data begin */
		sprintf(diDumpFile, "/data/di/di_out-yc%d.bin", idx);
		virt_addr = memremap(yc_addr, yc_size, MEMREMAP_WB);
		if (virt_addr) {
			di_dump_template((unsigned long)virt_addr, yc_size, diDumpFile);
			/* dump yc data end */
			memunmap((void *)virt_addr);
		}
	}

	VDO_LOG(VDO_LOG_LVL_INFO, "[di_dump_buffer] Dump Success! File:%s for DI\r\n", diDumpFile);

	return 0;
}
#endif
#endif
