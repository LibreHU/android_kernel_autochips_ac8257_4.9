subdir-ccflags-y += 

# Core
obj-y				+= pinctrl-mtk-common.o

# SoC Drivers
obj-$(CONFIG_PINCTRL_AC8257)	+= pinctrl-ac8257.o

# LibreHU 4.19: headers and APIs the 4.9 vendor drivers got implicitly (include/linux/librehu_compat_4_9.h)
subdir-ccflags-y += -include $(srctree)/include/linux/librehu_compat_4_9.h -Wno-macro-redefined
