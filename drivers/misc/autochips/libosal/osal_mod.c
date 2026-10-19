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
 * AutoChips Inc. (C) 2018. All rights reserved.
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


#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include "types.h"
//#include "x_os.h"
#include <linux/spinlock_types.h>

#define MULTIPULE_EVENT_NUM   8

#include <linux/types.h>


extern spinlock_t ac83xx_event_lock;

#define MTK_KERNEL_LINUX_LICENSE     "Proprietary"

#define DLOG(x...)  printk(x)

typedef struct
{
	struct miscdevice cdev;   /* Char device structure */
} osal_dev;

static osal_dev *_prOSALDev;

static int osaldev_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	return ret;
}

static int osaldev_release(struct inode *inode, struct file *file)
{
	return 0;
}

#include <asm/uaccess.h>

static long osaldev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    u32 ret = 0;
    return ret;
}

struct file_operations osaldev_fops = {
    .release = osaldev_release,
    .open = osaldev_open,
    .unlocked_ioctl = osaldev_ioctl,
};

static int __init drvosal_init(void)
{
    int result;

    _prOSALDev = kmalloc(sizeof(osal_dev), GFP_KERNEL);
    if (!_prOSALDev) {
        DLOG("[OSAL]: malloc device failed\r\n");
        return -ENOMEM;
    }
    memset((void *)_prOSALDev, 0, sizeof(osal_dev));

    _prOSALDev->cdev.name = "osaldev";
    _prOSALDev->cdev.minor = MISC_DYNAMIC_MINOR;
    _prOSALDev->cdev.fops = &osaldev_fops;

    result = misc_register(&(_prOSALDev->cdev));
    if (result == 0)
    {
        DLOG("[OSAL] osal dev init successes\r\n");
    }
    else
    {
        DLOG("[OSAL] osal dev init error\r\n");
        kfree(_prOSALDev);
        _prOSALDev = NULL;
    }

    return 0;
}

static void __exit drvosal_exit(void)
{
	if (_prOSALDev)
	{
		misc_deregister(&_prOSALDev->cdev);
		kfree(_prOSALDev);
        _prOSALDev = NULL;
	}
}

module_init(drvosal_init);
module_exit(drvosal_exit);

MODULE_LICENSE("GPL");

