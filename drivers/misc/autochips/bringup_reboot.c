/*
 * Bring-up aid: panics CONFIG_AC8257_BRINGUP_PANIC_SECS seconds after boot.
 *
 * A test kernel that hangs in userspace (no display, no USB) leaves no log: the unit stays on the
 * logo, and getting out of it needs SP Flash Tool, which reinitialises the RAM, so pstore is lost.
 * A panic instead reboots through the MTK exception path (as the stage-1 tests showed, the LK then
 * starts the normal boot partition, even after a recovery-mode boot), with the kernel log, which
 * includes init's messages (printk.devkmsg=on), kept in pstore: console-ramoops and dmesg-ramoops.
 * This program is free software; GPL v2.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>

static void ac8257_bringup_panic(struct work_struct *work)
{
	panic("ac8257 bring-up: timed panic after %d s, to keep the log in pstore",
	      CONFIG_AC8257_BRINGUP_PANIC_SECS);
}

static DECLARE_DELAYED_WORK(ac8257_bringup_work, ac8257_bringup_panic);

static int __init ac8257_bringup_panic_init(void)
{
	pr_info("ac8257 bring-up: timed panic in %d s\n", CONFIG_AC8257_BRINGUP_PANIC_SECS);
	schedule_delayed_work(&ac8257_bringup_work, CONFIG_AC8257_BRINGUP_PANIC_SECS * HZ);
	return 0;
}
late_initcall(ac8257_bringup_panic_init);
