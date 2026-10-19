#include <linux/of.h>
#include <linux/of_reserved_mem.h>

#ifdef CONFIG_HAVE_ARM_SMCCC
#include <mt-plat/mtk_secure_api.h>
#ifndef poweroff_cpu
#define poweroff_cpu(i) mt_secure_call(MTK_SIP_POWER_DOWN_CORE, i, 0, 0, 0)
#endif

#ifdef CONFIG_MACH_AC8257
#include <spm/mtk_spm.h>
#include <mtk_spm_reg.h>
#endif

#endif

#include "rsv_cpu_notifier.h"

unsigned int setup_rsv_cpu_mask = 0;
EXPORT_SYMBOL(setup_rsv_cpu_mask);

static int __init rsv_cpu_mask(char *str)
{
	get_option(&str, &setup_rsv_cpu_mask);
	return 0;
}
early_param("rsv_cpu_mask", rsv_cpu_mask);

static int rsv_cpu_notifier_callback(struct notifier_block *self,
	unsigned long event, void *data);
static struct notifier_block rsv_cpu_notifier = {
	.notifier_call = rsv_cpu_notifier_callback,
};

static phys_addr_t _rsv_cpu_rsv_mem_pa = 0;
static phys_addr_t _rsv_cpu_rsv_mem_len = 0;

static int __init rsv_cpu_rsv_mem_get(struct reserved_mem *rmem)
{
	_rsv_cpu_rsv_mem_pa = rmem->base;
	_rsv_cpu_rsv_mem_len = rmem->size;
	rsv_cpu_register_client(&rsv_cpu_notifier);
	return 0;
}
RESERVEDMEM_OF_DECLARE(rsv_cpu_rsv_mem, "autochips,rsv-cpu",
		rsv_cpu_rsv_mem_get);

extern int free_memblock_runtime(phys_addr_t phy_addr, phys_addr_t size);
static void rsv_cpu_release(unsigned int cpu)
{
	if (setup_rsv_cpu_mask & (1 << cpu)) {
#ifdef CONFIG_HAVE_ARM_SMCCC

#if defined(CONFIG_MACH_AC8257) && defined(__arm__)
		// The aarch64 NO need wait cpu in wfi.
		if (mtk_spm_base_ready()) {
			int wfi_b = 1 << (MP0_CPU0_STANDBYWFI_LSB + cpu);

			pr_info("detect cpu%u in wfi?\n", cpu);
			while ((readl(CPU_IDLE_STA) & wfi_b) == 0)
				;
			pr_info("Yes, cpu in wfi\n");
		}
#endif

		poweroff_cpu(cpu);
#endif
		setup_rsv_cpu_mask &= ~(1 << cpu);
	} else {
		pr_info("cpu%u is not used\n", cpu);
	}
	// TODO: per mask rsv cpu rsv mem release
	if (_rsv_cpu_rsv_mem_pa && _rsv_cpu_rsv_mem_len) {
		free_memblock_runtime(_rsv_cpu_rsv_mem_pa, _rsv_cpu_rsv_mem_len);
		_rsv_cpu_rsv_mem_pa = 0;
		_rsv_cpu_rsv_mem_len = 0;
	}
}

extern int cpu_up(unsigned int cpu);
static int rsv_cpu_notifier_callback(struct notifier_block *self,
	unsigned long event, void *data)
{
	unsigned int ev = (event >> RSV_CPU_EVENT_SHIFT) & RSV_CPU_EVENT_MASK;
	unsigned int val = event & RSV_CPU_EVENT_MASK;
	switch (ev) {
		case RSV_CPU_EVENT_RELEASE:
			rsv_cpu_release(val);
			cpu_up(val);
			break;
	}
	return 0;
}

