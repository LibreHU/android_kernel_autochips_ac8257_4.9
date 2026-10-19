/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors. Without
 * the prior written permission of AutoChips inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of AutoChips Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * AutoChips Inc. (C) 2016. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AUTOCHIPS SOFTWARE")
 * RECEIVED FROM AUTOCHIPS AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. AUTOCHIPS EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES AUTOCHIPS PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE AUTOCHIPS SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN AUTOCHIPS
 * SOFTWARE. AUTOCHIPS SHALL ALSO NOT BE RESPONSIBLE FOR ANY AUTOCHIPS SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AUTOCHIPS'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE AUTOCHIPS SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT AUTOCHIPS'S OPTION, TO REVISE OR REPLACE THE
 * AUTOCHIPS SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO AUTOCHIPS FOR SUCH AUTOCHIPS SOFTWARE AT ISSUE.
 */


#ifndef _AC8257_H_
#define _AC8257_H_
#include <asm/io.h>


/*************************************************************************/
/*   SYSTEM DEFINE                                                       */
/*************************************************************************/
/* Address define */
#define ARM2_EMI_OFFSET         0x60000000

#define ARM2_PCJUMP_CMD         0xea00fffe  //ldr pc, [pc #0x1000]
#define ARM2_JTAG_PINMUX        0x07        //switch JTAG pinmux to ARM9

//bim register
#define REG_ARM2_CONTROL        0x0300
#define ARM2_BLKGATE_BYPASS     (1<<3)
#define ARM2_COREDEBUG_ENABLE   (1<<2)
#define ARM2_CORECLK_ENABLE     (1<<1)
#define ARM2_CORE_RESET         (1<<0)

#define REG_DUALARM_INT         0x030C

#ifdef trace_debug
//io
#define REG_ARM2_JTAG           0x1600
#endif

/*************************************************************************/
/*   ARM2 SHARE INFO REGISTERS                                           */
/*************************************************************************/
#define REG_RW_SINFO0_REG   0x0150        //Dual Core Share Info Register 0
#define REG_RW_SINFO1_REG   0x0154        //Dual Core Share Info Register 1
#define REG_RW_SINFO2_REG   0x0158        //Dual Core Share Info Register 2
#define REG_RW_SINFO3_REG   0x015C        //Dual Core Share Info Register 3
#define REG_RW_SINFO4_REG   0x0160        //Dual Core Share Info Register 4
#define REG_RW_SINFO5_REG   0x0164        //Dual Core Share Info Register 5
#define REG_RW_SINFO6_REG   0x0168        //Dual Core Share Info Register 6
#define REG_RW_SINFO7_REG   0x016C        //Dual Core Share Info Register 7
#define REG_RW_SINFO8_REG   0x0170        //Dual Core Share Info Register 8
#define REG_RW_SINFO9_REG   0x0174        //Dual Core Share Info Register 9
#define REG_RW_SINFOA_REG   0x0178        //Dual Core Share Info Register A
#define REG_RW_SINFOB_REG   0x017C        //Dual Core Share Info Register B
#define REG_RW_SINFOC_REG   0x0180        //Dual Core Share Info Register C
#define REG_RW_SINFOD_REG   0x0184        //Dual Core Share Info Register D
#define REG_RW_SINFOE_REG   0x0188        //Dual Core Share Info Register E
#define REG_RW_SINFOF_REG   0x018C        //Dual Core Share Info Register F

/*************************************************************************/
/*   BIM&IO REG READ&WRITE                                               */
/*************************************************************************/
extern void __iomem *arm2_emi_base;
extern void __iomem *io_reg_base;
extern void __iomem *dualarm_reg_base;

#define HAL_WRITE32(reg, value)             writel(value, reg)
#define HAL_READ32(reg)                     readl(reg)

#define DUALARM_BIM_READ32(offset)          HAL_READ32((dualarm_reg_base + offset))
#define DUALARM_BIM_WRITE32(offset, value)  HAL_WRITE32((dualarm_reg_base + offset), (value))

#define ARM1_IO_READ32(offset)              HAL_READ32((io_reg_base + offset))
#define ARM1_IO_WRITE32(offset, value)      HAL_WRITE32((io_reg_base + offset), (value))

#endif
