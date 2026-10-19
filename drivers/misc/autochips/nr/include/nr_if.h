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
#ifndef _NR_IF_H_
#define _NR_IF_H_

#include "nr_drv.h"

/*****************************************************************************/
void nr_hw_Init(void);
void nr_hw_deInit(void);
void NrIsrInit(void);
/*after wchopen,before wchstart*/
void nr_hw_config(NR_PRM_T * ptNrPrm);
/*nr start*/
void nr_hw_trigger(void);
/*please call it in wch call back:GetWchBufIndx, for tell me which wch buffer done*/
void nr_buffer_receive_sync(void * nr_rec_buffer);
/*****************************************************************************/

void setNrReservemem(unsigned long pa, unsigned long va, unsigned int size);
void NrKernelThreadInit(void);
int NrBypass(int fgBypass);
int NrEnable3dFunc(int fgBypass);
int NrSetSwapMode(int swapMode);
int NrSetLevel(unsigned int u4Strength,unsigned int u4FNRStrength,
				unsigned int u4MNRStrength,unsigned int u4BNRStrength);

int NrDumpBuffer(unsigned int cnt);
int NrDumpReg(void);

#endif

