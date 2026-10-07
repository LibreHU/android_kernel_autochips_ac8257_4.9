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
 * gets out of. This program is free software; GPL v2.
 */
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/init.h>
#include <linux/kernel.h>
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

static void ac8257_bringup_panic(struct work_struct *work)
{
	ac8257_clear_bcb();
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
