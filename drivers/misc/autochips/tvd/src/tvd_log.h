/*************************************************************************
    > File Name: drivers/misc/autochips/tvd_new/src/tvd_log.h
    > Author: atc0176
    > Mail: xinquan.bian@autochips.com 
    > Created Time: Wed 10 Oct 2018 05:11:42 PM CST
 ************************************************************************/

#ifndef __TVD_LOG_H__
#define __TVD_LOG_H__

#ifdef __KERNEL__

#include "avin_log.h"

#ifndef VSS_LOG_TAG
#define VSS_LOG_TAG "[vss_tvd]"
#endif

#define tvd_dbg(...) do {vss_printk(VSS_LOG_DEBUG, ##__VA_ARGS__);} while(0)
#define tvd_info(...) do {vss_printk(VSS_LOG_INFO, ##__VA_ARGS__);} while(0)
#define tvd_warn(...) do {vss_printk(VSS_LOG_WARNING, ##__VA_ARGS__);} while(0)
#define tvd_err(...) do {vss_printk(VSS_LOG_ERR, ##__VA_ARGS__);} while(0)

#else

#if defined(ATC_ARM2_LK_SUPPORT)
#include <stdio.h>
#else // ARM2 
#include "types.h"
#define printf Printf
#endif

#define LOG_TAG "ARM2 TVD"

enum tvd_log_lvl {
	TVD_EMERG = 0,
	TVD_ALERT,
	TVD_CRIT,
	TVD_ERR,
	TVD_WARNING,
	TVD_NOTICE,
	TVD_INFO,
	TVD_DEBUG
};

extern int tvd_log_level;
extern const char *tvd_log_str[];

#define TVD_LOG(lvl, formatStr, args...)\
({ \
	if ((lvl) <= tvd_log_level) {\
		printf("[%s] [%s] " formatStr, LOG_TAG, tvd_log_str[(lvl)], ##args);\
	} \
})

#define tvd_dbg(...) do {TVD_LOG(TVD_DEBUG, ##__VA_ARGS__);} while(0)
#define tvd_info(...) do {TVD_LOG(TVD_INFO, ##__VA_ARGS__);} while(0)
#define tvd_warn(...) do {TVD_LOG(TVD_WARNING, ##__VA_ARGS__);} while(0)
#define tvd_err(...) do {TVD_LOG(TVD_ERR, ##__VA_ARGS__);} while(0)

#endif

#endif
