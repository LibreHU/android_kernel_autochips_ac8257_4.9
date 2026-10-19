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
/*
 *Department: ATC-SD-SD2
 *
 *Aurthor: yezhiqiang(atc0064)
 *
 *Date: 2017-07-31
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <asm/io.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/debugfs.h>
#include <nr_hal.h>
#include "nr_log.h"
#include "nr_if.h"
#include "nr_reg.h"


#define ATC_KERNEL_LINUX_LICENSE     "GPL"
#define ATC_NR_DRIVER_INFO	"ATC noise reduce driver"
#if 0
/*set bypass nr begin*/
static ssize_t nr_bypass_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	return 0;
}

static ssize_t nr_bypass_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned int ret = 0, fgBypass;

	NR_LOG(NR_LOG_LVL_INFO, "	Parameters: [fgBypass]\n");
	NR_LOG(NR_LOG_LVL_INFO, "		fgBypass:\n");
	NR_LOG(NR_LOG_LVL_INFO, "			0:enable NR; 1:bypass NR \n");

	ret = sscanf(buf, "%d", (unsigned int *)&fgBypass);
	if (ret < 0) {
		NR_LOG(NR_LOG_LVL_ERR, "nr_bypass_store sscanf error!\n");
		return -EINVAL;
	}

	NrBypass(fgBypass);

	return count;

}
static DEVICE_ATTR(nr4bypass, S_IWUSR | S_IRUGO, nr_bypass_show, nr_bypass_store);
/*set bypass nr end*/

/*set enable 3dnr begin*/
static ssize_t nr_en3d_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	return 0;
}

static ssize_t nr_en3d_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned int ret = 0, fgBypass;

	NR_LOG(NR_LOG_LVL_INFO, "	Parameters: [fgBypass]\n");
	NR_LOG(NR_LOG_LVL_INFO, "		fgBypass:\n");
	NR_LOG(NR_LOG_LVL_INFO, "			0:disable 3dNR; 1:enable 3dNR \n");

	ret = sscanf(buf, "%d", (unsigned int *)&fgBypass);
	if (ret < 0) {
		NR_LOG(NR_LOG_LVL_ERR, "nr_en3d_store sscanf error!\n");
		return -EINVAL;
	}

	NrEnable3dFunc(fgBypass);

	return count;

}
static DEVICE_ATTR(nr4en3d, S_IWUSR | S_IRUGO, nr_en3d_show, nr_en3d_store);
/*set enable 3dnr end*/

/*set swap mode begin*/
static ssize_t nr_swap_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	return 0;
}

static ssize_t nr_swap_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned int ret = 0, swapMode;

	NR_LOG(NR_LOG_LVL_INFO, "	Parameters: [swapMode]\n");
	NR_LOG(NR_LOG_LVL_INFO, "		swapMode:\n");
	NR_LOG(NR_LOG_LVL_INFO, "			level:[0~6] \n");

	ret = sscanf(buf, "%d", (unsigned int *)&swapMode);
	if (ret < 0) {
		NR_LOG(NR_LOG_LVL_ERR, "nr_swap_store sscanf error!\n");
		return -EINVAL;
	}

	NrSetSwapMode(swapMode);

	return count;

}
static DEVICE_ATTR(nr4swapmode, S_IWUSR | S_IRUGO, nr_swap_show, nr_swap_store);
/*set swap mode end*/

/*set nr level begin*/
static ssize_t nr_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	return 0;
}

static ssize_t nr_level_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned int ret = 0, u4Strength, u4FNRStrength, u4MNRStrength, u4BNRStrength;

	NR_LOG(NR_LOG_LVL_INFO, "	Parameters: [u4Strength, u4FNRStrength, u4MNRStrength, u4BNRStrength]\n");
	NR_LOG(NR_LOG_LVL_INFO, "		all value:\n");
	NR_LOG(NR_LOG_LVL_INFO, "			level:[0~3] \n");

	ret = sscanf(buf, "%d %d %d %d",
		(unsigned int *)&u4Strength, (unsigned int *)&u4FNRStrength, (unsigned int *)&u4MNRStrength, (unsigned int *)&u4BNRStrength);
	if (ret < 0) {
		NR_LOG(NR_LOG_LVL_ERR, "nr_level_store sscanf error!\n");
		return -EINVAL;
	}

	NrSetLevel(u4Strength, u4FNRStrength, u4MNRStrength, u4BNRStrength);

	return count;

}
static DEVICE_ATTR(nr4level, S_IWUSR | S_IRUGO, nr_level_show, nr_level_store);
/*set nr level end*/
#endif


/* ===========================================================================================
 * Using gpio to control cvbs backcar on/off.
 * Put two statements below to routine which need wake up completion.
 * extern struct completion nr_gpio_comp;
 * complete(&nr_gpio_comp);
 */
bool nr_stop_bc_gpio = false;

static int nr_gpio;
static unsigned bc_on_time  = 1500; /* ms */
static unsigned bc_off_time = 50;   /* ms */
DECLARE_COMPLETION(nr_gpio_comp);
void msleep(unsigned int msecs);

static void gpio_on_off(unsigned gpio_id, unsigned on_ms, unsigned off_ms, unsigned cnt)
{
	unsigned i = 0;
	for (i = 0; i < cnt; i++) {
		gpio_set_value(gpio_id, 1);
		msleep(on_ms);
		NR_LOG(NR_LOG_LVL_INFO, "process %i (%s) going to sleep\n", current->pid, current->comm);
		NR_LOG(NR_LOG_LVL_INFO, "before nr_gpio_comp = %u\n", nr_gpio_comp.done);
		/* if want get a confirmation before close backcar using following statement instead.
		 * wait_for_completion_interruptible(&nr_gpio_comp);*/
		wait_for_completion_interruptible_timeout(&nr_gpio_comp, msecs_to_jiffies(1000));
		NR_LOG(NR_LOG_LVL_INFO, "after  nr_gpio_comp = %u\n", nr_gpio_comp.done);

		gpio_set_value(gpio_id, 0);
		msleep(off_ms);
	}
}

static int gpio_thread_fn(void *gpio)
{
	unsigned id = *((unsigned *) gpio);
	gpio_direction_output(id, 1);
	for(;;) {
		gpio_on_off(id, bc_on_time, bc_off_time, 1);
		if (kthread_should_stop()) {
			break;
		}
	}
	return 0;
}

static ssize_t nr_gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
	{
	static struct task_struct *gpio_thread;
	char *info = "echo requirement: address value mask.\n"
		"If value and mask both equal zero then just read address val.\n";

	/* cvbs backcar button(GPIO 10) gpio number just for Q.
	 * cat /sys/devices/platform/1000b000.pinctrl/mt_gpio to get GPIO base number i.e. 0x149 */
	int gpio = 0x149 + 10;

	if (gpio_thread) {
		kthread_stop(gpio_thread);
		gpio_thread = NULL;
		return sprintf(buf, "%s", "stop gpio thread!\n");
	}

	gpio_thread = kthread_run(gpio_thread_fn, &gpio, "nr-gpio-thread");
	if (!IS_ERR(gpio_thread)) {
		NR_LOG(NR_LOG_LVL_INFO, "[%s]gpio_thread kthread_run success = 0x%p \n", __func__, gpio_thread);
		return sprintf(buf, "%s", "gpio thread run success!\n");
	} else {
		NR_LOG(NR_LOG_LVL_INFO, "[%s]gpio_thread kthread_run failed, errno = %d\n", __func__, gpio_thread);
		gpio_thread = NULL;
	}

	return sprintf(buf, "%s", info);
}

static ssize_t nr_gpio_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	unsigned int ret = 0;
	ret = sscanf(buf, "%u,%u", &bc_on_time, &bc_off_time);
	NR_LOG(NR_LOG_LVL_INFO, "input data is backcar on time:%ums, backcar off time:%ums",
		bc_on_time, bc_off_time);

	return count;
}

static DEVICE_ATTR(nr_bc_gpio, S_IWUSR | S_IRUGO, nr_gpio_show, nr_gpio_store);
/* =========================================================================================== */


/*set dump nr begin*/
static ssize_t nr_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t nr_dump_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int ret = 0;
	int num = 0;
	char string[20] = "";

	ret = sscanf(buf, "%10s %d", string, (int *)&num);
	if (ret < 0) {
		NR_LOG(NR_LOG_LVL_ERR, "nr_dump_store sscanf error!\n");
		return -EINVAL;
	}

	if (strcmp(string, "dumpBuffer") == 0) {
		NR_LOG(NR_LOG_LVL_INFO, "nr_dump_store: [dumpBuffer][%d] \n", num);
		if (NrDumpBuffer(num)) {
			NR_LOG(NR_LOG_LVL_ERR,"nr_dump_store NrDumpBuffer err!\n");
			return -EINVAL;
		}
	}
	else if(strcmp(string, "dumpReg") == 0) {
		NR_LOG(NR_LOG_LVL_INFO, "nr_dump_store: [dumpReg]\n");
		if (NrDumpReg()) {
			NR_LOG(NR_LOG_LVL_ERR,"nr_dump_store NrDumpReg err!\n");
			return -EINVAL;
		}
	}
	else{
		NR_LOG(NR_LOG_LVL_ERR, "nr_dump_store cmd invalid!\n");
	}

	return count;

}
static DEVICE_ATTR(nr4dump, S_IWUSR | S_IRUGO, nr_dump_show, nr_dump_store);
/*set dump nr end*/

static long nr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	NR_LOG(NR_LOG_LVL_INFO, "nr_ioctl not be implemented \n");
	/*to do*/

	/*****/
	return 0;
}

struct file_operations const nr_fops = {
	.unlocked_ioctl = nr_ioctl,
};

static struct miscdevice nr_dev = {
	MISC_DYNAMIC_MINOR,
	"nr",
	&nr_fops
};

#ifndef NO_IRQ
#define NO_IRQ 0
#endif
extern void __iomem* nr_reg;
extern void __iomem* clock_reg;
extern void __iomem* vdpsys_reg;
unsigned int IRQVECTOR = 0;
static int nr_probe(struct platform_device *pdev)
{
	unsigned int ret = -EINVAL;
	struct device_node *node;
	struct device_node *nd = pdev->dev.of_node;
	unsigned int property[4];
	unsigned int base_pa, base_va;
	unsigned int size;

	NR_LOG(NR_LOG_LVL_INFO, "nr_probe enter--->\n");

	nr_reg = of_iomap(nd, 0);
	if (!nr_reg) {
		NR_LOG(NR_LOG_LVL_ERR, "get NR base address nr_reg = 0x%lx \r\n",(unsigned int)nr_reg);
		return -1;
	}
	NR_LOG(NR_LOG_LVL_INFO, "get NR reg base address nr_reg = 0x%lx pa= 0x%lx\r\n",(unsigned int)nr_reg, (unsigned int)__pa(nr_reg));

	clock_reg = of_iomap(nd, 1);
	if (!clock_reg) {
		NR_LOG(NR_LOG_LVL_ERR, "get NR clock base address clock_reg = 0x%lx \r\n",(unsigned int)clock_reg);
		return -1;
	}
	NR_LOG(NR_LOG_LVL_INFO, "get NR clock base address clock_reg = 0x%lx pa=0x%lx\r\n",(unsigned int)clock_reg, (unsigned int)__pa(clock_reg));

	vdpsys_reg = of_iomap(nd, 2);
	if (!vdpsys_reg) {
		NR_LOG(NR_LOG_LVL_ERR, "get vdpsys base address vdpsys_reg = 0x%lx \r\n",(unsigned int)vdpsys_reg);
		return -1;
	}
	NR_LOG(NR_LOG_LVL_INFO, "get vdpsys base address vdpsys_reg = 0x%lx pa=0x%lx\r\n",(unsigned int)vdpsys_reg, (unsigned int)__pa(vdpsys_reg));

	IRQVECTOR = irq_of_parse_and_map(nd, 0);
	if (IRQVECTOR == NO_IRQ) {
		NR_LOG(NR_LOG_LVL_ERR, "get nr irq failed = %d \r\n", IRQVECTOR);
		return -1;
	}
	NR_LOG(NR_LOG_LVL_INFO, "get nr irq = %d \r\n", IRQVECTOR);

	node = of_find_compatible_node(NULL,NULL,"autochips,nr");
	if (node) {
		base_va = (unsigned long)of_iomap(node, 0);
		if (0 == base_va) {
			NR_LOG(NR_LOG_LVL_ERR, "noisereduce buffer of_iomap fail\r\n");
			goto err;
		}
		if (of_property_read_u32_array(node, "reg", (u32 *)property, 4)) {
			NR_LOG(NR_LOG_LVL_ERR, "get noisereduce buffer reserved memory node reg info fail\r\n");
			goto err;
		}
		base_pa = property[1];
		size = property[3];
		NR_LOG(NR_LOG_LVL_INFO, "noisereduce buffer base va:0x%lx, size:0x%lx, pa:0x%lx\n", base_va, size, base_pa);
		setNrReservemem(base_pa, base_va, size);
	} else {
		NR_LOG(NR_LOG_LVL_ERR, "can not find noisereduce buffer reserved memory node!!\r\n");
		goto err;
	}

	ret = misc_register(&nr_dev);
	if (ret) {
		NR_LOG(NR_LOG_LVL_ERR, "nr_probe: misc_register error %d\r\n", ret);
		goto err;
	}

	ret = device_create_file(nr_dev.this_device, &dev_attr_nr4dump);
	if (ret) {
		NR_LOG(NR_LOG_LVL_ERR, "cannot create dump dev file %d\n", ret);
		goto err;
	}

	ret = device_create_file(nr_dev.this_device, &dev_attr_nr_bc_gpio);
	if (ret) {
		NR_LOG(NR_LOG_LVL_ERR, "cannot create nr_bc_gpio dev file %d\n", ret);
		goto err;
	}
#if 0
	ret = os_device_create_file(nr_dev.this_device, &dev_attr_nr4bypass);
	if (ret) {
		NR_LOG(NR_LOG_LVL_ERR, "cannot create bypass dev file %d\r\n", ret);
		goto err;
	}

	ret = os_device_create_file(nr_dev.this_device, &dev_attr_nr4en3d);
	if (ret) {
		NR_LOG(NR_LOG_LVL_ERR, "cannot create enable 3d dev file %d\r\n", ret);
		goto err;
	}

	ret = os_device_create_file(nr_dev.this_device, &dev_attr_nr4swapmode);
	if (ret) {
		NR_LOG(NR_LOG_LVL_ERR, "cannot create swap dev file %d\r\n", ret);
		goto err;
	}

	ret = os_device_create_file(nr_dev.this_device, &dev_attr_nr4level);
	if (ret) {
		NR_LOG(NR_LOG_LVL_ERR, "cannot create swap dev file %d\r\n", ret);
		goto err;
	}
#endif

	//Register NR interrupt
	//NrIsrInit();

        NR_LOG(NR_LOG_LVL_INFO, "nr_probe leave--->\n");
err:
	return ret;
}

static int nr_remove(struct platform_device *pdev)
{
	NR_LOG(NR_LOG_LVL_INFO, "nr_remove<---\n");
#if 0
	os_device_remove_file(nr_dev.this_device, &dev_attr_nr4bypass);
	os_device_remove_file(nr_dev.this_device, &dev_attr_nr4en3d);
	os_device_remove_file(nr_dev.this_device, &dev_attr_nr4swapmode);
	os_device_remove_file(nr_dev.this_device, &dev_attr_nr4level);
#endif

	device_remove_file(nr_dev.this_device, &dev_attr_nr4dump);
	misc_deregister(&nr_dev);

	return 0;
}

static const struct of_device_id nr_of_ids[] = {
	{.compatible = "Autochips,noisereduce",},
	{}
};

static int nr_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	NR_LOG(NR_LOG_LVL_INFO, "nr_suspend before, clock_reg[1000,00C0]=0X:%X\n", u4ReadReg(clock_reg));
	// vWriteRegMsk(clock_reg, 0, (1 << 9) | (1 << 14)); //disable NR[bit 14] and DI[bit 9] clock
	_NrCloseClk();
	NR_LOG(NR_LOG_LVL_INFO, "nr_suspend after,  clock_reg[1000,00C0]=0X:%X\n", u4ReadReg(clock_reg));
	return 0;
}

static int nr_resume(struct platform_device *pdev)
{
	NR_LOG(NR_LOG_LVL_INFO, "nr_resume done nothing, clock_reg[1000,00C0]=%ux--->\n", u4ReadReg(clock_reg));
	return 0;
}

static struct platform_driver nr_plt_drv = {
	.driver = {
		   .name = "Autochips-noisereduce",
		   .owner = THIS_MODULE,
		   .of_match_table = nr_of_ids,
		   },
	.probe = nr_probe,
	.remove = nr_remove,
	.suspend = nr_suspend,
	.resume = nr_resume,
};


static struct dentry *nr_dbgfs;

static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{

	static char *s = "--nr debug not implement yet!--\n";
	/* following statement similar to
	 * return copy_to_user(ubuf, s, strlen(s)); */
	return simple_read_from_buffer(ubuf, count, ppos, s, strlen(s));
}

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t debug_write(struct file *file,
			   const char __user *ubuf, size_t count, loff_t *ppos)
{

	const int debug_bufmax = 512 - 1;
	size_t ret;
	char cmd_buffer[512];

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&cmd_buffer, ubuf, count))
		return -EFAULT;

	cmd_buffer[count] = 0;

	// process_dbg_cmd(cmd_buffer);

	return ret;

}

static const struct file_operations debug_fops = {
	.read = debug_read,
	.write = debug_write,
	.open = debug_open,
};

static int __init nr_init(void)
{
	int ret;

	NR_LOG(NR_LOG_LVL_INFO, "[NR]:nr_init--->\n");
	ret = platform_driver_register(&nr_plt_drv);
	if (ret)
		NR_LOG(NR_LOG_LVL_ERR, "[NR]: %s: register  driver failed\n", __func__);

	nr_dbgfs = debugfs_create_file("nr-debug", S_IFREG | 0444, NULL, (void *)0, &debug_fops);

	NrKernelThreadInit();

        return 0;
}

static void __exit nr_exit(void)
{
	NR_LOG(NR_LOG_LVL_INFO, "nr_exit<---\n");

	platform_driver_unregister(&nr_plt_drv);
}
module_init(nr_init);
module_exit(nr_exit);

MODULE_DESCRIPTION(ATC_NR_DRIVER_INFO);
MODULE_LICENSE(ATC_KERNEL_LINUX_LICENSE);

