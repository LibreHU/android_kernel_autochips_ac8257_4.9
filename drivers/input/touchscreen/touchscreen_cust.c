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

#define TS_INFO		printk

#include "touchscreen_cust.h"
#include "atc_customize.h"

#define TOUCH_MAX_POINT 5
#define TOUCHPOINT_INFO 5
#define TOUCHPOINT_BUF  64
#define TOUCHPOINT0 (0x1 << 0)
#define TOUCHPOINT1 (0x1 << 1)
#define TOUCHPOINT2 (0x1 << 2)
#define TOUCHPOINT3 (0x1 << 3)
#define TOUCHPOINT4 (0x1 << 4)
#define TOUCHPOWER  (0x1 << 5)
#define TOUCHPOINT_ADDR 0x2


#define DISPLAY_CMD_ID_START           0x40
#define DISPLAY_CMD_ID_SELF_CHECK     (DISPLAY_CMD_ID_START + 0)
#define DISPLAY_CMD_ID_TOUCH_POINT    (DISPLAY_CMD_ID_START + 1)
#define DISPLAY_CMD_ID_TOUCH_POINT_EXT    (DISPLAY_CMD_ID_START + 2)
#define DISPLAY_CMD_ID_TOUCH_POINT_OLD    (DISPLAY_CMD_ID_START + 3)
#define DISPLAY_CMD_ID_SCREEN_STATUS   (DISPLAY_CMD_ID_START + 6)
#define DISPLAY_CMD_ID_SCREEN_FAIL     (DISPLAY_CMD_ID_START + 8)
#define DISPLAY_CMD_ID_INPUT_VOLTAGE    (DISPLAY_CMD_ID_START + 9)
#define DISPLAY_CMD_ID_PRODUCT_ID       (DISPLAY_CMD_ID_START + 0xa)
#define DISPLAY_CMD_ID_VERSION           (DISPLAY_CMD_ID_START + 0xb)
#define DISPLAY_CMD_ID_REQUEST_BACKLIGHT  (DISPLAY_CMD_ID_START + 0xc)

#define HOST_CMD_ID_START                0x80
#define HOST_CMD_ID_STARTUP_STATUS       (HOST_CMD_ID_START + 0)
#define HOST_STARTUP_WAITING  0
#define HOST_STARTUP_READY    1
#define HOST_CMD_ID_AQUIRE_SCREEN_INFO    (HOST_CMD_ID_START + 1)
#define HOST_CMD_ID_SCREEN_MODE            (HOST_CMD_ID_START + 0x10)
#define SCREEN_MODE_STANDBY            0x0
#define SCREEN_MODE_ON                 0x1
#define SCREEN_MODE_START_POWER_OFF    0x2


#define POINT_STATUS_DOWN 0xC0
#define POINT_STATUS_UP   0xA0
#define POINT_STATUS_MOVE 0x90


void reportPoints(struct customize_ts_data *ts, int num,struct touchpoint *points, int cmd)
{
	int position = 0;
	struct touchpoint *cursor;

	for(position = 0; position < num; position++)
	{
		cursor = points + position;
		if((cursor->point_status == POINT_STATUS_DOWN)
				|| (cursor->point_status == POINT_STATUS_MOVE))
		{
			ts->touch_status = POINT_STATUS_DOWN;
			//input_mt_slot(ts->input_dev, cursor->point_id);
			//input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, cursor->x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, cursor->y);
			input_report_abs(ts->input_dev,ABS_PRESSURE,1);
			input_report_key(ts->input_dev,BTN_TOUCH,1);
			ts->cursor_last.x = cursor->x;
			ts->cursor_last.y = cursor->y;
			if (num != 1)
				input_mt_sync(ts->input_dev);
			//TS_INFO("[pt][%d]x=%d,y=%d down\n", cursor->point_id, cursor->x, cursor->y);
		}
		else if(cursor->point_status == POINT_STATUS_UP)
		{
			if (ts->touch_status == POINT_STATUS_UP && num == 1 && cmd == 0x43) {
				//input_mt_slot(ts->input_dev, position);
				//input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
				//input_mt_slot(ts->input_dev, 0);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, cursor->x);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, cursor->y);
				input_report_abs(ts->input_dev,ABS_PRESSURE,1);
				input_report_key(ts->input_dev,BTN_TOUCH,1);
				//input_mt_sync_frame(ts->input_dev);
				input_sync(ts->input_dev);
				//TS_INFO("--[pt][%d]x=%d,y=%d down\n", cursor->point_id, cursor->x, cursor->y);

				input_report_abs(ts->input_dev,ABS_PRESSURE,0);
				input_report_key(ts->input_dev,BTN_TOUCH,0);
				input_sync(ts->input_dev);

				ts->touch_status = POINT_STATUS_UP;
				//TS_INFO("[pt][%d]x=%d,y=%d up\n", cursor->point_id, cursor->x, cursor->y);
				return;
			}
			if (ts->touch_status == POINT_STATUS_UP && num == 1 && cmd == 0x41)  {
				if (ts->cursor_last.x != cursor->x  && ts->cursor_last.y != cursor->y) {
					input_report_abs(ts->input_dev, ABS_MT_POSITION_X, cursor->x);
					input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, cursor->y);
					input_report_abs(ts->input_dev,ABS_PRESSURE,1);
					input_report_key(ts->input_dev,BTN_TOUCH,1);
					//input_mt_sync_frame(ts->input_dev);
					input_sync(ts->input_dev);
					ts->cursor_last.x = cursor->x;
					ts->cursor_last.y = cursor->y;
					//printk("--[pt][%d]x=%d,y=%d down\n", cursor->point_id, cursor->x, cursor->y);

					input_report_abs(ts->input_dev,ABS_PRESSURE,0);
					input_report_key(ts->input_dev,BTN_TOUCH,0);
					input_sync(ts->input_dev);

					ts->touch_status = POINT_STATUS_UP;
					//TS_INFO("[pt][%d]x=%d,y=%d up\n", cursor->point_id, cursor->x, cursor->y);
				}
				return;
			}
			input_report_abs(ts->input_dev,ABS_PRESSURE,0);
			input_report_key(ts->input_dev,BTN_TOUCH,0);
			ts->touch_status = POINT_STATUS_UP;
			//TS_INFO("[pt][%d]x=%d,y=%d up\n", cursor->point_id, cursor->x, cursor->y);
		}
		else
		{
			TS_INFO("[error] point status 0x%x\n",cursor->point_status);
		}
	}
	input_sync(ts->input_dev);
}

int process_incoming_data(struct customize_ts_data *ts, char * buf, int size)
{
	char command_id = 0;
	int result = 0;
	//int backlight = 0;
	int point_number = 0;
	int position = 0;
	int x = 0,y = 0;
	int ext_len = 0;
	unsigned char ext_buf[64] = {0};
	struct touchpoint pointbuf[10];

	command_id = buf[0];
	switch(command_id)
	{
		case DISPLAY_CMD_ID_TOUCH_POINT_OLD:
			{
				point_number = buf[1];

				if(point_number > 10)
				{
					//TS_INFO("[TP]error data %d\n",point_number);
					return -1;
				}
				for(position = 0; position < point_number; position++)
				{
					pointbuf[position].point_id     = (int)buf[6*position + 2];
					pointbuf[position].point_status = (int)buf[6*position + 3];
					x = (int)buf[6*position + 4];
					x = x << 8;
					x |= (int)buf[6*position + 5];
					pointbuf[position].x = x;
					y = (int)buf[6*position + 6];
					y = y << 8;
					y |= (int)buf[6*position + 7];
					pointbuf[position].y = y;
				}

				reportPoints(ts, point_number, pointbuf, command_id);
			}
			break;
		case DISPLAY_CMD_ID_TOUCH_POINT:
			{
				point_number = buf[1];
				//TS_INFO("touch %d points\n",point_number);

				if(point_number > 2)
				{
					point_number = 2;
				}

				for(position = 0; position < point_number; position++)
				{
					pointbuf[position].point_id     = (int)buf[6*position + 2];
					pointbuf[position].point_status = (int)buf[6*position + 3];
					x = (int)buf[6*position + 4];
					x = x << 8;
					x |= (int)buf[6*position + 5];
					pointbuf[position].x = x;
					y = (int)buf[6*position + 6];
					y = y << 8;
					y |= (int)buf[6*position + 7];
					pointbuf[position].y = y;
				}

				ext_len = buf[14];
				if(ext_len)
				{
					TS_INFO("[TP]need to process more %d data\n",ext_len);
					//readFromBus(pScreen, ext_buf,ext_len, DISPLAY_CMD_ID_TOUCH_POINT);
					TS_INFO("[TP]ext[0] = 0x%x,0x%x\n",ext_buf[0],ext_buf[1]);
				}
				else
				{
					reportPoints(ts, point_number, pointbuf, command_id);
				}
			}
			break;

		case DISPLAY_CMD_ID_TOUCH_POINT_EXT:
			{
				point_number = buf[1];
				TS_INFO("get ext frame %d\n",point_number);
				if(point_number > 8)
				{
					TS_INFO("[TP]error data %d\n",point_number);
					return -1;
				}

				for(position = 0; position < point_number - 2; position++)
				{
					pointbuf[position + 2].point_id     = (int)buf[6*position + 1];
					pointbuf[position + 2].point_status = (int)buf[6*position + 2];
					x = (int)buf[6*position + 3];
					x = x << 8;
					x |= (int)buf[6*position + 4];
					pointbuf[position + 2].x = x;
					y = (int)buf[6*position + 5];
					y = y << 8;
					y |= (int)buf[6*position + 6];
					pointbuf[position + 2].y = y;
				}

				reportPoints(ts, point_number, pointbuf, command_id);
			}
			break;
		//case DISPLAY_CMD_ID_SELF_CHECK:
		//	sendHostStartUpStaus(pScreen,HOST_STARTUP_READY);
		//	break;

		case DISPLAY_CMD_ID_REQUEST_BACKLIGHT:
			TS_INFO("set screen mode\n");
			//setScreenMode(pScreen,SCREEN_MODE_ON,65535);
			break;

		case DISPLAY_CMD_ID_SCREEN_STATUS:
			//{
			//	break;
			//	backlight = buf[3];
			//	backlight = (backlight << 8) | buf[2];

			//	x = buf[5];
			//	x = (x << 8) | buf[6];
			//	y = buf[7];
			//	y = (y << 8) | buf[8];
			//}
			break;
		case DISPLAY_CMD_ID_SCREEN_FAIL:

			break;
		case DISPLAY_CMD_ID_INPUT_VOLTAGE:
			TS_INFO("[TP]input voltage 0x%x\n",buf[1]);
			break;
		case DISPLAY_CMD_ID_PRODUCT_ID:
			break;
		case DISPLAY_CMD_ID_VERSION:
			TS_INFO("[TP]HW Version 0x%x.0x%x\n",buf[1],buf[2]);
			TS_INFO("[TP]SW Version 0x%x.0x%x\n",buf[3],buf[4]);
			TS_INFO("[TP]SW BootProgram %d\n",buf[5]);
			break;
	}

	return result;
}

int ts_handle_irq(struct customize_ts_data *ts)
{
	int ret = 0;
	struct touuch_data * tsd = ts->private_data;
	struct i2c_client *client = ts->client;
	char touch_num = TOUCHPOINT_BUF;
	u8 touch_buffer[TOUCHPOINT_BUF] = {0};

	client->addr = tsd->touchpad_addr & 0xff;
	ret = ts->i2c_read(client, 0x0, touch_buffer, touch_num);
	if (ret) {
		TS_INFO("read TP status fail !\n");
		return 0;
	}

	process_incoming_data(ts, touch_buffer, touch_num);

	return 0;
}


#define CHECK_ADDRESS 0x3
int checkserializer(struct customize_ts_data *ts)
{
	int ret = 0, i = 0;
	struct touuch_data * tsd = ts->private_data;
	struct i2c_client *client = ts->client;
	u8 read_buffer = 0;

	if (!ts->i2c_read) {
		TS_INFO("i2c read func is not support!\n");
		return -1;
	}

	/* TI 927 */
	client->addr = tsd->serializer_addr & 0xff;

	/* try 3 times */
	for (i = 0; i < 3; i++) {
		ret = ts->i2c_read(client, CHECK_ADDRESS, &read_buffer, 1);
		if (ret) {
			TS_INFO("Check TI addr fail!\n");
			ret = -1;
		} else {
			TS_INFO("Check TI addr success !\n");
			return 0;
		}
	}

	client->addr = tsd->touchpad_addr & 0xff;

	return ret;
}


//#define CONFIG_SERIALIZER
#ifdef  CONFIG_SERIALIZER
#define SERIALIZER_CFG  3
int initserializer(struct customize_ts_data *ts)
{
	int ret = 0, i = 0;
	struct touuch_data * tsd = ts->private_data;
	struct i2c_client *client = ts->client;
	char sent_cfg[SERIALIZER_CFG][2] = {
		{0x3, 0xda},
		{0x7, 0x48},
		{0x8, 0x48}
	};

	/* Set touchpad addr to serializer */
	sent_cfg[1][1] = tsd->touchpad_addr << 0x1;
	sent_cfg[2][1] = tsd->touchpad_addr << 0x1;

	if (!ts->i2c_write)
		TS_INFO("i2c write func is not support!\n");

	/* TI 927 */
	client->addr = tsd->serializer_addr & 0xff;

	for (i = 0; i < SERIALIZER_CFG; i++) {
		ret = ts->i2c_write(client, sent_cfg[i][0], &sent_cfg[i][1], 1);
		if (ret) {
			TS_INFO("i2c write Serializer 0x%x <- 0x%x, fail\n",
					sent_cfg[i][0],sent_cfg[i][1]);
			return -1;
		} else {
			TS_INFO("i2c write Serializer 0x%x <- 0x%x, success\n",
					sent_cfg[i][0],sent_cfg[i][1]);
		}
	}
	client->addr = tsd->touchpad_addr & 0xff;
	return 0;
}

#define DESERIALIZER_CFG 2
static int initdeSerializer(struct customize_ts_data *ts)
{
	int ret = 0, i = 0;
	struct touuch_data * tsd = ts->private_data;
	struct i2c_client *client = ts->client;
	char sent_cfg[DESERIALIZER_CFG][2] = {
		{0x05, 0x9e},
		{0x21, 0x99},
	};

	if (!ts->i2c_write)
		TS_INFO("i2c write func is not support!\n");

	client->addr = tsd->deserializer_addr & 0xff;

	for (i = 0; i < DESERIALIZER_CFG; i++) {
		ret = ts->i2c_write(client, sent_cfg[i][0], &sent_cfg[i][1], 1);
		if (ret) {
			TS_INFO("i2c write deSerializer 0x%x <- 0x%x, fail\n",
					sent_cfg[i][0],sent_cfg[i][1]);
			return -1;
		} else {
			TS_INFO("i2c write deSerializer 0x%x <- 0x%x, success\n",
					sent_cfg[i][0],sent_cfg[i][1]);
		}
	}
	client->addr = tsd->touchpad_addr & 0xff;

	return 0;
}
#endif

static int init_ts(struct customize_ts_data *ts)
{
	struct touuch_data * tsd = NULL;
	int ret = 0;

    tsd = kmalloc(sizeof(*tsd), GFP_KERNEL);
    if (!ts)
        return -ENOMEM;

	ts->private_data = tsd;

    device_property_read_u32(&ts->client->dev,
            "serializer-addr", &tsd->serializer_addr);

    device_property_read_u32(&ts->client->dev,
            "deserializer-addr", &tsd->deserializer_addr);

    device_property_read_u32(&ts->client->dev,
            "touchpad-addr", &tsd->touchpad_addr);

	TS_INFO("serializer-addr = 0x%x, deserializer-addr = 0x%x, touchpad-addr= 0x%x\n",
			tsd->serializer_addr, tsd->deserializer_addr, tsd->touchpad_addr);

    device_property_read_u32(&ts->client->dev,
            "touchscreen-abs-x-max", &ts->abs_x_max);
    device_property_read_u32(&ts->client->dev,
            "touchscreen-abs-y-max", &ts->abs_y_max);
	TS_INFO("abs-x-max = 0x%x, abs-y-max = 0x%x \n",
		   ts->abs_x_max, ts->abs_y_max);

	ts->int_trigger_type = IRQ_TYPE_EDGE_FALLING;

	ret = checkserializer(ts);
#ifdef  CONFIG_SERIALIZER
	ret = initserializer(ts);
	if (ret)
		return ret;

	ret = initdeSerializer(ts);
#endif

	return ret;
}

static int deinit_ts(struct customize_ts_data *ts)
{
	struct touuch_data * tsd = ts->private_data;

	if (tsd)
		kfree(tsd);

	return 0;
}


int inputdev_cfg(struct customize_ts_data *ts)
{
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(ABS_X, ts->input_dev->absbit);
	set_bit(ABS_Y, ts->input_dev->absbit);
	set_bit(ABS_PRESSURE, ts->input_dev->absbit);

	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

	set_bit(ABS_MT_TRACKING_ID, ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MINOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);

	return 0;
}

int get_customize_operation(struct customize_ts_data *ts)
{
	ts->handle_irq = ts_handle_irq;
	ts->init_touchscreen  = init_ts;
	ts->inputdev_config = inputdev_cfg;
	ts->deinit_touchscreen = deinit_ts;

	return 0;
}

MODULE_DESCRIPTION("touchscreen driver");
MODULE_LICENSE("GPL v2");
