#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <linux/interrupt.h>

#include <linux/gpio.h>
#include <linux/delay.h>

#define tpm_info printk

struct touchscreen_menager {
	int register_address;
	struct mutex lock;
};

int ti_touchscreen_flag = 0;
EXPORT_SYMBOL(ti_touchscreen_flag);

struct touchscreen_menager atc_tp_menager;

static s32 tp_menager_probe(struct platform_device *pdev)
{
	struct touchscreen_menager *tp_menager = &atc_tp_menager;

	mutex_init(&tp_menager->lock);
	tp_menager->register_address = -1;

	return 0;
}

int atc_lock_tpm(void)
{
	struct touchscreen_menager *tp_menager = &atc_tp_menager;
	mutex_lock(&tp_menager->lock);
	tpm_info("Touchscreen menager locked\n");
	return 0;
}
EXPORT_SYMBOL(atc_lock_tpm);

int atc_unlock_tpm(void)
{
	struct touchscreen_menager *tp_menager = &atc_tp_menager;
	mutex_unlock(&tp_menager->lock);
	tpm_info("Touchscreen menager unlocked\n");
	return 0;
}
EXPORT_SYMBOL(atc_unlock_tpm);

int atc_register_tpm(int address)
{
	struct touchscreen_menager *tp_menager = &atc_tp_menager;

	tp_menager->register_address = address;
	tpm_info("Touchscreen menager register address 0x%x\n", address);
	return 0;
}
EXPORT_SYMBOL(atc_register_tpm);

int check_tp_inited(void)
{
	struct touchscreen_menager *tp_menager = &atc_tp_menager;

	return tp_menager->register_address;
}
EXPORT_SYMBOL(check_tp_inited);

static s32 tp_menager_remove(struct platform_device *pdev)
{
	struct touchscreen_menager *tp_menager = &atc_tp_menager;
	mutex_destroy(&tp_menager->lock);
	return 0;
}

static const struct of_device_id tpm_of_match[] = {
	{.compatible = "Autochips,tp-menager",},
	{},
};

struct platform_driver tp_menager_driver = {
	.probe = tp_menager_probe,
	.remove = tp_menager_remove,
	/*.shutdown = atc_pwm_shutdown, */
	.driver = {
		.name = "atc-tp-menager",
#ifdef CONFIG_OF
		.of_match_table = tpm_of_match,
#endif
	},
};


/* called when loaded into kernel */
static int __init tp_menager_init(void)
{
	int ret = 0;
	tpm_info("Touchscreen menager init!\n");

	ret = platform_driver_register(&tp_menager_driver);
	if (ret < 0) {
		tpm_info("platform_driver_register error\n");
		return ret;
	}

	return 0;
}

/* should never be called */
static void __exit tp_menager_exit(void)
{
	 platform_driver_unregister(&tp_menager_driver);
}

subsys_initcall(tp_menager_init);
module_exit(tp_menager_exit);
