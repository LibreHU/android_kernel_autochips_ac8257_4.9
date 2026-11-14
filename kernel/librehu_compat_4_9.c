// SPDX-License-Identifier: GPL-2.0
/*
 * LibreHU (4.19 port of the AutoChips AC8257 kernel): functions the MediaTek / AutoChips drivers written for 4.9
 * call and the 4.19 core does not have any more. Declared in include/linux/librehu_compat_4_9.h.
 */
#include <linux/export.h>
#include <linux/librehu_compat_4_9.h>

/*
 * "printk too much" detector of the MediaTek 4.9 printk.c (video codec, camera ISP, mtprintk). It only worked in
 * engineering builds (CONFIG_MTK_ENG_BUILD, not set in the 4.9 kernel of the UJC201): there these were already
 * no-ops returning 0, as here.
 */
void set_detect_count(int count)
{
}
EXPORT_SYMBOL(set_detect_count);

int get_detect_count(void)
{
	return 0;
}
EXPORT_SYMBOL(get_detect_count);

void set_logtoomuch_enable(int value)
{
}
EXPORT_SYMBOL(set_logtoomuch_enable);

int get_logtoomuch_enable(void)
{
	return 0;
}
EXPORT_SYMBOL(get_logtoomuch_enable);

/*
 * Tracepoints of include/trace/events/mtk_events.h (PPM, SSPM, CM manager, HPS, GPU, perf_tracker): the MediaTek
 * 4.9 kernel created them in kernel/trace/trace.c. Last in this file: CREATE_TRACE_POINTS only for this header.
 */
#define CREATE_TRACE_POINTS
#include <trace/events/mtk_events.h>
