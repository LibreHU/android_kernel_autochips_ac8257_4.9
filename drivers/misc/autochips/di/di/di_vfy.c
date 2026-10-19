#include "di_hal.h"
#include "di_if.h"
#include "types.h"
#include "display.h"
#include "pmx_hal.h"
#include "log.h"
//#include "reserve_memory.h"
#include "pmx_hw.h"
#include "vdp_hal.h"
#include "di_hal.h"

DI_PRM_T PALPARAM[4];
void di_inital_vfy_param(int w, int h)
{
	unsigned int i = 0;
	for (i = 0; i < 4; i++) {
		PALPARAM[i].src_rect.top = 0;
		PALPARAM[i].src_rect.left = 0;
		PALPARAM[i].src_rect.right = 720;
		PALPARAM[i].src_rect.bottom = 480;
		PALPARAM[i].src_width = 720;
		PALPARAM[i].src_height = 480;

		PALPARAM[i].duration = 0;
		PALPARAM[i].dst_rect.top = 0;
		PALPARAM[i].dst_rect.left = 0;
	}
#if 1
	PALPARAM[0].src_rect.right = w;
	PALPARAM[0].src_rect.bottom = h;
	PALPARAM[0].src_width = w;
	PALPARAM[0].src_height = h;
	//800x480
	PALPARAM[0].dst_rect.bottom = h;
	PALPARAM[0].dst_rect.right = w;
	PALPARAM[0].dst_width = w;
	PALPARAM[0].dst_height = h;
#else
	PALPARAM[0].src_rect.right = 1440;
	PALPARAM[0].src_rect.bottom = 960;
	PALPARAM[0].src_width = 1440;
	PALPARAM[0].src_height = 960;
	//800x480
	PALPARAM[0].dst_rect.bottom = 960;
	PALPARAM[0].dst_rect.right = 1440;
	PALPARAM[0].dst_width = 1440;
	PALPARAM[0].dst_height = 960;
#endif
	//1024x600
	PALPARAM[1].dst_rect.bottom = 600;
	PALPARAM[1].dst_rect.right = 1024;
	PALPARAM[1].dst_height = 600;
	PALPARAM[1].dst_width = 1024;


	//1280x800
	PALPARAM[2].dst_rect.bottom = 800;
	PALPARAM[2].dst_rect.right = 1280;
	PALPARAM[2].dst_height = 800;
	PALPARAM[2].dst_width = 1280;

	//1920x1080
	PALPARAM[3].dst_rect.bottom = 1080;
	PALPARAM[3].dst_rect.right = 1920;
	PALPARAM[3].dst_height = 1080;
	PALPARAM[3].dst_width = 1920;


}

