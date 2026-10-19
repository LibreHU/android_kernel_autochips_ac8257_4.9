obj-y += dfd.o

ifeq ($(CONFIG_MACH_AC8257),y)
obj-y += ac8257/
else
obj-$(CONFIG_MTK_DFD_INTERNAL_DUMP) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif
