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
#include <linux/delay.h>
#include "dual_hal.h"
#include "types.h"
#include "ac8257.h"
#include "dualarm.h"
#include "dualarm_log.h"

extern spinlock_t Dualarm_Lock;

//wait last interrupt clear
/*
 * LibreHU: Dualarm_Lock is also taken by the ARM2 -> ARM1 interrupt handlers (backcar_get_message(),
 * display_get_message()), so every holder disables local interrupts: with a plain spin_lock() an ARM2 interrupt
 * landing on the CPU that sends a message (the ARM2 answers right after MSG_ANDROID_APP_READY) spun forever.
 * The wait for the ARM2 to take the previous message is bounded as well (it runs with interrupts off and the
 * ARM2 may have stopped): after DUALARM_INT_CLEAR_TIMEOUT_US the message is dropped with an error.
 */
#define DUALARM_INT_CLEAR_TIMEOUT_US	50000

static bool dualarm_wait_int_clear(UINT32 reg, UINT32 bitsmask)
{
	unsigned int us;

	for (us = 0; us < DUALARM_INT_CLEAR_TIMEOUT_US; us++) {
		if (!(bitsmask & DUALARM_BIM_READ32(reg)))
			return true;
		udelay(1);
	}
	if (!(bitsmask & DUALARM_BIM_READ32(reg)))
		return true;
	DUALARM_LOG(DUALARM_LOG_LVL_ERR, "ARM2 did not take the previous message (int 0x%x), message dropped\n",
		    bitsmask);
	return false;
}

static BOOL _fgDualHALSetParmeterGroup0(UINT32 u4P1, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4)
{
    DUALARM_BIM_WRITE32(REG_RW_SINFO0_REG, u4P1);
    DUALARM_BIM_WRITE32(REG_RW_SINFO1_REG, u4P2);
    DUALARM_BIM_WRITE32(REG_RW_SINFO2_REG, u4P3);
    DUALARM_BIM_WRITE32(REG_RW_SINFO3_REG, u4P4);
    return TRUE;
}

static BOOL _fgDualHALGetParmeterGroup0(UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4)
{
    *pu4P1 = DUALARM_BIM_READ32(REG_RW_SINFO0_REG);
    *pu4P2 = DUALARM_BIM_READ32(REG_RW_SINFO1_REG);
    *pu4P3 = DUALARM_BIM_READ32(REG_RW_SINFO2_REG);
    *pu4P4 = DUALARM_BIM_READ32(REG_RW_SINFO3_REG);
    return TRUE;
}

static BOOL _fgDualHALSetParmeterGroup1(UINT32 u4P1, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4)
{
    DUALARM_BIM_WRITE32(REG_RW_SINFO5_REG, u4P2);
    DUALARM_BIM_WRITE32(REG_RW_SINFO6_REG, u4P3);
    DUALARM_BIM_WRITE32(REG_RW_SINFO7_REG, u4P4);
    DUALARM_BIM_WRITE32(REG_RW_SINFO4_REG, u4P1);
    return TRUE;
}

static BOOL _fgDualHALGetParmeterGroup2(UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4)
{
    *pu4P1 = DUALARM_BIM_READ32(REG_RW_SINFO8_REG);
    *pu4P2 = DUALARM_BIM_READ32(REG_RW_SINFO9_REG);
    *pu4P3 = DUALARM_BIM_READ32(REG_RW_SINFOA_REG);
    *pu4P4 = DUALARM_BIM_READ32(REG_RW_SINFOB_REG);

    return TRUE;
}

static BOOL _fgDualHALGetParmeterGroup3(UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4)
{
    *pu4P1 = DUALARM_BIM_READ32(REG_RW_SINFOC_REG);
    *pu4P2 = DUALARM_BIM_READ32(REG_RW_SINFOD_REG);
    *pu4P3 = DUALARM_BIM_READ32(REG_RW_SINFOE_REG);
    *pu4P4 = DUALARM_BIM_READ32(REG_RW_SINFOF_REG);

    return TRUE;
}

//============================================================================
// Public functions
//============================================================================
BOOL fgDualHALSetBootUpParameter(UINT32 u4P1, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4)
{
    return _fgDualHALSetParmeterGroup0(u4P1, u4P2, u4P3, u4P4);
}

BOOL fgDualHALGetBootUpParameter(UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4)
{
    return _fgDualHALGetParmeterGroup0(pu4P1, pu4P2, pu4P3, pu4P4);
}

BOOL fgDualHALSetSendCommandParameter(UINT32 u4P1, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4)
{
    return _fgDualHALSetParmeterGroup1(u4P1, u4P2, u4P3, u4P4);
}

BOOL fgDualHALGetReturnParameter(UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4)
{
    return _fgDualHALGetParmeterGroup2(pu4P1, pu4P2, pu4P3, pu4P4);	
}

void fgClearGroup(void)
{
    DUALARM_BIM_WRITE32(REG_RW_SINFO0_REG, 0);
    DUALARM_BIM_WRITE32(REG_RW_SINFO4_REG, 0);
    DUALARM_BIM_WRITE32(REG_RW_SINFO8_REG, 0);
    DUALARM_BIM_WRITE32(REG_RW_SINFOC_REG, 0);
}

void backcar_send_message(UINT32 u4MessageHeader, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4)
{

    unsigned long flags;
    int temp;

    spin_lock_irqsave(&Dualarm_Lock, flags);
    if (!dualarm_wait_int_clear(REG_DUALARM_INT, BACKCAR_INT_TO_ARM2)) {
        spin_unlock_irqrestore(&Dualarm_Lock, flags);
        return;
    }

    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "backcar write data to Group1\n");
    _fgDualHALSetParmeterGroup1(u4MessageHeader, u4P2, u4P3, u4P4);

    //enable arm1 to arm2 interrupt : bit 4 set 1
    temp = DUALARM_BIM_READ32(REG_DUALARM_INT) | BACKCAR_INT_TO_ARM2;
    DUALARM_BIM_WRITE32(REG_DUALARM_INT, temp );
    spin_unlock_irqrestore(&Dualarm_Lock, flags);
}

#ifdef CONFIG_ATC_ARM2_LOG_BUFFER
void arm2_log_buffer_send_message(void)
{
    unsigned long flags;
    int temp;
    spin_lock_irqsave(&Dualarm_Lock, flags);
    DUALARM_LOG(DUALARM_LOG_LVL_ERR, "Enter arm2_log_buffer_send_message %d\n", DUALARM_BIM_READ32(REG_DUALARM_INT));
    if (!dualarm_wait_int_clear(REG_DUALARM_INT, LOG_BUFFER_INT_TO_ARM2)) {
        spin_unlock_irqrestore(&Dualarm_Lock, flags);
        return;
    }
    temp = DUALARM_BIM_READ32(REG_DUALARM_INT) | LOG_BUFFER_INT_TO_ARM2;
    DUALARM_BIM_WRITE32(REG_DUALARM_INT, temp);
    DUALARM_LOG(DUALARM_LOG_LVL_ERR, "Left Arm2 log buffer send message temp = %d reg = %d\n", temp, DUALARM_BIM_READ32(REG_DUALARM_INT));
    spin_unlock_irqrestore(&Dualarm_Lock, flags);
}
#endif

void display_send_message(UINT32 u4MessageHeader, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4)
{
    unsigned long flags;
    int temp;

    spin_lock_irqsave(&Dualarm_Lock, flags);
    if (!dualarm_wait_int_clear(REG_DUALARM_INT, DISPLAY_INT_TO_ARM2)) {
        spin_unlock_irqrestore(&Dualarm_Lock, flags);
        return;
    }

    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "display write data to Group0\n");
    _fgDualHALSetParmeterGroup0(u4MessageHeader, u4P2, u4P3, u4P4);

    //enable amr1 to arm2 interrupt : bit 5 set 1
    temp = DUALARM_BIM_READ32(REG_DUALARM_INT) | DISPLAY_INT_TO_ARM2;
    DUALARM_BIM_WRITE32(REG_DUALARM_INT, temp );
    spin_unlock_irqrestore(&Dualarm_Lock, flags);
}

void backcar_get_message(UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4)
{
    unsigned long flags;
    int temp;

    spin_lock_irqsave(&Dualarm_Lock, flags);
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "backcar_get_message call\n");
    _fgDualHALGetParmeterGroup3(pu4P1, pu4P2, pu4P3, pu4P4);

    //clear arm2 to arm1 interrupt : bit0 set 1
    temp = DUALARM_BIM_READ32(REG_DUALARM_INT)  |(~BACKCAR_INT_TO_ARM1);
    DUALARM_BIM_WRITE32(REG_DUALARM_INT, temp);
    spin_unlock_irqrestore(&Dualarm_Lock, flags);
}

void display_get_message(UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4)
{
    unsigned long flags;
    int temp;

    spin_lock_irqsave(&Dualarm_Lock, flags);
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "display_get_message call\n");
    _fgDualHALGetParmeterGroup2(pu4P1, pu4P2, pu4P3, pu4P4);

    //clear arm1 to arm2 interrupt : bit1 set 1
    temp = (DUALARM_BIM_READ32(REG_DUALARM_INT))|(~DISPLAY_INT_TO_ARM1);
    DUALARM_BIM_WRITE32(REG_DUALARM_INT, temp);
    spin_unlock_irqrestore(&Dualarm_Lock, flags);
}

BOOL HWSendMessage(UINT32 u4MessageHeader, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4)
{
    UINT32 u4ModuleID = GETMODULEID(u4MessageHeader);
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "HWSendMessage u4ModuleID[%d]!\n",u4ModuleID);

    if (u4ModuleID == MODULE_BCAR)
    {
        backcar_send_message(u4MessageHeader, u4P2, u4P3, u4P4);
    }
    else if (u4ModuleID == MODULE_DISPLAY)
    {
        display_send_message(u4MessageHeader, u4P2, u4P3, u4P4);
    }

    return(TRUE);
}

BOOL HWGetMessage(UINT32 u4ModuleID, UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4)
{

    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "HWGetMessage call,moudle is %d\n", u4ModuleID);
    if (u4ModuleID == MODULE_BCAR)
    {
        backcar_get_message(pu4P1, pu4P2, pu4P3, pu4P4);
    }
    else if (u4ModuleID == MODULE_DISPLAY)
    {
        display_get_message(pu4P1, pu4P2, pu4P3, pu4P4);
    }

    return(TRUE);
}

