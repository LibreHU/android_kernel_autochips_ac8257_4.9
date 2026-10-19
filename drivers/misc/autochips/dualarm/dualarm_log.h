/*
 *Department: ATC-SD-SD5
 *
 *Aurthor: Yuanwu He(atc0066)
 *
 *Date: 2017-08-12
 */

#ifndef _DUALARM_LOG_H_
#define _DUALARM_LOG_H_

#include "linux/kernel.h"

extern u32 g_dualarm_dbg_lvl;
extern u8 *g_dualarm_log_level[];

#define DUALARM_LOG_LVL_ERR                         0
#define DUALARM_LOG_LVL_WARN                        1
#define DUALARM_LOG_LVL_INFO                        2
#define DUALARM_LOG_LVL_HAL                         3
#define DUALARM_LOG_LVL_DBG                         4
#define DUALARM_LOG_LVL_IRQ                         5


#define DUALARM_LOG(lvl, formatStr, ...)\
do { \
	if (lvl <= g_dualarm_dbg_lvl) {\
		if (lvl == DUALARM_LOG_LVL_ERR) {\
			pr_err("%s: %s, %d "formatStr, g_dualarm_log_level[lvl], __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		} \
		else if (lvl == DUALARM_LOG_LVL_WARN) {\
			pr_warn(" %s: %s, %d "formatStr, g_dualarm_log_level[lvl], __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		} \
		else if (lvl == DUALARM_LOG_LVL_INFO || \
			lvl == DUALARM_LOG_LVL_HAL || \
			lvl == DUALARM_LOG_LVL_DBG ) {\
			pr_info(" %s: "formatStr, g_dualarm_log_level[lvl], ##__VA_ARGS__); \
		} \
		else {\
			pr_debug(" %s:  "formatStr, g_dualarm_log_level[lvl], ##__VA_ARGS__); \
		} \
	} \
} while (0)


#endif

