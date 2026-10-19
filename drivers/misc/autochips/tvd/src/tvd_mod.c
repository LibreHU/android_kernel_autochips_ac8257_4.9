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

#include <linux/module.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>	/* proc file use */
#include "tvd_if.h"
#include "tvd_if_priv.h"
#include "tvd_log.h"
#include "ac8257_addr_utils.h"

#ifdef VSS_LOG_TAG
#undef VSS_LOG_TAG
#define VSS_LOG_TAG "[vss_tvd]"
#endif

#define TVD_NAME    "tvd"
/*in order to distinguish tvd register address easily*/
#define TVD_ADDR 0x12101000

static long tvd_ioctl(struct file *filp, unsigned int cmd, unsigned long data)
{
	int ret = 0;
	return  ret;
}


static const struct file_operations tvd_fops = {
	.unlocked_ioctl = tvd_ioctl,
};


static struct miscdevice tvd_dev = {
	MISC_DYNAMIC_MINOR,
	"tvd",
	&tvd_fops
};


static TVD_HW_RESOURCE g_hw_resource;

typedef enum {
	TVD_BASE_ADDR = 0,
	TVD_VDISYS_CONFIG_BASE_ADDR,
	TVD_CVBS_BASE_ADDR,
	TVD_CLOCK_BASE_ADDR,
	TVD_VDISYS_DRAM_BRIDGE_BASE_ADDR,
	TVD_BASEADDR_NUM
} TVD_BASEADDR_ENUM;

static int  tvd_probe(struct platform_device *pdev)
{
	struct device_node *dn = NULL;
	void __iomem *base_addr = NULL;
	int ret = 0;
	u32 regs[4] = {0};

	tvd_info("enter\n");

	dn = pdev->dev.of_node;
	if (strcmp(dn->name, "tvd") != 0) {
		tvd_err("Device node don't match the driver\n");
		return -ENOMEM;
	}

	base_addr = of_iomap(dn, TVD_BASE_ADDR);
	if (!base_addr) {
		tvd_err("Unable to ioremap tvd registers\n");
		return -ENOMEM;
	}
	g_hw_resource.reg_base.tvd_base = base_addr;

	base_addr = NULL;
	base_addr = of_iomap(dn, TVD_VDISYS_CONFIG_BASE_ADDR);
	if (!base_addr) {
		tvd_err("Unable to ioremap vdisys registers\n");
		return -ENOMEM;
	}
	g_hw_resource.reg_base.vdisys_base = base_addr;

	base_addr = NULL;
	base_addr = of_iomap(dn, TVD_CVBS_BASE_ADDR);
	if (!base_addr) {
		tvd_err("Unable to ioremap cvbs registers\n");
		return -ENOMEM;
	}
	g_hw_resource.reg_base.cvbs_base = base_addr;

	base_addr = NULL;
	base_addr = of_iomap(dn, TVD_CLOCK_BASE_ADDR);
	if (!base_addr) {
		tvd_err("Unable to ioremap tvd clock registers\n");
		return -ENOMEM;
	}
	g_hw_resource.reg_base.clk_base = base_addr;

	base_addr = NULL;
	base_addr = of_iomap(dn, TVD_VDISYS_DRAM_BRIDGE_BASE_ADDR);
	if (!base_addr) {
		tvd_err("Unable to ioremap vdisys dram bridge registers\n");
		return -ENOMEM;
	}
	g_hw_resource.reg_base.vdisys_dram_bridge_base = base_addr;

	g_hw_resource.tvd_irq = irq_of_parse_and_map(dn, 0);
	/*For TVD 3D Comb Filter*/
	dn = of_find_compatible_node(NULL, NULL, "autochips,tvd");
	if (dn) {
		ret = of_property_read_u32_array(dn, "reg", regs, 4);
		if (0 != ret) {
			tvd_err("of_property_read_u32_array reg Fail!\n");
			return -1;
		}
		g_hw_resource.tdc_dram_addr = ARM1_CPUADDR_TO_DEVADDR_NOM4U(regs[1]);//XXX
		tvd_info("get TDC_DRAM_BASE = 0x%lx\n", g_hw_resource.tdc_dram_addr);
	}else {
		tvd_err("can't find compatible node\n");
	}

	tvd_module_context_init(&g_hw_resource);

	/*End*/
	platform_set_drvdata(pdev, &tvd_dev);
	ret = misc_register(&(tvd_dev));
	if (ret) {
		tvd_err("register tvd device failed!\n");
		return ret;
	}

	tvd_info("leave\n");
	return ret;
}

static int  tvd_remove(struct platform_device *pdev)
{
	tvd_info("enter\n");
	tvd_module_context_deinit();
	tvd_info("leave\n");
	return 0;
}

static int tvd_resume(struct platform_device *device)
{
	return 0;
}

extern int tvd_hw_clear(void);

static int tvd_suspend(struct platform_device *device, pm_message_t state)
{
	tvd_hw_clear();
	tvd_info("tvd_suspend \n");
	return 0;
}

#define TVD_READ32(offset)		    readl((base + offset))
#define TVD_WRITE32(offset, value)	writel((value), (base + offset))

static ssize_t tvd_read_reg_proc(struct file *pFile, char __user *pBuffer, size_t Count, loff_t *off)
{
#if 1
	return 0;
#else
	char *p = (char*)pFile;
	void __iomem *base = g_hw_resource.reg_base.tvd_base;
	char** ppStart=NULL;
	long Length = 0;
	u32 i = 0;
	long ret = 0;

	tvd_info("enter pPage(%p),off(0x%lx),Count(%ld)", pFile, (unsigned long)off, (long int)Count);

	p += sprintf(p, "tvd register\n");

	for (i = 0x0; i <= 0x1AC; i += 4) {
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(TVD_ADDR + i),TVD_READ32(i));
	}
	*ppStart = (char*)((unsigned long)pFile + (unsigned long)off);

	Length = (long)((unsigned long)p - (unsigned long)pFile);
	if (Length > (long)off) {
		Length -= (long)off;
	} else {
		Length = 0;
	}

	ret = Length < Count ? Length : Count;

	tvd_info("leave and ret(%ld)", ret);
	return ((ssize_t)(ret));
#endif
}

static ssize_t tvd_write_reg_proc(struct file *pFile, const char __user *pBuffer, size_t Count, loff_t *p_off)
{
	char RegBuf[64];
	u32 CopyBufSize = (Count < (sizeof(RegBuf) - 1)) ? (Count) : (sizeof(RegBuf) - 1);
	u32 Addr = 0;
	u32 Data = 0;
	void __iomem *base = g_hw_resource.reg_base.tvd_base;

	tvd_info("pFile(%p),pBuffer(%p),Count(%ld)", pFile, pBuffer, (long int)Count);

	if (copy_from_user(RegBuf, pBuffer, CopyBufSize)) {
		tvd_err("copy_from_user() fail.");
		return -EFAULT;
	}

	if (sscanf(RegBuf, "%x %x", &Addr, &Data) == 2) {
		TVD_WRITE32((Addr), Data);
		tvd_err("Write => Addr: 0x%08X, Write Data: 0x%08X. Read Data: 0x%08X.",
			(unsigned int)(TVD_ADDR + Addr), Data, TVD_READ32(Addr));
	} else if (sscanf(RegBuf, "%x", &Addr) == 1) {
		tvd_err("Read => Addr: 0x%08X, Read Data: 0x%08X.",
			(unsigned int)(TVD_ADDR + Addr), TVD_READ32(Addr));
	}
	tvd_info("Count(%d)", (u32) Count);
	return ((ssize_t)Count);
}

static const struct of_device_id tvd_of_ids[] = {
	{ .compatible = "Autochips,tv-decoder", },
	{}
};

static struct platform_driver tvd_driver = {
	.probe      = tvd_probe,
	.remove     = tvd_remove,
	.resume     = tvd_resume,
	.suspend    = tvd_suspend,
	.driver     = {
		.name    = TVD_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = tvd_of_ids,
	},
};

static const struct file_operations tvd_proc_fops = {
	.read = tvd_read_reg_proc,
	.write = tvd_write_reg_proc,
};

static int __init tvd_init(void)
{
	struct proc_dir_entry *dir_entry = NULL;
	int ret;

	tvd_info("enter\n");
	ret = platform_driver_register(&tvd_driver);
	if(ret){
		tvd_err("tvd platform_driver_register err\n");
	}

	dir_entry = proc_create("driver/tvd_reg", 0, NULL, &tvd_proc_fops);
	if (!dir_entry) {
		tvd_err("create driver/tvd_reg file failed\n");
	}
	tvd_info("leave\n");
	return ret;
}

static void __exit tvd_exit(void)
{
	tvd_info("enter\n");
	platform_driver_unregister(&tvd_driver);
	tvd_info("leave\n");
}

module_init(tvd_init);
module_exit(tvd_exit);

MODULE_DESCRIPTION("atc tvd driver");
MODULE_LICENSE("GPL");
