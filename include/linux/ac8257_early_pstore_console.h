#ifndef _LINUX_AC8257_EARLY_PSTORE_CONSOLE_H
#define _LINUX_AC8257_EARLY_PSTORE_CONSOLE_H

#ifdef CONFIG_AC8257_EARLY_PSTORE_CONSOLE
void ac8257_early_pstore_console_init(void);
void ac8257_early_pstore_console_stop(void);
#else
static inline void ac8257_early_pstore_console_init(void) { }
static inline void ac8257_early_pstore_console_stop(void) { }
#endif

#endif
