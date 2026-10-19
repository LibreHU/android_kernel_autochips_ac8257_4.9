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

#ifndef _PMX_HW_H_
#define _PMX_HW_H_

#ifndef ATC_ARM2_LK_SUPPORT
//#include "x_hal_io.h"
#endif
//#include "x_hal_ic.h"
#include "drv_config.h"
#include "chip_ver.h"

#define PMX_HAL_REG_MODE_NULL             0
#define PMX_HAL_REG_MODE_READ             (1 << 0)
#define PMX_HAL_REG_MODE_WRITE            (1 << 1)
/* Time sync between VDP & PMX API, Since VDP_MSG_Q_T will delay 1 VSYNC*/
#define PMX_HAL_REG_MODE_DELAY_WRITE      (1 << 3)


#define HAL_PMX_SCLER_MAIN_REG_NUM     (0x100/4) //jg:fixme
#define HAL_PMX_SCLER_AUX_REG_NUM     (0x100/4) //jg:fixme
#define HAL_PMX_DISP_MAIN_REG_NUM     (0x100/4) //modify for MT8520 12
#define HAL_PMX_DISP_AUX_REG_NUM      (0x100/4) //modify for MT8520 8
#define HAL_PMX_VDOUT_MAIN_REG_NUM     (0x100/4) //modify for MT8520 12
#define HAL_PMX_VDOUT_AUX_REG_NUM      (0x100/4) //modify for MT8520 8
#define HAL_PMX_DISP_MAIN_Y_REG_NUM    (0x50/4)
#define HAL_PMX_DISP_MAIN_C_REG_NUM     (0x50/4)
#define HAL_PMX_DISP_AUX_C_REG_NUM     (0x50/4)
#define PMX_HAL_MIX_REG_NUM                (0x10/4)


//12001000h
typedef struct _HAL_PMX_DISP_MAIN_FIELD_T
{
  /* DWORD - 000 */
  unsigned int     fgHOR_VALID_END                  : 13;
  unsigned int 					      :3;
  unsigned int 	   fgHOR_VALID_START		      :13;
  unsigned int     				      :3;

  /* DWORD - 004 */
  unsigned int     fgNR_IN_VSYNC_END                  : 11;
  unsigned int 						:1;
  unsigned int     fgNR_IN_VSYNC_START		:11;//[12-22]
  unsigned int    					:1;//23
  unsigned int     fgNR_IN_VSYNC_POLAR		:1;//24
  unsigned int     fgNR_IN_HSYNC_POLAR		:1;//25
  unsigned int     fgNR_IN_FIELD_POLAR		:1;//26
  unsigned int     fgNR_IN_DE_SELF			:1;//27
  unsigned int     fgNR_IN_ODD_V_START_OPT		:1;
  unsigned int     fgNR_IN_ODD_V_END_OPT		:1;
  unsigned int 	   fgNR_IN_EVEN_V_START_OPT		:1;//30
  unsigned int     fgNR_IN_EVEN_V_END_OPT		:1;



  /* DWORD - 008 */
  unsigned int      fgHOR_NR_VALID_END                :13;
  unsigned int						:3;
  unsigned int      fgHOR_NR_VALID_START		:13;
  unsigned int						:3;


  /* DWORD - 00C */
  unsigned int      fgNR_IN_HOR_END                 	:13;
  unsigned int						:3;
  unsigned int      fgNR_IN_HOR_STAT	               :13;
  unsigned int						:3;


  /* DWORD - 010 */
  unsigned int      fgBYPASS_NR                 	:1;
  unsigned int						:31;

  /* DWORD - 014 */
  unsigned int      fgCOLOR_BAR_ON                   	:1;
  unsigned int	    fgCOLOR_BAR_TYPE			:1;
  unsigned int						:14;
  unsigned int      fgCOLOR_BAR_WIDTH			:8;
  unsigned int						:7;
  unsigned int	    fgENABLE422_444			:1;

  /* DWORD - 018 */
  unsigned int      fgDI_AGENT_TRIG                  	:1;
  unsigned int						:31;

  /* DWORD - 01C */
  unsigned int      fgFMT_H_WAIT_VDO_WDMA_START      :13;
  unsigned int         					:3;
  unsigned int      fgNR_H_WAIT_VDO_WDMA_START	:13;
  unsigned int						:3;

  /* DWORD - 020 */
  unsigned int                       : 32;

  /* DWORD - 024 */
  unsigned int                       : 32;

  /* DWORD - 028 */
  unsigned int                      : 32;

  /* DWORD - 02C */
  unsigned int                      : 32;

  /* DWORD - 030 */
  unsigned int                      : 32;

  /* DWORD - 034 */
  unsigned int                      : 32;

  /* DWORD - 038 */
  unsigned int                      : 32;

  /* DWORD - 03C */
  unsigned int                      : 32;

  /* DWORD - 040 */
  unsigned int                                : 32;

  /* DWORD - 044 */
  unsigned int                                        : 32;

  /* DWORD - 048 */   // 0x42048
  unsigned int 					:32;
/*  unsigned int        fgV1_Y                  : 1;
  unsigned int        fgV1_C                  : 1;
  unsigned int        fgV1_NO_SYM         : 1;
  unsigned int        fgV1_SYM_OPT            : 1;
  unsigned int        u1V1_Y_MSB              : 2;
  unsigned int        u1V1_C_MSB              : 2;
  unsigned int                                : 2;
  unsigned int        fgC_16_PHAEE            : 1;
  unsigned int        fgC_EVEN_FILTER         : 1;
  unsigned int                                : 20;*/

  /* DWORD - 04C */
  unsigned int                      : 32;

  /* DWORD - 050 */
  unsigned int        fgCRC_INIT              : 1;
  unsigned int        fgCRC_CLR               : 1;
  unsigned int                                : 6;
  unsigned int        u4CRC_RLT               : 24;

  /* DWORD - 054 */
  unsigned int        u2DERING_THRESHOLD_Y      : 10;
  unsigned int        u2DERING_THRESHOLD_C      : 10;
  unsigned int                                  : 4;
  unsigned int        u1DERING_TRANS        : 4;
  unsigned int        fgDERING_EN_Y             : 1;
  unsigned int        fgDERING_EN_C             : 1;
  unsigned int        fgDERING_EN_EX            : 1;
  unsigned int                                  : 1;

  /* DWORD - 058 */
  unsigned int                                : 16;
  unsigned int        u1BD_X_WDH          : 6;
  unsigned int                                : 2;
  unsigned int        u1BD_Y_WDH          : 6;
  unsigned int                                : 1;
  unsigned int        fgBD_ON             : 1;

  /* DWORD - 05C */
  unsigned int                                        : 2;
  unsigned int        u1BD_Y                  : 6;
  unsigned int                                : 2;
  unsigned int        u1BD_CB             : 6;
  unsigned int                                : 2;
  unsigned int        u1BD_CR             : 6;
  unsigned int                                : 8;

  /* DWORD - 060 */
  unsigned int                                : 32;

  /* DWORD - 064 */
  unsigned int                                : 32;

  /* DWORD - 068 */
  unsigned int                                : 32;

  /* DWORD - 06C */
  unsigned int        fgHDWN_EN               : 1;
  unsigned int                                : 1;
  unsigned int        u1HDWN_TT_MD            : 2;
  unsigned int                                : 1;
  unsigned int        fgFULL_FIFO             : 1;
  unsigned int                                : 2;
  unsigned int        fgHDWN_422          : 1;
  unsigned int        fgSIDE_BLK              : 1;
  unsigned int        fgSIDE_LNR              : 1;
  unsigned int        fgHDWN_444          : 1;
  unsigned int        u1HDWN_OPT          : 4;
  unsigned int        u2HDWN_FAC          : 16;

  /* DWORD - 070 */
  unsigned int        u2HDWN_HEND         : 12;
  unsigned int                                : 4;
  unsigned int        u2HDWN_HBGN         : 12;
  unsigned int                                : 3;
  unsigned int        fgHDWN_ALL              : 1;

  /* DWORD - 074 */
  unsigned int        u2HDWN_VOEND            : 11;
  unsigned int                                : 5;
  unsigned int        u2HDWN_VOBGN            : 11;
  unsigned int                                : 5;

  /* DWORD - 078 */
  unsigned int        u2HDWN_VEEND            : 11;
  unsigned int                                : 5;
  unsigned int        u2HDWN_VEBGN            : 11;
  unsigned int                                : 5;

  /* DWORD - 07C */
  unsigned int        u2HDWN_HO_END       : 13;
  unsigned int        u2SCALE_R_SPEED     : 3;
  unsigned int        u2HDWN_HO_BGN       : 12;
  unsigned int                                : 1;
  unsigned int        u1HDWN_VO_POS       : 3;

  /* DWORD - 080 */
  unsigned int        fgNR_ADJ_SYNC_EN        : 1;
  unsigned int        fgNR_ADJ_FWD            : 1;
  unsigned int        fgNR_TOGGLE_OPT     : 1;
  unsigned int                                : 5;
  unsigned int        u2NR_HSYNC_DELAY        : 12;
  unsigned int        u2NR_VSYNC_DELAY        : 12;

  /* DWORD - 084 */
  unsigned int        u2NR_VSYNC_END      : 12;
//  unsigned int                                : 1;
  unsigned int        u2NR_VSYNC_SRT      : 12;
  //unsigned int                                : 1;
  unsigned int        fgNR_VSYNC_POLAR        : 1;
  unsigned int        fgNR_HSYNC_POLAR        : 1;
  unsigned int        fgNR_FIELD_POLAR        : 1;
  unsigned int        fgNR_DE_SELF            : 1;
  unsigned int        fgNR_ODD_V_SRT_OPT  : 1;
  unsigned int        fgNR_ODD_V_END_OPT  : 1;
  unsigned int        fgNR_EVEN_V_SRT_OPT : 1;
  unsigned int        fgNR_EVEN_V_END_OPT : 1;

  /* DWORD - 088 */
  unsigned int        u2NR_HOR_END            : 12;
  unsigned int                                : 4;
  unsigned int        u2NR_HOR_SRT            : 12;
  unsigned int                                : 4;


  /* DWORD - 08C */
  unsigned int        u2NR_VO_END         : 11;
  unsigned int                                : 5;
  unsigned int        u2NR_VO_SRT         : 11;
  unsigned int                                : 5;

  /* DWORD - 090 */
  unsigned int        u2NR_VE_END         : 11;
  unsigned int                                : 5;
  unsigned int        u2NR_VE_SRT         : 11;
  unsigned int                                : 5;

  /* DWORD - 094 */ //0x42094
  unsigned int        u1HSYNWIDTH         : 8;
  unsigned int        u1VSYNWIDTH         : 5;
  unsigned int        fgHD_TP                 : 1; //[13] 1:HD 720 format,0:HD 1080 format
  unsigned int        fgHD_ON               : 1; //[14] 1:HD mode(1080i/720p),0:SD mode(480/576)
  unsigned int        fgPRGS                  : 1; //[15] Progressive output enable
  unsigned int                                : 1;
  unsigned int        fgPRGS_AUTOFLD      : 1;
  unsigned int        fgPRGS_INVFLD           : 1;
  unsigned int                                : 1;
  unsigned int        fgYUV_RST_OPT           : 1;
  unsigned int                                : 3;
  unsigned int        fgPRGS_FLD              : 1;
  unsigned int                                :2;
                 //NEW_SD_144M
  unsigned int        fgDSDCLK_EANBLE         :1;
  unsigned int        fgNEW_SD_MODE       : 1;
  unsigned int        fgNEW_SD_USE_EVEN   : 1;
  unsigned int        u1TVMODE                : 2; //[31:30] TV mode selection 00 NTSC 01 PAL_N 10 PAL_M 11 PAL_BDGHI

  /* DWORD - 098 */
  unsigned int                                : 16;
  unsigned int        u1PF_ADV                : 4;
  unsigned int        u1LUMACONTROL           : 12;

  /* DWORD - 09C */
  unsigned int        u2PXLLEN                : 13;
  unsigned int 					:3;
  unsigned int	      u2RIGHT_SKIP		:4;
  unsigned int                                : 12;

  /* DWORD - 0A0 */ // 0x420A0
  unsigned int        u2HACTEND               : 12;
  unsigned int                                : 4;
  unsigned int        u2HACTBGN               : 12;
  unsigned int                                : 1;
  unsigned int        fgLayer3_En             : 1;
  unsigned int        fgPmx_From_NR           : 1;
  unsigned int        fgLayer3_Trigger        : 1;

  /* DWORD - 0A4 */
  unsigned int        u2VOACTEND          : 12;
  unsigned int                                : 4;
  unsigned int        u2VOACTBGN          : 12;
  unsigned int        u1HIDE_OST              : 4;

  /* DWORD - 0A8 */
  unsigned int        u2VEACTEND              : 12;
  unsigned int                                : 4;
  unsigned int        u2VEACTBGN              : 12;
  unsigned int        u1HIDE_EST              : 4;

  /* DWORD - 0AC */
  unsigned int        fgVDO_EN                : 1;
  unsigned int        fgFMTM                  : 1;
  unsigned int                                : 1;
  unsigned int        fgHPOR                  : 1;
  unsigned int        fgVPOR                  : 1;
  unsigned int                                : 2;
  unsigned int        fgC_RST_SEL         : 1;
  unsigned int        u4PXLSEL                : 2;
  unsigned int        fgFTRST                 : 1;
  unsigned int        fgSHVSYN                : 1;
  unsigned int                                : 2;
  unsigned int        u1SYN_DEL               : 2;
  unsigned int                                : 3;
  unsigned int        fgUVSW                  : 1;
  unsigned int                                : 5;
  unsigned int        fgBLACK                 : 1;
  unsigned int                                : 1;
  unsigned int        fgPFOFF                 : 1;
  unsigned int        u1HW_OPT                : 4;

  /* DWORD - 0B0 */ // 0x420B0
  unsigned int        fgHSON                  : 1;
  unsigned int        fgHSLR                  : 1;
  //unsigned int        fgLPF_ON                : 1;
 // unsigned int        fgLPF_SL                : 1;
  unsigned int        u1ED_YUV_ST         : 2;
 // unsigned int        fgO_DIS                 : 1;
  //unsigned int        fgHDLPF                 : 1;
  unsigned int        u1YACC_ST               : 6;
  unsigned int        u1CACC_ST               :6;
  unsigned int        u2HSFACTOR              : 14;
  unsigned int        fgEVEN_FIR              : 1;
  unsigned int        fgPHASE_16              : 1;

  /* DWORD - 0B4 */
  unsigned int                                : 4;
  unsigned int        u1BIY                   : 4;
  unsigned int                                : 4;
  unsigned int        u1BICB                  : 4;
  unsigned int                                : 4;
  unsigned int        u1BICR                  : 4;
  unsigned int        fgPF2OFF                : 1;
  unsigned int        fgHIDE_L                : 1;
  unsigned int                                : 6;

  /* DWORD - 0B8 */
  unsigned int                                : 4;
  unsigned int        u1BGY                   : 4;
  unsigned int                                : 4;
  unsigned int        u1BGCB                  : 4;
  unsigned int                                : 4;
  unsigned int        u1BGCR                  : 4;
  unsigned int                                : 8;

  /* DWORD - 0BC */
  unsigned int        u1EDGE_RATIO            : 5;
  unsigned int        fgKNEE                  : 1;
  unsigned int        fgZCORE                 : 1;
  unsigned int        fgDNR                   : 1;
  unsigned int        u1CORE                  : 4;
  unsigned int        u1DOUT_CTL          : 3;
  unsigned int                                : 17;

  /* DWORD - 0C0 */
  unsigned int        u2DowndScaleEnd       : 16;
  unsigned int        u2DowndScaleStart     : 16;
  /* DWORD - 0C4 */
  unsigned int        u2C4Default             : 16;
  unsigned int        fgOLD_C_ACC         : 1;
  unsigned int                                : 1;
  unsigned int        fgOUT_SEL2	       :1;
  unsigned int        fgTVE_ND                : 1;
  unsigned int                                : 3;
  unsigned int        u1FIRST_PXL_LEAD        : 8;

  /* DWORD - 0C8 */
  unsigned int        NEW_SCL_MODE_EN         : 1;
  unsigned int        POST_SCL_USE_AC         : 1;
  unsigned int                                : 14;
  unsigned int        POST_SCL_WINDOW_LINEAR_SIZE_SEL : 2;
  unsigned int        POST_SCL_WINDOW_ACC_SIZE_SEL : 2;
  unsigned int        POST_SCL_PRE_DATA_NEXT_LUMA_Y_OPTION : 1; // 2'b00:2, 2'b01:4, 2'b10:6, 2'b11:8
  unsigned int        POST_SCL_NEXT_DATA_PRE_LUMA_Y_OPTION : 1;
  unsigned int        POST_SCL_PRE_DATA_NEXT_LUMA_C_OPTION : 1;
  unsigned int        POST_SCL_NEXT_DATA_PRE_LUMA_C_OPTION : 1;
  unsigned int                                : 7;
  unsigned int        POST_DIV2_SEL           : 1;

  // 0x420CC
  unsigned int        u4ScalerFactor          : 16;
  unsigned int        fgDemoModeEnable    : 1; //[16]
  unsigned int        fgDemoModeLeft      : 1; //[17]
  unsigned int        fgSD_27MHZ_ALignment:1;
  unsigned int 					: 1;
  unsigned int        u2DemoModeWidth     : 12;

 // 0x420D0
  /* DWORD - 0D0 */
  unsigned int        u2V_TOTAL_MIX           : 12;
  unsigned int                                : 4;
  unsigned int        u2H_TOTAL_MIX           : 13;
  unsigned int                                : 2;
  unsigned int        fgADJ_T_MIX             : 1;

  /* DWORD - 0D4 */ // 0x420D4
  unsigned int        u2V_TOTAL               : 12;
  unsigned int        horizontal_use_4fs      : 1;
  unsigned int        horizontal_use_3fs      : 1;
  unsigned int        horizontal_use_2fs      : 1;
  unsigned int        new_hd                  : 1;
  unsigned int        u2H_TOTAL               : 13;
  unsigned int                                : 1;
  unsigned int        horizontal_use_fs       : 1;
  unsigned int        fgADJ_T                 : 1;

  /* DWORD - 0D8 */
  unsigned int        INC_END                 : 12;
  unsigned int        INC_START               : 12;
  unsigned int        INC_STEP                : 8;

  /* DWORD - 0DC */
  unsigned int      DEC_END                   : 12;
  unsigned int        DEC_START               : 12;
  unsigned int        INIT_FACTOR             : 8;

  /* DWORD - 0E0 */
  unsigned int        FORCE_NEW_SCL_MODE_PATH_CLOCK_EN : 1;
  unsigned int        FORCE_OLD_SCL_MODE_PATH_CLOCK_EN : 1;
  unsigned int                                : 30;

  /* DWORD - 0E4 */
  unsigned int        fgMULTI_RATIO           : 1;
  unsigned int        fgDIRECT                : 1;
  unsigned int        fgC_FMTRST_M2           : 1;
  unsigned int        fgC_CHK_A_SCA           : 1;//0x423E4[3] luma key option
  unsigned int        fgY_ALL_8TAP_OUT        : 1;
  unsigned int        fgFACT_PREC             : 1;
  unsigned int        fgNOT_PST_D2            : 1;
  unsigned int        fgC_ALL_8TAP_OUT        : 1;
  unsigned int        fgHD_C_FIR_EN           : 1;
  unsigned int        u1LPF_SEL               : 3;
  unsigned int        fgUsePhase32            : 1;
  unsigned int        fgHD_C_POS              : 1;
  unsigned int        fgRST_PHASE         : 1;
  unsigned int        fgDemoModeHoriEnable   : 1;
  // MULTI_EXTEND[7:0]
  unsigned int        INIT_FACTOR_0           : 1; // bit 16
  unsigned int        INIT_FACTOR_9           : 1; // bit 17
  unsigned int        INC_STEP_0              : 1; // bit 18
  unsigned int                                : 1; // bit 19
  unsigned int        DEC_END_12              : 1; // bit 20
  unsigned int        DEC_START_12            : 1; // bit 21
  unsigned int        INC_END_12              : 1; // bit 22
  unsigned int        INC_START_12            : 1; // bit 23
  unsigned int        fgMVDO_4TAP_ONLY        : 1;
  unsigned int        fgMATCH_2T              : 1;
  unsigned int        fgOLD_NEXT_YC           : 1;
  unsigned int        fgNO_SUB_1              : 1;
  unsigned int        fgEN_FIRST_PXL_LEAD : 1;
  unsigned int        fgDownScaleMode         : 1;
  unsigned int        fg8TapCSControl         : 1;
  unsigned int                                : 1;

  //0x420E8
  unsigned int        u2HSYN_DELAY            : 12;
  unsigned int                                : 4;
  unsigned int        u2VSYN_DELAY            : 12;
  unsigned int                                : 3;
  unsigned int        fgADJ_FWD               : 1;

  //0x420EC
  unsigned int        u2DSDHSYN_DELAY         : 12;
  unsigned int                                : 4;
  unsigned int        u2DSDVSYN_DELAY         : 13;
  unsigned int                                : 2;
  unsigned int        fgDSDADJ_FWD                : 1;

  /* DWORD - 0F0 */
  unsigned int                                : 32;

  /* DWORD - 0F4 */
  unsigned int                                : 32;

  /* DWORD - 0F8 */
  unsigned int        fgCCONV_VP1         : 1;
  unsigned int        fgCC7TO6_VP1            : 1;
  unsigned int                                : 2;
  unsigned int        fgAverage422to444       : 1;
  unsigned int        fgOLD_CHROMA            : 1;
  unsigned int        fg235_TO_255_EN         : 1;
  unsigned int        fgDATA_235_255          : 1;
  unsigned int                                : 20;
  unsigned int     fgAdap_Chroma              : 1;//[28]
  unsigned int                                : 3;

  /* DWORD - 0FC */
  unsigned int        u1LUMA_KEY              : 12;
  unsigned int                                : 4;
  unsigned int        WINDOW_LINEAR_SIZE_SEL  : 2;
  unsigned int        WINDOW_ACC_SIZE_SEL     : 2;
  unsigned int        PRE_DATA_NEXT_LUMA_Y_OPTION : 1;
  unsigned int        NEXT_DATA_PRE_LUMA_Y_OPTION : 1;
  unsigned int        PRE_DATA_NEXT_LUMA_C_OPTION : 1;
  unsigned int        NEXT_DATA_PRE_LUMA_C_OPTION : 1;
  unsigned int                                : 7;
  unsigned int        POST_DIV2_SEL_2         : 1;
} HAL_PMX_DISP_MAIN_FIELD_T;


// 0x43000
typedef struct _HAL_PMX_DISP_AUX_FIELD_T
{
  /* DWORD - 000 */
  unsigned int        u4YHCOEF0               : 32;

  /* DWORD - 004 */
  unsigned int        u4YHCOEF1               : 32;

  /* DWORD - 008 */
  unsigned int        u4YHCOEF2               : 32;

  /* DWORD - 00C */
  unsigned int        u4YHCOEF3               : 32;

  /* DWORD - 010 */
  unsigned int        u4YHCOEF4               : 32;

  /* DWORD - 014 */
  unsigned int        u4YHCOEF5               : 32;

  /* DWORD - 018 */
  unsigned int        u4YHCOEF6               : 32;

  /* DWORD - 01C */
  unsigned int        u4YHCOEF7               : 32;

  /* DWORD - 020 */
  unsigned int        u4YHCOEF8               : 32;

  /* DWORD - 024 */
  unsigned int        u4YHCOEF9               : 32;

  /* DWORD - 028 */
  unsigned int        u4YHCOEF10              : 32;

  /* DWORD - 02C */
  unsigned int        u4YHCOEF11              : 32;

  /* DWORD - 030 */
  unsigned int        u4YHCOEF12              : 32;

  /* DWORD - 034 */
  unsigned int        u4YHCOEF13              : 32;

  /* DWORD - 038 */
  unsigned int        u4YHCOEF14              : 32;

  /* DWORD - 03C */
  unsigned int        u4YHCOEF15              : 32;

  /* DWORD - 040 */
  unsigned int                                : 32;

  /* DWORD - 044 */
  unsigned int                                        : 32;

  /* DWORD - 048 */
  unsigned int        fgV1_Y                  : 1;
  unsigned int        fgV1_C                  : 1;
  unsigned int        fgV1_NO_SYM         : 1;
  unsigned int        fgV1_SYM_OPT            : 1;
  unsigned int        u1V1_Y_MSB              : 2;
  unsigned int        u1V1_C_MSB              : 2;
  unsigned int                                : 2;
  unsigned int        fgC_16_PHAEE            : 1;
  unsigned int        fgC_EVEN_FILTER         : 1;
  unsigned int                                : 20;

  /* DWORD - 04C */
  unsigned int        u4YHCOEF19              : 32;

  /* DWORD - 050 */
  unsigned int        fgCRC_INIT              : 1;
  unsigned int        fgCRC_CLR               : 1;
  unsigned int                                : 6;
  unsigned int        u4CRC_RLT               : 24;

  /* DWORD - 054 */
  unsigned int          u2DERING_THRESHOLD_Y      : 10;
  unsigned int          u2DERING_THRESHOLD_C      : 10;
  unsigned int                                    : 4;
  unsigned int          u1DERING_TRANS        : 4;
  unsigned int          fgDERING_EN_Y             : 1;
  unsigned int          fgDERING_EN_C             : 1;
  unsigned int          fgDERING_EN_EX            : 1;
  unsigned int                                    : 1;


  /* DWORD - 058 */
  unsigned int                                : 16;
  unsigned int        u1BD_X_WDH          : 6;
  unsigned int                                : 2;
  unsigned int        u1BD_Y_WDH          : 6;
  unsigned int                                : 1;
  unsigned int        fgBD_ON             : 1;

  /* DWORD - 05C */
  unsigned int                                        : 2;
  unsigned int        u1BD_Y                  : 6;
  unsigned int                                : 2;
  unsigned int        u1BD_CB             : 6;
  unsigned int                                : 2;
  unsigned int        u1BD_CR             : 6;
  unsigned int                                : 8;

  /* DWORD - 060 */
  unsigned int                                : 32;

  /* DWORD - 064 */
  unsigned int                                : 32;

  /* DWORD - 068 */
  unsigned int                                : 32;

  /* DWORD - 06C */
  unsigned int        fgHDWN_EN               : 1;
  unsigned int                                : 1;
  unsigned int        u1HDWN_TT_MD            : 2;
  unsigned int                                : 1;
  unsigned int        fgFULL_FIFO             : 1;
  unsigned int                                : 2;
  unsigned int        fgHDWN_422          : 1;
  unsigned int        fgSIDE_BLK              : 1;
  unsigned int        fgSIDE_LNR              : 1;
  unsigned int        fgHDWN_444          : 1;
  unsigned int        u1HDWN_OPT          : 4;
  unsigned int        u2HDWN_FAC          : 16;

  /* DWORD - 070 */
  unsigned int        u2HDWN_HEND         : 12;
  unsigned int                                : 4;
  unsigned int        u2HDWN_HBGN         : 12;
  unsigned int                                : 3;
  unsigned int        fgHDWN_ALL              : 1;

  /* DWORD - 074 */
  unsigned int        u2HDWN_VOEND            : 11;
  unsigned int                                : 5;
  unsigned int        u2HDWN_VOBGN            : 11;
  unsigned int                                : 5;

  /* DWORD - 078 */
  unsigned int        u2HDWN_VEEND            : 11;
  unsigned int                                : 5;
  unsigned int        u2HDWN_VEBGN            : 11;
  unsigned int                                : 5;

  /* DWORD - 07C */
  unsigned int        u2HDWN_HO_END       : 12;
  unsigned int                                : 4;
  unsigned int        u2HDWN_HO_BGN       : 12;
  unsigned int                                : 1;
  unsigned int        u1HDWN_VO_POS       : 3;

  /* DWORD - 080 */
  unsigned int        fgNR_ADJ_SYNC_EN        : 1;
  unsigned int        fgNR_ADJ_FWD            : 1;
  unsigned int        fgNR_TOGGLE_OPT     : 1;
  unsigned int                                : 5;
  unsigned int        u2NR_HSYNC_DELAY        : 12;
  unsigned int        u2NR_VSYNC_DELAY        : 12;

  /* DWORD - 084 */
  unsigned int        u2NR_VSYNC_END      : 11;
  unsigned int                                : 1;
  unsigned int        u2NR_VSYNC_SRT      : 11;
  unsigned int                                : 1;
  unsigned int        fgNR_VSYNC_POLAR        : 1;
  unsigned int        fgNR_HSYNC_POLAR        : 1;
  unsigned int        fgNR_FIELD_POLAR        : 1;
  unsigned int        fgNR_DE_SELF            : 1;
  unsigned int        fgNR_ODD_V_SRT_OPT  : 1;
  unsigned int        fgNR_ODD_V_END_OPT  : 1;
  unsigned int        fgNR_EVEN_V_SRT_OPT : 1;
  unsigned int        fgNR_EVEN_V_END_OPT : 1;

  /* DWORD - 088 */
  unsigned int        u2NR_HOR_END            : 12;
  unsigned int                                : 4;
  unsigned int        u2NR_HOR_SRT            : 12;
  unsigned int                                : 4;


  /* DWORD - 08C */
  unsigned int        u2NR_VO_END         : 11;
  unsigned int                                : 5;
  unsigned int        u2NR_VO_SRT         : 11;
  unsigned int                                : 5;

  /* DWORD - 090 */
  unsigned int        u2NR_VE_END         : 11;
  unsigned int                                : 5;
  unsigned int        u2NR_VE_SRT         : 11;
  unsigned int                                : 5;

  /* DWORD - 094 */
  unsigned int        u1HSYNWIDTH         : 8;
  unsigned int        u1VSYNWIDTH         : 5;
  unsigned int        fgHD_TP                 : 1;
  unsigned int        fgHD_ON             : 1;
  unsigned int        fgPRGS                  : 1;
  unsigned int                                : 1;
  unsigned int        fgPRGS_AUTOFLD      : 1;
  unsigned int        fgPRGS_INVFLD           : 1;
  unsigned int                                : 1;
  unsigned int        fgYUV_RST_OPT           : 1;
  unsigned int                                : 3;
  unsigned int        fgPRGS_FLD              : 1;
  unsigned int                                : 5;
  unsigned int        u1TVMODE                : 2;

  /* DWORD - 098 */
  unsigned int                                : 16;
  unsigned int        u1PF_ADV                : 4;
  unsigned int                                : 12;

  /* DWORD - 09C */
  unsigned int        u2PXLLEN                : 12;
  unsigned int                                : 20;

  /* DWORD - 0A0 */
  unsigned int        u2HACTEND               : 12;
  unsigned int                                : 4;
  unsigned int        u2HACTBGN               : 12;
  unsigned int                                : 4;

  /* DWORD - 0A4 */
  unsigned int        u2VOACTEND          : 11;
  unsigned int                                : 5;
  unsigned int        u2VOACTBGN          : 11;
  unsigned int                                : 1;
  unsigned int        u1HIDE_OST              : 4;

  /* DWORD - 0A8 */
  unsigned int        u2VEACTEND              : 11;
  unsigned int                                : 5;
  unsigned int        u2VEACTBGN              : 11;
  unsigned int                                : 1;
  unsigned int        u1HIDE_EST              : 4;

  /* DWORD - 0AC */
  unsigned int        fgVDO_EN                : 1;
  unsigned int        fgFMTM                  : 1;
  unsigned int                                : 1;
  unsigned int        fgHPOR                  : 1;
  unsigned int        fgVPOR                  : 1;
  unsigned int                                : 2;
  unsigned int        fgC_RST_SEL         : 1;
  unsigned int        u4PXLSEL                : 2;
  unsigned int        fgFTRST                 : 1;
  unsigned int        fgSHVSYN                : 1;
  unsigned int                                : 2;
  unsigned int        u1SYN_DEL               : 2;
  unsigned int                                : 3;
  unsigned int        fgUVSW                  : 1;
  unsigned int                                : 5;
  unsigned int        fgBLACK                 : 1;
  unsigned int                                : 1;
  unsigned int        fgPFOFF                 : 1;
  unsigned int        u1HW_OPT                : 4;

  /* DWORD - 0B0 */ //0x423B0
  unsigned int        fgHSON                  : 1;
  unsigned int        fgHSLR                  : 1;
  unsigned int        fgLPF_ON                : 1;
  unsigned int        fgLPF_SL                : 1;
  unsigned int        u1ED_YUV_ST         : 2;
  unsigned int        fgO_DIS                 : 1;
  unsigned int        fgHDLPF                 : 1;
  unsigned int        u1YACC_ST               : 4;
  unsigned int        u1CACC_ST               : 4;
  unsigned int        u2HSFACTOR              : 14;
  unsigned int        fgEVEN_FIR              : 1;
  unsigned int        fgPHASE_16              : 1;

  /* DWORD - 0B4 */
  unsigned int                                : 4;
  unsigned int        u1BIY                   : 4;
  unsigned int                                : 4;
  unsigned int        u1BICB                  : 4;
  unsigned int                                : 4;
  unsigned int        u1BICR                  : 4;
  unsigned int        fgPF2OFF                : 1;
  unsigned int        fgHIDE_L                : 1;
  unsigned int                                : 6;

  /* DWORD - 0B8 */
  unsigned int                                : 4;
  unsigned int        u1BGY                   : 4;
  unsigned int                                : 4;
  unsigned int        u1BGCB                  : 4;
  unsigned int                                : 4;
  unsigned int        u1BGCR                  : 4;
  unsigned int                                : 8;

  /* DWORD - 0BC */
  unsigned int        u1EDGE_RATIO            : 5;
  unsigned int        fgKNEE                  : 1;
  unsigned int        fgZCORE                 : 1;
  unsigned int        fgDNR                   : 1;
  unsigned int        u1CORE                  : 4;
  unsigned int        u1DOUT_CTL          : 3;
  unsigned int                                : 17;

  /* DWORD - 0C0 */
  unsigned int                                : 32;

  /* DWORD - 0C4 */
  unsigned int        u2C4Default             : 16;
  unsigned int        fgOLD_C_ACC         : 1;
  unsigned int                                : 3;
  unsigned int        fgTVE_ND                : 1;
  unsigned int                                : 3;
  unsigned int        u1FIRST_PXL_LEAD        : 8;

  /* DWORD - 0C8 */
  unsigned int                                : 32;

  /* DWORD - 0CC */
  unsigned int                                : 32;

// 0x423D0 0x423D4
  /* DWORD - 0D0 */
  unsigned int        u2V_TOTAL_MIX           : 12;
  unsigned int                                : 4;
  unsigned int        u2H_TOTAL_MIX           : 13;
  unsigned int                                : 2;
  unsigned int        fgADJ_T_MIX             : 1;

  /* DWORD - 0D4 */ // 0x423D4
  unsigned int        u2V_TOTAL               : 12;
  unsigned int                                : 4;
  unsigned int        u2H_TOTAL               : 13;
  unsigned int                                : 2;
  unsigned int        fgADJ_T                 : 1;

  /* DWORD - 0D8 */
  unsigned int        INC_END                 : 12;
  unsigned int        INC_START               : 12;
  unsigned int        INC_STEP                : 8;

  /* DWORD - 0DC */
  unsigned int      DEC_END                 : 12;
  unsigned int        DEC_START               : 12;
  unsigned int        INIT_FACTOR         : 8;


  /* DWORD - 0E0 */
  unsigned int                                : 32;

  /* DWORD - 0E4 */
  unsigned int        fgMULTI_RATIO           : 1;
  unsigned int        fgDIRECT                : 1;
  unsigned int        fgC_FMTRST_M2           : 1;
  unsigned int        fgC_CHK_A_SCA           : 1; //0x423E4[3] luma key option
  unsigned int                                : 1;
  unsigned int        fgFACT_PREC         : 1;
  unsigned int        fgNOT_PST_D2            : 1;
  unsigned int                                : 1;
  unsigned int        fgHD_C_FIR_EN           : 1;
  unsigned int        u1LPF_SEL               : 3;
  unsigned int        fgUsePhase32            : 1;
  unsigned int        fgHD_C_POS              : 1;
  unsigned int        fgRST_PHASE         : 1;
  unsigned int                                : 1;
  unsigned int        u1MULTI_EXT         : 4;
  unsigned int                                : 4;
  unsigned int        fgMVDO_4TAP_ONLY        : 1;
  unsigned int        fgMATCH_2T              : 1;
  unsigned int        fgOLD_NEXT_YC           : 1;
  unsigned int        fgNO_SUB_1              : 1;
  unsigned int        fgEN_FIRST_PXL_LEAD : 1;
  unsigned int                                : 3;

  /* DWORD - 0E8 */ //0x423E8
  unsigned int        u2HSYN_DELAY            : 12;
  unsigned int                                : 4;
  unsigned int        u2VSYN_DELAY            : 12;
  unsigned int                                : 3;
  unsigned int        fgADJ_FWD               : 1;

  /* DWORD - 0EC */
  unsigned int                                : 32;

  /* DWORD - 0F0 */
  unsigned int                                : 32;

  /* DWORD - 0F4 */
  unsigned int                                : 32;

  /* DWORD - 0F8 */
  unsigned int        fgCCONV_VP1         : 1;
  unsigned int        fgCC7TO6_VP1            : 1;
  unsigned int                                : 2;
  unsigned int        fgAverage422to444       : 1;
  unsigned int        fgOLD_CHROMA            : 1;
  unsigned int        fg235_TO_255_EN         : 1;
  unsigned int        fgDATA_235_255          : 1;
  unsigned int                                : 20;
  unsigned int     fgAdap_Chroma              : 1;//[28]
  unsigned int                                : 3;

  /* DWORD - 0FC */
  unsigned int        u1LUMA_KEY              : 12;
  unsigned int                                : 20;
} HAL_PMX_DISP_AUX_FIELD_T;

//12001c00
typedef struct _HAL_PMX_DISP_MAIN_Y_FIELD_T
{
	/* DWORD - 000 */
	unsigned int	      u4YHCOEF0 	      : 32;

	/* DWORD - 004 */
	unsigned int	      u4YHCOEF1 	      : 32;

	/* DWORD - 008 */
	unsigned int	      u4YHCOEF2 	      : 32;

	/* DWORD - 00C */
	unsigned int	      u4YHCOEF3 	      : 32;

	/* DWORD - 010 */
	unsigned int	      u4YHCOEF4 	      : 32;

	/* DWORD - 014 */
	unsigned int	      u4YHCOEF5 	      : 32;

	/* DWORD - 018 */
	unsigned int	      u4YHCOEF6 	      : 32;

	/* DWORD - 01C */
	unsigned int	      u4YHCOEF7 	      : 32;

	/* DWORD - 020 */
	unsigned int	      u4YHCOEF8 	      : 32;

	/* DWORD - 024 */
	unsigned int	      u4YHCOEF9 	      : 32;

	/* DWORD - 028 */
	unsigned int	      u4YHCOEF10	      : 32;

	/* DWORD - 02C */
	unsigned int	      u4YHCOEF11	      : 32;

	/* DWORD - 030 */
	unsigned int	      u4YHCOEF12	      : 32;

	/* DWORD - 034 */
	unsigned int	      u4YHCOEF13	      : 32;

	/* DWORD - 038 */
	unsigned int	      u4YHCOEF14	      : 32;

	/* DWORD - 03C */
	unsigned int	      u4YHCOEF15	      : 32;

	/* DWORD - 040 */
	unsigned int				      : 32;

	/* DWORD - 044 */
	unsigned int					      : 32;

	/* DWORD - 048 */
	unsigned int	      fgV1_Y		      : 1;
	unsigned int	      fgV1_C		      : 1;
	unsigned int	      fgV1_NO_SYM	      : 1;
	unsigned int	      fgV1_SYM_OPT	      : 1;
	unsigned int	      u1V1_Y_MSB	      : 2;
	unsigned int	      u1V1_C_MSB	      : 2;
	unsigned int				      : 2;
	unsigned int	      fgC_16_PHAEE	      : 1;
	unsigned int	      fgC_EVEN_FILTER	      : 1;
	unsigned int				      : 20;

	/* DWORD - 04C */
	unsigned int	      u4YHCOEF19	      : 32;

}HAL_PMX_DISP_MAIN_Y_FIELD_T;

//12001D00
typedef struct _HAL_PMX_DISP_MAIN_C_FIELD_T
{
	/* DWORD - 000 */
	unsigned int        u4CHCOEF0               : 32;

	/* DWORD - 004 */
	unsigned int        u4CHCOEF1               : 32;

	/* DWORD - 008 */
	unsigned int        u4CHCOEF2               : 32;

	/* DWORD - 00C */
	unsigned int        u4CHCOEF3               : 32;

	/* DWORD - 010 */
	unsigned int        u4CHCOEF4               : 32;

	/* DWORD - 014 */
	unsigned int        u4CHCOEF5               : 32;

	/* DWORD - 018 */
	unsigned int        u4CHCOEF6               : 32;

	/* DWORD - 01C */
	unsigned int        u4CHCOEF7               : 32;

	/* DWORD - 020 */
	unsigned int        u4CHCOEF8               : 32;

	/* DWORD - 024 */
	unsigned int        u4CHCOEF9               : 32;

	/* DWORD - 028 */
	unsigned int        u4CHCOEF10              : 32;

	/* DWORD - 02C */
	unsigned int        u4CHCOEF11              : 32;

	/* DWORD - 030 */
	unsigned int        u4CHCOEF12              : 32;

	/* DWORD - 034 */
	unsigned int        u4CHCOEF13              : 32;

	/* DWORD - 038 */
	unsigned int        u4CHCOEF14              : 32;

	/* DWORD - 03C */
	unsigned int        u4CHCOEF15              : 32;

	/* DWORD - 040 */
	unsigned int                                : 32;

	/* DWORD - 044 */
	unsigned int                                        : 32;

	/* DWORD - 048 */
	unsigned int                                : 32;

	/* DWORD - 04C */
	unsigned int        u4CHCOEF19              : 32;
}HAL_PMX_DISP_MAIN_C_FIELD_T;

typedef struct _PMX_HAL_MIX_FIELD_T
{
  /* DWORD - 000 */
  unsigned int        fgVIDEO_SRC_SEL                 : 1;
  unsigned int        fgVIDEO_MIX_EN                  : 1;
  unsigned int        fgOSD1_MIX_EN                   : 1;
  unsigned int        fgOSD2_MIX_EN                   : 1;
  unsigned int        fgOSD3_MIX_EN                   : 1;
  unsigned int        fgOSD4_MIX_EN      	      : 1;
  unsigned int        				      : 1;
  unsigned int                                        : 1;
  unsigned int        u4MIX_LAYER1_SEL                : 3;
  unsigned int        fgDST_SEL_1                     : 1;
  unsigned int        u4MIX_LAYER2_SEL                : 3;
  unsigned int        fgDST_SEL_2                     : 1;
  unsigned int        u4MIX_LAYER3_SEL                : 3;
  unsigned int        fgDST_SEL_3                     : 1;
  unsigned int        u4MIX_LAYER4_SEL                : 3;
  unsigned int        fgDST_SEL_4                     : 1;
  unsigned int        u4MIX_LAYER5_SEL                : 3;
  unsigned int        fgDST_SEL_5                     : 1;
  unsigned int                                        : 2;
  unsigned int        fgVIDEO_TIMING_SEL              : 1;
  unsigned int        fgVIDEO_A_ADJ                   : 1;

  /* DWORD - 004 */
  unsigned int        u4VIDEO_A_IN_RANGE              : 8;
  unsigned int                                        : 8;
  unsigned int        u4VIDEO_A_OUT_RANGE             : 8;
  unsigned int                                        : 8;

  /* DWORD - 008*/
  unsigned int        fgOSD_SRC_SE                    : 1;
  unsigned int        fgOSD_SYNC_FLD_P                : 1;
  unsigned int        fgOSD_SYNC_H_P                  : 1;
  unsigned int        fgOSD_SYNC_V_P                  : 1;
  unsigned int        fgOSD_AUX_SRC_SE                : 1;
  unsigned int        fgOSD_AUX_SYNC_FLD_P            : 1;
  unsigned int        fgOSD_AUX_SYNC_H_P              : 1;
  unsigned int        fgOSD_AUX_SYNC_V_P              : 1;
  unsigned int        fgOSD_R_SRC_SE                  : 1;
  unsigned int        fgOSD_R_SYNC_FLD_P              : 1;
  unsigned int        fgOSD_R_SYNC_H_P                : 1;
  unsigned int        fgOSD_R_SYNC_V_P                : 1;
  unsigned int                                        : 20;

  /* DWORD - 00C*/
  unsigned int        fgINIT_CRC_OSD1                 : 1;
  unsigned int        fgINIT_CRC_OSD2                 : 1;
  unsigned int        fgINIT_CRC_OSD3                 : 1;
  unsigned int        fgINIT_CRC_OSD4                 : 1;
  unsigned int        fgINIT_CRC_OSD5                 : 1;
  unsigned int        fgINIT_CRC_OSD6                 : 1;
  unsigned int        fgINIT_CRC_OSD7                 : 1;
  unsigned int        fgINIT_CRC_FMT_F                : 1;
  unsigned int        fgINIT_CRC_FMT_R                : 1;
  unsigned int        fgINIT_CRC_VDO_F                : 1;
  unsigned int        fgINIT_CRC_VDO_R                : 1;
  unsigned int        fgINIT_CRC_SCL                  : 1;
  unsigned int        fgINIT_CRC_FMT                  : 1;
  unsigned int        fgINIT_CRC_FPD                  : 1;
  unsigned int                                        : 1;
  unsigned int        fgCLR_CRC_OSD1                  : 1;
  unsigned int        fgCLR_CRC_OSD2                  : 1;
  unsigned int        fgCLR_CRC_OSD3                  : 1;
  unsigned int        fgCLR_CRC_OSD4                  : 1;
  unsigned int        fgCLR_CRC_OSD5                  : 1;
  unsigned int        fgCLR_CRC_OSD6                  : 1;
  unsigned int        fgCLR_CRC_OSD7                  : 1;
  unsigned int        fgCLR_CRC_FMT_F                 : 1;
  unsigned int        fgCLR_CRC_FMT_R                 : 1;
  unsigned int        fgCLR_CRC_VDO_F                 : 1;
  unsigned int        fgCLR_CRC_VDO_R                 : 1;
  unsigned int        fgCLR_CRC_SCL                   : 1;
  unsigned int        fgCLR_CRC_FMT                   : 1;
  unsigned int        fgCLR_CRC_FPD                   : 1;
  unsigned int                                        : 1;
}PMX_HAL_MIX_FIELD_T;

typedef union _HAL_PMX_DISP_MAIN_UNION_T
{
  unsigned int              au4Reg[HAL_PMX_DISP_MAIN_REG_NUM];
  HAL_PMX_DISP_MAIN_FIELD_T   rField;
} HAL_PMX_DISP_MAIN_UNION_T;

typedef union _HAL_PMX_DISP_AUX_UNION_T
{
  unsigned int              au4Reg[HAL_PMX_DISP_AUX_REG_NUM];
  HAL_PMX_DISP_AUX_FIELD_T   rField;
} HAL_PMX_DISP_AUX_UNION_T;

typedef union _HAL_PMX_DISP_MAIN_Y_UNION_T
{
  unsigned int 		    au4Reg[HAL_PMX_DISP_MAIN_Y_REG_NUM];
  HAL_PMX_DISP_MAIN_Y_FIELD_T rField;
}HAL_PMX_DISP_MAIN_Y_UNION_T;

typedef union _HAL_PMX_DISP_MAIN_C_UNION_T
{
  unsigned int              au4Reg[HAL_PMX_DISP_MAIN_C_REG_NUM];
  HAL_PMX_DISP_MAIN_C_FIELD_T   rField;
} HAL_PMX_DISP_MAIN_C_UNION_T;

typedef union _PMX_HAL_MIX_UNION_T
{
  unsigned int              au4Reg[PMX_HAL_MIX_REG_NUM];
  PMX_HAL_MIX_FIELD_T    rField;
} PMX_HAL_MIX_UNION_T;


extern UCHAR _rPmxDispMainRegMode[HAL_PMX_DISP_MAIN_REG_NUM];
extern UCHAR _rPmxDispAuxRegMode[HAL_PMX_DISP_AUX_REG_NUM];
//extern UCHAR _rPmxMixRegMode[PMX_HAL_MIX_REG_NUM]; //now have not mix reg mode?

extern HAL_PMX_DISP_MAIN_UNION_T _rPmxDispMainSwReg;
extern HAL_PMX_DISP_MAIN_UNION_T  _rPmxDispAuxSwReg;
extern PMX_HAL_MIX_UNION_T   _rPmxHalMixSwReg;
extern volatile HAL_PMX_DISP_MAIN_UNION_T*  _prPmxDispMainHwReg;// = (HAL_PMX_DISP_MAIN_UNION_T*)HAL_PMX_DISP_MAIN_REG;
extern volatile HAL_PMX_DISP_MAIN_UNION_T*  _prPmxDispAuxHwReg;// = (HAL_PMX_DISP_MAIN_UNION_T*)HAL_PMX_DISP_AUX_REG;

extern volatile HAL_PMX_DISP_MAIN_Y_UNION_T*  _prPmxDispMainYHwReg;// = (HAL_PMX_DISP_MAIN_C_UNION_T*)HAL_PMX_DISP_MAIN_C_REG;
extern volatile HAL_PMX_DISP_MAIN_C_UNION_T*  _prPmxDispMainCHwReg;// = (HAL_PMX_DISP_MAIN_C_UNION_T*)HAL_PMX_DISP_MAIN_C_REG;

extern volatile HAL_PMX_DISP_MAIN_C_UNION_T*  _prPmxDispAuxCHwReg;// = (HAL_PMX_DISP_MAIN_C_UNION_T*)HAL_PMX_DISP_AUX_C_REG;

#endif  /* _PMX_HW_H_*/



