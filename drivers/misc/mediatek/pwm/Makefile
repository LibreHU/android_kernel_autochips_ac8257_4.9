#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
ifeq (y, $(filter y, $(CONFIG_MACH_AC8257)))
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pwm/ac8257/include
obj-y += $(subst ",,ac8257)/
else
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pwm/$(CONFIG_MTK_PLATFORM)/include
obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
endif

obj-$(CONFIG_MTK_PWM)	+= mtk_pwm.o
