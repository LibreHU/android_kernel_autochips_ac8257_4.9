#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/pstore.h>
#include <asm/io.h>
#include <asm/cacheflush.h>


typedef struct Arm2Log {
    /* console buffer*/
    u32       log_init;
    u32       log_write_over_flag;
    u32       log_reserve_size;
    u32       log_write_data_offset;
    u32       log_read_data_offset;
    u32       log_drop_count;
    u32       atc_log_read;
}TArm2Log, *PTARm2Log;

void *_pArm2Log = NULL;
static TArm2Log *_pCurArm2Header = NULL;
static void *_pArm2Reserve = NULL;
static TArm2Log *_pLastArm2Header = NULL;
static void *_pLastArm2Log = NULL;

static u32 cur_read_offset;
static u32 is_wake_up;
static u32 poll_wake_up;
static u32 last_dump_offset;
wait_queue_head_t arm2_log_wait;
static struct timer_list poll_timer;

#ifdef CONFIG_ARM64
extern void __inval_cache_range(unsigned long start, unsigned long end);
#endif


static void arm2_log_worker (unsigned long arg)
{
	u32 cur_log_offset = 0;

	if( _pCurArm2Header->log_init != 1) {
		mod_timer(&poll_timer, jiffies + msecs_to_jiffies(500));//500ms
		return;
	}

	cur_log_offset = _pCurArm2Header->log_write_data_offset;
	if (is_wake_up || (cur_log_offset != last_dump_offset)) {
		last_dump_offset = cur_log_offset;
		poll_wake_up = 1;
		wake_up(&arm2_log_wait);
	}

	mod_timer(&poll_timer, jiffies + msecs_to_jiffies(500));//500ms
}

static unsigned int arm2_log_poll(struct file *file,
					struct poll_table_struct *ptable)
{
	poll_wait(file, &arm2_log_wait, ptable);

	if( _pCurArm2Header->log_init != 1) {
		pr_err("init not ready  err:inlavid info--\n");
		return 0;
	}

	if (is_wake_up || poll_wake_up) {
		poll_wake_up = 0;
		is_wake_up = 0;
		return POLLIN | POLLRDNORM | POLLRDBAND;
	}
	return 0;
}


static ssize_t arm2_log_read(u32 addr, u32 size, char __user *buf)
{
	u32 temp_size;
	int ret;

	if (_pCurArm2Header->log_reserve_size - addr <= size) {
		temp_size = _pCurArm2Header->log_reserve_size - addr;
#ifdef CONFIG_ARM64
		__inval_cache_range((unsigned long)_pArm2Reserve + addr,
				    (unsigned long)_pArm2Reserve + addr + temp_size);
#else
		clean_dcache_area(_pArm2Reserve + addr, temp_size);
#endif
		if (copy_to_user(buf, _pArm2Reserve + addr, temp_size)) {
			pr_err("copy_to_user failed\n");
			ret = -EFAULT;
			goto out;
		}

#ifdef CONFIG_ARM64
		__inval_cache_range((unsigned long)_pArm2Reserve,
				    (unsigned long)_pArm2Reserve + size - temp_size);
#else
		clean_dcache_area(_pArm2Reserve, size - temp_size);
#endif
		if (copy_to_user(buf + temp_size, _pArm2Reserve, size - temp_size)) {
			pr_err("copy_to_user failed\n");
			ret = -EFAULT;
			goto out;
		}

		cur_read_offset = size - temp_size;
		_pCurArm2Header->log_read_data_offset = cur_read_offset;
	} else {
#ifdef CONFIG_ARM64
		__inval_cache_range((unsigned long)_pArm2Reserve + addr,
				    (unsigned long)_pArm2Reserve + addr + size);
#else
		clean_dcache_area(_pArm2Reserve + addr, size);
#endif
		if (copy_to_user(buf, _pArm2Reserve + addr, size)) {
			pr_err("copy_to_user failed\n");
			ret = -EFAULT;
			goto out;
		}

		cur_read_offset = addr + size;
		_pCurArm2Header->log_read_data_offset = cur_read_offset;
	}

	return size;
out:
	return ret;
}

static ssize_t cur_arm2_log_file_read(struct file *filp, char __user *buf,
						size_t count, loff_t *f_pos)
{
	int size = 0;
	u32 temp_read_offset = 0;
	u32 temp_write_offset = 0;
	int ret = 0;

	temp_write_offset = _pCurArm2Header->log_write_data_offset;
	temp_read_offset = _pCurArm2Header->log_read_data_offset;

	if (_pCurArm2Header->log_init != 1 ||
		_pCurArm2Header->log_write_data_offset > _pCurArm2Header->log_reserve_size) {
		pr_err("cur_arm2_log_file_read  err:inlavid info--\n");
		return -EINVAL;
	}

	if ((temp_write_offset - cur_read_offset > 0) ||
		((temp_write_offset < cur_read_offset) &&
		(_pCurArm2Header->log_reserve_size + temp_write_offset - temp_read_offset > 0))) {
		if (cur_read_offset < temp_write_offset) {
			size = temp_write_offset - cur_read_offset;
		} else {
			size = _pCurArm2Header->log_reserve_size + temp_write_offset - cur_read_offset;
		}

		if (size >= count) {
			size = count;
			is_wake_up = 1;
		}

		ret = arm2_log_read(cur_read_offset, size, buf);
		if (ret < 0) {
			pr_err("copy_to_user failed\n");
			ret = -EFAULT;
			goto out;
		}
	}

	_pCurArm2Header->atc_log_read = 1;
out:
	return ret;
}

static int arm2_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int arm2_log_file_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int last_arm2_log_show(struct seq_file *m, void *v)
{
	pr_info("last_arm2_log_show ++\n");
	if(m == NULL)
		return -EINVAL;
	if (_pLastArm2Header) {
		if (_pLastArm2Header->log_init != 1 ||
		    _pLastArm2Header->log_write_data_offset > _pLastArm2Header->log_reserve_size) {
			pr_err("last_arm2_log_show  err:inlavid info--\n");
			return -EINVAL;
		}
	} else {
		pr_err("last_arm2_log_show _pLastVissHeader NULL\n");
		return -EINVAL;
	}

	if (_pLastArm2Log) {
		pr_info("last_arm2_log_show data\n");
		seq_printf(m, "*************Arm2 log*************\n");
		seq_write(m, (void*)_pLastArm2Log, _pLastArm2Header->log_reserve_size);
	} else {
		seq_printf(m, "NO VALID DATA.\n");
	}

	pr_info("last_arm2_log_show --\n");
	return 0;
}

static int last_arm2_log_file_open(struct inode *inode, struct file *file)
{
	return single_open(file, last_arm2_log_show, inode->i_private);
}

static const struct file_operations cur_arm2_log_file_ops = {
    .owner = THIS_MODULE,
    .open = arm2_log_file_open,
    .read = cur_arm2_log_file_read,
    .poll = arm2_log_poll,
    .llseek = seq_lseek,
    .release = arm2_release,
};

static const struct file_operations last_arm2_log_file_ops = {
    .owner = THIS_MODULE,
    .open = last_arm2_log_file_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init arm2_log_init(void)
{
    struct device_node *node = NULL;
    void *bufp = NULL;
    unsigned int pra_dts[4]={0};
    unsigned int base_phys_addr, base_size;
    struct proc_dir_entry *cur_entry;
    struct proc_dir_entry *last_entry;

    node = of_find_compatible_node(NULL,NULL,"autochips,arm2-log");

    if (node) {
        if (of_property_read_u32_array(node,"reg",(u32 *)pra_dts,4)) {
            pr_err("[MTZ] faild get reserved addr and size in dts!\r\n");
            return 0;
        }
        base_phys_addr = (unsigned long)pra_dts[1];
        base_size = pra_dts[3];
        pr_err("base_phys_addr[0x%x],base_size[0x%x]\n", base_phys_addr, base_size);
        bufp = ioremap_wc(base_phys_addr,base_size);
        if (bufp){
            _pArm2Log = bufp;
            _pCurArm2Header = (TArm2Log*)_pArm2Log;
            _pArm2Reserve = (void *)(_pArm2Log + sizeof(TArm2Log));
            _pLastArm2Header = (TArm2Log*)(_pArm2Log +sizeof(TArm2Log) + _pCurArm2Header->log_reserve_size);
            _pLastArm2Log = (void *)((void*)_pLastArm2Header + sizeof(TArm2Log));
            pr_err("_pCurArm2Header[0x%p],_pArm2Reserve[0x%p] _pLastArm2Header[0x%p],_pLastArm2Log[0x%p]\n", _pCurArm2Header, _pArm2Reserve,_pLastArm2Header, _pLastArm2Log);

        }else {
            pr_err("ram_console: ioremap failed, [0x%x, 0x%x]\n", base_phys_addr, base_size);
            return 0;
        }
    } else {
	    pr_err("Can not find arm2-log node!");
        return 0;
    }

    cur_entry = proc_create("arm2_log", 0444, NULL, &cur_arm2_log_file_ops);
    last_entry = proc_create("last_arm2_log", 0444, NULL, &last_arm2_log_file_ops);
    if (!cur_entry) {
        pr_err("ram_console: failed to create proc arm2_log\n");
        return 0;
    }
    if (!last_entry) {
        pr_err("ram_console: failed to create proc last_arm2_log\n");
        return 0;
    }

	init_waitqueue_head(&arm2_log_wait);

	/* init the timer to update log info */
	init_timer(&poll_timer);
	poll_timer.expires = jiffies + msecs_to_jiffies(1000);  //1s
	poll_timer.function = &arm2_log_worker;
	poll_timer.data = 0;
	add_timer(&poll_timer);

    return 0;
}


module_init(arm2_log_init);

MODULE_DESCRIPTION("atc arm2_log driver");
MODULE_LICENSE(ATC_KERNEL_LINUX_LICENSE);
