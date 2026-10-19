/*
 *Department: ATC-SD-SD1
 *
 *Aurthor: Mingxu Wang(atc0027)
 *
 *Date: 2017-07-31
 */

#ifndef _WCH_HAL_H_
#define _WCH_HAL_H_

#include "wch_priv.h"
#include "wch_if.h"
#include "wch_reg.h"

#define WCH_OPEN_SHADOW		0
#define WCH_VERSION			"01.04.01"
typedef struct WCH_INPUT_INFO {
	WCH_DATA_FMT_E 		src_fmt;
	u32 		src_width;
	u32 		src_height;
	u32 		start_x;
	u32 		start_y_top;
	u32 		start_y_bot;
	u32			h_total;
	WCH_INDATA_TYPE_E 	input_data_type;
} WCH_INPUT_INFO_T;


typedef struct WCH_OUTPUT_INFO {
	WCH_DATA_FMT_E 			dst_fmt;
	WCH_DATA_MODE_E 		data_mode;
	WCH_ONE_PLANE_MODE_E	plane_mode;
	WCH_INDATA_TYPE_E 		input_data_type;
	u32 					dst_width;
	u32 					dst_stride;
	u32 					dst_height;
} WCH_OUTPUT_INFO_T;


typedef struct {
	WCH_ID_E 					wch_id;
	volatile WCH_REG_UNION_T 	*wch_hw_reg;
	volatile WCH_REG_UNION_T 	wch_sw_reg;
	u32 						regbit_clock;
	u32 						regbit_rst;
	u8 						is_get_hw_reg;
	u8 						is_wch_hal_inited;
}WCH_HAL_INFO_T;


void wch_img_status_clear(WCH_ID_E wch_id, uintptr_t base);
void wch_img_status_enable(WCH_ID_E wch_id, uintptr_t base);
void wch_hal_enable(WCH_ID_E wch_id);
void wch_hal_disable(WCH_ID_E wch_id);
void wch_hal_set_reg_base(WCH_ID_E wch_id, void *reg_base);
void wch_hal_set_clk_rst_base(WCH_ID_E wch_id, void *clk_rst_base);
u32 wch_hal_check_filed_stauts(WCH_ID_E wch_id);
void wch_hal_init(WCH_ID_E wch_id);
void wch_hal_deinit(WCH_ID_E wch_id);
void wch_hal_set_yc_addr(WCH_ID_E wch_id, u32 y_addr, u32 c_addr);
void wch_hal_set_input(WCH_ID_E wch_id, WCH_INPUT_INFO_T * input_info);
void wch_hal_set_output(WCH_ID_E wch_id, WCH_OUTPUT_INFO_T * output_info);
void wch_hal_start(WCH_ID_E wch_id);
void wch_hal_stop(WCH_ID_E wch_id);
void wch_hal_set_shadow(WCH_ID_E wch_id);

#endif

