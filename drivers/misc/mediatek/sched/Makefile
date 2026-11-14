#
# Copyright (C) 2017 MediaTek Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See http://www.gnu.org/licenses/gpl-2.0.html for more details.
#

LINUXINCLUDE += -include $(srctree)/kernel/sched/sched.h

# For CPU topology to user space
obj-y += cputopo.o

# For RT monitor
obj-$(CONFIG_MTK_RT_THROTTLE_MON) += rt_monitor.o

ccflags-y += -DMET_USER_EVENT_SUPPORT

#
# RQ stats for TLP estimation
#
# For user space operation
obj-$(CONFIG_MTK_SCHED_RQAVG_US) += rq_stats.o
# For kernel space operation
obj-$(CONFIG_MTK_SCHED_RQAVG_KS) += sched_avg.o

# sysfs interface for user to get EAS related info
# LibreHU 4.19: eas_plus.c and sched_ctl.c need the MediaTek 4.9 scheduler, not built (see sched_ctl_4_19.c)

# For CPU topology to user space
obj-y += sched_ctl_4_19.o
