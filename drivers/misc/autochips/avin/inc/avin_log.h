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

#ifndef AVIN_LOG_H
#define AVIN_LOG_H

#include <linux/printk.h>
#include <linux/types.h>
#include <mt-plat/aee.h>

/**
* the log level of video in subsystem
*/
enum vss_log_lvl {
	VSS_LOG_ERR,
	VSS_LOG_WARNING,
	VSS_LOG_INFO,
	VSS_LOG_DEBUG,
	VSS_LOG_MAX,
};

#define VSS_LOG_TAG
extern const char *vss_log_str[];
extern int log_debug;
extern int vss_aee_debug;

#define vss_aee_print(string, args...) do {\
	if (vss_aee_debug) {\
		char avin_name[100]; \
		snprintf(avin_name, 100, "[avin_hal]"string, ##args); \
		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_MMPROFILE_BUFFER, avin_name, "[avin_hal] error:"string, ##args);  \
	} \
} while (0)


#define vss_printk(level, fmt, args...)\
	do {\
		if (log_debug >= level)\
			pr_info(VSS_LOG_TAG " %s[%s] " fmt,  __func__, vss_log_str[(level)], ##args);\
	} while (0)

#define IRQ_LOG_KEEPER(_irq, _log_lvl, fmt, ...) do {\
	char *ptr;\
	char *des;\
	bool over_flow = false;\
	u32 *str_cnt;\
	u32 str_len;\
	enum vss_irq irq;\
	enum vss_log_lvl log_lvl;\
	if (_irq < IRQ_MAX || (_log_lvl < VSS_LOG_MAX)) {\
		irq = _irq;\
		log_lvl = _log_lvl;\
		str_cnt = &g_log_info[irq][log_lvl]._cnt;\
		switch(log_lvl) {\
			case VSS_LOG_ERR:\
				str_len = NORMAL_STR_LEN*ERR_PAGE;\
				break;\
			case VSS_LOG_WARNING:\
				str_len = NORMAL_STR_LEN*WARNING_PAGE;\
				break;\
			case VSS_LOG_INFO:\
				str_len = NORMAL_STR_LEN*INFO_PAGE;\
				break;\
			case VSS_LOG_DEBUG:\
				str_len = NORMAL_STR_LEN*DEBUG_PAGE;\
				break;\
			default:\
				str_len = 0;\
				break;\
		}\
		if (str_len) { \
			ptr = des = (char *)(g_log_info[irq][log_lvl]._str + *str_cnt);\
			sprintf((char *)(des), fmt, ##__VA_ARGS__);\
			if ('\0' != *(g_log_info[irq][log_lvl]._str + str_len - 1)) {\
				vss_printk(VSS_LOG_ERR, "log str over flow(%d)", irq);\
				over_flow = true;\
			} \
			if (over_flow) { \
				memset(g_log_info[irq][log_lvl]._str, 0x00, (str_len + CACHE_PAGE*NORMAL_STR_LEN)); \
				*str_cnt = 0; \
			} else {\
				while (*ptr++ != '\0') { \
					(*str_cnt)++;\
				}\
			} \
		} else { \
			vss_printk(VSS_LOG_WARNING, "the log type is error, so do nothing\n"); \
		} \
	} else {\
		vss_printk(VSS_LOG_ERR, "the input parameter is invalid\n");\
	}\
} while (0)

#define IRQ_LOG_PRINTER(_irq, _log_lvl) do {\
	enum vss_irq irq;\
	enum vss_log_lvl log_lvl;\
	u32 str_len;\
	if (_irq < IRQ_MAX || (_log_lvl < VSS_LOG_MAX)) {\
		irq = _irq;\
		log_lvl = _log_lvl;\
		switch(log_lvl) {\
			case VSS_LOG_ERR:\
				str_len = NORMAL_STR_LEN*ERR_PAGE;\
				break;\
			case VSS_LOG_WARNING:\
				str_len = NORMAL_STR_LEN*WARNING_PAGE;\
				break;\
			case VSS_LOG_INFO:\
				str_len = NORMAL_STR_LEN*INFO_PAGE;\
				break;\
			case VSS_LOG_DEBUG:\
				str_len = NORMAL_STR_LEN*DEBUG_PAGE;\
				break;\
			default:\
				str_len = 0;\
				break;\
		}\
		if (0 != g_log_info[irq][log_lvl]._cnt) {\
			if ('\0' != *(g_log_info[irq][log_lvl]._str + str_len - 1)) {\
				*(g_log_info[irq][log_lvl]._str + str_len - 1) = '\0';\
			}\
			vss_printk(VSS_LOG_INFO, "%s", g_log_info[irq][log_lvl]._str);\
		}\
	} else {\
		vss_printk(VSS_LOG_ERR, "the input parameter is invalid\n");\
	}\
} while (0);
#endif /*_AVIN_LOG_H_*/
