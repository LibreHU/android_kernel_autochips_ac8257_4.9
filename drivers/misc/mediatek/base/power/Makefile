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


obj-$(CONFIG_MACH_AC8257) += $(subst ",,$(CONFIG_MTK_PLATFORM))/
obj-$(CONFIG_MACH_AC8257) += spm/
obj-$(CONFIG_MTK_STATIC_POWER) += leakage_table_v2/
obj-$(CONFIG_MTK_CPU_FREQ) += cpufreq_v1/
obj-$(CONFIG_MTK_PTPOD) += eem_v2/
obj-$(CONFIG_MTK_UNIFY_POWER) += upower_v2/
obj-$(CONFIG_MACH_AC8257) += ppm_v3/
obj-$(CONFIG_MACH_AC8257) += dcm_v1/
obj-$(CONFIG_MACH_AC8257) += hps_v3/
obj-$(CONFIG_MACH_AC8257) += cpuidle_v3/
obj-$(CONFIG_MACH_AC8257) += clkbuf_v1/
obj-$(CONFIG_MACH_AC8257) += pbm_v4/
obj-$(CONFIG_MACH_AC8257) += mdpm_v1/
obj-$(CONFIG_MACH_AC8257) += power_gs_v1/
obj-$(CONFIG_MACH_AC8257) += udi_v1/
obj-$(CONFIG_MACH_AC8257) += mcdi/
ifeq ($(CONFIG_MTK_CM_MGR), y)
obj-$(CONFIG_MACH_AC8257) += cm_mgr_v1/
endif
