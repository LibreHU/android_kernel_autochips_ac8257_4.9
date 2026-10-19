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
#ifndef _DRV_PMX_IF_H_
#define _DRV_PMX_IF_H_

//#include "x_typedef.h"
//#include "drv_config.h" //ALTHD
//#include "x_plane_mxr.h"

#if 1 //from 3365 verify
/*  drv_av_d_rel.h */
typedef  enum
{
    RES_480I=0,
    RES_576I, //1
    RES_480P, //2
    RES_576P, //3
    RES_480P_1440,//4
    RES_576P_1440,//5
    RES_480P_2880,//6
    RES_576P_2880,//7
    RES_720P60HZ,//8
    RES_720P50HZ,//9
    RES_1080I60HZ,//10
    RES_1080I50HZ,//11
    RES_1080P60HZ,//12
    RES_1080P50HZ,//13
    RES_1080P30HZ,//14
    RES_1080P25HZ, //15
    RES_480I_2880,//16
    RES_576I_2880,//17
    RES_1080P24HZ, //18
    RES_1080P23_976HZ, //19, 1080P23.976hz
    RES_1080P29_97HZ, //20, 1080P29.97hz
    RES_3D_1080P23HZ, //21, 1080p47.952Hz
    RES_3D_1080P24HZ, //22, 1080p48hz
    RES_3D_720P60HZ, //23, 720p120hz
    RES_3D_720P50HZ, //24, 720p100hz
    RES_3D_720P30HZ, //25, 720p120hz
    RES_3D_720P25HZ, //26, 720p100hz
    RES_3D_576P50HZ, //27, 576p100hz
    RES_3D_480P60HZ, //28, 480p120hz
    RES_3D_1080I60HZ, //29, 1080i120hz
    RES_3D_1080I50HZ, //30, 1080i100hz
    RES_3D_1080I30HZ, //31, 1080i120hz
    RES_3D_1080I25HZ, //32, 1080i100hz
    RES_3D_576I25HZ, //33, 576i100hz
    RES_3D_480I30HZ, //34, 480i120hz
    RES_3D_576I50HZ, //35, 576i100hz
    RES_3D_480I60HZ, //36, 480i120hz
    RES_2D_480I60HZ, //37, 480i60hz
    RES_2D_576I50HZ, //38, 576i50hz
    RES_2D_640x480HZ,//39
    RES_PANEL_AUO_B089AW01, //40 //Total: 1344x625, Act: 1024x600, Frm: 60Hz, Clk: 50.4MHz
    RES_3D_720P60HZ_TB,  //41
    RES_3D_720P50HZ_TB,  //42
    RES_3D_1080I60HZ_SBS_HALF,//43
    RES_3D_1080I50HZ_SBS_HALF,//44
    RES_3D_1080P23HZ_TB, //45
    RES_3D_1080P24HZ_TB,//46

    RES_2160P_23_976HZ, //47
    RES_2160P_24HZ, //48
    RES_2160P_25HZ, //49
    RES_2160P_29_97HZ, //50
    RES_2160P_30HZ,  //51
    RES_2161P_24HZ,  //52


    RES_720P30HZ, // 53
    RES_720P25HZ, // 54
    RES_720P24HZ, // 55
    RES_720P23HZ, // 56
//3D frame packet
    RES_3D_1080P60HZ,// 57
    RES_3D_1080P50HZ,//58
    RES_3D_1080P30HZ,//59
    RES_3D_1080P29HZ,//60
    RES_3D_1080P25HZ,//61
    RES_3D_720P24HZ, //62
    RES_3D_720P23HZ, //63
//3D Top and bottom
    RES_3D_1080P60HZ_TB, //64
    RES_3D_1080P50HZ_TB, //65
    RES_3D_1080P30HZ_TB, //66
    RES_3D_1080P29HZ_TB,//67
    RES_3D_1080P25HZ_TB, //68
    RES_3D_1080I60HZ_TB, //69
    RES_3D_1080I50HZ_TB,//70
    RES_3D_1080I30HZ_TB,//71
    RES_3D_1080I25HZ_TB,//72
    RES_3D_720P30HZ_TB,//73
    RES_3D_720P25HZ_TB,//74
    RES_3D_720P24HZ_TB,//75
    RES_3D_720P23HZ_TB,//76
    RES_3D_576P50HZ_TB,//77
    RES_3D_576I25HZ_TB,//78
    RES_3D_576I50HZ_TB,//79
    RES_3D_480P60HZ_TB,//80
    RES_3D_480I30HZ_TB,//81
    RES_3D_480I60HZ_TB,//82
//3D Side by Side half
    RES_3D_1080P60HZ_SBS_HALF,//83
    RES_3D_1080P50HZ_SBS_HALF,//84
    RES_3D_1080P30HZ_SBS_HALF,//85
    RES_3D_1080P29HZ_SBS_HALF,//86
    RES_3D_1080P25HZ_SBS_HALF,//87
    RES_3D_1080P24HZ_SBS_HALF,//88
    RES_3D_1080P23HZ_SBS_HALF,//89
    RES_3D_1080I30HZ_SBS_HALF,//90
    RES_3D_1080I25HZ_SBS_HALF,//91
    RES_3D_720P60HZ_SBS_HALF,//92
    RES_3D_720P50HZ_SBS_HALF,//93
    RES_3D_720P30HZ_SBS_HALF,//94
    RES_3D_720P25HZ_SBS_HALF,//95
    RES_3D_720P24HZ_SBS_HALF,//96
    RES_3D_720P23HZ_SBS_HALF,//97
    RES_3D_576P50HZ_SBS_HALF,//98
    RES_3D_576I25HZ_SBS_HALF,//99
    RES_3D_576I50HZ_SBS_HALF,//100
    RES_3D_480P60HZ_SBS_HALF,//101
    RES_3D_480I30HZ_SBS_HALF,//102
    RES_3D_480I60HZ_SBS_HALF,//103

    RES_720I,               //104

    //add@jgao
    RES_480P_800 =4,//4
    RES_600P_800,
    RES_600P_1024,      //[xzr]add for fb_dev.c build error
    RES_768P_1024,
    RES_720P_1280,      //[xzr]add for fb_dev.c build error
    RES_800P_1280,

    RES_2160P =40,
    RES_480P_800_50HZ,//4
    RES_600P_800_50HZ,
    RES_600P_1024_50HZ,
    RES_720P_1280_50HZ, //[xzr]add for fb_dev.c build error
    RES_800P_1280_50HZ, //[xzr]add for fb_dev.c build error
    RES_MODE_NUM,          // dummy mode, used to determine the last mode
    RES_AUTO
}   PMX_RESOLUTION_MODE_T;
#else
typedef  enum
{
    RES_480I=0,
    RES_576I,
    RES_480P,
    RES_576P,
    RES_480P_800,  // 4
    RES_600P_800,  // 5
    RES_600P_1024, // 6
    RES_720P_1280, // 7
    RES_800P_1280, // 8
    RES_768P_1024, // 9
    RES_576P_1440, // 10
    RES_480P_2880,//6
    RES_576P_2880,//7
    RES_720P60HZ,//8
    RES_720P50HZ,//9
    RES_1080I60HZ,//10
    RES_1080I50HZ,//11
    RES_1080P60HZ,//12
    RES_1080P50HZ,//13
    RES_1080P30HZ,//14
    RES_1080P25HZ, //15
    RES_480I_2880,//16
    RES_576I_2880,//17
    RES_1080P24HZ, //18
    RES_1080P23_976HZ, //19, 1080P23.976hz
    RES_1080P29_97HZ, //20, 1080P29.97hz
    RES_3D_1080P23HZ, //21, 1080p47.952Hz
    RES_3D_1080P24HZ, //22, 1080p48hz
    RES_3D_720P60HZ, //23, 720p120hz
    RES_3D_720P50HZ, //24, 720p100hz
    RES_3D_720P30HZ, //25, 720p120hz
    RES_3D_720P25HZ, //26, 720p100hz
    RES_3D_576P50HZ, //27, 576p100hz
    RES_3D_480P60HZ, //28, 480p120hz
    RES_3D_1080I60HZ, //29, 1080i120hz
    RES_3D_1080I50HZ, //30, 1080i100hz
    RES_3D_1080I30HZ, //31, 1080i120hz
    RES_3D_1080I25HZ, //32, 1080i100hz
    RES_3D_576I25HZ, //33, 576i100hz
    RES_3D_480I30HZ, //34, 480i120hz
    RES_3D_576I50HZ, //35, 576i100hz
    RES_3D_480I60HZ, //36, 480i120hz
    RES_2D_480I60HZ, //37, 480i60hz
    RES_2D_576I50HZ, //38, 576i50hz
    RES_PANEL_AUO_B089AW01, //Total: 1344x625, Act: 1024x600, Frm: 60Hz, Clk: 50.4MHz
    RES_2160P,
    RES_480P_800_50HZ,//4
    RES_600P_800_50HZ,
    RES_600P_1024_50HZ,
    RES_720P_1280_50HZ,
    RES_800P_1280_50HZ,
    RES_MODE_NUM,          // dummy mode, used to determine the last mode
    RES_AUTO
}   PMX_RESOLUTION_MODE_T;
#endif

// Maximum number of plane mixer
#define PMX_MAX_NS				2
#define PMX_1					0	// Main display
#define PMX_2					1	// Aux display

#define PMX_VDOUT_NS        2
#define PMX_VDOUT_1         0
#define PMX_VDOUT_2         1


// Pmx Debug log mode
#define PMX_DBG_SET_MW      0
#define PMX_DBG_GET_MW      1
#define PMX_DBG_VDP         2
#define PMX_DBG_INTERLACE   3
#define PMX_DBG_NOAH        4
#define PMX_DBG_REG         5
#define PMX_DBG_TEST        6
#define PMX_DBG_3D_L_R      7

// Plane mixer configuration return value
#define PMX_SET_ERROR			0
#define PMX_SET_OK				1

// drv_sys_only: enable shadow, no video seen.
#if 0 //!CONFIG_DRV_ONLY && !CONFIG_DRV_VERIFY_SUPPORT && !CONFIG_DRV_FPGA_BOARD
#define PMX_SHADOWENABLE
#endif
#define SHDENBREG               40
#define PMX_DSD_SUPPORT         0



#define COLOR_SPACE_INTEGRATION

#define  fgVideoIsNtsc(ucFmt)  ((ucFmt == RES_480I) || (ucFmt == RES_480P) ||\
	                                            (ucFmt == RES_480P_1440)|| (ucFmt == RES_480P_2880)||\
	                                            (ucFmt == RES_720P60HZ)|| (ucFmt == RES_1080I60HZ)||\
	                                            (ucFmt == RES_720P30HZ) || (ucFmt == RES_720P23HZ) ||\
	                                            (ucFmt == RES_1080P60HZ)|| /*(ucFmt == RES_1080P30HZ)||*/\
	                                            (ucFmt == RES_480I_2880) || (ucFmt == RES_1080P23_976HZ)||\
	                                            (ucFmt == RES_1080P29_97HZ)|| (ucFmt == RES_3D_1080P23HZ)||\
	                                            (ucFmt == RES_3D_1080P29HZ)|| (ucFmt == RES_3D_720P60HZ)||\
	                                            (ucFmt == RES_3D_1080P60HZ)||(ucFmt == RES_3D_1080P29HZ)||\
	                                            (ucFmt == RES_3D_720P30HZ)||(ucFmt == RES_3D_720P23HZ)||\
	                                            (ucFmt == RES_3D_480P60HZ)||\
	                                            (ucFmt == RES_3D_1080I60HZ)|| (ucFmt == RES_3D_1080I30HZ)||\
	                                            (ucFmt == RES_3D_480I30HZ)|| (ucFmt == RES_3D_480I60HZ)||\
	                                            (ucFmt == RES_2D_480I60HZ) || (ucFmt == RES_PANEL_AUO_B089AW01)||\
	                                            (ucFmt == RES_2D_640x480HZ) ||\
	                                            (ucFmt == RES_3D_720P60HZ_TB) || (ucFmt == RES_3D_1080I60HZ_SBS_HALF)||\
	                                            (ucFmt == RES_3D_720P30HZ_TB)||(ucFmt == RES_3D_720P23HZ_TB)||\
	                                            (ucFmt == RES_3D_1080P60HZ_TB)||(ucFmt == RES_3D_1080P29HZ_TB)||\
	                                            (ucFmt == RES_3D_1080I60HZ_TB)||(ucFmt == RES_3D_1080I30HZ_TB)||\
	                                            (ucFmt == RES_3D_720P60HZ_SBS_HALF)||(ucFmt == RES_3D_720P30HZ_SBS_HALF)||\
	                                            (ucFmt == RES_3D_1080P23HZ_TB)||(ucFmt == RES_3D_720P23HZ_SBS_HALF)||\
	                                            (ucFmt == RES_3D_1080I30HZ_SBS_HALF)||(ucFmt == RES_3D_1080P23HZ_SBS_HALF)||\
	                                            (ucFmt == RES_3D_1080P60HZ_SBS_HALF)||(ucFmt == RES_3D_1080P29HZ_SBS_HALF)||\
	                                            (ucFmt == RES_3D_1080P23HZ_TB) ||\
	                                            (ucFmt == RES_2160P_23_976HZ)|| (ucFmt == RES_2160P_29_97HZ))

#define fgFMTisHD(ucFmt) ((ucFmt == RES_720P60HZ) || (ucFmt == RES_720P50HZ) ||\
																						 (ucFmt == RES_720P30HZ) || (ucFmt == RES_720P25HZ) ||\
																						 (ucFmt == RES_720P24HZ) || (ucFmt == RES_720P23HZ) ||\
                                             (ucFmt == RES_1080I60HZ)|| (ucFmt == RES_1080I50HZ)||\
                                             (ucFmt == RES_1080P60HZ)|| (ucFmt == RES_1080P50HZ)||\
                                             (ucFmt == RES_1080P30HZ)|| (ucFmt == RES_1080P25HZ)||\
                                             (ucFmt == RES_1080P24HZ)|| (ucFmt == RES_1080P23_976HZ)||\
                                             (ucFmt == RES_3D_1080P60HZ)||(ucFmt == RES_3D_1080P50HZ)||\
                                             (ucFmt == RES_3D_1080P30HZ)||(ucFmt == RES_3D_1080P25HZ)||\
                                             (ucFmt == RES_3D_1080P23HZ)||(ucFmt == RES_3D_1080P24HZ)||\
                                             (ucFmt == RES_3D_720P60HZ)||(ucFmt == RES_3D_720P50HZ)||\
                                             (ucFmt == RES_3D_720P30HZ)||(ucFmt == RES_3D_720P25HZ)||\
                                             (ucFmt == RES_3D_720P24HZ)||(ucFmt == RES_3D_720P23HZ)||\
                                             (ucFmt == RES_3D_1080I60HZ)||(ucFmt == RES_3D_1080I50HZ)||\
                                             (ucFmt == RES_3D_1080I30HZ)||(ucFmt == RES_3D_1080I25HZ)||\
                                             (ucFmt == RES_PANEL_AUO_B089AW01)||\
                                             (ucFmt == RES_3D_720P60HZ_TB)||(ucFmt == RES_3D_720P50HZ_TB)||\
                                             (ucFmt == RES_3D_720P30HZ_TB)||(ucFmt == RES_3D_720P25HZ_TB)||\
                                             (ucFmt == RES_3D_720P24HZ_TB)||(ucFmt == RES_3D_720P23HZ_TB)||\
                                             (ucFmt == RES_3D_1080I60HZ_TB)||(ucFmt == RES_3D_1080I50HZ_TB)||\
                                             (ucFmt == RES_3D_1080I30HZ_TB)||(ucFmt == RES_3D_1080I25HZ_TB)||\
                                             (ucFmt == RES_3D_720P60HZ_SBS_HALF)||(ucFmt == RES_3D_720P50HZ_SBS_HALF)||\
                                             (ucFmt == RES_3D_720P30HZ_SBS_HALF)||(ucFmt == RES_3D_720P25HZ_SBS_HALF)||\
                                             (ucFmt == RES_3D_720P24HZ_SBS_HALF)||(ucFmt == RES_3D_720P23HZ_SBS_HALF)||\
                                             (ucFmt == RES_3D_1080I60HZ_SBS_HALF)||(ucFmt == RES_3D_1080I50HZ_SBS_HALF)||\
                                             (ucFmt == RES_3D_1080I30HZ_SBS_HALF)||(ucFmt == RES_3D_1080I25HZ_SBS_HALF)||\
																						 (ucFmt == RES_3D_1080P60HZ_SBS_HALF)||(ucFmt == RES_3D_1080P50HZ_SBS_HALF)||\
																						 (ucFmt == RES_3D_1080P30HZ_SBS_HALF)||(ucFmt == RES_3D_1080P25HZ_SBS_HALF)||\
																						 (ucFmt == RES_3D_1080P24HZ_SBS_HALF)||(ucFmt == RES_3D_1080P23HZ_SBS_HALF)||\
                                             (ucFmt == RES_3D_1080P60HZ_TB)||(ucFmt == RES_3D_1080P50HZ_TB)||\
                                             (ucFmt == RES_3D_1080P30HZ_TB)||(ucFmt == RES_3D_1080P25HZ_TB)||\
                                             (ucFmt == RES_3D_1080P23HZ_TB)||(ucFmt == RES_3D_1080P24HZ_TB) ||\
                                             (ucFmt == RES_2160P_23_976HZ)||(ucFmt == RES_2160P_24HZ)||\
                                             (ucFmt == RES_2160P_25HZ)||(ucFmt == RES_2160P_29_97HZ)||\
                                             (ucFmt == RES_2160P_30HZ)||(ucFmt == RES_2161P_24HZ))

//shm
#define fgFMTis3D(ucFmt)	((ucFmt == RES_3D_1080P23HZ)||(ucFmt == RES_3D_1080P24HZ)||\
                                             (ucFmt == RES_3D_720P60HZ)||(ucFmt == RES_3D_720P50HZ)||\
                                             (ucFmt == RES_3D_720P30HZ)||(ucFmt == RES_3D_720P25HZ)||\
                                             (ucFmt == RES_3D_720P24HZ)||(ucFmt == RES_3D_720P23HZ)||\
                                             (ucFmt == RES_3D_1080P60HZ)||(ucFmt == RES_3D_1080P50HZ)||\
  																					 (ucFmt == RES_3D_1080P30HZ)||(ucFmt == RES_3D_1080P25HZ)||\
                                             (ucFmt == RES_3D_1080I60HZ)||(ucFmt == RES_3D_1080I50HZ)||\
                                             (ucFmt == RES_3D_1080I30HZ)||(ucFmt == RES_3D_1080I25HZ))

#define fgFMTis4k2k(ucFmt) ((ucFmt == RES_2160P_23_976HZ)||(ucFmt == RES_2160P_24HZ)||\
                                             (ucFmt == RES_2160P_25HZ)||(ucFmt == RES_2160P_29_97HZ)||\
                                             (ucFmt == RES_2160P_30HZ)||(ucFmt == RES_2161P_24HZ))


#define fgFMTisPalSD(ucFmt) (!(fgVideoIsNtsc(ucFmt)) && !(fgFMTisHD(ucFmt)))


#define fgIsHDRes(u1Res) ((u1Res == RES_720P30HZ)||(u1Res == RES_720P25HZ)||(u1Res == RES_720P24HZ)||\
                                           (u1Res == RES_720P23HZ)||(u1Res == RES_720P60HZ)||(u1Res == RES_720P50HZ)||\
                                           (u1Res == RES_1080I60HZ)||(u1Res == RES_1080I50HZ)||(u1Res == RES_1080P60HZ)||\
                                           (u1Res == RES_1080P50HZ)||(u1Res == RES_1080P30HZ)||(u1Res == RES_1080P25HZ)||\
                                           (u1Res == RES_1080P24HZ)||(u1Res == RES_1080P23_976HZ)||(u1Res == RES_1080P29_97HZ) ||\
                                           (u1Res == RES_3D_1080P60HZ) ||(u1Res == RES_3D_1080P50HZ) ||(u1Res == RES_3D_1080P30HZ) ||\
                                           (u1Res == RES_3D_1080P29HZ) ||(u1Res == RES_3D_1080P25HZ) ||(u1Res == RES_3D_1080P24HZ) ||\
                                           (u1Res == RES_3D_1080P23HZ) ||(u1Res == RES_3D_720P60HZ) ||(u1Res == RES_3D_720P50HZ) ||\
                                           (u1Res == RES_3D_720P30HZ) ||(u1Res == RES_3D_720P25HZ) ||(u1Res == RES_3D_720P24HZ) ||\
                                           (u1Res == RES_3D_720P23HZ) ||(u1Res == RES_3D_1080I60HZ) ||(u1Res == RES_3D_1080I50HZ) ||\
                                           (u1Res == RES_3D_1080I30HZ) ||(u1Res == RES_3D_1080I25HZ)||\
                                           (u1Res == RES_3D_1080P60HZ_TB) ||(u1Res == RES_3D_1080P50HZ_TB) ||(u1Res == RES_3D_1080P30HZ_TB) ||\
                                           (u1Res == RES_3D_1080P29HZ_TB) ||(u1Res == RES_3D_1080P25HZ_TB) ||(u1Res == RES_3D_1080P24HZ_TB) ||\
                                           (u1Res == RES_3D_1080P23HZ_TB) ||(u1Res == RES_3D_720P60HZ_TB) ||(u1Res == RES_3D_720P50HZ_TB) ||\
                                           (u1Res == RES_3D_720P30HZ_TB) ||(u1Res == RES_3D_720P25HZ_TB) ||(u1Res == RES_3D_720P24HZ_TB) ||\
                                           (u1Res == RES_3D_720P23HZ_TB) ||(u1Res == RES_3D_1080I60HZ_TB) ||(u1Res == RES_3D_1080I50HZ_TB) ||\
                                           (u1Res == RES_3D_1080I30HZ_TB) ||(u1Res == RES_3D_1080I25HZ_TB)||\
                                           (u1Res == RES_3D_1080P60HZ_SBS_HALF) ||(u1Res == RES_3D_1080P50HZ_SBS_HALF) ||(u1Res == RES_3D_1080P30HZ_SBS_HALF) ||\
                                           (u1Res == RES_3D_1080P29HZ_SBS_HALF) ||(u1Res == RES_3D_1080P25HZ_SBS_HALF) ||(u1Res == RES_3D_1080P24HZ_SBS_HALF) ||\
                                           (u1Res == RES_3D_1080P23HZ_SBS_HALF) ||(u1Res == RES_3D_720P60HZ_SBS_HALF) ||(u1Res == RES_3D_720P50HZ_SBS_HALF) ||\
                                           (u1Res == RES_3D_720P30HZ_SBS_HALF) ||(u1Res == RES_3D_720P25HZ_SBS_HALF) ||(u1Res == RES_3D_720P24HZ_SBS_HALF) ||\
                                           (u1Res == RES_3D_720P23HZ_SBS_HALF) ||(u1Res == RES_3D_1080I60HZ_SBS_HALF) ||(u1Res == RES_3D_1080I50HZ_SBS_HALF) ||\
                                           (u1Res == RES_3D_1080I30HZ_SBS_HALF) ||(u1Res == RES_3D_1080I25HZ_SBS_HALF)||\
                                           (u1Res == RES_2160P_24HZ) ||(u1Res == RES_2160P_25HZ) ||(u1Res == RES_2160P_30HZ)||(u1Res == RES_2161P_24HZ)||(u1Res == RES_2160P_23_976HZ)||(u1Res == RES_2160P_29_97HZ))

#define fgIsFullHDRes(u1Res) ((u1Res == RES_1080I60HZ)||(u1Res == RES_1080I50HZ)||(u1Res == RES_1080P60HZ)||\
                                           (u1Res == RES_1080P50HZ)||(u1Res == RES_1080P30HZ)||(u1Res == RES_1080P25HZ)||\
                                           (u1Res == RES_1080P24HZ)||(u1Res == RES_1080P23_976HZ)||(u1Res == RES_1080P29_97HZ) ||\
                                           (u1Res == RES_3D_1080P60HZ) ||(u1Res == RES_3D_1080P50HZ) ||(u1Res == RES_3D_1080P30HZ) ||\
                                           (u1Res == RES_3D_1080P29HZ) ||(u1Res == RES_3D_1080P25HZ) ||(u1Res == RES_3D_1080P24HZ) ||\
                                           (u1Res == RES_3D_1080P23HZ) ||(u1Res == RES_3D_1080I60HZ) ||(u1Res == RES_3D_1080I50HZ) ||\
                                           (u1Res == RES_3D_1080I30HZ) ||(u1Res == RES_3D_1080I25HZ)||\
                                           (u1Res == RES_3D_1080P60HZ_TB) ||(u1Res == RES_3D_1080P50HZ_TB) ||(u1Res == RES_3D_1080P30HZ_TB) ||\
                                           (u1Res == RES_3D_1080P29HZ_TB) ||(u1Res == RES_3D_1080P25HZ_TB) ||(u1Res == RES_3D_1080P24HZ_TB) ||\
                                           (u1Res == RES_3D_1080P23HZ_TB) ||(u1Res == RES_3D_1080I60HZ_TB) ||(u1Res == RES_3D_1080I50HZ_TB) ||\
                                           (u1Res == RES_3D_1080I30HZ_TB) ||(u1Res == RES_3D_1080I25HZ_TB)||\
                                           (u1Res == RES_3D_1080P60HZ_SBS_HALF) ||(u1Res == RES_3D_1080P50HZ_SBS_HALF) ||(u1Res == RES_3D_1080P30HZ_SBS_HALF) ||\
                                           (u1Res == RES_3D_1080P29HZ_SBS_HALF) ||(u1Res == RES_3D_1080P25HZ_SBS_HALF) ||(u1Res == RES_3D_1080P24HZ_SBS_HALF) ||\
                                           (u1Res == RES_3D_1080P23HZ_SBS_HALF) ||(u1Res == RES_3D_1080I60HZ_SBS_HALF) ||(u1Res == RES_3D_1080I50HZ_SBS_HALF) ||\
                                           (u1Res == RES_3D_1080I30HZ_SBS_HALF) ||(u1Res == RES_3D_1080I25HZ_SBS_HALF)||\
                                           (u1Res == RES_2160P_24HZ) ||(u1Res == RES_2160P_25HZ) ||(u1Res == RES_2160P_30HZ)||(u1Res == RES_2161P_24HZ)||(u1Res == RES_2160P_23_976HZ)||(u1Res == RES_2160P_29_97HZ))

#define fgIsTrueInterlaceSupport(u1Res) ((u1Res == RES_2D_480I60HZ)||(u1Res == RES_2D_576I50HZ)||(u1Res == RES_3D_1080I60HZ) || (u1Res == RES_3D_1080I50HZ) ||\
	                                                              (u1Res == RES_3D_1080I30HZ) ||(u1Res == RES_3D_1080I25HZ) ||(u1Res == RES_3D_576I25HZ) || (u1Res == RES_3D_480I30HZ) ||\
	                                                              (u1Res == RES_3D_576I50HZ) || (u1Res == RES_3D_480I60HZ))

// Plane mixer TV type
#define PMX_TV_TYPE_NTSC		         0
#define PMX_TV_TYPE_PAL_M		         1
#define PMX_TV_TYPE_PAL_N		         2
#define PMX_TV_TYPE_PAL			         3
#define PMX_TV_TYPE_PAL_1080P_24		 4
#define PMX_TV_TYPE_PAL_1080P_25		 5
#define PMX_TV_TYPE_PAL_1080P_30		 6
#define PMX_TV_TYPE_NTSC_1080P_23_9		 7 //1080P23.976hz
#define PMX_TV_TYPE_NTSC_1080P_29_9	     8//1080P29.97hz
#define PMX_TV_TYPE_720P3D	             9
#define PMX_TV_TYPE_1080i3D	             10
#define PMX_TV_TYPE_800X480              11
#define PMX_TV_TYPE_800X600              12
#define PMX_TV_TYPE_1024X600             13

//Vdo Src Type
#define PMX_VDO_720X480        0
#define PMX_VDO_800X480        1
#define PMX_VDO_800X600        0
#define PMX_VDO_1024X600       1


// Resolution
#define PMX_MODE_480I_WIDTH		720
#define PMX_MODE_480I_HEIGHT	480
#define PMX_MODE_480P_WIDTH		720
#define PMX_MODE_480P_HEIGHT	480
#define PMX_MODE_576I_WIDTH		720
#define PMX_MODE_576I_HEIGHT	576
#define PMX_MODE_576P_WIDTH		720
#define PMX_MODE_576P_HEIGHT	576
#define PMX_MODE_720P_WIDTH		1280
#define PMX_MODE_720P_HEIGHT	720
#define PMX_MODE_1080I_WIDTH	1920
#define PMX_MODE_1080I_HEIGHT	1080
#define PMX_MODE_1080P_WIDTH	1920
#define PMX_MODE_1080P_HEIGHT	1080
#define PMX_MODE_2160P_WIDTH  3840
#define PMX_MODE_2160P_HEIGHT	2160
#define PMX_MODE_2161P_WIDTH  4096
#define PMX_MODE_2161P_HEIGHT	2160


#define PMX_MODE_768P_WIDTH		1366
#define PMX_MODE_768P_HEIGHT	768
#define PMX_MODE_540P_WIDTH		1920
#define PMX_MODE_540P_HEIGHT	540
#define PMX_MODE_768I_WIDTH		1366
#define PMX_MODE_768I_HEIGHT	768
#define PMX_MODE_1536I_WIDTH	1366
#define PMX_MODE_1536I_HEIGHT	1536
#define PMX_MODE_720I_WIDTH		1280
#define PMX_MODE_720I_HEIGHT	720
#define PMX_MODE_1440I_WIDTH	1280
#define PMX_MODE_1440I_HEIGHT	1440

#define PMX_MODE_PANEL_AUO_B089AW01_WIDTH  1024
#define PMX_MODE_PANEL_AUO_B089AW01_HEIGHT  600

#define PMX_MODE_640_480_WIDTH	640
#define PMX_MODE_640_480_HEIGHT	480

//For Sharpness
#define PMX_DISP_SHARPNESS_LOW   0
#define PMX_DISP_SHARPNESS_MIDDLE 1
#define PMX_DISP_SHARPNESS_HIGH 2

//For Digital video Output Fmt

/*
typedef enum
{
  DRV_PMX_COLOR_SPACE_YCBCR,
  DRV_PMX_COLOR_SPACE_RGB,
} DRV_PMX_COLOR_SPACE_T;
*/
typedef enum
{
  DRV_GFX_COLOR_MODE_YCBCR = 0,
  DRV_GFX_COLOR_MODE_RGB,
} DRV_GFX_COLOR_MODE_T;

/*
typedef enum
{
  DRV_PMX_CS_YCBCR_601,
  DRV_PMX_CS_YCBCR_709
} DRV_PMX_CS_YCBCR_T;
*/
typedef enum
{
  DRV_PMX_CS_YCBCR_601 = 0,
  DRV_PMX_CS_YCBCR_709,
  DRV_PMX_CS_RGB
} DRV_PMX_COLOR_SPACE_T;


typedef enum
{
  DRV_PMX_PLANE_1 = 0, //bottom
  DRV_PMX_PLANE_2,
  DRV_PMX_PLANE_3,
  DRV_PMX_PLANE_4,
  DRV_PMX_PLANE_5,
  DRV_PMX_PLANE_6,
  DRV_PMX_PLANE_7,
  DRV_PMX_PLANE_8     // top
} DRV_PMX_PLANE_T;


typedef enum
{
    PMX_OUTPUT_CCIR = 0,
    PMX_OUTPUT_HDMI,
    PMX_OUTPUT_CAV,
    PMX_OUTPUT_CVBS,
    PMX_OUTPUT_NUM
} PMX_OUTPUT_E;


#endif // _DRV_PMX_IF_H
