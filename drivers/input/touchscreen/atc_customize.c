#include <linux/kernel.h>
#include <linux/dmi.h>
#include <linux/firmware.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/of.h>
#include <asm/unaligned.h>

#include "atc_customize.h"

extern int get_customize_operation(struct customize_ts_data *ts);
static const unsigned long customize_irq_flags[] = {
	IRQ_TYPE_EDGE_RISING,
	IRQ_TYPE_EDGE_FALLING,
	IRQ_TYPE_LEVEL_LOW,
	IRQ_TYPE_LEVEL_HIGH,
};

static int customize_i2c_read(struct i2c_client *client,
		char reg, u8 *buf, int len)
{
	struct i2c_msg msgs[2];
	unsigned char wbuf = reg;
	int ret;

	msgs[0].flags = 0;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 1;
	msgs[0].buf   = (u8 *)&wbuf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len;
	msgs[1].buf   = buf;

	ret = i2c_transfer(client->adapter, msgs, 2);
	return ret < 0 ? ret : (ret != ARRAY_SIZE(msgs) ? -EIO : 0);
}

static int customize_i2c_write(struct i2c_client *client, char reg, const u8 *buf,
		unsigned len)
{
	u8 *addr_buf;
	struct i2c_msg msg;
	int ret;

	addr_buf = kmalloc(len + 1, GFP_KERNEL);
	if (!addr_buf)
		return -ENOMEM;

	addr_buf[0] = reg & 0xFF;
	memcpy(&addr_buf[1], buf, len);

	msg.flags = 0;
	msg.addr = client->addr;
	msg.buf = addr_buf;
	msg.len = len + 1;

	ret = i2c_transfer(client->adapter, &msg, 1);
	kfree(addr_buf);
	return ret < 0 ? ret : (ret != 1 ? -EIO : 0);
}


static int customize_i2c_read16(struct i2c_client *client,
		u16 reg, u8 *buf, int len)
{
	struct i2c_msg msgs[2];
	u16 wbuf = cpu_to_be16(reg);
	int ret;

	msgs[0].flags = 0;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 2;
	msgs[0].buf   = (u8 *)&wbuf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len;
	msgs[1].buf   = buf;

	ret = i2c_transfer(client->adapter, msgs, 2);
	return ret < 0 ? ret : (ret != ARRAY_SIZE(msgs) ? -EIO : 0);
}

static int customize_i2c_write16(struct i2c_client *client, u16 reg, const u8 *buf,
		unsigned len)
{
	u8 *addr_buf;
	struct i2c_msg msg;
	int ret;

	addr_buf = kmalloc(len + 2, GFP_KERNEL);
	if (!addr_buf)
		return -ENOMEM;

	addr_buf[0] = reg >> 8;
	addr_buf[1] = reg & 0xFF;
	memcpy(&addr_buf[2], buf, len);

	msg.flags = 0;
	msg.addr = client->addr;
	msg.buf = addr_buf;
	msg.len = len + 2;

	ret = i2c_transfer(client->adapter, &msg, 1);
	kfree(addr_buf);
	return ret < 0 ? ret : (ret != 1 ? -EIO : 0);
}


/**
 * customize_ts_irq_handler - The IRQ handler
 *
 * @irq: interrupt number.
 * @dev_id: private data pointer.
 */

static irqreturn_t customize_ts_irq_handler(int irq, void *dev_id)
{
	struct customize_ts_data *ts = dev_id;

	if (ts->handle_irq)
		ts->handle_irq(ts);

	return IRQ_HANDLED;
}


static int customize_request_irq(struct customize_ts_data *ts)
{
	return devm_request_threaded_irq(&ts->client->dev, ts->client->irq,
			NULL, customize_ts_irq_handler,
			ts->irq_flags, ts->client->name, ts);
}

static int config_pinctrl(struct device *dev)
{
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_state;
	int ret = 0;

	pinctrl = devm_pinctrl_get(dev);

	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		dev_info(dev, "Cannot find pinctrl!\n");
		return ret;
	}

	pins_state = pinctrl_lookup_state(pinctrl, "sda");
	if (IS_ERR(pins_state)) {
		ret = PTR_ERR(pins_state);
		dev_err(dev,"Cannot find pinctrl state_sda!\n");
		return ret;
	} else {
		pinctrl_select_state(pinctrl, pins_state);
	}

	pins_state = pinctrl_lookup_state(pinctrl, "scl");
	if (IS_ERR(pins_state)) {
		ret = PTR_ERR(pins_state);
		dev_err(dev, "Cannot find pinctrl state_scl!\n");
		return ret;
	} else {
		pinctrl_select_state(pinctrl, pins_state);
	}

	pins_state = pinctrl_lookup_state(pinctrl, "int");
	if (IS_ERR(pins_state)) {
		ret = PTR_ERR(pins_state);
		dev_err(dev, "Cannot find pinctrl state_scl!\n");
		return ret;
	} else {
		pinctrl_select_state(pinctrl, pins_state);
	}

	pins_state = pinctrl_lookup_state(pinctrl, "rst");
	if (IS_ERR(pins_state)) {
		ret = PTR_ERR(pins_state);
		dev_err(dev, "Cannot find pinctrl state_scl!\n");
		return ret;
	} else {
		pinctrl_select_state(pinctrl, pins_state);
	}
	devm_pinctrl_put(pinctrl);

	return ret;
}

/**
 * customize_get_gpio_config - Get GPIO config from ACPI/DT
 *
 * @ts: customize_ts_data pointer
 */
static int customize_get_gpio_config(struct customize_ts_data *ts)
{
	int error;
	struct device *dev;
	struct gpio_desc *gpiod;

	if (!ts->client)
		return -EINVAL;
	dev = &ts->client->dev;

	config_pinctrl(dev);

	/* Get the interrupt GPIO pin number */
	gpiod = devm_gpiod_get_optional(dev, CUSTOMIZE_GPIO_INT_NAME, GPIOD_IN);
	if (IS_ERR(gpiod)) {
		error = PTR_ERR(gpiod);
		if (error != -EPROBE_DEFER)
			dev_dbg(dev, "Failed to get %s GPIO: %d\n",
					CUSTOMIZE_GPIO_INT_NAME, error);
		return error;
	}

	ts->gpiod_int = gpiod;

	/* Get the reset line GPIO pin number */
	gpiod = devm_gpiod_get_optional(dev, CUSTOMIZE_GPIO_RST_NAME, GPIOD_IN);
	if (IS_ERR(gpiod)) {
		error = PTR_ERR(gpiod);
		if (error != -EPROBE_DEFER)
			dev_dbg(dev, "Failed to get %s GPIO: %d\n",
					CUSTOMIZE_GPIO_RST_NAME, error);
		return error;
	}

	ts->gpiod_rst = gpiod;

	return 0;
}

/**
 * customize_request_input_dev - Allocate, populate and register the input device
 *
 * @ts: our customize_ts_data pointer
 *
 * Must be called during probe
 */
static int customize_request_input_dev(struct customize_ts_data *ts)
{
	int error;
#ifdef TOUCHPAD_KEYSUPPORT
	int i;
#endif

	ts->input_dev = devm_input_allocate_device(&ts->client->dev);
	if (!ts->input_dev) {
		dev_err(&ts->client->dev, "Failed to allocate input device.");
		return -ENOMEM;
	}

#ifdef TOUCHPAD_KEYSUPPORT
	if (ts->buttons) {
		input_set_capability(ts->input_dev, EV_KEY, BTN_TOUCH);

		for (i = 0; i < ts->buttons; i++)
			input_set_capability(ts->input_dev, EV_KEY, ts->key_event[i]);
	}
#endif
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
			0, ts->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
			0, ts->abs_y_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);

	if (ts->inputdev_config)
		ts->inputdev_config(ts);

	ts->input_dev->name = "customize Capacitive Main TouchScreen";
	ts->input_dev->phys = "input/ts";
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0x0416;
	ts->input_dev->id.product = ts->id;
	ts->input_dev->id.version = ts->version;
#ifdef TOUCHPAD_EXTERNFLAG
	if (ts->tp_extern) {
		ts->input_dev->name = "customize Capacitive Extern TouchScreen";
	}
#endif

	error = input_register_device(ts->input_dev);
	if (error) {
		dev_err(&ts->client->dev,
				"Failed to register input device: %d", error);
		return error;
	}

	return 0;
}

/**
 * customize_configure_dev - Finish device initialization
 *
 * @ts: our customize_ts_data pointer
 *
 * Must be called from probe to finish initialization of the device.
 * Contains the common initialization code for both devices that
 * declare gpio pins and devices that do not. It is either called
 * directly from probe or from request_firmware_wait callback.
 */
static int customize_configure_dev(struct customize_ts_data *ts)
{
	int error;
	unsigned int abs_x_max;
	unsigned int abs_y_max;

	ts->swapped_x_y = device_property_read_bool(&ts->client->dev,
			"touchscreen-swapped-x-y");
	ts->inverted_x = device_property_read_bool(&ts->client->dev,
			"touchscreen-inverted-x");
	ts->inverted_y = device_property_read_bool(&ts->client->dev,
			"touchscreen-inverted-y");

	//customize_read_config(ts);
	error = device_property_read_u32(&ts->client->dev,
			"touchscreen-abs-x-max", &abs_x_max);
	if (!error)
		ts->abs_x_max = abs_x_max;

	error = device_property_read_u32(&ts->client->dev,
			"touchscreen-abs-y-max", &abs_y_max);
	if (!error)
		ts->abs_y_max = abs_y_max;

	error = customize_request_input_dev(ts);
	if (error)
		return error;
	ts->irq_flags = customize_irq_flags[ts->int_trigger_type] | IRQF_ONESHOT;

	error = customize_request_irq(ts);
	if (error) {
		dev_err(&ts->client->dev, "request IRQ failed: %d\n", error);
		return error;
	}

	return 0;
}

static void customize_config_cb(const struct firmware *cfg, void *ctx)
{
	struct customize_ts_data *ts = ctx;

	customize_configure_dev(ts);

}

static int of_get_config(struct customize_ts_data *ts)
{
#ifdef TOUCHPAD_EXTERNFLAG
	device_property_read_u32(&ts->client->dev,
			"touchscreen-extern", &ts->tp_extern);
#endif
	return 0;

}

//extern int ti_touchscreen_flag;
static int customize_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct customize_ts_data *ts;
	int error;

	dev_dbg(&client->dev, "I2C Address: 0x%02x\n", client->addr);

	ts = devm_kzalloc(&client->dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->client = client;
	i2c_set_clientdata(client, ts);

	of_get_config(ts);

	error = customize_get_gpio_config(ts);
	if (error) {
		return error;
	}

	ts->i2c_read = customize_i2c_read;
	ts->i2c_write = customize_i2c_write;
	ts->i2c_read16 = customize_i2c_read16;
	ts->i2c_write16 = customize_i2c_write16;

	if (ts->gpiod_int && ts->gpiod_rst) {
		error = get_customize_operation(ts);
		if (error) {
			dev_err(&client->dev,
					"Failed to get operation: %d\n",
					error);
			return error;
		}

		if (ts->init_touchscreen) {
			error = ts->init_touchscreen(ts);
			if (error) {
				dev_err(&client->dev,
						"Failed to init touchscreen: %d\n",
						error);
				return error;
			}
		} else {
			return error;
		}

		error = request_firmware_nowait(THIS_MODULE, true, CUSTOMIZE_TS,
				&client->dev, GFP_KERNEL, ts,
				customize_config_cb);
		if (error) {
			dev_err(&client->dev,
					"Failed to invoke firmware loader: %d\n",
					error);
			return error;
		}

//		ti_touchscreen_flag = 1;
		return 0;
	}
	return 0;
}

static int customize_ts_remove(struct i2c_client *client)
{
	struct customize_ts_data *ts = i2c_get_clientdata(client);

	if (ts->deinit_touchscreen)
		ts->deinit_touchscreen(ts);

	return 0;
}

static const struct i2c_device_id customize_ts_id[] = {
	{ "customize", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, customize_ts_id);


#ifdef CONFIG_OF
static const struct of_device_id customize_of_match[] = {
	{ .compatible = "Autochips,customize" },
	{ }
};
MODULE_DEVICE_TABLE(of, customize_of_match);
#endif

static struct i2c_driver customize_ts_driver = {
	.probe = customize_ts_probe,
	.remove = customize_ts_remove,
	.id_table = customize_ts_id,
	.driver = {
		.name = "customize-TS",
		.of_match_table = of_match_ptr(customize_of_match),
	},
};
module_i2c_driver(customize_ts_driver);

MODULE_DESCRIPTION("customize touchscreen driver");
MODULE_LICENSE("GPL v2");
