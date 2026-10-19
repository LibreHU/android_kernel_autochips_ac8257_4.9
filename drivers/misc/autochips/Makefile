#
# Makefile - Autochips Linux driver.
# Copyright(c) 2015, Autochips Corporation.
#
ATC_OS_NAME := $(subst ",,$(CONFIG_ATC_OS))
ATC_PLATFORM_NAME := $(subst ",,$(CONFIG_ATC_PLATFORM))

#obj-$(CONFIG_ATC_LIB_OSAL) += libosal/
obj-$(CONFIG_ATC_AVIN) += avin/
obj-$(CONFIG_ATC_TVD) += tvd/
obj-y += dualarm/
obj-y += arm2_log/
obj-y += n4_video_decoder/
obj-$(CONFIG_ATC_WCH) += wch/
obj-$(CONFIG_ATC_NR) += nr/
obj-$(CONFIG_ATC_DI) += di/
obj-$(CONFIG_ATC_BACKCAR) += backcar/
obj-$(CONFIG_ATC_BOOT_STATE) += boot_state/
obj-$(CONFIG_ATC_METAZONE) += metazone/
obj-$(CONFIG_ATC_ARM2_LOG_BUFFER) += arm2_log_buffer/
obj-y += rsv_cpu/
