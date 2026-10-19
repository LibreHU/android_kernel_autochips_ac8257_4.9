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
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/sched.h>/*wake_up_process*/
#include <linux/kthread.h>/*kthread_create*/
#include <linux/err.h>/*IS_ERR,PTR_ERR*/
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>/*mutex*/
#include "x_os.h"
#include "x_typedef.h"
#include "log.h"
#include <asm-generic/bug.h>
#include <asm/uaccess.h>
#include <ion.h>
#else
//#include "x_bim_83xx.h"
//#include "general_op.h"
#include "log.h"
#endif

#include "di_if.h"
#include "pmx_hal.h"
#include "bringup.h"
#include "wch_if.h"
extern DI_PRM_T PALPARAM[4];
#ifndef ATC_ARM2_LK_SUPPORT
extern void __iomem* IO_BASE_BRINGUP;
#define HANDLE_NUM (DI_BUF_MAX*VIDEO_CHANNEL_NUM*3)

static struct ion_handle *ion_handles[HANDLE_NUM] = {NULL,};
static int ion_handle_idx;

#else
extern unsigned long IO_BASE_BRINGUP;
#endif

DI_ADDRESS_FUNC di_address_func = NULL;
struct ion_client  *g_ion_client = NULL;
#ifndef ATC_ARM2_LK_SUPPORT
struct ion_handle  * di_ion_to_phyaddr(int fd, physical_block *phys_blocks)
{
	struct ion_handle  *ion_handle = NULL;
	struct sg_table * sgt = NULL;
	struct scatterlist * sg = NULL;
	int dma_block_num = 0;
	unsigned long i = 0;

	VDO_LOG(VDO_LOG_LVL_DBG, "di_ion_to_phyaddr enter and fd is %d\n", fd);
	if (IS_ERR_OR_NULL(g_ion_client)) {
		VDO_LOG(VDO_LOG_LVL_WARN, "the ion client is null, so create it now\n");
		g_ion_client = ion_client_create(g_ion_device, "atc_avin");
	}
	ion_handle = ion_import_dma_buf_fd(g_ion_client, fd);
	if (IS_ERR_OR_NULL(ion_handle)) {
		VDO_LOG(VDO_LOG_LVL_ERR,"the ion handle is null\n");
		//ion_client_destroy(g_ion_client);
		return NULL;
	}

	sgt = ion_sg_table(g_ion_client, ion_handle);
	if (IS_ERR_OR_NULL(sgt)) {
		VDO_LOG(VDO_LOG_LVL_ERR,"the scatter table is null\n");
		goto ion_dma_map_failed;
	}

	if (ion_handle_idx >= HANDLE_NUM) {
		VDO_LOG(VDO_LOG_LVL_DBG, "memory mapping out of table size, attentaion mem leak\n");
		ion_handle_idx = 0;
		//goto ion_dma_map_failed;
	}

	sg = sgt->sgl;
	while (sg) {
		dma_block_num++;
		sg = sg_next(sg);
	}

	for_each_sg(sgt->sgl, sg, dma_block_num, i) {
		phys_blocks->addr = sg_phys(sg);
		phys_blocks->size = sg_dma_len(sg);
		VDO_LOG(VDO_LOG_LVL_DBG, "addr is 0x%08x, size is %d, num is %d\n", phys_blocks->addr, phys_blocks->size, dma_block_num);
	}
	//ion_free(g_ion_client, ion_handle);
	//ion_client_destroy(g_ion_client);
	if (ion_handles[ion_handle_idx]) {
		ion_free(g_ion_client, ion_handles[ion_handle_idx]);
	}
	ion_handles[ion_handle_idx++] = ion_handle;
	VDO_LOG(VDO_LOG_LVL_DBG, "di_ion_to_phyaddr leave!\n");

	return ion_handle;
	/* failed*/
ion_dma_map_failed:
	ion_free(g_ion_client, ion_handle);
	//ion_client_destroy(g_ion_client);
	return NULL;

}

void dev_ion_free(void)
{
	int i = 0;

	for (i = 0; i < HANDLE_NUM; i++) {
		if (ion_handles[i]) {
			ion_free(g_ion_client, ion_handles[i]);
			ion_handles[i] = NULL;
		}
	}
	ion_handle_idx = 0;

}
#endif
/*
	Description:Triger DI HW to work
*/
static int cnt = 0;
void di_hw_triger(atc_di_device DI_INTER_PARAM)
{
	VDO_LOG(VDO_LOG_LVL_INFO, " %s start \r\n",__func__);

	/*di_start_wch((DI_INTER_PARAM.param.rDstRect.right-DI_INTER_PARAM.param.rDstRect.left),
			(DI_INTER_PARAM.param.rDstRect.bottom-DI_INTER_PARAM.param.rDstRect.top));
	wch_hw_start(WCH_DI);
*/
	vPmxHalInit((DI_INTER_PARAM.param.rDstRect.right-DI_INTER_PARAM.param.rDstRect.left)
			,(DI_INTER_PARAM.param.rDstRect.bottom-DI_INTER_PARAM.param.rDstRect.top)
			,DI_INTER_PARAM.param.u4SrcWidth,DI_INTER_PARAM.param.u4SrcHeight);

	vPmxHalTrigerDIModule(DI_INTER_PARAM.param.u4Idx);
	cnt = 0;
}
extern void di_callback(di_buf_t *buf_info);

void di_get_addr_fun (void * buffer_info)
{
	if(buffer_info == NULL) {
		return ;
	}
	//di_signal_event();

	VDO_LOG(VDO_LOG_LVL_DBG, "enter \n");

	di_callback(buffer_info);

}

void di_register_get_addr_fun(DI_ADDRESS_FUNC di_get_addr_fun)
{
	VDO_LOG(VDO_LOG_LVL_INFO, " %s start\r\n",__func__);

	di_address_func=di_get_addr_fun;
}

void di_start_wch(u16 src_width, u16 src_height, u16 dst_width, u16 dst_height, u16 stride)
{
	WCH_CFG_T wch_cfg = {0};

	wch_cfg.wch_id = WCH_DI;
	wch_cfg.input_fmt = DATA_IN_FMT_YUV444;
	wch_cfg.src_width = src_width;
	wch_cfg.src_hight = src_height;
	wch_cfg.src_start_x = 0;
	wch_cfg.src_start_y_top = 0;
	wch_cfg.src_start_y_bot = 0;
	wch_cfg.input_data_type = INDATA_TYPE_P;

	wch_cfg.h_total=0xBB7;
	wch_cfg.output_fmt = DATA_OUT_FMT_YUV422_1PLANE;//DATA_OUT_FMT_YUV420_2PLANES;
	wch_cfg.plane_mode = ONE_PLANE_MODE_YUYV;
	wch_cfg.data_mode = DATA_MODE_SCANLINE;
	wch_cfg.dst_width = dst_width;
	wch_cfg.dst_stride = stride;
	wch_cfg.dst_height = dst_height;
	wch_cfg.output_data_type = OUTDATA_TYPE_FRAME;
	wch_cfg.get_wch_buf_info = di_address_func;
	wch_cfg.uv_swap = 0;
#ifndef ATC_ARM2_LK_SUPPORT
	//vPmxSetWchOutBuf();
#endif
	wch_hw_config(&wch_cfg);
	VDO_LOG(VDO_LOG_LVL_DBG, " %s wch config end\r\n", __func__);
}



