/*
* Copyright (c) 2013-2020 AutoChips Inc.
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
#include <linux/printk.h>
#include <linux/kernel.h>
#include <mt-plat/mtk_secure_api.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define MTK_TYPE_GPIO_POWER 0x1

//default pin is 150
#define MTK_NUMBER_GPIO_POWER 150

static int __init atc_gpio_config_init(void)
{
    struct device_node *dev_node;
    unsigned int val;
    unsigned int gpio_number;
    dev_node = of_find_compatible_node(NULL, NULL, "mediatek,atf_gpio_config");

    if (dev_node) {
        if (of_property_read_u32(dev_node, "mediatek,gpio", &val))
            gpio_number = MTK_NUMBER_GPIO_POWER;
        else
            gpio_number = val;
    }
    else
        gpio_number = MTK_NUMBER_GPIO_POWER;

    pr_notice("ATF power gpio number:%d\n",gpio_number);
    mt_secure_call(MTK_SIP_KERNEL_GPIO_CONFIG, MTK_TYPE_GPIO_POWER, gpio_number, 0, 0);
    return 0;
}

static void __exit atc_gpio_config_exit(void)
{
    return;
}


module_init(atc_gpio_config_init);
module_exit(atc_gpio_config_exit);
