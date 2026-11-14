/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LibreHU (4.19 port of the AutoChips AC8257 kernel): headers the MediaTek / AutoChips drivers written for 4.9
 * received through <linux/sched.h>, <linux/device.h> and friends before the header split of 4.11 and the
 * clean-ups of 4.12 - 4.19. Force-included (-include) for the vendor directories only, by their Makefiles.
 */
#ifndef _LINUX_LIBREHU_COMPAT_4_9_H
#define _LINUX_LIBREHU_COMPAT_4_9_H

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/sched/task_stack.h>
#include <linux/sched/types.h>
#include <linux/sched/rt.h>
#include <linux/sched/mm.h>
#include <linux/sched/cputime.h>
#include <linux/sched/loadavg.h>
#include <linux/sched/stat.h>
#include <linux/sched/debug.h>
#include <linux/sched/topology.h>
#include <linux/cpuhotplug.h>
#include <linux/librehu_cpu_notifier.h>	/* 4.10: register_cpu_notifier(), CPU_* actions */
#include <linux/pinctrl/consumer.h>
#include <linux/of.h>		/* struct of_device_id, of_*() */
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/device.h>	/* <linux/pm_wakeup.h>: wakeup_source_init() and friends */
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/vmstat.h>

/* 4.11: ACCESS_ONCE() is gone, READ_ONCE() / WRITE_ONCE() replace it */
#ifndef ACCESS_ONCE
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#endif

/* 4.11: cputime_t / cputime64_t are gone, CPU times are plain u64 (nanoseconds, or what get_cpu_idle_time() says) */
typedef u64 cputime_t;
typedef u64 cputime64_t;

/* 4.14: DRIVER_ATTR() is gone (DRIVER_ATTR_RW() and friends need fixed function names) */
#ifndef DRIVER_ATTR
#define DRIVER_ATTR(_name, _mode, _show, _store) \
	struct driver_attribute driver_attr_##_name = __ATTR(_name, _mode, _show, _store)
#endif

/* MediaTek kernel/time/sched_clock.c: sched_clock() with the counter value it used (declared in sched.h in 4.9) */
extern unsigned long long notrace sched_clock_get_cyc(unsigned long long *cyc_ret);

/* MediaTek 4.9 printk.c ("printk too much" detector, no-ops as in the 4.9 user build): kernel/librehu_compat_4_9.c */
void set_detect_count(int count);
int get_detect_count(void);
void set_logtoomuch_enable(int value);
int get_logtoomuch_enable(void);

#endif /* _LINUX_LIBREHU_COMPAT_4_9_H */
