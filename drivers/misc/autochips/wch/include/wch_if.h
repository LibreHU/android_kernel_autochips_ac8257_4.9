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
/*
 *Department: ATC-SD-SD1
 *
 *Aurthor: Mingxu Wang (atc0027)
 *
 *Date: 2017-7-26
 */

#ifndef _WCH_IF_H_
#define _WCH_IF_H_
#ifdef ATC_ARM2_LK_SUPPORT
#include "platform/mt_typedefs.h"
#endif
typedef enum  {
	WCH_VDI = 0,/*tvd*/
	WCH_DI,		/*DI*/
	WCH_MAX
} WCH_ID_E;

typedef enum  {
	DATA_MODE_SCANLINE,
	DATA_MODE_BLOCK,
	DATA_MODE_MAX
} WCH_DATA_MODE_E;

typedef enum  {
	INDATA_TYPE_P,			/*Progressive input data*/
	INDATA_TYPE_I,			/*Interlace input data*/
	INDATA_TYPE_MAX
} WCH_INDATA_TYPE_E;

typedef enum  {
	OUTDATA_TYPE_FRAME,			/*WCH output frame to buffer*/
	OUTDATA_TYPE_FILED,			/*WCH output filed to buffer*/
	OUTDATA_TYPE_MAX
} WCH_OUTDATA_TYPE_E;


typedef enum  {
	DATA_IN_FMT_YUV422,
	DATA_IN_FMT_YUV444,
	DATA_OUT_FMT_YUV422_1PLANE,
	DATA_OUT_FMT_YUV422_2PLANES,
	DATA_OUT_FMT_YUV420_2PLANES,
	DATA_FMT_MAX
} WCH_DATA_FMT_E;


typedef enum  {
	ONE_PLANE_MODE_YUYV = 0,
	ONE_PLANE_MODE_YVYU,
	ONE_PLANE_MODE_UYVY,
	ONE_PLANE_MODE_VYUY,
	ONE_PLANE_MODE_MAX
} WCH_ONE_PLANE_MODE_E;


/*
*  Description: Pointer for a callback, WCH will call it to give the filled buffer address to the WCH User.
*
*  Param:   @ yAddr: Y buffer address
*               @ cAddr: C buffer address
*
*  Return:  None
*/
typedef void (*get_wch_buffer_info)(void * buffer_info);

typedef struct {
	WCH_ID_E 		wch_id;			/*WCH id*/

	WCH_DATA_FMT_E 	input_fmt;		/* specify the src video fmt, 444 or 422*/
	u32 	src_width;      /* specify the src video Width of one filed */
	u32 	src_hight;     	/* specify the src video height of one filed */
	u32 	src_start_x;      		/* specify the start pixel position of active video in horizontal direction */
	u32 	src_start_y_top;  		/* specify the start position of active video in top field */
	u32 	src_start_y_bot;  		/* specify the start position of active video in bottom field */
	u32		h_total;				/* for wch_di only, specify the h total, DI and WCH_DI should use the same value*/
	WCH_INDATA_TYPE_E 		input_data_type;	/*specify the src video is progressive or interlace*/

	WCH_DATA_FMT_E 			output_fmt;		/*specify the dst video format, 422 1 plane/2 planes or 420 2 plans*/
	WCH_ONE_PLANE_MODE_E	plane_mode;		/*for 1plane, UYVY, VYUY, YUYV, YVYU*/
	u32 			dst_width;      /* specify the dst video Width of one filed */
	u32 			dst_stride;     /* specify the dst video stride of one filed */
	u32 			dst_height;     /* specify the dst video height of one filed */
	WCH_DATA_MODE_E 		data_mode;	 	/* specify the dst video mode, scanline or block mode*/
	WCH_OUTDATA_TYPE_E		output_data_type;	/*specify the dst video is frame or filed, if input is progressive, don't need to set this*/

	get_wch_buffer_info 	get_wch_buf_info;
	unsigned char 			uv_swap;        	/* Cr buffer connet with C need set 1, only support 444 input*/
} WCH_CFG_T, *PWCH_CFG_T;

enum {
	WCH_SUCCESS = 0,
	WCH_CONTEXT_NULL = 0xF0000000,
	WCH_FAIL_UNKNOWN,				/*unknown error*/
	WCH_INVALID_INPUT_PARAM,		/*the input paramater invalid*/
	WCH_INVALID_OUTPUT_PARAM,		/*the output paramater invalid*/
	WCH_WRONG_ID,					/*error WCH ID error*/
	WCH_ALLOC_BUF_FAIL,				/*alloc buffer for WCH fail*/
	WCH_STATE_ERROR,				/*call the function in a wrong state*/
	WCH_ERROR_MAX
};

/*
*  Description: Init a specified WCH, than you must config it
*
*  Param:   @ wch_id: WCH_VDI for tvd source,  WCH_DI for DI source
*
*  Return: @ WCH_SUCCESS OK
*             @ other FAIL
*/
u32 wch_hw_init(WCH_ID_E wch_id);

/*
*  Description: Config some paramter to a specified WCH, and then the WCH can work correctly
*                    by calling wch_hw_start.
*
*  Param:  @ wch_cfg: Specify the paramter for WCH setting
*
*  Return: @ WCH_SUCCESS OK
*              @ other FAIL
*/
u32 wch_hw_config(PWCH_CFG_T wch_cfg);

/*
*  Description: Set a DI buffer to WCH_DI, before config WCH_DI, should call this to set buffers,
*
*  Param:   @ wch_id: WCH_DI for DI source
*               @ yc_addr[]: the buffer address of setting to WCH_DI
*               @ buf_cnt: the buffer count of setting to WCH_DI
*
*  Return: @ WCH_SUCCESS OK
*              @ other FAIL
*/
u32 wch_hw_set_di_buffer(WCH_ID_E wch_id, uintptr_t y_addr, uintptr_t c_addr);

/*
*  Description: Sart a specified WCH. After it be called, the WCH will start working. You can stop 
*                    it by calling wch_hw_stop
*
*  Param:   @ wch_id: WCH_VDI for tvd source,  WCH_DI for DI source
*
*  Return: @ WCH_SUCCESS OK
*              @ other FAIL
*/
u32 wch_hw_start(WCH_ID_E wch_id);

/*
*  Description: Stop a specified WCH. After it be called, the WCH will stop working. You can restart wch
*                    by called wch_hw_start
*
*  Param:   @ wch_id: WCH_VDI for tvd source,  WCH_DI for DI source
*
*  Return: @ WCH_SUCCESS OK
*              @ other FAIL
*/
u32 wch_hw_stop(WCH_ID_E wch_id);

/*
*  Description: Deinit a specified WCH. After calling this function, the WCH will be released,
*                    and you don't need WCH working any more.
*
*  Param:   @ wch_id: WCH_VDI for tvd source,  WCH_DI for DI source
*
*  Return: @ WCH_SUCCESS OK
*             @ other FAIL
*/
u32 wch_hw_deinit(WCH_ID_E wch_id);

/*
*  Description: Deinit a specified WCH. After calling this function, the WCH will be released,
*                   	wch_vdi_status will clear
*
*  Param:   @ wch_id: Only for  WCH_VDI 
*
*  Return: @ WCH_SUCCESS OK
*             @ other FAIL
*/
u32 wch_hw_clear(WCH_ID_E wch_id);
/*
*  Description:
*                   	wch_di_triger_cnt will ++
*
*  Param:   @ wch_id: Only for  WCH_DI
*
*  Return: @ WCH_SUCCESS OK
*             @ other FAIL
*/
u32 wch_set_di_triger_status(WCH_ID_E wch_id);



u32 get_wch_garbage_addr(void);


#endif

