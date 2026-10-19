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
 *Department: ATC-SD-SD2
 *
 *Aurthor: yezhiqiang(atc0064)
 *
 *Date: 2017-07-31
 */
#ifndef _NR_REG_H_
#define _NR_REG_H_

#ifndef ATC_ARM2_LK_SUPPORT
#include <asm/io.h>
#else
#include <reg.h>
#endif

#define NR_REG_OFFSET           (nr_reg)//0x12003000
#define TNR_REG_BASE            0x1000//0x12004000

/// Noise Reduction register map
#define RW_NR_CURR_Y_RD_ADDR       0x000
#define RW_NR_CURR_C_RD_ADDR       0x004
#define RW_NR_HD_RANGE_MAP         0x008
#define RW_NR_HD_ACTIVE            0x01C
#define RW_NR_HD_HDE_RATIO         0x024
#define RW_NR_HD_LINE_OFST         0x028
#define RW_NR_HD_MODE_CTRL         0x02C
    #define BURST_READ             ((unsigned)0x1 << 23)
#define RW_NR_HD_SYNC_TRIGGER      0x030
#define RW_NR_HD_STATUS            0x03C
#define RW_NR_HD_PATH_ENABLE       0x040

#define RW_NR_MISC_CTRL            0x300
#define RW_NR_INT_CLR              0x30C
#define RW_NR_DRAM_CTRL_00         0x400
#define RW_NR_DRAM_CTRL_01         0x404
#define RW_NR_Y_WR_SADDR           0x410
#define RW_NR_Y_WR_EADDR           0x414
#define RW_NR_C_WR_SADDR           0x418
#define RW_NR_C_WR_EADDR           0x41C
//#define RW_NR_DRAM_CTRL_0A         0x428
//#define RW_NR_DRAM_CTRL_0C         0x430
//#define RW_NR_DRAM_CTRL_0E         0x438

#define RW_NR_MISC_CTRL_01         0x43C
	#define USE_HD_SRAM           ((unsigned)0x1 << 3)

#define RW_NR_DRAM_CTRL_10         0x440
	#define LAST_BURST_READ        ((unsigned)0x1 << 1)
#define RW_NR_TARGET_SIZE_CTRL     0x444
//#define RW_NR_MISC_CTRL_02         0x448
#define RW_NR_LAST_Y_RD_ADDR       0x450
#define RW_NR_LAST_C_RD_ADDR       0x458

#define RO_NR_WCHKSM_Y_00          0x500
#define RO_NR_WCHKSM_Y_01          0x504
#define RO_NR_WCHKSM_Y_02          0x508
#define RO_NR_WCHKSM_Y_03          0x50C
#define RO_NR_WCHKSM_Y_04          0x510
#define RO_NR_WCHKSM_Y_05          0x514
#define RO_NR_WCHKSM_Y_06          0x518
#define RO_NR_WCHKSM_Y_07          0x51C
#define RO_NR_WCHKSM_C_00          0x520
#define RO_NR_WCHKSM_C_01          0x524
#define RO_NR_WCHKSM_C_02          0x528
#define RO_NR_WCHKSM_C_03          0x52C
#define RO_NR_WCHKSM_C_04          0x530
#define RO_NR_WCHKSM_C_05          0x534
#define RO_NR_WCHKSM_C_06          0x538
#define RO_NR_WCHKSM_C_07          0x53C

#define RW_NR_MAIN_CTRL_00         0x800
#define RW_NR_MAIN_CTRL_01         0x804
#define RW_NR_CRC_SETTING          0x8A4

#define RW_NR_CRC_STATUS           0x9E4

#define RW_NR_2DNR_CTRL_00         0xD80
#define RW_NR_2DNR_CTRL_01         0xD84
#define RW_NR_2DNR_CTRL_02         0xD88
    #define SLICE_X_POSITION    ((unsigned)0x3FF << 16)
#define RW_NR_2DNR_CTRL_03         0xD8C
#define RW_NR_2DNR_CTRL_04         0xD90
#define RW_NR_2DNR_CTRL_05         0xD94
#define RW_NR_2DNR_CTRL_06         0xD98
#define RW_NR_2DNR_CTRL_07         0xD9C
#define RW_NR_2DNR_CTRL_08         0xDA0
#define RW_NR_2DNR_CTRL_09         0xDA4
#define RW_NR_2DNR_CTRL_0A         0xDA8
#define RW_NR_2DNR_CTRL_0B         0xDAC
#define RW_NR_2DNR_CTRL_0C         0xDB0
#define RW_NR_2DNR_CTRL_0D         0xDB4
#define RW_NR_2DNR_CTRL_0E         0xDB8
#define RW_NR_2DNR_CTRL_0F         0xDBC

#define RW_NR_2DNR_CTRL_10         0xDC0
#define RW_NR_2DNR_CTRL_11         0xDC4
#define RW_NR_2DNR_CTRL_12         0xDC8
#define RW_NR_2DNR_CTRL_13         0xDCC
#define RW_NR_2DNR_CTRL_14         0xDD0
#define RW_NR_2DNR_CTRL_15         0xDD4
#define RW_NR_2DNR_CTRL_16         0xDD8
#define RW_NR_2DNR_CTRL_17         0xDDC
#define RW_NR_2DNR_CTRL_18         0xDE0
#define RW_NR_2DNR_CTRL_19         0xDE4
#define RW_NR_2DNR_CTRL_1A         0xDE8
#define RW_NR_2DNR_CTRL_1B         0xDEC
#define RW_NR_2DNR_CTRL_1C         0xDF0
#define RW_NR_2DNR_CTRL_1D         0xDF4
#define RW_NR_2DNR_CTRL_1E         0xDF8
#define RW_NR_2DNR_CTRL_1F         0xE00
    #define SLICE_DEMO_CTRL      ((unsigned)0x1 << 13) // 1:left 0:right
    #define SLICE_DEMO_ENABLE    ((unsigned)0x1 << 12)
    #define BLOCK_METER_ENABLE   ((unsigned)0x1 << 8)
    #define BLOCK_PROC_ENABLE    ((unsigned)0x1 << 7)

#define RW_NR_2DNR_CTRL_20         0xE04
#define RW_NR_2DNR_CTRL_21         0xE08
#define RW_NR_2DNR_CTRL_22         0xE0C
#define RW_NR_2DNR_CTRL_23         0xE10
#define RW_NR_2DNR_CTRL_24         0xE14
#define RW_NR_2DNR_CTRL_25         0xE18
#define RW_NR_2DNR_CTRL_26         0xE1C
#define RW_NR_2DNR_CTRL_27         0xE20
#define RW_NR_2DNR_CTRL_28         0xE24
#define RW_NR_2DNR_CTRL_29         0xE28
#define RW_NR_2DNR_CTRL_2A         0xE2C
#define RW_NR_2DNR_CTRL_2B         0xE30
#define RW_NR_2DNR_CTRL_2C         0xE34
#define RW_NR_2DNR_CTRL_2D         0xE38
#define RW_NR_2DNR_CTRL_2E         0xE3C
#define RW_NR_2DNR_CTRL_2F         0xE40

#define RW_NR_2DNR_CTRL_30         0xE44
#define RW_NR_2DNR_CTRL_31         0xE48
#define RW_NR_2DNR_CTRL_32         0xE4C
#define RW_NR_2DNR_CTRL_33         0xE50
#define RW_NR_2DNR_CTRL_34         0xE54
#define RW_NR_2DNR_CTRL_35         0xE58
#define RW_NR_2DNR_CTRL_36         0xE5C
#define RW_NR_2DNR_CTRL_37         0xE60
    #define MESSSFT_SMOOTH_CO1MO    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_CO1MO    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_CO1MO    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_CO1MO    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_38         0xE64
    #define MESSSFT_MESS_CO1MO    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_CO1MO    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_CO1MO    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_CO1MO    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_39         0xE68
    #define MESSSFT_SMOOTH_CO1ST    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_CO1ST    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_CO1ST    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_CO1ST    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_3A         0xE6C
    #define MESSSFT_MESS_CO1ST    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_CO1ST    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_CO1ST    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_CO1ST    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_3B         0xE70
    #define MESSSFT_SMOOTH_CO2MO    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_CO2MO    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_CO2MO    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_CO2MO    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_3C         0xE74
    #define MESSSFT_MESS_CO2MO    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_CO2MO    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_CO2MO    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_CO2MO    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_3D         0xE78
    #define MESSSFT_SMOOTH_CO2ST    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_CO2ST    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_CO2ST    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_CO2ST    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_3E         0xE7C
    #define MESSSFT_MESS_CO2ST    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_CO2ST    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_CO2ST    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_CO2ST    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_3F         0xE80
    #define MESSSFT_SMOOTH_CO3MO    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_CO3MO    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_CO3MO    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_CO3MO    ((unsigned)0x3F << 0)

#define RW_NR_2DNR_CTRL_40         0xE84
    #define MESSSFT_MESS_CO3MO    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_CO3MO    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_CO3MO    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_CO3MO    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_41         0xE88
    #define MESSSFT_SMOOTH_CO3ST    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_CO3ST    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_CO3ST    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_CO3ST    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_42         0xE8C
    #define MESSSFT_MESS_CO3ST    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_CO3ST    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_CO3ST    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_CO3ST    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_43         0xE90
#define RW_NR_2DNR_CTRL_44         0xE94
#define RW_NR_2DNR_CTRL_45         0xE98
#define RW_NR_2DNR_CTRL_46         0xE9C
#define RW_NR_2DNR_CTRL_47         0xEA0
#define RW_NR_2DNR_CTRL_48         0xEA4
#define RW_NR_2DNR_CTRL_49         0xEA8
#define RW_NR_2DNR_CTRL_4A         0xEAC
#define RW_NR_2DNR_CTRL_4B         0xEB0
#define RW_NR_2DNR_CTRL_4C         0xEB4
#define RW_NR_2DNR_CTRL_4D         0xEB8
#define RW_NR_2DNR_CTRL_4E         0xEBC
#define RW_NR_2DNR_CTRL_4F         0xEC0
    #define MESSSFT_SMOOTH_FRST    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_FRST    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_FRST    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_FRST    ((unsigned)0x3F << 0)

#define RW_NR_2DNR_CTRL_50         0xEC4
    #define MESSSFT_MESS_FRST    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_FRST    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_FRST    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_FRST    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_51         0xEC8
    #define MESSSFT_SMOOTH_MO    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_MO    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_MO    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_MO    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_52         0xECC
    #define MESSSFT_MESS_MO    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_MO    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_MO    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_MO    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_53         0xED0
    #define MESSSFT_SMOOTH_ST    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_ST    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_ST    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_ST    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_54         0xED4
    #define MESSSFT_MESS_ST    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_ST    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_ST    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_ST    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_55         0xED8
    #define MESSSFT_SMOOTH_BK    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_BK    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_BK    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_BK    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_56         0xEDC
    #define MESSSFT_MESS_BK    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_BK    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_BK    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_BK    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_57         0xEE0
    #define MESSSFT_SMOOTH_DEF    ((unsigned)0x3F << 24)
    #define MESSTHL_SMOOTH_DEF    ((unsigned)0x3F << 16)
    #define MESSSFT_EDGE_DEF    ((unsigned)0x3F << 8)
    #define MESSTHL_EDGE_DEF    ((unsigned)0x3F << 0)
#define RW_NR_2DNR_CTRL_58         0xEE4
    #define MESSSFT_MESS_DEF    ((unsigned)0x3F << 24)
    #define MESSTHL_MESS_DEF    ((unsigned)0x3F << 16)
    #define MESSSFT_MOS_DEF    ((unsigned)0x3F << 8)
    #define MESSTHL_MOS_DEF    ((unsigned)0x3F << 0)

#define RW_NR_2DNR_CTRL_92         0xEF8
#define RW_NR_2DNR_CTRL_59         0xEFC
#define RW_NR_2DNR_CTRL_5A         0xF00
#define RW_NR_2DNR_CTRL_5B         0xF04
    #define BK_METER_WIDTH     ((unsigned)0x3FF << 16)
    #define BK_METER_HEIGHT    ((unsigned)0x7FF << 0)
#define RW_NR_2DNR_CTRL_5C         0xF08
#define RW_NR_2DNR_CTRL_5D         0xF0C
#define RW_NR_2DNR_CTRL_5E         0xF10
#define RW_NR_2DNR_CTRL_5F         0xF14

#define RW_NR_2DNR_CTRL_60         0xF18
#define RW_NR_2DNR_CTRL_61         0xF1C
#define RW_NR_2DNR_CTRL_62         0xF20
#define RW_NR_2DNR_CTRL_63         0xF24
#define RW_NR_2DNR_CTRL_64         0xF28
#define RW_NR_2DNR_CTRL_65         0xF2C
    #define MNR_SM_THR    ((unsigned)0xFF << 24)
    #define MNR_EDGE_THR    ((unsigned)0xFF << 16)
    #define SM_NUM_THR    ((unsigned)0xF << 8)
    #define NEAREDGE_SEL_WIDTH    ((unsigned)0xF << 0)
#define RW_NR_2DNR_CTRL_66         0xF30
#define RW_NR_2DNR_CTRL_67         0xF34
#define RW_NR_2DNR_CTRL_68         0xF38
#define RW_NR_2DNR_CTRL_69         0xF3C
#define RW_NR_2DNR_CTRL_6A         0xF40
#define RW_NR_2DNR_CTRL_6B         0xF44
#define RW_NR_2DNR_CTRL_6C         0xF48
#define RW_NR_2DNR_CTRL_6E         0xF50
    #define Y_GLOBAL_BLEND    ((unsigned)0xF << 28)
#define RW_NR_2DNR_CTRL_6F         0xF54

#define RW_NR_2DNR_CTRL_70         0xF58
#define RW_NR_2DNR_CTRL_71         0xF60
#define RW_NR_2DNR_CTRL_72         0xF64
#define RW_NR_2DNR_CTRL_73         0xF68
#define RW_NR_2DNR_CTRL_74         0xF6C
#define RW_NR_2DNR_CTRL_75         0xF70
#define RW_NR_2DNR_CTRL_76         0xF74
#define RW_NR_2DNR_CTRL_77         0xF78
#define RW_NR_2DNR_CTRL_78         0xF7C
#define RW_NR_2DNR_CTRL_79         0xF80
#define RW_NR_2DNR_CTRL_93         0xF84
#define RW_NR_2DNR_CTRL_94         0xF88
#define RW_NR_2DNR_CTRL_95         0xF8C
#define RW_NR_2DNR_CTRL_96         0xF90
#define RW_NR_2DNR_CTRL_97         0xF94
#define RW_NR_2DNR_CTRL_7E         0xF98
    #define BLDLV_BK_ST    ((unsigned)0xF << 28)
    #define BLDLV_SM_ST    ((unsigned)0xF << 24)
    #define BLDLV_MESS_ST    ((unsigned)0xF << 20)
    #define BLDLV_EDGE_ST    ((unsigned)0xF << 16)
    #define BLDLV_BK_DEF    ((unsigned)0xF << 12)
    #define BLDLV_SM_DEF    ((unsigned)0xF << 8)
    #define BLDLV_MESS_DEF    ((unsigned)0xF << 4)
    #define BLDLV_EDGE_DEF    ((unsigned)0xF << 0)
#define RW_NR_2DNR_CTRL_7F         0xF9C
    #define BLDLV_BK_BK    ((unsigned)0xF << 28)
    #define BLDLV_SM_BK    ((unsigned)0xF << 24)
    #define BLDLV_MESS_BK    ((unsigned)0xF << 20)
    #define BLDLV_EDGE_BK    ((unsigned)0xF << 16)
    #define BLDLV_BK_MO    ((unsigned)0xF << 12)
    #define BLDLV_SM_MO    ((unsigned)0xF << 8)
    #define BLDLV_MESS_MO    ((unsigned)0xF << 4)
    #define BLDLV_EDGE_MO    ((unsigned)0xF << 0)

#define RW_NR_2DNR_CTRL_80         0xFA0
    #define BLDLV_BK_FRST    ((unsigned)0xF << 28)
    #define BLDLV_SM_FRST    ((unsigned)0xF << 24)
    #define BLDLV_MESS_FRST    ((unsigned)0xF << 20)
    #define BLDLV_EDGE_FRST    ((unsigned)0xF << 16)
    #define BLDLV_BK_CO1    ((unsigned)0xF << 12)
    #define BLDLV_SM_CO1    ((unsigned)0xF << 8)
    #define BLDLV_MESS_CO1    ((unsigned)0xF << 4)
    #define BLDLV_EDGE_CO1    ((unsigned)0xF << 0)
#define RW_NR_2DNR_CTRL_81         0xFA4
    #define BLDLV_BK_CO2    ((unsigned)0xF << 28)
    #define BLDLV_SM_CO2    ((unsigned)0xF << 24)
    #define BLDLV_MESS_CO2    ((unsigned)0xF << 20)
    #define BLDLV_EDGE_CO2    ((unsigned)0xF << 16)
    #define BLDLV_BK_CO3    ((unsigned)0xF << 12)
    #define BLDLV_SM_CO3    ((unsigned)0xF << 8)
    #define BLDLV_MESS_CO3    ((unsigned)0xF << 4)
    #define BLDLV_EDGE_CO3    ((unsigned)0xF << 0)
#define RW_NR_2DNR_CTRL_82         0xFA8
#define RW_NR_2DNR_CTRL_83         0xFAC
    #define BLDLV_MOS_BK    ((unsigned)0xF << 28)
    #define BLDLV_MOS_MO    ((unsigned)0xF << 24)
    #define BLDLV_MOS_ST    ((unsigned)0xF << 20)
    #define BLDLV_MOS_DEF    ((unsigned)0xF << 16)
#define RW_NR_2DNR_CTRL_84         0xFB0
    #define BLDLV_MOS_CO3    ((unsigned)0xF << 12)
    #define BLDLV_MOS_CO2    ((unsigned)0xF << 8)
    #define BLDLV_MOS_CO1    ((unsigned)0xF << 4)
    #define BLDLV_NEAR_FRST    ((unsigned)0xF << 0)
#define RW_NR_2DNR_CTRL_85         0xFB4
#define RW_NR_2DNR_CTRL_86         0xFB8
#define RW_NR_2DNR_CTRL_87         0xFBC
#define RW_NR_2DNR_CTRL_88         0xFC0
#define RW_NR_2DNR_CTRL_89         0xFC4
#define RW_NR_2DNR_CTRL_8A         0xFC8
#define RW_NR_2DNR_CTRL_8B         0xFCC
#define RW_NR_2DNR_CTRL_8C         0xFD0
#define RW_NR_2DNR_CTRL_8D         0xFD4
#define RW_NR_2DNR_CTRL_8E         0xFD8

#define RW_NR_2DNR_CTRL_98         0xFDC
#define RW_NR_2DNR_CTRL_99         0xFE0
#define RW_NR_2DNR_CTRL_9A         0xFE4
#define RW_NR_2DNR_CTRL_9B         0xFE8
#define RW_NR_2DNR_CTRL_9C         0xFEC
#define RW_NR_2DNR_CTRL_9D         0xFF0

#define RW_NR_2DNR_CTRL_8F         0xFF4
#define RW_NR_2DNR_CTRL_90         0xFF8
#define RW_NR_2DNR_CTRL_91         0xFFC

//3D NR
#define RW_NR_3DNR_CTRL_00 (TNR_REG_BASE + 0x000)
    #define TNR_ENABLE           ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define TNR_NE3X3_BACK_SEL   ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define C_SRC_420            ((unsigned)0x1 << 29) //Fld(1,29,AC_MSKB3)//[29:29]
    #define C_SW_INIT            ((unsigned)0x1 << 27) //Fld(1,27,AC_MSKB3)//[27:27]
    #define C_V_BOUND_PROTECT    ((unsigned)0x1 << 26) //Fld(1,26,AC_MSKB3)//[26:26]
    #define BYPASS_INK_SEL       ((unsigned)0x1 << 25) //Fld(1,25,AC_MSKB3)//[25:25]
    #define NR_INK_SEL           ((unsigned)0x1 << 24) //Fld(1,24,AC_MSKB3)//[24:24]
    #define TNR_ATPG_CT          ((unsigned)0x1 << 23) //Fld(1,23,AC_MSKB2)//[23:23]
    #define TNR_ATPG_OB          ((unsigned)0x1 << 22) //Fld(1,22,AC_MSKB2)//[22:22]
    #define TNR_GAIN_SEL         ((unsigned)0x1 << 17) //Fld(1,17,AC_MSKB2)//[17:17]
    #define TNR_5365_EN          ((unsigned)0x1 << 16) //Fld(1,16,AC_MSKB2)//[16:16]
    #define DEMO_SIDE            ((unsigned)0x1 << 12)
    #define FRM_DW_WIDTH         ((unsigned)0x3FF << 0)
    #define NR_READ_ENABLE       ((unsigned)0x1 << 10) //Fld(1,10,AC_MSKB1)//[10:10]
    #define NR_C_DELAY_SEL       ((unsigned)0x1 << 9) //Fld(1,9,AC_MSKB1)//[9:9]
    #define NR_PROGRESSIVE       ((unsigned)0x1 << 8) //Fld(1,8,AC_MSKB1)//[8:8]
    #define NRYUV_MODE           ((unsigned)0x3 << 6) //Fld(2,6,AC_MSKB0)//[7:6]
    #define NR_DISPLAY_MODE      ((unsigned)0x3 << 4) //Fld(2,4,AC_MSKB0)//[5:4]
    #define NR_MAIN_PIP_SEL      ((unsigned)0x1 << 3) //Fld(1,3,AC_MSKB0)//[3:3]
    #define NR_BIT_SEL_RV        ((unsigned)0x1 << 2) //Fld(1,2,AC_MSKB0)//[2:2]
    #define NR_BIT_SEL_RU        ((unsigned)0x1 << 1) //Fld(1,1,AC_MSKB0)//[1:1]
    #define NR_BIT_SEL_RY        ((unsigned)0x1 << 0) //Fld(1,0,AC_MSKB0)//[0:0]
#define RW_NR_3DNR_CTRL_01 (TNR_REG_BASE + 0x004)
    #define TNR_ENABLE_EXCLUDE_MAX ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define TNR_ENABLE_DIFF_WEI    ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define TNR_UNI_QUAN           ((unsigned)0x7 << 24) //Fld(3,24,AC_MSKB3)//[26:24]
    #define TNR_BURST_QUAN         ((unsigned)0x7 << 20) //Fld(3,20,AC_MSKB2)//[22:20]
    #define TNR_CEN_WEI_QUAN       ((unsigned)0x7 << 16) //Fld(3,16,AC_MSKB2)//[18:16]
    #define CLAVGSEL               ((unsigned)0x3 << 12) //Fld(2,12,AC_MSKB1)//[13:12]
    #define TOTLCNT                ((unsigned)0xFFF << 0) //Fld(12,0,AC_MSKW10)//[11:0]
#define RW_NR_3DNR_CTRL_02 (TNR_REG_BASE + 0x008)
    #define ENFORCE_TBL            ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define ENFBCH                 ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define ENEGCOND               ((unsigned)0x3 << 28) //Fld(2,28,AC_MSKB3)//[29:28]
    #define ENCHDMO                ((unsigned)0x1 << 27) //Fld(1,27,AC_MSKB3)//[27:27]
    #define MOTPSEL                ((unsigned)0x1 << 26) //Fld(1,26,AC_MSKB3)//[26:26]
    #define C_USE_YTBL             ((unsigned)0x1 << 25) //Fld(1,25,AC_MSKB3)//[25:25]
    #define ENCMOUSEC              ((unsigned)0x1 << 24) //Fld(1,24,AC_MSKB3)//[24:24]
    #define ENYMOUSEY              ((unsigned)0x1 << 23) //Fld(1,23,AC_MSKB2)//[23:23]
    #define CMOUSEY                ((unsigned)0x1 << 22) //Fld(1,22,AC_MSKB2)//[22:22]
    #define ENWCUFIX               ((unsigned)0x1 << 21) //Fld(1,21,AC_MSKB2)//[21:21]
    #define REG_LP3X3SEL           ((unsigned)0x1 << 20) //Fld(1,20,AC_MSKB2)//[20:20]
    #define ENSDCR                 ((unsigned)0x1 << 19) //Fld(1,19,AC_MSKB2)//[19:19]
    #define ENHDMO                 ((unsigned)0x1 << 18) //Fld(1,18,AC_MSKB2)//[18:18]
    #define MANWCUH                ((unsigned)0x1 << 17) //Fld(1,17,AC_MSKB2)//[17:17]
    #define ENFWCH                 ((unsigned)0x1 << 16) //Fld(1,16,AC_MSKB2)//[16:16]
    #define B8MODE                 ((unsigned)0x1 << 15) //Fld(1,15,AC_MSKB1)//[15:15]
    #define ENHDSMO                ((unsigned)0x1 << 14) //Fld(1,14,AC_MSKB1)//[14:14]
    #define TAVGMODE               ((unsigned)0x1 << 13) //Fld(1,13,AC_MSKB1)//[13:13]
    #define MANWCU                 ((unsigned)0x1 << 12) //Fld(1,12,AC_MSKB1)//[12:12]
    #define DISGHF16               ((unsigned)0x1 << 11) //Fld(1,11,AC_MSKB1)//[11:11]
    #define FREEZE_3DNR            ((unsigned)0x1 << 10) //Fld(1,10,AC_MSKB1)//[10:10]
    #define ENMANDOTHK             ((unsigned)0x1 << 9) //Fld(1,9,AC_MSKB1)//[9:9]
    #define ENBPATVSYNC            ((unsigned)0x1 << 7) //Fld(1,7,AC_MSKB0)//[7:7]
    #define RD0SEL                 ((unsigned)0x1 << 6) //Fld(1,6,AC_MSKB0)//[6:6]
    #define ENVYDBP                ((unsigned)0x1 << 5) //Fld(1,5,AC_MSKB0)//[5:5]
    #define ENVCDBP                ((unsigned)0x1 << 4) //Fld(1,4,AC_MSKB0)//[4:4]
    #define LBMODE                 ((unsigned)0xF << 0) //Fld(4,0,AC_MSKB0)//[3:0]
#define RW_NR_3DNR_CTRL_03 (TNR_REG_BASE + 0x00C)
    #define ENWCUEQ64OPT           ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define OLDDMR                 ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define ENHSMPC                ((unsigned)0x1 << 29) //Fld(1,29,AC_MSKB3)//[29:29]
    #define EN_PV7FTI              ((unsigned)0x1 << 28) //Fld(1,28,AC_MSKB3)//[28:28]
    #define USEPWST                ((unsigned)0x1 << 27) //Fld(1,27,AC_MSKB3)//[27:27]
    #define PWFST3D                ((unsigned)0x1 << 26) //Fld(1,26,AC_MSKB3)//[26:26]
    #define DBLPSEL                ((unsigned)0x3 << 24) //Fld(2,24,AC_MSKB3)//[25:24]
    #define ENSMOGT                ((unsigned)0x1 << 23) //Fld(1,23,AC_MSKB2)//[23:23]
    #define RDSEL                  ((unsigned)0x1 << 22) //Fld(1,22,AC_MSKB2)//[22:22]
    #define BYPASS_2D_NR           ((unsigned)0x1 << 20) //Fld(1,20,AC_MSKB2)//[20:20]
    #define BPYSB2D                ((unsigned)0x1 << 19) //Fld(1,19,AC_MSKB2)//[19:19]
    #define BPCSB2D                ((unsigned)0x1 << 18) //Fld(1,18,AC_MSKB2)//[18:18]
    #define USEYCAVG               ((unsigned)0x1 << 17) //Fld(1,17,AC_MSKB2)//[17:17]
    #define ENMANGNR               ((unsigned)0x1 << 11) //Fld(1,11,AC_MSKB1)//[11:11]
    #define CUSEYWEI               ((unsigned)0x1 << 10) //Fld(1,10,AC_MSKB1)//[10:10]
    #define ENDBF                  ((unsigned)0x1 << 9) //Fld(1,9,AC_MSKB1)//[9:9]
    #define ENTBEXT                ((unsigned)0x1 << 8) //Fld(1,8,AC_MSKB1)//[8:8]
    #define EN_HBTNR               ((unsigned)0x1 << 7) //Fld(1,7,AC_MSKB0)//[7:7]
    #define ENRDPROT               ((unsigned)0x1 << 6) //Fld(1,6,AC_MSKB0)//[6:6]
    #define FDLYSEL                ((unsigned)0x3 << 4) //Fld(2,4,AC_MSKB0)//[5:4]
    #define RCOLOR_COND_BIGMO      ((unsigned)0x1 << 2) //Fld(1,2,AC_MSKB0)//[2:2]
    #define RCOLOR_COND_SMLMO      ((unsigned)0x1 << 1) //Fld(1,1,AC_MSKB0)//[1:1]
    #define RCOLOR_COND_STILL      ((unsigned)0x1 << 0) //Fld(1,0,AC_MSKB0)//[0:0]
#define RW_NR_3DNR_CTRL_04 (TNR_REG_BASE + 0x010)
    #define R_3DEDGE_1PXL          ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define R_UI3DEDGETYPE         ((unsigned)0x7 << 31) //Fld(3,28,AC_MSKB3)//[30:28]
    #define HFSEL                  ((unsigned)0x3 << 31) //Fld(2,22,AC_MSKB2)//[23:22]
    #define VFSEL                  ((unsigned)0x3 << 31) //Fld(2,20,AC_MSKB2)//[21:20]
    #define INKSEL_3DNR            ((unsigned)0x1F << 31) //Fld(5,12,AC_MSKW21)//[16:12]
    #define ENFYCCWCU              ((unsigned)0x1 << 11) //Fld(1,11,AC_MSKB1)//[11:11]
    #define ENFYCWCU               ((unsigned)0x1 << 10) //Fld(1,10,AC_MSKB1)//[10:10]
    #define ENTRYHSMOR             ((unsigned)0x1 << 9) //Fld(1,9,AC_MSKB1)//[9:9]
    #define RLYMOTH                ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_05 (TNR_REG_BASE + 0x014)
    #define EN_32LV                ((unsigned)0x1 << 20) //Fld(1,20,AC_MSKB2)//[20:20]
    #define EN_16LV                ((unsigned)0x1 << 19) //Fld(1,19,AC_MSKB2)//[19:19]
    #define EN_MANLSB              ((unsigned)0x1 << 18) //Fld(1,18,AC_MSKB2)//[18:18]
    #define ENP4MO                 ((unsigned)0x1 << 17) //Fld(1,17,AC_MSKB2)//[17:17]
    #define REG_MANLSB             ((unsigned)0x3 << 14) //Fld(2,14,AC_MSKB1)//[15:14]
    #define MOLPSEL                ((unsigned)0x3 << 12) //Fld(2,12,AC_MSKB1)//[13:12]
    #define ENIDDIN8               ((unsigned)0x1 << 11) //Fld(1,11,AC_MSKB1)//[11:11]
    #define ENID                   ((unsigned)0x1 << 10) //Fld(1,10,AC_MSKB1)//[10:10]
    #define B8MODE_ATR             ((unsigned)0x1 << 9) //Fld(1,9,AC_MSKB1)//[9:9]
    #define DISFCND                ((unsigned)0x1 << 8) //Fld(1,8,AC_MSKB1)//[8:8]
    #define ENDICOLOR              ((unsigned)0x1 << 7) //Fld(1,7,AC_MSKB0)//[7:7]
    #define ENBRNC                 ((unsigned)0x1 << 6) //Fld(1,6,AC_MSKB0)//[6:6]
    #define VGA3D                  ((unsigned)0x1 << 4) //Fld(1,4,AC_MSKB0)//[4:4]
    #define HFTBLSEL               ((unsigned)0x1 << 3) //Fld(1,3,AC_MSKB0)//[3:3]
    #define VSMSEL                 ((unsigned)0x1 << 2) //Fld(1,2,AC_MSKB0)//[2:2]
    #define ENCOLOR                ((unsigned)0x1 << 1) //Fld(1,1,AC_MSKB0)//[1:1]
    #define INV_UVSEQ              ((unsigned)0x1 << 0) //Fld(1,0,AC_MSKB0)//[0:0]
#define RW_NR_3DNR_CTRL_06 (TNR_REG_BASE + 0x018)
    #define TNR_CENTER_UNI_BOUND   ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define TNR_DIFF_Y_LP_BOUND    ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_07 (TNR_REG_BASE + 0x01C)
    #define CEGTH                  ((unsigned)0xFF << 24) //Fld(8,24,AC_FULLB3)//[31:24]
    #define SCHG_FDSEL             ((unsigned)0x3 << 20) //Fld(2,20,AC_MSKB2)//[21:20]
    #define YMOTH                  ((unsigned)0x3FF << 8) //Fld(10,8,AC_MSKW21)//[17:8]
    #define ENWHINK                ((unsigned)0x1 << 4) //Fld(1,4,AC_MSKB0)//[4:4]
    #define ENCTIIR                ((unsigned)0x1 << 3) //Fld(1,3,AC_MSKB0)//[3:3]
    #define ENIDF                  ((unsigned)0x1 << 2) //Fld(1,2,AC_MSKB0)//[2:2]
    #define ENPRFIR                ((unsigned)0x1 << 1) //Fld(1,1,AC_MSKB0)//[1:1]
    #define ENYTIIR                ((unsigned)0x1 << 0) //Fld(1,0,AC_MSKB0)//[0:0]
#define RW_NR_3DNR_CTRL_08 (TNR_REG_BASE + 0x020)
    #define INKTH_3DNR             ((unsigned)0x3FF << 20) //Fld(10,20,AC_MSKW32)//[29:20]
    #define FBIGMO_CNT             ((unsigned)0xFFFFF << 0) //Fld(20,0,AC_MSKDW)//[19:0]
#define RW_NR_3DNR_CTRL_09 (TNR_REG_BASE + 0x02C)
    #define CIIR_TBL               ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_0C (TNR_REG_BASE + 0x030)
    #define B2TBTH                 ((unsigned)0x7F << 16) //Fld(7,16,AC_MSKB2)//[22:16]
    #define C_MOTH                 ((unsigned)0x7F << 0) //Fld(7,0,AC_MSKB0)//[6:0]
#define RW_NR_3DNR_CTRL_0D (TNR_REG_BASE + 0x034)
    #define NR_DEMO_MODE           ((unsigned)0x3 << 30)
    #define LNBUFMODE              ((unsigned)0x3 << 30) //Fld(2,30,AC_MSKB3)//[31:30]
    #define FRM_WD_WIDTH           ((unsigned)0x3FF << 20) //Fld(10,20,AC_MSKW32)//[29:20]
    #define FSMALLMO_CNT           ((unsigned)0xFFFFF << 0) //Fld(20,0,AC_MSKDW)//[19:0]
#define RW_NR_3DNR_CTRL_0E (TNR_REG_BASE + 0x038)
    #define HDGTVDTH               ((unsigned)0x7FFF << 16) //Fld(15,16,AC_MSKW32)//[30:16]
#define RW_NR_3DNR_CTRL_0F (TNR_REG_BASE + 0x03C)
    #define FYSTDTH                ((unsigned)0xFF << 24) //Fld(8,24,AC_FULLB3)//[31:24]
    #define FCSTDTH                ((unsigned)0x3FF << 12) //Fld(10,12,AC_MSKW21)//[21:12]
    #define VYDGTTH                ((unsigned)0x3FF << 0) //Fld(10,0,AC_MSKW10)//[9:0]
#define RW_NR_3DNR_CTRL_10 (TNR_REG_BASE + 0x040)
    #define P4MOTBL                ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_11 (TNR_REG_BASE + 0x044)
    #define NOP4MOTBL              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_12 (TNR_REG_BASE + 0x048)
    #define SKINTBL                ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_13 (TNR_REG_BASE + 0x04C)
    #define SMOGTBL                ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_14 (TNR_REG_BASE + 0x050)
    #define SKINTBTH               ((unsigned)0x7F << 16) //Fld(7,16,AC_MSKB2)//[22:16]
    #define NOP4MOTBTH             ((unsigned)0x7F << 8) //Fld(7,8,AC_MSKB1)//[14:8]
    #define P4MOTBTH               ((unsigned)0x7F << 0) //Fld(7,0,AC_MSKB0)//[6:0]
#define RW_NR_3DNR_CTRL_15 (TNR_REG_BASE + 0x054)
    #define NCTH                   ((unsigned)0xFF << 20) //Fld(8,20,AC_MSKW32)//[27:20]
    #define FSTILL_CNT             ((unsigned)0xFFFFF << 0) //Fld(20,0,AC_MSKDW)//[19:0]
#define RW_NR_3DNR_CTRL_16 (TNR_REG_BASE + 0x058)
    #define FORCEVLP               ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define ENADVLP                ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define ENMEDIAN               ((unsigned)0x1 << 29) //Fld(1,29,AC_MSKB3)//[29:29]
    #define MOTIONTH_3DNR          ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define YHORDIFSMTH            ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_17 (TNR_REG_BASE + 0x05C)
    #define MANBWCUFYC_0           ((unsigned)0x3F << 24) //Fld(6,24,AC_MSKB3)//[29:24]
    #define HDSMOTH                ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define HDMOTH                 ((unsigned)0x3FF << 0) //Fld(10,0,AC_MSKW10)//[9:0]
#define RW_NR_3DNR_CTRL_18 (TNR_REG_BASE + 0x060)
    #define ENADCLV                ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define CLVTH                  ((unsigned)0x3FF << 20) //Fld(10,20,AC_MSKW32)//[29:20]
    #define YEGHSMTH               ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_19 (TNR_REG_BASE + 0x064)
    #define RFIRY                  ((unsigned)0xFFF << 12) //Fld(12,12,AC_MSKW21)//[23:12]
    #define RFIRC                  ((unsigned)0xFFF << 0) //Fld(12,0,AC_MSKW10)//[11:0]
#define RW_NR_3DNR_CTRL_1A (TNR_REG_BASE + 0x068)
    #define FSTILL_TBL             ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_1B (TNR_REG_BASE + 0x06C)
    #define FSMLMO_TBL             ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_1C (TNR_REG_BASE + 0x070)
    #define DEF_TBL                ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_1D (TNR_REG_BASE + 0x074)
    #define FBIGMO_TBL             ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_1E (TNR_REG_BASE + 0x078)
    #define BGMOTBTH               ((unsigned)0x7F << 24) //Fld(7,24,AC_MSKB3)//[30:24]
    #define DEFTBTH                ((unsigned)0x7F << 16) //Fld(7,16,AC_MSKB2)//[22:16]
    #define FSMLMOTBTH             ((unsigned)0x7F << 8) //Fld(7,8,AC_MSKB1)//[14:8]
    #define FSTILLTBTH             ((unsigned)0x7F << 0) //Fld(7,0,AC_MSKB0)//[6:0]
#define RW_NR_3DNR_CTRL_20 (TNR_REG_BASE + 0x080)
    #define CEGG                   ((unsigned)0xF << 28) //Fld(4,28,AC_MSKB3)//[31:28]
    #define YEGG                   ((unsigned)0xF << 24) //Fld(4,24,AC_MSKB3)//[27:24]
    #define VBMOCNT                ((unsigned)0xFFFFF << 0) //Fld(20,0,AC_MSKDW)//[19:0]
#define RW_NR_3DNR_CTRL_21 (TNR_REG_BASE + 0x084)
    #define SHIFTTH                ((unsigned)0x3FFF << 11) //Fld(10,11,AC_MSKW21)//[20:11]
    #define CRSUMBG                ((unsigned)0x7FFF << 0) //Fld(11,0,AC_MSKW10)//[10:0]
#define RW_NR_3DNR_CTRL_22 (TNR_REG_BASE + 0x088)
    #define DISCIWITHC             ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define SHIFTSEL               ((unsigned)0x1 << 29) //Fld(1,29,AC_MSKB3)//[29:29]
    #define DISSHIFT               ((unsigned)0x1 << 28) //Fld(1,28,AC_MSKB3)//[28:28]
    #define NOCSAMEPSEL            ((unsigned)0x1 << 27) //Fld(1,27,AC_MSKB3)//[27:27]
    #define NEWCR                  ((unsigned)0x3 << 24) //Fld(2,24,AC_MSKB3)//[25:24]
    #define BLOCKCRTH              ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define CISMALL                ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define YBPFTH                 ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_23 (TNR_REG_BASE + 0x08C)
    #define DOITEXV                ((unsigned)0x3 << 30) //Fld(2,30,AC_MSKB3)//[31:30]
    #define DOITEXH                ((unsigned)0x1 << 29) //Fld(1,29,AC_MSKB3)//[29:29]
    #define DOITSEL                ((unsigned)0x1 << 28) //Fld(1,28,AC_MSKB3)//[28:28]
    #define WITHCTH                ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define INKTH_CCS              ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define INK_3DNR               ((unsigned)0x3F << 0) //Fld(6,0,AC_MSKB0)//[5:0]
#define RW_NR_3DNR_CTRL_24 (TNR_REG_BASE + 0x090)
    #define CHROMATH               ((unsigned)0x1F << 26) //Fld(5,26,AC_MSKB3)//[30:26]
    #define CHROMASMTH             ((unsigned)0x1FFF << 12) //Fld(13,12,AC_MSKDW)//[24:12]
    #define CLPSEL                 ((unsigned)0x1 << 10) //Fld(1,10,AC_MSKB1)//[10:10]
    #define LUMADIFSMTH            ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_25 (TNR_REG_BASE + 0x094)
    #define WHOCOLORSMPIX          ((unsigned)0x1FFF << 19) //Fld(13,19,AC_MSKW32)//[31:19]
    #define WHOLECOLOR_EN          ((unsigned)0x1 << 18) //Fld(1,18,AC_MSKB2)//[18:18]
    #define CCSCR                  ((unsigned)0x1 << 17) //Fld(1,17,AC_MSKB2)//[17:17]
    #define ENCSMANU2              ((unsigned)0x1 << 16) //Fld(1,16,AC_MSKB2)//[16:16]
    #define CCSMANUG               ((unsigned)0xF << 12) //Fld(4,12,AC_MSKB1)//[15:12]
    #define ENCSMANU1              ((unsigned)0x1 << 11) //Fld(1,11,AC_MSKB1)//[11:11]
    #define ENBLOCK1               ((unsigned)0x1 << 10) //Fld(1,10,AC_MSKB1)//[10:10]
    #define YIPBOTHHP              ((unsigned)0x1 << 9) //Fld(1,9,AC_MSKB1)//[9:9]
    #define ENWORDG                ((unsigned)0x1 << 8) //Fld(1,8,AC_MSKB1)//[8:8]
    #define YIPBOTHHPCG            ((unsigned)0xF << 4) //Fld(4,4,AC_MSKB0)//[7:4]
    #define WORDCG                 ((unsigned)0xF << 0) //Fld(4,0,AC_MSKB0)//[3:0]
#define RW_NR_3DNR_CTRL_26 (TNR_REG_BASE + 0x098)
    #define LLV3                   ((unsigned)0x3FF << 20) //Fld(10,20,AC_MSKW32)//[29:20]
    #define CSMLVGTH               ((unsigned)0xFFFFF << 0) //Fld(20,0,AC_MSKDW)//[19:0]
#define RW_NR_3DNR_CTRL_27 (TNR_REG_BASE + 0x09C)
    #define ENRNDWFIX              ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define WFIX16_32              ((unsigned)0x1 << 29) //Fld(1,29,AC_MSKB3)//[29:29]
    #define WFIX8_16               ((unsigned)0x1 << 28) //Fld(1,28,AC_MSKB3)//[28:28]
    #define WFIX4_8                ((unsigned)0x1 << 27) //Fld(1,27,AC_MSKB3)//[27:27]
    #define WFIX2_4                ((unsigned)0x1 << 26) //Fld(1,26,AC_MSKB3)//[26:26]
    #define WFIX1_2                ((unsigned)0x1 << 25) //Fld(1,25,AC_MSKB3)//[25:25]
    #define WFIX0_1                ((unsigned)0x1 << 24) //Fld(1,24,AC_MSKB3)//[24:24]
    #define DINTH                  ((unsigned)0xF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define BP_INTERLV             ((unsigned)0x1 << 15) //Fld(1,15,AC_MSKB1)//[15:15]
    #define BP_FRTG                ((unsigned)0x1 << 14) //Fld(1,14,AC_MSKB1)//[14:14]
    #define IDFSEL                 ((unsigned)0x3 << 12) //Fld(2,12,AC_MSKB1)//[13:12]
    #define EN_LUMAINC             ((unsigned)0x1 << 11) //Fld(1,11,AC_MSKB1)//[11:11]
    #define LUMAINC                ((unsigned)0x3FF << 0) //Fld(10,0,AC_MSKW10)//[9:0]
#define RW_NR_3DNR_CTRL_28 (TNR_REG_BASE + 0x0A0)
    #define PEAKLUMATB             ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_29 (TNR_REG_BASE + 0x0A4)
    #define UVSMALLTH              ((unsigned)0xFF << 24) //Fld(8,24,AC_FULLB3)//[31:24]
    #define YI7X2MAXTH             ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define YI7X2MINTH             ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define UVSMSEL_3DNR           ((unsigned)0x3 << 4) //Fld(2,4,AC_MSKB0)//[5:4]
    #define PHNUM                  ((unsigned)0xF << 0) //Fld(4,0,AC_MSKB0)//[3:0]
#define RW_NR_3DNR_CTRL_2A (TNR_REG_BASE + 0x0A8)
    #define CCSEN                  ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define CCSEN4Y                ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define CO_SEL                 ((unsigned)0x1 << 21) //Fld(1,21,AC_MSKB2)//[21:21]
    #define RAWYO_SEL              ((unsigned)0x1 << 20) //Fld(1,20,AC_MSKB2)//[20:20]
    #define WCU_SEL                ((unsigned)0x1 << 19) //Fld(1,19,AC_MSKB2)//[19:19]
    #define WCU_WEIGHT             ((unsigned)0x4 << 16) //Fld(3,16,AC_MSKB2)//[18:16]
    #define WCU_OFFSET             ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define YIPBOTHHPCD_OLD        ((unsigned)0xF << 4) //Fld(4,4,AC_MSKB0)//[7:4]
    #define WORDCG_OLD             ((unsigned)0xF << 0) //Fld(4,0,AC_MSKB0)//[3:0]
#define RW_NR_3DNR_CTRL_2B (TNR_REG_BASE + 0x0AC)
    #define MOTION_RGHIGH          ((unsigned)0x3FF << 12) //Fld(10,12,AC_MSKW21)//[21:12]
    #define MOTION_RGLOW           ((unsigned)0x3FF << 0) //Fld(10,0,AC_MSKW10)//[9:0]
#define RW_NR_3DNR_CTRL_2C (TNR_REG_BASE + 0x0B0)
    #define REGTBTHX4              ((unsigned)0xFF << 24) //Fld(8,24,AC_FULLB3)//[31:24]
    #define REGTBTHX3              ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define REGTBTHX2              ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define REGTBTHX1              ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_2D (TNR_REG_BASE + 0x0B4)
    #define REGTBTHX8              ((unsigned)0xFF << 24) //Fld(8,24,AC_FULLB3)//[31:24]
    #define REGTBTHX7              ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define REGTBTHX6              ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define REGTBTHX5              ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_2E (TNR_REG_BASE + 0x0B8)
    #define ENREGTBTH              ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define ENPGGH                 ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define ENWCU_SEG              ((unsigned)0x1 << 29) //Fld(1,29,AC_MSKB3)//[29:29]
    #define PGGH                   ((unsigned)0x1F << 24) //Fld(5,24,AC_MSKB3)//[28:24]
    #define FYCHEGTH_VCD           ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define FYCHEGTH_C             ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define FYCHEGTH_Y             ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_30 (TNR_REG_BASE + 0x0C0)
    #define DKBOSTBL               ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_31 (TNR_REG_BASE + 0x0C4)
    #define DOCTBTH                ((unsigned)0x7F << 24) //Fld(7,24,AC_MSKB3)//[30:24]
    #define HEGTH                  ((unsigned)0xFF << 12) //Fld(8,12,AC_MSKW21)//[19:12]
    #define YLV_YTH                ((unsigned)0x3FF << 0) //Fld(10,0,AC_MSKW10)//[9:0]
#define RW_NR_3DNR_CTRL_32 (TNR_REG_BASE + 0x0C8)
    #define HFTBL                  ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_33 (TNR_REG_BASE + 0x0CC)
    #define HEND_METRIC            ((unsigned)0x7FF << 12) //Fld(11,12,AC_MSKW21)//[22:12]
    #define HSTART_METRIC          ((unsigned)0x7FF << 0) //Fld(11,0,AC_MSKW10)//[10:0]
#define RW_NR_3DNR_CTRL_34 (TNR_REG_BASE + 0x0D0)
    #define VEND_METRIC            ((unsigned)0x7FF << 12) //Fld(11,12,AC_MSKW21)//[22:12]
    #define VSTART_METRIC          ((unsigned)0x7FF << 0) //Fld(11,0,AC_MSKW10)//[10:0]
#define RW_NR_3DNR_CTRL_35 (TNR_REG_BASE + 0x0D4)
    #define DCR_TBL                ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_36 (TNR_REG_BASE + 0x0D8)
    #define DKMOTH                 ((unsigned)0xFF << 24) //Fld(8,24,AC_FULLB3)//[31:24]
    #define ST_MANWCU              ((unsigned)0x3F << 16) //Fld(6,16,AC_MSKB2)//[21:16]
    #define HDSTTH                 ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define HSMPCTBTH              ((unsigned)0x7F << 0) //Fld(7,0,AC_MSKB0)//[6:0]
#define RW_NR_3DNR_CTRL_37 (TNR_REG_BASE + 0x0DC)
    #define C_REC_01               ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_38 (TNR_REG_BASE + 0x0E0)
    #define C_REC_02               ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_39 (TNR_REG_BASE + 0x0E4)
    #define YEDGETH                ((unsigned)0xFF << 24) //Fld(8,24,AC_FULLB3)//[31:24]
    #define HSMOPIXCNT_TH          ((unsigned)0xFFFFF << 0) //Fld(20,0,AC_MSKDW)//[19:0]
#define RW_NR_3DNR_CTRL_3A (TNR_REG_BASE + 0x0E8)
    #define GNR_TBTH               ((unsigned)0x7F << 24) //Fld(7,24,AC_MSKB3)//[30:24]
    #define HIGHMOTH               ((unsigned)0xF << 20) //Fld(4,20,AC_MSKB2)//[23:20]
    #define HSMOPIXCNT_TH2         ((unsigned)0xFFFFF << 0) //Fld(20,0,AC_MSKDW)//[19:0]
#define RW_NR_3DNR_CTRL_3B (TNR_REG_BASE + 0x0EC)
    #define FSMOOTHTB              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_3C (TNR_REG_BASE + 0x0F0)
    #define BIGMOTH_3DNR           ((unsigned)0xFF << 24) //Fld(8,24,AC_FULLB3)//[31:24]
    #define MANBWCU2               ((unsigned)0x3F << 16) //Fld(6,16,AC_MSKB2)//[21:16]
    #define MANBWCUH               ((unsigned)0x3F << 8) //Fld(6,8,AC_MSKB1)//[13:8]
    #define MANBWCU                ((unsigned)0x3F << 0) //Fld(6,0,AC_MSKB0)//[5:0]
#define RW_NR_3DNR_CTRL_3D (TNR_REG_BASE + 0x0F4)
    #define R_UIPXLMOTIONTHL_MO2D  ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define R_UIPXLMOTIONTHL_ST2D  ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_3E (TNR_REG_BASE + 0x0F8)
    #define MANUFYCC_0             ((unsigned)0x3F << 31) //Fld(6,24,AC_MSKB3)//[29:24]
    #define HSTART_3DNR            ((unsigned)0x7FF << 12) //Fld(11,12,AC_MSKW21)//[22:12]
    #define HEND_3DNR              ((unsigned)0x7FF << 0) //Fld(11,0,AC_MSKW10)//[10:0]
#define RW_NR_3DNR_CTRL_3F (TNR_REG_BASE + 0x0FC)
    #define VSTART_3DNR            ((unsigned)0x7FF << 12) //Fld(11,12,AC_MSKW21)//[22:12]
    #define VEND_3DNR              ((unsigned)0x7FF << 0) //Fld(11,0,AC_MSKW10)//[10:0]
#define RW_NR_3DNR_CTRL_40 (TNR_REG_BASE + 0x100)
    #define R_UI_FRST_STEDGE_TABLE ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_41 (TNR_REG_BASE + 0x104)
    #define R_UI_FRST_MOEDGE_TABLE ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_42 (TNR_REG_BASE + 0x108)
    #define STEDGE_TBL             ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_43 (TNR_REG_BASE + 0x10C)
    #define MOEDGE_TBL             ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_44 (TNR_REG_BASE + 0x110)
    #define R_UI_FRST_STEDGE_TABLE_TH ((unsigned)0x7F << 24) //Fld(7,24,AC_MSKB3)//[30:24]
    #define R_UI_FRST_MOEDGE_TABLE_TH ((unsigned)0x7F << 16) //Fld(7,16,AC_MSKB2)//[22:16]
    #define STEDGE_TH                 ((unsigned)0x7F << 8) //Fld(7,8,AC_MSKB1)//[14:8]
    #define MOEDGE_TH                 ((unsigned)0x7F << 0) //Fld(7,0,AC_MSKB0)//[6:0]
#define RW_NR_3DNR_CTRL_45 (TNR_REG_BASE + 0x114)
    #define R_UI3DMOTYPE_MO           ((unsigned)0x7 << 28) //Fld(3,28,AC_MSKB3)//[30:28]
    #define R_UIPXLLPTHL_MO           ((unsigned)0x3FF << 16) //Fld(10,16,AC_MSKW32)//[25:16]
    #define R_UI3DMOTYPE_ST           ((unsigned)0x7 << 12) //Fld(3,12,AC_MSKB1)//[14:12]
    #define R_UIPXLLPTHL_ST           ((unsigned)0x3FF << 0) //Fld(10,0,AC_MSKW10)//[9:0]
#define RW_NR_3DNR_CTRL_46 (TNR_REG_BASE + 0x118)
    #define R_UICOLORTONETYPE         ((unsigned)0x7 << 28) //Fld(3,28,AC_MSKB3)//[30:28]
    #define R_UIPXLMOTIONTHL_MO       ((unsigned)0x3FF << 16) //Fld(10,16,AC_MSKW32)//[25:16]
    #define R_UIPXLMOTIONTHL_ST       ((unsigned)0x3FF << 0) //Fld(10,0,AC_MSKW10)//[9:0]
#define RW_NR_3DNR_CTRL_47 (TNR_REG_BASE + 0x11C)
    #define R_UINEWMOLPTP          ((unsigned)0x3 << 28) //Fld(2,28,AC_MSKB3)//[29:28]
    #define R_BDYEDGEDETEN_ALL     ((unsigned)0x1 << 26) //Fld(1,26,AC_MSKB3)//[26:26]
    #define R_BMOEDGEDETEN_FRMO    ((unsigned)0x1 << 25) //Fld(1,25,AC_MSKB3)//[25:25]
    #define R_BSTEDGEDETEN_FRMO    ((unsigned)0x1 << 24) //Fld(1,24,AC_MSKB3)//[24:24]
    #define R_BMOEDGEDETEN_FRST    ((unsigned)0x1 << 23) //Fld(1,23,AC_MSKB2)//[23:23]
    #define R_BSTEDGEDETEN_FRST    ((unsigned)0x1 << 22) //Fld(1,22,AC_MSKB2)//[22:22]
    #define R_BFRBASESTEN          ((unsigned)0x1 << 21) //Fld(1,21,AC_MSKB2)//[21:21]
    #define R_BDYFRBASESTEN        ((unsigned)0x1 << 20) //Fld(1,20,AC_MSKB2)//[20:20]
    #define R_UIFRMOCNTTHL3D       ((unsigned)0xF << 16) //Fld(4,16,AC_MSKB2)//[19:16]
    #define R_UIFRSTCNTTHL3D       ((unsigned)0xF << 12) //Fld(4,12,AC_MSKB1)//[15:12]
    #define R_UIFRSUMTHL3D         ((unsigned)0x7FF << 0) //Fld(11,0,AC_MSKW10)//[10:0]
#define RW_NR_3DNR_CTRL_48 (TNR_REG_BASE + 0x120)
    #define HEND_FRST              ((unsigned)0x7FF << 20) //Fld(11,20,AC_MSKW32)//[30:20]
    #define HSTART_FRST            ((unsigned)0x7FF << 0) //Fld(11,0,AC_MSKW10)//[10:0]
#define RW_NR_3DNR_CTRL_49 (TNR_REG_BASE + 0x124)
    #define VEND_FRST              ((unsigned)0x7FF << 20) //Fld(11,20,AC_MSKW32)//[30:20]
    #define VSTART_FRST            ((unsigned)0x7FF << 0) //Fld(11,0,AC_MSKW10)//[10:0]
#define RW_NR_3DNR_CTRL_4A (TNR_REG_BASE + 0x12C)
    #define R_UI3DMOTYPE_MO2D      ((unsigned)0x7 << 28) //Fld(3,28,AC_MSKB3)//[30:28]
    #define R_UIPXLLPTHL_MO2D      ((unsigned)0x3FF << 16) //Fld(10,16,AC_MSKW32)//[25:16]
    #define R_UI3DMOTYPE_ST2D      ((unsigned)0x7 << 12) //Fld(3,12,AC_MSKB1)//[14:12]
    #define R_B2DNEWMOEN           ((unsigned)0x1 << 11) //Fld(1,11,AC_MSKB1)//[11:11]
    #define R_B2DMOSUMTP           ((unsigned)0x1 << 10) //Fld(1,10,AC_MSKB1)//[10:10]
    #define R_UIPXLLPTHL_ST2D      ((unsigned)0x3FF << 0) //Fld(10,0,AC_MSKW10)//[9:0]
#define RW_NR_3DNR_CTRL_4C (TNR_REG_BASE + 0x130)
    #define B_FR_BASE_ST_NEWMO_3D  ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define PW_MOCNT_FINAL         ((unsigned)0x1FFFFF << 0) //Fld(21,0,AC_MSKDW)//[20:0]
#define RW_NR_3DNR_CTRL_4D (TNR_REG_BASE + 0x134)
    #define MOPIXCNTF              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_4E (TNR_REG_BASE + 0x138)
    #define TMOPIXCNTF             ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_4F (TNR_REG_BASE + 0x13C)
    #define NE3X3AVG               ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_50 (TNR_REG_BASE + 0x140)
    #define C_DARK_TUNE_EN         ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define C_DARK_RESOLUTION      ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_60 (TNR_REG_BASE + 0x180)
    #define C_NEW_ROUND_EN         ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define TRACK_PROGRESSIVE      ((unsigned)0x1 << 25) //Fld(1,25,AC_MSKB3)//[25:25]
    #define C_KEEP_MARGIN          ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define C_TRACK_SPEED          ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_70 (TNR_REG_BASE + 0x1C0)
    #define C_NEW_TABLE_EN         ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define C_NEW_TABLE_DIFF_THD   ((unsigned)0x7F << 24) //Fld(7,24,AC_MSKB3)//[30:24]
    #define C_NEW_SMOOTH_TH        ((unsigned)0x7F << 16) //Fld(7,16,AC_MSKB2)//[22:16]
    #define C_NEW_MESS_TH          ((unsigned)0x7F << 8) //Fld(7,8,AC_MSKB1)//[14:8]
    #define C_NEW_EDGE_TH          ((unsigned)0x7F << 0) //Fld(7,0,AC_MSKB0)//[6:0]
#define RW_NR_3DNR_CTRL_71 (TNR_REG_BASE + 0x1C4)
    #define C_EDGE_TABLE           ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_72 (TNR_REG_BASE + 0x1C8)
    #define C_MESS_TABLE           ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_73 (TNR_REG_BASE + 0x1CC)
    #define C_SMOOTH_TABLE         ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_78 (TNR_REG_BASE + 0x1E0)
    #define C_NEW_WCU0_EN          ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define C_INIT0                ((unsigned)0x3F << 24) //Fld(6,24,AC_MSKB3)//[29:24]
    #define C_EXP_P0               ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define C_START0               ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define C_GAIN0                ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_79 (TNR_REG_BASE + 0x1E4)
    #define C_NEW_WCU1_EN          ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define C_INIT1                ((unsigned)0x3F << 24) //Fld(6,24,AC_MSKB3)//[29:24]
    #define C_EXP_P1               ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define C_START1               ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define C_GAIN1                ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_7A (TNR_REG_BASE + 0x1E8)
    #define C_NEW_WCU2_EN          ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define C_INIT2                ((unsigned)0x3F << 24) //Fld(6,24,AC_MSKB3)//[29:24]
    #define C_EXP_P2               ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define C_START2               ((unsigned)0xFF << 8) //Fld(8,8,AC_FULLB1)//[15:8]
    #define C_GAIN2                ((unsigned)0xFF << 0) //Fld(8,0,AC_FULLB0)//[7:0]
#define RW_NR_3DNR_CTRL_80 (TNR_REG_BASE + 0x200)
    #define C_CLEAR                ((unsigned)0x1 << 31) //Fld(1,31,AC_MSKB3)//[31:31]
    #define C_SMALL_EN             ((unsigned)0x1 << 30) //Fld(1,30,AC_MSKB3)//[30:30]
    #define C_LUMA_AUX_EN          ((unsigned)0x1 << 29) //Fld(1,29,AC_MSKB3)//[29:29]
    #define C_SRC_SEL              ((unsigned)0x1 << 28) //Fld(1,28,AC_MSKB3)//[28:28]
    #define C_VLINE_ED             ((unsigned)0x7FF << 16) //Fld(11,16,AC_MSKW32)//[26:16]
    #define C_LUMA_SEL             ((unsigned)0x3 << 12) //Fld(2,12,AC_MSKB1)//[13:12]
    #define C_VLINE_ST             ((unsigned)0x7FF << 0) //Fld(11,0,AC_MSKW10)//[10:0]
#define RW_NR_3DNR_CTRL_81 (TNR_REG_BASE + 0x204)
    #define C_SMALL_COUNTTHL_SEL   ((unsigned)0xF << 28) //Fld(4,28,AC_MSKB3)//[31:28]
    #define C_SCALE_SEL            ((unsigned)0x7 << 24) //Fld(3,24,AC_MSKB3)//[26:24]
    #define C_MOTION_THL           ((unsigned)0xFF << 16) //Fld(8,16,AC_FULLB2)//[23:16]
    #define C_SMALL_DIFFTHL        ((unsigned)0xF << 12) //Fld(4,12,AC_MSKB1)//[15:12]
    #define C_DIFF_DET_SEL         ((unsigned)0x1 << 1) //Fld(1,1,AC_MSKB0)//[1:1]
    #define C_DIFF_DET_EN          ((unsigned)0x1 << 0) //Fld(1,0,AC_MSKB0)//[0:0]
#define RW_NR_3DNR_CTRL_82 (TNR_REG_BASE + 0x208)
    #define HPIX_ED                ((unsigned)0xFFF << 16) //Fld(12,16,AC_MSKW32)//[27:16]
    #define HPIX_ST                ((unsigned)0xFFF << 0) //Fld(12,0,AC_MSKW10)//[11:0]
#define RW_NR_3DNR_CTRL_8F (TNR_REG_BASE + 0x23C)
    #define STRENGTH_SUM_2D        ((unsigned)0xFFFFFF << 0) //Fld(24,0,AC_MSKDW)//[23:0]
#define RW_NR_3DNR_CTRL_90 (TNR_REG_BASE + 0x240)
    #define NEW_METER_DIFF_AREA2   ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_DIFF_AREA1   ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_91 (TNR_REG_BASE + 0x244)
    #define NEW_METER_DIFF_AREA4   ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_DIFF_AREA3   ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_92 (TNR_REG_BASE + 0x248)
    #define NEW_METER_DIFF_AREA6   ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_DIFF_AREA5   ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_93 (TNR_REG_BASE + 0x24C)
    #define NEW_METER_DIFF_AREA8   ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_DIFF_AREA7   ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_94 (TNR_REG_BASE + 0x250)
    #define NEW_METER_DIFF_AREA10  ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_DIFF_AREA9   ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_95 (TNR_REG_BASE + 0x254)
    #define NEW_METER_DIFF_AREA12  ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_DIFF_AREA11  ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_96 (TNR_REG_BASE + 0x258)
    #define NEW_METER_DIFF_AREA14  ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_DIFF_AREA13  ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_97 (TNR_REG_BASE + 0x25C)
    #define NEW_METER_DIFF_AREA16  ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_DIFF_AREA15  ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_98 (TNR_REG_BASE + 0x260)
    #define NEW_METER_DIFF_3D_H    ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_99 (TNR_REG_BASE + 0x264)
    #define NEW_METER_DIFF_3D_L    ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_9A (TNR_REG_BASE + 0x268)
    #define NEW_METER_SMOOTH_COUNT  ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_MOTION_STATUS ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_9B (TNR_REG_BASE + 0x26C)
    #define NEW_METER_SMALL_STATUS ((unsigned)0xFFFF << 16) //Fld(16,16,AC_FULLW32)//[31:16]
    #define NEW_METER_NOISE_VALUE  ((unsigned)0xFFFF << 0) //Fld(16,0,AC_FULLW10)//[15:0]
#define RW_NR_3DNR_CTRL_C0 (TNR_REG_BASE + 0x300)
    #define AREA00_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_C1 (TNR_REG_BASE + 0x304)
    #define AREA00_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_C2 (TNR_REG_BASE + 0x308)
    #define AREA00_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_C3 (TNR_REG_BASE + 0x30C)
    #define AREA00_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_C4 (TNR_REG_BASE + 0x310)
    #define AREA01_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_C5 (TNR_REG_BASE + 0x314)
    #define AREA01_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_C6 (TNR_REG_BASE + 0x318)
    #define AREA01_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_C7 (TNR_REG_BASE + 0x31C)
    #define AREA01_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_C8 (TNR_REG_BASE + 0x320)
    #define AREA02_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_C9 (TNR_REG_BASE + 0x324)
    #define AREA02_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_CA (TNR_REG_BASE + 0x328)
    #define AREA02_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_CB (TNR_REG_BASE + 0x32C)
    #define AREA02_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_CC (TNR_REG_BASE + 0x330)
    #define AREA03_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_CD (TNR_REG_BASE + 0x334)
    #define AREA03_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_CE (TNR_REG_BASE + 0x338)
    #define AREA03_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_CF (TNR_REG_BASE + 0x33C)
    #define AREA03_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D0 (TNR_REG_BASE + 0x340)
    #define AREA10_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D1 (TNR_REG_BASE + 0x344)
    #define AREA10_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D2 (TNR_REG_BASE + 0x348)
    #define AREA10_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D3 (TNR_REG_BASE + 0x34C)
    #define AREA10_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D4 (TNR_REG_BASE + 0x350)
    #define AREA11_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D5 (TNR_REG_BASE + 0x354)
    #define AREA11_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D6 (TNR_REG_BASE + 0x358)
    #define AREA11_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D7 (TNR_REG_BASE + 0x35C)
    #define AREA11_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D8 (TNR_REG_BASE + 0x360)
    #define AREA12_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_D9 (TNR_REG_BASE + 0x364)
    #define AREA12_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_DA (TNR_REG_BASE + 0x368)
    #define AREA12_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_DB (TNR_REG_BASE + 0x36C)
    #define AREA12_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_DC (TNR_REG_BASE + 0x370)
    #define AREA13_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_DD (TNR_REG_BASE + 0x374)
    #define AREA13_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_DE (TNR_REG_BASE + 0x378)
    #define AREA13_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_DF (TNR_REG_BASE + 0x37C)
    #define AREA13_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E0 (TNR_REG_BASE + 0x380)
    #define AREA20_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E1 (TNR_REG_BASE + 0x384)
    #define AREA20_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E2 (TNR_REG_BASE + 0x388)
    #define AREA20_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E3 (TNR_REG_BASE + 0x38C)
    #define AREA20_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E4 (TNR_REG_BASE + 0x390)
    #define AREA21_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E5 (TNR_REG_BASE + 0x394)
    #define AREA21_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E6 (TNR_REG_BASE + 0x398)
    #define AREA21_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E7 (TNR_REG_BASE + 0x39C)
    #define AREA21_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E8 (TNR_REG_BASE + 0x3A0)
    #define AREA22_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_E9 (TNR_REG_BASE + 0x3A4)
    #define AREA22_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_EA (TNR_REG_BASE + 0x3A8)
    #define AREA22_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_EB (TNR_REG_BASE + 0x3AC)
    #define AREA22_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_EC (TNR_REG_BASE + 0x3B0)
    #define AREA23_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_ED (TNR_REG_BASE + 0x3B4)
    #define AREA23_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_EE (TNR_REG_BASE + 0x3B8)
    #define AREA23_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_EF (TNR_REG_BASE + 0x3BC)
    #define AREA23_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F0 (TNR_REG_BASE + 0x3C0)
    #define AREA30_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F1 (TNR_REG_BASE + 0x3C4)
    #define AREA30_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F2 (TNR_REG_BASE + 0x3C8)
    #define AREA30_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F3 (TNR_REG_BASE + 0x3CC)
    #define AREA30_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F4 (TNR_REG_BASE + 0x3D0)
    #define AREA31_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F5 (TNR_REG_BASE + 0x3D4)
    #define AREA31_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F6 (TNR_REG_BASE + 0x3D8)
    #define AREA31_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F7 (TNR_REG_BASE + 0x3DC)
    #define AREA31_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F8 (TNR_REG_BASE + 0x3E0)
    #define AREA32_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_F9 (TNR_REG_BASE + 0x3E4)
    #define AREA32_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_FA (TNR_REG_BASE + 0x3E8)
    #define AREA32_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_FB (TNR_REG_BASE + 0x3EC)
    #define AREA32_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_FC (TNR_REG_BASE + 0x3F0)
    #define AREA33_CSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_FD (TNR_REG_BASE + 0x3F4)
    #define AREA33_PSUM            ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_FE (TNR_REG_BASE + 0x3F8)
    #define AREA33_DA              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0]
#define RW_NR_3DNR_CTRL_FF (TNR_REG_BASE + 0x3FC)
    #define AREA33_DS              ((unsigned)0xFFFFFFFF << 0) //Fld(32,0,AC_FULLDW)//[31:0] 


#define vNrWriteReg(dAddr, dVal)  writel(dVal, NR_REG_OFFSET + dAddr)
#define u4NrReadReg(dAddr)         readl(NR_REG_OFFSET + dAddr)

#if 0
#define vNrWriteReg(dAddr, dVal)  *(volatile unsigned int *)(NR_REG_OFFSET + dAddr) = dVal
#define u4NrReadReg(dAddr)         *(volatile unsigned int *)(NR_REG_OFFSET + dAddr)
#endif
#define vNRWriteRegMsk(dAddr, dVal, dMsk) vNrWriteReg((dAddr), (u4NrReadReg(dAddr) & (~(dMsk))) | ((dVal) & (dMsk)))

#define vWriteReg(dAddr, dVal)  *(volatile unsigned int *)(dAddr) = dVal
#define u4ReadReg(dAddr)         *(volatile unsigned int *)(dAddr)
#define vWriteRegMsk(dAddr, dVal, dMsk) vWriteReg((dAddr), (u4ReadReg(dAddr) & (~(dMsk))) | ((dVal) & (dMsk)))

#endif

