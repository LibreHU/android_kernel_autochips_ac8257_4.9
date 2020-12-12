/*
 *  Driver for Goodix Touchscreens
 *
 *  Copyright (c) 2014 Red Hat Inc.
 *  Copyright (c) 2015 K. Merker <merker@debian.org>
 *
 *  This code is based on gt9xx.c authored by andrew@goodix.com:
 *
 *  2010 - 2012 Goodix Technology.
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2 of the License.
 */

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

#define TOUCHPAD_KEYSUPPORT
#define TOUCHPAD_EXTERNFLAG

#define GTP_POLL_TIME       10     /* jiffy: ms*/
#define GTP_ADDR_LENGTH     2

#ifdef  TOUCHPAD_KEYSUPPORT
#define MAX_TOUCHPAD_KEYNUM 0x8
#define TPKEY_ARRAY_SIZE    0x20
#endif

#define MAX_TOUCHPAD_SUPPORT 0x4
#define TOUCHPAD_INFO 	0x5

extern int ti_touchscreen_flag;

struct goodix_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	int abs_x_max;
	int abs_y_max;
	bool swapped_x_y;
	bool inverted_x;
	bool inverted_y;
	unsigned int max_touch_num;
	unsigned int int_trigger_type;
	int cfg_len;
	struct gpio_desc *gpiod_int;
	struct gpio_desc *gpiod_rst;
	u16 id;
	u16 version;
	const char *cfg_name;
	struct completion firmware_loading_complete;
	unsigned long irq_flags;
	/* Extend */
#ifdef TOUCHPAD_EXTERNFLAG
	unsigned int tp_extern;
#endif
#ifdef TOUCHPAD_KEYSUPPORT
	unsigned int buttons;
	unsigned int bt_status;
	u32 key_event[MAX_TOUCHPAD_KEYNUM];
	u32 key_range[MAX_TOUCHPAD_KEYNUM][4];
#endif
	unsigned char ti_link;
	unsigned char ti_serializer;
	unsigned char ti_deserializer;
	unsigned char use_irq;

	struct hrtimer timer;
	struct workqueue_struct *goodix_wq;
	struct work_struct  work;

	u32 kps_info[MAX_TOUCHPAD_SUPPORT][TOUCHPAD_INFO]; /* abs-x-max abs-y-max swapped-x-y inverted-x inverted-y */
};

#define GOODIX_GPIO_INT_NAME		"irq"
#define GOODIX_GPIO_RST_NAME		"reset"

#define GOODIX_MAX_HEIGHT		4096
#define GOODIX_MAX_WIDTH		4096
#define GOODIX_INT_TRIGGER		1
#define GOODIX_CONTACT_SIZE		8
#define GOODIX_MAX_CONTACTS		10

#define GOODIX_CONFIG_MAX_LENGTH	240
#define GOODIX_CONFIG_911_LENGTH	186
#define GOODIX_CONFIG_967_LENGTH	228

/* Register defines */
#define GOODIX_REG_COMMAND		0x8040
#define GOODIX_CMD_SCREEN_OFF		0x05

#define GOODIX_READ_COOR_ADDR		0x814E
#define GOODIX_REG_CONFIG_DATA		0x8047
#define GOODIX_REG_ID			0x8140

#define RESOLUTION_LOC		1
#define MAX_CONTACTS_LOC	5
#define TRIGGER_LOC		6

static int init_sem_flag = 0;
static struct semaphore i2c_sem;

static const unsigned long goodix_irq_flags[] = {
	IRQ_TYPE_EDGE_RISING,
	IRQ_TYPE_EDGE_FALLING,
	IRQ_TYPE_LEVEL_LOW,
	IRQ_TYPE_LEVEL_HIGH,
};

/*
 * Those tablets have their coordinates origin at the bottom right
 * of the tablet, as if rotated 180 degrees
 */
static const struct dmi_system_id rotated_screen[] = {
#if defined(CONFIG_DMI) && defined(CONFIG_X86)
	{
		.ident = "Teclast X89",
		.matches = {
			/* tPAD is too generic, also match on bios date */
			DMI_MATCH(DMI_BOARD_VENDOR, "TECLAST"),
			DMI_MATCH(DMI_BOARD_NAME, "tPAD"),
			DMI_MATCH(DMI_BIOS_DATE, "12/19/2014"),
		},
	},
	{
		.ident = "Teclast X98 Pro",
		.matches = {
			/*
			 * Only match BIOS date, because the manufacturers
			 * BIOS does not report the board name at all
			 * (sometimes)...
			 */
			DMI_MATCH(DMI_BOARD_VENDOR, "TECLAST"),
			DMI_MATCH(DMI_BIOS_DATE, "10/28/2015"),
		},
	},
	{
		.ident = "WinBook TW100",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "WinBook"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TW100")
		}
	},
	{
		.ident = "WinBook TW700",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "WinBook"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TW700")
		},
	},
#endif
	{}
};

static int ti_write(struct i2c_client *client,char addr, char *buf, int len)
{
	struct i2c_msg msg = {0};
	char retries;
	char ret;

	msg.flags = 0;
	msg.addr = addr;
	msg.buf = buf;
	msg.len = len;

	for (retries = 0; retries < 2; retries++) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1) {
			return 0;
		}
		pr_info("no success, retry = %d\n", retries);
	}


	pr_info("Transfer fail!\n");
	return -1;
}

void select_ti_link(struct goodix_ts_data *ts)
{
	struct i2c_client *client = ts->client;
	char buffer[8];

	if (!ts->ti_link)
		return;

	buffer[0] = 0x1e;
	buffer[1] = ts->ti_link;

	ti_write(client, ts->ti_serializer, buffer, 2);
}

/**
 * goodix_i2c_read - read data from a register of the i2c slave device.
 *
 * @client: i2c device.
 * @reg: the register to read from.
 * @buf: raw write data buffer.
 * @len: length of the buffer to write
 */
static int goodix_i2c_read(struct i2c_client *client,
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

/**
 * goodix_i2c_write - write data to a register of the i2c slave device.
 *
 * @client: i2c device.
 * @reg: the register to write to.
 * @buf: raw data buffer to write.
 * @len: length of the buffer to write
 */
static int goodix_i2c_write(struct i2c_client *client, u16 reg, const u8 *buf,
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

static int goodix_i2c_write_u8(struct i2c_client *client, u16 reg, u8 value)
{
	return goodix_i2c_write(client, reg, &value, sizeof(value));
}

static int goodix_get_cfg_len(u16 id)
{
	switch (id) {
	case 911:
	case 9271:
	case 9110:
	case 927:
	case 928:
		return GOODIX_CONFIG_911_LENGTH;

	case 912:
	case 967:
		return GOODIX_CONFIG_967_LENGTH;

	default:
		return GOODIX_CONFIG_MAX_LENGTH;
	}
}

static int goodix_ts_read_input_report(struct goodix_ts_data *ts, u8 *data)
{
	int touch_num;
	int error;

	error = goodix_i2c_read(ts->client, GOODIX_READ_COOR_ADDR, data,
				GOODIX_CONTACT_SIZE + 1);
	if (error) {
		dev_err(&ts->client->dev, "I2C transfer error: %d\n", error);
		return error;
	}

	if (!(data[0] & 0x80))
		return -EAGAIN;

	touch_num = data[0] & 0x0f;
	if (touch_num > ts->max_touch_num)
		return -EPROTO;

	if (touch_num > 1) {
		data += 1 + GOODIX_CONTACT_SIZE;
		error = goodix_i2c_read(ts->client,
					GOODIX_READ_COOR_ADDR +
						1 + GOODIX_CONTACT_SIZE,
					data,
					GOODIX_CONTACT_SIZE * (touch_num - 1));
		if (error)
			return error;
	}

	return touch_num;
}

static void goodix_ts_report_touch(struct goodix_ts_data *ts, u8 *coor_data)
{
	int id = coor_data[0] & 0x0F;
	int i = 0, report = 0;
	int input_x = get_unaligned_le16(&coor_data[1]);
	int input_y = get_unaligned_le16(&coor_data[3]);
	int input_w = get_unaligned_le16(&coor_data[5]);

	/* Inversions have to happen before axis swapping */
	if (ts->inverted_x && ts->abs_x_max >= input_x)
		input_x = ts->abs_x_max - input_x;
	if (ts->inverted_y)
		input_y = ts->abs_y_max - input_y;
	if (ts->swapped_x_y)
		swap(input_x, input_y);

	if (input_x > ts->abs_x_max) {
		/* It means button */
		for (i = 0; i < ts->buttons; i++) {
			report = input_x >= ts->key_range[i][0] -
				(ts->key_range[i][2] / 2) &&
				input_x <= ts->key_range[i][0] +
				(ts->key_range[i][2] / 2) &&
				input_y >= ts->key_range[i][1] -
				(ts->key_range[i][3] / 2) &&
				input_y <= ts->key_range[i][1] +
				(ts->key_range[i][3] / 2) &&
				!(ts->bt_status & (1 << i));

			if (report) {
				ts->bt_status |= (1 << i);
				input_mt_slot(ts->input_dev, id);
				input_report_key(ts->input_dev, ts->key_event[i], 1);
				break;
			}
		}
	} else {
		input_mt_slot(ts->input_dev, id);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, input_w);
	}
}

/**
 * goodix_process_events - Process incoming events
 *
 * @ts: our goodix_ts_data pointer
 *
 * Called when the IRQ is triggered. Read the current device state, and push
 * the input events to the user space.
 */
static void goodix_process_events(struct goodix_ts_data *ts)
{
	u8  point_data[1 + GOODIX_CONTACT_SIZE * GOODIX_MAX_CONTACTS];
	int touch_num;
	int i;

	touch_num = goodix_ts_read_input_report(ts, point_data);
	if (touch_num < 0) {
		return;
	}
	if (touch_num == 0) {
		for (i = 0; i < ts->buttons; i++) {
			if (ts->bt_status & (1 << i)) {
				input_mt_slot(ts->input_dev, 0);
				input_report_key(ts->input_dev, ts->key_event[i], 0);
				ts->bt_status &= ~(0x1 << i);
			}
		}
		input_mt_sync_frame(ts->input_dev);
		input_sync(ts->input_dev);
		return;
	}

	for (i = 0; i < touch_num; i++)
		goodix_ts_report_touch(ts,
				&point_data[1 + GOODIX_CONTACT_SIZE * i]);

	input_mt_sync_frame(ts->input_dev);
	input_sync(ts->input_dev);
}

/**
 * goodix_ts_irq_handler - The IRQ handler
 *
 * @irq: interrupt number.
 * @dev_id: private data pointer.
 */
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
	struct goodix_ts_data *ts = dev_id;

	select_ti_link(ts);

	goodix_process_events(ts);

	if (goodix_i2c_write_u8(ts->client, GOODIX_READ_COOR_ADDR, 0) < 0)
		dev_err(&ts->client->dev, "I2C write end_cmd error\n");

	return IRQ_HANDLED;
}

static void goodix_free_irq(struct goodix_ts_data *ts)
{
	devm_free_irq(&ts->client->dev, ts->client->irq, ts);
}

static int goodix_request_irq(struct goodix_ts_data *ts)
{
	return devm_request_threaded_irq(&ts->client->dev, ts->client->irq,
					 NULL, goodix_ts_irq_handler,
					 ts->irq_flags, ts->client->name, ts);
}

#if 1
/**
 * goodix_check_cfg - Checks if config fw is valid
 *
 * @ts: goodix_ts_data pointer
 * @cfg: firmware config data
 */
static int goodix_check_cfg(struct goodix_ts_data *ts,
			    const struct firmware *cfg)
{
	int i, raw_cfg_len;
	u8 check_sum = 0;

	if (cfg->size > GOODIX_CONFIG_MAX_LENGTH) {
		dev_err(&ts->client->dev,
			"The length of the config fw is not correct");
		return -EINVAL;
	}

	raw_cfg_len = cfg->size - 2;
	for (i = 0; i < raw_cfg_len; i++)
		check_sum += cfg->data[i];
	check_sum = (~check_sum) + 1;
	if (check_sum != cfg->data[raw_cfg_len]) {
		dev_err(&ts->client->dev,
			"The checksum of the config fw is not correct");
		return -EINVAL;
	}

	if (cfg->data[raw_cfg_len + 1] != 1) {
		dev_err(&ts->client->dev,
			"Config fw must have Config_Fresh register set");
		return -EINVAL;
	}

	return 0;
}
#endif

/**
 * goodix_send_cfg - Write fw config to device
 *
 * @ts: goodix_ts_data pointer
 * @cfg: config firmware to write to device
 */
static int goodix_send_cfg(struct goodix_ts_data *ts,
			   const struct firmware *cfg)
{
#if 1
	int error;

	error = goodix_check_cfg(ts, cfg);
	if (error)
		return error;

	error = goodix_i2c_write(ts->client, GOODIX_REG_CONFIG_DATA, cfg->data,
				 cfg->size);
	if (error) {
		dev_err(&ts->client->dev, "Failed to write config data: %d",
			error);
		return error;
	}
	dev_dbg(&ts->client->dev, "Config sent successfully.");

	/* Let the firmware reconfigure itself, so sleep for 10ms */
	usleep_range(10000, 11000);
#endif
	return 0;
}

static int goodix_int_sync(struct goodix_ts_data *ts)
{
	int error;

	error = gpiod_direction_output(ts->gpiod_int, 0);
	if (error)
		return error;

	msleep(150);				/* T5: 50ms */

	error = gpiod_direction_input(ts->gpiod_int);
	if (error)
		return error;

	return 0;
}

void set_ti_rst_output_mode(struct goodix_ts_data *ts)
{
	struct i2c_client *client = ts->client;
	char buffer[8];

	if (!ts->ti_link)
		return;

	buffer[0] = 0x1e;
	buffer[1] = ts->ti_link;

	ti_write(client, ts->ti_serializer, buffer, 2);
	pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

	buffer[0] = 0xe;
	buffer[1] = 0x3;
	ti_write(client, ts->ti_serializer, buffer, 2);
	pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

	buffer[0] = 0x1e;
	buffer[1] = 0x5;
	ti_write(client, ts->ti_deserializer, buffer, 2);
	pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

	buffer[0] = 0x1f;
	buffer[1] = 0x03;
	ti_write(client, ts->ti_deserializer, buffer, 2);
	pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);
}

void set_ti_rst_local_mode(struct goodix_ts_data *ts)
{
	struct i2c_client *client = ts->client;
	char buffer[8];

	if (!ts->ti_link)
		return;

	buffer[0] = 0x1e;
	buffer[1] = ts->ti_link;

	ti_write(client, ts->ti_serializer, buffer, 2);
	pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

	buffer[0] = 0x1e;
	buffer[1] = 0x9;
	ti_write(client, ts->ti_deserializer, buffer, 2);
	pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);
}

void set_ti_int_output(struct goodix_ts_data *ts)
{
	struct i2c_client *client = ts->client;
	char buffer[8];

	if (!ts->ti_link)
		return;

	if (ts->ti_link == 0x1) {
		buffer[0] = 0x1e;
		buffer[1] = ts->ti_link;

		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

		buffer[0] = 0x0f;
		buffer[1] = 0x3;
		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

		buffer[0] = 0x1f;
		buffer[1] = 0x5;
		ti_write(client, ts->ti_deserializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);
	} else {
		buffer[0] = 0x1e;
		buffer[1] = 0x2;

		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

		buffer[0] = 0x0f;
		buffer[1] = 0x3;
		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

		buffer[0] = 0x1e;
		buffer[1] = ts->ti_link;

		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

		buffer[0] = 0x1f;
		buffer[1] = 0x5;
		ti_write(client, ts->ti_deserializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);
	}
}

void set_ti_int_input(struct goodix_ts_data *ts)
{
	struct i2c_client *client = ts->client;
	char buffer[8];

	if (!ts->ti_link)
		return;

	if (ts->ti_link == 0x1) {
		buffer[0] = 0x1e;
		buffer[1] = ts->ti_link;

		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

		buffer[0] = 0x0f;
		buffer[1] = 0x5;
		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

		buffer[0] = 0x1f;
		buffer[1] = 0x3;
		ti_write(client, ts->ti_deserializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);
	} else {
		buffer[0] = 0x1e;
		buffer[1] = 0x2;

		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);


		buffer[0] = 0x0f;
		buffer[1] = 0x5;
		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

		buffer[0] = 0x1e;
		buffer[1] = ts->ti_link;

		ti_write(client, ts->ti_serializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

		buffer[0] = 0x1f;
		buffer[1] = 0x3;
		ti_write(client, ts->ti_deserializer, buffer, 2);
		pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);
	}
}

/**
 * goodix_reset - Reset device during power on
 *
 * @ts: goodix_ts_data pointer
 */
static int goodix_reset(struct goodix_ts_data *ts)
{
	int error;

	set_ti_int_output(ts);
	/* begin select I2C slave addr */
	error = gpiod_direction_output(ts->gpiod_rst, 0);
	if (error) {
		return error;
	}

	msleep(140);				/* T2: > 10ms */

	/* HIGH: 0x28/0x29, LOW: 0xBA/0xBB */
	error = gpiod_direction_output(ts->gpiod_int, ts->client->addr == 0x14);
	if (error) {
		return error;
	}

	msleep(140);
	usleep_range(100, 2000);		/* T3: > 100us */

	error = gpiod_direction_output(ts->gpiod_rst, 1);
	if (error) {
		return error;
	}

	msleep(110);
	usleep_range(6000, 10000);		/* T4: > 5ms */

	/* end select I2C slave addr */
	//error = gpiod_direction_input(ts->gpiod_rst);
	//if (error) {
	//	return error;
	//}

	error = goodix_int_sync(ts);
	if (error)
		return error;

	set_ti_int_input(ts);

	return 0;
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
		dev_err(dev, "Cannot find pinctrl state_sda!\n");
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
 * goodix_get_gpio_config - Get GPIO config from ACPI/DT
 *
 * @ts: goodix_ts_data pointer
 */
static int goodix_get_gpio_config(struct goodix_ts_data *ts)
{
	int error;
	struct device *dev;
	struct gpio_desc *gpiod;

	if (!ts->client)
		return -EINVAL;
	dev = &ts->client->dev;

	config_pinctrl(dev);

	/* Get the interrupt GPIO pin number */
	gpiod = devm_gpiod_get_optional(dev, GOODIX_GPIO_INT_NAME, GPIOD_IN);
	if (IS_ERR(gpiod)) {
		error = PTR_ERR(gpiod);
		if (error != -EPROBE_DEFER)
			dev_dbg(dev, "Failed to get %s GPIO: %d\n",
				GOODIX_GPIO_INT_NAME, error);
		return error;
	}

	ts->gpiod_int = gpiod;

	/* Get the reset line GPIO pin number */
	gpiod = devm_gpiod_get_optional(dev, GOODIX_GPIO_RST_NAME, GPIOD_IN);
	if (IS_ERR(gpiod)) {
		error = PTR_ERR(gpiod);
		if (error != -EPROBE_DEFER)
			dev_dbg(dev, "Failed to get %s GPIO: %d\n",
				GOODIX_GPIO_RST_NAME, error);
		return error;
	}

	ts->gpiod_rst = gpiod;

	return 0;
}

/**
 * goodix_read_config - Read the embedded configuration of the panel
 *
 * @ts: our goodix_ts_data pointer
 *
 * Must be called during probe
 */
static void goodix_read_config(struct goodix_ts_data *ts)
{
	u8 config[GOODIX_CONFIG_MAX_LENGTH];
	int error;

	error = goodix_i2c_read(ts->client, GOODIX_REG_CONFIG_DATA,
				config, ts->cfg_len);
	if (error) {
		dev_warn(&ts->client->dev,
			 "Error reading config (%d), using defaults\n",
			 error);
		ts->abs_x_max = GOODIX_MAX_WIDTH;
		ts->abs_y_max = GOODIX_MAX_HEIGHT;
		if (ts->swapped_x_y)
			swap(ts->abs_x_max, ts->abs_y_max);
		ts->int_trigger_type = GOODIX_INT_TRIGGER;
		ts->max_touch_num = GOODIX_MAX_CONTACTS;
		return;
	}

	ts->abs_x_max = get_unaligned_le16(&config[RESOLUTION_LOC]);
	ts->abs_y_max = get_unaligned_le16(&config[RESOLUTION_LOC + 2]);
	dev_info(&ts->client->dev, "ts->abs_x_max = 0x%x, ts->abs_y_max = 0x%x\n",
			ts->abs_x_max, ts->abs_y_max);

	if (ts->swapped_x_y)
		swap(ts->abs_x_max, ts->abs_y_max);
	ts->int_trigger_type = config[TRIGGER_LOC] & 0x03;
	ts->max_touch_num = config[MAX_CONTACTS_LOC] & 0x0f;
	if (!ts->abs_x_max || !ts->abs_y_max || !ts->max_touch_num) {
		dev_err(&ts->client->dev,
			"Invalid config, using defaults\n");
		ts->abs_x_max = GOODIX_MAX_WIDTH;
		ts->abs_y_max = GOODIX_MAX_HEIGHT;
		if (ts->swapped_x_y)
			swap(ts->abs_x_max, ts->abs_y_max);
		ts->max_touch_num = GOODIX_MAX_CONTACTS;
	}

	if (dmi_check_system(rotated_screen)) {
		ts->inverted_x = true;
		ts->inverted_y = true;
		dev_dbg(&ts->client->dev,
			 "Applying '180 degrees rotated screen' quirk\n");
	}
}

/**
 * goodix_read_version - Read goodix touchscreen version
 *
 * @ts: our goodix_ts_data pointer
 */
static int goodix_read_version(struct goodix_ts_data *ts)
{
	int error;
	u8 buf[6];
	char id_str[5];

	error = goodix_i2c_read(ts->client, GOODIX_REG_ID, buf, sizeof(buf));
	if (error) {
		dev_err(&ts->client->dev, "read version failed: %d\n", error);
		return error;
	}

	memcpy(id_str, buf, 4);
	id_str[4] = 0;
	if (kstrtou16(id_str, 10, &ts->id))
		ts->id = 0x1001;

	ts->version = get_unaligned_le16(&buf[4]);

	dev_info(&ts->client->dev, "ID %d, version: %04x\n", ts->id,
		 ts->version);

	return 0;
}

/**
 * goodix_i2c_test - I2C test function to check if the device answers.
 *
 * @client: the i2c client
 */
static int goodix_i2c_test(struct i2c_client *client)
{
	int retry = 0;
	int error;
	u8 test;

	while (retry++ < 2) {
		error = goodix_i2c_read(client, GOODIX_REG_CONFIG_DATA,
					&test, 1);
		if (!error) {
			return 0;
		}

		dev_err(&client->dev, "i2c test failed attempt %d: %d\n",
			retry, error);
		msleep(120);
	}

	return error;
}

/**
 * goodix_request_input_dev - Allocate, populate and register the input device
 *
 * @ts: our goodix_ts_data pointer
 *
 * Must be called during probe
 */
static int goodix_request_input_dev(struct goodix_ts_data *ts)
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

	input_mt_init_slots(ts->input_dev, ts->max_touch_num,
			    INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);

	ts->input_dev->name = "Goodix Capacitive Main TouchScreen";
	ts->input_dev->phys = "input/ts";
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0x0416;
	ts->input_dev->id.product = ts->id;
	ts->input_dev->id.version = ts->version;
#ifdef TOUCHPAD_EXTERNFLAG
	if (ts->tp_extern) {
		ts->input_dev->name = "Goodix Capacitive Extern TouchScreen";
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

/*******************************************************
Function:
Timer interrupt service routine for polling mode.
Input:
timer: timer struct pointer
Output:
Timer work mode.
HRTIMER_NORESTART: no restart mode
 *********************************************************/
static enum hrtimer_restart goodix_ts_timer_handler(struct hrtimer *timer)
{
	struct goodix_ts_data
		*ts = container_of(timer, struct goodix_ts_data, timer);

	queue_work(ts->goodix_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(0, (GTP_POLL_TIME + 6) * 1000000),
			HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}



static void goodix_ts_work_func(struct work_struct *work)
{
	//int ret = 0;
	struct goodix_ts_data *ts = NULL;
	//struct i2c_adapter *adapter = ts->client->adapter;


    ts = container_of(work, struct goodix_ts_data, work);

	/* Returns 1 on success and 0 on contention */
	//ret = adapter->lock_ops->trylock_bus(adapter, 0);
	//if (ret) {

	if (down_interruptible(&i2c_sem))
		return;
	select_ti_link(ts);

	goodix_process_events(ts);

	if (goodix_i2c_write_u8(ts->client, GOODIX_READ_COOR_ADDR, 0) < 0)
		dev_err(&ts->client->dev, "I2C write end_cmd error\n");
	up(&i2c_sem);

    return;
}

/**
 * goodix_configure_dev - Finish device initialization
 *
 * @ts: our goodix_ts_data pointer
 *
 * Must be called from probe to finish initialization of the device.
 * Contains the common initialization code for both devices that
 * declare gpio pins and devices that do not. It is either called
 * directly from probe or from request_firmware_wait callback.
 */
static int goodix_configure_dev(struct goodix_ts_data *ts)
{
	int error;
	unsigned int tps = 0;
	unsigned int kps_temp[0x30] = {0};
	int i = 0;

	error = device_property_read_u32(&ts->client->dev,
						   "touchscreen-supported", &tps);
	if (!error)
		dev_info(&ts->client->dev, "TP supported = %d\n", tps);
	else {
		dev_err(&ts->client->dev, "Get tps Fail!\n");
		return -1;
	}

	error = device_property_read_u32_array(&ts->client->dev,
			        "tps-info", kps_temp, TOUCHPAD_INFO * tps);

	if (!error) {
		memcpy(ts->kps_info, kps_temp, TOUCHPAD_INFO * tps * sizeof(unsigned int));
	} else {
		dev_err(&ts->client->dev, "Get tps array fail!\n");
		return -1;
	}

	goodix_read_config(ts);

	for (i = 0; i < tps; i++) {
		dev_info(&ts->client->dev, "ts->abs_x_max  = %d, ts->kps_info[%d][0] = %d\n", ts->abs_x_max ,i, ts->kps_info[i][0]);
		if (ts->abs_x_max == ts->kps_info[i][0])
			break;
	}
	if (ts->abs_y_max == ts->kps_info[i][1]) {
		dev_info(&ts->client->dev, "Match Touchscreen!\n");
		ts->swapped_x_y = ts->kps_info[i][2];
		ts->inverted_x= ts->kps_info[i][3];
		ts->inverted_y= ts->kps_info[i][4];
		if (ts->swapped_x_y)
			swap(ts->abs_x_max, ts->abs_y_max);
	} else  {
		dev_err(&ts->client->dev, "Not Match Touchscreen!\n");
		return -1;
	}

	error = goodix_request_input_dev(ts);
	if (error)
		return error;

	if (ts->use_irq) {
		ts->irq_flags = goodix_irq_flags[ts->int_trigger_type] | IRQF_ONESHOT;
		error = goodix_request_irq(ts);
		if (error) {
			dev_err(&ts->client->dev, "request IRQ failed: %d\n", error);
			return error;
		}
		pr_info("%s,%d, irq mode\n",__func__,__LINE__);
	} else {
		pr_info("%s,%d, polling mode\n",__func__,__LINE__);

		ts->goodix_wq = create_singlethread_workqueue("goodix_wq");
		INIT_WORK(&ts->work, goodix_ts_work_func);

		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = goodix_ts_timer_handler;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

	return 0;
}

/**
 * goodix_config_cb - Callback to finish device init
 *
 * @ts: our goodix_ts_data pointer
 *
 * request_firmware_wait callback that finishes
 * initialization of the device.
 */
static void goodix_config_cb(const struct firmware *cfg, void *ctx)
{
	struct goodix_ts_data *ts = ctx;
	int error;

	if (cfg) {
		/* send device configuration to the firmware */
		error = goodix_send_cfg(ts, cfg);
		if (error)
			goto err_release_cfg;
	}

	goodix_configure_dev(ts);

err_release_cfg:
	release_firmware(cfg);
	complete_all(&ts->firmware_loading_complete);
}

int of_get_config(struct goodix_ts_data *ts)
{
	int value = 0;
	int ret = 0;

#ifdef TOUCHPAD_KEYSUPPORT
	int key_local[TPKEY_ARRAY_SIZE] = {0};

	device_property_read_u32(&ts->client->dev,
						   "touchscreen-buttons", &ts->buttons);

	if (ts->buttons && ts->buttons < MAX_TOUCHPAD_KEYNUM) {
		ret = device_property_read_u32_array(&ts->client->dev,
				"tpd-key-event", ts->key_event, ts->buttons);
		ret = device_property_read_u32_array(&ts->client->dev,
				"tpd-key-button", key_local, 4 * ts->buttons);
		if (!ret)
			memcpy(ts->key_range, key_local, 4 * TPKEY_ARRAY_SIZE);
	}

	dev_info(&ts->client->dev, "[Driver]TP key button = %d\n", ts->buttons);
#endif
	device_property_read_u32(&ts->client->dev,
			"irq_mode", &value);
	ts->use_irq = value;

	ret = device_property_read_u32(&ts->client->dev,
			"slave_addr", &value);
	if (!ret) {
		ts->client->addr = value;
		pr_info("%s,%d, value = 0x%x\n",__func__,__LINE__, ts->client->addr);
	}
#ifdef TOUCHPAD_EXTERNFLAG
	device_property_read_u32(&ts->client->dev,
			"touchscreen-extern", &ts->tp_extern);

#endif
	return 0;
}

static int ti_read(struct i2c_client *client,char addr, char reg, char *buf, int len)
{
	struct i2c_msg msgs[2];
	char wbuf = reg;
	int ret;

	msgs[0].flags = 0;
	msgs[0].addr  = addr;
	msgs[0].len   = 1;
	msgs[0].buf   = (u8 *)&wbuf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = addr;
	msgs[1].len   = len;
	msgs[1].buf   = buf;

	ret = i2c_transfer(client->adapter, msgs, 2);
	return ret < 0 ? ret : (ret != ARRAY_SIZE(msgs) ? -EIO : 0);
}

#define NO_LINK_SUPPORT_ACK		0x1
#define NO_LINK_SUPPORT_NOACK	0x2
#define LINK_SUPPORT_ACK		0x3
#define LINK_SUPPORT_NOACK		0x4

static void set_ti_as_pass_though(struct goodix_ts_data *ts)
{
	struct i2c_client *client = ts->client;
	char buffer[8];

	if (!ts->ti_link)
		return ;

	buffer[0] = 0x3;
	buffer[1] = 0x9a;

	ti_write(client, ts->ti_serializer, buffer, 2);
	pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

	buffer[0] = 0x17;
	buffer[1] = 0x9e;
	ti_write(client, ts->ti_serializer, buffer, 2);
	pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);

	buffer[0] = 0x05;
	buffer[1] = 0x9e;
	ti_write(client, ts->ti_deserializer, buffer, 2);
	pr_info("write 0x%x to 0x%x finish! \n", buffer[1], buffer[0]);
}

static int init_ti_link(struct goodix_ts_data *ts)
{
	struct i2c_client *client = ts->client;
	char buffer[8];
	unsigned int value = 0;
	int ret = 0;

	ts->ti_link = 0;
	ret = device_property_read_u32(&client->dev,
			"ti-link", &value);
	if (!ret)
		ts->ti_link = value;
	pr_info("%s,%d, ti-link = %d\n",
			__func__,__LINE__, ts->ti_link);

	if (ts->ti_link) {
		ret = device_property_read_u32(&client->dev,
				"ti-serializer", &value);
		if (!ret)
			ts->ti_serializer = value;

		ret = device_property_read_u32(&client->dev,
				"ti-deserializer", &value);
		if (!ret)
			ts->ti_deserializer = value;
	} else {
		pr_info("No Ti link device!\n");
	}

	/* serializer */
	buffer[0] = 0x1e;
	buffer[1] = ts->ti_link;

	ret = ti_read(client, ts->ti_serializer, 0x0, buffer, 1);
	if (!ret) {
		pr_info("read Ti address = 0x%x \n", buffer[0]);
		if (ts->ti_link)
			ret = LINK_SUPPORT_ACK;
		else
			ret = NO_LINK_SUPPORT_ACK;
	} else {
		pr_info("read Ti address fail!\n");
		if (ts->ti_link)
			ret = LINK_SUPPORT_NOACK;
		else
			ret = NO_LINK_SUPPORT_NOACK;
	}

	if (ret == LINK_SUPPORT_ACK)
		    set_ti_as_pass_though(ts);

	return ret;
}

#define GOODIX_SLAVE_ADDR1   0x5d
#define GOODIX_SLAVE_ADDR2   0x14
static int goodix_ts_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct goodix_ts_data *ts;
	int error;
	int ret;

	dev_dbg(&client->dev, "I2C Address: 0x%02x\n", client->addr);

	if (!init_sem_flag) {
		sema_init(&i2c_sem, 1);
		init_sem_flag = 1;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C check functionality failed.\n");
		return -ENXIO;
	}

	ts = devm_kzalloc(&client->dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	dev_info(&client->dev, "Goodix TP id = %d\n", client->addr);

	client->addr = GOODIX_SLAVE_ADDR1;
	ts->client = client;

	ret = init_ti_link(ts);
	if (ret == NO_LINK_SUPPORT_ACK) {
		dev_err(&client->dev, "No Ti link support but ack, goodix probe finish!\n");
		return ret;
	}

	if (ret == LINK_SUPPORT_NOACK) {
		dev_err(&client->dev, "Ti link support but noack, goodix probe finish!\n");
		return ret;
	}
	ti_touchscreen_flag = 1;

	set_ti_rst_output_mode(ts);

	i2c_set_clientdata(client, ts);
	init_completion(&ts->firmware_loading_complete);

	of_get_config(ts);

	error = goodix_get_gpio_config(ts);
	if (error)
		return error;


	if (ts->gpiod_int && ts->gpiod_rst) {
		/* reset the controller */
		error = goodix_reset(ts);
		if (error) {
			dev_err(&client->dev, "Controller reset failed.\n");
			return error;
		}
	}
	/* set rst as local mode */
	set_ti_rst_local_mode(ts);

	error = goodix_i2c_test(client);
	if (error) {
		dev_err(&client->dev, "I2C communication failure: %d\n", error);
		return error;
	}

	error = goodix_read_version(ts);
	if (error) {
		dev_err(&client->dev, "Read version failed.\n");
		return error;
	}

	ts->cfg_len = goodix_get_cfg_len(ts->id);

	if (ts->gpiod_int && ts->gpiod_rst) {
		/* update device config */
		ts->cfg_name = devm_kasprintf(&client->dev, GFP_KERNEL,
					      "goodix_%d_cfg.bin", ts->id);
		if (!ts->cfg_name)
			return -ENOMEM;

		error = request_firmware_nowait(THIS_MODULE, true, ts->cfg_name,
						&client->dev, GFP_KERNEL, ts,
						goodix_config_cb);
		if (error) {
			dev_err(&client->dev,
				"Failed to invoke firmware loader: %d\n",
				error);
			return error;
		}
		return 0;
	} else {
		error = goodix_configure_dev(ts);
		if (error)
			return error;
	}


	return 0;
}

static int goodix_ts_remove(struct i2c_client *client)
{
	struct goodix_ts_data *ts = i2c_get_clientdata(client);

	if (ts->gpiod_int && ts->gpiod_rst)
		wait_for_completion(&ts->firmware_loading_complete);

	return 0;
}

static int __maybe_unused goodix_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct goodix_ts_data *ts = i2c_get_clientdata(client);

	/* We need gpio pins to suspend/resume */
	if (!ts->gpiod_int || !ts->gpiod_rst) {
		disable_irq(client->irq);
		return 0;
	}

	wait_for_completion(&ts->firmware_loading_complete);

	/* Free IRQ as IRQ pin is used as output in the suspend sequence */
	goodix_free_irq(ts);

	return 0;
}

static int __maybe_unused goodix_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
	int error;

	error = goodix_request_irq(ts);
	if (error)
		return error;

	return 0;
}

static SIMPLE_DEV_PM_OPS(goodix_pm_ops, goodix_suspend, goodix_resume);

static const struct i2c_device_id goodix_ts_id[] = {
	{ "GDIX1001:00", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, goodix_ts_id);

#ifdef CONFIG_ACPI
static const struct acpi_device_id goodix_acpi_match[] = {
	{ "GDIX1001", 0 },
	{ "GDIX1002", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, goodix_acpi_match);
#endif

#ifdef CONFIG_OF
static const struct of_device_id goodix_of_match[] = {
	{ .compatible = "goodix,gt911" },
	{ .compatible = "goodix,gt9110" },
	{ .compatible = "goodix,gt912" },
	{ .compatible = "goodix,gt927" },
	{ .compatible = "goodix,gt9271" },
	{ .compatible = "goodix,gt928" },
	{ .compatible = "goodix,gt967" },
	{ }
};
MODULE_DEVICE_TABLE(of, goodix_of_match);
#endif

static struct i2c_driver goodix_ts_driver = {
	.probe = goodix_ts_probe,
	.remove = goodix_ts_remove,
	.id_table = goodix_ts_id,
	.driver = {
		.name = "Goodix-TS",
		.acpi_match_table = ACPI_PTR(goodix_acpi_match),
		.of_match_table = of_match_ptr(goodix_of_match),
		.pm = &goodix_pm_ops,
	},
};
module_i2c_driver(goodix_ts_driver);

MODULE_AUTHOR("Benjamin Tissoires <benjamin.tissoires@gmail.com>");
MODULE_AUTHOR("Bastien Nocera <hadess@hadess.net>");
MODULE_DESCRIPTION("Goodix touchscreen driver");
MODULE_LICENSE("GPL v2");
