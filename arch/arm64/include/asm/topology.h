/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_TOPOLOGY_H
#define __ASM_TOPOLOGY_H

#include <linux/cpumask.h>

struct cpu_topology {
	int thread_id;
	int core_id;
	int package_id;
	int llc_id;
	cpumask_t thread_sibling;
	cpumask_t core_sibling;
	cpumask_t llc_sibling;
};

extern struct cpu_topology cpu_topology[NR_CPUS];
extern unsigned long arch_get_max_cpu_capacity(int cpu);
extern unsigned long arch_get_cur_cpu_capacity(int cpu);

#define topology_physical_package_id(cpu)	(cpu_topology[cpu].package_id)
#define topology_core_id(cpu)		(cpu_topology[cpu].core_id)
#define topology_core_cpumask(cpu)	(&cpu_topology[cpu].core_sibling)
#define topology_sibling_cpumask(cpu)	(&cpu_topology[cpu].thread_sibling)
#define topology_llc_cpumask(cpu)	(&cpu_topology[cpu].llc_sibling)
/* MediaTek drivers (PPM, hotplug, perfmgr): capacity of a CPU at its highest OPP */
#define topology_max_cpu_capacity(cpu) (arch_get_max_cpu_capacity(cpu))

void init_cpu_topology(void);
void store_cpu_topology(unsigned int cpuid);
void remove_cpu_topology(unsigned int cpuid);
const struct cpumask *cpu_coregroup_mask(int cpu);

#ifdef CONFIG_NUMA

struct pci_bus;
int pcibus_to_node(struct pci_bus *bus);
#define cpumask_of_pcibus(bus)	(pcibus_to_node(bus) == -1 ?		\
				 cpu_all_mask :				\
				 cpumask_of_node(pcibus_to_node(bus)))

#endif /* CONFIG_NUMA */

#include <linux/arch_topology.h>

/* Replace task scheduler's default frequency-invariant accounting */
#define arch_scale_freq_capacity topology_get_freq_scale

/* Replace task scheduler's default max-frequency-invariant accounting */
#define arch_scale_max_freq_capacity topology_get_max_freq_scale

/* Replace task scheduler's default cpu-invariant accounting */
#define arch_scale_cpu_capacity topology_get_cpu_scale

/* Enable topology flag updates */
#define arch_update_cpu_topology topology_update_cpu_topology

/* Extras of CPU & Cluster functions */
extern int arch_cpu_is_big(unsigned int cpu);
extern int arch_cpu_is_little(unsigned int cpu);
extern int arch_is_multi_cluster(void);
extern int arch_is_smp(void);
extern int arch_get_nr_clusters(void);
extern int arch_get_cluster_id(unsigned int cpu);
extern void arch_get_cluster_cpus(struct cpumask *cpus, int cluster_id);
extern int arch_better_capacity(unsigned int cpu);
void arch_build_cpu_topology_domain(void);

#include <asm-generic/topology.h>

#endif /* _ASM_ARM_TOPOLOGY_H */
