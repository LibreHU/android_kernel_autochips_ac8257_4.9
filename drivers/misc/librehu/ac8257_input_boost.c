/*
 * Touch input boost: on a touch, the CPU minimum frequency is raised to freq_khz for duration_ms
 * through the MediaTek PPM system boost (the unused BOOST_BY_UT user, so that the PPM interface used
 * by the vendor modules is unchanged). Tunables in /sys/module/ac8257_input_boost/parameters/:
 * freq_khz (1533000; 0 disables), duration_ms (100).
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 */
#include <linux/input.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <mtk_ppm_api.h>

static unsigned int freq_khz = 1533000;
module_param(freq_khz, uint, 0644);
static unsigned int duration_ms = 100;
module_param(duration_ms, uint, 0644);

static bool boosted;
static DEFINE_MUTEX(boost_lock);	/* boost_on and boost_off may run on two CPUs */
static void ac8257_boost_on(struct work_struct *w);
static void ac8257_boost_off(struct work_struct *w);
static DECLARE_WORK(boost_on_work, ac8257_boost_on);
static DECLARE_DELAYED_WORK(boost_off_work, ac8257_boost_off);

static void ac8257_boost_on(struct work_struct *w)
{
	mutex_lock(&boost_lock);
	if (!boosted && freq_khz) {
		mt_ppm_sysboost_freq(BOOST_BY_UT, freq_khz);
		boosted = true;
	}
	mutex_unlock(&boost_lock);
	mod_delayed_work(system_wq, &boost_off_work, msecs_to_jiffies(duration_ms));
}

static void ac8257_boost_off(struct work_struct *w)
{
	mutex_lock(&boost_lock);
	if (boosted) {
		mt_ppm_sysboost_freq(BOOST_BY_UT, 0);
		boosted = false;
	}
	mutex_unlock(&boost_lock);
}

static void ac8257_boost_event(struct input_handle *handle, unsigned int type,
			       unsigned int code, int value)
{
	/* a new contact (BTN_TOUCH down or a new tracking id) */
	if ((type == EV_KEY && code == BTN_TOUCH && value) ||
	    (type == EV_ABS && code == ABS_MT_TRACKING_ID && value >= 0))
		queue_work(system_highpri_wq, &boost_on_work);
}

static int ac8257_boost_connect(struct input_handler *handler, struct input_dev *dev,
				const struct input_device_id *id)
{
	struct input_handle *handle;
	int ret;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;
	handle->dev = dev;
	handle->handler = handler;
	handle->name = "ac8257_input_boost";
	ret = input_register_handle(handle);
	if (ret)
		goto err_free;
	ret = input_open_device(handle);
	if (ret)
		goto err_unregister;
	return 0;
err_unregister:
	input_unregister_handle(handle);
err_free:
	kfree(handle);
	return ret;
}

static void ac8257_boost_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

/* touchscreens only */
static const struct input_device_id ac8257_boost_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT | INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] = BIT_MASK(ABS_MT_POSITION_X) },
	},
	{ },
};

static struct input_handler ac8257_boost_handler = {
	.event = ac8257_boost_event,
	.connect = ac8257_boost_connect,
	.disconnect = ac8257_boost_disconnect,
	.name = "ac8257_input_boost",
	.id_table = ac8257_boost_ids,
};

static int __init ac8257_input_boost_init(void)
{
	return input_register_handler(&ac8257_boost_handler);
}
late_initcall(ac8257_input_boost_init);

MODULE_DESCRIPTION("AC8257 touch input boost");
MODULE_LICENSE("GPL v2");
