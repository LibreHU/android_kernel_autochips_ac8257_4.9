#
# Makefile for the tvd driver (for Linux Kernel 3.18)
# 
#

ccflags-y += -I$(srctree)/drivers/misc/autochips/wch/include
ccflags-y += -I$(srctree)/drivers/misc/autochips/avin/inc
ccflags-y += -I$(srctree)/drivers/misc/autochips/inc

ccflags-y += -I$(src)/inc

obj-$(CONFIG_ATC_TVD) += tvd.o
tvd-objs := src/tvd_hal.o src/tvd_if.o src/tvd_mod.o src/tvd_log.o
