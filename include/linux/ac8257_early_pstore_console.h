#ifndef _LINUX_AC8257_EARLY_PSTORE_CONSOLE_H
#define _LINUX_AC8257_EARLY_PSTORE_CONSOLE_H

#include <linux/printk.h>

#ifdef CONFIG_AC8257_EARLY_PSTORE_CONSOLE
#define ac8257_boot_mark(fmt, ...) pr_info("ac8257: " fmt "\n", ##__VA_ARGS__)
void ac8257_early_pstore_console_init(void);
void ac8257_early_pstore_console_remap(void);
void ac8257_early_pstore_console_stop(void);
#else
#define ac8257_boot_mark(fmt, ...) do { } while (0)
static inline void ac8257_early_pstore_console_init(void) { }
static inline void ac8257_early_pstore_console_remap(void) { }
static inline void ac8257_early_pstore_console_stop(void) { }
#endif

#endif
