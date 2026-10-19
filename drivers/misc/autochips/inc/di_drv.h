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
#ifndef _DI_DRV_H_
#define _DI_DRV_H_

#define BUFFER_NUM 3
#define VIDEO_CHANNEL_NUM      (4)

#define DI_IOW(num, dtype)     _IOW('D', num, dtype)
#define DI_IOR(num, dtype)     _IOR('D', num, dtype)
#define DI_IOWR(num, dtype)    _IOWR('D', num, dtype)
#define DI_IO(num)             _IO('D', num)

typedef void (*get_di_buffer)(void *buffer_info);

typedef struct tagRECT {
	unsigned int left;
	unsigned int top;
	unsigned int right;
	unsigned int bottom;
} RECT, *PRECT;

typedef struct {
	bool enable_hw;			/**< 1:enable 0:disable*/
	unsigned int src_width;		/**< Src Buffer Width, 16 Alignment src width*/
	unsigned int src_height;	/**< Src Buffer Height, 32 Alignment src height*/
	unsigned int dst_width;		/**< Dst Buffer Width*/
	unsigned int dst_height;	/**< Dst Buffer Height*/
	RECT   src_rect;		/**< Src Rect*/
	RECT   dst_rect;		/**< Dst Show Area Rect*/
	unsigned int scan_line_mode;	/**< 1 means scan line mode, 0 means block mode*/
	unsigned int flag;		/**< bit 1 value declare support field video input*/
	unsigned int stride;		/**< real dst output width of wch_di */
} di_params;


typedef struct {
	int bufs[VIDEO_CHANNEL_NUM];
	int release_fence;
} di_buffer;

typedef struct {
	di_buffer input;
	di_buffer output;
	bool top_filed_fst[VIDEO_CHANNEL_NUM];//0 ,1
} di_cfg_bufs;

#define	DI_IOCTL_SET_PARAMS DI_IOW(301, di_params)
#define	DI_IOCTL_CFG_BUFS DI_IOWR(302, di_cfg_bufs)

#endif /*_DI_DRV_H_*/
