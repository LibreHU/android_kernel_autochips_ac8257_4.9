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


#include <linux/delay.h>
#include "atc_capture.h"
#include "avin_hal.h"
#include "avin_common.h"
#include "tvd_if.h"
#ifdef NR_MODULE_EXIST
#include "nr_if.h"
#endif
#include "di_hal.h"
#include "avin_log.h"

extern struct atc_avin_device *g_avin_dev;
extern struct mutex avin_lock;
extern bool avin_state;
extern uintptr_t di_add_arr[5];

static bool di_started;
static bool fd_setted;
SOURCE_INFO g_source_info;
#ifdef NR_MODULE_EXIST
static bool nr_started = false;
#endif
static struct timeval curr_time, previous_time;
static bool have_frame;

#define DI_DROP_FRAME_MAX 0
static int di_drop_frame_count = 0;


#ifdef NR_MODULE_EXIST
static void nr_frame_handle_sync(void * buffer_info)
{
	if (di_started) {
		di_buffer_receive_sync(buffer_info);
	} else {
		AVIN_BUFFER_T *buffer = (AVIN_BUFFER_T *)buffer_info;
		if (!IS_ERR_OR_NULL(buffer)) {
			atomic_set(&buffer->status, AVIN_BUF_FREE);
		}
		vss_printk(VSS_LOG_WARNING, "the di have not started, so we discard nr frame\n");
	}
}

#endif
static void di_frame_handle(void* _buffer_info)
{
	/*we will send buffer to buffer list in order to the user space dequeue*/
	struct atc_avin_device *avin_dev = g_avin_dev;
	struct avin_cap_buffer *tmp = NULL;
	struct capture_priv *cap_info;
	unsigned int total_time;

	AVIN_BUFFER_T *buffer_info = (AVIN_BUFFER_T *)_buffer_info;

	if (IS_ERR_OR_NULL(buffer_info)) {
		do_gettimeofday(&curr_time);
		previous_time = curr_time;
		have_frame = false;
	} else {
		if (vss_aee_debug) {
			do_gettimeofday(&curr_time);
			total_time = (((curr_time.tv_sec * 1000000 + curr_time.tv_usec) - (previous_time.tv_sec * 1000000 + previous_time.tv_usec)) / 1000);
			if (have_frame && (total_time > 20)) {
				vss_aee_print("time interval greater than 20ms and total_time is %u", total_time);
			}
			vss_printk(VSS_LOG_DEBUG, "time interval is %u\n", total_time);
			previous_time = curr_time;
			have_frame = true;
		}
		if (di_drop_frame_count < DI_DROP_FRAME_MAX) {
			di_drop_frame_count ++;
			di_set_output_buf_status(buffer_info->idex, AVIN_BUF_FREE);
			vss_printk(VSS_LOG_WARNING, "DI drop frame %d. total %d\n", di_drop_frame_count, DI_DROP_FRAME_MAX);
			return;
		}
	}

	spin_lock(&avin_dev->slock);
	if (!list_empty(&avin_dev->buf_queue)) {
		tmp = list_entry(avin_dev->buf_queue.next, struct avin_cap_buffer, list);
		avin_dev->cur_frm = tmp;
		list_del(&tmp->list);
	}

	if (tmp) {
		cap_info = (struct capture_priv *)vb2_plane_vaddr(&tmp->buf.vb2_buf, 0U);
		if (!IS_ERR_OR_NULL(cap_info)) {
			if (IS_ERR_OR_NULL(buffer_info)) {
				/*in order to notify signal status*/
				cap_info->source_info.source_width = g_source_info.source_width;
				cap_info->source_info.source_height = g_source_info.source_height;
				cap_info->source_info.signal_state = g_source_info.signal_state;
				cap_info->buf_index = -1;
				cap_info->buf_addr = 0;
				vss_printk(VSS_LOG_INFO, "the width is %d, the height is %d, signal status is %d\n",
					cap_info->source_info.source_width, cap_info->source_info.source_height, cap_info->source_info.signal_state);
			} else {
				cap_info->source_info.source_width = 0;
				cap_info->source_info.source_height = 0;
				cap_info->source_info.signal_state = SIGNAL_NONE;
				cap_info->buf_addr = buffer_info->y_addr;
				cap_info->buf_index = buffer_info->idex;
				di_add_arr[buffer_info->idex] = buffer_info->y_addr;
			}
			vb2_buffer_done(&tmp->buf.vb2_buf, VB2_BUF_STATE_DONE);
		} else {
			vss_printk(VSS_LOG_WARNING, "the private data pointer is null, this may be not correct\n");
			vss_aee_print("the vb2 plane is null");
		}
	} else {
		vss_printk(VSS_LOG_WARNING, "current frame pointer is null, emty status %d\n", list_empty(&avin_dev->buf_queue));
		if (!IS_ERR_OR_NULL(buffer_info)) {
			di_set_output_buf_status(buffer_info->idex, AVIN_BUF_FREE);
		}
		if (avin_dev->cur_frm && ((g_source_info.signal_state == SIGNAL_LOST) || (g_source_info.signal_state == SIGNAL_STABLE))) {
			cap_info = (struct capture_priv *)vb2_plane_vaddr(&avin_dev->cur_frm->buf.vb2_buf, 0U);
			cap_info->source_info.source_width = g_source_info.source_width;
			cap_info->source_info.source_height = g_source_info.source_height;
			cap_info->source_info.signal_state = g_source_info.signal_state;
			vss_printk(VSS_LOG_WARNING, "must notify signal message, even though the buffer is full\n");
			vss_aee_print("notify signal message when queue is full");
		}
	}
	spin_unlock(&avin_dev->slock);
}

#ifdef NR_MODULE_EXIST

static int avin_nr_init(unsigned int width ,unsigned int height)
{
	NR_PRM_T nr_param = {0,};

	vss_printk(VSS_LOG_INFO, "enter\n");

	if (nr_started) {
		vss_printk(VSS_LOG_WARNING, "leave, because the nr have started\n");
		return 0;
	}

	nr_hw_Init();

	nr_param.fgBypassEn = 0;
	nr_param.fgBurstRdEn= 0;
	nr_param.u1AddrSwapMode = 4;
	nr_param.u1FrameMode = 0;
	nr_param.u1DemoMode = 0;
	nr_param.u4PicWidth = width;
	nr_param.u4PicHeight= height;
	nr_param.u4Strength = 1;
	nr_param.u4FNRStrength = 3;
	nr_param.u4MNRStrength = 3;
	nr_param.u4BNRStrength = 3;
	nr_param.fgRangeRemapYEn = 0;
	nr_param.fgRangeRemapUVEn = 0;
	nr_param.u4RangeMapY = 0;
	nr_param.u4RangeMapUV = 0;
	nr_param.fgNoiseMeterEn = 0;
	nr_param.fgUseBlockMeter = 0;
	nr_param.GetNrBufInfo = nr_frame_handle_sync;
	nr_hw_config(&nr_param);
	nr_hw_trigger();

	nr_started = true;
	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;
}

int avin_nr_deinit(void)
{
	vss_printk(VSS_LOG_INFO, "enter\n");

	if (!nr_started) {
		vss_printk(VSS_LOG_WARNING, "leave, because the di have not started\n");
		return 0;
	}

	nr_started = false;
	nr_hw_deInit();
	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;
}
#endif

int avin_di_init(unsigned int width ,unsigned int height)
{
	DI_PRM_T di_param = {0,};
	int ret = -1;

	vss_printk(VSS_LOG_INFO, "enter\n");
	if (di_started) {
		vss_printk(VSS_LOG_WARNING, "leave, because the di have started\n");
		return 0;
	}

	di_param.src_width  = width;
	di_param.src_height = height;

	di_param.dst_width  = width;
	di_param.dst_height = height;

	di_param.duration = 0; //duration;

	di_param.src_rect.left = 0;
	di_param.src_rect.top = 0;
	di_param.src_rect.right = width;
	di_param.src_rect.bottom = height;

	di_param.dst_rect.left = 0;
	di_param.dst_rect.top = 0;
	di_param.dst_rect.right = width;
	di_param.dst_rect.bottom = height;

	di_param.getdibuffer = di_frame_handle;
	ret = di_hw_init();
	if(ret) {
		vss_printk(VSS_LOG_ERR, "init di failed\n");
		return -1;
	}

	ret = di_hw_config(&di_param);
	if(ret) {
		vss_printk(VSS_LOG_ERR, "config di failed\n");
		goto err;
	}

	ret = di_hw_start();
	if(ret){
		vss_printk(VSS_LOG_ERR, "start di failed\n");
		goto err;
	}

	di_started = true;
	fd_setted = false;
	di_drop_frame_count = 0;
	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;

err:
	di_hw_deinit();
	return -1;
}

int avin_di_deinit(void)
{
	int ret = -1;

	vss_printk(VSS_LOG_INFO, "enter\n");

	if (!di_started) {
		vss_printk(VSS_LOG_WARNING, "leave, because the di have not started\n");
		return 0;
	}

	di_started = false;
	fd_setted = false;

	ret = di_hw_stop();
	if(ret){
		vss_printk(VSS_LOG_ERR, "stop di failed\n");
		goto err;
	}

	ret = di_hw_deinit();
	if(ret){
		vss_printk(VSS_LOG_ERR, "deinit di failed\n");
		return -1;
	}

	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;
err:
	di_hw_deinit();
	return -1;
}

int avin_di_set_fd(int *pfd, int fd_num)
{
	vss_printk(VSS_LOG_INFO, "enter\n");

	if (!di_started) {
		vss_printk(VSS_LOG_ERR, "leave, because the di have not started\n");
		return -1;
	}

	if (fd_setted) {
		vss_printk(VSS_LOG_INFO, "update fd to di module, we need deinit di in order to free ion buffer\n");
		avin_di_deinit();
		avin_di_init(g_source_info.source_width, g_source_info.source_height);
		//avin_aee_print("update fd dynamicly");
	}

	di_buffer_output_receive(pfd, fd_num);
	fd_setted = true;

	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;
}

static void tvd_frame_handle_sync(void * buffer_info)
{
	tvd_hw_errorhandle();
#ifdef NR_MODULE_EXIST
	if (nr_started) {
		nr_buffer_receive_sync(buffer_info);
	} else {
		AVIN_BUFFER_T *buffer = (AVIN_BUFFER_T *)buffer_info;
		if (!IS_ERR_OR_NULL(buffer)) {
			atomic_set(&buffer->status, AVIN_BUF_FREE);
		}
		vss_printk(VSS_LOG_WARNING, "the nr have not started, so we discard wch_vdi frame\n");
	}
#else
	if (di_started) {
		di_buffer_receive_sync(buffer_info);
	}
	else {
		AVIN_BUFFER_T *buffer = (AVIN_BUFFER_T *)buffer_info;
		if (!IS_ERR_OR_NULL(buffer)) {
			atomic_set(&buffer->status, AVIN_BUF_FREE);
		}
		vss_printk(VSS_LOG_WARNING, "the di have not started, so we discard wch_vdi frame\n");
	}
#endif
}

static void tvd_signal_handle(const SOURCE_INFO *source_info)
{

	vss_printk(VSS_LOG_INFO, "enter\n");

	if(NULL == source_info) {
		vss_printk(VSS_LOG_ERR,"the input param is invalid\n");
		return;
	}

	//XXX Prevent Deadlock when avin_stop_streaming
	while (!mutex_trylock(&avin_lock)) {
		if (true != avin_state) {
			vss_printk(VSS_LOG_WARNING, "trylock fail. avin stream stoped, do nothing");
			return;
		}
		msleep_interruptible(1);
	}

	if (true != avin_state) {
		vss_printk(VSS_LOG_WARNING, "avin stream stoped, do nothing");
		mutex_unlock(&avin_lock);
		return;
	}

	switch(source_info->signal_state) {
		case SIGNAL_LOST:
		case SIGNAL_MODE_CHANGE_START:
			#ifdef NR_MODULE_EXIST
			avin_nr_deinit();
			#endif
			avin_di_deinit();
			break;
		case SIGNAL_STABLE:
		case SIGNAL_MODE_CHANGE_DONE:
			#ifdef NR_MODULE_EXIST
			avin_nr_init(source_info->source_width, source_info->source_height);
			#endif
			avin_di_init(source_info->source_width, source_info->source_height);
			break;
		default:
			break;
	}
	memcpy(&g_source_info, source_info, sizeof(*source_info));
	di_frame_handle(NULL);
	g_source_info.signal_state = SIGNAL_NONE;

	mutex_unlock(&avin_lock);
	vss_printk(VSS_LOG_INFO, "leave\n");
}

static void send_simulated_lost(void)
{
	vss_printk(VSS_LOG_INFO, "enter\n");
	g_source_info.signal_state = SIGNAL_LOST;
	g_source_info.source_width = 0;
	g_source_info.source_height = 0;
	di_frame_handle(NULL);
	vss_printk(VSS_LOG_INFO, "leave\n");
}

int avin_tvd_init(void)
{
	/*send simulated signal lost in order to cover the bug of suspend/resume which set fd before signal ready*/
	send_simulated_lost();
	tvd_callback_register((TVD_BUF_INFO_FUNC)tvd_frame_handle_sync, tvd_signal_handle);
	tvd_hw_init();
	return 0;
}

int avin_tvd_start(void)
{
	vss_printk(VSS_LOG_INFO, "enter\n");
	tvd_hw_start();
	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;
}

int avin_tvd_stop(void)
{
	vss_printk(VSS_LOG_INFO, "enter\n");
	tvd_hw_stop();
	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;
}

int avin_tvd_deinit(void)
{
	vss_printk(VSS_LOG_INFO, "enter\n");
	tvd_callback_unregister();
	tvd_hw_deinit();
	memset(&g_source_info, 0, sizeof(g_source_info));
	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;
}
