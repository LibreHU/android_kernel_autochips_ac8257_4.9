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
#ifndef _X_DRV_COMM_H_
#define _X_DRV_COMM_H_

/** In-band command types.
 */

typedef enum
{
    /**  Set STC (System Time Clock) value of the stream for all the paths created for the stream. */
    IBC_InbandCmdTypeSetStc,
    /**  Set the absolute time offset between the STC and the actual stream
     *   time.
     */
    IBC_InbandCmdTypeTimeOffset,
    /**  Indicates no data will be sent for the stream until processing is
     *   resumed by issuing HBI_Dec_Still(false).
     */
    IBC_InbandCmdTypeStill,
    /**  Indicates no more data following this command */
    IBC_InbandCmdTypeEndOfStream,
    /**  Notify when all data sent before command has been successfully processed */
    IBC_InbandCmdTypeNotify,
    /**  Stream has a discontinuity at this point.
     *   platform should clear its internal buffers and restart decoding
     *   at the next appropriate access point (e.g. an I-picture or a sequence header). */
//    IBC_InbandCmdTypeDiscontinuity,
    /**  To set the secret parameter needed for CPS (see [HBI_CPS]) */
    IBC_InbandCmdTypeSetSecretParameter,
    /**  To set the play list and play item for CPS (see [HBI_CPS]) */
    IBC_InbandCmdTypeSetPlayListPlayItem,
    /**  To set the cache clip info for RE 2.0 (see [HBI_CPS]). */
    IBC_InbandCmdTypeSetCacheClip,
    /**  To set the out time of the demux want to crop. */
    IBC_InbandCmdTypeSetDmxOutTime,
    /**  The DecDispOneFrame inband command made VDec decode one I-VOP frame
     *   when PicType is VID_DEC_PIC_TYPE_1ST_I_AFTER_SEQ_HDR
     */
    IBC_InbandCmdTypeDecDispOneFrame,

    /** MTK private define, ignore PTS. When dmx_sw gets this command, it will set all subsequent PTS to be invaild */
    IBC_InbandCmdTypeSetIgnorePTS,

    /** MTK private define. When VDP gets this command & the first picture displayed,\
        it will notify middleware. */
    IBC_InbandCmdTypeFirstPictureDisplay,

    /** Streams auto pause. When VDP or ADP gets this command, it will pause display,\
      it will notify middleware. */
    IBC_InbandCmdTypeAutoPause,

    /** To set the title id for CPS CSS */
    IBC_InbandCmdTypeSetTitleId,
    /** CPS info */
    IBC_InbandCmdTypeCpsCommonInfo,
    /** DVD-Audio CPS Info */
    IBC_InbandCmdTypeCpsDVDAInfo,
    /** SACD CPS Info */
    IBC_InbandCmdTypeCpsSACDInfo,

    /** Set video aspect ratio info */
    IBC_InbandCmdTypeSetVidAspectRatio,

    /** Inquiry the first pts */
    IBC_InbandCmdTypeInquiryFirstPTS,

    /** End of current data range */
    IBC_InbandCmdTypeEndOfRange,

    /** End of sequence for slow reverse one sequence */
    IBC_InbandCmdTypeSetSRSeqEnd,

    /** Set SRNotRequestPrevIFrame inband command after the current Sequence which
     *  is on the boundary of Non-Seamless SR playback. This means if driver found
     *  the boundary frame is open GOP case, driver can not decode the boundary
     *  frame even driver request the data from the previous Sequence.
     */
    IBC_InbandCmdTypeSetSRNotRequestPrevIFrame,

    /**  The DecDispLastFrame inband command made VDec decode last frame of chunk data
     *   when PicType is VID_DEC_PIC_TYPE_1ST_I_AFTER_SEQ_HDR
     */
    IBC_InbandCmdTypeDecDispLastFrame,

    /** Like IBC_InbandCmdTypeSetSRNotRequestPrevIFrame, but VDec will continue request
     *  next epmap, this is for BD only
     */
    IBC_InbandCmdTypeSetSRNotRequestPrevIFrameAndContinue,
  /**use PipMetaDataInband to replace pip time trigger2  for contolling pip display accurately. 
       *BDMW will judge whether the pipMeata is changed at the beginning of Fragment.if changed,will send this InbandCmd.
       */
    IBC_InbandCmdTypeMetaDataInfo,

    /** Indicate Splitter/CFA that it is a Netflix Data Info, containing audio stream id, video stream id, packet type (video/audio/mux), etc,
     *  this is for Netflix Adaption (Legacy) only
     */
    IBC_InbandCmdTypeNrdDataInfo,

    /** don't use this, just for get inband command type count **/
    IBC_InbandCmdMax
} IBC_InbandCmdType; //HBI_Dec_InbandCmdType

/** Action types that can be passed as parameters to in-band commands.
 */

typedef enum
{
    /** Continue processing after executing the in-band command */
    IBC_ActionTypeContinue,
    /** Suspend data and in-band command processing: obsolete. */
    IBC_ActionTypeSuspend,
    /** Continue processing after executing the in-band command, but don't need callback to MW */
    IBC_ActionTypeContinue_NoCallback
} IBC_ActionType; //HBI_Dec_ActionType

/** How to load the System Time Clock, the 33-bits clock that forms the
 *  engine of the presentation engine.
 */

typedef enum
{
    IBC_StcLoadMode_Never,              /**< Never load the STC, so it
                                             *   just runs from its random
                                             *   start value.
                                             */
    IBC_StcLoadMode_LoadFromStream,     /**< Load the STC every time a
                                             *   transport stream packet
                                             *   contains a PCR.
                                             */
    IBC_StcLoadMode_LoadFromStreamOnce, /**< Load the STC with the first
                                             *   PCR; then just keep
                                             *   counting.
                                             */
    IBC_StcLoadMode_LoadFromValue,      /**< Load the STC with the
                                             *   supplied value.
                                             */
    IBC_StcLoadMode_LoadFromScr,
    IBC_StcLoadMode_LoadFromDataTimestamp,
    IBC_StcLoadMode_LoadFromVideoPlayback

} IBC_StcLoadMode; //HBI_Dec_StcLoadMode

/** Attributes that define the parameter for the in-band command of type
 *  "set STC".
 */
typedef struct
{
    IBC_StcLoadMode mode;   /**< How to load the STC. */
    INT64 time;          /**< Time value to be set as the STC
                                 *   (System Time Clock). The data following
                                 *   this inband command will be synchronized
                                 *   to the new time base.
                                 */
} IBC_SetStcParams; //HBI_Dec_SetStcParams

/** Attributes that define the parameter for the in-band command of type
 *  "set STC".
 */
typedef struct
{
    INT64 time;          /**< Time offset between STC and stream. */
} IBC_TimeOffsetParams; //HBI_Dec_TimeOffsetParams

/** Attributes that define the parameter for the in-band command of type still
 */
typedef struct
{
    INT32 Reserved;/** Reserved */
} IBC_StillParams; //HBI_Dec_StillParams

/** Attributes that define the parameter for the in-band command of type end of stream.
 */
typedef struct
{
    BOOL fg_notify_cfa_end; 
} IBC_EndOfStreamParams; //HBI_Dec_EndOfStreamParams

/** Attributes that define the parameter for the in-band command of type notify.
 */
typedef struct IBC_NotifyParams_t
{
	INT32 Reserved;/** Reserved */
} IBC_NotifyParams; //HBI_Dec_NotifyParams

/** Attributes that define the parameter for the in-band command of type Discontinuity.
 */
typedef struct
{
    INT32 Reserved;/** Reserved */
} IBC_DiscontinuityParams; //HBI_Dec_DiscontinuityParams

/** Attributes that define the secret parameter for CPS (see [HBI_CPS]).
 */
typedef struct
{
    /** The secret parameter handle. See [HBI_CPS].
     *  Once the secret parameter handle has been sent through an in-band command by BDMW
     *  to the platform, ownership of the handle is passed to the platform and the BDMW
     *  must no longer use it. The platform is responsible for freeing the handle after it
     *  is no longer in use.
     */
    UINT32 cpsSpSlotHandle;
    UINT32 spn;
} IBC_SetSecretParameterParams; //HBI_Dec_SetSecretParameterParams

/** Attributes that define the play list and play item neccesary for CPS (see [HBI_CPS]).
 */
typedef struct
{
    /** The play list id */
    INT32 playListId;
    /** The play item id */
    INT32 playItemId;
} IBC_SetPlayListPlayItemParams; //HBI_Dec_SetPlayListPlayItemParams

typedef struct
{
    /** The title id */
    UINT32 titleId;
} IBC_SetTitleIdParams; //IBC_InbandCmdTypeSetTitleId

/** Attributes that define the CacheClip parameter for CPS / RE 2.0 (see
 *  [HBI_CPS]).
 */
typedef struct
{
    UINT32 dir;
    UINT32 clip;
} IBC_SetCacheClipParamsDef;

/** Attributes for the demuxer out time inband command.
 */
typedef struct
{
    INT64 time;               /**< Out time of demuxer to crop. */
} IBC_SetDmxOutTimeParamsDef;

/** Attributes for the decoder display one frame inband command.
 */
typedef struct
{
    INT32 Reserved;/** Reserved */
} IBC_DecDispOneFrameParamsDef;

typedef struct
{
  INT32 Reserved;/** Reserved */
} IBC_FirstPictureDisplayParams;

typedef struct
{
    INT32 Reserved;/** Reserved */
} IBC_DecDispLastFrameParamsDef;

typedef enum
{
    IBC_NrdDataType_PacketMux,
    IBC_NrdDataType_PacketAudio,
    IBC_NrdDataType_PacketVideo
} IBC_NrdDataType;

typedef struct
{
    IBC_NrdDataType e_type;
    UINT32 ui4_audio_stream_id;
    UINT32 ui4_video_stream_id;
    UINT64 ui8_preroll;
    UINT32 ui4_packet_size;
} IBC_NrdDataInfoParamsDef;

/* for IBC_CpsCommonInfoParamsDef.u4InfoValid */
#define IBC_CPS_INFO_CGMS_VALID       (UINT32)(1)
#define IBC_CPS_INFO_APS_VALID        (UINT32)(1 << 1)
#define IBC_CPS_INFO_ANALOG_SRC_VALID (UINT32)(1 << 2)
#define IBC_CPS_INFO_ICT_VALID        (UINT32)(1 << 3)
#define IBC_CPS_INFO_DOT_VALID        (UINT32)(1 << 4)
#define IBC_CPS_INFO_CSS_VALID        (UINT32)(1 << 5)
#define IBC_CPS_INFO_AACS_VALID       (UINT32)(1 << 6)
#define IBC_CPS_INFO_EPN_VALID        (UINT32)(1 << 7)
#define IBC_CPS_INFO_NPCNT_VALID        (UINT32)(1 << 8) //NotPassCnt
#define IBC_CPS_INFO_DCICCI_VALID        (UINT32)(1 << 9)


/* for IBC Cps info CGMS type definition */
#define IBC_CPS_INFO_CGMS_COPY_PERMITTED      (UINT32)(0)
#define IBC_CPS_INFO_CGMS_NO_MORE_COPY        (UINT32)(1)
#define IBC_CPS_INFO_CGMS_ONE_COPY_ALLOWED    (UINT32)(2)
#define IBC_CPS_INFO_CGMS_NO_COPY_PERMITTED   (UINT32)(3)
#define IBC_CPS_INFO_CGMS_RESERVED            (UINT32)(4)


/* for IBC Cps info CSS flag definition */
#define IBC_CPS_INFO_CSS_EXIST        (UINT32)(1)
#define IBC_CPS_INFO_CSS_DISC         (UINT32)(1 << 1)
#define IBC_CPS_INFO_CSS_TITLE        (UINT32)(1 << 2)


/* must not pass content to a specific output device for wmdrm output control */
#define IBC_CPS_INFO_NOT_PASS_TO_DIGITAL_VID  (UINT32)(1)       /* must not pass content to digital video outputs */
#define IBC_CPS_INFO_NOT_PASS_TO_ANALOG_TV    (UINT32)(1 << 1)  /* must not pass content to analog television outputs */


/** Attributes for the cps common info inband command.
 */
typedef struct
{
    UINT32 u4InfoValid;
    UINT8 u1Cgms;
    UINT8 u1Aps;
    UINT8 u1AnalogSrc;
    UINT8 u1ICT;
    UINT8 u1DOT;
    UINT8 u1CSS;
    UINT8 u1AACS;
    UINT8 u1EPN;
    UINT8 u1NotPassCnt; // must not pass content to a specific output device for wmdrm output control
    UINT8 u1DCICCI;
} IBC_CpsCommonInfoParamsDef;

/** Attributes for the cps dvd audio info inband command.
 */
typedef struct
{
    /* audio_copy_permission, audio_copy_number,
       audio_quality, audio_transaction */
    UINT8 u1DVDAInfo;
} IBC_CpsDVDAInfoParamsDef;

/** Attributes for the cps sacd info inband command.
 */
typedef struct
{
    /* */
    UINT8 u16SACDInfo[16];
} IBC_CpsSACDInfoParamsDef;

typedef enum
{
    IBC_VideoAspectRatioType_Unknown = 0,             /**< Video aspect ratio unknown. The platform may
                                                          either detect the aspect ratio during decode
                                                          or assume a default aspect ratio. Seamless
                                                          video playback is not guaranteed in such a
                                                          situation. */
    IBC_VideoAspectRatioType_4x3_Full,                /**< 4:3 */
    IBC_VideoAspectRatioType_14x9_Letterbox,          /**< 14:9 letterbox center */
    IBC_VideoAspectRatioType_14x9_LetterboxTop,       /**< 14:9 letterbox top */
    IBC_VideoAspectRatioType_16x9_Letterbox,          /**< 16:9 letterbox center */
    IBC_VideoAspectRatioType_16x9_LetterboxTop,       /**< 16:9 letterbox top */
    IBC_VideoAspectRatioType_Greater_16x9_Letterbox,  /**< > 16:9 letterbox center */
    IBC_VideoAspectRatioType_14x9_Full,               /**< 14:9 */
    IBC_VideoAspectRatioType_16x9_Full,               /**< 16:9 */
    IBC_VideoAspectRatioType_2_21x1                   /**< 2.21:1 */
} IBC_VideoAspectRatioType;

typedef struct
{
    IBC_VideoAspectRatioType aspectRatio;
    INT64                    time;               /**< Pts information. */
} IBC_SetVidAspectRatioParamsDef;

typedef struct
{
    BOOL nextSeqExist;
    BOOL videoNotExist;
    UINT64 u8NoVideoSeqOffset;
    INT64 time;
} IBC_SetSRSeqEndParamsDef;

typedef struct
{
    INT64                    endTime;            /**< Decoding end time pts information. */
} IBC_SetSRNotRequestPrevIFrameAndContinueParamsDef;

typedef struct
{
    INT64 time;
    INT32 x;
    INT32 y;
    INT32 width;
    INT32 height;
    BOOL  fullscr;
} IBC_MetaDataPosInfoParamsDef;

#define IBC_PIPMETADATA_POSINFO_IN_CHUNK_NUM 5
typedef struct
{
    BOOL fgIsPipLumaKeyingOn;
    UINT8  u1PipUpperLuma;
    UINT8  u1PipPosInChunkNum;
    IBC_MetaDataPosInfoParamsDef pPipPosInfo[IBC_PIPMETADATA_POSINFO_IN_CHUNK_NUM]; 
} IBC_MetaDataInfoParamsDef;

/** Attributes for in-band commands depending on the in-band command type
 */
typedef union
{
    /** Set stc command params */
    IBC_SetStcParams stc;
    /** Time offset command parameters. */
    IBC_TimeOffsetParams offset;
    /** Still command params */
    IBC_StillParams still;
    /** End of stream command params */
    IBC_EndOfStreamParams endOfStream;
    /** Notify command params */
    IBC_NotifyParams notify;
    /** Discontinuity command params */
    IBC_DiscontinuityParams discontinuity;
    /** Set Secret Parameter command params */
    IBC_SetSecretParameterParams setSecretParameters;
    /** Set PlayList and PlayItem command params */
    IBC_SetPlayListPlayItemParams setPlayListPlayItem;
    /** Set TitleId command params */
    IBC_SetTitleIdParams setTitleId;
    /** Set CacheClip command parameters. */
    IBC_SetCacheClipParamsDef setCacheClip;
    /** Set DmxOutTime command parameters. */
    IBC_SetDmxOutTimeParamsDef setDmxOutTime;
    /** DecDispOneFrame command parameters. */
    IBC_DecDispOneFrameParamsDef decDispOneFrame;
    /** Set First Picture Display command params */
    IBC_FirstPictureDisplayParams firstPictureDislay;
    /** Cps command info params */
    IBC_CpsCommonInfoParamsDef cpsCommonInfo;
    /** Cps dvd audio info params */
    IBC_CpsDVDAInfoParamsDef cpsDVDAInfo;
    /** Cps sacd info params */
    IBC_CpsSACDInfoParamsDef cpsSACDInfo;
    /** Set video aspect ratio params */
    IBC_SetVidAspectRatioParamsDef vidAspectRatio;
    /** Set slow reverse sequence end params */
    IBC_SetSRSeqEndParamsDef setSRSeqEnd;
    /** Set slow reverse not request previous I frame params */
    IBC_SetSRNotRequestPrevIFrameAndContinueParamsDef setSRNotRequestPrevIFrameAndContinue;

    IBC_MetaDataInfoParamsDef metaDataInfo;
    /** Nrd data info parameters. */
    IBC_NrdDataInfoParamsDef nrdDataInfo;
    /** DecDispLastFrame command parameters. */
    IBC_DecDispLastFrameParamsDef decDispLastFrame;
} IBC_InbandCmdUnion; //HBI_Dec_InbandCmdUnion

/** Attributes that define an in-band command.
 */
typedef struct
{
    IBC_InbandCmdType cmd;      /**< Inband command type */
    IBC_ActionType action;      /**< Inband command action type */
    IBC_InbandCmdUnion params;  /**< Depending on the actual type */
} IBC_InbandCmd; //HBI_Dec_InbandCmd;


typedef union
{
  UINT32 u4Pid;
  UINT32 u4Uid;
} IBC_ID_UNION;


typedef struct DRV_INBAND_CMD_TAG
{
  IBC_InbandCmd rMwInbandCmd;
  INT32 i4Id;
} DRV_INBAND_CMD;


typedef struct PBBUF_INBAND_CMD_TAG
{
  UINT32 u4NumPaths;
  IBC_ID_UNION uPathId[8];
  DRV_INBAND_CMD rDrvInbandCmd;
  UINT32 u4TransitionID;
} PBBUF_INBAND_CMD;

#endif // _X_DRV_COMM_H
