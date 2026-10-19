#
# Copyright (C) 2017 MediaTek Inc.
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

THERMAL_CHIP_DRIVER_DIR := $(srctree)/drivers/misc/mediatek/thermal/$(MTK_PLATFORM)
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/$(MTK_PLATFORM)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/video/include
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/ged/include
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/common/inc/
ccflags-y  += -I$(srctree)/drivers/power/mediatek/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/eem_v1/inc/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/dfrc/
ifeq ($(CONFIG_MTK_TINYSYS_SSPM_SUPPORT),y)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/sspm/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/sspm/$(MTK_PLATFORM)
endif


obj-$(CONFIG_THERMAL) += mtk_ta.o
ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "ac8257"))
ifeq ($(CONFIG_MTK_THERMAL_PA_VIA_ATCMD),y)
    obj-$(CONFIG_THERMAL) += mtk_cooler_mutt.o
endif
obj-$(CONFIG_THERMAL) += mtk_cooler_fps.o
ifneq ($(ATC_DISABLE_BATTERY_CHARGER),yes)
ifeq ($(CONFIG_MTK_PUMP_EXPRESS_PLUS_30_SUPPORT),y)
	obj-$(CONFIG_THERMAL) += mtk_cooler_bcct_v1.o
else
	ifeq ($(CONFIG_MTK_GAUGE_VERSION),30)
		obj-$(CONFIG_THERMAL) += mtk_cooler_bcct_v1.o
		ifeq ($(CONFIG_MTK_DUAL_CHARGER_SUPPORT),y)
			obj-$(CONFIG_THERMAL) += mtk_cooler_bcct_2nd.o
		endif
	else
		obj-$(CONFIG_THERMAL) += mtk_cooler_bcct.o
	endif
endif
endif
obj-$(CONFIG_THERMAL) += mtk_cooler_atm.o
obj-$(CONFIG_THERMAL) += mtk_cooler_dtm.o
obj-$(CONFIG_THERMAL) += mtk_cooler_sysrst.o
obj-$(CONFIG_THERMAL) += mtk_cooler_NR.o
obj-$(CONFIG_THERMAL) += mtk_cooler_VR_FPS.o
endif

