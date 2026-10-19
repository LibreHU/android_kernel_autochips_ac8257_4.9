/*
 *Copyright (c) 2016 AutoChips Inc.
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License version 2 as
 *published by the Free Software Foundation.
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *tvd-reg_base.h
 *define tvd register
 */
 

#ifndef TVD_REG_BASE_H
#define TVD_REG_BASE_H

#define TVD_BIT(nr)      (1UL << (nr))

#define CVBSPLL_CON0            (0x010)
#define CVBSPLL_EN              TVD_BIT(31)

#define CVBSPLL_CON1            (0x314)
#define CVBSPLL_DIV2_EN         TVD_BIT(0)

#define CVBSREFPLL_CON0         (0x318)


#define CVBSREFPLL_CON1         (0x014)
#define CVBSREFPLL_EN           TVD_BIT(7)
#define RG_CVBSPLL_DIV2_EN      TVD_BIT(8)


#define CVBSREFPLL_DIV26_EN     TVD_BIT(5)

#define PA_PLL_CON0             (0x900)
#define  RG_CLKSQ_EN            TVD_BIT(0)

/* CVBS ANALOG */
#define REG_ANA_BG_CFG0             (0x000)
#define REG_ANA_BG_CFG1             (0x004)
#define REG_ANA_CLKSQ_CFG0          (0x008)
#define REG_ANA_CVBS_CFG0           (0x00C)
    #define RG_CHA_MIDDLE_EN        TVD_BIT(22)
    #define RG_BLANK_EN             TVD_BIT(23)
    #define RG_AISEL                (0xff << 24)
#define REG_ANA_CVBS_CFG1           (0x010)
    #define CVBSADC1_IO_PWD_OFST    TVD_BIT(6)
    #define SHIFTA_PWD_OFST         TVD_BIT(20)
    #define PGABUFNA_PWD_OFST       TVD_BIT(21)
    #define CORE_PWD_OFST           TVD_BIT(22)
    #define IGEN_PWD_OFST           TVD_BIT(23)
    #define PROT_PWD_OFST           TVD_BIT(24)
    #define INMUX_PWD_OFST          TVD_BIT(25)
    #define CLAMP_PWD_OFST          TVD_BIT(26)
    #define CVBS_PWD_OFST           TVD_BIT(27)
#define REG_ANA_CVBS_CFG2           (0x014)
    #define CVBSADC1_CORE_PWD_OFST  TVD_BIT(25)
    #define CVBSADC1_CK_PWD_OFST    TVD_BIT(26)
    #define CVBSADC1_CK_SEL_OFST    TVD_BIT(27)
#define REG_ANA_CVBSPLL_CFG0        (0x018)
#define REG_ANA_CVBSPLL_CFG1        (0x01C)
#define REG_ANA_CVBSPLL_CFG2        (0x020)
#define REG_ANA_CVBSPLL_CFG3        (0x024)
#define REG_ANA_CVBSPLL_CFG4        (0x028)
#define REG_ANA_TSTOD_CFG0          (0x02C)

/* VDISYS CONFIG */
#define REG_VDISYS_CG_CLR0          (0x108)
#define REG_VDISYS_CVBS_ANAIF_CFG   (0x138)

#define REG_DFE_STA_00          (0x040)
#define VPRES_OFF               (1U << 1)    /* VPRES_OFF: loss signal*/
#define VPRES_ON                (1U << 0)    /* VPRES_ON : get signal*/

#define REG_STA_REG11           (0x044)
#define BLANK_LV                (0x3FF << 0)

#define REG_STA_REG13           (0x04C)
#define PY_LV                   (0x3FF << 0)

#define REG_STA_REG14           (0x050)
#define AGAIN_CODE              (0x7F << 8)
#define FINE_AGC                TVD_BIT(2)

#define REG_STA_REG15           (0x054)
#define CP_CUR                  (0x7F << 16)

#define REG_STA_REG18           (0x060)
#define COCH_DETECTED           TVD_BIT(0)

#define REG_STA_CDET_00         (0x080)
#define CKILL                   TVD_BIT(31)
#define MODE_TVD3D              (0x7 << 28)
#define BLOCK4DET               TVD_BIT(27)
#define V525_TVD3D              TVD_BIT(26)
#define IS443_TVD3D             TVD_BIT(25)
#define PHALT_TVD3D             TVD_BIT(24)
#define FH_NEG                  TVD_BIT(23)
#define FH_POS                  TVD_BIT(22)
#define STD_V625                TVD_BIT(21)
#define STD_V525                TVD_BIT(20)
#define NSTD_V625               TVD_BIT(19)
#define NSTD_V525               TVD_BIT(18)
#define NARRSYNC                TVD_BIT(17)
#define SB4CTG_FLAG             TVD_BIT(16)
#define PALSW_TGL_FLAG          TVD_BIT(15)
#define SVF_BSTDET_F            TVD_BIT(14)
#define VPRES_SVF               TVD_BIT(13)
#define VPRES_TVD3D             TVD_BIT(12)
#define SCF                     TVD_BIT(11)
#define PH_OLD                  TVD_BIT(10)
#define NA_STATE                (0x3 << 8)
#define NR_LEVEL                (0xFF << 0)

#define REG_TG_STA_00           (0x088)
#define HEAD_SWITHC             TVD_BIT(19)
#define TRICK                   TVD_BIT(15)
#define HEAD_SWITHC             TVD_BIT(19)
#define TRICK                   TVD_BIT(15)
#define VLOCK                   TVD_BIT(3)
#define HSYNC_LOCK              TVD_BIT(2)
#define HLOCK                   TVD_BIT(1)
#define LLOCK                   TVD_BIT(0)
#define SIGNAL_LOCK_STATUS      (0xF << 0)

#define REG_STA_REG23           (0x08C)
#define AVG_VLEN                (0x3FF << 0)

#define REG_STA_REG24           (0x090)
#define VCR_BV                  TVD_BIT(26)

#define REG_STA_CTG_01          (0x09C)
#define BLOCK                   TVD_BIT(8)
#define MODE_LOCK_STATUS        (0x1 << 8)


#define REG_STA_REG2A           (0x0A8)
#define ADC_CODE                (0xFFF << 0)

#define REG_STA_REG2B           (0x0AC)
#define VAR_CVBS_CLIP           (0x3FFFF << 0)

#define REG_STA_REG2D           (0x0B4)
#define LINE_STDFH_FLAG         TVD_BIT(29)
#define STA_LCNT                (0xFFF << 0)

#define TVD_INTR_EN             (0x40C)
#define TVD_INTR_STA            (0x000)
#define INTR_VSYNC_TVD          TVD_BIT(9)
#define INTR_MODE_TVD           TVD_BIT(1)
#define INTR_VPRES_TVD          TVD_BIT(0)

#define REG_VSRC_07             (0x434)
#define AAF_SEL                 (0x7 << 17)
#define RG_VSRC_INV_AIDX        TVD_BIT(8)
#define fgHwTvdSVID             TVD_BIT(7)

#define REG_DFE_01                     (0x4C4)
#define BLANK_WIN_START                (0xFF << 0)
#define DFE_BLANK_WIN_START_STD_L      (0x45)
#define DFE_BLANK_WIN_START_NSTD_L     (0x3F)

#define REG_DFE_02                (0x4C8)
#define Y4H_BW                    (0x3 << 0)

#define REG_DFE_03                (0x4CC)
#define AGC2_MODE                 (0xF << 28)
#define AGC2_PK_MODE              (0xF << 24)

#define REG_DFE_07                (0x4DC)
#define AGC2_MANUAL_ACODE         (0x7F << 16)

#define REG_DFE_08                (0x4E0)
#define DCLAMP_Y_EN               TVD_BIT(29)

#define REG_DFE_0A                (0x4E8)
#define CLAMP_TARGET_BLANK_LV     (0x3FF << 10)

#define REG_DFE_0E                (0x4F8)
#define VPRES_EN                  TVD_BIT(31)
#define VPRES_FORCE_ON            TVD_BIT(30)
#define MVPRES_TVD_EN             TVD_BIT(29)
#define MVPRES_CLAMP_EN           TVD_BIT(28)
#define MVPRES_AGC_EN             TVD_BIT(27)
#define MVPRES_BLANK_EN           TVD_BIT(26)
#define MVPRES_SYNC_EN            TVD_BIT(25)
#define MVPRES_HDET_EN            TVD_BIT(24)
#define MVPRES_VDET_EN            TVD_BIT(23)
#define MVPRES_TVD                TVD_BIT(22)
#define MVPRES_CLAMP              TVD_BIT(21)
#define MVPRES_AGC                TVD_BIT(20)
#define MVPRES_BLANK              TVD_BIT(19)
#define MVPRES_SYNC               TVD_BIT(18)
#define MVPRES_HDET               TVD_BIT(17)
#define MVPRES_VDET               TVD_BIT(16)
#define DCLAMP_UP_LIM(x)          (((x) & 0xFF) << 8)
#define DCLAMP_CHECK_LIM          TVD_BIT(7)
#define NR_DET_VPRES_SEL          TVD_BIT(6)
#define VPRES4TVD_MODE            TVD_BIT(5)
#define VPRES4PIC_MODE            TVD_BIT(4)
#define VPRES_SEL                 TVD_BIT(3)
#define VPRES_MASK(x)             (((x) & 0x7) << 0)

#define REG_DFE_19                (0x524)
#define AGC_SEL                   TVD_BIT(26)

#define REG_DFE_1F                (0x53C)

#define REG_CDET_00               (0x540)
#define BST_DET_ADAP              TVD_BIT(31)
#define DET443_SEL_ADAP           TVD_BIT(30)
#define PALSW_FAST_ADAP           TVD_BIT(29)
#define CTG_ADAP                  TVD_BIT(28)
#define CDET_START_ADAP           TVD_BIT(27)
#define CKILL_ADAP                TVD_BIT(26)
#define PALSW_ADAP                TVD_BIT(25)
#define CAGC_ADAP                 TVD_BIT(24)
#define CKILL_SEL(x)              (((x) & 0x3) << 22)
#define MODE_CKILL_EN             TVD_BIT(21)
#define SCF_SEL                   TVD_BIT(20)
#define MDET_V525_SEL             TVD_BIT(19)
#define MDET_SCF_EN               TVD_BIT(18)
#define NR_OUT_SEL(x)             (((x) & 0x3) << 16)
#define DET443_SEL                TVD_BIT(15)
#define HN_DET443_EN              TVD_BIT(14)
#define NTSC443_EN                TVD_BIT(13)
#define PALM_EN                   TVD_BIT(11)
#define PAL60_EN                  TVD_BIT(10)
#define SECAM_EN                  TVD_BIT(9)
#define INI_IS443                 TVD_BIT(8)
#define INI_PHALT                 TVD_BIT(7)
#define INI_V525                  TVD_BIT(6)
#define L525                      TVD_BIT(5)
#define MODE000                   TVD_BIT(4)
#define TVD_MMODE                 TVD_BIT(3)


#define REG_CDET_04               (0x550)
#define CTG_NA_SEL                TVD_BIT(29)
#define CDET_NA_SEL               TVD_BIT(28)

#define REG_TG_04                 (0x590)
#define LF_OFFSET_EN              TVD_BIT(12)

#define REG_TG_08                 (0x5A0)
#define VALIGN_SPEED(x)           (((x) & 0x3) << 30)
#define SKIP_VSPIKE               TVD_BIT(29)
#define FAST_VALIGN               TVD_BIT(28)
#define TGEN_DEBUG                TVD_BIT(27)
#define HVDET_FIXBLK              TVD_BIT(26)
#define FAST_VLOCK                TVD_BIT(25)
#define LLOCK_GUARD               TVD_BIT(24)
#define TKMODE_SEL                TVD_BIT(23)
#define TKMODE_THR(x)             (((x) & 0xF) << 19)
#define TVD_VCR_EN                TVD_BIT(18)
#define FDET_SEL                  TVD_BIT(16)
#define VFIR_EN                   TVD_BIT(15)
#define VFIR_SEL(x)               (((x) & 0x3) << 12)
#define NARRSYNC_TH(x)            (((x) & 0xF) << 8)
#define LCNT_DLY(x)               (((x) & 0xFF))

#define REG_TG_0B                 (0x5AC)
#define AUTO_AVDELAY              TVD_BIT(31)
#define AUTO_MLLOCK               TVD_BIT(30)
#define AUTO_LF_OFFSET            TVD_BIT(29)
#define AUTO_FAST_KP_GAIN         TVD_BIT(28)
#define AUTO_FASTV                TVD_BIT(27)
#define AUTO_FASTV_2S             TVD_BIT(26)
#define TG_VPRES_FORCE(x)         (((x) & 0x3) << 24)
#define L525_FORCE(x)             (((x) & 0x3) << 22)
#define LOCKVLEN_FORCE(x)         (((x) & 0x3) << 20)
#define DEF_VLEN_SEL              TVD_BIT(19)
#define TVD_PATGEN_EN             TVD_BIT(18)
#define TVD_PATGEN_MODE(x)        (((x) & 0x3) << 16)
#define IIR_SLICE_LIM(x)          (((x) & 0xFF) << 8)
#define HVDET_MBLK(x)             (((x) & 0xFF) << 0)

#define REG_TG_0D                 (0x5B4)
#define MXSD_FHNEG_EN             TVD_BIT(15)
#define HLEN_FHPOS_EN             TVD_BIT(14)

#define REG_CTG_00                (0x5E0)
#define SOBVLD_MASK_EN            TVD_BIT(26)
#define CTG_SWLBF                 TVD_BIT(21)
#define BST_0DEG                  TVD_BIT(10)

#define REG_CTG_05                (0x5F4)
#define BST_START_SEL             TVD_BIT(30)

#define CTG_07                    (0x5fc)
#define UV_DELAY                  (0x3 << 2)
#define Y_DELAY                   (0x3 << 0)

#define COMB_CTRL_01              (0x644)
#define DRAMBASEADR               (0x1FFFFFF << 0)

#define COMB_CTRL_02              (0x648)
#define EN3D                      TVD_BIT(31)

#define COMB_CTRL_03              (0x64C)
#define FSKBACK                   TVD_BIT(4)

#define COMB2D_0D                 (0x748)
#define CHANNEL_B_SEL             TVD_BIT(29)
#define DRAMBASEADR_MSB           TVD_BIT(28)

#define REG_SECAM_07              (0x61C)
#define YUV_CATCH_SEL             (0xF << 28)

#define REG_DFE_21                (0x7EC)
#define AGC_MLAGC_EN              TVD_BIT(30)

#define REG_DFE_22                (0x7F0)
#define AGC_MAGC                  (0xFF << 16)

#endif /* TVD_REG_BASE_H */
