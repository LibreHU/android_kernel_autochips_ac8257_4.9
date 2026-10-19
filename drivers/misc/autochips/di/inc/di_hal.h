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

#ifndef _DI_HAL_H_
#define _DI_HAL_H_

#ifndef ATC_ARM2_LK_SUPPORT
#include "x_os.h"
#include "di_drv.h"
#include "linux/irqreturn.h"
#else
#include "platform/mt_typedefs.h"
#include "sys/types.h"
#include "di_types.h"
#include <kernel/mutex.h>
#include "di_drv.h"

bool initalDIModule(void);
typedef void (*get_di_buffer)(void *buffer_info);
typedef volatile int atomic_t;

#endif

struct di_work_struct {
#ifndef ATC_ARM2_LK_SUPPORT
	struct work_struct work_store_buf;
#endif
	int work_buf_inY[VIDEO_CHANNEL_NUM][BUFFER_NUM];
	int work_buf_inC[VIDEO_CHANNEL_NUM][BUFFER_NUM];
	int work_buf_out[VIDEO_CHANNEL_NUM][BUFFER_NUM];
#ifndef ATC_ARM2_LK_SUPPORT
	struct sync_timeline *timeline_in;
	struct sync_timeline *timeline_out;
#else
	atomic_t status;
#endif
	bool top_filed_fst[VIDEO_CHANNEL_NUM];//0 ,1
	int type;//AVM CVBS
	int in_idx;
};


enum  {
	AVM,
	CVBS,
	DEV_MAX
};

enum {
	DI_FMT_YUV420,
	DI_FMT_YUV422,
};


#ifndef BUFFER_NUM
#define BUFFER_NUM 3
#endif
#ifndef VIDEO_CHANNEL_NUM
#define VIDEO_CHANNEL_NUM      (4)
#endif

#define DI_INTERLACE_SOURCE_EN  (0x1)
#define DI_TZG_OUTPUT_EN        (0x1)

#define DI_SUCCESS           0
#define DI_DUMP_BUFFER_ATTR (1)
#define DI_BUF_MAX_CNT      (5)
#define DI_IO_NUM           (2)
#define DI_IN_BUF           (0)
#define DI_OUT_BUF          (1)

#define DI_IN_BUF_YSIZE      (720*576)
#define DI_IN_BUF_CSIZE      (720*576>>1)
#define DI_OUT_BUF_SIZE      (720*576*2)
#define DI_DUMP_REG_SIZE     ((0x100>>2) * sizeof(unsigned int))

#define VDPSYS_REG_SIZE             	(0x134)

#define VDPSYS_INT_STATUS    		(0x0)
#define VDPSYS_INT_CLR       		(0x004)
#define VDPSYS_DI_CONFIG     		(0x008)
#define VDPSYS_CLK_CONFIG0  		(0x014)
#define VDPSYS_CLK_CONFIG1  		(0x018)
#define VDPSYS_VDO_INT_CLR   		(0x030)
#define VDPSYS_DISPFMT_INT_CLR  	(0x034)
#define VDPSYS_WR_CHN_DI_INT_CLR    	(0x038)
#define VDPSYS_CG_CON0              	(0x100)
#define VDPSYS_SW_RST_B             	(0x130)

typedef struct _DI_PRM_T_
{
	unsigned int src_width;		/**< Src Buffer Width*/
	unsigned int src_height;	/**< Src Buffer Height, 16 Alignment src width*/
	unsigned int dst_width;		/**< Dst Buffer Width, 32 Alignment src height*/
	unsigned int dst_height;	/**< Dst Buffer Height*/
	unsigned int duration;		/**< Duration for a frame to show, no used*/

	RECT   src_rect;		/**< Src Rect*/
	RECT   dst_rect;		/**< Dst Show Area Rect*/

	get_di_buffer getdibuffer;	/**< save avin callback*/
} DI_PRM_T;


#if DI_DUMP_BUFFER_ATTR
typedef struct _DI_BUF_ADDR_
{
	unsigned int di_in_buf_cnt;
	unsigned int di_out_buf_cnt;
	unsigned long u4YAddr[DI_IO_NUM][DI_BUF_MAX_CNT];//out yc_addr,in y_addr
	unsigned long u4CAddr[DI_IO_NUM][DI_BUF_MAX_CNT];//out yc_size,in c_addr
} DI_BUF_ADDR;
#endif

typedef struct {
	uintptr_t y_addr;		/**< intput buffer y address*/
	uintptr_t c_addr;		/**< intput buffer c address*/
	unsigned int idex;		/**< intput buffer index in input buffer list*/
	atomic_t status;		/**< intput buffer status*/
	unsigned int vsync_cnt;		/**< the remained number of intput buffer usage*/
} di_buf_t;

typedef struct {
	/**same with di_params struct**/
	bool enable_hw;			/**< 1:enable 0:disable*/
	unsigned int src_w;		/**< Src Buffer Width, 16 Alignment src width*/
	unsigned int src_h;		/**< Src Buffer Height, 32 Alignment src height*/
	unsigned int dst_w;		/**< Dst Buffer Width*/
	unsigned int dst_h;		/**< Dst Buffer Height*/
	RECT   src_rect;		/**< Src Rect*/
	RECT   dst_rect;		/**< Dst Show Area Rect*/
	unsigned int scan_line_mode;	/**< 1 means scan line mode, 0 means block mode*/
	/*****************************/

	/*internal params*/
	unsigned int flags;		/**< updateoverlay*/
	unsigned int deinterlace_mode;	/**< only use VDP_DI_MA4F_MODE currrently*/
	bool   first_field;		/**< 1:first field 0:not first field, which will be used when avin call di*/
	bool   need_resize;		/**< not use*/
	bool   top_field_first;		/**< 1:top first 0:bottom first*/
	struct mutex lock;
	get_di_buffer getdibuffer;	/**< save avin callback*/
	unsigned int stride;
} di_internal_params;

extern di_internal_params di_param[DEV_MAX];
//extern void msleep(unsigned int msecs);


/** @brief deinit timer
*   @param void
*   @return void
*   @note None
*/
void di_trigger_timer_deinit(void);

/** @brief init timer
*   @param void
*   @return void
*   @note AVIN create 16.6ms or 20ms timer.AVM create 8.8ms timer
*/
void di_trigger_timer_init(void);

/** @brief init work queue and clear di_param
*   @param void
*   @return void
*   @note None
*/
void di_init_params(void);

/** @brief signal DI work done
*   @param void
*   @return void
*   @note None
*/
void di_signal_trigger_event(void);

/** @brief wait DI work done
*   @param void
*   @return 0 is timeout, positive is success, negative is error
*   @note None
*/
int di_wait_trigger_event(void);

/** @brief DI params init
*   @param dev : AVM or CVBS
*   @param param : src & dst width and height and rect,
*   @return 0 is success,other fail.See Kernel error type for detail.
*   @note open or clease DI must call this function first to init/deinit basic params
*/
int di_hal_set_params(int dev, di_params *param);

/** @brief init wch and set vdp mode
*   @param dev : AVM or CVBS
*   @return 0 is success,other fail.See Kernel error type for detail.
*   @note None
*/
int di_hal_init(int dev);

/** @brief deinit wch and close vdp
*   @param dev : AVM or CVBS
*   @return 0 is success,other fail.See Kernel error type for detail.
*   @note None
*/
int di_hal_deinit(int dev);

/** @brief set out buffer to wch and trigger DI work
*   @param dev : AVM or CVBS
*   @param yin_addr : y input address array
*   @param cin_addr : c input address array
*   @param ycout_addr : out buffer which will set to wch
*   @param top_field_fst : 1 top field first, 0 bottom field first
*   @return 0 is success,other fail.See Kernel error type for detail.
*   @note None
*/
int di_work(int dev, uintptr_t *yin_addr, uintptr_t *cin_addr, uintptr_t ycout_addr, bool top_field_fst);


/** @brief fmt hardware irq handler
*   @param void
*   @return void
*   @note set buffer done and clear irq status
*/
void dispfmt_irq_handler(void);
#ifndef ATC_ARM2_LK_SUPPORT

/** @brief register dispfmt irq handler
*   @param vector : irq id
*   @param dev_id : device id which was not used
*   @return void
*   @note dispfmt irq handler
*/
irqreturn_t dispfmt_vsync_isr(int vector, void *dev_id);

/** @brief Implementaiton of queue worker, do DI process for AVM and AVIN
*   @param work : Pass the address and params needed to trigger DI to worker queue
*   @return void
*   @note None
*/
void di_work_func(struct work_struct *work);
#endif

/************Interface for AVIN start****************/
/** @brief No internal implementation
*   @param void
*   @return 0 is success,other fail.See Kernel error type for detail.
*   @note None
*/
unsigned int  di_hw_init(void);

/** @brief Configure the width and height of input and output video and set avin callback.
*   @param param : the width and height of input and output video and callback of avin
*   @return 0 is sucess,other fail.See Kernel error type for detail.
*   @note  Must be called by user if want to start DI.
*/
int di_hw_config(DI_PRM_T *param);

/** @brief No internal implementation
*   @param void
*   @return 0 is success,other fail.See Kernel error type for detail.
*   @note None
*/
unsigned int di_hw_start(void);

/** @brief stop DI work.
*   @param void
*   @return 0 is success, other fail.See Kernel error type for detail.
*   @note  Must be called by user after di_hw_init.
*/
unsigned int di_hw_stop(void);

/** @brief No internal implementation
*   @param void
*   @return 0 is success,other fail.See Kernel error type for detail.
*   @note None
*/
unsigned int di_hw_deinit(void);

/** @brief Set the input buffer to DI
*   @param inputBuffer : pointer to intput buffer which include y/c address and state of input
*   @return 0 is success,other fail.See Kernel error type for detail.
*   @note None
*/
unsigned int di_buffer_receive_sync(void *inputBuffer);

/** @brief Set the address of DI output
*   @param pfd : pointer to  fd arrary
*   @param fd_num : fd number of input
*   @return 0 is success,other fail.See Kernel error type for detail.
*   @note The address of hardware access is obtained by fd
*/
int di_buffer_output_receive(unsigned int *pfd, unsigned int fd_num);

/** @brief Set buffer status of DI output after avin run out
*   @param buf_idex : the index of buffer
*   @param buf_status : the status(AVIN_BUF_FREE or AVIN_BUF_BUSY) of buffer will be set
*   @return 0 is success,other fail.See Kernel error type for detail
*   @note None
*/
int di_set_output_buf_status(u32 buf_idex, u32 buf_status);

/** @brief Set buffer address of DI output for ARM2
*   @param void
*   @return void
*   @note None
*/
void set_di_outbuf(void);
/************Interface for AVIN end****************/

/** @brief Get DI status
*   @param dev  device id
*   @return true is cur device, 0 is not cur device
*   @note None
*/

int di_is_cur_dev(int dev);


/*test functions*/
void di_read_file(uintptr_t file_in_Y, uintptr_t file_in_C, int i, bool is_virt_addr);




#if DI_DUMP_BUFFER_ATTR
unsigned int di_dump_buffer(unsigned int di_io, unsigned int buf_idx);
void di_dump_template(const unsigned long virt_addr, const unsigned int size, const unsigned char diDumpFile[]);
void di_dump_register(void);
void di_save_yc_addr(unsigned int di_io, unsigned int di_buf_idx, unsigned long y_addr, unsigned long c_addr);
#endif

#endif  /* _DI_HAL_H_*/
