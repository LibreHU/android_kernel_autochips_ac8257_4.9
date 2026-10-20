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
