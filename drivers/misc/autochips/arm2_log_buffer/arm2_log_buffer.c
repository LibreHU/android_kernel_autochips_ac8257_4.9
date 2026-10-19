/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors. Without
 * the prior written permission of AutoChips inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of AutoChips Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * AutoChips Inc. (C) 2016. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AUTOCHIPS SOFTWARE")
 * RECEIVED FROM AUTOCHIPS AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. AUTOCHIPS EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES AUTOCHIPS PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE AUTOCHIPS SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN AUTOCHIPS
 * SOFTWARE. AUTOCHIPS SHALL ALSO NOT BE RESPONSIBLE FOR ANY AUTOCHIPS SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AUTOCHIPS'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE AUTOCHIPS SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT AUTOCHIPS'S OPTION, TO REVISE OR REPLACE THE
 * AUTOCHIPS SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO AUTOCHIPS FOR SUCH AUTOCHIPS SOFTWARE AT ISSUE.
 */


#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/pstore.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <asm/cacheflush.h>

#include "arm2_log_buffer.h"

#ifdef CONFIG_ARM64
extern void __inval_cache_range(unsigned long start, unsigned long end);
#endif

typedef struct Arm2Log {
	/* console buffer*/
	u32       log_init;
	u32       log_write_over_flag;
	u32       log_reserve_size;
	u32       log_write_data_offset;
	u32       log_read_data_offset;
	u32       log_drop_count;
	u32       atc_log_read;
} TArm2Log, *PTARm2Log;

#define ARM2_LOG_BUFFER_RESERVED_MEMORY_KEY "autochips,arm2-log"

static void *log_buffer_cur_arm2_log;
static TArm2Log *log_buffer_cur_arm2_header;
static void *log_buffer_arm2_reserve;

static struct work_struct log_buffer_wq;

static u32 cur_read_offset;

static char *log_buffer_by_arm2;
static char *log_buffer_by_arm2_header;

static phys_addr_t log_buffer_base_phys_addr;
static phys_addr_t log_buffer_base_size;

/**
 * Function:    Dump Arm2 Log Reserved Memory content
 * Arg1 offset: start addr offset relative to Arm2 Log Reserved Memory
 * Arg2 size:   size to Dump
 **/
static int dump_log_buffer(int offset, int size)
{
	int i = 0;
	u32 temp_size;
	int last = 0;

	//Driver not init
	if (NULL == log_buffer_by_arm2) {
		printk(KERN_ERR "[ARM2_LOG_BUFFER] arm2 log buffer not ready\n");
		return -1;
	}

	if (log_buffer_cur_arm2_header->log_reserve_size - offset <= size) {
		temp_size = log_buffer_cur_arm2_header->log_reserve_size - offset;
#ifdef CONFIG_ARM64
		__inval_cache_range((unsigned long)log_buffer_by_arm2 + offset,
				    (unsigned long)log_buffer_by_arm2 + offset + temp_size);
#else
		clean_dcache_area(log_buffer_by_arm2 + offset, temp_size);
#endif
		//Dump Log, Identify a line of log by '\n'
		for (i = 0; i < temp_size; i++) {
			if (*(log_buffer_by_arm2 + offset  + i) == '\n') {
				printk(KERN_INFO "%s", log_buffer_by_arm2 + offset + last);
				last = i + 2;
				i++;
			}
		}

#ifdef CONFIG_ARM64
		__inval_cache_range((unsigned long)log_buffer_by_arm2,
				    (unsigned long)log_buffer_by_arm2 + size - temp_size);
#else
		clean_dcache_area(log_buffer_by_arm2, size - temp_size);
#endif
		last = 0;
		//Dump Log, Identify a line of log by '\n'
		for (i = 0; i < size - temp_size; i++) {
			if (*(log_buffer_by_arm2 + i) == '\n') {
				printk(KERN_INFO "%s", log_buffer_by_arm2 + last);
				last = i + 2;
				i++;
			}
		}

		cur_read_offset = size - temp_size;
		log_buffer_cur_arm2_header->log_read_data_offset = cur_read_offset;
	} else {
#ifdef CONFIG_ARM64
		__inval_cache_range((unsigned long)log_buffer_by_arm2 + offset,
				    (unsigned long)log_buffer_by_arm2 + offset + size);
#else
		clean_dcache_area(log_buffer_by_arm2 + offset, size);
#endif
		//Dump Log, Identify a line of log by '\n'
		for (i = 0; i < size; i++) {
			if (*(log_buffer_by_arm2 + offset  + i) == '\n') {
				printk(KERN_INFO "%s", log_buffer_by_arm2 + offset + last);
				last = i + 2;
				i++;
			}
		}

		cur_read_offset = offset + size;
		log_buffer_cur_arm2_header->log_read_data_offset = cur_read_offset;
	}

	return last;
}

/**
 * Function:    A Task to Dump Arm2 Log Reserved Memory, Need to consider overwrite log buffer
 * Arg1 work:   Because of interrupt bottom half, need to use workqueue
 **/
static void arm2_log_buffer_dump_log(struct work_struct *work)
{
    int size = 0;
	u32 temp_read_offset = 0;
	u32 temp_write_offset = 0;
	int ret = 0;

	if (log_buffer_cur_arm2_header->log_init != 1 ||
		log_buffer_cur_arm2_header->log_write_data_offset > log_buffer_cur_arm2_header->log_reserve_size) {
		pr_err("arm2_log_buffer_dump_log  err:inlavid info--\n");
		return;
	}

	temp_write_offset = log_buffer_cur_arm2_header->log_write_data_offset;
	temp_read_offset = log_buffer_cur_arm2_header->log_read_data_offset;

	if ((temp_write_offset - cur_read_offset > 0) ||
		((temp_write_offset < cur_read_offset) &&
		(log_buffer_cur_arm2_header->log_reserve_size + temp_write_offset - temp_read_offset > 0))) {
		if (cur_read_offset < temp_write_offset) {
			size = temp_write_offset - cur_read_offset;
		} else {
			size = log_buffer_cur_arm2_header->log_reserve_size + temp_write_offset - cur_read_offset;
		}

		ret = dump_log_buffer(cur_read_offset, size);
	}
}

/**
 * Function:    when receive interrupt from arm2, start the bottom half of interrupt to dump
 * Arg1 irq:    Indicates the interrupt number to be allocated
 * Arg2 dev_id: Mainly used for sharing interrupt lines
 **/
irqreturn_t arm2_log_buffer_log_handler(int irq, void *dev_id)
{
	int temp;
	schedule_work(&log_buffer_wq);

	temp = ARM2_LOG_BUFFER_BIM_READ32(REG_DUALARM_INT)  | (0x08);
	ARM2_LOG_BUFFER_BIM_WRITE32(REG_DUALARM_INT, temp);

	return IRQ_HANDLED;
}

/**
 * Function:    Init Arm2 Log Reserved Memory from dts
 * Arg1 rmem:   the struct of reserved memory info
 **/
static int arm2_log_buffer_reserved_mem_of_init(struct reserved_mem *rmem)
{
	log_buffer_base_phys_addr = (phys_addr_t) rmem->base;
	log_buffer_base_size = (phys_addr_t) rmem->size;

	printk(KERN_INFO "[ARM2_LOG_BUFFER][INFO] base_phys_addr[0x%x], base_size[0x%x]\n",
		 log_buffer_base_phys_addr, log_buffer_base_size);

	return 0;
}


RESERVEDMEM_OF_DECLARE(arm2_log_buffer_reserved_init,
								ARM2_LOG_BUFFER_RESERVED_MEMORY_KEY,
								arm2_log_buffer_reserved_mem_of_init);

extern void arm2_log_buffer_send_message(void);

#ifdef CONFIG_OF
static const struct of_device_id arm2_log_buffer_match_ids[] = {
    { .compatible = "autochips,arm2_log_buffer", },
    {},
};
#endif

static int __init arm2_log_buffer_probe(struct platform_device *pdev)
{
	printk(KERN_INFO "ARM2_LOG_DRIVER HAVE PROBE\n");
	log_buffer_by_arm2_header = ioremap_wc(log_buffer_base_phys_addr, sizeof(TArm2Log));
	log_buffer_by_arm2 = ioremap_wc(log_buffer_base_phys_addr + sizeof(TArm2Log), log_buffer_base_size - sizeof(TArm2Log));
	if (log_buffer_by_arm2) {
		log_buffer_cur_arm2_log = log_buffer_by_arm2;
		log_buffer_cur_arm2_header = (TArm2Log *)log_buffer_by_arm2_header;
		log_buffer_arm2_reserve = (void *)(log_buffer_cur_arm2_log + sizeof(TArm2Log));
	} else {
		printk(KERN_ERR "ram_console: ioremap failed, [0x%x, 0x%x]\n",
			 log_buffer_base_phys_addr, log_buffer_base_size);
		return -EINVAL;
	}

	INIT_WORK(&log_buffer_wq, arm2_log_buffer_dump_log);

	//schedule_work(&log_buffer_wq);

	/* notify arm2 that log buffer driver is init ok */
	arm2_log_buffer_send_message();

	return 0;
}

static struct platform_driver log_buffer_driver_probe = {
	.probe = arm2_log_buffer_probe,
	.driver = {
		.name = "arm2_log_buffer",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = arm2_log_buffer_match_ids,
#endif
	},
};

static int __init arm2_log_buffer_init(void)
{
	int ret = 0;
	ret = platform_driver_register(&log_buffer_driver_probe);
	printk(KERN_INFO "ARM2_LOG_BUFFER_INIT\n");
	if (ret < 0) {
		printk(KERN_ERR "Arm2 log buffer driver probe failed, ret = %d\n", ret);
	}

	return ret;
}

module_init(arm2_log_buffer_init);
MODULE_DESCRIPTION("AutoChips arm2_log_buffer driver");
MODULE_LICENSE("GPL v2");
