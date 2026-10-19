/*
 * f_eap.c -- USB Externel Accessory Native Transport function driver
 *
 * Copyright (c) 2019 AutoChips Inc.
 * Author:		 atc6114
 * Time:		 2020.03.11
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/ratelimit.h>
#include <linux/usb/composite.h>

struct f_ean_opts {
	struct usb_function_instance	func_inst;
	int 							configfs_attr_test;
	bool							bound;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This is to protect the data from concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex					lock;
	int 							refcnt;
};

struct f_ean_opts *g_ean_opts = NULL;

static inline struct f_ean_opts *to_f_ean_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_ean_opts,
				func_inst.group);
}

#define USB_EAN_CONFIGFS_ITEM(_f_)					\
	static void _f_##_attr_release(struct config_item *item)	\
	{								\
		struct f_##_f_##_opts *opts = to_f_##_f_##_opts(item);	\
									\
		usb_put_function_instance(&opts->func_inst);		\
	}								\
									\
	static struct configfs_item_operations _f_##_item_ops = {	\
		.release	= _f_##_attr_release,			\
	};

#define USB_EAN_CONFIGFS_ITEM_ATTR_TEST(_f_)			\
	static ssize_t f_##_f_##_opts_test_show(struct config_item *item, \
						char *page) 	\
	{								\
		int result = 0 ;						\
		struct f_##_f_##_opts *opts = to_f_##_f_##_opts(item);	\
		mutex_lock(&opts->lock);				\
		pr_err("ean configfs test show %d.\n", opts->configfs_attr_test); \
		mutex_unlock(&opts->lock);				\
									\
		return result;						\
	}								\
									\
	static ssize_t f_##_f_##_opts_test_store(struct config_item *item, \
						 const char *page, size_t len)\
	{								\
		int ret = 0;					\
		u8 val = 0; 				\
		struct f_##_f_##_opts *opts = to_f_##_f_##_opts(item);	\
		mutex_lock(&opts->lock);				\
		if (opts->refcnt) { 				\
			mutex_unlock(&opts->lock);			\
			return -EBUSY;					\
		}							\
									\
		ret = kstrtou8(page, 0, &val);			\
		if (ret)					\
			return ret; 			\
		opts->configfs_attr_test = val; \
		pr_err("EAN configfs test store%d.\n", opts->configfs_attr_test);\
		mutex_unlock(&opts->lock);				\
		if (!ret)						\
			ret = len;					\
		return ret; 					\
	}

USB_EAN_CONFIGFS_ITEM(ean);
USB_EAN_CONFIGFS_ITEM_ATTR_TEST(ean);
CONFIGFS_ATTR(f_ean_opts_,test);

static struct configfs_attribute *ean_attrs[] = {
	&f_ean_opts_attr_test,
	NULL,
};

static struct config_item_type ean_func_type = {
	.ct_item_ops	= &ean_item_ops,
	.ct_attrs	= ean_attrs,
	.ct_owner	= THIS_MODULE,
};

/* ean uevent -start */

#if 1
static struct ean_evt_device
{
	const char	  *name;
	struct device *dev;
	int			  index;
} *ean_evt_dev = NULL;

static struct class *ean_evt_class = NULL;
static atomic_t 	 g_device_count;


static int ean_uevt_init(void)
{
	if((ean_evt_dev == NULL) && (ean_evt_class == NULL)){
		printk("ean: iap2 ean_uevt_init\n");
		ean_evt_dev = kzalloc(sizeof(struct ean_evt_device), GFP_KERNEL);
		ean_evt_class = class_create(THIS_MODULE, "ean"); //don't use the same name as host.
		ean_evt_dev->index = atomic_inc_return(&g_device_count);
		ean_evt_dev->name = "ean";
		ean_evt_dev->dev = device_create(ean_evt_class, NULL, MKDEV(0, ean_evt_dev->index), NULL, ean_evt_dev->name);
	}

	return 0;
}

static int ean_uevt_send(bool enable)
{
	char *envp[2];

	printk("%s %d: enter %d\n", __func__, __LINE__,enable);

	if (enable){
		envp[0] = "ean=ready"; //change to enable
	} else{
		envp[0] = "ean=disconnect";
	}
	envp[1] = NULL;
	if (ean_evt_dev != NULL) {
		kobject_uevent_env(&ean_evt_dev->dev->kobj, KOBJ_CHANGE, envp);
	}

	return 0;
}

static int ean_uevt_exit(void)
{
	printk("ean_uevt_exit\n");
	if ((ean_evt_class != NULL) && (ean_evt_dev != NULL)) {
	   device_destroy(ean_evt_class, MKDEV(0, ean_evt_dev->index));
	   class_destroy(ean_evt_class);
	   kfree(ean_evt_dev);
	   ean_evt_dev = NULL;
	   ean_evt_class = NULL;
	}
	return 0;
}
/*	ean uevent -end */
#endif

#define EAN_EP_BULK_BUF_SIZE			4096

/* number of tx requests to allocate */
#define TX_REQ_MAX 4
#define TX_RX_DELAY 10

/* values for EAN Device.state */
#define STATE_OFFLINE				0	/* initial state, disconnected */
#define STATE_READY 				1	/* ready for userspace calls */
#define STATE_BUSY					2	/* processing userspace calls */
#define STATE_CANCELED				3	/* transaction canceled by host */
#define STATE_ERROR 				4	/* error from completion routine */

#define EAN_INTERFACE_NUM			3  //Must be different from IAP2

static const char				 ean_dev_name[] = "ean";
static struct workqueue_struct	*ean_wq = NULL;
struct work_struct				 ean_readywork;
struct work_struct				 ean_disconnectwork;

enum {
	READ_BEGIN,
	READ_FINISH,
	DEFAULT_STATUS
};

struct f_ean {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;

	int state;
	int tx_error;
	int rx_error;

	atomic_t read_excl;
	atomic_t write_excl;
	atomic_t open_excl;

	struct list_head tx_idle;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	struct usb_request *rx_req;
	int rx_done;
	int read_finish;
};

/* Accroding to Accessory Inetrface Specification32.Chapter39.2 External Accessory Protocol
   Table 39-1.USB Interface Descriptor(Alternal Setting1- Active Session)... */

static struct usb_interface_descriptor ean_intf_alt0 = {
	.bLength			= sizeof ean_intf_alt0,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= EAN_INTERFACE_NUM,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 0,
	.bInterfaceClass	= USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 0xF0,
	.bInterfaceProtocol = 0x01,
};

static struct usb_interface_descriptor ean_intf_alt1 = {
	.bLength			= sizeof ean_intf_alt1,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= EAN_INTERFACE_NUM,
	.bAlternateSetting	= 1,
	.bNumEndpoints		= 2,
	.bInterfaceClass	= USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 0xF0,
	.bInterfaceProtocol = 0x01,
};

/* High-Speed  */
static struct usb_endpoint_descriptor ean_hs_bulkin_ep_desc = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize 	= cpu_to_le16(512),
};

static struct usb_endpoint_descriptor ean_hs_bulkout_ep_desc = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize 	= cpu_to_le16(512),

};

static struct usb_descriptor_header *ean_hs_descriptors[] = {
	(struct usb_descriptor_header *)&ean_intf_alt0,
	(struct usb_descriptor_header *)&ean_intf_alt1,
	(struct usb_descriptor_header *)&ean_hs_bulkin_ep_desc,
	(struct usb_descriptor_header *)&ean_hs_bulkout_ep_desc,
	NULL,
};

/* Full-Speed */
static struct usb_endpoint_descriptor ean_fs_bulkin_ep_desc = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor ean_fs_bulkout_ep_desc = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *ean_fs_descriptors[] = {
	(struct usb_descriptor_header *)&ean_intf_alt0,
	(struct usb_descriptor_header *)&ean_intf_alt1,
	(struct usb_descriptor_header *)&ean_fs_bulkin_ep_desc,
	(struct usb_descriptor_header *)&ean_fs_bulkout_ep_desc,
	NULL,
};

#define INTERFACE_STRING_EAN_IDX 0

static struct usb_string ean_string_defs[] = {
	[INTERFACE_STRING_EAN_IDX].s = "cn.carbit.ecprotocol",
	{  } /* end of list */
};

static struct usb_gadget_strings ean_string_table = {
	.language = 	0x0409, /* en-us */
	.strings =		ean_string_defs,
};

static struct usb_gadget_strings *ean_strings[] = {
	&ean_string_table,
	NULL,
};

static struct f_ean *g_ean_dev = NULL;

static inline struct f_ean *func_to_ean(struct usb_function *f)
{
	return container_of(f, struct f_ean, function);
}


static struct usb_request *ean_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		printk("EAN %s %d: kmalloc failed\n", __func__, __LINE__);
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void ean_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int ean_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void ean_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
void ean_req_put(struct f_ean *ean_dev, struct list_head *head, struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&ean_dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&ean_dev->lock, flags);
}

/* remove a request from the head of a list */
struct usb_request *ean_req_get(struct f_ean *ean_dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&ean_dev->lock, flags);
	if (list_empty(head)) {
		printk("ean list empty!\n");
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&ean_dev->lock, flags);
	return req;
}

static void ean_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct f_ean *ean_dev = g_ean_dev;
	unsigned long flags;

	spin_lock_irqsave(&ean_dev->lock, flags);
	if (req->status != 0)
		ean_dev->tx_error = 1;
	spin_unlock_irqrestore(&ean_dev->lock, flags);

	ean_req_put(ean_dev, &ean_dev->tx_idle, req);

	wake_up(&ean_dev->write_wq);

	printk("ean_complete_in = %d\n", req->status);
}

static void ean_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct f_ean *ean_dev = g_ean_dev;

	ean_dev->rx_done = 1;
	if (req->status != 0 && req->status != -ECONNRESET)
		ean_dev->rx_error = 1;

	wake_up(&ean_dev->read_wq);

	printk("carpay ean_complete_out = %d\n", req->status);
}

static int ean_create_bulk_ep(struct f_ean *ean_dev,
					 struct usb_endpoint_descriptor *in_desc,
					 struct usb_endpoint_descriptor *out_desc)
{
	struct usb_composite_dev *cdev = ean_dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		printk("carplay EAN create bulk ep in fail %s,%d\n", __func__,__LINE__);
		return -ENODEV;
	}
	ep->driver_data = ean_dev;
	ean_dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		printk("carplay EAN create bulk ep out fail %s,%d\n", __func__,__LINE__);
		return -ENODEV;
	}
	ep->driver_data = ean_dev;
	ean_dev->ep_out = ep;

	/* now allocate requests for ep in/out */
	req = ean_request_new(ean_dev->ep_out, EAN_EP_BULK_BUF_SIZE);
	if (!req)
		goto fail;
	req->complete = ean_complete_out;
	ean_dev->rx_req = req;

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = ean_request_new(ean_dev->ep_in, EAN_EP_BULK_BUF_SIZE);
		if (!req)
			goto fail;
		req->complete = ean_complete_in;
		ean_req_put(ean_dev, &ean_dev->tx_idle, req);
	}

	return 0;

 fail:
	printk("EAN %s %d: could not allocate requests\n", __func__, __LINE__);

	return -1;
}

static ssize_t ean_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
	struct f_ean *ean_dev = fp->private_data;
	struct usb_request *req;
	int r = count, xfer;
	int ret;
	unsigned long flags;
	static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 10);

	if (!ean_dev) {
		printk("carplay EAN %s %d: iap2_dev is NULL\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (count > EAN_EP_BULK_BUF_SIZE) {
		printk("EAN %s %d: count >EP_BULK_BUF_SIZE\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (ean_lock(&ean_dev->read_excl)) {
		if (__ratelimit(&ratelimit)) {
			printk("EAN %s %d: Failed, lock is busy\n", __func__, __LINE__);
		}
		return -EBUSY;
	}

	spin_lock_irqsave(&ean_dev->lock, flags);
	if (ean_dev->rx_error || ean_dev->state != STATE_READY) {
		printk("ean_dev %s %d: state(%d) rx_error(%d)\n",
				__func__, __LINE__, ean_dev->state, ean_dev->rx_error);
		spin_unlock_irqrestore(&ean_dev->lock, flags);
		r = -EIO;
		goto done;
	}else{
		spin_unlock_irqrestore(&ean_dev->lock, flags);
	}

 requeue_req:
	printk("ean %d: ean_dev %p\n", __LINE__, ean_dev);
	/* queue a request */
	spin_lock_irqsave(&ean_dev->lock, flags);
	req = ean_dev->rx_req;
	req->length = count;
	ean_dev->rx_done = 0;
	ean_dev->read_finish = READ_BEGIN;
	spin_unlock_irqrestore(&ean_dev->lock, flags);

	ret = usb_ep_queue(ean_dev->ep_out, req, GFP_ATOMIC);
	if (ret < 0) {
		printk("EAN %s %d: failed to queue req %p (%d)\n", __func__, __LINE__, req, ret);
		r = -EIO;
		//iap2_dev->rx_error = 1;
		//spin_unlock_irqrestore(&iap2_dev->lock, flags);
		goto done;
	} else {
		//printk("%s %s %d: rx %p queue\n", __FILE__, __func__, __LINE__, req);
	}

	/* wait for a request to complete = timeout!*/
	ret = wait_event_interruptible_timeout(ean_dev->read_wq,
		ean_dev->rx_error || ean_dev->rx_done || (STATE_READY == ean_dev->state), 1*HZ);
	if (ret < 0) {
		/*if (ret != -ERESTARTSYS)
			ean_dev->rx_error = 1;*/
		r = ret;
		usb_ep_dequeue(ean_dev->ep_out, req);
		printk("EAN %s %d: wait_event_interruptible time out !!!!!!\n",
				__func__, __LINE__);
		goto done;
	}

	spin_lock_irqsave(&ean_dev->lock, flags);
	if (ean_dev->state != STATE_READY) {
		printk("ean %s	%d state: %d \n", __func__, __LINE__, ean_dev->state);
		spin_unlock_irqrestore(&ean_dev->lock, flags);
		usb_ep_dequeue(ean_dev->ep_out, req);
		r = -EIO;
		goto done;
	}

	if (req->status != 0) {
		spin_unlock_irqrestore(&ean_dev->lock, flags);
		usb_ep_dequeue(ean_dev->ep_out, req);
		r = req->status;
		printk("ean %s %d: req->status(%d) \n", __func__, __LINE__,req->status);

		goto done;
	}

	if (!ean_dev->rx_error) {
		if (req->actual == 0){
			spin_unlock_irqrestore(&ean_dev->lock, flags);
			goto requeue_req; // If we got a 0-len packet, throw it back and try again
		}

		xfer = (req->actual < count) ? req->actual : count;

		if (copy_to_user(buf, req->buf, xfer)){
			r = -EFAULT;
		} else {
			r = xfer; //return actual value
		}

	} else {
		r = -EIO;
		printk("EAN %s %d: rx_error\n", __func__, __LINE__);
	}

	spin_unlock_irqrestore(&ean_dev->lock, flags);
 done:
	ean_unlock(&ean_dev->read_excl);
	spin_lock_irqsave(&ean_dev->lock, flags);
	ean_dev->read_finish = READ_FINISH;
	ean_dev->rx_error = 0;
	printk("ean %s %d: ean_dev %p, read_finish %d, rx_error %d, return: %d\n",
		__func__, __LINE__,
		ean_dev, ean_dev->read_finish, ean_dev->rx_error, r);
	spin_unlock_irqrestore(&ean_dev->lock, flags);

	return r;
}

static ssize_t ean_write(struct file *fp, const char __user *buf, size_t count, loff_t *pos)
{
	struct f_ean *ean_dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;
	unsigned long flags;
	//static int flow_state;

	if (!ean_dev) {
		printk("EAN %s %d: eande is NULL\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (ean_lock(&ean_dev->write_excl)) {
		printk("EAN %s %d: Failed, lock is busy\n", __func__, __LINE__);
		return -EBUSY;
	}

	while (count > 0) {
		spin_lock_irqsave(&ean_dev->lock, flags);
		if (ean_dev->tx_error || STATE_READY != ean_dev->state) {
			printk("EAN %s %d:State(%d) tx_error(%d)  \n",
				__func__, __LINE__, ean_dev->state, ean_dev->tx_error);
			ean_dev->tx_error = 0;
			r = -EIO;
			spin_unlock_irqrestore(&ean_dev->lock, flags);
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		spin_unlock_irqrestore(&ean_dev->lock, flags);

		ret = wait_event_interruptible_timeout(ean_dev->write_wq,
							(req = ean_req_get(ean_dev, &ean_dev->tx_idle))
							|| ean_dev->tx_error, 1*HZ);

		if (ret < 0) {
			r = ret;
			printk("ean %s %d: wait_event_interruptible time out !!!!!!\n",
					__func__, __LINE__);
			break;
		}
		spin_lock_irqsave(&ean_dev->lock, flags);
		if(ean_dev->tx_error){
			r = -EIO;
			spin_unlock_irqrestore(&ean_dev->lock, flags);
			break;
		}
		spin_unlock_irqrestore(&ean_dev->lock, flags);

		if (req != 0) {
			if (count > EAN_EP_BULK_BUF_SIZE)
				xfer = EAN_EP_BULK_BUF_SIZE;
			else
				xfer = count;
			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				break;
			}
			req->length = xfer;
			ret = usb_ep_queue(ean_dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				printk("EAN %s %d: xfer error %d\n", __func__, __LINE__,
					 ret);
				spin_lock_irqsave(&ean_dev->lock, flags);
				ean_dev->tx_error = 1;
				spin_unlock_irqrestore(&ean_dev->lock, flags);
				r = -EIO;
				break;
			}

			buf += xfer;
			count -= xfer;

			/* zero this so we don't try to free it on error exit */
			req = 0;
		}else{
			printk("EAN %s %d: req == 0 \n", __func__, __LINE__ );
			r = -EIO;
		}

	}

	if (req)
		ean_req_put(ean_dev, &ean_dev->tx_idle, req);

	ean_unlock(&ean_dev->write_excl);

	printk("ean  %s %d: return = %d\n", __func__, __LINE__, r);

	return r;
}

static unsigned int ean_poll(struct file *file, poll_table *wait)
{
	struct f_ean	*ean_dev  = file->private_data;
	unsigned int	ret = 0;

	poll_wait(file, &ean_dev->read_wq, wait);
	poll_wait(file, &ean_dev->write_wq, wait);

	if (/*condition*/1)
		ret |= POLLIN | POLLRDNORM;

	return ret;
}

static int ean_open(struct inode *ip, struct file *fp);
static int ean_release(struct inode *ip, struct file *fp);

/* file operations for ean device /dev/gean_dev */
static struct file_operations ean_fops = {
	.owner = THIS_MODULE,
	.read = ean_read,
	.write = ean_write,
	.open = ean_open,
	.poll = ean_poll,
	.release = ean_release,
};

static struct miscdevice ean_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = ean_dev_name,
	.fops = &ean_fops,
};

static int open_release_pair;

static spinlock_t open_lock;

static int ean_open(struct inode *ip, struct file *fp)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&open_lock, flags);

	if (!g_ean_dev) {
		printk("ean ean_open g_ean_dev is NULL\n");
		open_release_pair = 0;
		ret = ENODEV;
		goto OPEN_END;
	}

	if (open_release_pair > 0) {
		ret = 0;
		fp->private_data = g_ean_dev;
		goto OPEN_END;
	}

	open_release_pair++;
	fp->private_data = g_ean_dev;
	g_ean_dev->tx_error = 0;
	g_ean_dev->rx_error = 0;

 OPEN_END:
	printk("ean open_release_pair: %d", open_release_pair);

	spin_unlock_irqrestore(&open_lock, flags);

	return ret;
}

static int ean_release(struct inode *ip, struct file *fp)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&open_lock, flags);

	if (open_release_pair < 1) {
		ret = -1;
		goto RELEASE_END;
	}

	open_release_pair--;

 RELEASE_END:

	printk("ean open_release_pair: %d", open_release_pair);

	spin_unlock_irqrestore(&open_lock, flags);

	return ret;
}

static void ean_cleanup(void)
{
	printk("ean %s %d \n", __func__, __LINE__);

	if (ean_wq != NULL){
		flush_workqueue(ean_wq);
		destroy_workqueue(ean_wq);
		ean_wq = NULL;
	}

	if (g_ean_dev != NULL){
		kfree(g_ean_dev);
		g_ean_dev = NULL;
	}

	if (g_ean_opts != NULL){
		kfree(g_ean_opts);
		g_ean_opts = NULL;
	}
	ean_uevt_exit();
}

static void ean_uevent_ready(struct work_struct *work)
{
	ean_uevt_send(true);
}

static void ean_uevent_disconnect(struct work_struct *work)
{
	ean_uevt_send(false);
}

static int ean_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_ean *ean_dev = func_to_ean(f);
	int ret;
	int status;
	struct usb_string *us;

	printk("ean %s %d: enter\n", __func__, __LINE__);
	ean_dev->cdev = cdev;

	us = usb_gstrings_attach(cdev, ean_strings,
				 ARRAY_SIZE(ean_string_defs));
	if (IS_ERR(us))
		return PTR_ERR(us);

	ean_intf_alt0.iInterface = us[INTERFACE_STRING_EAN_IDX].id;
	ean_intf_alt1.iInterface = us[INTERFACE_STRING_EAN_IDX].id;

	/* allocate instance-specific interface IDs */
	status = usb_interface_id(c, f);
	pr_info("ean %s %d bInterfaceNumber:%d	  \n", __func__, __LINE__, status);
	if (status < 0) {
		ERROR(cdev, "%s: can't bind, err %d\n", f->name, status);
		return status;
	}

	ean_intf_alt0.bInterfaceNumber = status;
	ean_intf_alt1.bInterfaceNumber = status;

	/* allocate endpoints */
	ret = ean_create_bulk_ep(ean_dev, &ean_fs_bulkin_ep_desc, &ean_fs_bulkout_ep_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		ean_hs_bulkin_ep_desc.bEndpointAddress	= ean_fs_bulkin_ep_desc.bEndpointAddress;
		ean_hs_bulkout_ep_desc.bEndpointAddress = ean_fs_bulkout_ep_desc.bEndpointAddress;
	}

	printk("ean %s %d: leave\n", __func__, __LINE__);
	return 0;
}

static void ean_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_ean *ean_dev = func_to_ean(f);
	struct usb_request *req;
	unsigned long flags;
	unsigned int txReqNum =  TX_REQ_MAX;
	int read_finish = 0;

	 printk("ean %s %d: enter\n", __func__, __LINE__);
	 //ean_cleanup();
	spin_lock_irqsave(&ean_dev->lock, flags);
	ean_dev->tx_error = 1;
	ean_dev->state = STATE_OFFLINE;
	ean_dev->rx_error = 1;
	spin_unlock_irqrestore(&ean_dev->lock, flags);
	wake_up(&ean_dev->read_wq);
	wake_up(&ean_dev->write_wq);
	while(1) {
		spin_lock_irqsave(&ean_dev->lock, flags);
		read_finish = ean_dev->read_finish;
		printk("ean %s %d : state(%d) rx_error(%d) read_finish(%d)\n", __func__, __LINE__,
		ean_dev->state, ean_dev->rx_error, ean_dev->read_finish);
		spin_unlock_irqrestore(&ean_dev->lock, flags);

		if (read_finish == 0) {
			msleep(TX_RX_DELAY);
			continue;
		} else if(read_finish == READ_FINISH|| read_finish == DEFAULT_STATUS) {
			break;
		}
	}

	ean_request_free(ean_dev->rx_req, ean_dev->ep_out);
	while ((req = ean_req_get(ean_dev, &ean_dev->tx_idle))||txReqNum){
		if(req!= 0){
			ean_request_free(req, ean_dev->ep_in);
			txReqNum--;
		}
		msleep(TX_RX_DELAY);
	}

	printk("ean %s %d: leave\n", __func__, __LINE__);
}

static int ean_function_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_ean *ean_dev = func_to_ean(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;
	unsigned long flags;

	printk("ean ean_set_alt intf: %d alt: %d\n", intf, alt);

#if 0
	switch (alt) {
	case 0:
		usb_ep_disable(ean_dev->ep_in);
		usb_ep_disable(ean_dev->ep_out);

		spin_lock_irqsave(&ean_dev->lock, flags);
		ean_dev->state = STATE_OFFLINE;
		spin_unlock_irqrestore(&ean_dev->lock, flags);

		return 0;
	case 1:
#endif
		ret = config_ep_by_speed(cdev->gadget, f, ean_dev->ep_in);
		if (ret) {
			printk("ean %s %d: usb_ep_enable in failed\n", __func__, __LINE__);
			return ret;
		}

		ret = usb_ep_enable(ean_dev->ep_in);
		if (ret)
			return ret;

		ret = config_ep_by_speed(cdev->gadget, f, ean_dev->ep_out);
		if (ret) {
			printk("ean %s %d: usb_ep_enable out failed\n", __func__, __LINE__);
			return ret;
		}

		ret = usb_ep_enable(ean_dev->ep_out);
		if (ret) {
			usb_ep_disable(ean_dev->ep_in);
			printk("ean %s %d: usb_ep_enable in failed\n", __func__, __LINE__);
			return ret;
		}
#if 0
	default:
		return -EINVAL;
	}
#endif
	spin_lock_irqsave(&ean_dev->lock, flags);
	ean_dev->state = STATE_READY;
	spin_unlock_irqrestore(&ean_dev->lock, flags);

	wake_up(&ean_dev->read_wq); /* readers may be blocked waiting for us to go online */

	if(ean_wq){
		printk(" ean_set_alt queue_work\n");
		queue_work(ean_wq, &ean_readywork);
	}

	printk("ean_set_alt intf leave\n");

	return 0;
}

static void ean_function_disable(struct usb_function *f)
{
	struct f_ean *ean_dev = func_to_ean(f);

	printk("ean_function_disable enter \n");

	if(ean_wq){
		printk("ean_function_disable queue_work\n");
		queue_work(ean_wq, &ean_disconnectwork);
	}

	usb_ep_disable(ean_dev->ep_in);
	usb_ep_disable(ean_dev->ep_out);

	/* readers may be blocked waiting for us to go online */
	wake_up(&ean_dev->read_wq);
}

static int ean_setup(void)
{
	struct f_ean *ean_dev;
	unsigned long flags;

	ean_uevt_init();

	if(g_ean_dev == NULL) {
		ean_dev = kzalloc(sizeof(*ean_dev), GFP_KERNEL);
		if (!ean_dev)
			return -ENOMEM;

		INIT_WORK(&ean_readywork, ean_uevent_ready);
		INIT_WORK(&ean_disconnectwork, ean_uevent_disconnect);
		spin_lock_init(&ean_dev->lock);
		spin_lock_init(&open_lock);

		init_waitqueue_head(&ean_dev->read_wq);
		init_waitqueue_head(&ean_dev->write_wq);

		atomic_set(&ean_dev->open_excl, 0);
		atomic_set(&ean_dev->read_excl, 0);
		atomic_set(&ean_dev->write_excl, 0);

		INIT_LIST_HEAD(&ean_dev->tx_idle);

		g_ean_dev = ean_dev;

		spin_lock_irqsave(&ean_dev->lock, flags);
		ean_dev->state = STATE_OFFLINE;
		//ean_dev->tx_done = 0;
		ean_dev->tx_error = 0;
		ean_dev->rx_error = 0;
		ean_dev->read_finish = DEFAULT_STATUS;
		spin_unlock_irqrestore(&ean_dev->lock, flags);
	}

	if(ean_wq == NULL){
		printk("%s %d: ean create_singlethread_workqueue\n", __func__, __LINE__);
		ean_wq	= create_singlethread_workqueue("ean");
		if (!ean_wq) {
			printk("ean %s: Unable to create workqueue: ean\n", __func__);
			if(g_ean_dev != NULL){
				kfree(g_ean_dev);
				g_ean_dev = NULL;
			}
			return -ENOMEM;
		}
	}
	return 0;
}

static void ean_free_inst(struct usb_function_instance *f)
{
	printk("%s %d enter\n", __func__, __LINE__);

	ean_cleanup();
}

static struct usb_function_instance *ean_alloc_inst(void)
{
	printk("ean %s %d enter\n", __func__, __LINE__);

	if (g_ean_opts == NULL) {
		g_ean_opts = kzalloc(sizeof(*g_ean_opts), GFP_KERNEL);
		if (!g_ean_opts)
			return ERR_PTR(-ENOMEM);

		mutex_init(&g_ean_opts->lock);
		g_ean_opts->func_inst.free_func_inst = ean_free_inst;
		config_group_init_type_name(&g_ean_opts->func_inst.group, "", &ean_func_type);
	}

	if(ean_setup() != 0){
		kfree(g_ean_opts);
		return ERR_PTR(-ENOMEM);
	}

	return &g_ean_opts->func_inst;
}

static void ean_free(struct usb_function *f)
{
	struct f_ean *ean_dev;
	struct f_ean_opts *opts;
	printk("EAN %s %d enter\n", __func__, __LINE__);

	misc_deregister(&ean_device);

	ean_dev = func_to_ean(f);
	opts = container_of(f->fi, struct f_ean_opts, func_inst);
	//kfree(ean_dev);
	mutex_lock(&opts->lock);
	opts->refcnt--;
	mutex_unlock(&opts->lock);
}

static struct usb_function *ean_alloc(struct usb_function_instance *fi)
{
	struct f_ean *ean_dev = g_ean_dev;
	struct f_ean_opts	*opts;
	int ret = 0;

	opts = container_of(fi, struct f_ean_opts, func_inst);
	mutex_lock(&opts->lock);
	opts->refcnt++;
	mutex_unlock(&opts->lock);

	printk("ean %s %d enter\n", __func__, __LINE__);

	ret = misc_register(&ean_device);
	if (ret){
		printk("ean_alloc  misc_register fail\n");
	}

	ean_dev->function.name = "ean";
	ean_dev->function.strings = ean_strings;
	ean_dev->function.fs_descriptors = ean_fs_descriptors;
	ean_dev->function.hs_descriptors = ean_hs_descriptors;

	ean_dev->function.bind = ean_function_bind;
	ean_dev->function.unbind = ean_function_unbind;
	ean_dev->function.set_alt = ean_function_set_alt;
	ean_dev->function.disable = ean_function_disable;
	ean_dev->function.free_func = ean_free;

	printk("ean_alloc leave\n");

	return &ean_dev->function;
}


DECLARE_USB_FUNCTION_INIT(ean, ean_alloc_inst, ean_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jesse");
