/*
 * Stage-1 placeholder for AutoChips' "lcm_driver_common" (AC8257, Jancar UJC201).
 *
 * The stock driver is data driven: it reads the panel description (resolution, DSI timings, power
 * sequence, MIPI->LVDS bridge init table) from the "metazone" partition through ATC_METAZONE
 * (lcm_params_info_init, get_specific_part_extended_panel_info, lvds_push_table in the stock kernel).
 * Neither is reconstructed yet.
 *
 * This placeholder only reports a 1024x600 DSI video-mode panel and never touches the panel or the
 * bridge: LK already powered and initialised them, so the picture may survive as long as these
 * timings are close to LK's. Suspend / resume do nothing (the screen stays on). Replace with the
 * reconstructed driver (see docs/RECONSTRUCTION_STATUS.md, display).
 *
 * Timings from the KR070IA4T 1024x600 DSI panel of the same BSP: reference values, not the stock ones.
 */
#include "lcm_drv.h"

#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 600

static struct LCM_UTIL_FUNCS lcm_util;

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));
	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->dsi.mode = SYNC_EVENT_VDO_MODE;
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.packet_size = 256;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count = FRAME_WIDTH * 3;
	params->dsi.vertical_sync_active = 6;
	params->dsi.vertical_backporch = 3;
	params->dsi.vertical_frontporch = 20;
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 6;
	params->dsi.horizontal_backporch = 48;
	params->dsi.horizontal_frontporch = 16;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
	params->dsi.PLL_CLOCK = 221;
	params->dsi.cont_clock = 1;
}

/* LK did it: the panel and the bridge are already running. */
static void lcm_init(void)
{
}

static void lcm_suspend(void)
{
}

static void lcm_resume(void)
{
}

struct LCM_DRIVER lcm_driver_common_lcm_drv = {
	.name = "lcm_driver_common",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
};
