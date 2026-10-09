/*
 * /dev/gpios_ioctl: GPIO access for the Jancar userspace (com.jancar.services, libJanCarIVI.so GPIO
 * class), CONFIG_JANCAR_GPIOS of the stock kernel (misc_gpio_init/misc_gpio_ioctl).
 *
 * The ioctl argument points to an int, the index N of a pin; the pin is the "ac8227l_pin_N" GPIO of
 * the "mediatek,ac8227l-gpio-ioctl" node of the stock DTB. Commands, as in the stock kernel:
 *   0x6b00: output high    0x6b01: output low    0x6b02: input    0x6b03: return the value
 * Without this device jancar.services cannot switch the board GPIOs (power of the peripherals it
 * then talks to over I2C, AV-in, ...) and its CarService stalls (ANR).
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 */
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>

#define JANCAR_GPIO_SET_HIGH	0x6b00
#define JANCAR_GPIO_SET_LOW	0x6b01
#define JANCAR_GPIO_SET_INPUT	0x6b02
#define JANCAR_GPIO_GET		0x6b03

#define JANCAR_GPIO_MAX		256

static DEFINE_MUTEX(jancar_gpio_lock);
static DECLARE_BITMAP(jancar_gpio_requested, JANCAR_GPIO_MAX);

/* GPIO number of pin index n, requested once (the stock driver requests it at every call) */
static int jancar_gpio_lookup(int n)
{
	struct device_node *np;
	char name[32];
	int gpio, ret;

	if (n < 0 || n >= JANCAR_GPIO_MAX)
		return -EINVAL;
	np = of_find_compatible_node(NULL, NULL, "mediatek,ac8227l-gpio-ioctl");
	if (!np)
		return -ENODEV;
	snprintf(name, sizeof(name), "ac8227l_pin_%d", n);
	gpio = of_get_named_gpio(np, name, 0);
	of_node_put(np);
	if (!gpio_is_valid(gpio))
		return -ENOENT;
	if (!test_bit(n, jancar_gpio_requested)) {
		ret = gpio_request(gpio, "ac8227l_pin_ioctl");
		if (ret && ret != -EBUSY)
			return ret;
		set_bit(n, jancar_gpio_requested);
	}
	return gpio;
}

static long jancar_gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int n, gpio;
	long ret = 0;

	if (copy_from_user(&n, (void __user *)arg, sizeof(n))) {
		pr_info("[jancar]copy_from_user failed\n");
		return -EFAULT;
	}
	if (cmd < JANCAR_GPIO_SET_HIGH || cmd > JANCAR_GPIO_GET) {
		pr_info("cmd:%d not support!!!\n", cmd);
		return 0;
	}

	mutex_lock(&jancar_gpio_lock);
	gpio = jancar_gpio_lookup(n);
	if (gpio < 0) {
		pr_debug("jancar_gpios: pin %d: %d\n", n, gpio);
		ret = gpio;
		goto out;
	}
	switch (cmd) {
	case JANCAR_GPIO_SET_HIGH:
		ret = gpio_direction_output(gpio, 1);
		break;
	case JANCAR_GPIO_SET_LOW:
		ret = gpio_direction_output(gpio, 0);
		break;
	case JANCAR_GPIO_SET_INPUT:
		ret = gpio_direction_input(gpio);
		break;
	case JANCAR_GPIO_GET:
		ret = gpio_get_value(gpio);
		break;
	}
out:
	mutex_unlock(&jancar_gpio_lock);
	return ret;
}

static const struct file_operations jancar_gpio_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = jancar_gpio_ioctl,
	.compat_ioctl = jancar_gpio_ioctl,
};

static struct miscdevice jancar_gpio_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gpios_ioctl",
	.fops = &jancar_gpio_fops,
	.mode = 0666,	/* crwxrwxrwx on the stock unit; jancar.services runs as system */
};

static int __init jancar_gpio_init(void)
{
	int ret = misc_register(&jancar_gpio_dev);

	if (ret)
		pr_err("jancar_gpios: misc_register failed: %d\n", ret);
	return ret;
}
module_init(jancar_gpio_init);

MODULE_DESCRIPTION("Jancar /dev/gpios_ioctl");
MODULE_LICENSE("GPL v2");
