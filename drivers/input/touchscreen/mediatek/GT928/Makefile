# include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifeq ($(CONFIG_ARCH_MTK_PROJECT), "ac8257_jac")
ccflags-y += -DBUILD_JAC_BSP
endif
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/GT928/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT928/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/

obj-y	+=  goodix_tool.o
obj-y	+=  gt9xx_driver.o
obj-y	+=  gt9xx_update.o
