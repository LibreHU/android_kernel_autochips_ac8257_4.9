/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/bug.h>
#include <linux/suspend.h>
#include <asm/cacheflush.h>
#include <mt-plat/mtk_secure_api.h>
#include <mt-plat/mtk_auxadc_intf.h>
#include <linux/topology.h>
#include "mtk_hps_internal.h"

#ifdef CONFIG_MTK_ICCS_SUPPORT
#include <mach/mtk_cpufreq_api.h>
#include <mtk_iccs.h>
#endif

#define BUCK_CTRL_DBLOG		(0)

/* FIXME : follow K44 first */
#define CPU_PRI_PERF 20
/* FIXME : follow K44 first */

static struct notifier_block cpu_hotplug_nb;

/*
 * LibreHU (UJC201): the cluster is switched to HW mode below (mp_enter_suspend), where the SPMC powers a core off
 * by itself once PSCI CPU_OFF has put it in WFI. The ATF's MTK_SIP_POWER_DOWN_CORE (plat_pm.c mt_core_ops ->
 * "spmc: power off core") then writes the SW power control of the core and polls SPM PWR_STATUS (0x10006180) for
 * it to drop, with no timeout: on the UJC201 that poll never ends and the whole SoC hangs at the first CPU going
 * offline (29/10: echo 0 > cpu3/online, pm_test "processors"). 1 = do not send the MTK SIP power down calls at
 * CPU_DEAD (PSCI alone); /sys/module/mtk_hotplug_cb/parameters/skip_sip_power_down.
 */
static bool skip_sip_power_down;
module_param(skip_sip_power_down, bool, 0644);
static DECLARE_BITMAP(cpu_cluster0_bits, CONFIG_NR_CPUS);
struct cpumask *mtk_cpu_cluster0_mask = to_cpumask(cpu_cluster0_bits);

static DECLARE_BITMAP(cpu_cluster1_bits, CONFIG_NR_CPUS);
struct cpumask *mtk_cpu_cluster1_mask = to_cpumask(cpu_cluster1_bits);

static DECLARE_BITMAP(cpu_cluster2_bits, CONFIG_NR_CPUS);
struct cpumask *mtk_cpu_cluster2_mask = to_cpumask(cpu_cluster2_bits);

static unsigned long default_cluster0_mask = 0x000F;
static unsigned long default_cluster1_mask = 0x00F0;
static unsigned long default_cluster2_mask = 0x0300;

static int cpu_hotplug_cb_notifier(struct notifier_block *self,
					 unsigned long action, void *hcpu)
{
	unsigned int cpu = (long)hcpu;
	struct cpumask cpuhp_cpumask;
	struct cpumask cpu_online_cpumask;
	unsigned int first_cpu;
	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
		if (cpu < cpumask_weight(mtk_cpu_cluster0_mask)) {
			first_cpu = cpumask_first_and(cpu_online_mask,
						mtk_cpu_cluster0_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_MTK_ICCS_SUPPORT
				if (hps_get_iccs_pwr_status(cpu >> 2) == 0x7) {
					iccs_set_cache_shared_state(cpu >> 2,
					0);
					break;
				}
#endif
			}
		} else if ((cpu >= cpumask_weight(mtk_cpu_cluster0_mask)) &&
			(cpu < (cpumask_weight(mtk_cpu_cluster0_mask) +
				  cpumask_weight(mtk_cpu_cluster1_mask)))) {
			first_cpu = cpumask_first_and(cpu_online_mask,
			mtk_cpu_cluster1_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_MTK_ICCS_SUPPORT
				if (hps_get_iccs_pwr_status(cpu >> 2) == 0x7) {
					iccs_set_cache_shared_state(cpu >> 2,
					0);
					break;
				}
#endif
			}
		} else if ((cpu >= (cpumask_weight(mtk_cpu_cluster0_mask) +
				cpumask_weight(mtk_cpu_cluster1_mask))) &&
				(cpu < (cpumask_weight(mtk_cpu_cluster0_mask) +
				cpumask_weight(mtk_cpu_cluster1_mask) +
				cpumask_weight(mtk_cpu_cluster2_mask))))  {
			first_cpu = cpumask_first_and(cpu_online_mask,
			mtk_cpu_cluster2_mask);
			if (first_cpu == CONFIG_NR_CPUS) {
#ifdef CONFIG_MTK_ICCS_SUPPORT
				if (hps_get_iccs_pwr_status(cpu >> 2) == 0x7) {
					iccs_set_cache_shared_state(cpu >> 2,
					0);
					break;
				}
#endif
				mt_secure_call(MTK_SIP_POWER_UP_CLUSTER, 2, 0,
				0, 0);
			}
		}
		break;

#ifdef CONFIG_HOTPLUG_CPU
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		if (skip_sip_power_down) {
			pr_info("[LibreHU] cpu%u dead: MTK SIP power down skipped (PSCI only)\n", cpu);
			break;
		}
		mt_secure_call(MTK_SIP_POWER_DOWN_CORE, cpu, 0, 0, 0);
		arch_get_cluster_cpus(&cpuhp_cpumask,
		arch_get_cluster_id(cpu));
		cpumask_and(&cpu_online_cpumask, &cpuhp_cpumask,
		cpu_online_mask);
		if (!cpumask_weight(&cpu_online_cpumask)) {
#ifdef CONFIG_MTK_ICCS_SUPPORT
			if (hps_get_iccs_pwr_status(cpu >> 2) == 0xb) {
				mt_cpufreq_set_iccs_frequency_by_cluster(1,
				cpu >> 2, iccs_get_shared_cluster_freq());
				iccs_set_cache_shared_state(cpu >> 2, 1);
				break;
			}
#endif
			mt_secure_call(MTK_SIP_POWER_DOWN_CLUSTER, cpu/4, 0,
			0, 0);
		}
		break;
#endif /* CONFIG_HOTPLUG_CPU */

	default:
		break;
	}
	return NOTIFY_OK;
}
static __init int hotplug_cb_init(void)
{
	int ret;
	int i;

	mp_enter_suspend(0, 1);/*Switch LL cluster to HW mode*/
	cpumask_clear(mtk_cpu_cluster0_mask);
	cpumask_clear(mtk_cpu_cluster1_mask);
	cpumask_clear(mtk_cpu_cluster2_mask);
	mtk_cpu_cluster0_mask->bits[0] = default_cluster0_mask;
	mtk_cpu_cluster1_mask->bits[0] = default_cluster1_mask;
	mtk_cpu_cluster2_mask->bits[0] = default_cluster2_mask;
	for (i = 0; i < num_possible_cpus(); i++)
		set_cpu_present(i, true);
	cpu_hotplug_nb = (struct notifier_block) {
		.notifier_call	= cpu_hotplug_cb_notifier,
		.priority	= CPU_PRI_PERF + 1,
	};

	ret = register_cpu_notifier(&cpu_hotplug_nb);
	if (ret)
		return ret;
	pr_info("CPU Hotplug Low Power Notification\n");
	return 0;
}
early_initcall(hotplug_cb_init);
