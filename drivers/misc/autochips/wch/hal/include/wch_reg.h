/*
 *Department: ATC-SD-SD1
 *
 *Aurthor: Mingxu Wang (atc0027)
 *
 *Date: 2017-7-26
 */

 #ifndef _WCH_REG_H_
 #define _WCH_REG_H_


#define IDX_VDOIN_EN			(0x00/4)
#define IDX_VDOIN_MODE			(0x04/4)
#define IDX_YBUF0_ADDR			(0x08/4)
#define IDX_ACT_LINE			(0x0c/4)
#define IDX_CBUF0_ADDR			(0x10/4)
#define IDX_DW_NEED				(0x14/4)
#define IDX_HPIXEL				(0x18/4)
#define IDX_HBLACK				(0x1c/4)
#define IDX_INPUT_CTRL			(0x20/4)
#define IDX_TOP_BOT_START_LINE	(0x24/4)
#define IDX_WRAPPER_3D			(0x28/4)
#define IDX_WRAPPER_3D_VSYNC	(0x2c/4)
#define IDX_WRAPPER_3D_SETTING	(0x30/4)
#define IDX_HCNT_SETTING		(0x34/4)
#define IDX_HCNT_SETTING_1		(0x38/4)
#define IDX_VSCALE				(0x3c/4)
#define IDX_HSCALE				(0x40/4)
#define IDX_DEBUG_PORT_1		(0x44/4)
#define IDX_DEBUG_PORT_2		(0x48/4)
#define IDX_DEBUG_PORT_3		(0x4c/4)
#define IDX_DEBUG_PORT_4		(0x50/4)
#define IDX_DRAM_PROTECT1		(0x58/4)
#define IDX_DRAM_PROTECT2		(0x5c/4)
#define IDX_DRAM_BYTE_EN		(0x60/4)
#define IDX_YBUF1_ADDR			(0x64/4)
#define IDX_CBUF1_ADDR			(0x68/4)
#define IDX_YBUF2_ADDR			(0x6c/4)
#define IDX_CBUF2_ADDR			(0x70/4)
#define IDX_YBUF3_ADDR			(0x74/4)
#define IDX_CBUF3_ADDR			(0x78/4)
#define IDX_VDOIN_REQOUT		(0x7C/4)
#define IDX_VDOIN_YCSAMPLE		(0x80/4)
#define IDX_VDOIN_DUMP			(0x84/4)
#define IDX_BLOCKMODE			(0x88/4)

#define OFFSET_CLOCK_CTL		(0x100)
#define OFFSET_RESET_CTL		(0x130)

#define WCH_BIT(x)				(1U << (x))
#define HAL_WRITE32(reg, value)		*(volatile u32 *)(reg) = value
#define HAL_READ32(reg)			*(volatile u32 *)(reg)

#define WCH_READ32(base, offset)		HAL_READ32((base + offset))
#define WCH_WRITE32(base, offset, value)	HAL_WRITE32((base + offset), (value))

#define SET_BIT(addr, Bit) \
	do { \
		volatile u32 tv = *(volatile u32 *)(addr); \
		tv |= (Bit); \
		*(unsigned int *)(addr) = tv; \
	} while (0)


#define WCH_SET_BIT(base, offset, Bit) \
	do { \
		volatile u32 tv = WCH_READ32(base, offset); \
		tv |= (Bit); \
		WCH_WRITE32(base, offset, tv); \
	} while (0)

#define WCH_CLR_BIT(base, offset, Bit) \
	do { \
		volatile u32 tv = WCH_READ32(base, offset); \
		tv &= (~(Bit)); \
		WCH_WRITE32(base, offset, tv); \
	} while (0)


struct VDOIN_EN_REG {/* 0x00 */
/*0*/	 u32 VI_EN : 1;
/*1*/	 u32 Vdoindata_test : 1;
/*2*/	 u32 BGVdoformat : 1;
/*3*/	 u32 PRGS : 1;
/*4*/	 u32 SRAM_EN_SEL : 1;
		 u32 VDOIN_EN_reserved1 : 1;
/*6*/	 u32 ExtEAv : 1;
	 	 u32 VDOIN_EN_reserved2 : 3;
/*10*/	 u32 BGenVsyn601Inv : 1;
/*11*/	 u32 sd_480I_mix : 1;
/*12*/	 u32 FldInv : 1;
	 	 u32 VDOIN_EN_reserved3 : 1;
/*14*/	 u32 Linear_ena : 1;
/*15*/	 u32 addr_swap_1 : 1;
	 	 u32 VDOIN_EN_reserved4 : 2;
/*19:18*/u32 one_plane_mode : 2;
/*20*/	 u32 one_plane_ena : 1;
/*21*/ 	 u32 Internal_data_ena : 1;
	 	 u32 VDOIN_EN_reserved5 : 1;
/*24:23*/u32 SWAP : 2;
/*25*/	 u32 mode_422 : 1;
/*26*/	 u32 Fram_clr : 1;
/*27*/	 u32 Fld0dis : 1;
/*28*/	 u32 Fld1dis : 1;
	 	 u32 VDOIN_EN_reserved6 : 2;
/*31*/	 u32 Field_int : 1;
};

struct VDOIN_MODE_REG {/* 0x04 */
	 	 u32 VDOIN_MODE_reserved1 : 4;
/*6:4*/	 u32 Fifo_THRES : 3;
	 	 u32 VDOIN_MODE_reserved2 : 20;
/*27*/	 u32 ReqActNew : 1;
/*28*/	 u32 ReqNew : 1;
/*29*/	 u32 YCFieldEnd : 1;
/*30*/	 u32 MaskEn : 1;
	 	 u32 VDOIN_MODE_reserved3 : 1;
};

struct YBUF0_ADDR_REG {/* 0x08 */
/*29:0*/	u32 YBUF0_ADDR : 30;
		 	u32 YBUF0_ADDR_reserved4 : 2;
};

struct ACT_LINE_REG {/* 0x0C */
/*11:0*/	u32 ACTLINE : 12;
/*18:12*/	u32 BGVSyn601Det : 7;
/*19*/		u32 IndexMode : 1;
/*27:20*/	u32 IndexModeThres : 8;
	 	 	u32 ACT_LINE_reserved1 : 4;
};

struct CBUF0_ADDR_REG {/* 0x10 */
/*29:0*/	u32 CBUF0_ADDR : 30;
	 	 	u32 CBUF0_ADDR_reserved1 : 2;
};

struct DW_NEED_REG {/* 0x14 */
/*11:0*/	u32 DW_NEED_Y_LINE : 12;
	 	 	u32 DW_NEED_reserved1 : 4;
/*27:16*/	u32 DW_NEED_C_LINE : 12;
	 	 	u32 DW_NEED_reserved2 : 4;
};

struct HPIXEL_REG {/* 0x18 */
/*12:0*/	u32 NPIXEL : 13;
	 	 	u32 HPIXEL_reserved1 : 3;
/*28:16*/	u32 NPIXEL_PRE : 13;
	 	  	u32 HPIXEL_reserved2 : 3;
};

struct HBLACK_REG {/* 0x1C */
/*9:0*/  u32 MASK_CNT : 10;
	 	 u32 HBLACK_reserved1 : 6;
/*27:16*/u32 bgactiveline_1 : 12;
	 	 u32 HBLACK_reserved2 : 4;
};

struct INPUT_CTRL_REG {/* 0x20 */
/*1:0*/	 u32 vdoin_Y_chn_sel : 2;
/*3:2*/	 u32 vdoin_CB_chn_sel : 2;
/*5:4*/	 u32 vdoin_CR_chn_sel : 2;
/*6*/	 u32 vdoin_inserve_bit : 1;
/*7*/	 u32 vdoin_12bit_mode : 1;
/*8*/	 u32 vdoin_10bit_mode : 1;
/*9*/	 u32 vdoin_444_mode : 1;
/*10*/	 u32 vdoin_c_del_sel : 1;
/*11*/	 u32 vdoin_c2_del_sel : 1;
/*13:12*/u32 pxl_sel : 2;
/*14*/	 u32 h_edge_sel : 1;
/*15*/	 u32 BGenVsyn_Inv : 1;
/*16*/	 u32 BGenHsyn_Inv : 1;
	 	 u32 INPUT_CTRL_reserved1 : 1;
/*19:18*/u32 YCin_del_sel_bit_1_0 : 2;
/*21:20*/u32 Cin_del_sel : 2;
/*22*/	 u32 c_sample_inv : 1;
/*23*/	 u32 sd_4fs_opt : 1;
/*24*/	 u32 YCin_del_sel_bit_2 : 1;
/*25*/	 u32 skip_line_en : 1;
/*26*/	 u32 new_int_ena : 1;
/*27*/	 u32 vdoin_3d_wrap : 1;
/*28*/	 u32 hqwrena_2t_syn : 1;
/*29*/	 u32 hqwrena_2t : 1;
/*30*/	 u32 sd_444_2fs : 1;
/*31*/	 u32 sd_480i_mix_eco : 1;
};

struct TOP_BOT_START_LIN_REG {/* 0x24 */
/*11:0*/	u32 TL : 12;
	 	 	u32 TOP_BOT_START_LIN_reserved1 : 4;
/*27:16*/	u32 BL : 12;
	 	 	u32 TOP_BOT_START_LIN_reserved2 : 4;
};

struct WRAPPER_3D_REG {/* 0x28 */
/*11:0*/	u32 htotal : 12;
	 	 	u32 WRAPPER_3D_reserved1 : 4;
/*27:16*/	u32 vtotal : 12;
	 	 	u32 WRAPPER_3D_reserved2 : 4;
};

struct WRAPPER_3D_VSYNC_REG {/* 0x2C */
	 	 u32 WRAPPER_3D_VSYNC_reserved1 : 16;
/*27:16*/u32 vsync_sta : 12;
	 	 u32 WRAPPER_3D_VSYNC_reserved2 : 4;
};

struct WRAPPER_3D_SETTING_REG {/* 0x30 */
/*7:0*/  u32 vdoin_3d_vsyn_width : 8;
	 	 u32 WRAPPER_3D_SETTING_reserved1 : 8;
/*23:16*/u32 vdoin_3d_hsyn_width : 8;
/*24*/	 u32 vdoin_3d_hsyn_in_polarity : 1;
/*25*/	 u32 vdoin_3d_vsyn_in_polarity : 1;
/*26*/	 u32 vdoin_3d_timing_free_run : 1;
/*27*/	 u32 vdoin_3d_timing_reset : 1;
	 	 u32 WRAPPER_3D_SETTING_reserved2 : 2;
/*31:30*/u32 swrst : 2;
};

struct HCNT_SETTING_REG {/* 0x34 */
/*12:0*/	 u32 HCNT : 13;
	 	 u32 HCNT_SETTING_reserved1 : 3;
/*28:16*/u32 HACTCNT : 13;
	 	 u32 HCNT_SETTING_reserved2 : 3;
};

struct HCNT_SETTING_1_REG {/* 0x38 */
/*9:0*/	 u32 YHCNT : 10;
	 	 u32 HCNT_SETTING_1_reserved1 : 6;
/*25:16*/u32 CHCNT : 10;
	 	 u32 HCNT_SETTING_1_reserved2 : 6;
};

struct VSCALE_REG {/* 0x3C */
/*0*/	 u32 fld_reset : 1;
/*1*/	 u32 Vsyn_reset : 1;
/*2*/	 u32 Vdifld_reset : 1;
	 	 u32 VSCALE_reserved1 : 2;
/*5*/	 u32 Fld_int : 1;
/*6*/	 u32 Vsysn_int : 1;
/*7*/	 u32 Vdifld_int : 1;
/*8*/	 u32 Vsynccena : 1;
/*9*/	 u32 Int720p : 1;
/*10*/	 u32 Linest_ena : 1;
/*11*/	 u32 bgaddr_sel : 1;
/*12*/	 u32 bgsramcs_ena : 1;
/*13*/	 u32 bgtest_bar : 1;
/*14*/	 u32 Vdifld_int2 : 1;
	 	 u32 VSCALE_reserved2 : 1;
/*25:16*/u32 bghsize_dw : 10;
	 	 u32 VSCALE_reserved3 : 6;
};

struct HSCALE_REG {/* 0x40 */
/*11:0*/	u32 BGTopLine_1 : 12;
	 	 	u32 HSCALE_reserved1 : 4;
/*27:16*/	u32 BGBottomLine_1 : 12;
	 	 	u32 HSCALE_reserved2 : 4;
};

struct DEBUG_PORT_REG {/* 0x44 */
/*3:0*/  u32 Inbuf_state : 4;
	 	 u32 DEBUG_PORT_reserved1 : 4;
/*13:8*/	 u32 Outctl_state : 6;
	 	 u32 DEBUG_PORT_reserved2 : 2;
/*19:16*/u32 AV_STATE : 4;
	 	 u32 DEBUG_PORT_reserved3 : 4;
/*24*/	 u32 BLV_hsync : 1;
/*25*/	 u32 BLV_vsync : 1;
/*26*/	 u32 BLV_field : 1;
/*27*/	 u32 frame : 1;
/*28*/	 u32 AV_flag : 1;
/*29*/	 u32 H_cnt_cyc : 1;
/*30*/	 u32 Field_inv_flag : 1;
/*31*/	 u32 Src_type : 1;
};

struct DEBUG_PORT_2_REG {/* 0x48 */
/*6:0*/  	u32 Hq1udcnt : 7;
			u32 DEBUG_PORT_1_reserved1 : 1;
/*14:8*/ 	u32 Hq2udcnt : 7;
			u32 DEBUG_PORT_1_reserved2 : 1;
/*16*/   	u32 Frm_cnt : 1;
	 	 	u32 DEBUG_PORT_1_reserved3 : 1;
/*18*/   	u32 nxt_frm_cnt : 1;
	 	 	u32 DEBUG_PORT_1_reserved4 : 1;
/*20*/   	u32 frame : 1;
/*21*/   	u32 src_type : 1;
	 	 	u32 DEBUG_PORT_1_reserved5 : 10;
};

struct DEBUG_PORT_3_REG {/* 0x4C */
/*0*/	u32 wr_req_org : 1;
/*1*/	u32 wr_last_org : 1;
/*2*/	u32 wr_req : 1;
/*3*/	u32 wr_last : 1;
/*4*/	u32 Hd2req : 1;
/*5*/	u32 Hd1req : 1;
		u32 DEBUG_PORT_3_reserved1 : 26;
};

struct DEBUG_PORT_4_REG {/* 0x50 */
/*12:0*/ u32 h_total_detect : 13;
	 	 u32 INDEBUG_STATUS_reserved1 : 3;
/*27:16*/u32 v_total_detect : 12;
/*28*/	 u32 field_signal : 1;
	 	 u32 INDEBUG_STATUS_reserved2 : 3;
};

struct DRAM_PROTECT1_REG {/* 0x58 */
/*27:0*/	u32 Mgprotect_ptrt : 28;
	 		u32 DRAM_PROTECT1_reserved1 : 1;
/*29*/		u32 Mgprotect_ena1 : 1;
/*30*/		u32 Mgprotect_ena : 1;
/*31*/		u32 Mgprotect_clr : 1;
};

struct DRAM_PROTECT2_REG {/* 0x5C */
/*27:0*/ u32 Mgprotect_ptrb : 28;
	 	 u32 DEBUG_PORT_5_reserved1 : 3;
/*31*/	 u32 Overflow_flag : 1;
};

struct DRAM_BYTE_EN_REG {/* 0x60 */
	 	 u32 DRAM_BYTE_EN_reserved1 : 8;
/*23:8*/ u32 byte_data : 16;
/*24*/   u32 byte_risc_ena : 1;
	 	 u32 DRAM_BYTE_EN_reserved2 : 7;
};

struct YBUF1_ADDR_REG {/* 0x64 */
/*29:0*/ u32 YBUF1_ADDR : 30;
	 	 u32 YBUF1_ADDR_reserved1 : 2;
};

struct CBUF1_ADDR_REG {/* 0x68 */
/*29:0*/ u32 CBUF1_ADDR : 30;
	 	 u32 CBUF1_ADDR_reserved1 : 2;
};

struct YBUF2_ADDR_REG {/* 0x6C */
/*29:0*/ u32 YBUF2_ADDR : 30;
	 	 u32 YBUF2_ADDR_reserved1 : 2;
};

struct CBUF2_ADDR_REG {/* 0x70 */
/*29:0*/ u32 CBUF2_ADDR : 30;
	 	 u32 CBUF2_ADDR_reserved1 : 2;
};

struct YBUF3_ADDR_REG {/* 0x74 */
/*29:0*/ u32 YBUF3_ADDR : 30;
	 	 u32 YBUF3_ADDR_reserved1 : 2;
};

struct CBUF3_ADDR_REG {/* 0x78 */
/*29:0*/ u32 CBUF3_ADDR : 30;
	 	 u32 CBUF3_ADDR_reserved1 : 2;
};

struct VDIN_REQOUT_REG {/* 0x7C */
/*22:0*/ 	u32 CH_PROTECT_THRES : 23;
/*23*/   	u32 CH_PROTECT_DISABLE : 1;
/*24*/   	u32 CH_REQ_EMPTY : 1;
/*26:25*/	u32 BGYCADDR_AUTO_CNT : 2;
/*27*/		u32 BGYCADDR_AUTO_CNT_CLR : 1;
/*30:28*/  u32 BGYCADDR_AUTO_CFG : 3;
/*31*/   	u32 BGYCADDR_AUTO_EN : 1;
};

struct VDOIN_YCSAMPLE_REG {/* 0x80 */
/*7:0*/  u32 Y_PATTERN_VALUE : 8;
/*23:8*/ u32 C_PATTERN_VALUE : 16;
/*24*/	  u32 Y_PATTERN_EN : 1;
/*25*/   u32 C_PATTERN_EN : 1;
/*26*/   u32 C_SAMPLE_EN : 1;
/*27*/   u32 ext_field : 1;
/*29:28*/u32 YC_LATCH_MODE : 2;
/*30*/   u32 ch_vrst_en : 1;
	 	  u32 VDOIN_YCSAMPLE_reserved1 : 1;
};

struct VDOIN_DUMP_REG {/* 0x84 */
/*29:0*/ u32 debug_depth : 30;
/*30*/   u32 debug_end_clr : 1;
/*31*/   u32 debug_mode : 1;
};

struct VDOIN_BLOCKMODE_REG {/* 0x88 */
/*12:0*/	u32 BlockActHcnt : 13;
	 		u32 VDOIN_BLOCKMODE_reserved1 : 19;
};

struct WCH_REG {
	struct VDOIN_EN_REG VDOIN_EN;/* 0x00 */
	struct VDOIN_MODE_REG VDOIN_MODE;/* 0x04 */
	struct YBUF0_ADDR_REG YBUF0_ADDR;/* 0x08 */
	struct ACT_LINE_REG ACT_LINE;/* 0x0C */
	struct CBUF0_ADDR_REG CBUF0_ADDR;/* 0x10 */
	struct DW_NEED_REG DW_NEED;/* 0x14 */
	struct HPIXEL_REG HPIXEL;/* 0x18 */
	struct HBLACK_REG HBLACK;/* 0x1C */
	struct INPUT_CTRL_REG INPUT_CTRL;/* 0x20 */
	struct TOP_BOT_START_LIN_REG TOP_BOT_START_LIN;/* 0x24 */
	struct WRAPPER_3D_REG WRAPPER_3D;/* 0x28 */
	struct WRAPPER_3D_VSYNC_REG WRAPPER_3D_VSYNC;/* 0x2C */
	struct WRAPPER_3D_SETTING_REG WRAPPER_3D_SETTING;/* 0x30 */
	struct HCNT_SETTING_REG HCNT_SETTING;/* 0x34 */
	struct HCNT_SETTING_1_REG HCNT_SETTING_1;/* 0x38 */
	struct VSCALE_REG VSCALE;/* 0x3C */
	struct HSCALE_REG HSCALE;/* 0x40 */
	struct DEBUG_PORT_REG DEBUG_PORT;/* 0x44 */
	struct DEBUG_PORT_2_REG DEBUG_PORT_2;/* 0x48 */
	struct DEBUG_PORT_3_REG DEBUG_PORT_3;/* 0x4C */
	struct DEBUG_PORT_4_REG DEBUG_PORT_4;/* 0x50 */
	u32 reserved1;
	struct DRAM_PROTECT1_REG DRAM_PROTECT1;/* 0x58 */
	struct DRAM_PROTECT2_REG DRAM_PROTECT2;/* 0x5C */
	struct DRAM_BYTE_EN_REG DRAM_BYTE_EN;/* 0x60 */
	struct YBUF1_ADDR_REG YBUF1_ADDR;/* 0x64 */
	struct CBUF1_ADDR_REG CBUF1_ADDR;/* 0x68 */
	struct YBUF2_ADDR_REG YBUF2_ADDR;/* 0x6C */
	struct CBUF2_ADDR_REG CBUF2_ADDR;/* 0x70 */
	struct YBUF3_ADDR_REG YBUF3_ADDR;/* 0x74 */
	struct CBUF3_ADDR_REG CBUF3_ADDR;/* 0x78 */
	struct VDIN_REQOUT_REG VDIN_REQOUT;/* 0x7C */
	struct VDOIN_YCSAMPLE_REG VDOIN_YCSAMPLE;/* 0x80 */
	struct VDOIN_DUMP_REG VDOIN_DUMP;/* 0x84 */
	struct VDOIN_BLOCKMODE_REG VDOIN_BLOCKMODE;/* 0x88 */
};

#define WCH_REG_NUM		(0x8c/4)
typedef union _WCH_REG_UNION_T {
	u32 reg[WCH_REG_NUM];
	struct WCH_REG reg_filed;
}WCH_REG_UNION_T,*PWCH_REG_UNION_T;

#define WCH_HAL_WRITE32(_reg_, _val_)   		(*((volatile u32*)(_reg_)) = (_val_))
#define WCH_HAL_READ32(_reg_)           		(*((volatile u32*)(_reg_)))

#endif
