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
#ifndef _BRINGUP_H_
#define _BRINGUP_H_

void bringupinit3365(void);
#ifndef ATC_ARM2_LK_SUPPORT
extern void __iomem* IO_BASE_BRINGUP;
#else
extern unsigned long IO_BASE_BRINGUP;
#endif
extern void PmxVerifySetMode_720480Vdo(unsigned char ucVdoId, unsigned int u4SrcFmt, unsigned int u4PmxFmt, unsigned int u4OutFmt,
        unsigned int u4CavFmt, unsigned char ucTvType, unsigned char ucFit, unsigned char ucInterlace, unsigned char ucVdoInterlace, unsigned char ucSrcType,
        unsigned char uc3d, unsigned int ucScanline, unsigned int u4AddrY, unsigned int u4AddrC);
#endif
