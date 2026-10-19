/*
* Copyright (c) 2016 AutoChips Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef _DEBUG_LOG_H

#define _DEBUG_LOG_H

#ifndef ATC_ARM2_LK_SUPPORT
#include <linux/printk.h>
#include "x_os.h"
#include "avin_log.h"
#else
#include "platform/mt_typedefs.h"
#include "sys/types.h"
#include "di_types.h"

#include "debug.h"
#endif

extern const char *_pcVideoLogLevel[];
#ifndef ATC_ARM2_LK_SUPPORT
#define VDO_LOG_LVL_OFF                         0
#define VDO_LOG_LVL_ERR                         VSS_LOG_ERR
#define VDO_LOG_LVL_WARN                        VSS_LOG_WARNING
#define VDO_LOG_LVL_INFO                        VSS_LOG_INFO
#define VDO_LOG_LVL_TRACE                       VSS_LOG_DEBUG
#define VDO_LOG_LVL_DBG                         VSS_LOG_DEBUG
#define VDO_LOG_LVL_IRQ                         VSS_LOG_DEBUG
#define VDO_LOG_LVL_REGRW                       VSS_LOG_DEBUG

#define LOG_CTRL_AVIN 1
#else
#define VDO_LOG_LVL_OFF                         0
#define VDO_LOG_LVL_ERR                         1
#define VDO_LOG_LVL_WARN                        2
#define VDO_LOG_LVL_INFO                        3
#define VDO_LOG_LVL_TRACE                       4
#define VDO_LOG_LVL_DBG                         5
#define VDO_LOG_LVL_IRQ                         6
#define VDO_LOG_LVL_REGRW                       7
#endif //end _ARM2_

#ifdef ATC_ARM2_LK_SUPPORT
#define atomic_set atomic_swap

#define mutex_lock mutex_acquire
#define mutex_unlock mutex_release
#define atomic_read(x) (*x)
#define VDO_LOG(lvl, format, ...) \
{ \
	if (lvl <= VDO_LOG_LVL_INFO) { \
		dprintf(0, "[ARM2_DI]: "format, ##__VA_ARGS__); \
	} \
}
#else

#if LOG_CTRL_AVIN

#ifdef VSS_LOG_TAG
#undef VSS_LOG_TAG
#define VSS_LOG_TAG "[vss_di]"
#endif

#define VDO_LOG vss_printk
#else
#define VDO_LOG(lvl, format, ...) \
{ \
	if (lvl <= VDO_LOG_LVL_INFO) { \
		if (lvl == VDO_LOG_LVL_ERR) { \
			pr_err("%s:[%s %d] "format, _pcVideoLogLevel[lvl], __func__, __LINE__, ##__VA_ARGS__); \
		} else if (lvl == VDO_LOG_LVL_WARN) { \
			pr_warn("%s:[%s %d]  "format, _pcVideoLogLevel[lvl], __func__, __LINE__, ##__VA_ARGS__); \
		} else if (lvl == VDO_LOG_LVL_INFO) { \
			pr_info("%s:[%s %d] "format, _pcVideoLogLevel[lvl], __func__, __LINE__, ##__VA_ARGS__); \
		} else if (lvl <= VDO_LOG_LVL_DBG) { \
			pr_info("%s: "format, _pcVideoLogLevel[lvl], ##__VA_ARGS__); \
		} else { \
			pr_info("%s: "format, _pcVideoLogLevel[lvl], ##__VA_ARGS__); \
		} \
	} \
}
#endif

#endif
#define ENTER()	\
	do {	\
		VDO_LOG(VDO_LOG_LVL_DBG, "%s enter\n", __func__); \
	} while (0)
#define LEAVE()	\
	do {	\
		VDO_LOG(VDO_LOG_LVL_DBG, "%s leave\n", __func__); \
	} while (0)

#endif

