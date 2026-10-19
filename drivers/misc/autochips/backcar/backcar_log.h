#ifndef BACKCAR_LOG_H
#define BACKCAR_LOG_H

#include <linux/printk.h>

#define LOG_TAG

enum bakcar_log_lvl {
    BACKCAR_EMERG = 0,
    BACKCAR_ALERT,
    BACKCAR_CRIT,
    BACKCAR_ERR,
    BACKCAR_WARNING,
    BACKCAR_NOTICE,
    BACKCAR_INFO,
    BACKCAR_DEBUG
};

extern u8 *backcar_log_level_str[];

#define BACKCAR_LOG(lvl, formatStr, args...)\
({ \
    switch ((lvl)) {\
    case BACKCAR_ERR:\
        pr_err(LOG_TAG "[%s] %s(line:%d) "formatStr, backcar_log_level_str[(lvl)], __func__, __LINE__,  ##args);\
        break;\
    case BACKCAR_WARNING:\
    case BACKCAR_INFO:\
        pr_info(LOG_TAG "[%s] %s(line:%d) "formatStr, backcar_log_level_str[(lvl)], __func__, __LINE__, ##args);\
        break;\
    case BACKCAR_DEBUG:\
        pr_debug(LOG_TAG "[%s] %s(line:%d) "formatStr, backcar_log_level_str[(lvl)], __func__, __LINE__, ##args);\
        break;\
    default:\
        break;\
    } \
})

#endif
