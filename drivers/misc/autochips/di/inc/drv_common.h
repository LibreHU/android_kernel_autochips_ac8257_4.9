/*
* Copyright (c) 2016 AutoChips Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#if ! defined(DRV_COMMON_H)
#define DRV_COMMON_H

//#define USE_2_ES
/////////////////////////////////////////////////////////////////////////////////////
//						Below added for BD_P								    //
/////////////////////////////////////////////////////////////////////////////////////
#include "drv_config.h"
#include "chip_ver.h"

#ifdef __cplusplus
extern "C" {
#endif


#define NEW_PAUSE_MODE 1

// For FLV media use sorenson H263 codec @2009/06/26
#if CONFIG_DRV_SUPPORT_SORENSON_H263
#define DRV_SUPPORT_SORENSON_H263  (1)
#else
#define DRV_SUPPORT_SORENSON_H263  (0)
#endif

#if DRV_SUPPORT_SORENSON_H263
#define DRV_SORENSON_H263_HW_HDR_DETECTION 0
#else
#define DRV_SORENSON_H263_HW_HDR_DETECTION 0
#endif

#define CONFIG_AUDIO_SUPPORT_CMDQ_MULTI_PTS   0
#define CONFIG_AUDIO_SUPPORT_CMD_Q_TX	1
//#define DRV_HIGH_BITRATE_PROC_CFG  (1)
#if CONFIG_DRV_HIGH_BITRATE_SPECIAL_PROC
#define CONFIG_DRV_SUPPORT_CMD_Q_TX (1) // Can open directly later without considering high bitrate
#else
#define CONFIG_DRV_SUPPORT_CMD_Q_TX (0)
#endif

#define CONFIG_DRV_SUPPORT_MPG_PSCODE	(1)

#if CONFIG_DRV_SUPPORT_CMD_Q_TX
#define DMX_MAX_TX_CNT_FOR_CMD_Q  (50)
#define LPDMX_CMD_Q_TX_REG_DW_IDX  (19)

typedef struct
{
  UINT32 u4TxOfst;
  UINT32 u4TxLen;
  BOOL	 fgEndAU;
} CMDQ_TX_ENTRY_T;
#else
// See PTX_CMDQ_NUM in dmx_verify.h ???
// Can use DMX_MAX_TX_CNT_FOR_CMD_Q instead of PTX_CMDQ_NUM ???
#define DMX_MAX_TX_CNT_FOR_CMD_Q  (40) // (1)
#endif

// Can remove following 2 define if all ready @2009/01/06
#if CONFIG_DRV_SUPPORT_RM
#define CONFIG_DRV_SUPPORT_RM_VID_DYNC_MEM (1)
#define CONFIG_DRV_SUPPORT_RM_COOK_AUD (0)
#else
#define CONFIG_DRV_SUPPORT_RM_VID_DYNC_MEM (0)
#define CONFIG_DRV_SUPPORT_RM_COOK_AUD (0)
#endif

// Max Xia Add for PVR HW Insert Header(Startcode) for Video
#define PVR_HW_INSERT_HDR					(0)
#if PVR_HW_INSERT_HDR
#define MAX_PVR_HW_INSERT_HDR_NUM			5
#define PVR_HW_INSERT_HDR_MASK_0_BYTES		0x00
#define PVR_HW_INSERT_HDR_MASK_1_BYTES		0x10
#define PVR_HW_INSERT_HDR_MASK_2_BYTES		0x18
#define PVR_HW_INSERT_HDR_MASK_3_BYTES		0x1C
#define PVR_HW_INSERT_HDR_MASK_4_BYTES		0x1E
#define PVR_HW_INSERT_HDR_MASK_5_BYTES		0x1F
#endif


/// Supported address swap mode in driver layer
typedef enum
{
  ASM_0 = 0,	       ///< 8520 no address swap
  ASM_1,		 ///< 8520 address swap mode 1
  ASM_2,		  ///< 8520 address swap mode 2
  ASM_3,		 ///< 5351 address swap mode 0
  ASM_4,		 ///< 5351 address swap mode 1
  ASM_5,		 ///< 5351 address swap mode 2
  ASM_6,		 ///< 5351 address swap mode 3
}DRV_ASM;

/// Supported frame buffer type in driver layer
typedef enum
{
  FBT_420_RS = 0,     ///< YCbCr 420 raster scan
  FBT_420_BK,		///< YCbCr 420 block
  FBT_422_RS,		///< YCbCr 422 raster scan
  FBT_422_BK,		///< YCbCr 422 block
  FBT_420_BK_YCBIND,	  ///< YCbCr 420 block, Y C memory are bound, for H.264 request
  FBT_420_BK_YONLY,	   ///< YCbCr 420 block, Y memory only, no CbCr, for H.264 request
  FBT_WORKSPACE,    ///< One continue memory, like JPEG working space
  FBT_PBBUF,   ///< One continue memory, overlay with one HD main buffer
  FBT_BGIMG  /// < One continue memory, overlay with whole sub buffer
} DRV_FBTYPE;


//#define DRV_SUPPORT_ADDRESS_SWAP
#define DRV_ADDRESS_SWAP_MODE ASM_5

#define DRV_ADDRESS_SWAP_OFF ASM_0

#define DRV_SUPPORT_DEC_ERR_DROP_LEVEL

//#define DRV_SUPPORT_VDEC_DOWN_SCALE

#define DRV_SUPPORT_FRC_VAR

#define DRV_VDP_SUPPORT_ONE_SOURCE_TWO_DISPLAY

#ifdef DRV_VDEC_VDP_RACING
#define DRV_VDEC_SUPPORT_FBM_OVERLAY
#endif

#if CONFIG_DRAM256_MODEL

#define DRV_FBM_ORIG_DSCL_OVERLAP
#define DRV_FBM_GENERAL_MEM_ALLOC
#ifdef DRV_FBM_GENERAL_MEM_ALLOC
#define DRV_PBBUF_FRMBUF_OVERLAP
#endif

#endif

#if CONFIG_DRAM256_MODEL
#define DRV_BKIMG_FRMBUF_OVERLAP
#endif

#define SetVCodec(u4PicType, eVCodec)  (u4PicType = (u4PicType & 0xFFFF00FF) | (eVCodec << 8))
#define GetVCodec(u4PicType)  ((u4PicType & 0x0000FF00) >> 8)

#define IsMpeg2Pic(u4PicType) (((u4PicType & 0x0000FF00) >> 8) == VC_MPEG2)

#define IsM4vPic(u4PicType) ((((u4PicType & 0x0000FF00) >> 8) == VC_MPEG4) || \
				(((u4PicType & 0x0000FF00) >> 8) == VC_DIVX311) || \
				(((u4PicType & 0x0000FF00) >> 8) == VC_DIVX4) || \
				(((u4PicType & 0x0000FF00) >> 8) == VC_DIVX6) || \
				(((u4PicType & 0x0000FF00) >> 8) == VC_H263) || \
				(((u4PicType & 0x0000FF00) >> 8) == VC_H263_SORENSON))

#define IsDivxPic(u4PicType) ((((u4PicType & 0x0000FF00) >> 8) == VC_DIVX311) || \
			     (((u4PicType & 0x0000FF00) >> 8) == VC_DIVX4) || \
			     (((u4PicType & 0x0000FF00) >> 8) == VC_DIVX6))

#define IsWMVPic(u4PicType) ((((u4PicType & 0x0000FF00) >> 8) == VC_WMV1) || \
			     (((u4PicType & 0x0000FF00) >> 8) == VC_WMV2) || \
			     (((u4PicType & 0x0000FF00) >> 8) == VC_WMV3) || \
			     (((u4PicType & 0x0000FF00) >> 8) == VC_VC1))

#define IsH264Pic(u4PicType) (((u4PicType & 0x0000FF00) >> 8) == VC_H264)

// *********************************************************************
// Picture Coding Type
// *********************************************************************
/*! \name Extra Type of Picture information
* @{
*/
#define SEQ_HDR        ((u32)1 << 16)	 ///< Access unit include a sequence header
#define GOP_HDR        ((u32)1 << 17)	 ///< Access unit include a GOP header
#define SEQ_END        ((u32)1 << 18)	 ///< Access unit include a sequence end
#define ANGLE_END      ((u32)1 << 19)	 ///< Access unit include a angle end
#define SEQ_PS	       ((u32)1 << 20)	 ///< Access unit include a sequence parameter set in H264
#define PIC_PS	       ((u32)1 << 21)	 ///< Access unit include a picture parameter set in H264
#define SEI	       ((u32)1 << 22)	 ///< Access unit include a supplement enhancement information in H264
#define REF_PIC        ((u32)1 << 23)	 ///< Access unit is a reference picture in H264
#define IDR_PIC        ((u32)1 << 24)	 ///< Access unit is a IDR picture in H264
#define ENTRY_PTR      ((u32)1 << 25)	 ///< Access unit is a Entry Pointer in WMV
#define AUTO_PAUSE     ((u32)1 << 26)
#define MULTISLICE_PIC ((u32)1 << 27)	 ///Access unit is a multi-slice picture (only for H264)
#define SUB_SEQ_PS     ((u32)1 << 28)	 ///Access unit is a multi-slice picture (only for H264)
#define PREFIX_NAL     ((u32)1 << 29)	 ///Access unit is a multi-slice picture (only for H264)

/*! @} */

/*! \name MPEG2 Picture Coding Type
* @{
*/
#define I_TYPE		  1
#define P_TYPE		  2
#define B_TYPE		  3
#define D_TYPE		  4
#define DUMMY_TYPE	  5	   // used for record mp2 into AVI file in MTK recorder
//#define V_SEQ_HDR	     8
//#define V_GOP_HDR	     9
//#define V_SEQ_END	     10
//#define AGL_SEQ_END	    11
/*! @} */

/*! \name MPEG4 Picture Coding Type
* @{
*/
#define VIS_OBJ 	  ((u32)0x08b)   ///< visual_object_start_code,	   000001B5
#define VID_OBJ_LAY	  ((u32)0x085)   ///< video_object_layer_start_code, 000001[20-2f]
//#define VID_OBJ	      (__u32)0x084  ///< video_object_start_code,	000001[00~1f]
#define GOVOP		  ((u32)0x089)  ///< group_of_vop_start_code,	  000001B3
#define I_VOP		  ((u32)0x080)  ///< vop_start_code		  000001B6
#define P_VOP		  ((u32)0x081)  ///< vop_start_code		  000001B6
#define B_VOP		  ((u32)0x082)  ///< vop_start_code		  000001B6
#define S_VOP		  ((u32)0x083)  ///< vop_start_code		  000001B6
#define SH_I_VOP	  ((u32)0x098)  ///< short_video_start_marker
#define SH_P_VOP	  ((u32)0x099)  ///< short_video_start_marker
#define DX3_I_FRM	  ((u32)0x0f0)  ///< generated by firmware
#define DX3_P_FRM	  ((u32)0x0f1) ///< generated by firmware
#define DUMMY_FRM	  ((u32)0x0f2)  ///< generated by firmware, used for dummy frame, like vop_coded 0
/*! @} */

/*! \name H264 Picture Coding Type
* @{
*/
#define I_SLICE 	  ((u32)0x064)  ///< slice type 2 as I slice
#define P_SLICE 	  ((u32)0x065)   ///< slice type 0 as P slice
#define B_SLICE 	  ((u32)0x066)   ///< slice type 1 as B slice
#define SI_SLICE	  ((u32)0x067)   ///< slice type 4 as SI slice
#define SP_SLICE	  ((u32)0x068)   ///< slice type 3 as SP slice
#define I_ALL_SLICE	  ((u32)0x06a)   ///< slice type 7 as I slice, all I type slices in this picture
#define P_ALL_SLICE	  ((u32)0x06b)   ///< slice type 5 as P slice, all P type slices in this picture
#define B_ALL_SLICE	  ((u32)0x06c)   ///< slice type 6 as B slice, all B type slices in this picture
#define SI_ALL_SLICE	  ((u32)0x06d)   ///< slice type 9 as I slice, all SI type slices in this picture
#define SP_ALL_SLICE	  ((u32)0x06f)  ///< slice type b as P slice, all SP type slices in this picture
/*! @} */

/*! \name WMV Picture Coding Type
* @{
*/
#define IVOP	      ((u32)0x0a0)
#define PVOP	      ((u32)0x0a1)
#define BVOP	      ((u32)0x0a2)
#define BIVOP	      ((u32)0x0a3)
#define SKIPFRAME     ((u32)0x0a4)
/*! @} */

/*! \name Real Video Picture Coding Type
* @{
*/
#define INTRAPIC	  ((u32)0x0B0)
#define FORCED_INTRAPIC   ((u32)0x0B1)
#define INTERPIC	  ((u32)0x0B2)
#define TRUEBPIC	  ((u32)0x0B3)
/*! @} */



/*! \name Picture Coding Type Marco
* @{
*/

#define fgIsIType_MP2(arg)    ((arg & 0x0FF) == I_TYPE)

#define fgIsIType_MP4(arg)    (((arg & 0x0FF) == I_VOP) || \
			      ((arg & 0x0FF) == SH_I_VOP) || \
			      ((arg & 0x0FF) == DX3_I_FRM))

#define fgIsIType_AVC(arg)    ((arg & IDR_PIC) || \
			      ((arg & 0x0FF) == I_ALL_SLICE) || \
			      ((arg & 0x0FF) == I_SLICE) || \
			      ((arg & 0x0FF) == SI_SLICE) || \
			      ((arg & 0x0FF) == SI_ALL_SLICE))

#define fgIsIType_WMV(arg)   ((arg & 0x0FF) == IVOP)

#define fgIsPType_MP2(arg)   (((arg & 0x0FF) == P_TYPE) || \
				   ((arg & 0x0FF) == DUMMY_TYPE))

#define fgIsPType_AVC(arg)   (((arg & 0x0FF) == P_SLICE) || \
			     ((arg & 0x0FF) == SP_SLICE) || \
			     ((arg & 0x0FF) == P_ALL_SLICE) || \
			     ((arg & 0x0FF) == SP_ALL_SLICE))

#define fgIsPType_WMV(arg)   (((arg & 0x0FF) == PVOP) || \
			     ((arg & 0x0FF) == SKIPFRAME))

#define fgIsBType_MP2(arg)   ((arg & 0x0FF) == B_TYPE)

#if 0
#define fgIsPType_MP4(arg)   (((arg & 0x0FF) == P_VOP) || \
			     ((arg & 0x0FF) == S_VOP) || \
			      ((arg & 0x0FF) == SH_P_VOP) || \
			      ((arg & 0x0FF) == DX3_P_FRM) || \
			      ((arg & 0x0FF) == DUMMY_FRM))
#define fgIsBType_MP4(arg)   ((arg & 0x0FF) == B_VOP)

#else
#define fgIsPType_MP4(arg)   (((arg & 0x0FF) == P_VOP) || \
			      ((arg & 0x0FF) == S_VOP) || \
			      ((arg & 0x0FF) == SH_P_VOP) || \
			      ((arg & 0x0FF) == DX3_P_FRM))

#define fgIsBType_MP4(arg)   ((arg & 0x0FF) == B_VOP || \
			     ((arg & 0x0FF) == DUMMY_FRM))
#endif

#define fgIsBType_AVC(arg)   (((arg & 0x0FF) == B_SLICE) || \
			     ((arg & 0x0FF) == SP_SLICE) || \
			     ((arg & 0x0FF) == B_ALL_SLICE))

#define fgIsBType_WMV(arg)   (((arg & 0x0FF) == BVOP) || \
			     ((arg & 0x0FF) == BIVOP))


#define fgIsSeqHdr(arg)  ((arg & SEQ_HDR) || \
				(arg & SEQ_PS) || \
			  ((arg & 0x0FF) == VIS_OBJ) || \
			  ((arg & 0x0FF) == VID_OBJ_LAY) || \
			  ((arg & 0x0FF) == SH_I_VOP) || \
			  ((arg & 0x0FF) == DX3_I_FRM))
#define fgIsGopHdr(arg)  ((arg & GOP_HDR) || \
			  ((arg & 0x0FF) == GOVOP))
#define fgIsSeqEnd(arg)  (arg & SEQ_END)
#define fgIsAngleEnd(arg)  (arg & ANGLE_END)

#define fgIsIType(arg)	  ((((u32)arg & (u32)0x00FF) == (u32)(I_TYPE)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(I_VOP)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(SH_I_VOP)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(DX3_I_FRM)) || \
			   ((u32)arg & (u32)(IDR_PIC)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(I_ALL_SLICE)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(I_SLICE)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(SI_SLICE)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(SI_ALL_SLICE)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(IVOP)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(INTRAPIC)) || \
			   (((u32)arg & (u32)0x00FF) == (u32)(FORCED_INTRAPIC)))
#if 0
#define fgIsPType(arg)	 (((arg & 0x0FF) == P_TYPE) || \
					 ((arg & 0x0FF) == P_VOP) || \
					 ((arg & 0x0FF) == S_VOP) || \
					 ((arg & 0x0FF) == SH_P_VOP) || \
					 ((arg & 0x0FF) == DX3_P_FRM) || \
					 ((arg & 0x0FF) == DUMMY_TYPE) || \
					 ((arg & 0x0FF) == DUMMY_FRM) || \
					 ((arg & 0x0FF) == PVOP) || \
					 ((arg & 0x0FF) == P_SLICE) || \
					 ((arg & 0x0FF) == SP_SLICE) || \
					 ((arg & 0x0FF) == P_ALL_SLICE) || \
					 ((arg & 0x0FF) == SP_ALL_SLICE) || \
					 ((arg & 0x0FF) == SKIPFRAME))
#define fgIsBType(arg)	 (((arg & 0x0FF) == B_TYPE) || \
					 ((arg & 0x0FF) == B_VOP) || \
					 ((arg & 0x0FF) == BVOP) || \
					 ((arg & 0x0FF) == B_SLICE) || \
					 ((arg & 0x0FF) == SP_SLICE) || \
					 ((arg & 0x0FF) == B_ALL_SLICE) || \
					 ((arg & 0x0FF) == BIVOP))
#else
#define fgIsPType(arg)	 (((arg & (u32)0x0FF) == (u32)P_TYPE) || \
			 ((arg & (u32)0x0FF) == (u32)P_VOP) || \
			 ((arg & (u32)0x0FF) == (u32)S_VOP) || \
			 ((arg & (u32)0x0FF) == (u32)SH_P_VOP) || \
			 ((arg & (u32)0x0FF) == (u32)DX3_P_FRM) || \
			 ((arg & (u32)0x0FF) == (u32)DUMMY_TYPE) || \
			 ((arg & (u32)0x0FF) == (u32)PVOP) || \
			 ((arg & (u32)0x0FF) == (u32)P_SLICE) || \
			 ((arg & (u32)0x0FF) == (u32)SP_SLICE) || \
			 ((arg & (u32)0x0FF) == (u32)P_ALL_SLICE) || \
			 ((arg & (u32)0x0FF) == (u32)SP_ALL_SLICE) || \
			 ((arg & (u32)0x0FF) == (u32)SKIPFRAME) || \
			 ((arg & (u32)0x0FF) == (u32)INTERPIC))


#define fgIsBType(arg)	 (((arg & (u32)0x0FF) == (u32)B_TYPE) || \
			((arg & (u32)0x0FF) == (u32)B_VOP) || \
			((arg & (u32)0x0FF) == (u32)DUMMY_FRM) || \
			((arg & (u32)0x0FF) == (u32)BVOP) || \
			((arg & (u32)0x0FF) == (u32)B_SLICE) || \
			((arg & (u32)0x0FF) == (u32)SP_SLICE) || \
			((arg & (u32)0x0FF) == (u32)B_ALL_SLICE) || \
			((arg & (u32)0x0FF) == (u32)BIVOP) || \
			((arg & (u32)0x0FF) == (u32)TRUEBPIC))

#endif

#define fgIsM4vPic(arg)  (((arg & 0x0FF) & 0x80) > 0)
#define fgIsRefType(arg)  ((arg & REF_PIC) || \
				       (fgIsPType(arg)) || \
				       (fgIsIType(arg)))
#define fgIsH264IDRType(arg)  (arg & IDR_PIC)
#define fgIsH264IType(arg)  ((arg & IDR_PIC) || ((arg & 0x0FF) == I_ALL_SLICE))
#define fgIsH264FileDataIType(arg)  ((arg & IDR_PIC) || ((arg & 0x0FF) == I_ALL_SLICE) || ((arg & 0x0FF) == I_SLICE))
#define fgIsH264GopEntry(arg)  ((arg & SEQ_PS) && (fgIsH264IType(arg)))
#define fgIsH264FileDataGopEntry(arg)  ((arg & SEQ_PS) && (fgIsH264FileDataIType(arg)))

#define fgIsM4vShort(arg)  (((arg & 0x0FF) == SH_I_VOP) || \
					     ((arg & 0x0FF) == SH_P_VOP))

#define SetPicType(u4PicType, ePicType)  (u4PicType = (u4PicType & 0xFFFFFF00) | (ePicType))
#define GetPicType(u4PicType)  (u4PicType & 0x000000FF)
#define fgIsStillPic(u4PicType)   (((fgIsSeqHdr(u4PicType) && fgIsGopHdr(u4PicType)) || ((u4PicType&SEQ_PS) && (u4PicType&PIC_PS))) && fgIsIType(u4PicType) && fgIsSeqEnd(u4PicType))
#define fgIsDummyPic(arg)  ((arg & 0x0FF) == DUMMY_FRM)

/*! @} */


// *********************************************************************
// Picture Structure
// *********************************************************************
/*! \name Picture Structure
* @{
*/
#define TOP_FLD_PIC	  1
#define BTM_FLD_PIC	  2
#define FRM_PIC 	  3
// The follow 2 define use in Reference Field Picture
#define TWO_FLDPIC_TOPFIRST  4
#define TWO_FLDPIC_BTMFIRST  5
#define ERR_PIC_STRUCT	     0xFF
/*! @} */

// *********************************************************************
// Frame rate code
// *********************************************************************
/*! \name Frame rate code
* @{
*/
// frame_rate_code in Table 6-4 of 13818-2
#define FRC_23_976   1
#define FRC_24		 2
#define FRC_25		 3
#define FRC_29_97     4
#define FRC_30		 5
#define FRC_50		 6
#define FRC_59_94     7
#define FRC_60		 8
// Reserved by 13818-2 and defined here for other frame rates
#define FRC_1		  9
#define FRC_5		  10
#define FRC_8		  11
#define FRC_10		 12
#define FRC_12		 13
#define FRC_15		 14
#define FRC_16		 15
#define FRC_17		 16
#define FRC_18		 17
#define FRC_20		 18
// Reserved by WMV and defined here for other frame rates
#define FRC_2		  19
#define FRC_6		  20
#define FRC_48		 21
#define FRC_70		 22
#define FRC_120 	23
#define FRC_VAR 	24
#define FRC_MAX        (FRC_VAR+1)
/*! @} */

// *********************************************************************
// TV system
// *********************************************************************
/*! \name Tv system
* @{
*/
#define TVS_NTSC       1
#define TVS_PAL 	2
/*! @} */

// *********************************************************************
// Aspect ratio
// *********************************************************************
/*! \name Aspect ratio
* @{
*/
#define ASP_UNKNOW	      0     // unknow
#define ASP_1_1 	   1	 // SAR, 1:1
#define ASP_4_3 	   2	 // DAR, 4:3 PAN-SCAN (NORMAL)
#define ASP_16_9	  3	// DAR, 16:9 FULL
#define ASP_221_1	 4     // DAR, 2.21:1
#define ASP_4_3_LB	      5     // DAR, 4:3 LB
#define ASP_16_9_NORMAL 	 6     // DAR, 16:9 NORMAL
#define ASP_UNDEFINED	7
#define ASP_MAX_ASPECT_RATIO	8
/*! @} */

typedef enum {
  HDMI_PICTURE_4_3 = 0,
  HDMI_PICTURE_16_9
} HDMI_PICTURE_ASPECT_RATIO_T;


//the HDMI_AFD_FORMT_T is defined by CEA-861D spec Table 88 AFD coding, kenny 2008/7/1
typedef enum {
  HDMI_BOX_16_9 = 0x02, //box 16:9 (top)
  HDMI_BOX_14_9 = 0x03, //box 14:9 (top)
  HDMI_BOX_OVER_16_9 = 0x04, //box > 16:9 (center)
  HDMI_AS_PICTURE_AR = 0x08, //As the coded frame
  HDMI_4_3_CENTER = 0x09, //4:3 (center)
  HDMI_16_9_CENTER = 0x0a, //16:9 (center)
  HDMI_14_9_CENTER = 0x0b, //14:9 (center)
  HDMI_4_3_WITH_14_9_CENTER = 0x0d, //4:3 (with shoot & protect 14:9 center)
  HDMI_16_9_WITH_14_9_CENTER = 0x0e, //16:9 (with shoot & protect 14:9 center)
  HDMI_4_3_WITH_4_3_CENTER = 0x0f, //16:9 (with shoot & protect 4:3 center)

} HDMI_AFD_FORMT_T;




// Colour Primary
#define COLOR_PRIMARY_709				1
#define COLOR_PRIMARY_601				2


// *********************************************************************
// Pic Flag Info
// *********************************************************************
/*! \name Pic Flag Info
* @{
*/
#define FBG_FLG_ALL				    (0xFFFFFFFF)
#define FBG_FLG_PROGRESSIVE_SEQ 	(0x1 << 0)     ///<  progressive seq
#define FBG_FLG_PROGRESSIVE_FRM 	(0x1 << 1)	///<  progressive frm
#define FBG_FLG_TOP_FLD_FIRST		   (0x1 << 2)	   ///<  Top field first
#define FBG_FLG_REPEAT_1ST_FLD		  (0x1 << 3)	  ///<	repeat 1st field
#define FBG_FLG_B_PIC_IN_RA		    (0x1 << 4)	    ///<  B pic in Random Access
#define FBG_FLG_1_FLD_PIC		      (0x1 << 5)      ///<  frame buffer constructed by 1 field pic
#define FBG_FLG_2_FLD_PIC		      (0x1 << 6)      ///<  frame buffer constructed by 2 field pic
#define FBG_FLG_NIPB_2_IPB		     (0x1 << 7)      ///<  1 pic decode from !IPB to IPB
#define FBG_FLG_WITH_XVYCC		   (0x1 << 8)	   ///<  XVYCC Bitstream
#define FBG_FLG_DRIP_PIC		       (0x1 << 9)      ///<  Drip picture
#define FBG_FLG_OPEN_B			       (0x1 << 10)	///<  Open B picture
#define FBG_FLG_ADR_SWAP_ON		(0x1 << 11)	///<  Address swap mode on/off
#define FBG_FLG_RASTER_SCAN_MODE		    (0x1 << 12)     ///<  Address swap mode
#define FBG_FLG_PTS_CHK 		      (0x1 << 13)     ///<  PTS needs to check
#define FBG_FLG_FORCE_DISP		  (0x1 << 14)	  ///<	Force disp
#define FBG_FLG_PTS_RESET		    (0x1 << 15)     ///<  PTS reset
#define FBG_FLG_INTERLACE_FRM	       (0x1 << 16)     ///<  Interlaced Frm
#define FBG_FLG_REAL_PROGRESSIVE_FRM	 (0x1 << 17)	 ///<  BD and Progressive frame
#define FBG_FLG_USE_PTS 		       (0x1 << 18)
#define FBG_FLG_NR_PROCESSED			(0x1 << 19)
#define FBG_FLG_DISPLAY_REF		    (0x1 << 20)
#define FBG_FLG_BD_DISC 			(0x1 << 21)
#define FBG_FLG_AVC_CODEC		     (0x1 << 22)
#define FBG_FLG_DECODING		    (0x1 << 23)
#define FBG_FLG_L_SIGHT 		   (0x1 << 24)	    ///<  MVC for L sight frame
#define FBG_FLG_R_SIGHT 		   (0x1 << 25)	    ///<  MVC for R sight frame
#define FBG_FLG_SVCD_DISC		     (0x1 << 26)      ///<  MVC for R sight frame
#define FBG_FLG_LAST_TFX_FRAME			(0x1 << 27)
#define FBG_FLG_SHARP_PROCESSED 	     (0x1 << 28)

/*! @} */

/*! \name Picture Coding Type Marco
* @{
*/
#define fgIsFbFlagSet(arg1, arg2)  (arg1 & arg2)
#define vSetFbFlag(arg1, arg2)	(arg1 |= arg2)
#define vClrFbFlag(arg1, arg2)	(arg1 &= (~arg2))

#define fgIs3DSource(arg)  (arg & (FBG_FLG_L_SIGHT | FBG_FLG_R_SIGHT))

// *********************************************************************
// VDSCL Flag Info
// *********************************************************************
/*! \name Pic Flag Info
* @{
*/
#define VDSCL_FLG_ALL				      0xFFFFFFFF
#define VDSCL_FLG_NO_VERTICAL_SUGGEST	      		(0x1 << 0)
/*! @} */

/*! \name Picture Coding Type Marco
* @{
*/
#define fgIsVDSCLFlagSet(arg1, arg2)  (arg1 & arg2)
#define vSetVDSCLFlag(arg1, arg2)  (arg1 |= arg2)
#define vClrVDSCLFlag(arg1, arg2)  (arg1 &= (~arg2))


// *********************************************************************
// Block size
// *********************************************************************
/*! \name Block size
* @{
*/
#define BLOCKSIZE_H					       16
#define BLOCKSIZE_V					       32
//#define DRV_ALIGN_MASK(value, mask)			(((value) + (mask - 1)) & (~(mask - 1)))
#define DRV_ALIGN_MASK(value, mask)			((((value) + ((mask) - 1)) / (mask)) * (mask))
/*! @} */

// *********************************************************************
// DSP reserved fifo size
// *********************************************************************
/*! \name DSP reserved fifo size
* @{
*/
#define DSP_RESERVED_AUDIO_MAX_SZ		24576  //24k byte  /*!< (Legacy Playback Special) */
#define DSP_RESERVED_AUDIO_LPCM_SZ		24576  //24k byte
#define DSP_RESERVED_AUDIO_AC_3_SZ		24576  //24k byte
#define DSP_RESERVED_AUDIO_DTS_SZ		24576  //24k byte
#define DSP_RESERVED_AUDIO_DOLBY_LOSSLESS_SZ	24576  //24k byte
#define DSP_RESERVED_AUDIO_DD_PLUS_PRI_SZ	24576  //24k byte
#define DSP_RESERVED_AUDIO_DTS_HD_NO_XLL_SZ	24576  //24k byte
#define DSP_RESERVED_AUDIO_DTS_HD_XLL_SZ	24576  //24k byte
#define DSP_RESERVED_AUDIO_DD_PLUS_SEC_SZ	24576  //24k byte
#define DSP_RESERVED_AUDIO_DTS_HD_LBR_SZ	24576  //24k byte
#define DSP_RESERVED_AUDIO_MPEG_SZ		24576  //24k byte
#define DSP_RESERVED_AUDIO_DOLBY_TRUE_HD_COMPATIBLE_MODE_SZ 24576  //24k byte



// *********************************************************************
// xvYCC Info
// *********************************************************************
/*! \name xvYCC Info
* @{
*/
/// xvYCC data
typedef  enum
{
  XVYCC_TYPE_NONE = 0,
  XVYCC_TYPE_AVCHD = 1,
  XVYCC_TYPE_AVCHD_FORCE = 2,
} XVYCC_TYPE_T;

typedef struct
{
    XVYCC_TYPE_T exvYCCType;
    UINT32 u4RedData;
    UINT32 u4GreenData;
    UINT32 u4BlueData;
}XVYCC_INFO_T;



// *********************************************************************
// Disc Type
// *********************************************************************
/*! \name Disc Type
* @{
*/
typedef enum
{
  DT_BD = 0,		///< BD
  DT_VCD,		 ///< VCD / SVCD
  DT_DVD_VIDEO,    ///< DVD video
  DT_DVD_AUDIO,    ///< DVD audio
  DT_DVD_MVR,	     ///< DVD -VR
  DT_DVD_PVR,	      ///< DVD + VR
  DT_DATADISC,	    ///< Data Disc
  DT_NRD,		 ///< Netflix
  DT_FLV,		 ///< FLV
  DT_UNKNOW	    ///< unknow
}DiscType;
/*! @} */

//#define fgIsSVCDData(eDiscType, fgMPEG2)  (FALSE)
#define fgIsSVCDData(eDiscType, fgMPEG2)  ((eDiscType == DT_VCD) && fgMPEG2)
#define fgIsBDData(eDiscType)  (eDiscType == DT_BD)
#define fgIsFileData(eDiscType)  (eDiscType == DT_DATADISC)
#define fgIsDVDData(eDiscType)	(eDiscType == DT_DVD_VIDEO || eDiscType == DT_DVD_AUDIO || eDiscType == DT_DVD_MVR || eDiscType == DT_DVD_PVR)
#define fgIsProgressiveBDData(eDiscType, fgProgressFrm)  ((eDiscType == DT_BD) && fgProgressFrm)

#ifdef __cplusplus
}
#endif

#endif //DRV_COMMON_H

