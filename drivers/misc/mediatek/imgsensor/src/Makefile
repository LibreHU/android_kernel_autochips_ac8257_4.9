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

subdir-ccflags-y += -I$(srctree)/drivers/i2c/busses/ \
             -I$(srctree)/drivers/misc/mediatek/ccu/inc

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))

MTK_PLATFORM_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)
MTK_PROJECT_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)

ifneq ($(findstring k61v1_demo_64_bsp, $(ARCH_MTK_PROJECT)),)
subdir-ccflags-y += -D K61V1_DEMO_64_BSP
endif

ifneq ($(findstring ac8257_demo_1g_32, $(ARCH_MTK_PROJECT)),)
subdir-ccflags-y += -D AC8257_DEMO_1G_32
endif

CONFIG_MTK_PLATFORM_SENSOR := $(CONFIG_MTK_PLATFORM)

subdir-ccflags-y += -I$(MTK_PROJECT_PATH_ROOT)/camera/camera

MTK_DRIVER_PATH  := $(srctree)/drivers/misc/mediatek

subdir-ccflags-y += -I$(MTK_DRIVER_PATH)/camera/$(MTK_PLATFORM) \
             -I$(MTK_DRIVER_PATH)/imgsensor/inc \
             -I$(MTK_DRIVER_PATH)/imgsensor/src/$(CONFIG_MTK_PLATFORM_SENSOR)/camera_project/$(ARCH_MTK_PROJECT)/camera_hw \
             -I$(MTK_DRIVER_PATH)/imgsensor/src/$(CONFIG_MTK_PLATFORM_SENSOR)/camera_hw	\
             -I$(MTK_DRIVER_PATH)/cam_cal/inc \
             -I$(MTK_DRIVER_PATH)/smi \
             -I$(MTK_DRIVER_PATH)/include/mt-plat/$(MTK_PLATFORM)/include/mach \
             -I$(MTK_DRIVER_PATH)/imgsensor/src/$(CONFIG_MTK_PLATFORM_SENSOR)


CUSTOM_KERNEL_FLAGS := $(addprefix -D, $(shell echo \
             $(CONFIG_CUSTOM_KERNEL_IMGSENSOR) | tr a-z A-Z))
subdir-ccflags-y += $(CUSTOM_KERNEL_FLAGS)

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM_SENSOR))/

