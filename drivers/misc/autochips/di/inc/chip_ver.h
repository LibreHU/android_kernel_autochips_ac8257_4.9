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

/*****************************************************************************
*  Chip version header file
*****************************************************************************/
#if 0
#ifndef _CHIP_VER_H_
#define _CHIP_VER_H_

#define CONFIG_CHIP_VER_AC83XX 3360
//#define MM_SUPPORT_DIVXHT31
//#ifdef CONFIG_ARCH_AC83XX
#define CONFIG_CHIP_VER_CURR CONFIG_CHIP_VER_AC83XX
//#endif

#ifndef UNUSED
#define UNUSED(x) x
#endif

#define TOTAL_MEM_FOR_128   1

#define MM_FT_CFG           1

#define TT_MM_25M           1
#define TT_MM_30M           2
#define TT_MM_40M           3

#define TT_MM_1080P         1
#define TT_MM_720P          2

#define TT_MM_AVCL30        1
#define TT_MM_AVCL32        2
#define TT_MM_AVCL41        3

#define TT_MM_RESOLUTION    TT_MM_1080P
#define TT_MM_BITRATE       TT_MM_40M
#define TT_MM_AVC_LEVEL     TT_MM_AVCL30

#endif /* _CHIP_VER_H_ */
#endif 
