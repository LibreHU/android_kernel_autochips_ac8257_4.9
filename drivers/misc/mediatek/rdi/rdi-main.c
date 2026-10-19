/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/highmem.h>
#include <linux/backing-dev.h>
#include <linux/splice.h>
#include <linux/pfn.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/aio.h>
#include <asm/uaccess.h>



#define get_unmapped_area_mem	NULL

static inline int private_mapping_ok(struct vm_area_struct *vma)
{
	return 1;
}


#if defined(CONFIG_DEVMEM) || defined(CONFIG_DEVKMEM)
#ifdef CONFIG_STRICT_DEVMEM
static inline int page_is_allowed(unsigned long pfn)
{
	return devmem_is_allowed(pfn);
}
static inline int range_is_allowed(unsigned long pfn, unsigned long size)
{
	u64 from = ((u64)pfn) << PAGE_SHIFT;
	u64 to = from + size;
	u64 cursor = from;

	while (cursor < to) {
		if (!devmem_is_allowed(pfn)) {
			printk(KERN_INFO "Program %s tried to /dev/mem  %Lx->%Lx.\n", current->comm, from, to);
			return 0;
		}
		cursor += PAGE_SIZE;
		pfn++;
	}
	return 1;
}
#else
static inline int page_is_allowed(unsigned long pfn)
{
	return 1;
}
static inline int range_is_allowed(unsigned long pfn, unsigned long size)
{
	return 1;
}
#endif
#endif


static int rdi_open(struct inode *inode, struct file *filp)
{
	printk("%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}


static int rdi_mmap(struct file *fp, struct vm_area_struct *vma)
{
    size_t size = vma->vm_end - vma->vm_start;
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;

	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t)size - 1 < offset)
		return -EINVAL;

	if (!private_mapping_ok(vma))
		return -ENOSYS;

	vma->vm_page_prot = phys_mem_access_prot(fp, vma->vm_pgoff,
						 size,
						 vma->vm_page_prot);

	/* Remap-pfn-range will mark the range VM_IO */
	if (remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff,
			    size,
			    vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}



/******************************************************************************
 * Module related
 *****************************************************************************/
static const struct file_operations rdi_fops = {
	.owner = THIS_MODULE,
	.open = rdi_open,
	.mmap = rdi_mmap,
};


/* QHQ RT Monitor end */
static struct miscdevice rdi_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "rdi0",
	.fops = &rdi_fops,
};


static int __init rdi_init(void)
{
	int err = 0;
	err = misc_register(&rdi_dev);
	if (unlikely(err)) {
		printk("rdi: failed to register rdi0 device!\n");
		return err;
	}

	return err;
}

static void __exit rdi_exit(void)
{
	misc_deregister(&rdi_dev);

}
module_init(rdi_init);
module_exit(rdi_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek RDI Driver");
MODULE_AUTHOR("MediaTek Inc.");
