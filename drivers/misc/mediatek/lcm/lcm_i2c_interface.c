/*
 * Copyright (C) 2018  AutoChips Inc.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/of.h>
#include <linux/module.h>

#include "lcm_i2c_interface.h"

/**********************************************************
  *
  *   [I2C Slave Setting]
  *
  *********************************************************/
#define INTERFACE_SLAVE_ADDR	(0x0e >> 1)  /*947 slave addr*/
#define INTERFACE_IN_I2C_CHANNEL 2

/**********************************************************
  *
  *   [Global Variable]
  *
  *********************************************************/
static struct i2c_client *interface_client_list[LCM_ID_MAX] = {NULL, NULL};

extern unsigned int g_mobilelog;

static const struct i2c_device_id lcm_transfer_ic_i2c_id[] = { {"LCM_BRIDGE_IC", 0}, {} };
static int interface_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int interface_driver_remove(struct i2c_client *client);

static const struct of_device_id lcm_transfer_ic_of_table[] = {
	{.compatible = "autochips,lcm_bridge_ic"},
	{},
};

static struct i2c_driver interface_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "lcm_bridge_ic",
			.of_match_table = lcm_transfer_ic_of_table,
		   },
	.probe = interface_driver_probe,
	.remove = interface_driver_remove,
	.id_table = lcm_transfer_ic_i2c_id,
};

/**********************************************************
  *
  *   [I2C Function For Read/Write interface for Transfer Ic]
  *
  *********************************************************/
int interface_read_byte(int lcm_id, unsigned char addr, unsigned char cmd, unsigned char *out)
{
	char cmd_buf = 0x00;
	int ret = 0;

	if (lcm_id != LCM_ID_PRIMARY && lcm_id != LCM_ID_SECONDARY) {
		LCM_INTERFACE_ERROR("lcm_id = %d error\n", lcm_id);

		return -EINVAL;
	}

	if ( !interface_client_list[lcm_id] ) {
		LCM_INTERFACE_ERROR("interface_client NULL!\n");

		return -EINVAL;
	}

	cmd_buf = cmd;
	if (out == NULL) {
		LCM_INTERFACE_ERROR("parameter is invalid! \n");

		return -EINVAL;
	}

	interface_client_list[lcm_id]->flags = 0;
	interface_client_list[lcm_id]->addr = addr;
	ret = i2c_master_send(interface_client_list[lcm_id], &cmd_buf, 1);
	if (ret != 1) {
		LCM_INTERFACE_ERROR("send addr:0x%x error! \n", cmd_buf);

		return -EINVAL;
	}

	interface_client_list[lcm_id]->flags = 1;
	ret = i2c_master_recv(interface_client_list[lcm_id], (char *)out, 1);
	if (ret != 1) {
		LCM_INTERFACE_ERROR("read value:0x%x! ret=%x\n", *out, ret);

		return -EINVAL;
	}

	/*new_client->ext_flag = 0; */
	/*LCM_INTERFACE_INFO("read value:0x%x at addr:0x%x! \n", *out, cmd_buf);*/

	return 0;
}

int interface_write_byte(int lcm_id, unsigned char addr, unsigned char cmd, unsigned char in)
{
	char write_data[2] = { 0 };
	int ret = 0;

	write_data[0] = cmd;
	write_data[1] = in;

	if (lcm_id != LCM_ID_PRIMARY && lcm_id != LCM_ID_SECONDARY) {
		LCM_INTERFACE_ERROR("lcm_id = %d error\n", lcm_id);

		return -EINVAL;
	}

	if (!interface_client_list[lcm_id]) {
		LCM_INTERFACE_ERROR("interface_client NULL!\n");

		return -EINVAL;
	}
	interface_client_list[lcm_id]->addr = addr;
	ret = i2c_master_send(interface_client_list[lcm_id], write_data, 2);
	if (ret != 2) {
		LCM_INTERFACE_ERROR("addr:0x%x, value:0x%x error!\n", write_data[0], write_data[1]);

		return -EINVAL;
	}

	if (g_mobilelog) {
		write_data[1] = 0;
		ret = interface_read_byte(lcm_id, addr, cmd, &write_data[1]);
		if (ret) {
			LCM_INTERFACE_ERROR("ret=%d\n", ret);

			return-EINVAL;
		}

		if ((unsigned char)write_data[1] != in) {
			LCM_INTERFACE_ERROR("addr:0x%02x , write:0x%02x, read:0x%02x!\n", cmd, in, write_data[1]);

			return -EINVAL;
		} else {
			LCM_INTERFACE_INFO("addr:0x%02x , write:0x%02x, read:0x%02x!\n", cmd, in, write_data[1]);

			return -EINVAL;
		}
	}
	/*LCM_INTERFACE_INFO("interface_write_byte addr:0x%x, value:0x%x success!\n", write_data[0], write_data[1]);*/

	return 0;
}

int interface_read_buffer(int lcm_id, unsigned char addr, unsigned char cmd, unsigned char *out, int len)
{
	int ret = 0;

	if (lcm_id != LCM_ID_PRIMARY && lcm_id != LCM_ID_SECONDARY) {
		LCM_INTERFACE_ERROR("lcm_id = %d error\n", lcm_id);

		return -EINVAL;
	}

	if ( !interface_client_list[lcm_id] ) {
		LCM_INTERFACE_ERROR("interface_client NULL!\n");

		return -EINVAL;
	}

	if (out == NULL) {
		LCM_INTERFACE_ERROR("parameter is invalid! \n");

		return -EINVAL;
	}

	interface_client_list[lcm_id]->flags = 0;
	interface_client_list[lcm_id]->addr = addr;
	ret = i2c_master_send(interface_client_list[lcm_id], &cmd, 1);
	if (ret != 1) {
		LCM_INTERFACE_ERROR("send addr:0x%x error! \n", cmd);

		return -EINVAL;
	}

	interface_client_list[lcm_id]->flags = 1;
	ret = i2c_master_recv(interface_client_list[lcm_id], (char *)out, len);
	if (ret != len) {
		LCM_INTERFACE_ERROR("read value:0x%x! ret=%x\n", *out, ret);

		return -EINVAL;
	}

	/*new_client->ext_flag = 0; */
	/*LCM_INTERFACE_INFO("read value:0x%x at addr:0x%x! \n", *out, cmd_buf);*/

	return ret;
}
EXPORT_SYMBOL(interface_read_buffer);

int interface_write_buffer(int lcm_id, unsigned char addr, unsigned char *buffer, int len)
{
	int ret = 0;
	char *write_data = NULL;

	if (lcm_id != LCM_ID_PRIMARY && lcm_id != LCM_ID_SECONDARY) {
		LCM_INTERFACE_ERROR("lcm_id = %d error\n", lcm_id);

		return -EINVAL;
	}

	if ( !interface_client_list[lcm_id] ) {
		LCM_INTERFACE_ERROR("interface_client_list[%d] NULL!\n", lcm_id);

		return -EINVAL;
	}


	interface_client_list[lcm_id]->addr = addr;
	ret = i2c_master_send(interface_client_list[lcm_id], buffer, len);
	if (ret != len) {
		LCM_INTERFACE_ERROR("addr:0x%x, value:%s error!\n", addr, buffer);

		return -EINVAL;
	}

	if (g_mobilelog) {
		write_data = (char *)kmalloc(len + 1, GFP_KERNEL);
		ret = interface_read_buffer(lcm_id, addr, buffer[0], write_data, len);
		if (ret) {
			LCM_INTERFACE_ERROR("ret=%d\n", ret);
			kfree(write_data);
			return -EINVAL;
		}

		if (strcmp(write_data, buffer)) {
			LCM_INTERFACE_ERROR("addr:0x%02x , write:%s, read:%s!\n", addr, buffer, write_data);
			kfree(write_data);
			return -EINVAL;
		} else {
			LCM_INTERFACE_INFO("addr:0x%02x , write:%s, read:%s!\n", addr, buffer, write_data);
			kfree(write_data);
			return -EINVAL;
		}

		kfree(write_data);
	}


	return ret;
}
EXPORT_SYMBOL(interface_write_buffer);


static int interface_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device_node *devnode;
	int lcm_id = -1;

	LCM_INTERFACE_INFO("name=%s addr=0x%x\n", client->name, client->addr);

	devnode = client->dev.of_node;
	if (0 != of_property_read_u32(devnode, "lcm_id", &lcm_id)) {
		LCM_INTERFACE_ERROR("get lcm_id fail!\r\n");
		return -1;
	}

	if (lcm_id != LCM_ID_PRIMARY && lcm_id != LCM_ID_SECONDARY) {
		LCM_INTERFACE_ERROR("lcm_id = %d error!\r\n", lcm_id);
		return -1;
	}

	interface_client_list[lcm_id] = client;

	/*
	 * The UJC201 dtbo declares its only bridge (lcm_bridge_ic@2d on i2c0) with lcm_id = 0, which is
	 * LCM_ID_SECONDARY here, while the primary panel code talks to LCM_ID_PRIMARY: the bridge was never
	 * programmed again after its power was cut (black panel after a real suspend). A lone bridge serves
	 * the primary panel too, until a real primary one shows up.
	 */
	if (lcm_id == LCM_ID_SECONDARY && !interface_client_list[LCM_ID_PRIMARY]) {
		interface_client_list[LCM_ID_PRIMARY] = client;
		LCM_INTERFACE_INFO("lcm_id = %d also used as the primary bridge\n", lcm_id);
	}

	LCM_INTERFACE_INFO("lcm_id = %d\n", lcm_id);

	return 0;
}

static int interface_driver_remove(struct i2c_client *client)
{
	LCM_INTERFACE_INFO("enter\n");

	if (client == interface_client_list[LCM_ID_PRIMARY]) {
		interface_client_list[LCM_ID_PRIMARY] = NULL;
		LCM_INTERFACE_INFO("LCM_ID_PRIMARY = %d\n", LCM_ID_PRIMARY);
	}
	if (client == interface_client_list[LCM_ID_SECONDARY]) {
		interface_client_list[LCM_ID_SECONDARY] = NULL;
		LCM_INTERFACE_INFO("LCM_ID_SECONDARY = %d\n", LCM_ID_SECONDARY);
	}

	i2c_unregister_device(client);

	return 0;
}

static int __init transfer_ic_interface_init(void)
{
	LCM_INTERFACE_INFO("enter\n");

	if (i2c_add_driver(&interface_driver) != 0) {
		LCM_INTERFACE_ERROR("failed to register interface i2c driver.\n");
	}else {
		LCM_INTERFACE_INFO("Success to register interface i2c driver.\n");
	}

	return 0;
}

static void __exit transfer_ic_interface_exit(void)
{
	i2c_del_driver(&interface_driver);
}

arch_initcall(transfer_ic_interface_init);
module_exit(transfer_ic_interface_exit);

MODULE_DESCRIPTION("AutoChips Bridge Chip Interface Driver");
MODULE_LICENSE("GPL");
