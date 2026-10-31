/*
 * Copyright (C) 2015 MediaTek Inc.
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
#ifndef _CPU_CTRL_H
#define _CPU_CTRL_H


#include <mtk_ppm_api.h>

enum {
	CPU_KIR_PERF = 0,
	CPU_KIR_FPSGO,
	CPU_KIR_WIFI,
	CPU_KIR_BOOT,
	CPU_KIR_TOUCH,
	CPU_KIR_PERFTOUCH,
	CPU_KIR_USB,
	CPU_MAX_KIR
};
extern unsigned int mt_cpufreq_get_freq_by_idx(int id, int idx);
#ifdef CONFIG_MACH_AC8257
extern unsigned int mt_cpufreq_get_stock_freq_by_idx(int id, int idx);
#endif

/*
 * Frequency of an OPP index set by the vendor (boot_freq of the init scripts, touch_opp of the user
 * touch boost, built with FPSGO): an index of the stock table. The AC8257 overclock table has six
 * more OPPs on top, so the same index there meant a frequency up to 600 MHz higher (OPP 0: 2601
 * instead of 2001 MHz).
 */
static inline unsigned int cpu_ctrl_opp_to_freq(int cluster, int opp)
{
#ifdef CONFIG_MACH_AC8257
	return mt_cpufreq_get_stock_freq_by_idx(cluster, opp);
#else
	return mt_cpufreq_get_freq_by_idx(cluster, opp);
#endif
}
extern int update_userlimit_cpu_freq(int kicker, int num_cluster
				, struct ppm_limit_data *freq_limit);
extern int update_userlimit_cpu_core(int kicker, int num_cluster
				, struct ppm_limit_data *core_limit);

#endif /* _CPU_CTRL_H */
