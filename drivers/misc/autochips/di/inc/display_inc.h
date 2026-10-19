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
#ifndef _DISPLAY_INC_H
#define _DISPLAY_INC_H

#ifndef ATC_ARM2_LK_SUPPORT
#include "linux/fb.h"
#include "di_hal.h"
//#include "types.h"
#else
#include "platform/mt_typedefs.h"
#include "sys/types.h"
#include "di_types.h"

#endif
#include "display.h"



#define ENABLE_PANEL_1024_768        0
#define ENABLE_PANEL_1280_800        0
#define ENABLE_PANEL_TTL_800_480     1

#define MASTER_MODE_ENABLE           0

#define TM070DDHG   0   /*1:is TM070DDHG LCD;0:isn't*/

#if TM070DDHG
#define MASTER_1024_600_HSTART       162
#define MASTER_1024_600_VSTART       26
#else
#define MASTER_1024_600_HSTART       104
#define MASTER_1024_600_VSTART       15
#endif

#define MASTER_1024_768_HSTART       104
#define MASTER_1024_768_VSTART       15
#define MASTER_1280_800_HSTART       130
#define MASTER_1280_800_VSTART       23
#define MASTER_800_480_HSTART        106
#define MASTER_800_480_VSTART        22

#define PRIMARY_OSD_X_OFFSET  0
#define PRIMARY_OSD_Y_OFFSET  0
#define PRIMARY_OSD_RIGHT_CORDER_X_MARGIN   0
#define PRIMARY_OSD_RIGHT_CORDER_Y_MARGIN   0

#define LCD_TYPE_LVDS  (1)
#define LCD_TYPE_TTL   (0)

#ifdef __cplusplus

extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

extern  u32  _u4DispMode;
extern  u32  _u4LCDWidth;
extern  u32  _u4LCDHeight;
extern  u32  _u4LCDType;
extern  u32  _u4RearOutputMode;
extern  int     aBitsPerPixel[];

#ifdef __cplusplus
}
#endif	/* __cplusplus */
/*#define VIDEO_MODE                          RES_480P*/
#define DISPLAY_MODE                        RES_480P_800
#define OUTPUT_WIDTH                800
#define OUTPUT_HEIGHT               480

#if ENABLE_PANEL_1280_800
#define PRIMARY_OSD_WIDTH     1280
#define PRIMARY_OSD_HEIGHT    800
#elif ENABLE_PANEL_TTL_800_480
#define PRIMARY_OSD_WIDTH     800
#define PRIMARY_OSD_HEIGHT    480
#else
#define PRIMARY_OSD_WIDTH     1024
#if ENABLE_PANEL_1024_768
#define PRIMARY_OSD_HEIGHT    768
#else
#define PRIMARY_OSD_HEIGHT    600
#endif
#endif

#if MONITOR_VDO_FPS
extern u32 vdo_fps_en[2];
extern u32 vdo_flip_cnt[2];
#endif

#ifndef ATC_ARM2_LK_SUPPORT
extern void __iomem *osdf_reg;
extern void __iomem *osd1_reg;
extern void __iomem *osd2_reg;
extern void __iomem *osd3_reg;
extern void __iomem *osd4_reg;
extern void __iomem *osd5_reg;
extern void __iomem *osdr_reg;
extern void __iomem *osdr1_reg;
extern void __iomem *osdr2_reg;
extern void __iomem *osdr3_reg;
extern void __iomem *fmtf_reg;
extern void __iomem *clk_reg;
extern void __iomem *vdof_reg;
extern void __iomem *vdo_8tap_reg;
extern void __iomem *fmtr_reg;
extern void __iomem *vdor_reg;
extern void __iomem *scl_reg;
extern void __iomem *sclf_reg;
extern void __iomem *mix_reg;
extern void __iomem *tlcp_reg;
extern void __iomem *togc_reg;
extern void __iomem *tcon_reg;
extern void __iomem *ddds_reg;
extern void __iomem *lvds_reg;
extern void __iomem *lvdsa_reg;
extern void __iomem *tve_reg;
extern void __iomem *tvebk1_reg;
extern void __iomem *tvebk2_reg;
#else
extern unsigned int osdf_reg;
extern unsigned int osd1_reg;
extern unsigned int osd2_reg;
extern unsigned int osd3_reg;
extern unsigned int osd4_reg;
extern unsigned int osd5_reg;
extern unsigned int osdr_reg;
extern unsigned int osdr1_reg;
extern unsigned int osdr2_reg;
extern unsigned int osdr3_reg;
extern unsigned int fmtf_reg;
extern unsigned int clk_reg;
extern unsigned int vdof_reg;
extern unsigned int vdo_8tap_reg;
extern unsigned int fmtr_reg;
extern unsigned int vdor_reg;
extern unsigned int scl_reg;
extern unsigned int sclf_reg;
extern unsigned int mix_reg;
extern unsigned int tlcp_reg;
extern unsigned int togc_reg;
extern unsigned int tcon_reg;
extern unsigned int ddds_reg;
extern unsigned int lvds_reg;
extern unsigned int lvdsa_reg;
extern unsigned int tve_reg;
extern unsigned int tvebk1_reg;
extern unsigned int tvebk2_reg;
#endif
extern unsigned int fmtf_irq;
extern unsigned int fmtr_irq;
extern unsigned int scl_irq;
extern struct clk *clk_imgrsz;
extern struct clk *clk_osdrsz;
extern struct clk *clk_lvds;
extern struct clk *clk_scl;
extern struct clk *clk_osdf;
extern struct clk *clk_osdr;
extern struct clk *clk_tcon;
extern struct clk *clk_fmtf;
extern struct clk *clk_fmtr;

extern struct clk *clk_cvbs;
extern struct clk *clk_cav;

extern struct clk *clk_osd1;
extern struct clk *clk_osd2;
extern struct clk *clk_osd3;
extern struct clk *clk_osd4;
extern struct clk *clk_osdr2;
extern struct clk *clk_osdr3;
extern struct clk *clk_scltg;
extern struct pinctrl *pinctrl_fb;


extern unsigned long fbm_base;
extern unsigned long fbm_size;


extern void * fbm_va;
extern int COLOR_DEPTH_32_BIT;
extern u32 _u4Tm070ddhg;
extern u32 fr_follow;


extern FB_CONFIG_T g_rFBConfig;

extern u32 front_fb_current_addr;

//#if defined(CONFIG_ATC_OS_android)/*for AEE add by yzq*/
extern unsigned int dal_base_pa;
extern void* dal_base_va;
#define dal_buf_size (1024*600*2 + 4096)
//#endif

#define FR_FOLLOW_NONE			(0)
#define FR_FOLLOW_UI			(1)
#define FR_FOLLOW_VIDEO			(2)
#define FR_FOLLOW_MAX			(3) /*FR_FOLLOW_UI | FR_FOLLOW_VIDEO*/

#define LCD_WIDTH_PHYSCIAL         155 /*(mm)*/
#define LCD_HEIGHT_PHYSCIAL        90  /*(mm)*/
#if defined(CONFIG_ATC_OS_android)
#define ANDROID_NUMBER_OF_BUFFERS 12
#elif defined(CONFIG_ATC_OS_linux)
#define ANDROID_NUMBER_OF_BUFFERS 3
#endif
#define CURSOR_TRANSFER_COLOR  0xff00

#define SW_2D_ACCELERATION /*software 2D acceleration enable*/


#define RESET_HW_ENGINE  (false)


#ifndef AC823X_ARM2
extern bool subtitle_osd_init(unsigned int addr,  OSD_DATA_T *prData);
extern bool subtitle_osd_palette_init(unsigned int addr, OSD_DATA_T *prData);
#endif
extern void subtitle_osd_off(unsigned int flags);

extern void display_init(unsigned long, unsigned long, unsigned int, unsigned int);


extern void display_uninit(void);
/*extern void display_flip(struct fb_var_screeninfo *prInfo);*/
extern void TurnOnTve(u32 u4LayerID, bool fgOn);

#ifndef AC823X_ARM2
extern void vGetFBConfigFromShareMemory(FB_CONFIG_T *prFBConfig);
#endif
extern u32 create_rear_osd_follow_with_front(u32 plane, u32 src_width, u32 src_height, u32 addr);
extern void rear_osd_follow_with_front_flip(u32 u4Plane, u32 u4Addr);



/*add by mtk94020 for subtitle*/
#define SUBTITLE_OSD_X_OFFSET  28
#define SUBTITLE_OSD_Y_OFFSET  20
#define SUBTITLE_OSD_RIGHT_CORDER_X_MARGIN   40
#define SUBTITLE_OSD_RIGHT_CORDER_Y_MARGIN   30

#define SUBTITLE_OSD_WIDTH  400
#define SUBTITLE_OSD_HEIGHT 200

#ifdef CONFIG_ATC_OS_linux
extern u32 dal_base;
extern u32 dal_size ;

#define DAL_OSD_REGION_PHY_BASE	(dal_base + dal_size - 0x2000)
#else
#define DAL_OSD_REGION_PHY_BASE	(fbm_base + fbm_size - 0xA000)
#endif

#define OSD_REGION_PHY_BASE		(fbm_base + fbm_size - 0x8000)
#define ARM2_FBDRV_SHARE_MEMORY_PA	(fbm_base + fbm_size - 0x6000)
#define ARM2_FBDRV_SHARE_GAMMA_PA	(fbm_base + fbm_size - 0x1000)

//#define FB_PHYSICAL_TO_VIRTUAL(x)	(fbm_va - fbm_base + (x))

#endif


