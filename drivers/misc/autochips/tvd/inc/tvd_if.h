/*
 *Copyright (c) 2016 AutoChips Inc.
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License version 2 as
 *published by the Free Software Foundation.
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *tvd-drv-interface.h
 *support tvd driver interface which control tvd hardware
 */

#ifndef __TVD_IF_H__ 
#define __TVD_IF_H__ 

#if 1
#include "avin_common.h"
#else
typedef enum {
	SIGNAL_NONE,
	SIGNAL_STABLE,
	SIGNAL_LOST,
	SIGNAL_MODE_CHANGE_START,
	SIGNAL_MODE_CHANGE_DONE,
	SIGNAL_ERROR,
	SIGNAL_MAX,
} SIGNAL_STATE_E;

typedef struct {
	SIGNAL_STATE_E  signal_state;
	u16 source_width;
	u16 source_height;
} SOURCE_INFO;
#endif

/*
 *Description:TVD module retrun diferent value
 */
enum
{
    TVD_SUCCESS = 0,
    TVD_FAILED,
    TVD_INVALID_INPUT_PARAM
};

#if 0
/**************************************************************************
 *@brief TVD Color Process Convsion
 *Color process formula:
 *Y = Y * y_gain + y_offset
 *U = (U-128) * u_cosgain + (V -128) * u_singain + u_offset + 128
 *V = (V - 128) * v_cosgain - (U - 128)* v_singain + v_offset + 128
***************************************************************************/

typedef struct {
    u16            u_cosgain;
    u16            u_singain;

    u16            v_cosgain;
    u16            v_singain;

    u16            y_gain;

    u8             y_offset;
    u8             u_offset;
    u8             v_offset;
} TVD_COLOR_PROCESS_PARAMETER_T, *PTVD_COLOR_PROCESS_PARAMETER_T;
#endif

typedef void* TVD_BUF_INFO_FUNC;
typedef void(*TVD_SOURCE_INFO_FUNC) (const SOURCE_INFO* source_info);


/*
 *note   : the call order is tvd_hw_init->tvd_callback_register->tvd_hw_config(this is option)->tvd_hw_start->
 *tvd_hw_stop->tvd_callback_unregister->tvd_hw_deinit
 */


/*
 *param1  : it is y/c address callback function
 *param2  : it is source info callback function
 *return : null
 *note   : register callback function which is used to set y/c address to NR or notify source info to backcar sdk
 */
void tvd_callback_register(TVD_BUF_INFO_FUNC buf_func, TVD_SOURCE_INFO_FUNC source_func);



/*
 *param  : null
 *return : null
 *note   : unregister callback function
 */
void tvd_callback_unregister(void);


/*
 *param  : null
 *return : when operation succeeds, it returns TVD_SUCCESS, others return TVD_FAILED
 *note   : This function open analog & clamp power and initial tvd common register
 */
int tvd_hw_init(void);


/*
 *param  : null
 *return : when operation succeeds, it returns TVD_SUCCESS, others return -TVD_FAILED
 *note   : This function reset tvd register and close analog & clamp power
 */
int tvd_hw_deinit(void);


/*
 *param  : null
 *return : when operation succeeds, it returns TVD_SUCCESS, others return -TVD_FAILED
 *note   : This function enable tvd interrupt
 */
int tvd_hw_start(void);


/*
 *param  : null
 *return : when operation succeeds, it returns TVD_SUCCESS, others  return -TVD_FAILED
 *note   : This function disable tvd interrupt
 */
int tvd_hw_stop(void);

#if 0
/*
 *param  : null
 *return : when operation succeeds, it return TVD_SUCCESS,  it returns -TVD_INVALID_INPUT_PARAM when input parameter is error,
 *         others return -TVD_FAILED
 *note   : This function config tvd color process parameter, if you use default config, there is no need to call this interface
 */

int tvd_hw_config(TVD_COLOR_PROCESS_PARAMETER_T args);
#endif

/*
 * color_type: TVD_COLOR_TYPE_*
 */
int tvd_hw_color_process(int color_type, int value);


/*
 * tvd hardware error hanle
 * call it from tvd_frame_callback
 */
void tvd_hw_errorhandle(void);

#endif /* __TVD_IF_H__ */



