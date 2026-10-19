/*
* Copyright (c) 2019 AutoChips Inc.
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

/**
 * @file   ac8257_alsa_debug.c
 * @brief
 * @author shiyuan dong
 * @email  shiyuan.dong@autochips.com
 */
#include "ac8257_alsa_debug.h"

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>

unsigned int g_alsa_dump_enable = 0;
unsigned int g_dump_id = 0;
unsigned int g_dump_dl = 0;

static int dump_enable(unsigned int enable, unsigned int dev_id, unsigned int is_dl)
{
	if (enable) {
		g_alsa_dump_enable = 1;
	} else {
		g_alsa_dump_enable = 0;
	}
	g_dump_id = dev_id;
	g_dump_dl = is_dl;
	return 0;
}

unsigned int write_file(char *tmp_file, unsigned int dev_id, unsigned int is_dl, const unsigned char *wbuf, unsigned int length)
{
	unsigned int ret = 0;
	struct file *fp;
	mm_segment_t fs;

	char filename[128];

	//dump data switch
	if(0 == g_alsa_dump_enable || dev_id != g_dump_id || is_dl != g_dump_dl) {
		return 0;
	}

	sprintf(filename, "%s_%d", tmp_file, dev_id);
	pr_info("write_file filename is %s.\n", filename);

	fp = filp_open(filename, (O_RDWR | O_CREAT | O_APPEND), 0x644U);
	if(0 == fp ) {
		pr_err("write_file, file name is %s, filp_open error fp is null. \n" , filename);
		return -1;
	}

	if (IS_ERR(fp) || !fp->f_op) {
		pr_err("write_file, file name is %s, filp_open error 0x%x. \n", filename, IS_ERR(fp) );
		return (-2) ;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);

	if (wbuf) {
		if ((ret = fp->f_op->write(fp, wbuf, length, &fp->f_pos)) < 0) {
			pr_err("Write %u bytes to file %s error %d\n", length, filename, ret);
			return (-3);
		}
	}
	if (IS_ERR(fp)) {
        pr_err("write_file fp %p is err.\n", fp);
		filp_close(fp, NULL);
	}
	set_fs(fs);
	return ret;
}


static ssize_t atc_alsa_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len = 0;
	pr_info("%s echo dump enable %u dev_id %u is_dl %u > /sys/devices/platform/11220000.audio/atc_alsa\n",
                __func__, g_alsa_dump_enable, g_dump_id, g_dump_dl);
	return len;
}

static ssize_t atc_alsa_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int  cmd = 0;
	unsigned int arg1 = 0;
	unsigned int arg2 = 0;
	unsigned int arg3 = 0;

	pr_info("%s echo cmd enable dev_id is_dl > /sys/devices/platform/11220000.audio/atc_alsa\n", __func__);

	if (4 == sscanf(buf, "%d %d %d %d", &cmd, &arg1, &arg2, &arg3)) {

		switch (cmd) {
			case 0: //dump
				dump_enable(arg1, arg2, arg3);
				break;
			default:
				break;
		}
    }  else {
		pr_err("%s param error!\n", __func__);
		pr_err("%s echo cmd enable dev_id is_dl > /sys/devices/platform/11220000.audio/atc_alsa\n", __func__);
	}

	pr_info("%s end!\n", __func__);

    return count;
}

static DEVICE_ATTR(atc_alsa, 0664, atc_alsa_show, atc_alsa_store);

void alsa_create_debug_file(struct device *dev)
{
	device_create_file(dev, &dev_attr_atc_alsa);
}
