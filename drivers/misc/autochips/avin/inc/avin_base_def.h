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


#ifndef AVIN_BASE_DEF_H
#define AVIN_BASE_DEF_H


#include <linux/videodev2.h>
#include <linux/types.h>


enum frame_status {
	FRAME_EMPTY,
	FRAME_READING,
	FRAME_READY,
	FRAME_ERROR
};

struct wch_ycbuf {
	unsigned long yAddr;
	unsigned long cAddr;
	u32 width;
	u32 height;
};


struct framebuf {
	int idx;
	int length;
	volatile enum frame_status status;
	u8 *data;
	unsigned long userptr;
};

struct v4l2_data {
	int streaming;
	struct v4l2_pix_format pix;
	int memset_cnt;
	int num_frames;
	int cur_frames;
	struct framebuf buffers[VIDEO_MAX_FRAME];
};


#endif


