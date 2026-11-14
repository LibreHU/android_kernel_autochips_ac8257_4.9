obj-$(CONFIG_AC8257_RECOVERY_MEM) += recovery_mem.o
obj-$(CONFIG_AC8257_EARLY_PSTORE_CONSOLE) += early_pstore_console.o
obj-$(CONFIG_AC8257_BRINGUP_PANIC) += bringup_reboot.o
obj-$(CONFIG_AC8257_INPUT_BOOST) += ac8257_input_boost.o
ccflags-$(CONFIG_AC8257_INPUT_BOOST) += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/ac8257/include

# LibreHU 4.19: headers and APIs the 4.9 vendor drivers got implicitly (include/linux/librehu_compat_4_9.h)
subdir-ccflags-y += -include $(srctree)/include/linux/librehu_compat_4_9.h -Wno-macro-redefined
