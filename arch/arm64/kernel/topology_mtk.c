// SPDX-License-Identifier: GPL-2.0
/*
 * CPU and cluster helpers of the MediaTek / AutoChips 4.9 kernel (arch/arm64/kernel/topology_dts.c there),
 * rebuilt on the 4.19 arm64 topology for the MediaTek drivers that call them (cpufreq, PPM, hotplug, DCM,
 * perfmgr, CMO). A cluster is a package of the 4.19 topology: a "clusterN" of the device tree cpu-map, or MPIDR
 * affinity level 1 without a cpu-map. The 4.9 EAS energy tables and HMP domains are not rebuilt.
 *
 * LibreHU (4.19 port of the AC8257 kernel).
 */

#include <linux/arch_topology.h>
#include <linux/atomic.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/percpu.h>
#include <linux/sched/topology.h>
#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/topology.h>

static inline int mtk_cluster_of(unsigned int cpu)
{
	int id = cpu_topology[cpu].package_id;

	return id < 0 ? 0 : id;
}

int arch_get_nr_clusters(void)
{
	unsigned int cpu;
	int max_id = 0;

	for_each_possible_cpu(cpu)
		max_id = max(max_id, mtk_cluster_of(cpu));
	return max_id + 1;
}

int arch_is_multi_cluster(void)
{
	return arch_get_nr_clusters() > 1;
}

int arch_get_cluster_id(unsigned int cpu)
{
	return mtk_cluster_of(cpu);
}

void arch_get_cluster_cpus(struct cpumask *cpus, int cluster_id)
{
	unsigned int cpu;

	cpumask_clear(cpus);
	for_each_possible_cpu(cpu)
		if (mtk_cluster_of(cpu) == cluster_id)
			cpumask_set_cpu(cpu, cpus);
}

unsigned long arch_get_max_cpu_capacity(int cpu)
{
	return topology_get_cpu_scale(NULL, cpu);
}

unsigned long arch_get_cur_cpu_capacity(int cpu)
{
	unsigned long scale_freq = topology_get_freq_scale(cpu);

	if (!scale_freq)
		scale_freq = SCHED_CAPACITY_SCALE;
	return topology_get_cpu_scale(NULL, cpu) * scale_freq / SCHED_CAPACITY_SCALE;
}

/* 1 when every CPU has the same capacity (one cluster of identical cores, as the AC8257) */
int arch_is_smp(void)
{
	unsigned long cap, min_cap = ULONG_MAX, max_cap = 0;
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		cap = topology_get_cpu_scale(NULL, cpu);
		min_cap = min(min_cap, cap);
		max_cap = max(max_cap, cap);
	}
	return min_cap == max_cap;
}

/* As the 4.9 kernel: "big" is decided by the core type (Cortex-A57 / A72) */
int arch_cpu_is_big(unsigned int cpu)
{
	switch (MIDR_PARTNUM(per_cpu(cpu_data, cpu).reg_midr)) {
	case ARM_CPU_PART_CORTEX_A57:
	case ARM_CPU_PART_CORTEX_A72:
		return 1;
	default:
		return 0;
	}
}

int arch_cpu_is_little(unsigned int cpu)
{
	return !arch_cpu_is_big(cpu);
}

int arch_better_capacity(unsigned int cpu)
{
	unsigned long min_cap = ULONG_MAX;
	unsigned int i;

	for_each_possible_cpu(i)
		min_cap = min(min_cap, topology_get_cpu_scale(NULL, i));
	return topology_get_cpu_scale(NULL, cpu) > min_cap;
}

/* Frequency bookkeeping that MediaTek cpufreq drivers feed (4.9 topology_dts.c interface) */
static DEFINE_PER_CPU(atomic_long_t, mtk_cpu_max_freq);
static DEFINE_PER_CPU(atomic_long_t, mtk_cpu_min_freq);

void arch_scale_set_curr_freq(int cpu, unsigned long freq)
{
	unsigned long max = atomic_long_read(&per_cpu(mtk_cpu_max_freq, cpu));

	if (max)
		arch_set_freq_scale(cpumask_of(cpu), freq, max);
}

void arch_scale_set_max_freq(int cpu, unsigned long freq)
{
	atomic_long_set(&per_cpu(mtk_cpu_max_freq, cpu), freq);
}

void arch_scale_set_min_freq(int cpu, unsigned long freq)
{
	atomic_long_set(&per_cpu(mtk_cpu_min_freq, cpu), freq);
}

unsigned long arch_scale_get_max_freq(int cpu)
{
	return atomic_long_read(&per_cpu(mtk_cpu_max_freq, cpu));
}

unsigned long arch_scale_get_min_freq(int cpu)
{
	return atomic_long_read(&per_cpu(mtk_cpu_min_freq, cpu));
}
