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
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/io.h>
#include <asm/io.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include<linux/proc_fs.h>
#include<linux/sched.h>
#include<linux/errno.h>
#include<linux/fcntl.h>
#include<linux/seq_file.h>
#include<linux/cdev.h>

#include<linux/kernel.h>
#include<linux/string.h>
#include<linux/delay.h>
#include<linux/vmalloc.h>

#include "cli.h"

extern void __iomem *io_base;
extern unsigned long MEM_BASE;
extern unsigned long MEM_END;

#define FILE_BASE "/data/backup/memdata"

static void show_usage(void)
{
	pr_info("arguments error,usage:\n");
	pr_info("mr <address> [len]:\n");
	pr_info("dump <address> [size] [number] [time]:\n");
	pr_info("mw <address> [len] <val>:\n");
	pr_info("ior <address> [len]:\n");
	pr_info("iow <address> [len] <val>:\n");
}

/*
 * This function dump data in memory for the specified address.
 * p:          start address
 * data_size:  dump data size
 * dump_count: Number of times to dump memory
 */
static int save_mem_data(void *p, unsigned long data_size, unsigned int dump_count)
{
	struct file *fp;
	mm_segment_t fs;
	int ret;

	char path[25] = {0};
	char suffix[5] = {0};

	if (p == NULL) {
		pr_err("bad parameter!\n");
		return -EINVAL;
	}

	strcpy(path, FILE_BASE);
	snprintf(suffix, 5, "%d", dump_count);
	strncat(path, suffix, strlen(suffix));

	fp = filp_open(path, O_CREAT|O_RDWR, 0644);
	if (NULL == fp) {
		pr_err("file name is %s, file_open error fp is null. \n", path);
		return -EINVAL;
	}
	if (IS_ERR(fp)) {
		pr_err("file name is %s, file_open error 0x%x. \n", path, IS_ERR(fp));
		return -ENOENT;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	ret = vfs_write(fp, (char __user *)p, (size_t)data_size, &fp->f_pos);
	if (ret < 0) {
		pr_err("The memory data write failed!\n");
		filp_close(fp, NULL);
		set_fs(fs);
		return -EINVAL;
	}

	pr_info("Loaded! file path : %s \n",  path);

	filp_close(fp, NULL);

	set_fs(fs);

	return 0;
}

static int _CLI_DUMP(int argc, const cmd_args *argv)
{
	unsigned long addr = 0;
	unsigned long len = 0;

	int ret = 0;
	u32 *p = NULL;

	unsigned int dumpN = 1;
	unsigned int t = 0;

	int i = 0;

	char *dest = NULL;

	switch (argc) {
	case 5:
		addr = argv[1].u;
		len = argv[2].u;
		dumpN = argv[3].u;
		t = argv[4].u;
		break;
	case 4:
		addr = argv[1].u;
		len = argv[2].u;
		dumpN = argv[3].u;
		break;
	case 3:
		addr = argv[1].u;
		len = argv[2].u;
		break;
	case 2:
		addr = argv[1].u;
		break;
	default:
		show_usage();
		return -EINVAL;
	}

#ifndef CONFIG_XEN
	if (addr < MEM_BASE)
		addr = addr + MEM_BASE;
#endif

	if ((addr + len) > MEM_END) {
		pr_err("%s: invalid addr(0x%8lx) len(0x%lx)\n", argv[0].str, addr, len);
		return -EINVAL;
	}
	pr_info("%s: addr(0x%8lx) size(0x%lx)\n", argv[0].str, addr, len);

#ifdef CONFIG_XEN
	if (addr < MEM_BASE)
		p = ioremap(addr & ~3, len);
	else
#endif
	p = phys_to_virt(addr & ~3);

	dest = vmalloc(len + 10);
	if (!dest) {
		pr_err("vmalloc failed!\n");
		return -ENOMEM;
	}

	for (i = 0; i < dumpN; i++) {
		memcpy(dest, p, len);
		ret = save_mem_data(dest, len, i + 1);
		if (ret < 0) {
			pr_err("save data is failed!\n");
			vfree(dest);
			return -ENOENT;
		}
		if (t > 0)
			msleep(t);
	}
	vfree(dest);

#ifdef CONFIG_XEN
	if (addr < MEM_BASE)
		iounmap(p);
#endif

	return 0;
}

static int _CLI_MR(int argc, const cmd_args *argv)
{
	unsigned long addr = 0;
	unsigned long len = 0;
	int i = 0;
	u32 *p = NULL;

	switch (argc) {
	case 3:
		addr = argv[1].u;
		len = argv[2].u;
		break;
	case 2:
		addr = argv[1].u;
		break;
	default:
		show_usage();
		return -EINVAL;
	}

#ifndef CONFIG_XEN
	if (addr < MEM_BASE)
		addr = addr + MEM_BASE;
#endif

	if ((addr + len) > MEM_END) {
		pr_err("%s: invalid addr(0x%8lx) len(0x%lx)\n", argv[0].str, addr, len);
		return -EINVAL;
	}

	pr_info("%s: addr(0x%8lx) len(0x%lx)\n", argv[0].str, addr, len);
#ifdef CONFIG_XEN
	if (addr < MEM_BASE)
		p = ioremap(addr & ~3, len);
	else
#endif
	p = phys_to_virt(addr & ~3);

	for (; i < len || i == 0; i += 4, p++) {
		if (i%32 == 0)
			pr_info("\n");
		pr_info("%08x ", *p);
	}
	pr_info("\n");

#ifdef CONFIG_XEN
	if (addr < MEM_BASE)
		iounmap(p);
#endif
	return 0;
}

static int _CLI_MW(int argc, const cmd_args *argv)
{
	unsigned long addr = 0;
	unsigned long len = 0;
	unsigned long val = 0;
	int i = 0;
	u32 *p = NULL;

	switch (argc) {
	case 4:
		addr = argv[1].u;
		len = argv[2].u;
		val = argv[3].u;
		break;
	case 3:
		addr = argv[1].u;
		val = argv[2].u;
		break;
	default:
		show_usage();
		return -EINVAL;
	}

#ifndef CONFIG_XEN
	if (addr < MEM_BASE)
		addr = addr + MEM_BASE;
#endif
	if ((addr + len) > MEM_END) {
		pr_err("%s: invalid addr(0x%8lx) len(0x%lx)\n", argv[0].str, addr, len);
		return -EINVAL;
	}
	pr_info("%s: addr(0x%8lx) len(0x%lx) val(0x%lx)\n", argv[0].str, addr, len, val);

#ifdef CONFIG_XEN
	if (addr < MEM_BASE)
		p = ioremap(addr & ~3, len);
	else
#endif
	p = phys_to_virt(addr & ~3);

	for (; i < len || i == 0; i += 4, p++) {
		if (i%32 == 0)
			pr_info("\n");
		*p = val;
		pr_info("%08x ", *p);
	}
	pr_info("\n");

#ifdef CONFIG_XEN
	if (addr < MEM_BASE)
		iounmap(p);
#endif

	return 0;
}

static int _CLI_IOR(int argc, const cmd_args *argv)
{
	unsigned long addr = 0;
	unsigned long len = 0;
	int i = 0;
	u32 *p;

	switch (argc) {
	case 3:
		addr = argv[1].u;
		len = argv[2].u;
		break;
	case 2:
		addr = argv[1].u;
		break;
	default:
		show_usage();
		return -EINVAL;
	}

	if ((addr < IO_BASE) || (addr + len) > (IO_BASE + IO_SIZE)) {
		pr_err("%s: invalid addr(0x%8lx) len(0x%lx)\n", argv[0].str, addr, len);
		return -EINVAL;
	}

	pr_info("%s: addr(0x%8lx) len(0x%lx)\n", argv[0].str, addr, len);
	p = io_base - IO_BASE + (addr & ~3);

	for (; i < len || i == 0; i += 4, p++) {
		if (i%32 == 0)
			pr_info("\n");
		pr_info("%08x ", __raw_readl(p));
	}
	pr_info("\n");

	return 0;
}

static int _CLI_IOW(int argc, const cmd_args *argv)
{
	unsigned long addr = 0;
	unsigned long len = 0;
	unsigned long val = 0;
	int i = 0;
	u32 *p;

	switch (argc) {
	case 4:
		addr = argv[1].u;
		len = argv[2].u;
		val = argv[3].u;
		break;
	case 3:
		addr = argv[1].u;
		val = argv[2].u;
		break;
	default:
		show_usage();
		return -EINVAL;
	}

	if ((addr < IO_BASE) || (addr + len) > (IO_BASE + IO_SIZE)) {
		pr_err("%s: invalid addr(0x%8lx) len(0x%lx)\n", argv[0].str, addr, len);
		return -EINVAL;
	}

	pr_info("%s: addr(0x%8lx) len(0x%lx) val(0x%lx)\n", argv[0].str, addr, len, val);
	p = io_base - IO_BASE + (addr & ~3);

	for (; i < len || i == 0; i += 4, p++) {
		if (i%32 == 0)
			pr_info("\n");
		__raw_writel(val, p);
		pr_info("%08x ", __raw_readl(p));
	}
	pr_info("\n");

	return 0;
}

STATIC_COMMAND_START
	{"mr", 	"MemRead", 	&_CLI_MR},
	{"mw", 	"MemWrite", &_CLI_MW},
	{"dump", "dumpMem", &_CLI_DUMP},
	{"ior", "RegRead", 	&_CLI_IOR},
	{"iow", "RegWrite", &_CLI_IOW},
STATIC_COMMAND_END(mem);
