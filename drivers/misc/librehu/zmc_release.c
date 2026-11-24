/*
 * "mediatek,zone_movable_cma" memory given back to Linux when ZONE_MOVABLE_CMA is not built.
 *
 * The LK adds a reserved-memory node "zone-movable-cma-memory" to the device tree it passes (UJC201,
 * 6 GiB: base 0xc0000000, size 0xffc00000, about 4 GiB). On the 4.9 kernel (stock, AutoChips GPL and
 * LibreHU), MediaTek's ZMC (drivers/misc/mediatek/zone_movable_cma, CONFIG_ZONE_MOVABLE_CMA) asks the
 * secure memory driver what to carve out of it; the device tree asks for no secure video memory
 * ("memory-ssmr: setup_feature_size, svp-size: 0x0", "SSMR init: skipped, no requirement"), so no CMA
 * area is made ("0K cma-reserved", CmaTotal 0 kB) and zmc_memory_init() puts the whole region back in
 * memblock: ordinary RAM, in ZONE_DMA up to 4 GiB and ZONE_NORMAL above, used by every allocation
 * ("Memory: 5863548K/6209656K available" on LibreHU 4.9.337, 5850660K on the stock kernel booting TWRP).
 * Nothing on the unit uses that memory as secure memory (stock: "memory-ssmr: [INIT FAIL]: cma is not
 * inited").
 *
 * This 4.19 kernel does not build ZMC, and the generic reserved-memory code kept the whole region
 * reserved: "1670328K/6209656K available ... 4539328K reserved" (unit tries of 15/11/2026). Given back to
 * memblock here, as on 4.9; arch/arm64/mm/init.c makes no ZONE_MOVABLE of it (until 24/11/2026 it did,
 * which left only ZONE_DMA32 under 0xc0000000, about 1.9 GiB, to the kernel, the GPU and ION).
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
	pr_info("ac8257: %s %pa+%pa given back as ordinary memory, as on 4.9 (no ZONE_MOVABLE_CMA)\n",
		rmem->name, &rmem->base, &rmem->size);
	/* not a reserved region any more */
	return -ENODEV;
}
RESERVEDMEM_OF_DECLARE(ac8257_zmc_release, "mediatek,zone_movable_cma", ac8257_zmc_release);
