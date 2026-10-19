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
#ifndef __VDP_H__
#define __VDP_H__

#define MAX_BUFF_CNT   20

#define VDP_STATUS_HIDE    (0)  /* initialize*/
#define VDP_STATUS_PREPARE (1)
#define VDP_STATUS_SHOW    (2)
#define VDP_STATUS_FFRW    (3)  /* fast forward*/
#define VDP_STATUS_SEEK    (4)
#define VDP_STATUS_CLEAR   (5)  /* clear all buffer*/
#define VDP_STATUS_BLACK   (6)

#define VDP_CLR_NONE_BUF   (0)  /* HD mode use resizer buffer do not clear buffer in isr*/
#define VDP_CLR_REGU_BUF   (1)  /* regular clear buffer for deinterlace*/
#define VDP_CLR_ONCE_BUF   (2)  /* seek or fast forward case clear buffer for once*/

typedef struct _VDP_PARAM {
	u32 u4SrcWidth;     /* 16 Alignment src width*/
	u32 u4SrcHeight;    /* 32 Alignment src height*/
	RECT   rSrcRect;
	RECT   rDstRect;
	u32 u4SrcType;      /* MM/DVD/AVIN/Backcar*/
	u32 u4Flags;        /* block/scanline, deinterlace, updateoverlay/flip*/
	u32 u4FrmBuffY[MAX_BUFF_CNT];
	u32 u4FrmBuffC[MAX_BUFF_CNT];
	u32 u4Duration[MAX_BUFF_CNT];
	u32 u4VsyncCnt[MAX_BUFF_CNT];
	uintptr_t u4BufferInfor[MAX_BUFF_CNT];
	u32 u4VdpStatus;
	u32 u4VdpMode;      /* video mode: HD/SD mode*/
	u32 u4DeintMode;
	u32 u4PullDownMode;
	u32 u4CurrIdx;      /* MM update frame buffer idx*/
	u32 u4DispIdx;      /* deinterlace show frame buffer idx*/
	u32 u4PrevIdx;      /* deinterlace reference frame buffer idx*/
	u32 u4ClearIdx;     /* unused buffer and need clear buffer for MM*/
	u32 u4NeedClrBuff;
	u32 u4NeedShowBuff;
	bool   fgFirstField;
	bool   fgNeedResizer;  /* Ying-ToDo: rmvb interlace support?*/
	#ifdef CONFIG_ATC_OS_android
	bool   fgNeedRotate;
	u32  u4RotDegree;
	#endif
	bool   fgProgSrc;
	bool   fgTopFiledFirst;
	bool   fgRepeatFirstField;/* first field 1, and second field 0*/
	bool   fgProgSeq;         /* Progressive sequece*/
	bool   fgPullDownFlagValid;
} VDP_PARAM;

extern VDP_PARAM rData[3];
extern void vVdpIsr(u32 u4VdpIdx);

#endif /* __VDP_H__ */



