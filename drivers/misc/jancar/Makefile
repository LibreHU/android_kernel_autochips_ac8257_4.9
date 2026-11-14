obj-$(CONFIG_JANCAR_GPIOS) += jancar_gpios.o

# LibreHU 4.19: headers and APIs the 4.9 vendor drivers got implicitly (include/linux/librehu_compat_4_9.h)
subdir-ccflags-y += -include $(srctree)/include/linux/librehu_compat_4_9.h -Wno-macro-redefined
