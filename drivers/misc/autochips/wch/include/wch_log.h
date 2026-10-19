/*
 *Department: ATC-SD-SD2
 *
 *Aurthor: Mingxu Wang(atc0027)
 *
 *Date: 2016-12-05
 */

#ifndef _WCH_LOG_H_
#define _WCH_LOG_H_

#ifndef ATC_ARM2_LK_SUPPORT
#include "linux/kernel.h"
#include "wch_priv.h"
#include "avin_log.h"

extern u32 g_wch_dbg_lvl;
extern const char *g_wch_log_level[];
extern const char *g_wch_name[WCH_NUM];

#ifdef VSS_LOG_TAG
#undef VSS_LOG_TAG
#define VSS_LOG_TAG "[vss_wch]"
#endif

#define WCH_LOG_LVL_ERR                         VSS_LOG_ERR
#define WCH_LOG_LVL_WARN                        VSS_LOG_WARNING
#define WCH_LOG_LVL_INFO                        VSS_LOG_INFO
#define WCH_LOG_LVL_HAL                         VSS_LOG_DEBUG
#define WCH_LOG_LVL_DBG                         VSS_LOG_DEBUG
#define WCH_LOG_LVL_IRQ                         VSS_LOG_DEBUG
#define WCH_LOG	                                vss_printk

#define WCH_LOG_INFO(wchId, formatStr, args...) \
	do{ \
		if (wchId == WCH_VDI) { \
			vss_printk(WCH_LOG_LVL_INFO, formatStr, ##args); \
        } else { \
			vss_printk(WCH_LOG_LVL_DBG,  formatStr, ##args); \
		} \
	} while (0)
#else

#define WCH_LOG_LVL_ERR                         0
#define WCH_LOG_LVL_WARN                        1
#define WCH_LOG_LVL_INFO                        2
#define WCH_LOG_LVL_HAL                         3
#define WCH_LOG_LVL_DBG                         4
#define WCH_LOG_LVL_IRQ                         5

#define vss_aee_print
#include <stdio.h>
#define WCH_LOG(lvl, formatStr, ...) \
    do{ \
        if (lvl <= WCH_LOG_LVL_HAL) { \
            printf("[ARM2 WCH]  "formatStr,  ##__VA_ARGS__); \
        } \
    } while (0)

#define WCH_LOG_INFO(wchId, formatStr, args...) \
	WCH_LOG(WCH_LOG_LVL_INFO, formatStr, ##args);

#include "arch/ops.h"
#define atomic_set atomic_swap
#define atomic_read(x) (*x)
#endif

#endif
