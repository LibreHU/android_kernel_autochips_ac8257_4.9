/*
 *Department: ATC-SD-SD1
 *
 *Aurthor: Mingxu Wang(atc0027)
 *
 *Date: 2017-08-01
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
//#include <linux/clk-private.h>
#include <linux/pinctrl/consumer.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/proc_fs.h>	/* proc file use */

#include "wch_log.h"
#include "wch_if.h"
#include "wch_priv.h"

#define ATC_KERNEL_LINUX_LICENSE     "GPL"
#define ATC_WCH_DRIVER_INFO	"ATC write channel driver"

#define MMISC_MODE_NAME              "WCH"
#define MMISC_VER_MAJOR              10
#define MMISC_VER_MINOR              00
#define MMISC_VER_REV                00

#define WCH_IOMAP_REG_NUM			(3 * WCH_NUM)

#define HAL_WRITE32(reg, value)		writel((value), (reg))
#define HAL_READ32(reg)				readl(reg)

#define WCH_READ32(base, offset)		HAL_READ32(((base) + (offset)))
#define WCH_WRITE32(base, offset, value)	HAL_WRITE32(((base) + (offset)), (value))

static u32 _dump_buf_idx_cnt[WCH_NUM] = {0};
static void __iomem *_wch_reg_base[WCH_IOMAP_REG_NUM] = {NULL};

static long wch_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	WCH_LOG(WCH_LOG_LVL_INFO, "wch_ioctl not be implemented \n");
	/*to do*/

	/*****/
	return 0;
}

struct file_operations const wch_fops = {
	.unlocked_ioctl = wch_ioctl,
};

static struct miscdevice wch_dev = {
	MISC_DYNAMIC_MINOR,
	"wch",
	&wch_fops
};


#if WCH_DUMP_BUFFER_ATTR
static ssize_t wch_dumpbuffer_show(struct device *dev, struct device_attribute *attr, char *buf)
{

/* ssize_t size; */
/* size = sprintf(buf, "Wch Log Level %d\n", (int); */

	return 0;
}

bool wch_dump_buffer(WCH_ID_E wch_id, uintptr_t y_addr, uintptr_t c_addr, u32 width, u32 hight)
{
	u8 dump_file[256];
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos;
	s32 ret = 0, y_size = 0, c_size = 0;
	void *virt_addr = NULL;

	y_size = width * hight;
	c_size = y_size / 2;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	/* write y data begin */
	sprintf(dump_file, "/storage/ext_sdcard2/y%02d.raw", _dump_buf_idx_cnt[wch_id]);
	WCH_LOG(WCH_LOG_LVL_INFO, "[wch_dump_buffer] Dump File:%s for wch%d\r\n",
		dump_file, (int)wch_id);

	fp = filp_open(dump_file, O_CREAT | O_RDWR, 0777);
	if (IS_ERR(fp)) {
		ret = PTR_ERR(fp);
		WCH_LOG(WCH_LOG_LVL_ERR, "[wch_dump_buffer] open file %s error wch%d return %d\n",
			dump_file, (int)wch_id, ret);
		return false;
	}

	pos = 0;
	virt_addr = ioremap((phys_addr_t)(y_addr), y_size);
	ret = vfs_write(fp, (char __user *)virt_addr, y_size, &pos);
	WCH_LOG(WCH_LOG_LVL_INFO, "[wch_dump_buffer] write file map addr 0x%lx, 0x%lx size %d, pos %d, ret %d\r\n",
		(y_addr), (unsigned long)virt_addr, (int)y_size, (int)pos, ret);
	iounmap((void __iomem *)virt_addr);

	filp_close(fp, NULL);
	/* write y data end */

	/* write cbcr data begin */
	sprintf(dump_file, "/storage/ext_sdcard2/cbcr%02d.raw", _dump_buf_idx_cnt[wch_id]);
	WCH_LOG(WCH_LOG_LVL_INFO, "[wch_dump_buffer] Dump File:%s for wch%d\r\n", dump_file,
		(int)wch_id);

	fp = filp_open(dump_file, O_CREAT | O_RDWR, 0777);
	if (IS_ERR(fp)) {
		ret = PTR_ERR(fp);
		WCH_LOG(WCH_LOG_LVL_ERR, "[wch_dump_buffer] open file %s error wch%d return %d\n",
			dump_file, (int)wch_id, ret);
		return false;
	}

	pos = 0;
	virt_addr = ioremap((phys_addr_t)(c_addr), c_size);
	ret = vfs_write(fp, (char __user *)virt_addr, c_size, &pos);
	WCH_LOG(WCH_LOG_LVL_INFO, "[wch_dump_buffer] write file addr 0x%lx, 0x%lx size %d, pos %d, ret: %d\r\n",
		(c_addr), (unsigned long)virt_addr, (int)c_size, (int)pos, ret);
	/* write cbcr data */
	iounmap((void __iomem *)virt_addr);
	filp_close(fp, NULL);
	set_fs(old_fs);

	return true;
}

static ssize_t wch_dumpbuffer_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	u32 buf_idx = 0;
	u32 width = 0;
	u32 hight = 0;
	u32 frame_num_to_dump = 0;
	u32 frame_cnt = 0;
	WCH_ID_E wch_id = 0;
	int ret;
	u32 dump_buf_cnt = 0;
	u32 buf_cnt = 0;
	uintptr_t y_addr = 0;
	uintptr_t c_addr = 0;

	WCH_LOG(WCH_LOG_LVL_INFO, "wch_dumpbuffer_store enter!\n");

	ret = sscanf(buf, "%d %d %d %d", (int *)&wch_id, (int *)&width, (int *)&hight, (int *)&frame_num_to_dump);
	if (ret < 0) {
		pr_err("sscanf error!\n");
		return -EINVAL;
	}
	WCH_LOG(WCH_LOG_LVL_INFO, "wch_dumpbuffer_store width: %d, hight: %d, frame_num_to_dump: %d!\n",
		width, hight, frame_num_to_dump);

	if (wch_id != WCH_VDI) {
		WCH_LOG(WCH_LOG_LVL_INFO, "wch_dumpbuffer_store wch_id error: %d!\n", wch_id);
		return -EINVAL;
	}

	dump_buf_cnt = wch_hw_get_dump_frame_cnt(wch_id);
	buf_cnt = wch_hw_get_buf_cnt(wch_id);

	for (frame_cnt = 0; frame_cnt < frame_num_to_dump; frame_cnt++) {
		if (dump_buf_cnt > 0) {
			buf_idx = _dump_buf_idx_cnt[wch_id] % buf_cnt;
			WCH_LOG(WCH_LOG_LVL_DBG, "wch_dumpbuffer_store wch id %d, buffer id %d\r\n",
				(int)wch_id, (int)buf_idx);
			wch_hw_get_yc_addr(wch_id, &y_addr, &c_addr, buf_idx);
			/* dump begin */
			WCH_LOG(WCH_LOG_LVL_ERR, "wch_dumpbuffer_store dumpbuffer!\n");
			wch_dump_buffer(wch_id, y_addr,  c_addr, width, hight);
			/* dump end */
			_dump_buf_idx_cnt[wch_id]++;
			dump_buf_cnt--;
		}
	}

	return count;
}

static DEVICE_ATTR(wch_dump_buffer, S_IWUSR | S_IRUGO, wch_dumpbuffer_show, wch_dumpbuffer_store);

#endif

static ssize_t wch_loglevel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t size;

	size = sprintf(buf, "Wch Log Level %d\n", (int)g_wch_dbg_lvl);

	return size;
}

static ssize_t wch_loglevel_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int ret;

	ret = sscanf(buf, "%d", (int *)&g_wch_dbg_lvl);
	if (ret < 0) {
		pr_err("sscanf error!\n");
		return -EINVAL;
	}
	return (ssize_t)count;
}

static DEVICE_ATTR(wchloglevel, S_IWUSR | S_IRUGO, wch_loglevel_show, wch_loglevel_store);

static ssize_t wch_write_reg_proc(struct file *pFile, const char __user *pBuffer, size_t Count, loff_t *p_off)
{
	char RegBuf[64];
	int CopyBufSize = (Count < (sizeof(RegBuf) - 1)) ? (Count) : (sizeof(RegBuf) - 1);
	int Addr = 0;
	int Data = 0;
	int wch_id = 0;
	int ret_cnt = 0;
	void __iomem *base = NULL;

	WCH_LOG(WCH_LOG_LVL_INFO, "wch_write_reg_proc\r\n");

	if (copy_from_user(RegBuf, pBuffer, CopyBufSize)) {
		WCH_LOG(WCH_LOG_LVL_ERR, "copy_from_user() fail.\r\n");
		return -EFAULT;
	}
	WCH_LOG(WCH_LOG_LVL_INFO, "copy_from_user() RegBuf: %s\r\n", RegBuf);

	ret_cnt = sscanf(RegBuf, "%d %d %d", (int *)&wch_id, (int *)&Addr, (int *)&Data);
	if ((wch_id > WCH_DI) ||
		(ret_cnt < 2)) {
		WCH_LOG(WCH_LOG_LVL_ERR, "parameter error: wch_id: %d, ret_cnt: %d\r\n",
			wch_id, ret_cnt);
		return ((ssize_t)Count);
	}

	base = _wch_reg_base[wch_id];

	if (3 == ret_cnt) {
		WCH_LOG(WCH_LOG_LVL_INFO, "Write => wch_id: %d\r\n", wch_id);
		WCH_WRITE32(base, (Addr), Data);
		WCH_LOG(WCH_LOG_LVL_INFO, "Write => Addr: 0x%08X, Write Data: 0x%08X. Read Data: 0x%08X.\r\n",
			(unsigned int)(Addr), Data, WCH_READ32(base, Addr));
	} else {
		if (0xFF == Addr) {
			u32 i = 0;
			WCH_LOG(WCH_LOG_LVL_INFO, "Read => wch_id: %d.\r\n", wch_id);
			for(i=0; i<=0x88; i+=4) {
				WCH_LOG(WCH_LOG_LVL_INFO, "Addr: 0x%08X value: 0x%08X.\r\n",
					(unsigned int)(base + i), WCH_READ32(base, i));
			}
		} else {
			WCH_LOG(WCH_LOG_LVL_INFO, "Read => wch_id: %d, Addr: 0x%08X, Read Data: 0x%08X.\r\n",
				wch_id, (unsigned int)(Addr), WCH_READ32(base, Addr));
		}
	}

	return ((ssize_t)Count);
}

static const struct file_operations wch_proc_fops = {
	.write = wch_write_reg_proc,
};

static ssize_t wch_misc_proc(struct file *pFile, const char __user *pBuffer, size_t Count, loff_t *p_off)
{
	char RegBuf[64];
	char cmd[20] = "";
	int CopyBufSize = (Count < (sizeof(RegBuf) - 1)) ? (Count) : (sizeof(RegBuf) - 1);
	int Data = 0;
	int wch_id = 0;
	int ret_cnt = 0;

	WCH_LOG(WCH_LOG_LVL_INFO, "wch_misc_proc\r\n");

	if (copy_from_user(RegBuf, pBuffer, CopyBufSize)) {
		WCH_LOG(WCH_LOG_LVL_ERR, "copy_from_user() fail.\r\n");
		return -EFAULT;
	}
	WCH_LOG(WCH_LOG_LVL_INFO, "copy_from_user() RegBuf: %s\r\n", RegBuf);

	ret_cnt = sscanf(RegBuf, "%10s %d %d", cmd, (int *)&wch_id, (int *)&Data);
	if ((wch_id > WCH_DI) || (ret_cnt < 2)) {
		WCH_LOG(WCH_LOG_LVL_INFO, "Error Cmd: %s\r\n", RegBuf);
		return ((ssize_t)Count);
	}
	if (0 == strcmp(cmd, "wch_enable")) {
		wch_hw_enable(wch_id);
		WCH_LOG(WCH_LOG_LVL_INFO, "wch(%d) enable\r\n", wch_id);
	} else if (0 == strcmp(cmd, "wch_disable")) {
		wch_hw_disable(wch_id);
		WCH_LOG(WCH_LOG_LVL_INFO, "wch(%d) disable\r\n", wch_id);
	} else if (0 == strcmp(cmd, "garbage_log")) {
		if (2 == ret_cnt) {
			wch_hw_set_garbage_log(Data);
			WCH_LOG(WCH_LOG_LVL_INFO, "set garbage log (%d)\r\n", Data);
		}
	} else {
		WCH_LOG(WCH_LOG_LVL_INFO, "the input parameter is not suitable\n");
	}

	return ((ssize_t)Count);
}

static const struct file_operations wch_misc_fops = {
	.write = wch_misc_proc,
};

static ssize_t wch_IntrruptInterval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t size = 0;

	/*size = sprintf(buf, "Wch interrupt interval %d\n", g_show_isr_interval);*/

	return size;
}

static ssize_t wch_IntrruptInterval_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
#if 0
	int ret;
	char *buffer = "done";
	ret = sscanf(buf, "%d %s", (int *)&g_show_isr_interval, buffer);
	if (ret < 0) {
		pr_err("sscanf error!\n");
		return -EINVAL;
	}
#endif
	return (ssize_t)count;
}

static DEVICE_ATTR(wchInterruptInterval, S_IWUSR | S_IRUGO, wch_IntrruptInterval_show, wch_IntrruptInterval_store);


static int wch_get_reserve_mem(void)
{
	int ret = -EINVAL;
	u32 regs[4] = {0};
	struct device_node *node_reserve = NULL;

	node_reserve = of_find_compatible_node(NULL, NULL, "autochips,wch-vdi");
	if (!node_reserve) {
		WCH_LOG(WCH_LOG_LVL_ERR, "of_find_compatible_node atc-wchvdi Fail\r\n");
		return -1;
	}

	ret = of_property_read_u32_array(node_reserve, "reg", regs, 4);
	if (0 != ret) {
		WCH_LOG(WCH_LOG_LVL_ERR, "of_property_read_u32_array reg Fail\r\n");
		return ret;
	}

	WCH_LOG(WCH_LOG_LVL_INFO, "wch reserve base = %x,  reserve size = %x\n", regs[1], regs[3]);

	if ((0 == regs[1]) || (0 == regs[3])) {
		WCH_LOG(WCH_LOG_LVL_ERR, "get memory error\r\n");
		return -1;
	}
	wch_hw_set_reserve_base(regs[1]);

	return 0;
}

static int wch_get_reg_base(struct device_node *node)
{
	s32 i = 0;

	for (i = 0; i < WCH_NUM; i++) {
	/*get clk rst base*/
		_wch_reg_base[WCH_NUM*2 + i] = (void __iomem *)of_iomap(node, i + WCH_NUM);
		if (NULL == _wch_reg_base[WCH_NUM*2 + i]) {
			WCH_LOG(WCH_LOG_LVL_ERR, "get clk_rst base failed \r\n");
			return -1;
		}
		WCH_LOG(WCH_LOG_LVL_INFO, "get clk_rst base ok: %p \r\n", _wch_reg_base[WCH_NUM*2 + i]);
		wch_hw_set_clk_rst_base(i, _wch_reg_base[WCH_NUM*2 + i]);
		wch_hw_set_irq_clr_base(i, _wch_reg_base[WCH_NUM*2 + i]);
		/*get reg base*/
		_wch_reg_base[i] = (void __iomem *)of_iomap(node, i);
		if (NULL == _wch_reg_base[i]) {
			WCH_LOG(WCH_LOG_LVL_ERR, "get %s reg base address failed = %p \r\n",
				g_wch_name[i], _wch_reg_base[i]);
			return -1;
		}
		WCH_LOG(WCH_LOG_LVL_INFO, "get %s reg base address = %p \r\n", g_wch_name[i], _wch_reg_base[i]);
		wch_hw_set_reg_base(i, (void *)_wch_reg_base[i]);
	}

	return 0;
}

#ifndef NO_IRQ
#define NO_IRQ 0
#endif

static int wch_get_irq_vector(struct device_node *node)
{
	s32 i = 0;
	unsigned int irq_vector = 0;

	for (i = 0; i < WCH_NUM; i++) {
		irq_vector = irq_of_parse_and_map(node, i);
		if (irq_vector == NO_IRQ) {
			WCH_LOG(WCH_LOG_LVL_ERR, "get %s irq failed = %d \r\n", g_wch_name[i], irq_vector);
			return -1;
		}
		wch_hw_set_irq(i, irq_vector);

		WCH_LOG(WCH_LOG_LVL_INFO, "get %s irq = %d \r\n", g_wch_name[i], irq_vector);
	}

	return 0;
}

static int wch_probe(struct platform_device *pdev)
{
	int ret = -EINVAL;
	s32 i = 0;
	struct device_node *node = pdev->dev.of_node;

	WCH_LOG(WCH_LOG_LVL_INFO, "enter\n");

	if (0 != wch_get_reg_base(node)) {
		goto err;
	}

	if (0 != wch_get_irq_vector(node)) {
		goto err;
	}


	if (0 != wch_get_reserve_mem()) {
		goto err;
	}

	ret = misc_register(&wch_dev);
	if (ret)
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_init: misc_register error %d\r\n", ret);

#if WCH_DUMP_BUFFER_ATTR
	ret = device_create_file(wch_dev.this_device, &dev_attr_wch_dump_buffer);
	if (ret)
		WCH_LOG(WCH_LOG_LVL_ERR, "cannot create dev file %d\r\n", ret);
#endif

	/* Add wchloglevel variable in /sys/devices/virtual/misc/wch */
	ret = device_create_file(wch_dev.this_device, &dev_attr_wchloglevel);
	if (ret)
		WCH_LOG(WCH_LOG_LVL_ERR, "cannot create dev file %d\r\n", ret);

	ret = device_create_file(wch_dev.this_device, &dev_attr_wchInterruptInterval);
	if (ret)
		WCH_LOG(WCH_LOG_LVL_ERR, "cannot create dev file %d\r\n", ret);

	return ret;

err:
    for (i = 0; i < WCH_IOMAP_REG_NUM; i++) {
		if (NULL != _wch_reg_base[i]) {
			iounmap (_wch_reg_base[i]);
			_wch_reg_base[i] = NULL;
		}
	}

	return ret;
}

static int wch_remove(struct platform_device *pdev)
{
	WCH_LOG(WCH_LOG_LVL_INFO, "wch_remove<---\n");
#if WCH_DUMP_BUFFER_ATTR
	device_remove_file(wch_dev.this_device, &dev_attr_wch_dump_buffer);
#endif
	device_remove_file(wch_dev.this_device, &dev_attr_wchloglevel);

	device_remove_file(wch_dev.this_device, &dev_attr_wchInterruptInterval);

	misc_deregister(&wch_dev);

	return 0;
}

static void wch_shutdown(struct platform_device *pdev)
{
	int i = 0;

	WCH_LOG(WCH_LOG_LVL_INFO, "wch_shutdown <---\n");

	for (i = 0; i < WCH_NUM; i++) {
			wch_hw_deinit(i);
	}
}

static const struct of_device_id wch_of_ids[] = {
	{.compatible = "Autochips,writechannel",},
	{}
};

static struct platform_driver wch_plt_drv = {
	.driver = {
		   .name = "Autochips-writechannel",
		   .owner = THIS_MODULE,
		   .of_match_table = wch_of_ids,
		   },
	.probe = wch_probe,
	.remove = wch_remove,
	.shutdown = wch_shutdown,
};


static int __init wch_dev_init(void)
{
	int ret = 0;
	struct proc_dir_entry *dir_entry = NULL;

	WCH_LOG(WCH_LOG_LVL_INFO, "init--->\n");
	ret = platform_driver_register(&wch_plt_drv);
	if (ret){
		WCH_LOG(WCH_LOG_LVL_ERR, "[wch]: %s: register  driver failed\n", __func__);
    }

	dir_entry = proc_create("driver/wch_reg", 0, NULL, &wch_proc_fops);
	if (!dir_entry) {
		WCH_LOG(WCH_LOG_LVL_ERR, "create driver/wch_reg file failed\n");
	}
	dir_entry = proc_create("driver/wch_misc", 0, NULL, &wch_misc_fops);
	if (!dir_entry) {
		WCH_LOG(WCH_LOG_LVL_ERR, "create driver/wch_misc file failed\n");
	}

	return ret;
}

static void __exit wch_dev_exit(void)
{
	WCH_LOG(WCH_LOG_LVL_INFO, "exit<---\n");

	platform_driver_unregister(&wch_plt_drv);
}

module_init(wch_dev_init);
module_exit(wch_dev_exit);

MODULE_DESCRIPTION(ATC_WCH_DRIVER_INFO);
MODULE_LICENSE(ATC_KERNEL_LINUX_LICENSE);

