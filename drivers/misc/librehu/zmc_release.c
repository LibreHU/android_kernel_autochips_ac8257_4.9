/*
 * "mediatek,zone_movable_cma" memory given back to Linux when ZONE_MOVABLE_CMA is not built.
 *
 * The LK adds a reserved-memory node "zone-movable-cma-memory" to the device tree it passes (UJC201,
 * 6 GiB: base 0xc0000000, size 0xffc00000, about 4 GiB). MediaTek's ZMC
 * (drivers/misc/mediatek/zone_movable_cma, CONFIG_ZONE_MOVABLE_CMA, built in the 4.9 kernel) turns it
 * into the movable zone of ordinary RAM: "Memory: 5864028K/6209656K available" under 4.9. This 4.19
 * kernel does not build ZMC (no TEE, no secure video memory user), and the generic reserved-memory
 * code kept the whole region reserved: "1670328K/6209656K available ... 4539328K reserved" (unit
 * tries of 15/11/2026). Given back to memblock as ordinary memory here, as the "Abort reserve memory"
 * path of zmc_memory_init() does.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <linux/of_reserved_mem.h>

static int __init ac8257_zmc_release(struct reserved_mem *rmem)
{
	memblock_free(rmem->base, rmem->size);
	memblock_add(rmem->base, rmem->size);
	pr_info("ac8257: %s %pa+%pa given back as ordinary memory (no ZONE_MOVABLE_CMA)\n",
		rmem->name, &rmem->base, &rmem->size);
	/* not a reserved region any more */
	return -ENODEV;
}
RESERVEDMEM_OF_DECLARE(ac8257_zmc_release, "mediatek,zone_movable_cma", ac8257_zmc_release);
