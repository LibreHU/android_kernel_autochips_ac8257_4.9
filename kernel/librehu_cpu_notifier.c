// SPDX-License-Identifier: GPL-2.0
/*
 * LibreHU (4.19 port of the AutoChips AC8257 kernel): the CPU hotplug notifiers of 4.9, removed in 4.10, for the
 * MediaTek / AutoChips drivers written for them (include/linux/librehu_cpu_notifier.h). One notifier chain, called
 * from two dynamic cpuhp states set up before the secondary CPUs come up:
 *  - CPUHP_BP_PREPARE_DYN, on the control CPU: CPU_UP_PREPARE before a CPU comes up; CPU_DEAD once it is down, or
 *    CPU_UP_CANCELED when its bring-up failed before CPU_ONLINE;
 *  - CPUHP_AP_ONLINE_DYN, on the CPU itself: CPU_ONLINE once it is up, or CPU_DOWN_FAILED when an unplug failed
 *    after CPU_DOWN_PREPARE (cpuhp then calls the startup again); CPU_DOWN_PREPARE when it goes down.
 * As in 4.9, a notifier refusing CPU_UP_PREPARE or CPU_DOWN_PREPARE (NOTIFY_BAD) stops the hotplug and the notifiers
 * called before it get CPU_UP_CANCELED / CPU_DOWN_FAILED; the _FROZEN forms are sent during suspend and resume.
 * CPU_STARTING, CPU_DYING (on the CPU, interrupts off) and CPU_POST_DEAD are not sent: no driver built in this
 * kernel needs them (CPU_DYING only with CONFIG_LOCAL_WDT in the MediaTek watchdog, not set).
 * Notifiers are registered and called under cpu_maps_update_begin(), as in 4.9.
 */
#include <linux/cpu.h>
#include <linux/cpuhotplug.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/librehu_cpu_notifier.h>
#include <linux/notifier.h>
#include <linux/percpu.h>
#include <linux/printk.h>

static RAW_NOTIFIER_HEAD(librehu_cpu_chain);
/* CPU_ONLINE sent and not followed by CPU_DEAD yet (CPU_DEAD, else CPU_UP_CANCELED) */
static DEFINE_PER_CPU(bool, librehu_cpu_online_sent);
/* CPU_DOWN_PREPARE accepted: a new startup of the online state is the rollback of a failed unplug */
static DEFINE_PER_CPU(bool, librehu_cpu_down_prepared);

static inline unsigned long librehu_cpu_frozen(void)
{
	return cpuhp_tasks_frozen ? CPU_TASKS_FROZEN : 0;
}

/* Calls the chain; on a refusal, @undo to the notifiers called before the refusing one. Returns an errno. */
static int librehu_cpu_notify(unsigned long action, unsigned long undo, unsigned int cpu)
{
	void *hcpu = (void *)(long)cpu;
	int nr_calls = 0;
	int ret;

	ret = notifier_to_errno(__raw_notifier_call_chain(&librehu_cpu_chain, action | librehu_cpu_frozen(), hcpu,
							  -1, &nr_calls));
	if (ret && undo)
		__raw_notifier_call_chain(&librehu_cpu_chain, undo | librehu_cpu_frozen(), hcpu, nr_calls - 1, NULL);
	return ret;
}

static int librehu_cpu_up_prepare(unsigned int cpu)
{
	per_cpu(librehu_cpu_online_sent, cpu) = false;
	per_cpu(librehu_cpu_down_prepared, cpu) = false;
	return librehu_cpu_notify(CPU_UP_PREPARE, CPU_UP_CANCELED, cpu);
}

static int librehu_cpu_dead(unsigned int cpu)
{
	unsigned long action = per_cpu(librehu_cpu_online_sent, cpu) ? CPU_DEAD : CPU_UP_CANCELED;

	per_cpu(librehu_cpu_online_sent, cpu) = false;
	per_cpu(librehu_cpu_down_prepared, cpu) = false;
	librehu_cpu_notify(action, 0, cpu);
	return 0;
}

static int librehu_cpu_online(unsigned int cpu)
{
	unsigned long action = per_cpu(librehu_cpu_down_prepared, cpu) ? CPU_DOWN_FAILED : CPU_ONLINE;

	per_cpu(librehu_cpu_down_prepared, cpu) = false;
	per_cpu(librehu_cpu_online_sent, cpu) = true;
	librehu_cpu_notify(action, 0, cpu);
	return 0;
}

static int librehu_cpu_down_prepare(unsigned int cpu)
{
	int ret = librehu_cpu_notify(CPU_DOWN_PREPARE, CPU_DOWN_FAILED, cpu);

	if (!ret)
		per_cpu(librehu_cpu_down_prepared, cpu) = true;
	return ret;
}

int __register_cpu_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_register(&librehu_cpu_chain, nb);
}
EXPORT_SYMBOL(__register_cpu_notifier);

int register_cpu_notifier(struct notifier_block *nb)
{
	int ret;

	cpu_maps_update_begin();
	ret = __register_cpu_notifier(nb);
	cpu_maps_update_done();
	return ret;
}
EXPORT_SYMBOL(register_cpu_notifier);

void __unregister_cpu_notifier(struct notifier_block *nb)
{
	raw_notifier_chain_unregister(&librehu_cpu_chain, nb);
}
EXPORT_SYMBOL(__unregister_cpu_notifier);

void unregister_cpu_notifier(struct notifier_block *nb)
{
	cpu_maps_update_begin();
	__unregister_cpu_notifier(nb);
	cpu_maps_update_done();
}
EXPORT_SYMBOL(unregister_cpu_notifier);

static int __init librehu_cpu_notifier_init(void)
{
	unsigned int cpu;
	int ret;

	cpus_read_lock();
	for_each_online_cpu(cpu)
		per_cpu(librehu_cpu_online_sent, cpu) = true;
	cpus_read_unlock();

	ret = cpuhp_setup_state_nocalls(CPUHP_BP_PREPARE_DYN, "librehu/cpu_notifier:prepare",
					librehu_cpu_up_prepare, librehu_cpu_dead);
	if (ret < 0)
		goto err;
	ret = cpuhp_setup_state_nocalls(CPUHP_AP_ONLINE_DYN, "librehu/cpu_notifier:online",
					librehu_cpu_online, librehu_cpu_down_prepare);
	if (ret < 0)
		goto err;
	return 0;
err:
	pr_err("librehu: no hotplug state for the 4.9 CPU notifiers (%d)\n", ret);
	return ret;
}
early_initcall(librehu_cpu_notifier_init);
