
/*
 *Department: ATC-SD-SD1
 *
 *Aurthor: Mingxu Wang (atc0027)
 *
 *Date: 2017-7-26
 */
#ifdef ATC_ARM2_LK_SUPPORT
#include "string.h"
#include <platform/mt_reg_base.h>
#else
#include <linux/string.h>
#endif
#include "wch_priv.h"
#include "wch_hal.h"
#include "wch_reg.h"
#include "wch_log.h"

#define WCH_TVD_N_HTOTAL	0x338
#define WCH_TVD_P_HTOTAL	0x359
#define WCH_SHOW_PATTERN 	0


static uintptr_t _clk_rst_base[WCH_NUM] = {
#ifdef ATC_ARM2_LK_SUPPORT
	0x02800000 + ARM9_IO_BASE,
	0x02000000 + ARM9_IO_BASE
#endif
};/*init for arm2*/
static uintptr_t _wch_reg[WCH_NUM] = {
#ifdef ATC_ARM2_LK_SUPPORT
	0x02802000 + ARM9_IO_BASE,
	0x02005000 + ARM9_IO_BASE
#endif
}; /*init for arm2*/

static WCH_HAL_INFO_T _wch_hal_info[WCH_NUM] = {
	{.is_wch_hal_inited = false},
	{.is_wch_hal_inited = false}
};

static const char *_wch_data_mode[DATA_MODE_MAX] = {
	"DATA_MODE_SCANLINE",
	"DATA_MODE_BLOCK"
};

static const char *_wch_indata_type[INDATA_TYPE_MAX] = {
	"INDATA_TYPE_P",
	"INDATA_TYPE_I"
};

static const char *_wch_data_fmt[DATA_FMT_MAX] = {
	"DATA_IN_FMT_YUV422",
	"DATA_IN_FMT_YUV444",
	"DATA_OUT_FMT_YUV422_1PLANE",
	"DATA_OUT_FMT_YUV422_2PLANES",
	"DATA_OUT_FMT_YUV420_2PLANES"
};

static u32 _wch_int_clr_offset[WCH_NUM] = {
	0x3C,
	0x04
};

static u32 _wch_int_clr_bit[WCH_NUM] = {
	0,
	3
};


static void wch_hal_set_info(WCH_ID_E wch_id)
{
	u32 regbit_clock[WCH_NUM] = {0x00000008, 0x00001000};
	u32 regbit_rst[WCH_NUM] = {0x00000002, 0x00000004};

	memset(&_wch_hal_info[wch_id], 0, sizeof(_wch_hal_info[wch_id]));
	_wch_hal_info[wch_id].wch_id = wch_id;
	_wch_hal_info[wch_id].is_get_hw_reg = false;
	_wch_hal_info[wch_id].regbit_clock = regbit_clock[wch_id];
	_wch_hal_info[wch_id].regbit_rst = regbit_rst[wch_id];
	_wch_hal_info[wch_id].wch_hw_reg = (WCH_REG_UNION_T *)(_wch_reg[wch_id]);
	WCH_LOG_INFO(wch_id,"wch_hal_set_info wch_hw_reg: %p\n",
		 _wch_hal_info[wch_id].wch_hw_reg);
}

static void wch_clk_enable(WCH_ID_E wch_id)
{
	u32 reg_clock = 0;
	u32 reg_reset = 0;
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];

	reg_clock = WCH_HAL_READ32(_clk_rst_base[wch_id] + OFFSET_CLOCK_CTL);
	reg_reset = WCH_HAL_READ32(_clk_rst_base[wch_id] + OFFSET_RESET_CTL);

	WCH_HAL_WRITE32(_clk_rst_base[wch_id] + OFFSET_CLOCK_CTL, reg_clock & ~ info->regbit_clock);
	WCH_HAL_WRITE32(_clk_rst_base[wch_id] + OFFSET_RESET_CTL, reg_reset & ~ (info->regbit_rst));
	WCH_HAL_WRITE32(_clk_rst_base[wch_id] + OFFSET_RESET_CTL, reg_reset | info->regbit_rst);
}

static void wch_clk_disable(WCH_ID_E wch_id)
{
	u32 reg_clock = 0;
	u32 reg_reset = 0;
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];

	reg_clock = WCH_HAL_READ32(_clk_rst_base[wch_id] + OFFSET_CLOCK_CTL);
	reg_reset = WCH_HAL_READ32(_clk_rst_base[wch_id] + OFFSET_RESET_CTL);

	WCH_HAL_WRITE32(_clk_rst_base[wch_id] + OFFSET_RESET_CTL, reg_reset & ~ (info->regbit_rst));
	WCH_HAL_WRITE32(_clk_rst_base[wch_id] + OFFSET_RESET_CTL, reg_reset | info->regbit_rst); //reset
	WCH_HAL_WRITE32(_clk_rst_base[wch_id] + OFFSET_CLOCK_CTL, reg_clock | info->regbit_clock); //disable clock
}
void wch_hal_enable(WCH_ID_E wch_id)
{
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];
	volatile WCH_REG_UNION_T *wch_sw_reg = NULL;
	volatile WCH_REG_UNION_T *wch_hw_reg = NULL;

	if (!_wch_hal_info[wch_id].is_wch_hal_inited) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch is not inited \n");
		return;
	}
	wch_hw_reg = info->wch_hw_reg;
	wch_sw_reg = &info->wch_sw_reg;
	wch_sw_reg->reg_filed.VDOIN_EN.VI_EN = 1;
	wch_hw_reg->reg[IDX_VDOIN_EN] = wch_sw_reg->reg[IDX_VDOIN_EN];
}

void wch_hal_disable(WCH_ID_E wch_id)
{
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];
	volatile WCH_REG_UNION_T *wch_sw_reg = NULL;
	volatile WCH_REG_UNION_T *wch_hw_reg = NULL;

	if (!_wch_hal_info[wch_id].is_wch_hal_inited) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch is not inited \n");
		return;
	}
	wch_hw_reg = info->wch_hw_reg;
	wch_sw_reg = &info->wch_sw_reg;
	wch_sw_reg->reg_filed.VDOIN_EN.VI_EN = 0;
	wch_hw_reg->reg[IDX_VDOIN_EN] = wch_sw_reg->reg[IDX_VDOIN_EN];
}

static void wch_get_hw_reg_to_sw(WCH_ID_E wch_id)
{
	u32 reg_cnt = 0;
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];

	if (!info->is_get_hw_reg) {
		WCH_LOG_INFO(wch_id, "wch_get_hw_reg_to_sw wch_hw_reg: %p\n",
			 _wch_hal_info[wch_id].wch_hw_reg);
		for (; reg_cnt < WCH_REG_NUM; reg_cnt++) {
			info->wch_sw_reg.reg[reg_cnt] = info->wch_hw_reg->reg[reg_cnt];
			WCH_LOG(WCH_LOG_LVL_DBG,"wch_hal_set_info wch_hw_reg->reg[%d]: 0x%x, regaddr: %p\n", 
				reg_cnt, info->wch_hw_reg->reg[reg_cnt], info->wch_hw_reg->reg + reg_cnt);
		}
		info->is_get_hw_reg = true;
	}
}

void wch_img_status_clear(WCH_ID_E wch_id, uintptr_t base)
{
	WCH_SET_BIT(base, (_wch_int_clr_offset[wch_id]), WCH_BIT(_wch_int_clr_bit[wch_id]));
}

void wch_img_status_enable(WCH_ID_E wch_id, uintptr_t base)
{
	WCH_CLR_BIT(base, (_wch_int_clr_offset[wch_id]), WCH_BIT(_wch_int_clr_bit[wch_id]));
}

void wch_hal_set_reg_base(WCH_ID_E wch_id, void *reg_base)
{
	_wch_reg[wch_id] = (uintptr_t)reg_base;
}

void wch_hal_set_clk_rst_base(WCH_ID_E wch_id, void *clk_rst_base)
{
	_clk_rst_base[wch_id] = (uintptr_t)clk_rst_base;
}

u32 wch_hal_check_filed_stauts(WCH_ID_E wch_id)
{
	u32 filed_flag = 0;
	u32 reg50 = 0;
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];

	reg50 = info->wch_hw_reg->reg[0x50/4];
	filed_flag = (reg50 & WCH_BIT(28)) >> 28;
	/*
	WCH_LOG(WCH_LOG_LVL_DBG, "wch get filed status,,  filed_flag: %d, reg50: 0x%x, cur_time: %d\n", 
		filed_flag, reg50, GetTickCount());
	*/

	return filed_flag;
}

void wch_hal_init(WCH_ID_E wch_id)
{
	if (_wch_hal_info[wch_id].is_wch_hal_inited) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch is already inited  \n");
		return;
	}

	wch_hal_set_info(wch_id);
    wch_clk_enable(wch_id);
	wch_get_hw_reg_to_sw(wch_id);
	_wch_hal_info[wch_id].is_wch_hal_inited = true;
}

void wch_hal_deinit(WCH_ID_E wch_id)
{
	wch_clk_disable(wch_id);
	_wch_hal_info[wch_id].is_wch_hal_inited = false;
}

void wch_hal_set_yc_addr(WCH_ID_E wch_id, u32 y_addr, u32 c_addr)
{
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];
	volatile WCH_REG_UNION_T *wch_sw_reg = NULL;
	volatile WCH_REG_UNION_T *wch_hw_reg = NULL;

	if (!info->is_wch_hal_inited) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hal_set_yc_addr wch hal not be inited \n");
		return;
	}
	WCH_LOG(WCH_LOG_LVL_DBG, "wch_hal_set_yc_addr Y = %x, C = %x \n", y_addr, c_addr);

	wch_hw_reg = info->wch_hw_reg;
	wch_sw_reg = &info->wch_sw_reg;

	wch_sw_reg->reg_filed.YBUF0_ADDR.YBUF0_ADDR = y_addr;
	wch_sw_reg->reg_filed.CBUF0_ADDR.CBUF0_ADDR = c_addr;

	wch_hw_reg->reg[IDX_YBUF0_ADDR] = wch_sw_reg->reg[IDX_YBUF0_ADDR];
	wch_hw_reg->reg[IDX_CBUF0_ADDR] = wch_sw_reg->reg[IDX_CBUF0_ADDR];
}


void wch_hal_set_input(WCH_ID_E wch_id, WCH_INPUT_INFO_T * input_info)
{
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];
	volatile WCH_REG_UNION_T *wch_sw_reg = NULL;
	volatile WCH_REG_UNION_T *wch_hw_reg = NULL;

	if (!info->is_wch_hal_inited) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hal_set_input wch hal not be inited \n");
		return;
	}
	wch_hw_reg = info->wch_hw_reg;
	wch_sw_reg = &info->wch_sw_reg;

	WCH_LOG(WCH_LOG_LVL_HAL,
		"src_fmt: %s,src_width=%d,src_height=%d,start_x=%d,start_y_top=%d,start_y_bot=%d,input_data_type: %s\n",
			_wch_data_fmt[input_info->src_fmt], input_info->src_width, input_info->src_height,
			input_info->start_x, input_info->start_y_top, input_info->start_y_bot,
			_wch_indata_type[input_info->input_data_type]);

	wch_sw_reg->reg_filed.VDOIN_EN.Field_int = 1;
	wch_sw_reg->reg_filed.VDOIN_EN.FldInv = 1;
	wch_sw_reg->reg_filed.VDOIN_EN.BGVdoformat = 1;

	wch_sw_reg->reg_filed.INPUT_CTRL.vdoin_Y_chn_sel = 0x0;
	wch_sw_reg->reg_filed.INPUT_CTRL.vdoin_CB_chn_sel = 0x1;
	wch_sw_reg->reg_filed.INPUT_CTRL.vdoin_CR_chn_sel = 0x2;

	wch_sw_reg->reg_filed.ACT_LINE.BGVSyn601Det = 0x1;
	wch_sw_reg->reg_filed.ACT_LINE.IndexMode = 0x1;

	wch_sw_reg->reg_filed.VSCALE.fld_reset = 1;
	wch_sw_reg->reg_filed.VSCALE.Vsyn_reset = 1;
	wch_sw_reg->reg_filed.VSCALE.Vdifld_reset = 0;
	wch_sw_reg->reg_filed.VSCALE.VSCALE_reserved1 = 0;
#if WCH_SHOW_PATTERN
	wch_sw_reg->reg_filed.VSCALE.bgtest_bar = 1;
#endif
	wch_sw_reg->reg_filed.VSCALE.bghsize_dw = (input_info->src_width >> 4);

	wch_sw_reg->reg_filed.VDOIN_YCSAMPLE.C_SAMPLE_EN = 0x1;
	wch_sw_reg->reg_filed.VDOIN_YCSAMPLE.ext_field = 0x1;
	wch_sw_reg->reg_filed.VDOIN_YCSAMPLE.ch_vrst_en = 0x1;
	wch_sw_reg->reg_filed.VDOIN_YCSAMPLE.VDOIN_YCSAMPLE_reserved1 = 0x1;

	wch_sw_reg->reg_filed.VDOIN_MODE.YCFieldEnd = 0;

	switch (wch_id) {
	case WCH_VDI:
		/*interrupt method choose*/
		wch_sw_reg->reg_filed.VSCALE.Fld_int = 0;
		wch_sw_reg->reg_filed.VSCALE.Vsysn_int = 1;
		wch_sw_reg->reg_filed.VSCALE.Vdifld_int = 0;
		wch_sw_reg->reg_filed.VSCALE.Vdifld_int2 = 0;
		wch_sw_reg->reg_filed.INPUT_CTRL.new_int_ena = 0;

		wch_sw_reg->reg_filed.VSCALE.bgsramcs_ena = 1;
		wch_sw_reg->reg_filed.HCNT_SETTING.HACTCNT = input_info->src_width;
		wch_sw_reg->reg_filed.VDOIN_EN.SRAM_EN_SEL = 1;
		wch_sw_reg->reg_filed.VDOIN_EN.BGenVsyn601Inv = 1;
		wch_sw_reg->reg_filed.HPIXEL.NPIXEL = 0;
		wch_sw_reg->reg_filed.HPIXEL.NPIXEL_PRE = 0;
		wch_sw_reg->reg_filed.TOP_BOT_START_LIN.BL = 0;
		wch_sw_reg->reg_filed.TOP_BOT_START_LIN.TL = 0;
		if (DATA_IN_FMT_YUV444 == input_info->src_fmt) {
			wch_sw_reg->reg_filed.INPUT_CTRL.sd_480i_mix_eco = 0;
		}

		if (input_info->src_height == 480){/*NTSC*/
			wch_sw_reg->reg_filed.HCNT_SETTING.HCNT = WCH_TVD_N_HTOTAL;
		} else if (input_info->src_height == 576){/*PAL*/
			wch_sw_reg->reg_filed.HCNT_SETTING.HCNT = WCH_TVD_P_HTOTAL;
		}
		break;

	case WCH_DI:
		/*interrupt method choose*/
		wch_sw_reg->reg_filed.VSCALE.Fld_int = 1;
		wch_sw_reg->reg_filed.VSCALE.Vsysn_int = 1;
		wch_sw_reg->reg_filed.VSCALE.Vdifld_int = 1;
		wch_sw_reg->reg_filed.VSCALE.Vdifld_int2 = 1;
		wch_sw_reg->reg_filed.INPUT_CTRL.new_int_ena = 1;

		wch_sw_reg->reg_filed.VDOIN_MODE.MaskEn = 0;
		wch_sw_reg->reg_filed.VDOIN_MODE.ReqNew = 1;
		wch_sw_reg->reg_filed.VDOIN_EN.SRAM_EN_SEL = 0;
		wch_sw_reg->reg_filed.VDOIN_EN.BGenVsyn601Inv = 0;
		wch_sw_reg->reg_filed.ACT_LINE.IndexModeThres = 0x20;
		wch_sw_reg->reg_filed.INPUT_CTRL.h_edge_sel = 1;
		wch_sw_reg->reg_filed.INPUT_CTRL.BGenVsyn_Inv = 1;
		wch_sw_reg->reg_filed.HPIXEL.NPIXEL_PRE = 1;
		wch_sw_reg->reg_filed.HCNT_SETTING.HACTCNT = input_info->src_width << 1;
		wch_sw_reg->reg_filed.VSCALE.bgsramcs_ena = 0;

		wch_sw_reg->reg_filed.HPIXEL.NPIXEL = input_info->start_x;
		wch_sw_reg->reg_filed.TOP_BOT_START_LIN.BL = input_info->start_y_bot;
		wch_sw_reg->reg_filed.TOP_BOT_START_LIN.TL = input_info->start_y_top;
		wch_sw_reg->reg_filed.VDOIN_MODE.VDOIN_MODE_reserved2 = 0;
		wch_sw_reg->reg_filed.VDOIN_MODE.VDOIN_MODE_reserved3 = 0;
		wch_sw_reg->reg_filed.VDOIN_MODE.Fifo_THRES = 0x4;
		wch_sw_reg->reg_filed.HCNT_SETTING.HCNT = input_info->h_total;
		break;

	default:
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hsl_set_input unkown wch_id: %d\n", wch_id);
		break;
	}

	switch (input_info->src_fmt) {
	case DATA_IN_FMT_YUV444:
		wch_sw_reg->reg_filed.INPUT_CTRL.vdoin_444_mode = 1;
		break;

	case DATA_IN_FMT_YUV422:
		wch_sw_reg->reg_filed.INPUT_CTRL.vdoin_444_mode = 0;
		break;

	default:
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hsl_set_input not support src_fmt: %d\n", input_info->src_fmt);
		break;
	}

	switch (input_info->input_data_type) {
	case INDATA_TYPE_P:
		wch_sw_reg->reg_filed.VDOIN_EN.PRGS = 1;
		wch_sw_reg->reg_filed.ACT_LINE.ACTLINE = input_info->src_height - 1;
		break;

	case INDATA_TYPE_I:
		wch_sw_reg->reg_filed.VDOIN_EN.PRGS = 0;
		wch_sw_reg->reg_filed.ACT_LINE.ACTLINE = (input_info->src_height >> 1) - 1;
		break;

	default:
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hsl_set_input not support data type: %d\n", input_info->input_data_type);
		break;
	}

	wch_hw_reg->reg[IDX_VDOIN_EN] = wch_sw_reg->reg[IDX_VDOIN_EN];
	WCH_LOG_INFO(wch_id, "wch_hal_set_input wch_hw_reg: %p VDOIN_EN: %p \n",
		wch_hw_reg, wch_hw_reg->reg + IDX_VDOIN_EN);
	WCH_LOG_INFO(wch_id, "wch_hal_set_input VDOIN_EN val: 0x%08x \n",
		WCH_HAL_READ32(wch_hw_reg->reg + IDX_VDOIN_EN));
	wch_hw_reg->reg[IDX_VDOIN_MODE] = wch_sw_reg->reg[IDX_VDOIN_MODE];
	wch_hw_reg->reg[IDX_ACT_LINE] = wch_sw_reg->reg[IDX_ACT_LINE];
	wch_hw_reg->reg[IDX_HPIXEL] = wch_sw_reg->reg[IDX_HPIXEL];
	wch_hw_reg->reg[IDX_TOP_BOT_START_LINE] = wch_sw_reg->reg[IDX_TOP_BOT_START_LINE];
	wch_hw_reg->reg[IDX_INPUT_CTRL] = wch_sw_reg->reg[IDX_INPUT_CTRL];
	wch_hw_reg->reg[IDX_HCNT_SETTING] = wch_sw_reg->reg[IDX_HCNT_SETTING];
	wch_hw_reg->reg[IDX_HCNT_SETTING_1] = wch_sw_reg->reg[IDX_HCNT_SETTING_1];
	wch_hw_reg->reg[IDX_VSCALE] = wch_sw_reg->reg[IDX_VSCALE];
	wch_hw_reg->reg[IDX_VDOIN_YCSAMPLE] = wch_sw_reg->reg[IDX_VDOIN_YCSAMPLE];
}

void wch_hal_set_output(WCH_ID_E wch_id, WCH_OUTPUT_INFO_T * output_info)
{
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];
	volatile WCH_REG_UNION_T *wch_sw_reg = NULL;
	volatile WCH_REG_UNION_T *wch_hw_reg = NULL;

	if (!info->is_wch_hal_inited) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hal_set_output wch hal not be inited \n");
		return;
	}
	wch_hw_reg = info->wch_hw_reg;
	wch_sw_reg = &info->wch_sw_reg;

	WCH_LOG_INFO(wch_id,
		"dst_fmt: %s, data_mode: %s, dst_width=%d, dst_height=%d \n",
			_wch_data_fmt[output_info->dst_fmt], _wch_data_mode[output_info->data_mode],
			output_info->dst_width, output_info->dst_height);

	switch (output_info->data_mode) {
		case DATA_MODE_SCANLINE:
			wch_sw_reg->reg_filed.VDOIN_EN.Linear_ena = 1;
			break;

		case DATA_MODE_BLOCK: /*16x32*/
			wch_sw_reg->reg_filed.VDOIN_EN.Linear_ena = 0;
			wch_sw_reg->reg_filed.VDOIN_EN.addr_swap_1 = 0;
			wch_sw_reg->reg_filed.VDOIN_EN.SWAP = 0;
			wch_sw_reg->reg_filed.VDOIN_BLOCKMODE.BlockActHcnt = 720;
			break;

		default:
			WCH_LOG(WCH_LOG_LVL_ERR, "wch_hsl_set_output not support data_mode: %d\n", output_info->data_mode);
			break;
	}

	wch_sw_reg->reg_filed.HCNT_SETTING_1.CHCNT = (output_info->dst_width >> 4) - 1;
	wch_sw_reg->reg_filed.HCNT_SETTING_1.YHCNT = (output_info->dst_width >> 4) - 1;

	switch (output_info->dst_fmt) {
	case DATA_OUT_FMT_YUV422_1PLANE:
	case DATA_OUT_FMT_YUV422_2PLANES:
		if (DATA_OUT_FMT_YUV422_1PLANE == output_info->dst_fmt) {
			u32 one_plane_mode[ONE_PLANE_MODE_MAX] = {0x0, 0x1, 0x2, 0x3};

			if (output_info->plane_mode < ONE_PLANE_MODE_MAX) {
				wch_sw_reg->reg_filed.VDOIN_EN.one_plane_mode = one_plane_mode[output_info->plane_mode]; 
			} else {
				wch_sw_reg->reg_filed.VDOIN_EN.one_plane_mode = 0x0;
			}
			wch_sw_reg->reg_filed.VDOIN_EN.one_plane_ena = 1;
#if 0
			wch_sw_reg->reg_filed.VSCALE.Fld_int = 0;
			wch_sw_reg->reg_filed.VSCALE.Vsysn_int = 0;
			wch_sw_reg->reg_filed.VSCALE.Vdifld_int = 0;
#endif
			wch_sw_reg->reg_filed.VSCALE.bghsize_dw = (output_info->dst_stride >> 3);
			wch_sw_reg->reg_filed.VDOIN_EN.mode_422 = 0;
			wch_sw_reg->reg_filed.HCNT_SETTING_1.CHCNT = (output_info->dst_width >> 3) - 1;
			wch_sw_reg->reg_filed.HCNT_SETTING_1.YHCNT = (output_info->dst_width >> 3) - 1;
			wch_sw_reg->reg_filed.VDOIN_YCSAMPLE.C_SAMPLE_EN = 0;
		} else {
			wch_sw_reg->reg_filed.VDOIN_EN.mode_422 = 1;
			wch_sw_reg->reg_filed.VDOIN_EN.one_plane_ena = 0;
		}

		switch (output_info->input_data_type) {
			case INDATA_TYPE_P:
				wch_sw_reg->reg_filed.DW_NEED.DW_NEED_C_LINE = output_info->dst_height - 1;
				wch_sw_reg->reg_filed.DW_NEED.DW_NEED_Y_LINE = output_info->dst_height - 1;
				break;

			case INDATA_TYPE_I:
				wch_sw_reg->reg_filed.DW_NEED.DW_NEED_C_LINE = (output_info->dst_height >> 1) - 1;
				wch_sw_reg->reg_filed.DW_NEED.DW_NEED_Y_LINE = (output_info->dst_height >> 1) - 1;
				break;

			default:
				WCH_LOG(WCH_LOG_LVL_ERR, "wch_hsl_set_output not support data type: %d\n", output_info->input_data_type);
				break;
		}
		break;

	case DATA_OUT_FMT_YUV420_2PLANES:
		wch_sw_reg->reg_filed.VDOIN_EN.mode_422 = 0;

		switch (output_info->input_data_type) {
			case INDATA_TYPE_P:
				wch_sw_reg->reg_filed.DW_NEED.DW_NEED_C_LINE = (output_info->dst_height >> 1) - 1;
				wch_sw_reg->reg_filed.DW_NEED.DW_NEED_Y_LINE = output_info->dst_height - 1;
				break;

			case INDATA_TYPE_I:
				wch_sw_reg->reg_filed.DW_NEED.DW_NEED_C_LINE = (output_info->dst_height >> 2) - 1;
				wch_sw_reg->reg_filed.DW_NEED.DW_NEED_Y_LINE = (output_info->dst_height >> 1) - 1;
				break;

			default:
				WCH_LOG(WCH_LOG_LVL_ERR, "wch_hsl_set_output not support data type: %d\n", output_info->input_data_type);
				break;
		}
		break;

	default:
		WCH_LOG(WCH_LOG_LVL_ERR,
			"wch_hw_set_output unkown output fmt dst_fmt=%d \n", output_info->dst_fmt);
		break;
	}

	wch_hw_reg->reg[IDX_VDOIN_EN] = wch_sw_reg->reg[IDX_VDOIN_EN];
	wch_hw_reg->reg[IDX_DW_NEED] = wch_sw_reg->reg[IDX_DW_NEED];
	wch_hw_reg->reg[IDX_HCNT_SETTING_1] = wch_sw_reg->reg[IDX_HCNT_SETTING_1];
	wch_hw_reg->reg[IDX_VSCALE] = wch_sw_reg->reg[IDX_VSCALE];
	wch_hw_reg->reg[IDX_VDOIN_YCSAMPLE] = wch_sw_reg->reg[IDX_VDOIN_YCSAMPLE];
	wch_hw_reg->reg[IDX_BLOCKMODE] = wch_sw_reg->reg[IDX_BLOCKMODE];
}

void wch_hal_start(WCH_ID_E wch_id)
{
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];
	volatile WCH_REG_UNION_T *wch_sw_reg = NULL;
	volatile WCH_REG_UNION_T *wch_hw_reg = NULL;

	WCH_LOG(WCH_LOG_LVL_HAL, "wch version [%s]\n", WCH_VERSION);
	if (!info->is_wch_hal_inited) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hal_start wch hal not be inited \n");
		return;
	}
	wch_hw_reg = info->wch_hw_reg;
	wch_sw_reg = &info->wch_sw_reg;

	WCH_LOG(WCH_LOG_LVL_HAL, "wch_hal_start id = %d,  \n", (int)wch_id);

	wch_sw_reg->reg_filed.VDIN_REQOUT.CH_REQ_EMPTY = 1;
	wch_hw_reg->reg[IDX_VDOIN_REQOUT] = wch_sw_reg->reg[IDX_VDOIN_REQOUT];
	wch_sw_reg->reg_filed.VDIN_REQOUT.CH_PROTECT_DISABLE = 1;
	wch_hw_reg->reg[IDX_VDOIN_REQOUT] = wch_sw_reg->reg[IDX_VDOIN_REQOUT];

	wch_sw_reg->reg_filed.VDOIN_EN.VI_EN = 1;
	wch_hw_reg->reg[IDX_VDOIN_EN] = wch_sw_reg->reg[IDX_VDOIN_EN];
	WCH_LOG_INFO(wch_id, "wch_hal_start wch_hw_reg: %p VDOIN_EN: %p \n",
		wch_hw_reg, wch_hw_reg->reg + IDX_VDOIN_EN);
	WCH_LOG_INFO(wch_id, "wch_hal_start VDOIN_EN val: 0x%x \n",
		WCH_HAL_READ32(wch_hw_reg->reg + IDX_VDOIN_EN));

	wch_sw_reg->reg_filed.WRAPPER_3D_SETTING.swrst = 0x3;
	wch_hw_reg->reg[IDX_WRAPPER_3D_SETTING] = wch_sw_reg->reg[IDX_WRAPPER_3D_SETTING];
	wch_sw_reg->reg_filed.WRAPPER_3D_SETTING.swrst = 0x0;
	wch_hw_reg->reg[IDX_WRAPPER_3D_SETTING] = wch_sw_reg->reg[IDX_WRAPPER_3D_SETTING];

#if WCH_OPEN_SHADOW
	/*After open shadow, writing register will take effect after the next interrupt, so open it at the start end*/
	wch_sw_reg->reg_filed.VDOIN_YCSAMPLE.YC_LATCH_MODE = 0x1;
	wch_hw_reg->reg[IDX_VDOIN_YCSAMPLE] = wch_sw_reg->reg[IDX_VDOIN_YCSAMPLE];
#endif
}

void wch_hal_stop(WCH_ID_E wch_id)
{
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];
	volatile WCH_REG_UNION_T *wch_sw_reg = NULL;
	volatile WCH_REG_UNION_T *wch_hw_reg = NULL;

	if (!info->is_wch_hal_inited) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hal_stop wch hal not be inited \n");
		return;
	}

	wch_hw_reg = info->wch_hw_reg;
	wch_sw_reg = &info->wch_sw_reg;

	WCH_LOG(WCH_LOG_LVL_HAL, "wch_hal_stop id = %d,  \n", (int)wch_id);

#if WCH_OPEN_SHADOW
	/*We should close shadow firt, than writing other registers will take effect immediately*/
	wch_sw_reg->reg_filed.VDOIN_YCSAMPLE.YC_LATCH_MODE = 0x2;
	wch_hw_reg->reg[IDX_VDOIN_YCSAMPLE] = wch_sw_reg->reg[IDX_VDOIN_YCSAMPLE];
#endif
	wch_sw_reg->reg_filed.WRAPPER_3D_SETTING.swrst = 0x3;
	wch_hw_reg->reg[IDX_WRAPPER_3D_SETTING] = wch_sw_reg->reg[IDX_WRAPPER_3D_SETTING];
	wch_sw_reg->reg_filed.VDOIN_EN.VI_EN = 0;
	wch_hw_reg->reg[IDX_VDOIN_EN] = wch_sw_reg->reg[IDX_VDOIN_EN];
	WCH_LOG(WCH_LOG_LVL_HAL, "wch_hal_stop OK\n");
}

void wch_hal_set_shadow(WCH_ID_E wch_id)
{
	WCH_HAL_INFO_T *info = &_wch_hal_info[wch_id];
	volatile WCH_REG_UNION_T *wch_sw_reg = NULL;
	volatile WCH_REG_UNION_T *wch_hw_reg = NULL;

	if (!info->is_wch_hal_inited) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hal_start wch hal not be inited \n");
		return;
	}
	wch_hw_reg = info->wch_hw_reg;
	wch_sw_reg = &info->wch_sw_reg;
	wch_sw_reg->reg_filed.VDOIN_YCSAMPLE.YC_LATCH_MODE = 0x1;
	wch_hw_reg->reg[IDX_VDOIN_YCSAMPLE] = wch_sw_reg->reg[IDX_VDOIN_YCSAMPLE];
}

