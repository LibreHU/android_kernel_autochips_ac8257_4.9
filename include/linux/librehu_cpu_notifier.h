/* SPDX-License-Identifier: GPL-2.0 */
/*
 * LibreHU (4.19 port of the AutoChips AC8257 kernel): the CPU hotplug notifiers of 4.9 (register_cpu_notifier(),
 * the CPU_* actions), removed in 4.10, for the MediaTek / AutoChips drivers written for them. Implemented over two
 * dynamic cpuhp states by kernel/librehu_cpu_notifier.c, see there for the mapping.
 *
 * The action values are the ones of 4.9. 4.19 kept CPU_ONLINE, CPU_UP_PREPARE, CPU_DEAD and CPU_POST_DEAD with
 * the same values, but reuses 0x0008 (CPU_DEAD_FROZEN) and 0x000B (CPU_BROKEN) for the states of kernel/smpboot.c:
 * this header redefines them, so it is only for the vendor drivers (through librehu_compat_4_9.h), never for the
 * core code that uses the 4.19 meaning.
 */
#ifndef _LINUX_LIBREHU_CPU_NOTIFIER_H
#define _LINUX_LIBREHU_CPU_NOTIFIER_H

#include <linux/cpu.h>
#include <linux/notifier.h>

#undef CPU_ONLINE
#undef CPU_UP_PREPARE
#undef CPU_DEAD
#undef CPU_DEAD_FROZEN
#undef CPU_POST_DEAD
#undef CPU_BROKEN

#define CPU_ONLINE		0x0002 /* CPU (unsigned)v is up */
#define CPU_UP_PREPARE		0x0003 /* CPU (unsigned)v coming up */
#define CPU_UP_CANCELED		0x0004 /* CPU (unsigned)v NOT coming up */
#define CPU_DOWN_PREPARE	0x0005 /* CPU (unsigned)v going down */
#define CPU_DOWN_FAILED		0x0006 /* CPU (unsigned)v NOT going down */
#define CPU_DEAD		0x0007 /* CPU (unsigned)v dead */
#define CPU_DYING		0x0008 /* CPU (unsigned)v not running any task, not handling interrupts, soon dead */
#define CPU_POST_DEAD		0x0009 /* CPU (unsigned)v dead, cpu_hotplug lock is dropped */
#define CPU_STARTING		0x000A /* CPU (unsigned)v soon running */
#define CPU_DYING_IDLE		0x000B /* CPU (unsigned)v dying, reached idle loop */
#define CPU_BROKEN		0x000C /* CPU (unsigned)v did not die properly */

/* Used for CPU hotplug events occurring while tasks are frozen due to a suspend operation in progress */
#define CPU_TASKS_FROZEN	0x0010

#define CPU_ONLINE_FROZEN	(CPU_ONLINE | CPU_TASKS_FROZEN)
#define CPU_UP_PREPARE_FROZEN	(CPU_UP_PREPARE | CPU_TASKS_FROZEN)
#define CPU_UP_CANCELED_FROZEN	(CPU_UP_CANCELED | CPU_TASKS_FROZEN)
#define CPU_DOWN_PREPARE_FROZEN	(CPU_DOWN_PREPARE | CPU_TASKS_FROZEN)
#define CPU_DOWN_FAILED_FROZEN	(CPU_DOWN_FAILED | CPU_TASKS_FROZEN)
#define CPU_DEAD_FROZEN		(CPU_DEAD | CPU_TASKS_FROZEN)
#define CPU_DYING_FROZEN	(CPU_DYING | CPU_TASKS_FROZEN)
#define CPU_STARTING_FROZEN	(CPU_STARTING | CPU_TASKS_FROZEN)

/* Notifier priorities of 4.9 the drivers use (higher runs first) */
#ifndef CPU_PRI_PERF
#define CPU_PRI_PERF		20
#define CPU_PRI_MIGRATION	10
#define CPU_PRI_SMPBOOT		9
#define CPU_PRI_WORKQUEUE_UP	5
#define CPU_PRI_WORKQUEUE_DOWN	(-5)
#endif

#ifdef CONFIG_SMP
int register_cpu_notifier(struct notifier_block *nb);
int __register_cpu_notifier(struct notifier_block *nb);
void unregister_cpu_notifier(struct notifier_block *nb);
void __unregister_cpu_notifier(struct notifier_block *nb);
#else
static inline int register_cpu_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int __register_cpu_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline void unregister_cpu_notifier(struct notifier_block *nb)
{
}

static inline void __unregister_cpu_notifier(struct notifier_block *nb)
{
}
#endif

#define register_hotcpu_notifier(nb)		register_cpu_notifier(nb)
#define __register_hotcpu_notifier(nb)		__register_cpu_notifier(nb)
#define unregister_hotcpu_notifier(nb)		unregister_cpu_notifier(nb)
#define __unregister_hotcpu_notifier(nb)	__unregister_cpu_notifier(nb)

#define cpu_notifier(fn, pri) {					\
	static struct notifier_block fn##_nb =			\
		{ .notifier_call = fn, .priority = pri };	\
	register_cpu_notifier(&fn##_nb);			\
}

#define __cpu_notifier(fn, pri) {				\
	static struct notifier_block fn##_nb =			\
		{ .notifier_call = fn, .priority = pri };	\
	__register_cpu_notifier(&fn##_nb);			\
}

#define hotcpu_notifier(fn, pri)	cpu_notifier(fn, pri)
#define __hotcpu_notifier(fn, pri)	__cpu_notifier(fn, pri)

#endif /* _LINUX_LIBREHU_CPU_NOTIFIER_H */
