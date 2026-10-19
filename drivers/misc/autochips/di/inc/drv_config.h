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
#if ! defined(DRV_CONFIG_H)
#define DRV_CONFIG_H

//#include "section.h"
#include "drv_def.h"

#define CONFIG_DRV_QUICK_START_BOOT             0

#define CONFIG_DRV_FPGA_BOARD                   0
#define CONFIG_DRV_ONLY                         1
#define CONFIG_DRV_VERIFY_SUPPORT               0
#define CONFIG_DRV_SEMIHOSTING_SUPPORT          0
#define CONFIG_DRV_BLACKSCREEN_PROC             0

//#if CONFIG_SECTION_BUILD_ARM2
#if CONFIG_DRV_MT8520
#define DEFAULT_ARM2
#define DSP_CLK_256M
#define CONFIG_DRV_NR_SUPPORT                   0
#else
#define CONFIG_DRV_NR_SUPPORT                   0
#endif
//#endif

#define USE_ONE_ESM_FRO_AUDIO

#define CONFIG_DRV_SHUT_DOWN_ARM2               0
#define CONFIG_DRV_ENABLE_DCM                   0
#define CONFIG_DRV_ENABLE_POST_PROC             0
#define CONFIG_DRV_3D_SUPPORT                   0
#define CONFIG_DRV_HDWN_SUPPORT		            1
#define CONFIG_DRV_WIFI_SUPPORT		            0
#define CONFIG_DRV_ALTHD_SUPPORT	            0
#define CONFIG_DRV_IFCON_AV_OUT                 0

#define CC_EMULATION			                0
#define __MODEL_slt__			                0
#define ENABLE_MULTIMEDIA		                1
#define SYNC_PES_HEADER		                    1
#define CONFIG_DRV_PCR_VERIFY		            0
#define DEMOD_VERIFY			                0
#define PLAYCARD_VERIFY		                    0
#define PVR_ISR_QUEUE_SUPPORT                   0
#define PVR_FIFO_WATER_LV                       0

#define CONFIG_DRV_KEYBOARD_SUPPORT		        0
#define CONFIG_DRV_AUDIO_IN_SUPPORT		        0
#define CONFIG_SONY_DRV_AUDIO_IN_SUPPORT		0
#define CONFIG_SONY_DRV_TVS_SUPPORT             0
#define CONFIG_DRV_SPDIF_TYPE_LC89058           0
#define CONFIG_DRV_SPDIF_TYPE_CS8415            1
#define CONFIG_DRV_HDMI_RX_IN_SUPPORT		    0
#define CONFIG_DRV_MIC_SUPPORT		            0

#define CONFIG_DRV_SPDIF_TYPE                   CONFIG_DRV_SPDIF_TYPE_LC89058

#ifdef __linux__
#define CONFIG_DRV_LINUX                        1
#else
#define CONFIG_DRV_LINUX                        0
#endif

#define CONFIG_DRV_LINUX_DATA_CONSISTENCY       0

#define	CONFIG_DRV_ACON_STANDBY	                0
#define	CONFIG_DRV_DISABLE_ARM2	                0
#define	CONFIG_MINI_OS	                        0

#define CONFIG_DRV_FAST_LOGO                    0
#define CONFIG_DRV_FASTBOOT_LOGS_OFF            0
#define CONFIG_DRV_CUSTOMER_FASTLOGO            0
#define CONFIG_DRV_BONDING_ENABLE               0
#define CONFIG_DRV_AVM_ENABLE                   0
#define CONFIG_DRV_ANIMATION_ENABLE             0
#define CONFIG_DRV_SUSPEND_TO_DRAM              0
#define CONFIG_DRV_OPWRSB_PU                    0
#define CONFIG_DRV_TRAY_IN_ACON                 0

// Audio Sound Effects config version 2
#define CONFIG_SCC_AUD_SE_SUPPORT               0
// (aud_se_v2)
#define CONFIG_AUD_SE_V2_EN                     1

#define CONFIG_AUD_SE_MW_CLI_SUPPORT            0

#define CONFIG_DRV_SUPPORT_DVD_AUDIO            0

#define CONFIG_DRV_HIGH_BITRATE_SPECIAL_PROC    1		// For PVR HW Command Queue

#define CONFIG_DRV_DVD_24P_SUPPORT              0
#define CONFIG_DRV_SUPPORT_PARTY                0

#define CONFIG_SUPPORT_MCU_SIF                  0

#define CONFIG_DRV_SUPPORT_LINUX_DLL_SPLITTER   0

#define CONFIG_OSD5_SUPPORT                     0

#define CONFIG_DRV_VIRTUAL_ADDR                 1

#define CONFIG_CCIR_COMPANION_CHIP              0

#define CONFIG_DRV_SUPPORT_FLV                  1

#define CONFIG_DRV_SUPPORT_SORENSON_H263        1

#define CONFIG_DRV_SUPPORT_RM                   1

#define CONFIG_DRV_PWMDAC_SUPPORT               0

#define CONFIG_DRV_LINEIN_PWMDAC_SUPPORT        0

#define CONFIG_DRV_ISPDIF_IN_SUPPORT            0

#define CONFIG_DRV_ISPDIF_IN_MASTERMODE_SUPPORT 0

#define CONFIG_START_BIT                        0
#define CONFIG_SUPPORT_MICOM                    0
#define	CONFIG_DRV_MP_ICE                       0

#define CONFIG_DRV_OPWRSB_PU_SUPPORT            0

#define CONFIG_PQ_CLI_SUPPORT                   0

#define CONFIG_DRV_YMH_SUPPORT                  0

#define CONFIG_DRV_SSH_SUPPORT                  0

#define CONFIG_DRV_ALSA_SOUND_SUPPORT           0

#define CONFIG_DRV_V4L2_SUPPORT                 0

//for Streaming button sound
#define CONFIG_AUD_BTN_MEM_PHASE3               0

#define CONFIG_AUD_CD_RIPPING_EN                0

#define CONFIG_AUD_DUAL_PRIMARY_SUPPORT         1
//#define DYNAMIC_ALLOC_AFIFO
#define CONFIG_AUD_DUAL_PRIMARY_ASRC_SUPPORT    1

#define CONFIG_AUD_DVD_MIX_SUPPORT              1

#define CONFIG_AUD_DSP_CHECK_MUTE_SUPPORT       0  // aud DSP CHECK MUTE support
#define CONFIG_AUD_VOL_GAIN_ADJUST              1

#define CONFIG_AUD_EARPHONE_CHECK_SUPPORT       0

#define CONFIG_AUD_HDMI_CLK_SUPPORT             0

#define CONFIG_DUALCORE_ARM2                    0

#define CONFIG_DRAM256_MODEL                    0

#if (CONFIG_DUALCORE_ARM2 && CONFIG_DRAM256_MODEL)
#error "Dram256 Model isn't enough to support DualCore!!"
#endif

#define CONFIG_DRV_FPORTABLE_SUPPORT            0

#define CONFIG_DRV_PAS45_SUPPORT                0

#define CONFIG_AUD_WORKBUF_384M_EN              0

#define CONFIG_LOGO_AUTO_PARSE_SUPPORT          0

#define CONFIG_EXT_VIDEO_CHIP_SUPPORT           0

#define CONFIG_DRAM256_PLUS_NEWFEATURE          0

#define CONFIG_DRV_NEW_SD_MODE_SUPPORT          0

#define CONFIG_FAST_POWER_DOWN                  0

#define CONFIG_DRV_CEC_PULLUP_SUPPORT           0


#ifdef __linux__
#define UNIFORM_DRV_CALLBACK                    0
#define CONFIG_SYS_MEM_PHASE3                   0
#define __UNDER_WINCE__                         0
#define  CONFIG_SUSPEND_TO_DRAM                 0
#define CONFIG_SYS_MEM_PHASE2                   0
//#include "drv_config_cps.h"

#else
#define __UNDER_WINCE__                         1
#endif

#endif //DRV_COMMON_H

