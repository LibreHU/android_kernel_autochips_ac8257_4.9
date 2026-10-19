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
/*
 *Department: ATC-SD-SD5
 *
 *Date: 2018-11-26
 */

#ifndef _NR_LOG_H_
#define _NR_LOG_H_

#ifndef ATC_ARM2_LK_SUPPORT
/* kernel */
#include "linux/kernel.h"
#include "avin_log.h"

#ifdef VSS_LOG_TAG
#undef VSS_LOG_TAG
#define VSS_LOG_TAG "[vss_nr]"
#endif

#define NR_LOG_LVL_ERR                         VSS_LOG_ERR
#define NR_LOG_LVL_WARN                        VSS_LOG_WARNING
#define NR_LOG_LVL_INFO                        VSS_LOG_INFO
#define NR_LOG_LVL_HAL                         VSS_LOG_DEBUG
#define NR_LOG_LVL_DBG                         VSS_LOG_DEBUG
#define NR_LOG_LVL_IRQ                         VSS_LOG_DEBUG

#else
/* arm2 */
#define NR_LOG_LVL_ERR                         0
#define NR_LOG_LVL_WARN                        1
#define NR_LOG_LVL_INFO                        2
#define NR_LOG_LVL_HAL                         3
#define NR_LOG_LVL_DBG                         4
#define NR_LOG_LVL_IRQ                         5
#endif

extern int _u4NR_DBG_LVL;
extern const char *_pcNrLogLevel[];

#ifdef ATC_ARM2_LK_SUPPORT
#include <debug.h>

/* arm2 */
#define NR_LOG(lvl, formatStr, ...)\
		 do{ \
			 if (lvl <= _u4NR_DBG_LVL) {\
				 dprintf(CRITICAL, "[ARM2 NR]"formatStr, ##__VA_ARGS__);\
			 }\
		 } while (0)

#include "arch/ops.h"
#define atomic_set atomic_swap
#define atomic_read(x) (*x)
#define atomic_inc(x) ((*x)++)
#define atomic_dec(x) ((*x)--)

#else

/* kernel */
#if 1 // keep log style follow avin
#define NR_LOG vss_printk
#else
#define NR_LOG(lvl, formatStr, ...)\
	 do { \
		 if (lvl <= _u4NR_DBG_LVL) {\
			 if (lvl == NR_LOG_LVL_ERR) {\
				 pr_err("%s: "formatStr, _pcNrLogLevel[lvl], ##__VA_ARGS__); \
			 } \
			 else if (lvl == NR_LOG_LVL_WARN) {\
				 pr_warn("%s: "formatStr, _pcNrLogLevel[lvl], ##__VA_ARGS__); \
			 } \
			 else if (lvl == NR_LOG_LVL_INFO || lvl == NR_LOG_LVL_HAL) {\
				 pr_info("%s: "formatStr, _pcNrLogLevel[lvl], ##__VA_ARGS__); \
			 } \
			 else {\
				 pr_debug("%s: "formatStr, _pcNrLogLevel[lvl], ##__VA_ARGS__); \
			 } \
		 } \
	 } while (0)
#endif

#endif

#endif

