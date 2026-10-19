/*
* Copyright (c) 2016 AutoChips Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#ifndef _VDP_HW_H_
#define _VDP_HW_H_

#include "pmx_hw.h"
//#include "x_lint.h"
#ifndef ATC_ARM2_LK_SUPPORT
//#include "x_hal_io.h"
#endif
//#include "x_hal_ic.h"
#include "chip_ver.h"
#include "drv_config.h"
//#include "x_hal_ic.h"

#ifndef ATC_ARM2_LK_SUPPORT
extern void __iomem* IO_BASE_BRINGUP;
extern void __iomem *fmtf_reg;
#define DI_REG_OFFSET   	(IO_BASE_BRINGUP)//0x12000000
#define ISR_BASE		(IO_BASE_BRINGUP)//0x12000000
#define FMT_BASE		(fmtf_reg)
#define VDP_BASE		(vdof_reg)
#define CLK_BASE		(clk_reg)
#define VDO_8TAP_BASE		(vdo_8tap_reg)
#else
//extern unsigned long IO_BASE_BRINGUP;//0x90000000
//extern unsigned int fmtf_reg;
//extern unsigned int clk_reg;
#include "platform/mt_reg_base.h"

#define ISR_BASE		(ARM9_IO_BASE + 0x02000000)
#define DI_REG_OFFSET   	(ARM9_IO_BASE + 0x02000000)
#define FMT_BASE		(ARM9_IO_BASE + 0x02001000)
#define VDP_BASE		(ARM9_IO_BASE + 0x02002400)
#define CLK_BASE		(ARM9_IO_BASE + 0x000000C0)
#define VDO_8TAP_BASE		(ARM9_IO_BASE + 0x02002100)

#endif

#define VDP_HAL_VDO_REG_NUM 		(0x100/4)
#define HAL_VDO_APQ_REG_NUM		(0x100/4)

#ifndef ATC_ARM2_LK_SUPPORT
#define ReadREG(dAddr)         readl(DI_REG_OFFSET + dAddr)
#define WriteREG(dAddr, dVal)  writel(dVal, DI_REG_OFFSET + dAddr)

#define FmtReadREG(dAddr)         readl(FMT_BASE + dAddr)
#define FmtWriteREG(dAddr, dVal)  writel(dVal, FMT_BASE + dAddr)

#define VDPReadREG(dAddr)         readl(VDP_BASE + dAddr)
#define VDPWriteREG(dAddr, dVal)  writel(dVal, VDP_BASE + dAddr)

#define VDO8TAPReadREG(dAddr)         readl(VDO_8TAP_BASE + dAddr)
#define VDO8TAPWriteREG(dAddr, dVal)  writel(dVal, VDO_8TAP_BASE + dAddr)

#define ISRReadREG(dAddr)         readl(ISR_BASE + dAddr)
#define ISRWriteREG(dAddr, dVal)  writel(dVal, ISR_BASE + dAddr)

#define CLKReadREG(dAddr)         readl(CLK_BASE + dAddr)
#define CLKWriteREG(dAddr, dVal)  writel(dVal, CLK_BASE + dAddr)
#else
#define ReadREG(dAddr)         (*(volatile unsigned int*)(DI_REG_OFFSET + dAddr))
#define WriteREG(dAddr, dVal) (*(volatile unsigned int*)(DI_REG_OFFSET + (dAddr)) = dVal)

#define FmtReadREG(dAddr)        (*(volatile unsigned int*)(FMT_BASE + dAddr))
#define FmtWriteREG(dAddr, dVal) (*(volatile unsigned int*)(FMT_BASE + (dAddr)) = dVal)

#define VDPReadREG(dAddr)        (*(volatile unsigned int*)(VDP_BASE + dAddr))
#define VDPWriteREG(dAddr, dVal) (*(volatile unsigned int*)(VDP_BASE + (dAddr)) = dVal)

#define ISRReadREG(dAddr)         (*(volatile unsigned int*)(ISR_BASE + dAddr))
#define ISRWriteREG(dAddr, dVal)  (*(volatile unsigned int*)(ISR_BASE + (dAddr)) = dVal)

#define CLKReadREG(dAddr)         (*(volatile unsigned int*)(CLK_BASE + dAddr))
#define CLKWriteREG(dAddr, dVal)  (*(volatile unsigned int*)(CLK_BASE + (dAddr)) = dVal)

#endif



#define CLKWriteREGMsk(arg, val, msk) CLKWriteREG((arg), (CLKReadREG(arg) & (~(msk))) | ((val) & (msk)))
#define FmtWriteREGMsk(arg, val, msk) FmtWriteREG((arg), (FmtReadREG(arg) & (~(msk))) | ((val) & (msk)))
#define VDPWriteREGMsk(arg, val, msk) VDPWriteREG((arg), (VDPReadREG(arg) & (~(msk))) | ((val) & (msk)))
#define VDPReadREGFEILD(addr, offset, mask) ((VDPReadREG(addr) & mask) >> offset)

#define WriteREGMsk(arg, val, msk) WriteREG((arg), (ReadREG(arg) & (~(msk))) | ((val) & (msk)))
#define WriteRegMsk(arg, val, msk) WriteREG((arg), (ReadREG(arg) & (~(msk))) | ((val) & (msk)))

#define ISRWriteREGMsk(arg, val, msk) ISRWriteREG((arg), (ISRReadREG(arg) & (~(msk))) | ((val) & (msk)))
#define ISRWriteRegMsk(arg, val, msk) ISRWriteREG((arg), (ISRReadREG(arg) & (~(msk))) | ((val) & (msk)))

// 0x12002400
typedef struct _HAL_VDO_FIELD_T
{
	/* DWORD - 000 */
	//unsigned int							:	5;
	unsigned int			u4YAddrY			:	30;
	unsigned int									:	2;

	/* DWORD - 004 */
	//unsigned int							:	5;
	unsigned int			u4YAddrC				:	30;
	unsigned int							:	2;

	/* DWORD - 008 */
	//unsigned int							:	5;
	unsigned int			u4XAddrY				:	30;
	unsigned int							:	2;

	/* DWORD - 00C */
	//unsigned int							:	5;
	unsigned int			u4XAddrC			:	30;
	unsigned int							:	2;

	/* DWORD - 010 */ //0x42410
	unsigned int			u4Hblock			:	8;
	unsigned int			u4DW_Need			:	8;
	unsigned int			u4Pic_Height					:	11;
	unsigned int								:	1;
	unsigned int			u4Hblock9_8			:	2;
	unsigned int								:	1;
	unsigned int			fgSwap_Off				:	1;

	/* DWORD - 014 */
	unsigned int			u4Vscale			:	16;
	unsigned int			u4P_Skip			:	11;
	unsigned int							:	3;
	unsigned int			u4Shadow_Mode			:	1;
	unsigned int			u4Enable_to_Layer2		:	1;


	/* DWORD - 018 */
	unsigned int			u4Y_StaMbr				:	7;
	unsigned int							:	1;
	unsigned int			u4C_StaMbr					:	7;
	unsigned int												  : 	  1;
	unsigned int			u4Max_Mbr					:	7;
	unsigned int												  : 	  1;
	unsigned int			u4Total_Mbr				:	7;
	unsigned int									:	1;

	/* DWORD - 01C */ //0x4241c
	unsigned int			fgVdoEn				:	1;
	unsigned int			fgRace				:	1;
	unsigned int			fgYLAvg				:	1;
	unsigned int			fgCLAvg				:	1;
	unsigned int			fgHalfY				:	1;
	unsigned int			fgHalfC				:	1;
	unsigned int			fgYLR					:	1;
	unsigned int			fgCLR						:	1;
	unsigned int			fgMpeg1				:	1;
	unsigned int			fgCDG				:	1;
	unsigned int			fgAdrSw				:	1; //[10] DRAM address swapping enable
	unsigned int			fgXHalf				:	1; //[11] Horizental sample reduce to half (for progressive mode)
	unsigned int										:	1; //[12]
	unsigned int			fgAPF				:	1; //[13]
	unsigned int			fgY2FF				:	1; //[14]
	unsigned int			fgC2FF				:	1; // 15
	unsigned int			fgTFld				:	1; // 16
	unsigned int			fgMI2P				:	1; // 17
	unsigned int			fgStill				:	1; //[18] force all picture regions as still regions
	unsigned int			fgMove				:	1; // 19
	unsigned int			fgPD32_F			:	1; // 20
	unsigned int			fgDISP3				:	1; // 21
	unsigned int			fgPD32				:	1; // 22
	unsigned int			fgDEL_BK			:	1; // 23
	unsigned int			fgTst0				:	1; // 24
	unsigned int			fgTst1				:	1; // 25
	unsigned int			fgTst2				:	1; // 26
	unsigned int			fgTst3				:	1; // 27
	unsigned int			fgAutoRst			:	1; // 28
	unsigned int			fgTst5				:	1; // 29
	unsigned int			fgF_CMB				:	1; // 30
	unsigned int			fgTst7				:	1; // 31

	/* DWORD - 020 */
	unsigned int			u4YSLTT				:	11;
	unsigned int							:	5;
	unsigned int			u4YSLBB					:	11;
	unsigned int							:	5;

	/* DWORD - 024 */
	unsigned int			u4CSLTT				:	11;
	unsigned int							:	5;
	unsigned int			u4CSLBB				:	11;
	unsigned int							:	5;

	/* DWORD - 028 */
	unsigned int			u4YSSLTT			:	8;
	unsigned int			u4YSSLBB			:	8;
	unsigned int			u4YSSLBT			:	8;
	unsigned int			u4YSSLTB			:	8;

	/* DWORD - 02C */
	unsigned int			u4CSSLTT			:	8;
	unsigned int			u4CSSLBB			:	8;
	unsigned int			u4CSSLBT			:	8;
	unsigned int			u4CSSLTB			:	8;

	/* DWORD - 030 */ // 0x42430
	unsigned int			fgYFrm				:	1; //[0]
	unsigned int			fgCFrm				:	1;// [1]
	unsigned int			fgPFld				:	1; // [2]
	unsigned int			fgBPic				:	1; // 3
	unsigned int			fgFldB				:	1; // 4
	unsigned int			fgAFld				:	1; // 5
	unsigned int			fgUAFld				:	1; // 6
	unsigned int			fgBPic2				:	1; // 7
	unsigned int			fgND1ACT			:	1; // 8
	unsigned int			fgCR80					:	1; // 9
	unsigned int			fgDiOff				:	1;//10
	unsigned int							:	1;//11
	unsigned int			fgL1NF16			:	1;//12
	unsigned int			fgFi_Sel			:	1;//13
	unsigned int							:	1;//14
	unsigned int			fgSram_RD_CS_4T		:	1;//15
	unsigned int							:	4; // 16,17,18,19
	unsigned int			fgSRAM					:	1; // 20
	unsigned int			fgYUV422			:	1; // 21
	unsigned int									:	1; // 22
	unsigned int								:	1;
	unsigned int			fgYSL0					:		1;
	unsigned int			fgCSL0						:	1;
	unsigned int			fgYS3L						:	1;
	unsigned int			fgYS3_NA					:	1;
	unsigned int			fgAYS3L_DIS 				:	1;
	unsigned int			fgWY_PF						:	1;
	unsigned int			fgNF5S_1ST			:	1;
	unsigned int							:	1;

	/* DWORD - 034 */ // 0x42434
	unsigned int			u4FIFO_Lim			:	7;	// [6:0]
	unsigned int							:	1;
	unsigned int			u4FIFO_Ini			:	8;	// [15:8]
	unsigned int			u4HW_Opt			:	8;	// [23:16]
	unsigned int			u4FIFO_2nd			:	7;	// [30:24]
	unsigned int							:	1;

	/* DWORD - 038 */
	unsigned int			u4RBTH_WA			:	3;
	unsigned int							:	5;
	unsigned int			u4CT_THRD			:	8;
	unsigned int			u4MTHRD						:	8;
	unsigned int			u4CTHRD						:	7;
	unsigned int							:	1;

	/* DWORD - 03C */
	//unsigned int			u4MBR_Cnt			:	7;//sync with 8317 vodrst,the two[7:0] dont used in 3365 verify
	//unsigned int			fgVDOV				:	1;//
	unsigned int					u4VDORST						:		8;
	unsigned int			u4CS_MBR			:	7;//cs_mbl
	unsigned int			fgTVFld					:	1;
	unsigned int			u4YL_MBR			:	7;//yl_mbl
	unsigned int			fgSWAP					:	1;
	unsigned int			u4Y_LINE			:	8;

	/* DWORD - 040 */
	unsigned int			u4FLD_DYN_8F_LOW		:	8;
	unsigned int			u4FLD_DYN_8F_HIGH		:	8;
	unsigned int			u4FLD_DYN_8F_CNT		:	8;
	unsigned int			u4COMB_8F_MODE0		:	1;// M8F_SEL
	unsigned int			u4COMB_8F_MODE1		:	1;// M8F_XZ_SEL
	unsigned int			u4COMB_8F_MODE2		:	1;
	unsigned int			u4COMB_8F_MODE3		:	1;
	unsigned int			u4COMB_8F_MODE4		:	1;// DYN_INV
	unsigned int			u4COMB_8F_MODE5		:	1; // DYN_8F enable dynamic 8-field motion detection
	unsigned int			u4COMB_8F_MODE6		:	1;//SCN_OFF
	unsigned int			u4COMB_8F_MODE7		:	1;

	/* DWORD - 044 */
	unsigned int			u4SAW_DYN_8F_LOW		:	8;
	unsigned int			u4SAW_DYN_8F_HIGH			:	8;
	unsigned int			u4SAW_DYN_8F_CNT		:	8;
	unsigned int			u4SAW_8F_MODE				:	8;//

	/* DWORD - 048 */
	unsigned int			u4SAW_DYN_2_LOW				:	8;
	unsigned int			u4SAW_DYN_2_HIGH		:	8;
	unsigned int			u4SAW_DYN_3_LOW				:	8;
	unsigned int			u4SAW_DYN_3_HIGH		:	8;

	/* DWORD - 04C */
	unsigned int			u4COMB_DYN_2_LOW		:	8;
	unsigned int			u4COMB_DYN_2_HIGH			:	8;
	unsigned int			u4COMB_DYN_3_LOW		:	8;
	unsigned int			u4COMB_DYN_3_HIGH			:	8;

	/* DWORD - 050 */
	unsigned int			u4YSLBT			:	11;
	unsigned int							:	5;
	unsigned int			u4YSLTB			:	11;
	unsigned int							:	5;

	/* DWORD - 054 */
	unsigned int			u4CSLBT			:	11;
	unsigned int							:	5;
	unsigned int			u4CSLTB			:	11;
	unsigned int							:	5;

		/* DWORD - 058 */
	unsigned int							:	5;
		unsigned int			u4MBAvg1				:	25 ;
		unsigned int							:	2;

		/* DWORD - 05C */
	unsigned int							:	5;
		unsigned int			u4MBAvg2				:	25;
		unsigned int							:	2;

		/* DWORD - 060 */
	unsigned int			u4Comb_rst			:	1;
		unsigned int			u4CMB_CNT_YX			:	19;  //
		unsigned int			u4Region_level_0_5_reg	:	12;

		/* DWORD - 064 */
		unsigned int			fgGet3F						:	1;//
		unsigned int			fgPH_A1						:	1;
		unsigned int			fgB3F						:	1;
		unsigned int									:	1;
		unsigned int			fgGMLine				:	1; //4/
		unsigned int			fgUMLine				:	1;
		unsigned int			fgNew3F						:	1;
		unsigned int			fgCMB_L						:	1;
		unsigned int			fgNLS						:	1;
		unsigned int			fgNLS2						:	1;
		unsigned int									:	1;
		unsigned int			fgOpt1						:	1;
		unsigned int			fgYI2P						:	1;//12
		unsigned int			fgCI2P						:	1;
		unsigned int									:	1;
		unsigned int			fgSP_LD						:	1;
		unsigned int			fgMBL_M2				:	1;
		unsigned int			fgMB_4F						:	1;
		unsigned int			fgM_AVG						:	1;
		unsigned int			fgF_STL						:	1;
		unsigned int			fgNSCL						:	1;//20
		unsigned int			fgOR_MD						:	1;
		unsigned int			fgIG_PK						:	1;
		unsigned int			fgCMB_S						:	1;
		unsigned int			fgS_CMB						:	1;//24
		unsigned int			fgS_MD						:	1;
		unsigned int			fgCC_SEL				:	1;
		unsigned int			fgCMB_1F				:	1;
		unsigned int			fgSTCMB						:	1;
		unsigned int			fgC_CMB						:	1;
		unsigned int			fgOpt2						:	1;
		unsigned int			fgNsc						:	1;

		/* DWORD - 068 */
		unsigned int			u4M3F_TH				:	8;
		unsigned int			u4M3F_LO			:	8;
		unsigned int			u4DMBL						:	7;//
		unsigned int							:	5;
		unsigned int			u4YS_CR_D				:	2;
		unsigned int							:	1;
		unsigned int			fgMBL_S2				:	1;

		/* DWORD - 06C */
		unsigned int			u4Y_STA_T			:	5;//[0-4]
		unsigned int									:	3;
		unsigned int			u4Y_STA_B			:	5;
		unsigned int									:	3;//15
		unsigned int			u4C_STA_T			:	3;
	unsigned int			u4Dl_Md				:	1;
	unsigned int			u4Dl_2Nd			:	1;
		unsigned int									:	3;
		unsigned int			u4C_STA_B			:	3;//
		unsigned int									:	5;

		/* DWORD - 070 */
		unsigned int			fgFLT_TW				:	1;
		unsigned int			fgCF_TW						:	1;
		unsigned int			fgM_EXP						:	1;
		unsigned int			fgS_EXP						:	1;
		unsigned int			fgNew4F						:	1;
		unsigned int										:	1;
		unsigned int			fgOPT0						:	1;//6
		unsigned int										:	1;
		unsigned int			fgOPT2						:	1;
		unsigned int			fgOPT3						:	1;
		unsigned int			fgU_MA4F					:	1;
		unsigned int			fgOPT4						:	1;
		unsigned int			fgNL_CS						:	1;//12
		unsigned int			fgSel_S2					:	1;//13
		unsigned int			fgPF_S2						:	1;
		unsigned int			fgCPF_S2						:	1;
		unsigned int										:	2;
		unsigned int			fgC_LO_B					:	1;
		unsigned int			fgOPT7						:	1;
		unsigned int			fgOPT8						:	1;
		unsigned int			fgOPT9						:	1;
		unsigned int			fgCSCON						:	1;
		unsigned int			fgCSCEN						:	1;
		unsigned int										:	1;
		unsigned int			fgST_EX_6F						:	1;
		unsigned int			fgDEL_YL					:	1;
		unsigned int			fgDEL_CL					:	1;
		unsigned int			fgEHF						:	1;
		unsigned int			fgOPT12						:	1;
		unsigned int										:	1;
		unsigned int			fgOPT14						:	1;

		/* DWORD - 074 */
		unsigned int			u4M_CNT_XZ						:	20;  //fld_XZ_motion_red_r
		unsigned int			u4Motion_Level_24Region_0611  :	12;


		/* DWORD - 078 */ // 0x42478
		unsigned int			fgYFIR_ON					:	1;	// 0
		unsigned int			fgYFIR_LNR						:	1;	// 1
		unsigned int			fgGAU62_15_0575 				:	1;	// 2
		unsigned int			fgGAU62_15_0675 				:	1;	// 3
		unsigned int			fgYFIR_CF_PRG					:	1;	// 4
		unsigned int			fgPH16							:	1;	// 5
		unsigned int			fgEVN_FIR						:	1;	// 6
		unsigned int			fgHD_1920_4TAP_EN				:	1;	//7
		unsigned int												:	3;	//8-10
		unsigned int			fgINTRA_EDGEPMODE				:	1;	// [11] turn intra mode with edge preserving
		unsigned int			fgHd_Scaling_Down				:	1;
		unsigned int			fgBE_4LINE_FORCE				:	1;	//[13]co-used for MT8560
		unsigned int			fgBE_4LINE_DIS					:	1;
		unsigned int			fgDRAM_TST_MODE 				:	1;
		unsigned int				fgDown_Scl_4tap_force			:	1; //[16]
		unsigned int			fgCSP_DIS						:	1;	//17
		unsigned int			fgYS5L							:	1;	//18
		unsigned int			fgAYS5L_DIS 					:	1;	//19
		unsigned int			fgYS4L							:	1;	//20
		unsigned int			fgAYS4L_DIS 					:	1;	//21
		unsigned int			fgYS2L							:	1;	//22
		unsigned int			fgAYS2L_DIS 					:	1;	//23
		unsigned int			fgYSL							:	1;	//24
		unsigned int			fgAYSL_DIS						:	1;	//25
		unsigned int			fgSP_CNT						:	1;	//26
		unsigned int			fgYC_PF 						:	1;	//27
		unsigned int			u4SPF_LIM						:	4;	//[28:31]

		/* DWORD - 07C */ //0x4247c
		unsigned int				  u4NewAddrSwapMode 			  : 	  2; //[1:0]  5351swap mode selection
		unsigned int												  : 	  1;//
		unsigned int				  u4Tb_Field_Source 			  : 	  1; //yy, h265 source
		unsigned int				  u4Video_Opt4					  : 	  1;
		unsigned int				  u4Video_Opt5					  : 	  1;
		unsigned int				  u4Video_Opt6					  : 	  1;
		unsigned int				  u4Video_Opt7					  : 	  1;
		unsigned int				  u4Video_Opt8					  : 	  1;
		unsigned int				  fgNewASMEn					  : 	  1; // [9]  swap mode enable
		unsigned int				  u4Video_Opt10 				  : 	  1;
		unsigned int				  u4Video_Opt11 				  : 	  1;
		unsigned int				  fgPROT_WR_STA 				  : 	  1;
		unsigned int				  fgPROT_WR_END 				  : 	  1;
		unsigned int				  fgBurstLengthEnable			  : 	  1;	//[14]
		unsigned int				  u4Video_Opt15 				  : 	  1;
		unsigned int				  u4AddrSwapMode				  : 	  1;
		unsigned int				  u4Video_Opt17 				  : 	  1;
		unsigned int												  : 	  1;
		unsigned int												  : 	  1;
		unsigned int				  u4Video_Opt20 				  : 	  1;
		unsigned int				  u4Video_Opt21 				  : 	  1;
		unsigned int				  u4Video_Opt22 				  : 	  1;
		unsigned int				  u4Video_Opt23 				  : 	  1;
		unsigned int				  u4Video_Opt24 				  : 	  1;
		unsigned int				  fgLuma_WFld					  : 	  1;
		unsigned int				  fgCRC_Region					  : 	  1;
		unsigned int				  u4Video_Opt27 				  : 	  1;
		unsigned int				  u4Video_Opt28 				  : 	  1;
		unsigned int				  u4Video_Opt29 				  : 	  1;
		unsigned int				  u4Video_Opt30 				  : 	  1;
		unsigned int				  u4Video_Opt31 				  : 	  1;

		//	unsigned int			u4Video_Opt   :	32;   // 16: addrswap mode


		/* DWORD - 080 */
	//unsigned int							:	5;
		unsigned int			u4WAddrY			:	30;//PTR_WF_Y
		unsigned int							:	2;

		/* DWORD - 084 */
	//unsigned int							:	5;
		unsigned int			u4ZAddrY			:	30;//PTR_ZF_Y
		unsigned int							:	2;

		/* DWORD - 088 */
		unsigned int			u4FDIFF_TH3 					:	24;//
		unsigned int			fgMA4F							:	1;
		unsigned int			u4FDIFF_CTRL					:	1;
	unsigned int			u4REGION_BLENDING_MIN		:	1;
	unsigned int			MAX_PIX_PDIFF			:	1;
		unsigned int			fgBP_YC 						:	1;
		unsigned int			u4ASAW							:	2;
		unsigned int			fgMA6F							:	1;

		/* DWORD - 08C */
		unsigned int			u4FDIFF_TH2 					:	24;
		unsigned int			u4MA_Video_Mode0				:		1;
		unsigned int			u4MA_Video_Mode1				:		1;
		unsigned int			u4MA_Video_Mode2				:		1;
		unsigned int			u4MA_Video_Mode3				:		1;
		unsigned int			u4MA_Video_Mode4				:		1;
		unsigned int			u4MA_Video_Mode5				:		1;
		unsigned int			u4MA_Video_Mode6				:		1;
		unsigned int			u4MA_Video_Mode7				:		1;

		/* DWORD - 090 */
		unsigned int			u4FDIFF_TH1 					:	24;
		unsigned int			u4MA_HW_Option0 			:		1 ;
		unsigned int			u4MA_HW_Option1 			:		1 ;
		unsigned int			u4MA_HW_Option2 			:		1 ;
		unsigned int			u4MA_HW_Option3 			:		1 ;
		unsigned int			u4MA_HW_Option4 			:		1 ;
		unsigned int			u4MA_HW_Option5 			:		1 ;
		unsigned int			u4MA_HW_Option6 			:		1 ;
		unsigned int			u4MA_HW_Option7 			:		1 ;


		/* DWORD - 094 */
		unsigned int			u4TH_MIN_XZ 					:	10;
		unsigned int							:	2;
		unsigned int			u4TH_MED_XZ 					:	10;
		unsigned int							:	2;
		unsigned int			u4MA_TST_Mode				:		8 ;

		/* DWORD - 098 */
		unsigned int			u4TH_NM_XZ						:	10;
		unsigned int											:	2 ;
		unsigned int			u4TH_ED_XZ						:	10;
		unsigned int											:	2 ;
		unsigned int			u4H_ED_TH					:		8 ;

		/* DWORD - 09C */
		unsigned int			u4TH_MIN_YW 					:	9 ;
		unsigned int											:	3 ;
		unsigned int			u4TH_MED_YW 					:	9 ;
		unsigned int											:	3 ;
		unsigned int			u4SAW_TH						:	8 ;

		/* DWORD - 0A0 */
		unsigned int			u4TH_NM_YW						:	9 ;
		unsigned int											:	3 ;
		unsigned int			u4TH_ED_YW						:	9 ;
		unsigned int											:	3 ;
		unsigned int			u4WH_TX_TH						:	8 ;

		  /* DWORD - 0A4 */
		unsigned int			u4FCH_MIN_XZ					:	10 ;
		unsigned int											:	2 ;
		unsigned int			u4FCH_NM_XZ 					:	10;
		unsigned int											:	2 ;
		unsigned int			u4VMV_FCH						:	8 ;

		/* DWORD - 0A8 */
		unsigned int			u4FCH_MIN_YW					:	9 ;
		unsigned int											:	3 ;
		unsigned int			u4FCH_NM_YW 					:	9 ;
		unsigned int											:	1 ;
		unsigned int			u4CRM_LVL						:	2 ;
		unsigned int			u4FR_LVL						:	2 ;
		unsigned int			u4X_POS_ST						:	6 ;

		/* DWORD - 0AC */
		unsigned int			u4CRM_SAW						:	8 ;
		unsigned int			u4TV_LINE_ST					:	8 ;
		unsigned int			u4FD_CNT						:	8 ;
		unsigned int			u4MA_QUALITY_MODE				:	3 ;
		unsigned int			u4APPLY_TV_LINE_SET 			:	1 ;
		unsigned int			u4OTHERS						:	4 ;

		/* DWORD - 0B0 */
		unsigned int			u4EDGE_P_TH 					:	8 ;
		unsigned int			u4EDGE_VERT_TH					:	8 ;
		unsigned int			u4EDGE_CROSS_TH 				:	8 ;
		unsigned int			u4MA_EDGE_MODE0 				:	1 ;
		unsigned int			u4MA_EDGE_MODE1 				:	1 ;
		unsigned int			u4MA_EDGE_MODE2 				:	1 ;
		unsigned int			u4MA_EDGE_MODE3 				:	1 ;
		unsigned int			u4MA_EDGE_MODE4 				:	1 ;
		unsigned int			u4MA_EDGE_MODE5 				:	1 ;
		unsigned int			u4MA_EDGE_MODE6 				:	1 ;
		unsigned int			u4MA_EDGE_MODE7 				:	1 ;
		/* DWORD - 0B4 */
		unsigned int			u4EDGE_63D_TH					:	8 ;
		unsigned int			u4EDGE_ABS_GRAD_TH				:	8 ;
		unsigned int			u4EDGE_UD_RESTRICT_TH			:	8 ;
		unsigned int			u4MA_EDGE_ADV_CTRL				:	8 ;

		/* DWORD - 0B8 */ // 424b8 ege misc ellaneous thresholds
		unsigned int			u4EDGE_MULTI_EDGE_TH			:	8 ;
		unsigned int			u4EDGE_MEDGE_CNT_TH 			:	4 ;
		unsigned int							:	4 ;
		unsigned int			u4EDGE_3LINE_GRAD_TH			:	8 ;
		unsigned int			u4MA_EDGE_MISC					:	8 ;

		/* DWORD - 0BC */
		unsigned int			u4EDGE_HOR_DIFF_TH				:	8 ;
		unsigned int			u4EDGE_EXP_TH					:	8 ;
		unsigned int			u4EDGE_V3_CTRL					:	8 ;
		unsigned int			u4EDGE_V3_QUAL				  :	8 ;
		//unsigned int			u4EDGE_V3_QUAL_1				:	7 ;

		/* DWORD - 0C0 */ // 0x424c0 motion detection advance
		unsigned int			fgEXP_MOTION					:	1 ;
		unsigned int			fgEXP_STILL 					:	1 ;
		unsigned int			fgEXP_2PT						:	1 ;
		unsigned int			fgBLEND_EXP_OFF 				:	1 ;
		unsigned int			fgGET5F 						:	1 ;
		unsigned int			fgWA_NA24						:	1 ;
		unsigned int			fgSAW_5L						:	1 ; //[6]
		unsigned int			fgSAW_5LA						:	1 ;
		unsigned int			u4ADPT_ICP_TH					:	8 ;
		unsigned int			u4VAC_6F						:	4 ;
		unsigned int			fgMD_EXP						:	1 ;
		unsigned int			fgVT_BL 						:	1 ;
		unsigned int			fgOLD_SAW						:	1 ;
		unsigned int			fgMA5F							:	1 ;
		unsigned int			fgNO_M_W						:	1 ;	//[24] 6/8-field motion detection dram write disable
		unsigned int			fgMA8F_OR						:	1 ;	//[25] 8-field motion detection
		unsigned int			fgFIX_ICP						:	1 ; //[26]	fix ICP enable
		unsigned int			fgADPT_FIX_ICP					:	1 ;
		unsigned int			fgECTL_6F						:	1 ; //[28] 6/8-field motion also apply edge-preserving interpolation
		unsigned int			fgCUE_6F						:	1 ;
		unsigned int			fgBLEND_EXP_OFF_6F				:	1 ;
		unsigned int			fgSPTH_6F						:	1 ;

		/* DWORD - 0C4 */ // 0x424c4 pull down field like Not Use
		unsigned int			u4PD_LINE_UNLIKE_TH 			:	8 ;//
		unsigned int			u4PD_LINE_UNLIKE_INTV			:	5 ;
		unsigned int											:	3 ;
		unsigned int			u4PD_COMB_TH					:	7 ; 	// comb counter threshold
		unsigned int											:	1 ;
		unsigned int					fgVDO_32_PD_EN					:		1 ;
		unsigned int											:	1 ;
		unsigned int					fgVDO_32_PD_MODE				:		1 ;
		unsigned int											:	1 ;
		unsigned int			u4PD_CTRL_MODE_HD_EN			:	1 ;
		unsigned int			u4PD_CTRL_MODE_rest 			:	3 ;

		/* DWORD - 0C8 */ // 0x424c8 pull down band pass filter
		unsigned int			u4LUMA_KEY_TH					:	8 ; // [7:0] luma key threshold
		unsigned int			u4BD_BPF_THRD					:	8 ;
		unsigned int			u4FIFO_UNDERRUN_CNT 			:	7 ;
		unsigned int			u4FIFO_UNDERRUN_SEL 			:	1 ;
		unsigned int			fgPD_UL_SEL 					:	1 ;
		unsigned int			fgLM_KEY						:	1 ; // [25] luma key enable
		unsigned int			fgLUMAKEY_CHROMA_SEL			:	1 ;
		unsigned int											:	5 ;

		/* DWORD - 0CC */
		unsigned int			u4PD_DST_START					:	8 ;
		unsigned int			u4PD_DST_END					:	8 ;
		unsigned int			u4SUBTITLE_THRD 				:	8 ;
		unsigned int			fgSUBTITLE_ERASE_EN 			:	1 ;
		unsigned int			fgSUBTITLE_REG_EN				:	1 ;
		unsigned int			fgSUBTITLE_REG_VDO				:	1 ;
		unsigned int											:	5 ;

		/* DWORD - 0D0 */ // 0x424d0 choma motion detection
		unsigned int			fgCRM_3FMD						:	1 ; // motino 3-field motion detection
		unsigned int			fgCRM_FDIFF 					:	1 ;
		unsigned int			fgCRM_EXP_OFF					:	1 ;
		unsigned int			fgC_INTER_X 					:	1 ;
		unsigned int			fgC_VT_121						:	1 ;
		unsigned int			fgC_VT_BLEND					:	1 ;
		unsigned int			fgCRM_3LSAW 					:	1 ;
		unsigned int					fgCHROMA_SAW_CNT_4LINE_SEL		:	1 ;
		unsigned int			u4CRM_DIFF						:	8 ;
		unsigned int			fgFDIFF_LMT 					:	1 ;
		unsigned int			fgFDIFF_SAW 					:	1 ;
		unsigned int			u4FDIFF_ADJ 					:	2 ;
		unsigned int							:	4;
		unsigned int			u4CRM_MOTION_CNT_TH 			:	8 ;


		/* DWORD - 0D4 */
		unsigned int			u4PD_FLD_LIKE_TH				:	16;
		unsigned int			u4PD_SCN_CHG_TH 				:	8 ;
		unsigned int			u4DYN_8F_THRD					:	8 ;

		/* DWORD - 0D8 */
	unsigned int							:	5;
		unsigned int			u4MBAvg3						:	25;
		unsigned int							:	2 ;

		/* DWORD - 0DC */
	unsigned int							:	5;
		unsigned int			u4PROT_END_ADDR 				:	25;
		unsigned int							:	2 ;

		/* DWORD - 0E0 */ // 0x424E0
		unsigned int			u4DW_NEED_HD					:	9 ;//[8:0]
		unsigned int			u4MA_X_R_SCL					:	3 ;//[11:9]
		unsigned int			u4MA_Y_R_SC 					:	2 ;//[13:12]
		unsigned int			u4HD_CFG						:	6 ;//[19:14]
		unsigned int			fgHD_LINE_MODE					:	1 ;//[20]
		unsigned int			fgHD_MEM_1920					:	1 ;//[21]

		unsigned int			fgHD_MEM						:	1 ;  // 22
		unsigned int			fgSLE							:	1 ;  // [23] scan-line based dram address enable
		unsigned int			fgHD_EN 						:	1 ; // 24
		unsigned int			fgDN_FLT						:	1 ;
		unsigned int			fgND_MR_END 					:	1 ;
		unsigned int			fgSH_SRAM						:	1 ;
		unsigned int			fgSAW_4L						:	1 ;
		unsigned int			fgF_L_SEL						:	1 ;
		unsigned int			fgI_IN_P						:	1 ;
		unsigned int			fgF_PRGS						:	1 ;

		/* DWORD - 0E4 */
		unsigned int			u4RM_YF_Y						:	3 ;
		unsigned int			fgRM_YY_EN						:	1 ;
		unsigned int			u4RM_YF_C						:	3 ;
		unsigned int			fgRM_YC_EN						:	1 ;
		unsigned int			u4RM_XF_Y						:	3 ;
		unsigned int			fgRM_XY_EN						:	1 ;
		unsigned int			u4RM_XF_C						:	3 ;
		unsigned int			fgRM_XC_EN						:	1 ;
		unsigned int			u4RM_ZF_Y						:	3 ;
		unsigned int			fgRM_ZY_EN						:	1 ;
		unsigned int			u4RM_ZF_C						:	3 ;
		unsigned int			fgRM_ZC_EN						:	1 ;
		unsigned int			u4RM_WF_Y						:	3 ;
		unsigned int			fgRM_WY_EN						:	1 ;
		unsigned int			u4RM_AF_Y						:	3 ;
		unsigned int			fgRM_AY_EN						:	1 ;

	/* DWORD - 0E8 */
	unsigned int			fgRR_YY_EN						:	1 ;
	unsigned int			fgRR_YY_SEL 					:	1 ;
		unsigned int			fgRR_YC_EN						:	1 ;
	unsigned int			fgRR_YC_SEL 					:	1 ;
		unsigned int			fgRR_XY_EN						:	1 ;
	unsigned int			fgRR_XY_SEL 					:	1 ;
		unsigned int			fgRR_XC_EN						:	1 ;
	unsigned int			fgRR_XC_SEL 					:	1 ;
		unsigned int			fgRR_ZY_EN						:	1 ;
	unsigned int			fgRR_ZY_SEL 					:	1 ;
		unsigned int			fgRR_ZC_EN						:	1 ;
	unsigned int			fgRR_ZC_SEL 					:	1 ;
		unsigned int			fgRR_WY_EN						:	1 ;
	unsigned int			fgRR_WY_SEL 					:	1 ;
		unsigned int			fgRR_AY_EN						:	1 ;
	unsigned int			fgRR_AY_SEL 					:	1 ;
		unsigned int							:	16;


	/* DWORD - 0EC */
	//unsigned int							:	5;
	unsigned int			u4PTR_AF_Y						:	30;
		unsigned int							:	2 ;

		/* DWORD - 0F0 */
	unsigned int			u4FLD_WY_MOTION 				:	20;
	unsigned int			u4Motion_Level_24Region_1217	:	12;

		/* DWORD - 0F4 */
	unsigned int			u4FLD_WX_COMB					:	20;
	unsigned int			u4Motion_Level_24Region_1823	:	12;

		/* DWORD - 0F8 */
	unsigned int			u4WMV_DISABLE					:	32;

		/* DWORD - 0FC */
	//unsigned int							:	5;
	unsigned int			u4ZAddrC						:	30;
		unsigned int									:	2 ;


} HAL_VDO_FIELD_T;

typedef union _HAL_VDO_UNION_T {
	u32					au4Reg[VDP_HAL_VDO_REG_NUM];
	HAL_VDO_FIELD_T 		rField;
} HAL_VDO_UNION_T;



// 0x42F00/0x43F00
typedef struct _HAL_VDO_APQ_FIELD_T
{
		/* DWORD - 000 */ //0x42F00
		unsigned int			u4CRC_Chksum_00		:	24;
		unsigned int								:	4;
		unsigned int			fgInitCRC			:	1;
		unsigned int			fgClrCRC			:	1;
		unsigned int								:	2;

		/* DWORD - 004 */
		unsigned int			u4CRC_Chksum_01		:	24;//crc_result_dram
	unsigned int			u4State				:	4;
	unsigned int							:	4;

		/* DWORD - 008 */
		unsigned int			u4CRC_Chksum_02		:	24;//crc_result_y_field_lun
	unsigned int							:	8;

		/* DWORD - 00C */
		unsigned int			u4CRC_Chksum_03		:	24;//crc_result_y_field_chrome
	unsigned int							:	8;

		/* DWORD - 010 */
		unsigned int			u4CRC_Chksum_04		:	24;//crc_result_x_field_sram
	unsigned int							:	8;

		/* DWORD - 014 */
		unsigned int			u4CRC_Chksum_05		:	24;//crc_result_w_field_sram
	unsigned int							:	8;

		/* DWORD - 018 */
		unsigned int			u4CRC_Chksum_06		:	24;//crc_result_z_field_sram
	unsigned int							:	8;

		/* DWORD - 01C */ // 0x42F1C
		unsigned int			u4Frm_Motion_Sum		:	22;
		unsigned int								:	1;
		unsigned int			u422Film_Exit_Status		:	3;
		unsigned int			u432Film_Exit_Status		:	3;
		unsigned int								:	1;
		unsigned int			u432Film_Status			:	1;
		unsigned int			u422Film_Status			:	1;

		/* DWORD - 020 */ // 0x42F20
		unsigned int					fgMskStaLineT					:		11;
		unsigned int													:		5;
		unsigned int					fgMskStaLineB					:		11;
		unsigned int													:		4;
		unsigned int					fgRMsk							:		1;

		/* DWORD - 024 */
		unsigned int					fgEndLineT						:		11;
		unsigned int													:		5;
		unsigned int					fgEndLintB						:		11;
		unsigned int													:		5;

		/* DWORD - 028 */
		unsigned int													:		32;

		/* DWORD - 02C */
		unsigned int													:		32;

		// IC >= 8530
		/* DWORD - 030 */
		unsigned int					u4CombCounter					:		22;
		unsigned int													:		10;

		/* DWORD - 034 */
		unsigned int					u4EdgeFieldSawStatus			:		22;
		unsigned int													:		10;

		/* DWORD - 038 */
		unsigned int					u4MotionBlks					:		9; // [8:0] Sum of Motion Block
		unsigned int													:		23;

		/* DWORD - 03C */ // 0x42F3C Film Status 0B
		unsigned int			u4Saw_Fld_Mo_Sq_32			:	1;//
		unsigned int			u4Edge_Fld_Mo_Sq_32 		:	1;
		unsigned int			u4Saw_Fld_Mo_Sq_22			:	1;
		unsigned int			u4Edge_Fld_Mo_Sq_22 			:	1;
		unsigned int			u4Frm_Statistic_Sq		:	1; // [4] Frame Statics Sequence Status
		unsigned int			u4Frm_Motion_Sq			:	1; // [5]
		unsigned int			u4Frm_Similar_Sq		:	1; // [6] Frame Similar Sequence Status
		unsigned int			u432_SawFld_Sq				:	1; // [7] 32 Film Detector - SawField Statistic Sequence Status
		unsigned int			u4Fld_Similar_Sq		:	1; // [8] Field Simular Sequence Status
		unsigned int			u432_EdgeFld_Sq			:	1;
		unsigned int			u422_SawFld_Sq			:	1;
		unsigned int			u422_EdgeFld_Sq			:	1;
		unsigned int											:	20;

		/* DWORD - 040 */ //different
		unsigned int					u4PD22CombSel					:		3;//[2:0]//
		unsigned int					u4PDSelBField					:		1;//[3]
		unsigned int					u4BFWeight						:		3;//[6:4]
		unsigned int					u4ForceBFSel					:		1;//[7]
		unsigned int					u4FRameSelInv					:		1;//[8]
		unsigned int													:		3;//[11:9]
		unsigned int					u4EdgeP_Level_Y_Th				:		4;//[15:12]
		unsigned int					u4EdgeP_Level_C_Th				:		4;//[19:16]
		unsigned int					u4New_Deinter_Out_Mode			:		1;//[20]
		unsigned int					fgFUSION_OUT_DISABLE_Y			:		1;
		unsigned int					fgFUSION_OUT_DISABLE_C			:		1;
		unsigned int					fgFUSION_OUTPUT_DATA_EN 		:		1;
		unsigned int					u4Frame_Fake_Saw_En 			:		1;
		unsigned int													:		6;
		unsigned int					u4Ouput_Motion_Level_Test_Mode	:		1;

		/* DWORD - 044 */
		unsigned int					u4Ma6f_Saw_Option				:		1;
		unsigned int													:		1;//
		unsigned int					u4YZ_Field_Sawtooth_En			:		1;
		unsigned int					u4Sawtooth_3Line_Option 		:		5;
		unsigned int					u4Saw_Cnt_Line__Th				:		12;
		unsigned int					u4Saw_Cnt_Pixel_Th				:		12;

		/* DWORD - 048 */
		unsigned int											:	 32;

		/* DWORD - 04C */// no use
		unsigned int			u4VEdgeth					:	8;
		unsigned int			u4SelNewMd					:	1;
		unsigned int			u4SelNewPixDif				:	1;
		unsigned int			u4SelNewHEdgeDet			:	1;
		unsigned int			u4EnVEdgeMag					:	1;
		unsigned int			u4SelBigVEdge					:	1;
		unsigned int			u4FrameVEdge					:	1;
		unsigned int			u4MagSawOnly					:	1;
		unsigned int			u4NoSpMoveChk					:	1;
		unsigned int			u4UseUpDownClamp				:	1;
		unsigned int			u4ClampNoMagnify				:	1;
		unsigned int			u4LRDiffMono					:	1;
		unsigned int											:	1;
		unsigned int			u4HDiffLvRange					:	2;
		unsigned int			u4ClampMethod					:	2;
		unsigned int			u4MagMethod 					:	2;
		unsigned int											:	6;

		/* DWORD - 050 */ // 0x42F50
		unsigned int			u4CRM_Motion_Cnt			:	20;//[19:0] chroma motion counter between X-Z field
		unsigned int											:	4;
		unsigned int			u4Sum_24_Motion_Level			:	7;
		unsigned int											:	1;

		/* DWORD - 054 */
	unsigned int							:	5;
		unsigned int					fgPRT_STA_ADDR				   :	   23;
		unsigned int													:		4;

		/* DWORD - 058 */
		unsigned int													:		32;

		/* DWORD - 05c */
		unsigned int					u4RBound						:		10;
		unsigned int													:		6;
		unsigned int					u4BBound						:		10;
		unsigned int													:		6;

		/* DWORD - 060 */
		unsigned int													:		24;
		unsigned int					u4FrMoCurThd					:		8; // [31:24] Frame Motion Current Pixel Threshold_2

		/* DWORD - 064 */
		unsigned int													:		32;

		/* DWORD - 068 */
		unsigned int													:		32;

		/* DWORD - 06C */
		unsigned int													:		32;

		/* DWORD - 070 */
		unsigned int													:		32;

		/* DWORD - 074 */
		unsigned int													:		32;

		/* DWORD - 078 */
		unsigned int													:		32;

		/* DWORD - 07C */
		unsigned int													:		32;

		/* DWORD - 080 */
		unsigned int													:		32;

		/* DWORD - 084 */
		unsigned int													:		32;

		/* DWORD - 088 */
		unsigned int					u4Max_TH						:		16;
		unsigned int					u4Min_TH						:		16;

		/* DWORD - 08C */
		unsigned int													:		32;

		/* DWORD - 090 */
		unsigned int													:		32;

		/* DWORD - 094 */
		unsigned int													:		32;

		/* DWORD - 098 */
		unsigned int													:		32;

		/* DWORD - 09c */
		unsigned int													:		16;
		unsigned int					u4LBound						:		10;
		unsigned int													:		6;

		/* DWORD - 0A0 */
		unsigned int					u4UBound						:		10;
		unsigned int													:		22;

		/* DWORD - 0A4 */
		unsigned int													:		32;

		/* DWORD - 0A8 */
		unsigned int													:		32;

		/* DWORD - 0AC */ // 0x42FAC
		unsigned int					u4FrMoPxlDiffThd				:		8;// [7:0] Frame Motion Detector - Pixel Difference Threshold
		unsigned int													:		24;

		/* DWORD - 0B0 */
		unsigned int													:		32;

		/* DWORD - 0B4 */
		unsigned int					u4Dyn_th1						:		9;
		unsigned int													:		2;
		unsigned int					u4Dyn_th2						:		9;
		unsigned int					u4Dyn_th3						:		9;
		unsigned int													:		3;

		/* DWORD - 0B8 */
		unsigned int													:		6;
		unsigned int					u4Edge_saw_type1				:		2;
		unsigned int													:		24;

		/* DWORD - 0BC */
		unsigned int													:		32;

		/* DWORD - 0C0 */
		unsigned int													:		32;
		unsigned int													:		32;
		unsigned int													:		32;
		unsigned int													:		32;

		/* DWORD - 0D0 */
		unsigned int													:		32;
		unsigned int													:		32;
		unsigned int													:		32;
		unsigned int													:		32;

		/* DWORD - 0E0 */ // 0x42FE0
		unsigned int					u4Chroma_Saw_Cnt_XY 			:		20; // [19:0] chroma sawthooth counter between X-Y fields
		unsigned int													:		12;

		/* DWORD - 0E4 */
		unsigned int					u4Single_Line_Th_Low			:		10;    //This 0~6 bit is for ECO item in 8560
		unsigned int													:		6;
		unsigned int					u4Single_Line_Th_High			:		10;
		unsigned int													:		5;
		unsigned int					u4Single_Line_Detect_Disable	:		1;		//This bit is for ECO item in 8560

		/* DWORD - 0E8 */
		unsigned int													:		32;

		/* DWORD - 0EC */ //0x42FEC
		unsigned int					u4Demo_Repeat_Width 			:		8;
		unsigned int													:		16;
		unsigned int					u4Demo_Mddi_Simple_Enable		:		1; //[24]
		unsigned int					u4Demo_Sel_Intra_Inter			:		1;
		unsigned int					u4Demo_New_Md					:		1;
		unsigned int					u4Demo_Be_2_4_Line				:		1;
		unsigned int					fgDEMO_MEMA_OLD_MA				:		1;	 //name co-used with 8555
		unsigned int													:		2;
		unsigned int					u4Demo_Repeat_Enable			:		1; //[31] enable demo repeate mode

		/* DWORD - 0F0 */
		unsigned int					u4Clamp_Spa_Temp_Diff_Sel		:		3;
		unsigned int					u4Clamp_Refer_Large 			:		1;
		unsigned int					u4Clamp_Spadiff_Sel 			:		1;
		unsigned int					u4Clamp_4line_Always_Add		:		1;
		unsigned int					u4Clamp_4line_Con_En			:		1;
		unsigned int					u4Clamp_Dis_Con_En				:		1;
		unsigned int					u4Clamp_Dis_Option				:		1;
		unsigned int													:		11;
		unsigned int					u4Clamp_4line_Diff_Th			:		12;

		/* DWORD - 0F4 */
		unsigned int					u4Multi_Burst_Cnt_Th			:		8; // 0 - 7
		unsigned int					u4Multi_Burst_Diff_Sel			:		1; // 8
		unsigned int					u4blend_Exp_Off_Sel 			:		1; // 9
		unsigned int					u4Force_Still_With_EdgeP_En 	:		1; // 10
		unsigned int					u4Single_Line_Detection_Dis 	:		1; // 11
		unsigned int													:		1; // 12
		unsigned int													:		1; // 13
		unsigned int					u4Pre_Fetch_Line_Still_Dis		:		1; // 14
		unsigned int													:		1; // 15
		unsigned int													:		1; // 16
		unsigned int					u4Mema_Chroma_Bk_En 			:		1; // 17		 //co-used with 8555
		unsigned int					u4Chroma_Multi_Burst_En 		:		1; // 18
		unsigned int					u4Chroma_Multi_Burst_Pixel_Sel	:		1; // 19
		unsigned int													:		1; // 20
		unsigned int					u4Jaggy_smooth_enable			:		1; // 21
		unsigned int													:		1; // 22
		unsigned int													:		1; // 23
		unsigned int					u4Chroma_Multi_Burst_Threshold	:		8;	// 24-31

		/* DWORD - 0F8 */
		unsigned int													:		32;

		/* DWORD - 0FC */
		unsigned int					u4Multi_Edge_Cnt				:		20;
		unsigned int					u4Line_Saw_Cnt_Total			:		12;
} HAL_VDO_APQ_FIELD_T;

typedef union _HAL_VDO_APQ_UNION_T
{
	u32					au4Reg[HAL_VDO_APQ_REG_NUM];
	HAL_VDO_APQ_FIELD_T	rField;
} HAL_VDO_APQ_UNION_T;

#define VDP_1 0   /* Front*/
#define VDP_2 1   /* Rear*/


#define GET_VDP_HW_PTR(id, reg) \
	do { \
		if (id == VDP_1) { \
			reg = (HAL_VDO_UNION_T *) vdof_reg; \
		} else { \
			return; \
		} \
	} while (0)

#define GET_VDP_PTR(id, reg, mode) \
	do { \
		if (id == VDP_1) { \
			reg = &_rVdp1SwReg; \
			mode = _rVdp1RegMode; \
		} else { \
			return; \
		} \
	} while (0)

#define GET_VDP_PTR_REGION(id, reg, mode, src, out) \
	do { \
		if (id == VDP_1) { \
			reg = &_rVdp1SwReg; \
			mode = _rVdp1RegMode; \
			src = &_rVdp1SrcRegion; \
			out = &_rVdp1OutRegion; \
		} else { \
			return; \
		} \
	} while (0)

#define GET_PMX_DISP_PTR(id, reg, mode) \
	do { \
		if (id == VDP_1) { \
			reg.pDispMain = &_rPmxDispMainSwReg; \
			mode	  = _rPmxDispMainRegMode; \
		} else { \
			return; \
		} \
	} while (0)

#define GET_VDP_APQ_PTR(id, reg, mode) \
						do { \
								if (id == VDP_1) { \
										reg = &_rVdp1ApqSwReg; \
										mode = _rVdp1ApqRegMode; \
								} else { \
										return; \
								} \
						} while (0)

#endif	/* _VDP_HW_H_*/









