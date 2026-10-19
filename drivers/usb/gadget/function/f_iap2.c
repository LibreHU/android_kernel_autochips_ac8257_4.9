/*
 * f_iap2.c -- USB IOS Accessory Protocol v2.0(IAP2) function driver
 *
 * Copyright (c) 2019 AutoChips Inc.
 * Author:		 atc6047
 * Time:		 2019.5.7 17:30
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


struct f_iap2_opts {
	struct usb_function_instance	func_inst;
	int configfs_attr_test;
	bool				bound;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This is to protect the data from concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex			lock;
	int				refcnt;
};

struct f_iap2_opts *g_opts = NULL;

static inline struct f_iap2_opts *to_f_iap2_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_iap2_opts,
				func_inst.group);
}

#define USB_IAP2_CONFIGFS_ITEM(_f_)					\
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

#define USB_IAP2_CONFIGFS_ITEM_ATTR_TEST(_f_)			\
	static ssize_t f_##_f_##_opts_test_show(struct config_item *item, \
						char *page)		\
	{								\
		int result = 0 ;						\
		struct f_##_f_##_opts *opts = to_f_##_f_##_opts(item);	\
		mutex_lock(&opts->lock);				\
		pr_err("usb gadget iap2 configfs test show %d.\n", opts->configfs_attr_test); \
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
		if (opts->refcnt) {					\
			mutex_unlock(&opts->lock);			\
			return -EBUSY;					\
		}							\
									\
		ret = kstrtou8(page, 0, &val);			\
		if (ret)					\
			return ret;				\
		opts->configfs_attr_test = val; \
		pr_err("usb gadget iap2 configfs test store%d.\n", opts->configfs_attr_test);\
		mutex_unlock(&opts->lock);				\
		if (!ret)						\
			ret = len;					\
		return ret;						\
	}

USB_IAP2_CONFIGFS_ITEM(iap2);
USB_IAP2_CONFIGFS_ITEM_ATTR_TEST(iap2);
CONFIGFS_ATTR(f_iap2_opts_,test);

static struct configfs_attribute *iap2_attrs[] = {
	&f_iap2_opts_attr_test,
	NULL,
};

static struct config_item_type iap2_func_type = {
	.ct_item_ops	= &iap2_item_ops,
	.ct_attrs	= iap2_attrs,
	.ct_owner	= THIS_MODULE,
};

/* carplay uevent -start */

#if 1
static struct carplay_device
{
  const char	*name;
  struct device	*dev;
  int			 index;
}*g_carplay_dev = NULL;

static struct class *g_carplay_class = NULL;
static atomic_t g_device_count;



static int carplay_uevt_init(void)
{
	if((g_carplay_dev == NULL)&&(g_carplay_class == NULL)){
		printk("carplay_gadget: iap2 carplay_uevt_init\n");
		g_carplay_dev = kzalloc(sizeof(struct carplay_device), GFP_KERNEL);
		g_carplay_class = class_create(THIS_MODULE, "carplay_gadget_iap2"); //don't use the same name as host.
		g_carplay_dev->index = atomic_inc_return(&g_device_count);
		g_carplay_dev->name = "carplay_gadget_iap2";
		g_carplay_dev->dev = device_create(g_carplay_class, NULL, MKDEV(0, g_carplay_dev->index), NULL, g_carplay_dev->name);
	}

	return 0;
}

static int carplay_uevt_send(bool enable)
{
	char *envp[2];

	printk("%s %s %d: iap2 carplay uevent send %d\n", __FILE__, __func__, __LINE__,enable);

	if (enable){
	  envp[0] = "carplay=ready"; //change to enable
	}else{
	  envp[0] = "carplay=disconnect";
	}
	envp[1] = NULL;
	if (g_carplay_dev != NULL) {
		kobject_uevent_env(&g_carplay_dev->dev->kobj, KOBJ_CHANGE, envp);
	}

	return 0;

}

static int carplay_uevt_exit(void)
{
	printk("carplay_gadget: iap2 carplay_uevt_exit\n");
	if ((g_carplay_class != NULL) && (g_carplay_dev != NULL)) {
	   device_destroy(g_carplay_class, MKDEV(0, g_carplay_dev->index));
	   class_destroy(g_carplay_class);
	   kfree(g_carplay_dev);
	   g_carplay_dev = NULL;
	   g_carplay_class = NULL;
	}
	return 0;
}
/*	carplay uevent -end */
#endif

#define IAP2_EP_BULK_BUF_SIZE			4096

/* number of tx requests to allocate */
#define TX_REQ_MAX 4
#define TX_RX_DELAY 10

static const char iap2_dev_name[] = "iAP2Dev";
static struct workqueue_struct	*iap2_wq = NULL;
struct work_struct	iap2_readywork;
struct work_struct	iap2_disconnectwork;


enum{
READ_BEGIN,
READ_FINISH,
DEFAULT_STATUS
};


struct f_iap2 {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;

	int online;
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


/* R29, P485, 60.4.2 IAP2 interface Descriptor */
static struct usb_interface_descriptor iap2_interface_desc = {
	.bLength		= sizeof iap2_interface_desc,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber =	2,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 2,
	.bInterfaceClass	= 0xff,
	.bInterfaceSubClass	= 0xf0,
	.bInterfaceProtocol	= 0x00,
};


/* High-Speed  */
static struct usb_endpoint_descriptor iap2_hs_bulkin_ep_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),

};

static struct usb_endpoint_descriptor iap2_hs_bulkout_ep_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),

};

static struct usb_descriptor_header *iap2_hs_descriptors[] = {
	(struct usb_descriptor_header *)&iap2_interface_desc,
	(struct usb_descriptor_header *)&iap2_hs_bulkin_ep_desc,
	(struct usb_descriptor_header *)&iap2_hs_bulkout_ep_desc,
	NULL,
};


/* Full-Speed */
static struct usb_endpoint_descriptor iap2_fs_bulkin_ep_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = 	USB_ENDPOINT_XFER_BULK,

};

static struct usb_endpoint_descriptor iap2_fs_bulkout_ep_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *iap2_fs_descriptors[] = {
	(struct usb_descriptor_header *)&iap2_interface_desc,
	(struct usb_descriptor_header *)&iap2_fs_bulkin_ep_desc,
	(struct usb_descriptor_header *)&iap2_fs_bulkout_ep_desc,
	NULL,
};


/* refer R29 P227 Interface 0/0 (iAP Interface) *//* string descriptors: */

#define STRING_IAP2_IDX	0

static struct usb_string iap2_string_defs[] = {
	[STRING_IAP2_IDX].s = "iAP Interface",
	{  } /* end of list */
};

static struct usb_gadget_strings iap2_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		iap2_string_defs,
};

static struct usb_gadget_strings *iap2_strings[] = {
	&iap2_string_table,
	NULL,
};


static struct f_iap2 *g_iap2_dev = NULL;

static inline struct f_iap2 *func_to_iap2(struct usb_function *f)
{
	return container_of(f, struct f_iap2, function);
}


static struct usb_request *iap2_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		printk("carplay %s %s %d: kmalloc failed\n", __FILE__, __func__, __LINE__);
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void iap2_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		req->buf = NULL;
		usb_ep_free_request(ep, req);
	}
}

static inline int iap2_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void iap2_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
void iap2_req_put(struct f_iap2 *iap2_dev, struct list_head *head, struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&iap2_dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&iap2_dev->lock, flags);
}

/* remove a request from the head of a list */
struct usb_request *iap2_req_get(struct f_iap2 *iap2_dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&iap2_dev->lock, flags);
	if (list_empty(head)) {
		printk("carplay list empty!\n");
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&iap2_dev->lock, flags);
	return req;
}

static void iap2_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct f_iap2 *iap2_dev = g_iap2_dev;
	unsigned long flags;

	spin_lock_irqsave(&iap2_dev->lock, flags);
	if (req->status != 0)
		iap2_dev->tx_error = 1;
	spin_unlock_irqrestore(&iap2_dev->lock, flags);

	iap2_req_put(iap2_dev, &iap2_dev->tx_idle, req);

	wake_up(&iap2_dev->write_wq);

	printk("carplay iap2_complete_in = %d\n", req->status);
}

static void iap2_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct f_iap2 *iap2_dev = g_iap2_dev;

	iap2_dev->rx_done = 1;
	if (req->status != 0 && req->status != -ECONNRESET)
		iap2_dev->rx_error = 1;

	wake_up(&iap2_dev->read_wq);

	printk("carpay iap2_complete_out = %d\n", req->status);
}

static int iap2_create_bulk_ep(struct f_iap2 *iap2_dev,
					 struct usb_endpoint_descriptor *in_desc,
					 struct usb_endpoint_descriptor *out_desc)
{
	struct usb_composite_dev *cdev = iap2_dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		printk("carplay create bulk ep in fail%s,%s,%d\n", __func__,__FILE__,__LINE__);
		return -ENODEV;
	}
	ep->driver_data = iap2_dev;
	iap2_dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		printk("carplay create bulk ep out fail%s,%s,%d\n", __func__,__FILE__,__LINE__);
		return -ENODEV;
	}
	ep->driver_data = iap2_dev;
	iap2_dev->ep_out = ep;

	/* now allocate requests for ep in/out */
	req = iap2_request_new(iap2_dev->ep_out, IAP2_EP_BULK_BUF_SIZE);
	if (!req)
		goto fail;
	req->complete = iap2_complete_out;
	iap2_dev->rx_req = req;

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = iap2_request_new(iap2_dev->ep_in, IAP2_EP_BULK_BUF_SIZE);
		if (!req)
			goto fail;
		req->complete = iap2_complete_in;
		iap2_req_put(iap2_dev, &iap2_dev->tx_idle, req);
	}

	return 0;

 fail:
	printk("carplay %s %s %d: could not allocate requests\n", __FILE__, __func__, __LINE__);

	return -1;
}

static ssize_t iap2_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
	struct f_iap2 *iap2_dev = fp->private_data;
	struct usb_request *req;
	int r = count, xfer;
	int ret;
	unsigned long flags;
	static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 10);

	if (!iap2_dev) {
		printk("carplay %s %s %d: iap2_dev is NULL\n", __FILE__, __func__, __LINE__);
		return -ENODEV;
	}

	if (count > IAP2_EP_BULK_BUF_SIZE) {
		printk("carplay %s %s %d: count > IAP2_EP_BULK_BUF_SIZE\n", __FILE__, __func__, __LINE__);
		return -EINVAL;
	}

	if (iap2_lock(&iap2_dev->read_excl)) {
		if (__ratelimit(&ratelimit)) {
			printk("carplay %s %s %d: Failed, lock is busy\n", __FILE__, __func__, __LINE__);
		}
		return -EBUSY;
	}

	spin_lock_irqsave(&iap2_dev->lock, flags);
	if (iap2_dev->rx_error || !iap2_dev->online) {
		printk("carplay %s %s %d: online(%d) rx_error(%d)\n",
				__FILE__, __func__, __LINE__, iap2_dev->online, iap2_dev->rx_error);
		spin_unlock_irqrestore(&iap2_dev->lock, flags);
		r = -EIO;
		goto done;
	}else{
		spin_unlock_irqrestore(&iap2_dev->lock, flags);
	}

 requeue_req:
	printk("carplay %s %s %d: iap2_dev %p\n", __FILE__, __func__, __LINE__, iap2_dev);
	/* queue a request */
	spin_lock_irqsave(&iap2_dev->lock, flags);
	req = iap2_dev->rx_req;
	req->length = count;
	iap2_dev->rx_done = 0;
	iap2_dev->read_finish = READ_BEGIN;
	if (iap2_dev->online == 0) {
		printk("carplay %s %s %d: online == 0 \n", __FILE__, __func__, __LINE__);
		spin_unlock_irqrestore(&iap2_dev->lock, flags);
		r = -EIO;
		goto done;
	}
	spin_unlock_irqrestore(&iap2_dev->lock, flags);

	ret = usb_ep_queue(iap2_dev->ep_out, req, GFP_ATOMIC);
	if (ret < 0) {
		printk("carplay %s %s %d: failed to queue req %p (%d)\n", __FILE__, __func__, __LINE__, req, ret);
		r = -EIO;
		//iap2_dev->rx_error = 1;
		//spin_unlock_irqrestore(&iap2_dev->lock, flags);
		goto done;
	} else {
		//printk("%s %s %d: rx %p queue\n", __FILE__, __func__, __LINE__, req);
	}

	/* wait for a request to complete = timeout!*/
	ret = wait_event_interruptible_timeout(iap2_dev->read_wq, iap2_dev->rx_error || iap2_dev->rx_done || (!iap2_dev->online),1*HZ);
	if (ret < 0) {
		/*if (ret != -ERESTARTSYS)
			iap2_dev->rx_error = 1;*/
		r = ret;
		usb_ep_dequeue(iap2_dev->ep_out, req);
		printk("carplay %s %s %d: wait_event_interruptible time out !!!!!!\n",
				__FILE__, __func__, __LINE__);
		goto done;
	}

	spin_lock_irqsave(&iap2_dev->lock, flags);

	if (req->status != 0) {
		spin_unlock_irqrestore(&iap2_dev->lock, flags);
		usb_ep_dequeue(iap2_dev->ep_out, req);
		r = req->status;
		printk("carplay %s %s %d: req->status(%d) \n", __FILE__, __func__, __LINE__,req->status);

		goto done;
	}

	if (!iap2_dev->rx_error) {
		if (req->actual == 0){
			spin_unlock_irqrestore(&iap2_dev->lock, flags);
			goto requeue_req; // If we got a 0-len packet, throw it back and try again
		}

		xfer = (req->actual < count) ? req->actual : count;
		spin_unlock_irqrestore(&iap2_dev->lock, flags);
		if (copy_to_user(buf, req->buf, xfer)){
			printk("%s occurs error:req->buf: %p,  buf: %p,  xfer: %d \n",
				__func__, req->buf, buf, xfer);
			r = -EFAULT;
		}
		else {
			r = xfer; //return actual value
		}

	} else{
		spin_unlock_irqrestore(&iap2_dev->lock, flags);
		r = -EIO;
		printk("carplay %s %s %d: rx_error\n", __FILE__, __func__, __LINE__);
	}

 done:
	iap2_unlock(&iap2_dev->read_excl);
	spin_lock_irqsave(&iap2_dev->lock, flags);
	iap2_dev->read_finish = READ_FINISH;
	iap2_dev->rx_error = 0;
	printk("carplay %s %s %d: iap2_dev %p, read_finish %d, rx_error %d, return: %d\n",
		__FILE__, __func__, __LINE__,
		iap2_dev, iap2_dev->read_finish, iap2_dev->rx_error, r);
	spin_unlock_irqrestore(&iap2_dev->lock, flags);

	return r;
}

static ssize_t iap2_write(struct file *fp, const char __user *buf, size_t count, loff_t *pos)
{
	struct f_iap2 *iap2_dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;
	unsigned long flags;
	//static int flow_state;

	if (!iap2_dev) {
		printk("carplay %s %s %d: iap2_dev is NULL\n", __FILE__, __func__, __LINE__);
		return -ENODEV;
	}

	if (iap2_lock(&iap2_dev->write_excl)) {
		printk("carplay %s %s %d: Failed, lock is busy\n", __FILE__, __func__, __LINE__);
		return -EBUSY;
	}

	while (count > 0) {
		spin_lock_irqsave(&iap2_dev->lock, flags);
		if (iap2_dev->tx_error || !iap2_dev->online) {
			printk("carplay %s %s %d:online(%d) tx_error(%d)\n", __FILE__, __func__, __LINE__,iap2_dev->online,iap2_dev->tx_error );
			iap2_dev->tx_error = 0;
			r = -EIO;
			spin_unlock_irqrestore(&iap2_dev->lock, flags);
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		spin_unlock_irqrestore(&iap2_dev->lock, flags);

		ret = wait_event_interruptible_timeout(iap2_dev->write_wq,
							(req = iap2_req_get(iap2_dev, &iap2_dev->tx_idle))
							|| iap2_dev->tx_error, 1*HZ);

		if (ret < 0) {
			r = ret;
			printk("carplay %s %s %d: wait_event_interruptible time out !!!!!!\n",
					__FILE__, __func__, __LINE__);
			break;
		}
		spin_lock_irqsave(&iap2_dev->lock, flags);
		if(iap2_dev->tx_error){
			r = -EIO;
			spin_unlock_irqrestore(&iap2_dev->lock, flags);
			break;
		}
		spin_unlock_irqrestore(&iap2_dev->lock, flags);

		if (req != 0) {
			if (count > IAP2_EP_BULK_BUF_SIZE)
				xfer = IAP2_EP_BULK_BUF_SIZE;
			else
				xfer = count;
			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				break;
			}

			req->length = xfer;
			ret = usb_ep_queue(iap2_dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				printk("carplay %s %s %d: xfer error %d\n", __FILE__, __func__, __LINE__,
					 ret);
				spin_lock_irqsave(&iap2_dev->lock, flags);
				iap2_dev->tx_error = 1;
				spin_unlock_irqrestore(&iap2_dev->lock, flags);
				r = -EIO;
				break;
			}

			buf += xfer;
			count -= xfer;

			/* zero this so we don't try to free it on error exit */
			req = 0;
		}else{
			printk("carplay %s %s %d: req == 0 \n", __FILE__, __func__, __LINE__ );
			r = -EIO;
		}

	}

	if (req)
		iap2_req_put(iap2_dev, &iap2_dev->tx_idle, req);

	iap2_unlock(&iap2_dev->write_excl);

	printk("carplay %s %s %d: return = %d\n", __FILE__, __func__, __LINE__, r);

	return r;
}

static unsigned int iap2_poll(struct file *file, poll_table *wait)
{
	struct f_iap2	*iap2_dev  = file->private_data;
	unsigned int	ret = 0;

	poll_wait(file, &iap2_dev->read_wq, wait);
	poll_wait(file, &iap2_dev->write_wq, wait);

	if (/*condition*/1)
		ret |= POLLIN | POLLRDNORM;

	return ret;
}

static int iap2_open(struct inode *ip, struct file *fp);
static int iap2_release(struct inode *ip, struct file *fp);

/* file operations for iap2 device /dev/giap2_dev */
static struct file_operations iap2_fops = {
	.owner = THIS_MODULE,
	.read = iap2_read,
	.write = iap2_write,
	.open = iap2_open,
	.poll = iap2_poll,
	.release = iap2_release,
};

static struct miscdevice iap2_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = iap2_dev_name,
	.fops = &iap2_fops,
};

static int open_release_pair;

static spinlock_t open_lock;

static int iap2_open(struct inode *ip, struct file *fp)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&open_lock, flags);

	if (!g_iap2_dev) {
		printk("carplay iap2_open g_iap2_dev is NULL\n");
		open_release_pair = 0;
		ret = ENODEV;
		goto OPEN_END;
	}

	if (open_release_pair > 0) {
		ret = 0;
		fp->private_data = g_iap2_dev;
		goto OPEN_END;
	}

	open_release_pair++;
	fp->private_data = g_iap2_dev;
	g_iap2_dev->tx_error = 0;
	g_iap2_dev->rx_error = 0;

 OPEN_END:
	printk("carplay open_release_pair: %d", open_release_pair);

	spin_unlock_irqrestore(&open_lock, flags);

	return ret;
}

static int iap2_release(struct inode *ip, struct file *fp)
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

	printk("carplay open_release_pair: %d", open_release_pair);

	spin_unlock_irqrestore(&open_lock, flags);

	return ret;
}

/*static void iap2_offline(void)
{
	g_iap2_dev->rx_done = 1;
	wake_up(&g_iap2_dev->read_wq);
	wake_up(&g_iap2_dev->write_wq);

	if (g_iap2_dev != NULL)
	g_iap2_dev->online = 0;
}*/

static void iap2_cleanup(void)
{
	printk("carplay %s %s %d \n", __FILE__, __func__, __LINE__);

	if(iap2_wq != NULL){
		flush_workqueue(iap2_wq);
		destroy_workqueue(iap2_wq);
		iap2_wq = NULL;
	}

	if(g_iap2_dev != NULL){
		kfree(g_iap2_dev);
		g_iap2_dev = NULL;
	}

	if(g_opts != NULL){
		kfree(g_opts);
		g_opts = NULL;
	}

	carplay_uevt_exit();
}

static void iap2_uevent_ready(struct work_struct *work)
{
	carplay_uevt_send(true);
}

static void iap2_uevent_disconnect(struct work_struct *work)
{
	carplay_uevt_send(false);
}



static int iap2_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_iap2 *iap2_dev = func_to_iap2(f);
	int ret;
	int status;

	printk("carplay %s %s %d: enter\n", __FILE__, __func__, __LINE__);
	iap2_dev->cdev = cdev;

	status = usb_interface_id(c, f);
	if (status < 0)
		return status;
	iap2_interface_desc.bInterfaceNumber = status;

	/* R29 P227, Interface : 0 / 0 (iAP Interface) */
	status = usb_string_id(c->cdev);
	if (status < 0)
		return status;
	iap2_string_defs[STRING_IAP2_IDX].id = status;
	iap2_interface_desc.iInterface = status;

	/* allocate endpoints */
	ret = iap2_create_bulk_ep(iap2_dev, &iap2_fs_bulkin_ep_desc, &iap2_fs_bulkout_ep_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		iap2_hs_bulkin_ep_desc.bEndpointAddress  = iap2_fs_bulkin_ep_desc.bEndpointAddress;
		iap2_hs_bulkout_ep_desc.bEndpointAddress = iap2_fs_bulkout_ep_desc.bEndpointAddress;
	}

	printk("carplay %s %s %d: leave\n", __FILE__, __func__, __LINE__);
	return 0;
}

static void iap2_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_iap2 *iap2_dev = func_to_iap2(f);
	struct usb_request *req;
	unsigned long flags;
	unsigned int txReqNum =  TX_REQ_MAX;
	int read_finish = 0;

	 printk("carplay %s %s %d: enter\n", __FILE__, __func__, __LINE__);
	 //iap2_cleanup();
	spin_lock_irqsave(&iap2_dev->lock, flags);
	iap2_dev->tx_error = 1;
	iap2_dev->online = 0;
	iap2_dev->rx_error = 1;
	spin_unlock_irqrestore(&iap2_dev->lock, flags);
	wake_up(&iap2_dev->read_wq);
	wake_up(&iap2_dev->write_wq);
	while(1) {
		spin_lock_irqsave(&iap2_dev->lock, flags);
		read_finish = iap2_dev->read_finish;
		printk("carplay %s %s %d : online(%d) rx_error(%d) read_finish(%d)\n",__FILE__, __func__, __LINE__,
		iap2_dev->online, iap2_dev->rx_error, iap2_dev->read_finish);
		spin_unlock_irqrestore(&iap2_dev->lock, flags);

		if (read_finish == 0) {
			msleep(TX_RX_DELAY);
			continue;
		} else if(read_finish == READ_FINISH|| read_finish == DEFAULT_STATUS) {
			break;
		}
	}

	usb_ep_disable(iap2_dev->ep_in);
	usb_ep_disable(iap2_dev->ep_out);
	iap2_request_free(iap2_dev->rx_req, iap2_dev->ep_out);
	while ((req = iap2_req_get(iap2_dev, &iap2_dev->tx_idle))||txReqNum){
		if(req!= 0){
			iap2_request_free(req, iap2_dev->ep_in);
			txReqNum--;
		}
		msleep(TX_RX_DELAY);
	}

	printk("carplay %s %s %d: leave\n", __FILE__, __func__, __LINE__);
}

extern bool g_carplay_setalt;
static int iap2_function_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_iap2 *iap2_dev = func_to_iap2(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;
	unsigned long flags;

	printk("carplay iap2_set_alt intf: %d alt: %d\n", intf, alt);

	ret = config_ep_by_speed(cdev->gadget, f, iap2_dev->ep_in);
	if (ret) {
		printk("carplay %s %s %d: usb_ep_enable in failed\n", __FILE__, __func__, __LINE__);
		goto finish;
	}
	ret = usb_ep_enable(iap2_dev->ep_in);
	if (ret)
		goto finish;

	ret = config_ep_by_speed(cdev->gadget, f, iap2_dev->ep_out);
	if (ret) {
		printk("carplay %s %s %d: usb_ep_enable out failed\n", __FILE__, __func__, __LINE__);
		goto finish;
	}

	ret = usb_ep_enable(iap2_dev->ep_out);
	if (ret) {
		usb_ep_disable(iap2_dev->ep_in);
		printk("carplay %s %s %d: usb_ep_enable in failed\n", __FILE__, __func__, __LINE__);
		goto finish;
	}

	spin_lock_irqsave(&iap2_dev->lock, flags);
	iap2_dev->online = 1;
	spin_unlock_irqrestore(&iap2_dev->lock, flags);

	wake_up(&iap2_dev->read_wq); /* readers may be blocked waiting for us to go online */

	/* send uevent to iap2 read */
	//carplay_uevt_send(true);
	if(iap2_wq){
		printk("carplay iap2_set_alt queue_work\n");
		queue_work(iap2_wq, &iap2_readywork);
	}

	printk("carplay iap2_set_alt intf leave\n");

finish:
	if (0 == ret)
		g_carplay_setalt = true;

	return 0;
}

static void iap2_function_disable(struct usb_function *f)
{
	printk("carplay iap2_function_disable enter \n");

	if(iap2_wq){
		printk("carplay iap2_function_disable queue_work\n");
		queue_work(iap2_wq, &iap2_disconnectwork);
	}

}

static int iap2_setup(void)
{
	struct f_iap2 *iap2_dev;
	unsigned long flags;

	carplay_uevt_init();

	if(g_iap2_dev == NULL){
		iap2_dev = kzalloc(sizeof(*iap2_dev), GFP_KERNEL);
		if (!iap2_dev)
			return -ENOMEM;

		INIT_WORK(&iap2_readywork, iap2_uevent_ready);
		INIT_WORK(&iap2_disconnectwork, iap2_uevent_disconnect);
		spin_lock_init(&iap2_dev->lock);
		spin_lock_init(&open_lock);

		init_waitqueue_head(&iap2_dev->read_wq);
		init_waitqueue_head(&iap2_dev->write_wq);

		atomic_set(&iap2_dev->open_excl, 0);
		atomic_set(&iap2_dev->read_excl, 0);
		atomic_set(&iap2_dev->write_excl, 0);

		INIT_LIST_HEAD(&iap2_dev->tx_idle);

		g_iap2_dev = iap2_dev;

		spin_lock_irqsave(&iap2_dev->lock, flags);
		iap2_dev->online = 0;
		//iap2_dev->tx_done = 0;
		iap2_dev->tx_error = 0;
		iap2_dev->rx_error = 0;
		iap2_dev->read_finish = DEFAULT_STATUS;
		spin_unlock_irqrestore(&iap2_dev->lock, flags);
	}

	if(iap2_wq == NULL){
		printk("%s %s %d: iap2 create_singlethread_workqueue\n", __FILE__, __func__, __LINE__);
		iap2_wq  = create_singlethread_workqueue("iap2");
		if (!iap2_wq) {
			printk("carplay %s: Unable to create workqueue: iap2\n", __func__);
			if(g_iap2_dev != NULL){
				kfree(g_iap2_dev);
				g_iap2_dev = NULL;
			}
			return -ENOMEM;
		}
	}

	return 0;

}


static void iap2_free_inst(struct usb_function_instance *f)
{
	printk("carplay %s %s %d enter\n", __FILE__, __func__, __LINE__);

	iap2_cleanup();
}

static struct usb_function_instance *iap2_alloc_inst(void)
{

	printk("carplay %s %s %d enter\n", __FILE__, __func__, __LINE__);

	if(g_opts == NULL){
		g_opts = kzalloc(sizeof(*g_opts), GFP_KERNEL);
		if (!g_opts)
			return ERR_PTR(-ENOMEM);

		mutex_init(&g_opts->lock);
		g_opts->func_inst.free_func_inst = iap2_free_inst;
		config_group_init_type_name(&g_opts->func_inst.group, "", &iap2_func_type);
	}

	if(iap2_setup() != 0){
		kfree(g_opts);
		return ERR_PTR(-ENOMEM);
	}

	return &g_opts->func_inst;
}

static void iap2_free(struct usb_function *f)
{
	struct f_iap2 *iap2_dev;
	struct f_iap2_opts *opts;
	printk("carplay %s %s %d enter\n", __FILE__, __func__, __LINE__);

	misc_deregister(&iap2_device);

	iap2_dev = func_to_iap2(f);
	opts = container_of(f->fi, struct f_iap2_opts, func_inst);
	//kfree(iap2_dev);
	mutex_lock(&opts->lock);
	opts->refcnt--;
	mutex_unlock(&opts->lock);
}


static struct usb_function *iap2_alloc(struct usb_function_instance *fi)
{
	struct f_iap2 *iap2_dev = g_iap2_dev;
	struct f_iap2_opts	*opts;
	int ret = 0;

	opts = container_of(fi, struct f_iap2_opts, func_inst);
	mutex_lock(&opts->lock);
	opts->refcnt++;
	mutex_unlock(&opts->lock);

	printk("carplay %s %s %d enter\n", __FILE__, __func__, __LINE__);

	ret = misc_register(&iap2_device);
	if (ret){
		printk("carplay iap2_alloc  misc_register fail\n");
	}

	iap2_dev->function.name = "iap2";
	iap2_dev->function.strings = iap2_strings;
	iap2_dev->function.fs_descriptors = iap2_fs_descriptors;
	iap2_dev->function.hs_descriptors = iap2_hs_descriptors;

	iap2_dev->function.bind = iap2_function_bind;
	iap2_dev->function.unbind = iap2_function_unbind;
	iap2_dev->function.set_alt = iap2_function_set_alt;
	iap2_dev->function.disable = iap2_function_disable;
	iap2_dev->function.free_func = iap2_free;

	printk("carplay iap2_alloc leave\n");

	return &iap2_dev->function;
}


DECLARE_USB_FUNCTION_INIT(iap2, iap2_alloc_inst, iap2_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("WYZ");
