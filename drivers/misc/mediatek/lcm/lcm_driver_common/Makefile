#
# Copyright (c) 2019 AutoChips Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#

#
# Makefile for misc devices that really don't fit anywhere else.
#
ccflags-y += -D$(shell echo $(CONFIG_ARCH_MTK_PROJECT) | tr a-z A-Z)

obj-y += lcm_driver_common.o

ccflags-$(CONFIG_MTK_LCM) += -I$(srctree)/drivers/misc/mediatek/lcm/inc

