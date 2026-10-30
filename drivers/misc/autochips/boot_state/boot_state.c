/*
* Copyright (c) 2016 AutoChips Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <linux/io.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/syscore_ops.h>
#include <linux/mutex.h>
#include <linux/suspend.h>
#include <linux/workqueue.h>

#include "boot_state.h"

static const char * const status_boot[STATUS_LAST] = {
	[STATUS_UNKNOWN] = "unknown",
	[STATUS_BOOT_START] = "boot_start",
	[STATUS_BOOT_END] = "boot_end",
	[STATUS_SUSPEND_START] = "suspend_start",
	[STATUS_SUSPEND_END] = "suspend_end",
	[STATUS_RESUME_START] = "resume_start",
	[STATUS_RESUME_END] = "resume_end",
	[STATUS_SHUTDOWN_START] = "shutdown_start",
	[STATUS_SHUTDOWN_END] = "shutdown_end",
};

static BLOCKING_NOTIFIER_HEAD(bs_chain_head);


int register_bs_notifer(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&bs_chain_head, nb);
}
EXPORT_SYMBOL_GPL(register_bs_notifer);

int unregister_bs_notifer(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&bs_chain_head, nb);
}
EXPORT_SYMBOL_GPL(unregister_bs_notifer);

int bs_notifer_call_chain(unsigned long val)
{
	return blocking_notifier_call_chain(&bs_chain_head, val, NULL);
}
EXPORT_SYMBOL_GPL(bs_notifer_call_chain);

struct kobject *boot_state_kobj;
EXPORT_SYMBOL_GPL(boot_state_kobj);

int boot_level = STATUS_UNKNOWN;

/*
 * STATUS_RESUME_END only comes from userspace ("resume_end" written here by the QuickBoot service).
 * From STATUS_RESUME_START (boot_state_syscore_resume()) until then, hub.c keeps b_is_in_suspend
 * set: one enumeration attempt per USB device, usb-storage errors not retried, SCSI timeouts and
 * retries halved, bulk NAKs turned into errors on the MUSB port. A resume that userspace does not
 * report left the USB stack in that state until the next suspend: report it from here when
 * userspace has not done it resume_end_ms after the resume (0: never).
 */
static unsigned int resume_end_ms = 10000;
module_param(resume_end_ms, uint, 0644);
MODULE_PARM_DESC(resume_end_ms, "ms after a resume before the kernel reports resume_end itself (0: never)");

/* state changes from process context: sysfs and bs_resume_end_work */
static DEFINE_MUTEX(bs_state_lock);

static int state_change(int level)
{
	if (level >= STATUS_LAST || level < STATUS_UNKNOWN)
		return -EINVAL;

	pr_info("[BS] state_change(%d), and call notifier now\n", level);
	boot_level = level;
	bs_notifer_call_chain(boot_level);

	return boot_level;
}

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *attr,
					char *buf)
{
	char *s = buf;
	int level;

	for (level = STATUS_UNKNOWN; level < STATUS_LAST; level++) {
		if (status_boot[level]) {
			if (level == boot_level)
				s += sprintf(s, "[%s] ", status_boot[level]);
			else
				s += sprintf(s, "%s ", status_boot[level]);
		}
	}

	if (s != buf)
		*(s-1) = '\n';

	return (s - buf);
}

static ssize_t state_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	const char * const *s;
	char *p;
	int len;
	int level = STATUS_UNKNOWN;
	int error = -EINVAL;

	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;

	for (s = &status_boot[level]; level < STATUS_LAST; s++, level++) {
		if (*s && len == strlen(*s) && !strncmp(buf, *s, len)) {
			mutex_lock(&bs_state_lock);
			state_change(level);
			mutex_unlock(&bs_state_lock);
			error = 0;
			break;
		}
	}

	return error ? error : n;
}

static struct kobj_attribute state_attr = __ATTR(state,
			0644,
			state_show,
			state_store);

static int boot_state_syscore_suspend(void)
{
	state_change(STATUS_SUSPEND_END);
	return 0;
}

static void boot_state_syscore_resume(void)
{
	state_change(STATUS_RESUME_START);
}


static struct syscore_ops boot_state_syscore_ops = {
	.suspend = boot_state_syscore_suspend,
	.resume = boot_state_syscore_resume,
};

static void bs_resume_end_work_func(struct work_struct *work)
{
	mutex_lock(&bs_state_lock);
	if (boot_level == STATUS_RESUME_START) {
		pr_info("[BS] no resume_end from userspace %u ms after the resume, sent by the kernel\n",
			resume_end_ms);
		state_change(STATUS_RESUME_END);
	}
	mutex_unlock(&bs_state_lock);
}

static DECLARE_DELAYED_WORK(bs_resume_end_work, bs_resume_end_work_func);

static int boot_state_pm_notify(struct notifier_block *nb, unsigned long event, void *unused)
{
	switch (event) {
	case PM_SUSPEND_PREPARE:
		/* suspending again before the deadline: the next resume starts a new one */
		cancel_delayed_work_sync(&bs_resume_end_work);
		break;
	case PM_POST_SUSPEND:
		/* freezable: never runs between the freeze and the thaw of a suspend */
		if ((STATUS_RESUME_START == boot_level) && resume_end_ms)
			queue_delayed_work(system_freezable_wq, &bs_resume_end_work,
					   msecs_to_jiffies(resume_end_ms));
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block boot_state_pm_nb = {
	.notifier_call = boot_state_pm_notify,
};

static int __init boot_state_init(void)
{
	int ret = 0;

	boot_state_kobj = kobject_create_and_add("boot", NULL);
	if (!boot_state_kobj)
		return -ENOMEM;
	ret = sysfs_create_file(boot_state_kobj, &state_attr.attr);

	if (ret)
		pr_err("[BS] Fail to create file /sys/boot/state");

	register_syscore_ops(&boot_state_syscore_ops);
	register_pm_notifier(&boot_state_pm_nb);

	return ret;
}

static void __exit boot_state_exit(void)
{
	unregister_pm_notifier(&boot_state_pm_nb);
	cancel_delayed_work_sync(&bs_resume_end_work);
	unregister_syscore_ops(&boot_state_syscore_ops);
	kobject_put(boot_state_kobj);
}

module_init(boot_state_init);
module_exit(boot_state_exit);
MODULE_DESCRIPTION("ATC Boot State Driver");
MODULE_LICENSE("GPL");
