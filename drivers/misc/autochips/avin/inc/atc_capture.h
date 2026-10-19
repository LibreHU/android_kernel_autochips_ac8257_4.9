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



#ifndef ATC_CAPTURE_H
#define ATC_CAPTURE_H

/* Header files */
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-device.h>
#include <linux/io.h>
#include <linux/videodev2.h>
#include <linux/v4l2-dv-timings.h>
#include "avin_common.h"

/* Macros */
#define AVIN_MOD_NAME			"AVIN"
#define AVIN_VER_MAIN			1
#define AVIN_VER_MINOR			0
#define AVIN_VER_REV			0
#define AVIN_DRIVER_VERSION		"1.0.0"
#define AVIN_DRIVER_NAME		"avin_capture"

#define AVIN_DEVICE_ID_BASE			10
#define AVIN_CAPTURE_MAX_CHANNELS	2
#define AVIN_CAPTURE_MAX_BUFFER		5
#define AVIN_MODE_MAX_NAME			30
#define AVIN_VIDEO_INDEX			0
#define AVIN_NUMBER_OF_OBJECTS		1

#define CVBS_MAX_PORT_NUM			5


/* This structure will store size parameters as per the mode selected by user */
struct avin_channel_config_params {
	char name[AVIN_MODE_MAX_NAME];	/* Name of the mode */
	u16 width;			/* Indicates width of the image */
	u16 height;			/* Indicates height of the image */
	u8 frm_fmt;			/* Interlaced (0) or progressive (1) */
	u8 ycmux_mode;			/* This mode requires one (0) or two (1)
					   channels */
	u16 eav2sav;			/* length of eav 2 sav */
	u16 sav2eav;			/* length of sav 2 eav */
	u16 vsize;			/* Vertical size of the image */
	u8 capture_format;		/* Indicates whether capture format
					 * is in BT or in CCD/CMOS */
	u8  vbi_supported;		/* Indicates whether this mode
					 * supports capturing vbi or not */
	u8 hd_sd;			/* HDTV (1) or SDTV (0) format */
	v4l2_std_id stdid;		/* SDTV format */
	struct v4l2_dv_timings dv_timings;	/* HDTV format */
};

enum data_size {
	_8BITS = 0,
	_10BITS,
	_12BITS,
};

enum avin_if_type {
	AVIN_IF_BT656,
	AVIN_IF_BT1120,
	AVIN_IF_RAW_BAYER
};

struct avin_interface {
	enum avin_if_type if_type;
	unsigned hd_pol:1;
	unsigned vd_pol:1;
	unsigned fid_pol:1;
};


struct avin_input {
	struct v4l2_input input;
	const char *subdev_name;
	u32 input_route;
	u32 output_route;
};

struct avin_capture_chan_config {
	struct avin_interface avin_if;
	const struct avin_input *inputs;
	int input_count;
};

struct avin_capture_config {
	int (*setup_input_channel_mode)(int);
	int (*setup_input_path)(int, const char *);
	struct avin_capture_chan_config chan_config[1];
	const char *card_name;
	struct v4l2_async_subdev **asd;	/* Flat array, arranged in groups */
	int *asd_sizes;		/* 0-terminated array of asd group sizes */
};


/* structure for avin parameters */
struct avin_video_params {
	__u8 storage_mode;	/* Indicates field or frame mode */
	unsigned long hpitch;
	v4l2_std_id stdid;
};

struct avin_params {
	struct avin_interface iface;
	struct avin_video_params video_params;
	struct avin_channel_config_params std_info;
	union param {
		enum data_size data_sz;
	} params;
};



/* Enumerated data type to give id to each device per channel */
enum avin_channel_id {
	AVIN_CHANNEL0_VIDEO = 0,
	AVIN_CHANNEL1_VIDEO,
};


enum avin_channel_state {
	AVIN_CHANNEL_STATE_IDLE = 0,
	AVIN_CHANNEL_STATE_STOPPED,
	AVIN_CHANNEL_STATE_STARTED,
};

struct video_obj {
	enum v4l2_field buf_field;
	/* Currently selected or default standard */
	v4l2_std_id stdid;
	struct v4l2_dv_timings dv_timings;
};

struct avin_cap_buffer {
	struct vb2_v4l2_buffer buf;
	struct list_head list;
};

struct common_obj {
	/* Pointer pointing to current v4l2_buffer */
	struct avin_cap_buffer *cur_frm;
	/* Pointer pointing to current v4l2_buffer */
	struct avin_cap_buffer *next_frm;
	/* Used to store pixel format */
	struct v4l2_format fmt;
	/* Buffer queue used in video-buf */
	struct vb2_queue buffer_queue;
	/* allocator-specific contexts for each plane */
	struct vb2_alloc_ctx *alloc_ctx;
	u32 alloc_size;
	/* Queue of filled frames */
	struct list_head dma_queue;
	/* Used in video-buf */
	spinlock_t irqlock;
	/* lock used to access this structure */
	struct mutex lock;

	int i;
	
	/* Function pointer to set the addresses */
	void (*set_addr) (unsigned long, unsigned long, unsigned long,
			  unsigned long);
	/* offset where Y top starts from the starting of the buffer */
	u32 ytop_off;
	/* offset where Y bottom starts from the starting of the buffer */
	u32 ybtm_off;
	/* offset where C top starts from the starting of the buffer */
	u32 ctop_off;
	/* offset where C bottom starts from the starting of the buffer */
	u32 cbtm_off;
	/* Indicates width of the image data */
	u32 width;
	/* Indicates height of the image data */
	u32 height;
};

struct channel_obj {
	/* Identifies video device for this channel */
	struct video_device *video_dev;
	/* Indicates id of the field which is being displayed */
	u32 field_id;
	/* flag to indicate whether decoder is initialized */
	u8 initialized;

	enum avin_channel_state state;
	/* subdev corresponding to the current input, may be NULL */
	struct v4l2_subdev *sd;
	/* avin configuration params */
	struct avin_params avinparams;
	/* common object array */
	struct common_obj common[AVIN_NUMBER_OF_OBJECTS];
	/* video object */
	struct video_obj video;
};

struct atc_avin_device {
	struct v4l2_device v4l2_dev;
	struct video_device video_dev;
	struct vb2_queue vid_q;
	struct list_head buf_queue;
	struct avin_cap_buffer *cur_frm;
	struct avin_cap_buffer *next_frm;
	spinlock_t slock;      /*protect buf_queue*/
	struct mutex lock;
};

#endif

