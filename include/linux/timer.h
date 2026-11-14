/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TIMER_H
#define _LINUX_TIMER_H

#include <linux/list.h>
#include <linux/ktime.h>
#include <linux/stddef.h>
#include <linux/debugobjects.h>
#include <linux/stringify.h>
#include <linux/android_kabi.h>

struct timer_list {
	/*
	 * All fields that change during normal runtime grouped to the
	 * same cacheline
	 */
	struct hlist_node	entry;
	unsigned long		expires;
	void			(*function)(struct timer_list *);
	u32			flags;

#ifdef CONFIG_LOCKDEP
	struct lockdep_map	lockdep_map;
#endif
#ifdef CONFIG_MEDIATEK_SOLUTION
	/* LibreHU: 4.9-style timers of the MediaTek / AutoChips drivers (setup_timer(), init_timer(), .data) */
	void			(*legacy_function)(unsigned long);
	unsigned long		data;
#endif

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
};

#ifdef CONFIG_LOCKDEP
/*
 * NB: because we have to copy the lockdep_map, setting the lockdep_map key
 * (second argument) here is required, otherwise it could be initialised to
 * the copy of the lockdep_map later! We use the pointer to and the string
 * "<file>:<line>" as the key resp. the name of the lockdep_map.
 */
#define __TIMER_LOCKDEP_MAP_INITIALIZER(_kn)				\
	.lockdep_map = STATIC_LOCKDEP_MAP_INIT(_kn, &_kn),
#else
#define __TIMER_LOCKDEP_MAP_INITIALIZER(_kn)
#endif

/*
 * A deferrable timer will work normally when the system is busy, but
 * will not cause a CPU to come out of idle just to service it; instead,
 * the timer will be serviced when the CPU eventually wakes up with a
 * subsequent non-deferrable timer.
 *
 * An irqsafe timer is executed with IRQ disabled and it's safe to wait for
 * the completion of the running instance from IRQ handlers, for example,
 * by calling del_timer_sync().
 *
 * Note: The irq disabled callback execution is a special case for
 * workqueue locking issues. It's not meant for executing random crap
 * with interrupts disabled. Abuse is monitored!
 */
#define TIMER_CPUMASK		0x0003FFFF
#define TIMER_MIGRATING		0x00040000
#define TIMER_BASEMASK		(TIMER_CPUMASK | TIMER_MIGRATING)
#define TIMER_DEFERRABLE	0x00080000
#define TIMER_PINNED		0x00100000
#define TIMER_IRQSAFE		0x00200000
#define TIMER_ARRAYSHIFT	22
#define TIMER_ARRAYMASK		0xFFC00000

#define TIMER_TRACE_FLAGMASK	(TIMER_MIGRATING | TIMER_DEFERRABLE | TIMER_PINNED | TIMER_IRQSAFE)

#define __TIMER_INITIALIZER(_function, _flags) {		\
		.entry = { .next = TIMER_ENTRY_STATIC },	\
		.function = (_function),			\
		.flags = (_flags),				\
		__TIMER_LOCKDEP_MAP_INITIALIZER(		\
			__FILE__ ":" __stringify(__LINE__))	\
	}

#define DEFINE_TIMER(_name, _function)				\
	struct timer_list _name =				\
		__TIMER_INITIALIZER(_function, 0)

/*
 * LOCKDEP and DEBUG timer interfaces.
 */
void init_timer_key(struct timer_list *timer,
		    void (*func)(struct timer_list *), unsigned int flags,
		    const char *name, struct lock_class_key *key);

#ifdef CONFIG_DEBUG_OBJECTS_TIMERS
extern void init_timer_on_stack_key(struct timer_list *timer,
				    void (*func)(struct timer_list *),
				    unsigned int flags, const char *name,
				    struct lock_class_key *key);
#else
static inline void init_timer_on_stack_key(struct timer_list *timer,
					   void (*func)(struct timer_list *),
					   unsigned int flags,
					   const char *name,
					   struct lock_class_key *key)
{
	init_timer_key(timer, func, flags, name, key);
}
#endif

#ifdef CONFIG_LOCKDEP
#define __init_timer(_timer, _fn, _flags)				\
	do {								\
		static struct lock_class_key __key;			\
		init_timer_key((_timer), (_fn), (_flags), #_timer, &__key);\
	} while (0)

#define __init_timer_on_stack(_timer, _fn, _flags)			\
	do {								\
		static struct lock_class_key __key;			\
		init_timer_on_stack_key((_timer), (_fn), (_flags),	\
					#_timer, &__key);		 \
	} while (0)
#else
#define __init_timer(_timer, _fn, _flags)				\
	init_timer_key((_timer), (_fn), (_flags), NULL, NULL)
#define __init_timer_on_stack(_timer, _fn, _flags)			\
	init_timer_on_stack_key((_timer), (_fn), (_flags), NULL, NULL)
#endif

/**
 * timer_setup - prepare a timer for first use
 * @timer: the timer in question
 * @callback: the function to call when timer expires
 * @flags: any TIMER_* flags
 *
 * Regular timer initialization should use either DEFINE_TIMER() above,
 * or timer_setup(). For timers on the stack, timer_setup_on_stack() must
 * be used and must be balanced with a call to destroy_timer_on_stack().
 */
#define timer_setup(timer, callback, flags)			\
	__init_timer((timer), (callback), (flags))

#define timer_setup_on_stack(timer, callback, flags)		\
	__init_timer_on_stack((timer), (callback), (flags))

#ifdef CONFIG_MEDIATEK_SOLUTION
/*
 * LibreHU (4.19 port of the AC8257 kernel): the timer API of 4.14 and before, removed in 4.15, for the MediaTek /
 * AutoChips drivers written for 4.9. The 4.9 callback, which takes "unsigned long data", is called by a trampoline
 * with the data given to setup_timer() (or stored in .data and .legacy_function around init_timer()).
 */
static inline void legacy_timer_fn(struct timer_list *t)
{
	if (t->legacy_function)
		t->legacy_function(t->data);
}

/* The 4.9 DEFINE_TIMER(name, function, expires, data) */
#define DEFINE_LEGACY_TIMER(_name, _function, _expires, _data)		\
	struct timer_list _name = {					\
		.entry = { .next = TIMER_ENTRY_STATIC },		\
		.function = legacy_timer_fn,				\
		.expires = (_expires),					\
		.flags = 0,						\
		.legacy_function = (_function),				\
		.data = (_data),					\
		__TIMER_LOCKDEP_MAP_INITIALIZER(			\
			__FILE__ ":" __stringify(__LINE__))		\
	}

#define __legacy_setup_timer(_timer, _fn, _data, _flags, _setup)		\
	do {								\
		_setup((_timer), legacy_timer_fn, (_flags));		\
		(_timer)->legacy_function = (_fn);			\
		(_timer)->data = (_data);				\
	} while (0)
#define setup_timer(timer, fn, data)					\
	__legacy_setup_timer(timer, fn, data, 0, timer_setup)
#define setup_timer_on_stack(timer, fn, data)				\
	__legacy_setup_timer(timer, fn, data, 0, timer_setup_on_stack)
#define setup_deferrable_timer(timer, fn, data)			\
	__legacy_setup_timer(timer, fn, data, TIMER_DEFERRABLE, timer_setup)
#define setup_deferrable_timer_on_stack(timer, fn, data)		\
	__legacy_setup_timer(timer, fn, data, TIMER_DEFERRABLE, timer_setup_on_stack)
#define setup_pinned_timer(timer, fn, data)				\
	__legacy_setup_timer(timer, fn, data, TIMER_PINNED, timer_setup)
/* As in 4.9, init_timer() keeps .legacy_function and .data: some drivers set them before (pinctrl EINT debounce) */
#define init_timer(timer)						\
	timer_setup((timer), legacy_timer_fn, 0)
#define init_timer_pinned(timer)					\
	timer_setup((timer), legacy_timer_fn, TIMER_PINNED)
#define init_timer_deferrable(timer)					\
	timer_setup((timer), legacy_timer_fn, TIMER_DEFERRABLE)
#define init_timer_pinned_deferrable(timer)				\
	timer_setup((timer), legacy_timer_fn, TIMER_PINNED | TIMER_DEFERRABLE)
#define init_timer_on_stack(timer)					\
	timer_setup_on_stack((timer), legacy_timer_fn, 0)
#define init_timer_deferrable_on_stack(timer)				\
	timer_setup_on_stack((timer), legacy_timer_fn, TIMER_DEFERRABLE)
#endif /* CONFIG_MEDIATEK_SOLUTION */

#ifdef CONFIG_DEBUG_OBJECTS_TIMERS
extern void destroy_timer_on_stack(struct timer_list *timer);
#else
static inline void destroy_timer_on_stack(struct timer_list *timer) { }
#endif

#define from_timer(var, callback_timer, timer_fieldname) \
	container_of(callback_timer, typeof(*var), timer_fieldname)

/**
 * timer_pending - is a timer pending?
 * @timer: the timer in question
 *
 * timer_pending will tell whether a given timer is currently pending,
 * or not. Callers must ensure serialization wrt. other operations done
 * to this timer, eg. interrupt contexts, or other CPUs on SMP.
 *
 * return value: 1 if the timer is pending, 0 if not.
 */
static inline int timer_pending(const struct timer_list * timer)
{
	return timer->entry.pprev != NULL;
}

extern void add_timer_on(struct timer_list *timer, int cpu);
extern int del_timer(struct timer_list * timer);
extern int mod_timer(struct timer_list *timer, unsigned long expires);
extern int mod_timer_pending(struct timer_list *timer, unsigned long expires);
extern int timer_reduce(struct timer_list *timer, unsigned long expires);

/*
 * The jiffies value which is added to now, when there is no timer
 * in the timer wheel:
 */
#define NEXT_TIMER_MAX_DELTA	((1UL << 30) - 1)

extern void add_timer(struct timer_list *timer);

extern int try_to_del_timer_sync(struct timer_list *timer);
extern int del_timer_sync(struct timer_list *timer);

#define del_singleshot_timer_sync(t) del_timer_sync(t)

extern void init_timers(void);
extern void run_local_timers(void);
struct hrtimer;
extern enum hrtimer_restart it_real_fn(struct hrtimer *);

#if defined(CONFIG_SMP) && defined(CONFIG_NO_HZ_COMMON)
struct ctl_table;

extern unsigned int sysctl_timer_migration;
int timer_migration_handler(struct ctl_table *table, int write,
			    void __user *buffer, size_t *lenp,
			    loff_t *ppos);
#endif

unsigned long __round_jiffies(unsigned long j, int cpu);
unsigned long __round_jiffies_relative(unsigned long j, int cpu);
unsigned long round_jiffies(unsigned long j);
unsigned long round_jiffies_relative(unsigned long j);

unsigned long __round_jiffies_up(unsigned long j, int cpu);
unsigned long __round_jiffies_up_relative(unsigned long j, int cpu);
unsigned long round_jiffies_up(unsigned long j);
unsigned long round_jiffies_up_relative(unsigned long j);

#ifdef CONFIG_HOTPLUG_CPU
int timers_prepare_cpu(unsigned int cpu);
int timers_dead_cpu(unsigned int cpu);
#else
#define timers_prepare_cpu	NULL
#define timers_dead_cpu		NULL
#endif

#endif
