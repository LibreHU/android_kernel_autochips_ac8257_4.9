/*
 * SPM and SCP memory in a recovery-partition boot.
 *
 * In a normal boot the LK reserves 0x77ff0000 (64 KiB, mblock-9-SPM-reserved) and 0x9f900000 (6 MiB,
 * mblock-10-SCP-reserved) and adds them to the device tree it passes. In a recovery-partition boot it
 * does neither, and Linux used them as ordinary RAM. Stage 2k reserved them in the built-in device
 * tree instead, but the LK of a normal boot then finds its own reservations overlapping ours
 * ("reserved_memory_conflict_check fatal error keep while (1)") and never starts the kernel: its
 * watchdog resets the unit (stage 2s, boot partition stuck at the logo).
 *
 * So keep them out of Linux here, and only when the LK says "recovery" on the command line (it does
 * in a recovery-partition boot only). Removed from memblock, as "no-map" reserved memory would be.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <linux/sizes.h>

static const struct {
	phys_addr_t base;
	phys_addr_t size;
	const char *name;
} ac8257_recovery_mem[] __initconst = {
	{ 0x77ff0000, SZ_64K, "SPM" },
	{ 0x9f900000, 6 * SZ_1M, "SCP" },
};

/* "recovery" alone on the command line: early params run after the DT memory nodes are added */
static int __init ac8257_recovery_mem_setup(char *unused)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ac8257_recovery_mem); i++) {
		memblock_remove(ac8257_recovery_mem[i].base, ac8257_recovery_mem[i].size);
		pr_info("ac8257: recovery boot, %s memory %pa+%pa kept out of Linux\n",
			ac8257_recovery_mem[i].name, &ac8257_recovery_mem[i].base,
			&ac8257_recovery_mem[i].size);
	}
	return 0;
}
early_param("recovery", ac8257_recovery_mem_setup);

/*
 * Diagnostic (4.19 port): the 4.19 kernel crashes in a boot-partition boot only (stack corruption in
 * __arm_v7s_unmap, silent resets), never from the recovery partition. Show whether the SPM and SCP
 * regions are kept out of the page allocator in this boot, and the /reserved-memory nodes the LK passed.
 * A region whose pages are neither no-map (no valid pfn) nor PageReserved was handed to Linux as RAM
 * while the SPM / SCP firmware may still write to it.
 */
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_address.h>

static int __init ac8257_reserved_mem_report(void)
{
	struct device_node *parent, *np;
	int i;

	for (i = 0; i < ARRAY_SIZE(ac8257_recovery_mem); i++) {
		unsigned long pfn = PHYS_PFN(ac8257_recovery_mem[i].base);
		unsigned long end = PHYS_PFN(ac8257_recovery_mem[i].base + ac8257_recovery_mem[i].size);
		unsigned long nomap = 0, reserved = 0, ram = 0;

		for (; pfn < end; pfn++) {
			if (!pfn_valid(pfn))
				nomap++;
			else if (PageReserved(pfn_to_page(pfn)))
				reserved++;
			else
				ram++;
		}
		pr_info("ac8257: %s %pa+%pa: %lu pages no-map, %lu reserved, %lu given to Linux%s\n",
			ac8257_recovery_mem[i].name, &ac8257_recovery_mem[i].base,
			&ac8257_recovery_mem[i].size, nomap, reserved, ram,
			ram ? " (NOT PROTECTED)" : "");
	}

	parent = of_find_node_by_path("/reserved-memory");
	if (!parent)
		return 0;
	for_each_child_of_node(parent, np) {
		struct resource res;

		if (of_address_to_resource(np, 0, &res))
			pr_info("ac8257: reserved-memory %s (no reg)%s\n", np->name,
				of_property_read_bool(np, "no-map") ? " no-map" : "");
		else
			pr_info("ac8257: reserved-memory %s %pR%s\n", np->full_name, &res,
				of_property_read_bool(np, "no-map") ? " no-map" : "");
	}
	of_node_put(parent);
	return 0;
}
late_initcall(ac8257_reserved_mem_report);
