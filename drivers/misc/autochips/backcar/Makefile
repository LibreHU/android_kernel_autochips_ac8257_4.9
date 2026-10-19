#
# Makefile for AC8257P BackCar communication Driver
#

obj-y += backcar.o

ifeq ($(CONFIG_ARCH_MTK_PROJECT), "ac8257_jac")
ccflags-y += -DBUILD_JAC_BSP
endif

ccflags-y += -I$(srctree)/drivers/misc/autochips/inc/
ccflags-y += -I$(srctree)/drivers/misc/autochips/dualarm/
ccflags-y += -I$(srctree)/drivers/misc/autochips/rsv_cpu/

