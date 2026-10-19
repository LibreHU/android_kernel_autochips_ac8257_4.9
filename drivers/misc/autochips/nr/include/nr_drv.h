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

#ifndef _NR_DRV_H_
#define _NR_DRV_H_

#ifndef ATC_ARM2_LK_SUPPORT
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#else
#include <stdint.h> //uintptr_t
#include <mt_typedefs.h>
#endif

#include "avin_common.h"


typedef void (*get_nr_buffer_info)(void* nrBufferInfo);
typedef struct _NR_PRM_T_
{
	/* bypass nr function,in this time nr works as a dma */
	/* 0 or 1,default set to 0. */
	unsigned char fgBypassEn;

	/* dram burst read funciton, up to 64-type data can be fetched from external dram in one dram protocol command.
	    Enable this funciton can help to increase dram bandwidth utilization. */
	/* 0 or 1,default set to 0. */
	unsigned char fgBurstRdEn;

	/* The decoded video data can be stored in dram in different format,which is called as address swap mode. */
	/* 0 ~6,default set to 4. */
	unsigned char u1AddrSwapMode;

	/* nr proccesing unit can be frame based or field based.If nr is in frame based operation,it will treat the picture as
        	    frame manner.If nr is in field based operation,it will process only one field of the picture.Furthermore,the nr module
        	    can automatically process two fields in one command, which is called two-field mode.*/
	/* 0 ~3,default set 0. */
	unsigned char u1FrameMode;

	/* the function can show processed and unprocessed picture in same screen.*/
	/* 0 or 1,default set to 0. */
	unsigned char u1DemoMode;

	/* frame's width and height */
	unsigned int u4PicWidth;
	unsigned int u4PicHeight;

	/* the alligned width and height to frame's width and height. */
	unsigned int u4FrameWidth;
	unsigned int u4FrameHeight;

	/* the total level to control NR's effect. when u4Strength != 0, the below three level is not work.*/
	unsigned int u4Strength;

	/* different level for MNR/BNR/FNR */
	unsigned int u4MNRStrength;
	unsigned int u4FNRStrength;
	unsigned int u4BNRStrength;

	/* the decoded video data from decoder should apply range remapping process before display. The NR
        	    current picture read agent supports the range remapping funciton.*/
	/* 0 or 1,all of below default set to 0. */
	unsigned char fgRangeRemapYEn;
	unsigned char fgRangeRemapUVEn;
	unsigned int u4RangeMapY;
	unsigned int u4RangeMapUV;
	unsigned char fgNoiseMeterEn;
	unsigned char fgUseBlockMeter;
	/* the call back to submit NR's buffer address to avin. */
	get_nr_buffer_info GetNrBufInfo;
}NR_PRM_T;


typedef struct _NR_BUFFER_T_{
	uintptr_t  u4CurrRdYAddr;   //Read Y Start
	uintptr_t  u4CurrRdCAddr;	 //Read C Start
	uintptr_t  u4LastRdYAddr;
	uintptr_t  u4LastRdCAddr;
	uintptr_t  u4CurrWrYAddr; //Write Y Start
	uintptr_t  u4CurrWrCAddr; //Write C Start
} NR_BUFFER_T;

#define NR_BUF_MAX_CNT		5
typedef struct {
	unsigned int u4BufIdx;
	uintptr_t u4YBuf[NR_BUF_MAX_CNT];
	uintptr_t u4CBuf[NR_BUF_MAX_CNT];
} NR_ALLC_BUF_T, *PNR_ALLC_BUF_T;


typedef struct _nrLock {
	#ifndef ATC_ARM2_LK_SUPPORT
	spinlock_t lock;
	#endif
	unsigned long flags;
	unsigned int u4LockCnt;
} nrLock;

typedef struct wchYCaddr {
	uintptr_t yaddr;
	uintptr_t caddr;
} WCHYCADDR;

#endif

