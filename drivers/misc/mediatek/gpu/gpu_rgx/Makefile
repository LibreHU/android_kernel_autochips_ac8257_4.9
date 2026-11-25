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

MTK_GPU_VERSION = $(subst ",,$(CONFIG_MTK_GPU_VERSION))
$(info *MTK_GPU_VERSION 2 = $(word 2,$(MTK_GPU_VERSION)))

# LibreHU: the third word picks the DDK when it is one of the trees below ("rgx clark m1.11ED5425693": DDK 1.11 of the
# Vulkan branch); otherwise the second word, as MediaTek did ("rgx clark m1.9ED4971894" builds m1.10ED5130912).
ifeq ($(word 3,$(MTK_GPU_VERSION)),m1.11ED5425693)
	obj-y += m1.11ED5425693/
else ifeq ($(word 2,$(MTK_GPU_VERSION)),doma)
	obj-y += m1.10ED5130912/
else ifeq ($(word 2,$(MTK_GPU_VERSION)),lorne)
	obj-y += m1.10ED5150755/
else ifeq ($(word 2,$(MTK_GPU_VERSION)),clark)
	obj-y += m1.10ED5130912/
endif

