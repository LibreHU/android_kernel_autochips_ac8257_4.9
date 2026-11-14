// SPDX-License-Identifier: GPL-2.0
/*
 * LibreHU (4.19 port of the AutoChips AC8257 kernel): sched_ctl.c and eas_plus.c of the MediaTek 4.9 kernel drive
 * the MediaTek HMP / EAS scheduler changes, the core isolation and the global cpusets of the 4.9 kernel/sched and
 * kernel/cpuset.c, which this ACK 4.19 core does not have (WALT is not in ACK 4.19 either). They are not built;
 * what the other MediaTek drivers call of them is kept here, without effect, as they had none without WALT.
 */
#include <linux/export.h>
#include <linux/notifier.h>
#include <mt-plat/mtk_sched.h>

int sched_walt_enable(int user, int en)
{
	return 0;
}
EXPORT_SYMBOL(sched_walt_enable);

int register_sched_hint_notifier(struct notifier_block *nb)
{
	return 0;
}
EXPORT_SYMBOL(register_sched_hint_notifier);

int unregister_sched_hint_notifier(struct notifier_block *nb)
{
	return 0;
}
EXPORT_SYMBOL(unregister_sched_hint_notifier);
