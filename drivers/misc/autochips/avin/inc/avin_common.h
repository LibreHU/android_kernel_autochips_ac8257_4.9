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


#ifndef AVIN_COM_STRUCT_
#define AVIN_COM_STRUCT_

typedef enum {
	SIGNAL_NONE,
	SIGNAL_STABLE,
	SIGNAL_LOST,
	SIGNAL_MODE_CHANGE_START,
	SIGNAL_MODE_CHANGE_DONE,
	SIGNAL_ERROR,
	SIGNAL_MAX,
} SIGNAL_STATE_E;

typedef struct {
	SIGNAL_STATE_E  signal_state;
	u16 source_width;
	u16 source_height;
} SOURCE_INFO;

struct capture_priv {
	unsigned int    buf_index;
	unsigned int    buf_addr;
	SOURCE_INFO     source_info;
};

#if defined(__KERNEL__) || defined(ATC_ARM2_LK_SUPPORT)
#if defined(ATC_ARM2_LK_SUPPORT)
typedef volatile int atomic_t;
#endif
typedef struct {
	uintptr_t y_addr;
	uintptr_t c_addr;
	unsigned int idex;
	atomic_t status;
	unsigned int vsync_cnt;
} AVIN_BUFFER_T;

typedef enum {
	AVIN_BUF_FREE = 0,
	AVIN_BUF_BUSY,
} AVIN_BUFFER_STATUS_E;
#endif

#if defined(__KERNEL__)
#define DISPLAY_BUFFER_NUM                     5
#define SCREEN_WIDTH                           1280
#define SCREEN_HEIGHT                          800

/*The command to set parameter*/
#define V4L2_CID_SET_MIRROR		(V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_SET_FMT		(V4L2_CID_PRIVATE_BASE + 10)
#define V4L2_CID_SET_FD 		(V4L2_CID_PRIVATE_BASE + 20)
#define V4L2_CID_PROCESS_COLOR 		(V4L2_CID_PRIVATE_BASE + 30)


/*The command to get parameter*/
#define V4L2_CID_GET_SIGNAL_MESSAGE     (V4L2_CID_PRIVATE_BASE + 50)
#define V4L2_CID_GET_HDMI_MHL_DEVICE    (V4L2_CID_PRIVATE_BASE + 100)
#define V4L2_CID_GET_HDMI_MHL_VDORECT   (V4L2_CID_PRIVATE_BASE + 101)
#define V4L2_CID_GET_ORIENTATION        (V4L2_CID_PRIVATE_BASE + 102)
#define V4L2_CID_GET_HDMI_MHL_VDORECT0  (V4L2_CID_PRIVATE_BASE + 103)
#define V4L2_CID_GET_HDMI_MHL_VDORECT1  (V4L2_CID_PRIVATE_BASE + 104)
#define V4L2_CID_GET_HDMI_MHL_VDORECT2  (V4L2_CID_PRIVATE_BASE + 105)
#define V4L2_CID_GET_HDMI_MHL_VDORECT3  (V4L2_CID_PRIVATE_BASE + 106)
#define V4L2_CID_GET_HDMI_MHL_SIGNAL    (V4L2_CID_PRIVATE_BASE + 107)
#endif

#endif
