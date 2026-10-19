#
# Copyright (C) 2016 MediaTek Inc.
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

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

#subdir-ccflags-y += -DMTK_VPU_EMULATOR

MTK_VPU_CORE := 1

subdir-ccflags-y += -DMTK_VPU_CORE=$(MTK_VPU_CORE)

subdir-ccflags-y += -DMTK_VPU_DVT


# FPGA environment
ifeq ($(CONFIG_MTK_FPGA),y)
subdir-ccflags-y += -DMTK_VPU_FPGA_PORTING
else ifeq ($(CONFIG_FPGA_EARLY_PORTING),y)
subdir-ccflags-y += -DMTK_VPU_FPGA_PORTING
endif

# MDLA config
ifeq ($(CONFIG_MTK_MDLA_SUPPORT),y)
subdir-ccflags-y += -DMTK_MDLA_SUPPORT
endif

