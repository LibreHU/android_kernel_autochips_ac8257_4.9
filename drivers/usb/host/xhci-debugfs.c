/*
 * MediaTek xHCI Host Controller Driver
 *
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include "xhci-debugfs.h"

/* TESTMODE */
#define XHCI_TEST_MODE_DISABLE		(0x0)
#define XHCI_TEST_J					(0x1)
#define XHCI_TEST_K					(0x2)
#define XHCI_SE0_NAK				(0x3)
#define XHCI_TEST_PACKET			(0x4)
#define XHCI_FORCE_ENABLE			(0x5)

#define USB3_PORT_LWS	(BIT(16))
#define USB3_PORT_PLS	(BIT(5) | BIT(6) | BIT(7) | BIT(8))
#define USB3_PORT_PLS_COMPLIANCE_MODE	(BIT(6) | BIT(8))

static struct dentry *xhci_debugfs_root;

static int xhci_compliance_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, inode->i_private);
}

static ssize_t xhci_compliance_mode_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	char buf[18];
	struct seq_file *s = file->private_data;
	struct xhci_hcd_mtk *mtk = s->private;
	struct usb_hcd *hcd;
	struct xhci_hcd *xhci;
	u32 value;

	memset(buf, 0x00, sizeof(buf));

	if (!mtk)
		return -EINVAL;

	hcd = mtk->hcd;
	if (!hcd)
		return -EINVAL;

	xhci = hcd_to_xhci(hcd);
	if (!xhci)
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count))) {
		xhci_err(xhci, "%s: copy fail error is : %d\n",
			__func__, EFAULT);
		return -EFAULT;
	}

	xhci_info(xhci, "%s: CMD = %s\n", __func__, buf);

	if (!strncmp(buf, "force compliance", 16)) {
		xhci_info(xhci, "%s: force compliance mode\n", __func__);

		value = readl(&xhci->op_regs->port_status_base);
		value &= ~USB3_PORT_PLS;
		value |= (USB3_PORT_PLS_COMPLIANCE_MODE | USB3_PORT_LWS);
		writel(value, &xhci->op_regs->port_status_base);
	}

	xhci_info(xhci, "%s: [COMPLIANCE MODE] end\n", __func__);

	return count;
}


static int xhci_test_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, inode->i_private);
}

static ssize_t xhci_test_mode_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct xhci_hcd_mtk *mtk = s->private;
	struct usb_hcd *hcd;
	struct xhci_hcd *xhci;
	char buf[18];

	if (!mtk)
		return -EINVAL;

	hcd = mtk->hcd;
	if (!hcd)
		return -EINVAL;

	xhci = hcd_to_xhci(hcd);
	if (!xhci)
		return -EINVAL;

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count))) {
		xhci_err(xhci, "%s: copy fail error is : %d\n",
				__func__, EFAULT);
		return -EFAULT;
	}

	xhci_info(xhci, "%s: CMD = %s\n", __func__, buf);

	if (!strncmp(buf, "open", 4)) {
		xhci_info(xhci, "%s: open\n", __func__);
		/*clear test mode if has been in test mode*/
		xhci_hub_control(hcd, ClearPortFeature, USB_PORT_FEAT_TEST,
				(XHCI_TEST_MODE_DISABLE << 8) | 0x1, NULL, 0);
	}

	if (!strncmp(buf, "close", 5)) {
		xhci_info(xhci, "%s: close\n", __func__);
		xhci_hub_control(hcd, ClearPortFeature, USB_PORT_FEAT_TEST,
				(XHCI_TEST_MODE_DISABLE << 8) | 0x1,  NULL, 0);
	}

	if (!strncmp(buf, "suspend", 7)) {
		xhci_info(xhci, "%s: test suspend\n", __func__);
		/*Not verified*/
		xhci_hub_control(hcd, SetPortFeature, USB_PORT_FEAT_SUSPEND,
				0x1,  NULL, 0);
	}

	if (!strncmp(buf, "resume", 6)) {
		xhci_info(xhci, "%s: test resume\n", __func__);
		/*Not verified*/
		xhci_hub_control(hcd, ClearPortFeature, USB_PORT_FEAT_SUSPEND,
				0x1,  NULL, 0);
	}

	if (!strncmp(buf, "test J", 6)) {
		xhci_info(xhci, "%s: test J\n", __func__);
		xhci_hub_control(hcd, ClearPortFeature, USB_PORT_FEAT_TEST,
				0x1,  NULL, 0);
		xhci_hub_control(hcd, SetPortFeature, USB_PORT_FEAT_TEST,
				(XHCI_TEST_J << 8) | 0x1,  NULL, 0);
	}

	if (!strncmp(buf, "test K", 6)) {
		xhci_info(xhci, "%s: test K\n", __func__);
		xhci_hub_control(hcd, ClearPortFeature, USB_PORT_FEAT_TEST
				, 0x1,  NULL, 0);
		xhci_hub_control(hcd, SetPortFeature, USB_PORT_FEAT_TEST,
				(XHCI_TEST_K << 8) | 0x1,  NULL, 0);
	}

	if (!strncmp(buf, "test SE0 NAK", 12)) {
		xhci_info(xhci, "%s: test SE0 NAK\n", __func__);
		xhci_hub_control(hcd, ClearPortFeature, USB_PORT_FEAT_TEST,
				0x1,  NULL, 0);
		xhci_hub_control(hcd, SetPortFeature, USB_PORT_FEAT_TEST,
				(XHCI_SE0_NAK << 8) | 0x1,  NULL, 0);
	}

	if (!strncmp(buf, "test packet", 10)) {
		xhci_info(xhci, "%s: test packet\n", __func__);
		xhci_hub_control(hcd, ClearPortFeature, USB_PORT_FEAT_TEST,
				0x1,  NULL, 0);
		xhci_hub_control(hcd, SetPortFeature, USB_PORT_FEAT_TEST,
				(XHCI_TEST_PACKET << 8) | 0x1,  NULL, 0);
	}

	if (!strncmp(buf, "test force enable", 17)) {
		xhci_info(xhci, "%s: test force enable\n", __func__);
		xhci_hub_control(hcd, ClearPortFeature, USB_PORT_FEAT_TEST,
				0x1,  NULL, 0);
		xhci_hub_control(hcd, SetPortFeature, USB_PORT_FEAT_TEST,
				(XHCI_FORCE_ENABLE << 8) | 0x1,  NULL, 0);
	}

	xhci_info(xhci, "%s: end\n", __func__);

	return count;
}

static const struct file_operations xhci_compliance_mode_fops = {
	.open = xhci_compliance_mode_open,
	.write = xhci_compliance_mode_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations xhci_test_mode_fops = {
	.open = xhci_test_mode_open,
	.write = xhci_test_mode_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int xhci_init_debugfs(struct xhci_hcd_mtk *mtk)
{
	struct dentry *root;
	struct dentry *file;
	int ret;

	root = debugfs_create_dir("xhci", NULL);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}

	file = debugfs_create_file("compliancemode", 0644,
				root, mtk, &xhci_compliance_mode_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("testmode", 0644,
				root, mtk, &xhci_test_mode_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	xhci_debugfs_root = root;

	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

void /* __init_or_exit */ xhci_exit_debugfs(struct xhci_hcd_mtk *mtk)
{
	debugfs_remove_recursive(xhci_debugfs_root);
}
