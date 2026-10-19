#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include/ \
             -I$(srctree)/drivers/misc/mediatek/video/common/ \
             -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/videox/ \
             -I$(srctree)/drivers/misc/mediatek/base/power/ac8257/ \
             -I$(srctree)/drivers/misc/mediatek/lcm/inc \
             -I$(srctree)/drivers/misc/mediatek/m4u/$(MTK_PLATFORM)/ \
             -I$(srctree)/drivers/misc/mediatek/mmp/

ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/$(MTK_PLATFORM)/dispsys

ccflags-y += \
		-I$(srctree)/drivers/misc/mediatek/cmdq/v3/

ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/$(MTK_PLATFORM)/


obj-y += ddp_color.o

ifeq ($(strip $(CONFIG_MTK_PQ_COLOR_MODE)),"MDP")
    ccflags-y += -DDISP_COLOR_OFF
else ifeq ($(strip $(CONFIG_MTK_PQ_COLOR_MODE)),"OFF")
    ccflags-y += -DDISP_COLOR_OFF
endif

ccflags-y += -DCCORR_SUPPORT

ccflags-y += -DSUPPORT_WCG

ifeq ($(strip $(CONFIG_MTK_PQ_COLOR_MODE)),"MDP")
    ccflags-y += -DMDP_COLOR_ON
else ifeq ($(strip $(CONFIG_MTK_PQ_COLOR_MODE)),"DISP_MDP")
    ccflags-y += -DDISP_MDP_COLOR_ON
else ifeq ($(strip $(CONFIG_MTK_PQ_COLOR_MODE)),"DISP")
    ccflags-y += -DDISP_COLOR_ON
endif

ifeq ($(strip $(CONFIG_MTK_LCM_PHYSICAL_ROTATION)),"180")
    ccflags-y += -DLCM_PHYSICAL_ROTATION_180
else ifeq ($(strip $(CONFIG_MTK_LCM_PHYSICAL_ROTATION)),"270")
    ccflags-y += -DLCM_PHYSICAL_ROTATION_270
else ifeq ($(strip $(CONFIG_MTK_LCM_PHYSICAL_ROTATION_HW)),y)
    ccflags-y += -DLCM_PHYSICAL_ROTATION_180
endif
