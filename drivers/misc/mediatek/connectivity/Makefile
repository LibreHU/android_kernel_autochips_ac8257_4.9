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

# Connectivity combo driver
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
    subdir-ccflags-y += -I$(srctree)/
    subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
    subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/clkbuf_v1
    subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/clkbuf_v1/$(MTK_PLATFORM)
    subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
    subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat
    subdir-ccflags-y += -I$(srctree)/drivers/mmc/core
    subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci/$(MTK_PLATFORM)
    subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/eccci/

    # Do Nothing, move to standalone repo
    MODULE_NAME := connadp
    obj-y += $(MODULE_NAME).o
    $(MODULE_NAME)-objs += common/connectivity_build_in_adapter.o
    $(MODULE_NAME)-objs += common/wmt_build_in_adapter.o


    # LibreHU: WMT, Wi-Fi, BT, GPS and FM built into the kernel, or as modules named as the stock ones
    # (CONFIG_LIBREHU_CONN_MODULES) (README.librehu)
    ifneq ($(CONFIG_MTK_COMBO_DRV_BUILD_IN)$(CONFIG_LIBREHU_CONN_MODULES),)
        TOP := $(srctree)/drivers/misc/mediatek/connectivity
        WMT_SRC_FOLDER := $(TOP)/common
        TARGET_BUILD_VARIANT ?= user
        export TOP WMT_SRC_FOLDER TARGET_BUILD_VARIANT

        # The AC8257 consys is the MT6761 one (same WMT platform code, same
        # GPS EMI offset as the stock wmt_drv.ko / gps_drv.ko). Wi-Fi gen4m
        # has no mt6761 platform folder: the mt6765 one is used, both are
        # CONNAC 1.0 "soc1_0" (WIFI_RAM_CODE_soc1_0 firmware).
        export MTK_COMBO_CHIP := CONNAC
        export BT_PLATFORM := connac1x
        export CONFIG_MTK_COMBO_WIFI_HIF := axi
        export WLAN_CHIP_ID := 6765
        export MTK_ANDROID_WMT := y
        export MTK_ANDROID_EMI := y
        export ADAPTOR_OPTS := CONNAC
        export WIFI_IP_SET := 1
        export CFG_BUILD_CONNAC2 := false
        export CFG_FM_CHIP_ID := 6761
        export CFG_FM_CHIP := mt6631

        ifeq ($(CONFIG_MTK_COMBO_DRV_BUILD_IN),y)
        export LIBREHU_CONN_OBJ := y
        subdir-ccflags-y += -D MTK_WCN_REMOVE_KERNEL_MODULE
        subdir-ccflags-y += -D MTK_WCN_BUILT_IN_DRIVER
        else
        export LIBREHU_CONN_OBJ := m
        export LIBREHU_CONN_MODULES := y
        endif

        obj-y += common/
        obj-y += wlan/adaptor/
        obj-y += wlan/core/gen4m/
        obj-y += gps/
        obj-y += fmradio/
        obj-y += bt/mt66xx/wmt/
    endif

# Otherwise we were called directly from the command line;
# invoke the kernel build system.
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD  := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
