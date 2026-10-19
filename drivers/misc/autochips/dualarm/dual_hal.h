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

#ifndef DUAL_HAL_H
#define DUAL_HAL_H
#include "types.h"

#define BACKCAR_INT_TO_ARM1         (0xFFFFFFFE)    //ARM9_INT bit0  0: ARM9 to ARM11 interrupt vector 58
#define DISPLAY_INT_TO_ARM1         (0xFFFFFFFD)    //ARM9_INT bit1  0: ARM9 to ARM11 interrupt vector 59
#ifdef CONFIG_ATC_ARM2_LOG_BUFFER
#define LOG_BUFFER_INT_TO_ARM1      (0xFFFFFFF7)    //ARM9_INT bit3  0: ARM9 to ARM11 interrupt vector 60
#endif
#define BACKCAR_INT_TO_ARM2         (0x00000010)    //ARM9_INT bit4  1: ARM11 to ARM9 interrupt vector 58
#define DISPLAY_INT_TO_ARM2         (0x00000020)    //ARM9_INT bit5  1: ARM11 to ARM9 interrupt vector 59
#ifdef CONFIG_ATC_ARM2_LOG_BUFFER
#define LOG_BUFFER_INT_TO_ARM2      (0x00000080)    //ARM9_INT bit7  1: ARM11 to ARM9 interrupt vector 60
#endif

#define MODULEIDMASK                (0xFF000000)
#define MESSAGEIDMASK               (0x0000FFFF)

#define MODULEIDSHIFT               (24)
#define GETMODULEID(m)              ((m & MODULEIDMASK) >> MODULEIDSHIFT)
#define GETMESSAGEID(m)             ((m) & MESSAGEIDMASK)
#define MSG_COMBINE(module,id)      (((module) << MODULEIDSHIFT) | (id))

BOOL HWGetMessage(UINT32 u4ModuleID, UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4);
BOOL HWSendMessage(UINT32 u4MessageHeader, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4);
EXTERN void backcar_get_message(UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4);
EXTERN void backcar_send_message(UINT32 u4MessageHeader, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4);
EXTERN void display_get_message(UINT32 *pu4P1, UINT32 *pu4P2, UINT32 *pu4P3, UINT32 *pu4P4);
EXTERN void display_send_message(UINT32 u4MessageHeader, UINT32 u4P2, UINT32 u4P3, UINT32 u4P4);

#endif  // DUAL_HAL_H
