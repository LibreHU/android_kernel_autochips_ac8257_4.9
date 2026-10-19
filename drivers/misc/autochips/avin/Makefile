#
# Makefile for the avin V4L2 driver (for Linux Kernel 3.18)
# 
#

ccflags-y += -I$(srctree)/drivers/misc/autochips/libosal/inc/
ccflags-y += -I$(srctree)/drivers/misc/autochips/tvd/inc/
ccflags-y += -I$(srctree)/drivers/misc/autochips/avin/inc/
ccflags-y += -I$(srctree)/drivers/misc/autochips/nr/include/
ccflags-y += -I$(srctree)/drivers/misc/autochips/di/inc/
ccflags-y += -I$(srctree)/drivers/misc/autochips/inc/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
ifneq ($(strip $(CONFIG_ATC_BUILD_GO)), y)
ccflags-y += -D NR_MODULE_EXIST
endif

obj-$(CONFIG_ATC_AVIN) += avin.o
avin-objs = src/avin_mod.o src/avin_hal.o
