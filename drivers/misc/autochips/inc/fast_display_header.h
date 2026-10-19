#ifndef FAST_DISPLAY_HEADER_H
#define FAST_DISPLAY_HEADER_H

#include <asm/swab.h>
#ifndef bool
 #define bool _Bool
#endif

#define HANDLE_MAX  2            /**< fast handle supported max ,each handle means one layer */
#define FAST_DISP_ZORDER_MIN     100
#define FAST_DISP_ZORDER_MAX     128

enum OVERLAY_HW_CAPACITY {
	SUPPORT_RESIZE = 1 << 0,        /**< overlay support resize.*/
	SUPPORT_FLIP   = 1 << 1,        /**< overlay support filp.*/
	SUPPORT_CROP   = 1 << 2,        /**< overlay support crop.*/
	SUPPORT_ALPHA  = 1 << 3,        /**< overlay support alpha.*/
	SUPPORT_ROTATE = 1 << 4,        /**< overlay support rotate.*/
	SUPPORT_COLOR_KEY    = 1 << 5,        /**< overlay support color key.*/
	SUPPORT_MAX_CAPACITY = 0x3F,          /**< overlay maximum support capacity.*/
};

struct display_info {
	unsigned int disp_id;            /**< display screen ID: 0,front; 1 rear(if exist).*/
	unsigned int width;              /**< screen width in pixels.*/
	unsigned int height;             /**< screen height in pixels.*/
	unsigned int width_phys;         /**< screen width in millimeters.*/
	unsigned int height_phys;        /**< screen height in millimeters.*/
	unsigned int fps;                /**< frame per second(screen's refresh rate).*/
	unsigned int max_layer_num;      /**< overly maximum supported layer number.*/
	bool         is_connected;       /**< if screen panel is connected to SOC.*/
	unsigned int hw_capacity;        /**< overly supports functionality,i.e. reisze, flip, etc..*/
};

#define MAKE_DISP_FORMAT_ID(id, bpp)  (((id) << 8) | (bpp))

enum DISP_FMT {
	DISP_FMT_UNKNOWN  = 0,
	DISP_FMT_RGB565      = MAKE_DISP_FORMAT_ID(1, 2),
	DISP_FMT_RGB888      = MAKE_DISP_FORMAT_ID(2, 3),
	DISP_FMT_BGR888      = MAKE_DISP_FORMAT_ID(3, 3),
	DISP_FMT_ARGB8888    = MAKE_DISP_FORMAT_ID(4, 4),
	DISP_FMT_ABGR8888    = MAKE_DISP_FORMAT_ID(5, 4),
	DISP_FMT_RGBA8888    = MAKE_DISP_FORMAT_ID(6, 4),
	DISP_FMT_BGRA8888    = MAKE_DISP_FORMAT_ID(7, 4),
	DISP_FMT_YUV422      = MAKE_DISP_FORMAT_ID(8, 2),
	DISP_FMT_XRGB8888    = MAKE_DISP_FORMAT_ID(9, 4),
	DISP_FMT_XBGR8888    = MAKE_DISP_FORMAT_ID(10, 4),
	DISP_FMT_RGBX8888    = MAKE_DISP_FORMAT_ID(11, 4),
	DISP_FMT_BGRX8888    = MAKE_DISP_FORMAT_ID(12, 4),
	DISP_FMT_UYVY        = MAKE_DISP_FORMAT_ID(13, 2),
	DISP_FMT_YUV420_P    = MAKE_DISP_FORMAT_ID(14, 2),
	DISP_FMT_YV12        = MAKE_DISP_FORMAT_ID(16, 1),
	DISP_FMT_PARGB8888   = MAKE_DISP_FORMAT_ID(17, 4),
	DISP_FMT_PABGR8888   = MAKE_DISP_FORMAT_ID(18, 4),
	DISP_FMT_PRGBA8888   = MAKE_DISP_FORMAT_ID(19, 4),
	DISP_FMT_PBGRA8888   = MAKE_DISP_FORMAT_ID(20, 4),
	DISP_FMT_DIM         = MAKE_DISP_FORMAT_ID(21, 0),
	DISP_FMT_RGBA1010102 = MAKE_DISP_FORMAT_ID(22, 4),
	DISP_FMT_RGBA_FP16   = MAKE_DISP_FORMAT_ID(23, 4),
	DISP_FMT_BPP_MASK = 0xFF,
};

typedef enum OVL_ROTATE_DEG {
	OVERLAY_ROTATE_0   = 0,          /**< rotate buffer 0 degree(not rotate).*/
	OVERLAY_ROTATE_90  = 1,          /**< rotate buffer 90 degree.*/
	OVERLAY_ROTATE_180 = 2,          /**< rotate buffer 180 degree.*/
	OVERLAY_ROTATE_270 = 3,          /**< rotate buffer 270 degree.*/
} OVERLAY_ROTATE_DEG;

typedef enum OVL_FLIP {
	OVERLAY_FLIP_NONE = 0 << 4,      /**< not flip(mirror) buffer.*/
	OVERLAY_FLIP_H    = 1 << 4,      /**< flip(mirror) buffer horizontally.*/
	OVERLAY_FLIP_V    = 2 << 4,      /**< filp(mirror) buffer vertically.*/
	OVERLAY_FLIP_HV   = 3 << 4,      /**< filp(mirror) buffer horizontally & vertically.*/
} OVERLAY_FLIP;


enum DISP_SRC_TYPE {
	UI_SRC    = 0,                    /**< source buffer from Android UI.*/
	VIDEO_SRC = 1,                    /**< source buffer from video(camera etc.).*/
};

struct rectangle {
	unsigned int left;                    /**< rectangle x-offset from origin */
	unsigned int top;                     /**< rectangle y-offset from origin */
	unsigned int width;                   /**< rectangle width */
	unsigned int height;                  /**< rectangle height */
};

struct overlay_layer {
	unsigned int        layer_id;              /**< overlay layer number, 0-11(TOTAL_OVL_LAYER_NUM)*/
	bool                layer_enable;          /**< enable this layer*/
	unsigned int        frame_id;              /**< unique frame id, use for sync between frame buffer and display manager when switch path*/
	unsigned int        security;              /**< not used by now*/
	bool                alpha_enable;          /**< alpha functionaliy enable*/
	unsigned int        global_alpha;          /**< aplha value for all frame pixels*/
	enum DISP_SRC_TYPE  src_type;              /**< Android UI or video*/
	enum DISP_FMT       src_fmt;               /**< source format,i.e. RGB888 etc.*/
	unsigned int        src_width;             /**< source width  in pixels*/
	unsigned int        src_height;            /**< source height in pixels*/
	unsigned int        src_pitch;             /**< byte count per one horizontal line, set to width*/
	bool                src_color_key_enable;  /**< if color key is used. color key is use for alpha control when aplha not available.*/
	unsigned int        src_color_key;         /**< color key value. if buffer pixel value equal key value, then this pixel is ignored(just like alpha 0).*/
	struct rectangle    crop_rect;             /**< if crop src buffer, crop rectangle*/
	struct rectangle    dst_rect;              /**< on sreen display rectange*/
	int                 layer_rotation;        /**< layer rotation degree*/
	int                 layer_flip;            /**< layer flip manner*/
	int                 ion_fd;                /**< ion file descriptor from overlay ioctl caller, for sharing buffer.*/
	int                 fence_fd;              /**< release fence file descriptor. used by producer for indicating buffer was costed done*/
	int                 acquire_fence;         /**< used by producer for indicating buffer producing done.*/
	int                 fast_handle;           /**< fast display handle is unique for each layer */
};

struct layers_config {
	unsigned int         layer_count;
	struct overlay_layer *layers_arr;
};

struct layer_zorder {
	int fast_handle;            /**< fast display handle is unique for each layer, return to userspace to identifcate layer */
	int z_order;               /**< each layer has a z-order, to decide whether the layer is up or down range:100-127 */
};

#define GET_DISPLAY_INFO        _IOR('S', 81, struct display_info)
#define SET_OVERLAY_LAYERS      _IOW('S', 82, struct layers_config)
/* fastavm exit eventually, release fastavm used overlay layer resource. */
#define FASTDISP_EXIT_FINALLY   _IO('S', 83)
#define GET_FAST_HANDLE         _IOR('S', 84, int)
#define SET_OVERLAY_Z_ORDER     _IOW('S', 85, struct layer_zorder)

/* ioctl for getting fast display exit or not, 1:exit, 0:alive */
#define GET_FASTDISPLAY_STATUS _IOR('S', 86, int)

#endif /*FAST_DISPLAY_HEADER_H*/
