/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>
#include <asm/errno.h>
#include <asm/unwind.h>

#include "proc-macros.S"

/*
 *	v7_dma_inv_range(start,end)
 *
 *	Invalidate the data cache within the specified region; we will
 *	be performing a DMA operation in this region and we want to
 *	purge old data in the cache.
 *
 *	- start   - virtual start address of region
 *	- end     - virtual end address of region
 */
ENTRY(v7_dma_inv_range)
	dcache_line_size r2, r3
	sub	r3, r2, #1
	tst	r0, r3
	bic	r0, r0, r3
#ifdef CONFIG_ARM_ERRATA_764369
	ALT_SMP(W(dsb))
	ALT_UP(W(nop))
#endif
	mcrne	p15, 0, r0, c7, c14, 1		@ clean & invalidate D / U line
	addne	r0, r0, r2

	tst	r1, r3
	bic	r1, r1, r3
	mcrne	p15, 0, r1, c7, c14, 1		@ clean & invalidate D / U line
	cmp	r0, r1
1:
	mcrlo	p15, 0, r0, c7, c6, 1		@ invalidate D / U line
	addlo	r0, r0, r2
	cmplo	r0, r1
	blo	1b
	dsb	st
	ret	lr
ENDPROC(v7_dma_inv_range)

/*
 *      __inval_dcache_area(start,size)
 *
 *      Ensure that any D-cache lines for the interval [start, start+size)
 *      are cleaned or/and invalidated to the PoC.
 *
 *      - start        - virtual start address of region (EL0/EL1)
 *      - size         - size in question
 */
ENTRY(__inval_dcache_area)
	add	r1, r1, r0
	b	v7_dma_inv_range
ENDPROC(__inval_dcache_area)

