/*
 * Bring-up aid: panics CONFIG_AC8257_BRINGUP_PANIC_SECS seconds after boot.
 *
 * A test kernel that hangs in userspace (no display, no USB) leaves no log: the unit stays on the
 * logo, and getting out of it needs SP Flash Tool, which reinitialises the RAM, so pstore is lost.
 * A panic instead reboots through the MTK exception path (as the stage-1 tests showed, the LK then
 * starts the normal boot partition, even after a recovery-mode boot), with the kernel log, which
 * includes init's messages (printk.devkmsg=on), kept in pstore: console-ramoops and dmesg-ramoops.
 *
 * Before panicking it also clears the bootloader message command, if any, in the "para" partition
 * (the misc partition of this unit, see the TWRP fstab of the device tree): when Android crash-loops
 * with a test kernel, Rescue Party writes "boot-recovery" there and the LK then boots the recovery
 * partition at every start, which, with the test kernel in recovery, is a loop only SP Flash Tool
 * gets out of.
 *
 * Reboots requested by userspace are turned into the same panic: Android rebooting itself into
 * recovery (Rescue Party, init after a critical service crash) would otherwise start the test kernel
 * again and again, before the timed panic ever fires.
 *
 * "ac8257_panic_secs=N" on the kernel command line overrides the delay (0 disables the timed panic,
 * the reboot interception stays), so a test image can be given a longer window for a live adb session
 * without rebuilding the kernel.
 *
 * At run time, as root, in /sys/module/ac8257_bringup/parameters/:
 *   panic_secs        write N: timed panic N seconds from now (0 disarms it); read: last value set
 *   time_left         read only: seconds before the timed panic, 0 when disarmed
 *   intercept_reboot  1 (default): userspace reboots become a panic; 0: normal reboots ("adb reboot")
 * This program is free software; GPL v2.
 */
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/workqueue.h>

#define AC8257_MMC_DEVT		MKDEV(MMC_BLOCK_MAJOR, 0)	/* mmcblk0, the eMMC */
#define BCB_COMMAND_SIZE	32				/* struct bootloader_message.command */

static void ac8257_clear_bcb(void)
{
	struct disk_part_iter piter;
	struct hd_struct *part;
	struct block_device *bdev = NULL;
	struct buffer_head *bh;
	struct gendisk *disk;
	int partno;

	disk = get_gendisk(AC8257_MMC_DEVT, &partno);
	if (!disk)
		return;
	disk_part_iter_init(&piter, disk, 0);
	while ((part = disk_part_iter_next(&piter))) {
		if (part->info && !strcmp(part->info->volname, "para")) {
			bdev = bdget(part_devt(part));
			break;
		}
	}
	disk_part_iter_exit(&piter);
	put_disk(disk);
	if (!bdev || blkdev_get(bdev, FMODE_READ | FMODE_WRITE, NULL)) {
		pr_info("ac8257 bring-up: para partition not found\n");
		return;
	}
	bh = __bread(bdev, 0, 512);
	if (bh) {
		/* Only an AOSP command ("boot-recovery", "boot-bootloader"...): leave anything else. */
		if (!strncmp(bh->b_data, "boot-", 5)) {
			pr_info("ac8257 bring-up: clearing bootloader message command \"%.*s\"\n",
				BCB_COMMAND_SIZE, bh->b_data);
			memset(bh->b_data, 0, BCB_COMMAND_SIZE);
			mark_buffer_dirty(bh);
			sync_dirty_buffer(bh);
		}
		brelse(bh);
	}
	blkdev_put(bdev, FMODE_READ | FMODE_WRITE);
}

static int ac8257_panic_secs = CONFIG_AC8257_BRINGUP_PANIC_SECS;
static bool ac8257_intercept_reboot = true;
static bool ac8257_bringup_ready;		/* workqueues usable: set by the late initcall */
static unsigned long ac8257_panic_deadline;	/* jiffies; valid while the work is pending */

static int __init ac8257_panic_secs_setup(char *str)
{
	return kstrtoint(str, 0, &ac8257_panic_secs) == 0;
}
__setup("ac8257_panic_secs=", ac8257_panic_secs_setup);

static void ac8257_bringup_panic(struct work_struct *work)
{
	ac8257_clear_bcb();
	panic("ac8257 bring-up: timed panic (%d s), to keep the log in pstore",
	      ac8257_panic_secs);
}

static DECLARE_DELAYED_WORK(ac8257_bringup_work, ac8257_bringup_panic);

static void ac8257_bringup_arm(void)
{
	if (ac8257_panic_secs > 0) {
		ac8257_panic_deadline = jiffies + ac8257_panic_secs * HZ;
		mod_delayed_work(system_wq, &ac8257_bringup_work, ac8257_panic_secs * HZ);
		pr_info("ac8257 bring-up: timed panic in %d s\n", ac8257_panic_secs);
	} else {
		cancel_delayed_work(&ac8257_bringup_work);
		pr_info("ac8257 bring-up: timed panic disabled\n");
	}
}

#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "ac8257_bringup."

static int ac8257_panic_secs_set(const char *val, const struct kernel_param *kp)
{
	int secs, ret = kstrtoint(val, 0, &secs);

	if (ret)
		return ret;
	if (secs < 0 || secs > INT_MAX / HZ)
		return -EINVAL;
	ac8257_panic_secs = secs;
	if (ac8257_bringup_ready)	/* from the command line: the initcall arms it */
		ac8257_bringup_arm();
	return 0;
}

static const struct kernel_param_ops ac8257_panic_secs_ops = {
	.set = ac8257_panic_secs_set,
	.get = param_get_int,
};
module_param_cb(panic_secs, &ac8257_panic_secs_ops, &ac8257_panic_secs, 0644);

static int ac8257_time_left_get(char *buf, const struct kernel_param *kp)
{
	long left = 0;

	if (delayed_work_pending(&ac8257_bringup_work))
		left = max_t(long, 0, (long)(ac8257_panic_deadline - jiffies)) / HZ;
	return scnprintf(buf, PAGE_SIZE, "%ld\n", left);
}

static const struct kernel_param_ops ac8257_time_left_ops = {
	.get = ac8257_time_left_get,
};
module_param_cb(time_left, &ac8257_time_left_ops, NULL, 0444);

module_param_named(intercept_reboot, ac8257_intercept_reboot, bool, 0644);

static int ac8257_bringup_reboot_notify(struct notifier_block *nb, unsigned long code, void *cmd)
{
	if (!ac8257_intercept_reboot)
		return NOTIFY_DONE;
	ac8257_clear_bcb();
	panic("ac8257 bring-up: reboot requested (event %lu, \"%s\"), turned into a panic",
	      code, cmd ? (char *)cmd : "");
	return NOTIFY_DONE;
}

/* Also once early (the eMMC is up by then): a crash later on then ends in a normal boot as well. */
static void ac8257_bringup_early_clear(struct work_struct *work)
{
	ac8257_clear_bcb();
}

static DECLARE_DELAYED_WORK(ac8257_bringup_clear_work, ac8257_bringup_early_clear);

static struct notifier_block ac8257_bringup_reboot_nb = {
	.notifier_call = ac8257_bringup_reboot_notify,
	.priority = INT_MAX,
};

static int __init ac8257_bringup_panic_init(void)
{
	ac8257_bringup_ready = true;
	ac8257_bringup_arm();
	schedule_delayed_work(&ac8257_bringup_clear_work, 10 * HZ);
	register_reboot_notifier(&ac8257_bringup_reboot_nb);
	return 0;
}
late_initcall(ac8257_bringup_panic_init);
