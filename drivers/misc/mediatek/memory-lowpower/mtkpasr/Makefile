ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)/
ifeq ($(CONFIG_MTK_EMI),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/emi/submodule/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/emi/$(MTK_PLATFORM)/
else
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
endif
obj-y += mtkpasr_drv.o mtkpasr_hw.o
