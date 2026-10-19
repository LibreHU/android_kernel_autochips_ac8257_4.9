obj-$(CONFIG_MTK_VIBRATOR) := vibrator_drv.o

ifeq (y, $(filter y, $(CONFIG_MACH_AC8257)))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/vibrator/ac8257
obj-y += $(subst ",,ac8257)/
else
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/vibrator/$(MTK_PLATFORM)
endif
