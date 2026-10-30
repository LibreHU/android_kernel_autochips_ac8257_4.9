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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/io.h>
#include <linux/math64.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <mt-plat/mtk_chip.h>

/* local include */
#include "mtk_cpufreq_api.h"
#include "mtk_upower.h"
#include "mtk_unified_power_data.h"
#include "mtk_devinfo.h"
#include "upmu_common.h"

//#ifndef EARLY_PORTING_SPOWER
#include "mtk_common_static_power.h"
//#endif

#undef  BIT
#define BIT(bit)	(1U << (bit))

#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
/**
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r:	Range in the form of MSB:LSB
 */
#define BITMASK(r)	\
	(((unsigned int) -1 >> (31 - MSB(r))) & ~((1U << LSB(r)) - 1))

/**
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS(r, val)	((val << LSB(r)) & BITMASK(r))

#define GET_BITS_VAL(_bits_, _val_)   \
	(((_val_) & (BITMASK(_bits_))) >> ((0) ? _bits_))
/* #if (NR_UPOWER_TBL_LIST <= 1) */
struct upower_tbl final_upower_tbl[NR_UPOWER_BANK] = {};
/* #endif */

int degree_set[NR_UPOWER_DEGREE] = {
		UPOWER_DEGREE_0,
		UPOWER_DEGREE_1,
		UPOWER_DEGREE_2,
		UPOWER_DEGREE_3,
		UPOWER_DEGREE_4,
		UPOWER_DEGREE_5,
};

/* collect all the raw tables */
#define INIT_UPOWER_TBL_INFOS(name, tbl) {__stringify(name), &tbl}
struct upower_tbl_info upower_tbl_list[NR_UPOWER_TBL_LIST][NR_UPOWER_BANK] = {
	/* FY */
	[0] = {
	/*INIT_UPOWER_TBL_INFOS(UPOWER_BANK_L, upower_tbl_l_FY), */
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_LL, upower_tbl_ll_FY),
	/*INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_L, upower_tbl_cluster_l_FY), */
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_LL, upower_tbl_cluster_ll_FY),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CCI, upower_tbl_cci_FY),
	},
	/* SB */
	[1] = {
	/*INIT_UPOWER_TBL_INFOS(UPOWER_BANK_L, upower_tbl_l_SB), */
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_LL, upower_tbl_ll_SB),
	/*INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_L, upower_tbl_cluster_l_SB),*/
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_LL, upower_tbl_cluster_ll_SB),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CCI, upower_tbl_cci_SB),
	},
	/* C65T */
#if 0
	[2] = {
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_L, upower_tbl_l_C65T),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_LL, upower_tbl_ll_C65T),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_L, upower_tbl_cluster_l_C65T),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_LL, upower_tbl_cluster_ll_C65T),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CCI, upower_tbl_cci_C65T),
	},
	/* C65 */
	[3] = {
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_L, upower_tbl_l_C65),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_LL, upower_tbl_ll_C65),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_L, upower_tbl_cluster_l_C65),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_LL, upower_tbl_cluster_ll_C65),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CCI, upower_tbl_cci_C65),
	},
	/* C62 */
	[4] = {
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_L, upower_tbl_l_C62),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_LL, upower_tbl_ll_C62),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_L, upower_tbl_cluster_l_C62),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_LL, upower_tbl_cluster_ll_C62),
	INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CCI, upower_tbl_cci_C62),
	},
#endif
};
/* Upower will know how to apply voltage that comes from EEM */
unsigned char upower_recognize_by_eem[NR_UPOWER_BANK] = {
#if 0
	UPOWER_BANK_L, /* L EEM apply voltage to L upower bank */
#endif
	UPOWER_BANK_LL, /* LL EEM apply voltage to LL upower bank */
#if 0
	UPOWER_BANK_L, /* L EEM apply voltage to CLS_L upower bank */
#endif
	UPOWER_BANK_LL, /* LL EEM apply voltage to CLS_LL upower bank */
	UPOWER_BANK_CCI, /* CCI EEM apply voltage to CCI upower bank */
};

/* Used for rcu lock, points to all the raw tables list*/
struct upower_tbl_info *p_upower_tbl_infos = &upower_tbl_list[0][0];

#ifndef EARLY_PORTING_SPOWER
int upower_bank_to_spower_bank(int upower_bank)
{
	int ret;

	switch (upower_bank) {
#if 0
	case UPOWER_BANK_L:
		ret = MTK_SPOWER_CPUL;
		break;
#endif
	case UPOWER_BANK_LL:
		ret = MTK_SPOWER_CPULL;
		break;
#if 0
	case UPOWER_BANK_CLS_L:
		ret = MTK_SPOWER_CPUL_CLUSTER;
		break;
#endif
	case UPOWER_BANK_CLS_LL:
		ret = MTK_SPOWER_CPULL_CLUSTER;
		break;

	case UPOWER_BANK_CCI:
		ret = MTK_SPOWER_CCI;
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}
#endif

#if 0
static void upower_scale_l_cap(void)
{
	unsigned int ratio;
	unsigned int temp;
	unsigned int max_cap = 1024;
	int i, j;
	struct upower_tbl *tbl;

	/* get L opp0's cap and calculate scaling ratio */
	/* ratio = round_up(1024 * 1000 / opp0 cap) */
	/* new cap = orig cap * ratio / 1000 */
	tbl = upower_tbl_infos[UPOWER_BANK_L].p_upower_tbl;
	temp = tbl->row[UPOWER_OPP_NUM - 1].cap;
	ratio = ((max_cap * 1000) + (temp - 1)) / temp;
	upower_debug("scale ratio = %d, orig cap = %d\n", ratio, temp);

	/* if L opp0's cap is 1024, no need to scale cap anymore */
	if (temp == 1024)
		return;

	/* scaling L and cluster L cap value */
	for (i = 0; i < NR_UPOWER_BANK; i++) {
		if ((i == UPOWER_BANK_L) || (i == UPOWER_BANK_CLS_L)) {
			tbl = upower_tbl_infos[i].p_upower_tbl;
			for (j = 0; j < UPOWER_OPP_NUM; j++) {
				temp = tbl->row[j].cap;
				tbl->row[j].cap = temp * ratio / 1000;
			}
		}
	}

	/* check opp0's cap after scaling */
	tbl = upower_tbl_infos[UPOWER_BANK_L].p_upower_tbl;
	temp = tbl->row[UPOWER_OPP_NUM - 1].cap;
	if (temp != 1024) {
		upower_debug("new cap is not 1024 after scaling (%d)\n", ratio);
		tbl->row[UPOWER_OPP_NUM - 1].cap = 1024;
	}
}
#endif

/****************************************************
 * According to chip version get the raw upower tbl *
 * and let upower_tbl_infos points to it.           *
 * Choose a non used upower tbl location and let    *
 * upower_tbl_ref points to it to store target      *
 * power tbl.                                       *
 ***************************************************/

#ifdef CONFIG_MACH_AC8257
/*
 * LibreHU: power table of the CPU overclock table (CPU_LEVEL_3, cpufreq_v1 mach/ac8257 opp table, 16 OPPs
 * up to 2601 MHz). There is no upower table for it: without this the FY table was used, read by OPP
 * index (upower_get_power: row = 15 - opp), so OPP 0 at 2.6 GHz got the power of 2.0 GHz and the PPM
 * power / thermal budget and the scheduler energy model underestimated the high OPPs (too high, then
 * hard thermal throttling). Rows of the frequencies the FY table has are copied from it; above 2001 MHz
 * they are extrapolated from the 2001 MHz row: capacity ~ f, dynamic power ~ f.V^2, leakage ~ V^3
 * (on the high side, so that the PPM throttles early rather than late). Capacities are then rescaled so
 * that the highest OPP is 1024, the end of the scheduler capacity scale.
 */
#define AC8257_OC_LEVEL		3
static const unsigned int ac8257_fy_mhz[UPOWER_OPP_NUM] = {	/* FY table, OPP 0 first */
	2001, 1917, 1834, 1767, 1700, 1633, 1533, 1466, 1400, 1308, 1216, 1125, 1056, 987, 918, 850,
};
static const unsigned int ac8257_oc_mhz[UPOWER_OPP_NUM] = {	/* CPU_DVFS_FREQx_LL_OC */
	2601, 2501, 2401, 2301, 2201, 2101, 2001, 1917, 1834, 1700, 1533, 1400, 1308, 1216, 1125, 987,
};
static const unsigned int ac8257_oc_volt[UPOWER_OPP_NUM] = {	/* CPU_DVFS_VOLTx_VPROC_LL_OC, 10 uV */
	105000, 105000, 105000, 105000, 105000, 103750, 102500, 99375,
	96250, 91250, 85000, 80000, 77500, 75000, 72500, 68750,
};
static struct upower_tbl ac8257_oc_upower_tbl[NR_UPOWER_BANK];
static struct upower_tbl_info ac8257_oc_upower_tbl_list[NR_UPOWER_BANK];

static void ac8257_build_oc_upower_tbl(void)
{
	unsigned int b, j, k, d;

	for (b = 0; b < NR_UPOWER_BANK; b++) {
		const struct upower_tbl *fy = upower_tbl_list[0][b].p_upower_tbl;
		struct upower_tbl *oc = &ac8257_oc_upower_tbl[b];
		const struct upower_tbl_row *top = &fy->row[UPOWER_OPP_NUM - 1];	/* 2001 MHz */
		unsigned long long v0 = top->volt ? top->volt : 102500;

		*oc = *fy;
		for (j = 0; j < UPOWER_OPP_NUM; j++) {
			struct upower_tbl_row *r = &oc->row[UPOWER_OPP_NUM - 1 - j];
			unsigned long long f = ac8257_oc_mhz[j], v = ac8257_oc_volt[j];

			for (k = 0; k < UPOWER_OPP_NUM; k++)
				if (ac8257_fy_mhz[k] == ac8257_oc_mhz[j])
					break;
			if (k < UPOWER_OPP_NUM) {
				*r = fy->row[UPOWER_OPP_NUM - 1 - k];
			} else {
				/* one factor at a time: no 64-bit overflow (power < 2^32 uW, V < 2^17) */
				unsigned long long x;

				r->cap = div64_u64(top->cap * f, ac8257_fy_mhz[0]);
				x = div64_u64((u64)top->dyn_pwr * f, ac8257_fy_mhz[0]);
				x = div64_u64(x * v, v0);
				r->dyn_pwr = (unsigned int)div64_u64(x * v, v0);
				for (d = 0; d < NR_UPOWER_DEGREE; d++) {
					x = div64_u64((u64)top->lkg_pwr[d] * v, v0);
					x = div64_u64(x * v, v0);
					r->lkg_pwr[d] = (unsigned int)div64_u64(x * v, v0);
				}
			}
			r->volt = ac8257_oc_volt[j];
		}
		/* the scheduler capacity scale ends at 1024: the highest OPP gets it */
		{
			unsigned long long top_cap = oc->row[UPOWER_OPP_NUM - 1].cap;

			if (top_cap > 1024)
				for (j = 0; j < UPOWER_OPP_NUM; j++)
					oc->row[j].cap = div64_u64(oc->row[j].cap * 1024, top_cap);
		}
		ac8257_oc_upower_tbl_list[b].name = upower_tbl_list[0][b].name;
		ac8257_oc_upower_tbl_list[b].p_upower_tbl = oc;
	}
}
#endif

void get_original_table(void)
{
	/* unsigned int bin = 0; */
	unsigned short idx = 0; /* default use FY table */
	unsigned int i, j;

	idx = mt_cpufreq_get_cpu_level();
	upower_debug("idx = %d", idx);
	/* get location of reference table */
#ifdef CONFIG_MACH_AC8257
	if (idx == AC8257_OC_LEVEL) {
		ac8257_build_oc_upower_tbl();
		upower_tbl_infos = &ac8257_oc_upower_tbl_list[0];
	} else
#endif
	{
		if (idx >= NR_UPOWER_TBL_LIST)
			idx = 0;
		upower_tbl_infos = &upower_tbl_list[idx][0];
	}

	/* get location of target table */
	upower_tbl_ref = &final_upower_tbl[0];

	upower_debug("idx %d dest:%p, src:%p\n",
		(idx+1)%NR_UPOWER_TBL_LIST, upower_tbl_ref, upower_tbl_infos);

	/* If disable upower, ptr will point to original upower table */
	p_upower_tbl_infos = upower_tbl_infos;

	/*
	 *  Clear volt fields before eem run.
	 *  If eem is enabled, it will apply volt into it. If eem is disabled,
	 *  the values of volt are 0 , and upower will apply orig volt into it
	 */
	for (i = 0; i < NR_UPOWER_BANK; i++) {
		for (j = 0; j < UPOWER_OPP_NUM; j++)
			upower_tbl_ref[i].row[j].volt = 0;
	}
	for (i = 0; i < NR_UPOWER_BANK; i++)
		upower_debug("bank[%d] dest:%p dyn_pwr:%u, volt[0]%u\n",
			i, &upower_tbl_ref[i],
			upower_tbl_ref[i].row[0].dyn_pwr,
			upower_tbl_ref[i].row[0].volt);

#if 0
	/* Not support L+ now, scale L and cluster L cap to 1024 */
	upower_scale_l_cap();
#endif
}

MODULE_DESCRIPTION("MediaTek Unified Power Driver v0.0");
MODULE_LICENSE("GPL");
