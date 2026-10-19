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
			state_change(level);
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

	return ret;
}

static void __exit boot_state_exit(void)
{
	unregister_syscore_ops(&boot_state_syscore_ops);
	kobject_put(boot_state_kobj);
}

module_init(boot_state_init);
module_exit(boot_state_exit);
MODULE_DESCRIPTION("ATC Boot State Driver");
MODULE_LICENSE("GPL");
