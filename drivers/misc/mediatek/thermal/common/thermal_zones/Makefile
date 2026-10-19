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
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/power/$(MTK_PLATFORM)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/hal/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/auxadc/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/common/inc/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/base/power/eem_v1/inc/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/imgsensor/inc/







ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "elbrus"))
#obj-$(CONFIG_THERMAL) += mtk_ts_6311buck.o
#obj-$(CONFIG_THERMAL) += mtk_ts_abb.o
#obj-$(CONFIG_THERMAL) += mtk_ts_battery.o
obj-$(CONFIG_THERMAL) += mtk_ts_bts.o
obj-$(CONFIG_THERMAL) += mtk_ts_btsmdpa.o
#obj-$(CONFIG_THERMAL) += mtk_ts_buck.o
obj-$(CONFIG_THERMAL) += mtk_ts_cpu.o
#obj-$(CONFIG_THERMAL) += mtk_ts_pa.o
#obj-$(CONFIG_THERMAL) += mtk_ts_pa_thput.o
#obj-$(CONFIG_THERMAL) += mtk_ts_pmic.o
#obj-$(CONFIG_THERMAL) += ts_da9214.o
#obj-$(CONFIG_THERMAL) += mtk_ts_dram.o

ifneq ($(CONFIG_MTK_COMBO_WIFI),)
    #obj-$(CONFIG_THERMAL) += mtk_ts_wmt.o
endif

obj-$(CONFIG_THERMAL) += mtk_tsAll.o
endif




ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "ac8257"))

obj-$(CONFIG_THERMAL) += mtk_ts_battery.o

obj-$(CONFIG_THERMAL) += mtk_ts_bts.o
obj-$(CONFIG_THERMAL) += mtk_ts_btsmdpa.o

obj-$(CONFIG_THERMAL) += mtk_ts_cpu_noBank.o
#obj-$(CONFIG_THERMAL) += mtk_ts_pa.o
#obj-$(CONFIG_THERMAL) += mtk_ts_pa_thput.o
obj-$(CONFIG_THERMAL) += mtk_ts_pmic.o
#obj-$(CONFIG_THERMAL) += mtk_ts_charger.o
#obj-$(CONFIG_THERMAL) += mtk_ts_charger2.o


ifneq ($(CONFIG_MTK_COMBO_WIFI),)
    obj-$(CONFIG_THERMAL) += mtk_ts_wmt.o
endif

obj-$(CONFIG_THERMAL) += mtk_tsAll.o
#obj-$(CONFIG_THERMAL) += mtk_ts_imgsensor.o
obj-$(CONFIG_THERMAL) += mtk_ts_dctm.o
endif

