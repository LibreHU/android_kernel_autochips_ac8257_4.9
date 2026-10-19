ifeq (y, $(filter y, $(CONFIG_MACH_AC8257)))
MTK_PLATFORM:= $(subst ",,ac8257)
else
MTK_PLATFORM:= $(subst ",,$(CONFIG_MTK_PLATFORM))
endif

ccflags-y += -I$(srctree)/drivers/misc/mediatek/auxadc/$(MTK_PLATFORM)

obj-$(CONFIG_MTK_AUXADC) += mtk_auxadc.o

