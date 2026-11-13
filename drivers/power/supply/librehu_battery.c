// SPDX-License-Identifier: GPL-2.0
/*
 * LibreHU: a battery always full, on mains, for the Jancar UJC201 (AutoChips AC8257 head unit, no battery).
 *
 * The UJC201 device tree has no "mediatek,bat_gm30" node: the MediaTek gauge driver (mediatek/battery) never probes
 * and no power supply of type battery exists, so the health HAL logs "healthd: No battery devices found" and gives a
 * level of 0. The stock Android hid it (it showed 100 %); LineageOS shows 0-1 %. Here Android reads a battery at
 * 100 %, full, 4.2 V, 25 C, and mains online ("ac", only when no other power supply already has that name).
 * Off with librehu_battery.enable=0 on the kernel command line.
 *
 * Copyright (C) 2026 LibreHU
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

static bool enable = true;
module_param(enable, bool, 0444);
MODULE_PARM_DESC(enable, "register the full battery and mains power supplies (default: yes)");

static enum power_supply_property librehu_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property librehu_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int librehu_battery_get_property(struct power_supply *psy, enum power_supply_property psp,
					union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = POWER_SUPPLY_STATUS_FULL;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = 100;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = 4200000; /* uV */
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = 250; /* tenths of a degree Celsius */
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int librehu_ac_get_property(struct power_supply *psy, enum power_supply_property psp,
				   union power_supply_propval *val)
{
	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;
	val->intval = 1;
	return 0;
}

static const struct power_supply_desc librehu_battery_desc = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = librehu_battery_props,
	.num_properties = ARRAY_SIZE(librehu_battery_props),
	.get_property = librehu_battery_get_property,
};

static const struct power_supply_desc librehu_ac_desc = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = librehu_ac_props,
	.num_properties = ARRAY_SIZE(librehu_ac_props),
	.get_property = librehu_ac_get_property,
};

/* A power supply of that name already registered (a real driver): left alone. */
static bool librehu_power_supply_exists(const char *name)
{
	struct power_supply *psy = power_supply_get_by_name(name);

	if (!psy)
		return false;
	power_supply_put(psy);
	return true;
}

static struct power_supply *librehu_register(struct platform_device *pdev, const struct power_supply_desc *desc)
{
	struct power_supply_config cfg = {};
	struct power_supply *psy;

	if (librehu_power_supply_exists(desc->name)) {
		pr_info("librehu_battery: %s already registered, kept\n", desc->name);
		return NULL;
	}
	psy = power_supply_register_no_ws(&pdev->dev, desc, &cfg);
	if (IS_ERR(psy)) {
		pr_err("librehu_battery: %s not registered (%ld)\n", desc->name, PTR_ERR(psy));
		return NULL;
	}
	pr_info("librehu_battery: %s registered (full, on mains)\n", desc->name);
	return psy;
}

/* After the real power supply drivers (late), so that one of them keeps its name. */
static int __init librehu_battery_init(void)
{
	struct platform_device *pdev;

	if (!enable) {
		pr_info("librehu_battery: off (librehu_battery.enable=0)\n");
		return 0;
	}
	pdev = platform_device_register_simple("librehu-battery", -1, NULL, 0);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);
	librehu_register(pdev, &librehu_ac_desc);
	librehu_register(pdev, &librehu_battery_desc);
	return 0;
}
late_initcall(librehu_battery_init);

MODULE_DESCRIPTION("LibreHU: battery always full on mains (head unit without battery)");
MODULE_LICENSE("GPL");
