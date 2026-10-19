/*
* Copyright (c) 2019 AutoChips Inc.
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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/scatterlist.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/usb.h>
#include "musb_core.h"

#define RQ_ROLESIWTCH              0x51
#define RQTYPE_HTOD                0x40
#define RQ_VAULE                   0x01
#define RQ_GET_CAPABILITIES        0x53
#define RQTYPE_CAPABILITIES        0xc0
#define SUPPORT_CARPLAY            0x01
#define GET_CAPABILITIES_TIME       100

#define OTG_PATH                   "/mt_usb/musb-hdrc/"
#define MAX_RETRY_GET_CARPLAY_CAPABILITIES_TIMES                 10

struct carplay_dev {
	struct usb_interface *intf;
	struct usb_device *dev;
};

static struct kobject *musbcarplay_kobj;
bool carplayconnected = false;
bool needRoleSwitch = false;
bool usbCmdDone = false;
struct carplay_dev *apple_dev = NULL;
static struct mutex lock;
bool apple;

void setCarplayConnectStatus(bool status)
{
	printk("carplay  setCarplayConnectStatus carplayconnected:%d  status:%d\n",
	    carplayconnected,status);

	mutex_lock(&lock);
	carplayconnected = status;
	if(!status){
		if(apple_dev != NULL){
			apple_dev->dev = NULL;
			apple_dev->intf = NULL;
			kfree(apple_dev);
			apple_dev = NULL;
			apple = false;
		}
	}
	mutex_unlock(&lock);
}
EXPORT_SYMBOL(setCarplayConnectStatus);

bool getCarplayConnectStatus(void)
{
	printk("carplay  getCarplayStatus carplayconnected:%d \n", carplayconnected);

	return carplayconnected;
}
EXPORT_SYMBOL(getCarplayConnectStatus);

bool getRoleSwitchStatus(void)
{
	printk("carplay  getRoleSwitchStatus needRoleSwitch:%d\n", needRoleSwitch);

	return needRoleSwitch;
}
EXPORT_SYMBOL(getRoleSwitchStatus);

static struct class *g_carplay_class = NULL;
static atomic_t g_device_count;
static struct carplay_device
{
	const char    *name;
	struct device   *dev;
	int                   index;
}*g_carplay_dev;

static struct workqueue_struct	*carplay_wq;
struct work_struct	carplay_work;

static int carplay_uevt_init(void)
{
	g_carplay_dev = kzalloc(sizeof(struct carplay_device), GFP_KERNEL);
	g_carplay_class = class_create(THIS_MODULE, "carplay");
	g_carplay_dev->index = atomic_inc_return(&g_device_count);
	g_carplay_dev->name = "carplay";
	g_carplay_dev->dev = device_create(g_carplay_class, NULL, MKDEV(0, g_carplay_dev->index),
					   NULL, g_carplay_dev->name);

	return 0;
}
static int carplay_uevt_send(bool bconn)
{
	char *envp[2];

	if (bconn){
		envp[0] = "carplay=connect";
	} else{
		envp[0] = "carplay=disconnect";
	}
	envp[1] = NULL;
	if (g_carplay_dev != NULL) {
		printk("host carplay_uevt_send %d \n", bconn);
		kobject_uevent_env(&g_carplay_dev->dev->kobj, KOBJ_CHANGE, envp);
		return 0;
	}

	return -1;
}
static int carplay_uevt_exit(void)
{
	if ( (g_carplay_class != NULL) && (g_carplay_dev != NULL)) {
		device_destroy(g_carplay_class, MKDEV(0, g_carplay_dev->index));
		class_destroy(g_carplay_class);
		kfree(g_carplay_dev);
		g_carplay_class = NULL;
		g_carplay_dev = NULL;
	}

	return 0;
}

void startCarplayWork(void)
{
	if (carplay_wq != NULL)
		queue_work(carplay_wq, &carplay_work);
}
EXPORT_SYMBOL(startCarplayWork);


static void carplay_uevent_work(struct work_struct *work)
{
	//dump_stack();
	carplay_uevt_send(false);
}

static ssize_t musb_carplayenablestatus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = -EINVAL;

	ret = sprintf(buf , needRoleSwitch? "needRoleSwitch":"IPOD");
	printk("carplay musb_carplayenablestatus_show:%d %s\n", needRoleSwitch,buf);

	return ret;
}

static ssize_t musb_carplayenablestatus_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	int ret = -EINVAL;

	printk("carplay musb_carplaystatus_store:%s\n",buf);
	if((strcmp("carplay", buf) == 0)||(strcmp("carlife", buf) == 0)||(strcmp("carbit", buf) == 0))
		needRoleSwitch = true;
	else
		needRoleSwitch = false;

	ret = n;

	return ret;
}

static DEVICE_ATTR(carplayenablestatus, 0644, musb_carplayenablestatus_show, musb_carplayenablestatus_store);
static ssize_t musb_carplayconnectstatus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = -EINVAL;
	mutex_lock(&lock);
	ret = sprintf(buf , carplayconnected? "connected":"disconnected");
	mutex_unlock(&lock);
	printk("carplay musb_carplayconnectstatus_show:%d %s\n", carplayconnected,buf);
	return ret;
}
static ssize_t musb_carplayconnectstatus_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	int ret = -EINVAL;
	printk("carplay musb_carplayconnectstatus_store:%s\n",buf);
	mutex_lock(&lock);
	if((strcmp("connected", buf) == 0))
		carplayconnected = true;
	else if((strcmp("disconnected", buf) == 0))
		carplayconnected = false;
	mutex_unlock(&lock);

	ret = n;
	return ret;
}
static DEVICE_ATTR(carplayconnectstatus, 0644, musb_carplayconnectstatus_show, musb_carplayconnectstatus_store);
static ssize_t roleswicth_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = -EINVAL;
	return ret;
}

static ssize_t getCarplayCapabilities(unsigned int *wvalue)
{
	int ret = -EINVAL;

	if (apple_dev != NULL && apple_dev->dev != NULL) {
	    ret = usb_control_msg(apple_dev->dev,
			    usb_sndctrlpipe(apple_dev->dev, 0),
			    RQ_GET_CAPABILITIES,
			    RQTYPE_CAPABILITIES,
			    0x00,
			    0,
			    wvalue,
			    sizeof(unsigned int),
			    GET_CAPABILITIES_TIME);
	}
	pr_info("carplay GET_CAPABILITIES wValue(%d) ret(%d)\n", *wvalue, ret);

	return ret;
}

static ssize_t requestRoleSwitch(void)
{
	int ret = -EINVAL;

	if (apple_dev != NULL && apple_dev->dev != NULL) {
		if (getCarplayConnectStatus()) {
			ret = usb_control_msg(apple_dev->dev,
					usb_sndctrlpipe(apple_dev->dev, 0),
					RQ_ROLESIWTCH,
					RQTYPE_HTOD,
					RQ_VAULE,
					0,
					NULL,
					0,
					USB_CTRL_GET_TIMEOUT);
			if (ret < 0) {
				pr_info("carplay device role switch fail!\n");
			} else {
				pr_info("carplay device role switch success!\n");
			}
		}
	}

	return ret;
}

static ssize_t roleswicth_cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
	int ret = -EINVAL, err = -1;
	unsigned int capabilities_value = 0, retry = 0;
	printk("carplay role_switch_cmd_store:%s\n",buf);
	mutex_lock(&lock);
	if (apple_dev == NULL) {
		printk("no carplay device attach.\n");
		mutex_unlock(&lock);
		return ret;
	}

	if (!strncmp(buf, "role", 4)){
		printk("role switch cmd usbCmdDone:%d\n",usbCmdDone);
		if(getRoleSwitchStatus() && getCarplayConnectStatus() && (!usbCmdDone)){
			getCarplayCapabilities(&capabilities_value);
			while (!(SUPPORT_CARPLAY & capabilities_value)
				&& (retry++ < MAX_RETRY_GET_CARPLAY_CAPABILITIES_TIMES)) {
				printk("carplay getcapabilities retry(%d)\n", retry);
				getCarplayCapabilities(&capabilities_value);
			}

			if (SUPPORT_CARPLAY & capabilities_value) {
				err = requestRoleSwitch();
			} else 
				printk("carplay feature on Device maybe not be enabled!\n");
		}
	}

	mutex_unlock(&lock);
	if (err < 0) {
		ret = -1;
	}else {
		ret = n;
	}
	return ret;
}
static DEVICE_ATTR(role, 0644, roleswicth_cmd_show, roleswicth_cmd_store);
static struct attribute *musbcarplay_attributes[] = {
	&dev_attr_carplayconnectstatus.attr,
	&dev_attr_carplayenablestatus.attr,
	&dev_attr_role.attr,
	NULL
};

static const struct attribute_group musbcarplay_attr_group = {
	.attrs = musbcarplay_attributes,
};

static int carplay_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev;
	char *kobj_path;

	kobj_path = kobject_get_path(&intf->dev.kobj, GFP_KERNEL);
	if (strstr(kobj_path, OTG_PATH) == 0) {
		printk("Not otg port, do not probe:%s\n", kobj_path);
		return -ENOMEM;
	}

	printk("++ carplay probe ++\n");

	udev = interface_to_usbdev(intf);

	if(apple_dev != NULL){
		setCarplayConnectStatus(false);
	}

	if(apple_dev == NULL){
		apple_dev = kzalloc(sizeof(*apple_dev), GFP_KERNEL);
		if (!apple_dev)
		return -ENOMEM;

		apple_dev->dev = udev;
		apple_dev->intf = intf;
		apple = true;
	}
	if (apple_dev->dev == NULL)
		printk( "apple_dev->dev error\n");

	if ((strncmp("1-1", dev_name(&udev->dev), 3) == 0) ){
		setCarplayConnectStatus(true);
		carplay_uevt_send(true);
	}

	if(getRoleSwitchStatus()) {
		usb_set_intfdata(intf, apple_dev);
		return 0;
	} else {
		printk("carplay not in use, do not probe\n");
		return -ENOMEM;
	}
}

int carplay_suspend(void)
{
	printk("carplay_suspend+++++++++\n");

	if (getCarplayConnectStatus()) {
		setCarplayConnectStatus(false);
	}

	return 0;
}
EXPORT_SYMBOL(carplay_suspend);


int carplay_resume(void)
{
	printk("carplay_resume++++++++++\n");

	return 0;
}
EXPORT_SYMBOL(carplay_resume);

static void carplay_disconnect(struct usb_interface *intf)
{
	char *kobj_path;

	printk( "++  carplay_disconnect enter++\n");
	kobj_path = kobject_get_path(&intf->dev.kobj, GFP_KERNEL);
	if (strstr(kobj_path, OTG_PATH) == 0) {
		printk("Not otg port just return:%s\n", kobj_path);
		return;
	}

}

static const struct usb_device_id id_table[] = {

	/*-------------------------------------------------------------*/

	/* EZ-USB devices which download firmware to replace (or in our
	 * case augment) the default device implementation.
	 */

	/* generic EZ-USB FX2 controller (or development board) */
	{USB_DEVICE(0x05ac, 0x12a8),
	 },
	/*-------------------------------------------------------------*/

	{}
};

/* MODULE_DEVICE_TABLE(usb, id_table); */

static struct usb_driver carplay_driver = {
	.name = "carplay",
	.id_table = id_table,
	.probe = carplay_probe,
	.disconnect = carplay_disconnect,

	//.suspend = carplay_suspend,
	//.resume = carplay_resume,
};

/*-------------------------------------------------------------------------*/

static int __init carplay_init(void)
{
	int status = 0;

	printk("carplay_init register carplay_driver\n");
	carplay_uevt_init();

	musbcarplay_kobj = kobject_create_and_add("carplay", NULL);
	if (!musbcarplay_kobj)
		printk("%s: musbcarplay_kobj fail\n",__func__);

	status = sysfs_create_group(musbcarplay_kobj, &musbcarplay_attr_group);
	if (status)
		printk("%s:  create musbcarplay_attr_group fail\n",__func__);

	carplay_wq  = create_singlethread_workqueue("carplay_wq");
	if (!carplay_wq) {
		printk("carplay %s: Unable to create workqueue: carplay\n", __func__);
		return -ENOMEM;
	}
 	INIT_WORK(&carplay_work, carplay_uevent_work);
	mutex_init(&lock);

	return usb_register(&carplay_driver);
}
module_init(carplay_init);

static void __exit carplay_exit(void)
{
	printk("carplay_exit %s,%s,%d\n",__func__,__FILE__,__LINE__);
	flush_work(&carplay_work);
	destroy_workqueue(carplay_wq);
	carplay_uevt_exit();
	sysfs_remove_group(musbcarplay_kobj, &musbcarplay_attr_group);
	usb_deregister(&carplay_driver);
	mutex_destroy(&lock);
}
module_exit(carplay_exit);

MODULE_DESCRIPTION("USB Carplay Driver");
MODULE_LICENSE("GPL");

