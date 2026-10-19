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
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#ifndef ATC_ARM2_LK_SUPPORT
#include <linux/ioctl.h>
#include "x_os.h"
#include "avin_common.h"
//#include "types.h"
#else
#include "platform/mt_typedefs.h"
#include "sys/types.h"
#include "di_types.h"

#include "avin_common.h"

#endif
#include <di_hal.h>



#define MONITOR_VDO_FPS 1
#define KERNEL_STANDARD_API

#define VOUT_DI_FUN_EN            (1)
#define VOUT_DI_SRC_PROG          (0 << 1) /* progressive source*/
#define VOUT_DI_SRC_INTER_FRM     (1 << 1) /* interlace frame source*/
#define VOUT_DI_SRC_INTER         (1 << 2) /* interlace source*/
#define VOUT_DI_SRC_MASK          (3 << 1)
#define VOUT_DI_TOP_FLD_FIRST     (1 << 3) /* top filed first*/
#define VOUT_DI_RPT_FIRST_FLD     (1 << 4) /* repeat first filed*/
#define VOUT_DI_PROG_SEQ          (1 << 5) /* progressive sequece*/
#define VOUT_DI_SEEK_LOCATE       (1 << 6) /* seek locate*/
#define VOUT_DI_FF_RW             (1 << 7) /* fast forward*/
#define VOUT_DI_PD                (1 << 8)  /* pull down flag*/

#define VOUT_BG_OUTPUT_EN         (1 << 16)
#define  BOOT_ANIMATION_OSD_PLANE    OSD_PLANE_1

enum {
	SRC_COLOR_KEY,
	DST_COLOR_KEY,
};

struct OSD_INFO {
	s32 posx;
	s32 posy;
	s32 width;
	s32 height;
};

struct VDP_ALPHA {
	s32 enable;
	s32 inalpha;
	s32 outalpha;
};

struct VOUT_PARAM {
	u32 y_phy_addr;
	u32 c_phy_addr;
	u32 device_name;
	u32 duration;
	u32 disp_flags;
};

struct OSD_PARAM {
	u32 buf_phy_addr;
};

struct OVERLAY_PARAM {
	u32 u4Idx;
	u32 u4SrcWidth;
	u32 u4SrcHeight;
	RECT   rSrcRect;
	RECT   rDstRect;
	uintptr_t u4BufferInfor;
	uintptr_t u4PhysicalAddressY;
	uintptr_t u4PhysicalAddressC;
	u32 u4Status;
	u32 u4Flags;
	u32 device_name;
	/*De-interlace interface*/
	u32 u4Duration;
	bool   fgProgSrc;
	bool   fgTopFiledFirst;
	bool   fgRepeatFirstField;/* first field 1, and second field 0*/
	bool   fgProgSeq;         /* Progressive sequece*/
	bool   fgPullDownFlagValid;
};

struct FRONT_REAR_PARAM {
	u32 u4PreIdx;
	u32 u4Idx;
	u32 u4PreFlags;
	u32 u4Flags;
	u32 device_name;
};

struct FMT_BG_PARAM {
	u32 u4Idx;
	u32 u4Color;
	bool   fgEnable;
};

typedef struct {
	u32 u4TopLeftXMargin;
	u32 u4TopLeftYMargin;
	u32 u4BottomRightXMargin;
	u32 u4BottomRightYMargin;
}CVBS_OUTPUT_MARGIN_T;

typedef struct _FB_CONFIG {
	u32 u4VideoMode;
	u32 u4Tm070ddhg;/*used for checking wether current LCD is TM070DDHG LCD.1:yes,0:no.*/
	CVBS_OUTPUT_MARGIN_T rCVBSMargin;
} FB_CONFIG_T;

#define MAX_FENCE_CNT   10
typedef struct _FB_SYNC
{
    UINT32 u4AcquireFenceCnt;
    int *pru4AcquireFence;
    int *pru4ReleaseFence;

}FB_SYNC_T;

typedef struct _OSD_DATA_T {
	u32 u4Flags;
	u32 u4SrcColorKey;
	RECT   rDestRect;
	u32 u4PixelFormat;
	u32 u4OutputPath;
	u32 u4BitmapPA;/*phiscial address*/
	u32 u4BlockID;
	u32 u4Width;
	u32 u4Height;
	u32 device_name;
} OSD_DATA_T;

typedef struct {
	void *srcdata;
	void *dstdata;
	s32 srcWidth;
	s32 srcHeight;
	s32 dstWidth;
	s32 dstHeight;
	u32 ycbuf[2];
} imgrez_data;

typedef struct YUV420BLOCK_TO_ARGB8888_BUF_T {
	u32 ycbuf[2];
	void *vaddr;
	s32 width;
	s32 height;
	s32 dstWidth;
	s32 dstHeight;
	u32 u4DestARGB8888Pa;
        bool fgBlock;
} YUV420BLOCK_TO_ARGB8888_BUF_T;

typedef struct YUV420BLOCK_TO_NV12_BUF_T {
	u32 srcbuf[2];
	s32 srcwidth;
	s32 srcheight;
	s32 srcbufwidth;
	s32 srcbufheight;
	u32 dstbuf[2];
	s32 dstwidth;
	s32 dstheight;
	s32 dstbufwidth;
	s32 dstbufheight;
}YUV420BLOCK_TO_NV12_BUF_T;

enum {
	FRONT_VDO = 0x00,
	REAR_VDO   = 0x01,
	BACKCAR_VDO = 0x02,
};

typedef enum {
	USB     = 0xa,
	DVD     = 0xb,
	AVIN_1  = 0xc,
	BACKCAR = 0xd,
	YPBPR   = 0xe,
	VGA     = 0xf,
	DGI     = 0x10,
	VDO_HDMI    = 0x11,
	AVIN_2  = 0x12,
	AVIN_3  = 0x13,
	AVIN_4  = 0x14,
	AVIN_5  = 0x15,
} VIDEO_SRC_TYPE;

#define srcMax 12 /*the number of VIDEO_SRC_TYPE's members*/

typedef struct _CP_CONFIG {
	VIDEO_SRC_TYPE SrcType;
	s32 i4GHue;
	s32 i4YGain;
	s32 i4UGain;
	s32 i4VGain;
	s32 i4Contr;
	s32 i4Brit;
	s32 i4Satr;
} CP_CONFIG;

#define VDP_BLOCK_MODE       0x0
#define VDP_SCANLINE_MODE    0x1 /* 1 is scanline mode and 0 is block mode */
#define VDP_FRAMELOCK_MODE   0x2
#define VDP_TOP_LEVEL        0x04
#define VDP_ENABLE_DEINT     0x1000
#define VDP_PLAY_FF_RW       0x2000
#define VDP_SEEK_LOCATE      0x4000
#define VDP_ENABLE_PDDI      0x8000
#define VDP_UPDATE_OVERLAY   0x80000
#define VDP_FLIP_ADDRESS     0x40000


#define VDP_ROTATE_90		 0x040
#define VDP_ROTATE_270		 0x080


#define SUBTITLE_IOC_BASE 'w'
#define OSD_BASE      'O'

#ifndef VIDIOC_QBUF
#ifdef ATC_ARM2_LK_SUPPORT
#define VIDIOC_QBUF                     (0xC044560F) /*_IOWR('V', 15, struct OVERLAY_PARAM)*/
#endif
#endif
#ifndef VIDIOC_STREAMON
#define VIDIOC_STREAMON               	(0x40045612) //_IOWR('V', 18, struct OVERLAY_PARAM)
#endif
#ifndef VIDIOC_STREAMOFF
#define VIDIOC_STREAMOFF                (0x40045613) /*_IOWR('V', 19, struct OVERLAY_PARAM)*/
#endif

#ifndef ATC_ARM2_LK_SUPPORT
#define STIOC_SETOSDINFO                _IOWR(SUBTITLE_IOC_BASE, 0, struct OSD_INFO)
#define STIOC_SETOSD                    _IOWR(SUBTITLE_IOC_BASE, 1, unsigned long)
#define STIOC_SET_DST_CK                _IOWR(SUBTITLE_IOC_BASE, 2, s32)
#define STIOC_SET_VDO_ALPHA             _IOWR(SUBTITLE_IOC_BASE, 3, struct VDP_ALPHA)
#define STIOC_SETOSD_8_BIT_PALETTE      _IOWR(SUBTITLE_IOC_BASE, 4, unsigned long)
#define STIOC_SETOSD_HIDE               _IOWR(SUBTITLE_IOC_BASE, 5, unsigned long)
#define STIOC_SET_FMT_BLACK             _IOWR(SUBTITLE_IOC_BASE, 6, unsigned long)
#define STIOC_SET_SRC_CLR_KEY           _IOWR(SUBTITLE_IOC_BASE, 7, unsigned long)
#define STIOC_TVD2TVE                   _IOWR(SUBTITLE_IOC_BASE, 8, s32)
#define STIOC_SET_DST_RECT              _IOWR(SUBTITLE_IOC_BASE, 9, unsigned long)
#define STIOC_SET_VDP_PARAM             _IOWR(SUBTITLE_IOC_BASE, 10, struct OVERLAY_PARAM)
#define VIDIOC_QBUF                     _IOWR(SUBTITLE_IOC_BASE, 17, struct OVERLAY_PARAM)
#define YUV420_TO_YC_IOCTL              _IOWR(SUBTITLE_IOC_BASE, 11, imgrez_data)
#define STIOC_GET_VDP_SYNC_STATUS       _IOWR(SUBTITLE_IOC_BASE, 12, u32)/*mtk94106 add*/
#define STIOC_SET_PLANE_ORDER           _IOWR(SUBTITLE_IOC_BASE, 13, unsigned long)
#define STIOC_RESET_PLANE_ORDER         _IOWR(SUBTITLE_IOC_BASE, 14, unsigned long)
#define STIOC_SET_FRONT_REAR_PARAM      _IOWR(SUBTITLE_IOC_BASE, 15, struct FRONT_REAR_PARAM) /*mtk94098*/
#define STIOC_SET_SUB_FLAG              _IOWR(SUBTITLE_IOC_BASE, 16, s32) /*mtk94020*/
#endif
#define  BUFFER_FENCE_SYNC             0x1008

#define OSD_DATA_SRC_CLR_KEY            0x01
#define OSD_DATA_ALPHA_BLENDING         0x02
#define OSD_DATA_OUPPUT_PATH            0x04
#define OSD_DATA_BITMAP_PA              0x08
#define OSD_DATA_SET_DEST_RECT          0x10
#define YUV420_BLOCK_TO_ARGB8888        0x0002000a
#define YUV420_BLOCK_TO_NV12			0x0002000b
#define TVE_TURNON                      0x1002


enum {
	OSD_PIXEL_FORMAT_RGB565   = 0xb,
	OSD_PIXEL_FORMAT_ARGB8888 = 0xe,
};



/* OSD Memory info*/

#define OSD_BLOCK_SIZE   0x100000

#define OVERLAY_OSD_SET_SRC_CLRKEY      _IOWR(OSD_BASE, 0, s32)
#define OVERLAY_OSD_SET_ALPHA_BLENDING  _IOWR(OSD_BASE, 1, s32)
#define OVERLAY_OSD_SHOW                _IOWR(OSD_BASE, 2, s32)
#define OVERLAY_OSD_HIDE                _IOWR(OSD_BASE, 3, s32)
#define OVERLAY_OSD_SET_SRCRECT         _IOWR(OSD_BASE, 4, s32)
#define OVERLAY_OSD_GET_SRCRECT         _IOWR(OSD_BASE, 5, s32)
#define OVERLAY_OSD_SET_OVERLAY_PARAM   _IOWR(OSD_BASE, 6, s32)
#define OVERLAY_OSD_SET_DSTRECT         _IOWR(OSD_BASE, 7, s32)
#define OVERLAY_OSD_GET_DSTRECT         _IOWR(OSD_BASE, 8, s32)
#define OVERLAY_OSD_REQUEST_LAYER       _IOWR(OSD_BASE, 9, s32)
#define OVERLAY_OSD_RELEASE_LAYER       _IOWR(OSD_BASE, 10, s32)



typedef struct _OSD_OVERLAY_PARAM_T {
	s32 i4PixelFormat;
	s32 i4Width;
	s32 i4Height;
} OSD_OVERLAY_PARAM_T;


typedef struct _OSD_CTL {
	s32  i4Plane;
	s32  i4Value;
} OSD_CTL_T;
/*setParameter  param define*/
#define SET_DST_COLOR_KEY               0x01
#define SET_VIDEO_ALPHA                 0x02
#define SET_OSD_HIDE                    0x03
#define SET_SRC_CLK_KEY                 0x04
#define SET_TVD_TO_TVE                  0x05
#define SET_OSD_OUTPUT_PATH             0x06
#define SET_OSD_USING_PRIVATE_DEST_RECT   0x07

extern void set_colorkey(u32 type, u32 colorkey, u32 enable);
extern void vSetBacklightIntensity(u32 u4Value);

extern void set_pool_param(u64 base, u64 va, u32 size);
#ifdef ATC_ARM2_LK_SUPPORT
//typedef u32 phys_addr_t;
//extern void WchSetSourceBaseAddr(phys_addr_t wchReservebase);
#endif

#endif


