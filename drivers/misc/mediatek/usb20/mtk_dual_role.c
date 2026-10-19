/*
 * Copyright (C) 2018 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/usb/class-dual-role.h>
#include "musb_core.h"
#include "mtk_musb.h"

#ifdef CONFIG_DUAL_ROLE_USB_INTF
static enum dual_role_property mt_dual_role_props[] = {
	DUAL_ROLE_PROP_MODE,
	DUAL_ROLE_PROP_PR,
	DUAL_ROLE_PROP_DR,
	/* DUAL_ROLE_PROP_VCONN_SUPPLY, */
};

static int mt_dual_role_get_prop(struct dual_role_phy_instance *dual_role,
			enum dual_role_property prop, unsigned int *val)
{
	struct musb *musb = dev_get_drvdata(dual_role->dev.parent);
	int ret = 0;
	int mode, pr, dr;

	dev_info(musb->controller, "prop=%d, power=%d, is_host=%d",
			 prop, musb->power, is_host_active(musb));

	if (/*musb->power*/ true) {
		if (is_host_active(musb)) {
			mode = DUAL_ROLE_PROP_MODE_DFP;
			pr = DUAL_ROLE_PROP_PR_SRC;
			dr = DUAL_ROLE_PROP_DR_HOST;
		} else {
			mode = DUAL_ROLE_PROP_MODE_UFP;
			pr = DUAL_ROLE_PROP_PR_SRC;
			dr = DUAL_ROLE_PROP_DR_DEVICE;
		}
	} else {
		mode = DUAL_ROLE_PROP_MODE_NONE;
		pr = DUAL_ROLE_PROP_PR_NONE;
		dr = DUAL_ROLE_PROP_DR_NONE;
	}

	switch (prop) {
	case DUAL_ROLE_PROP_MODE:
		*val = mode;
		break;
	case DUAL_ROLE_PROP_PR:
		*val = pr;
		break;
	case DUAL_ROLE_PROP_DR:
		*val = dr;
		break;
	case DUAL_ROLE_PROP_SUPPORTED_MODES:
		*val = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mt_dual_role_prop_is_writeable(
	struct dual_role_phy_instance *dual_role, enum dual_role_property prop)
{
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	return 1;
#else
	/* not support writeable */
	return 0;
#endif
}

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
extern void mt_usb_connect(void);
extern void mt_usb_dev_disconnect(void);
extern void mt_usb_host_connect(int delay);
extern void mt_usb_host_disconnect(int delay);
extern void musb_set_vbus_by_gpio(int is_on);
static int atc_dual_role_set_prop_dr(struct musb *musb,
				const unsigned int val)
{
#if 1
	if((val == DUAL_ROLE_PROP_DR_HOST && is_host_active(musb)) ||
		(val == DUAL_ROLE_PROP_DR_DEVICE && is_peripheral_active(musb))) {
		DBG(0, "atc_dual_role_set_prop_dr %d\n", is_host_active(musb));
		return 0;
	}
#endif

	switch (val) {
	case DUAL_ROLE_PROP_DR_HOST:
		DBG(0, "switch role to host\n");
		musb->role_switch_state = MSUB_ROLE_SWITCHING_DEV2HOS;
#ifdef CONFIG_ATC_USB_HSRX_DISC
		musb->hcd->flags &= ~(1U << HCD_FLAG_DISC_POLL_RDY);
#endif
		/*This is workaround to fix carplay QB can not
		* be recognized issue.
		*/
		musb_set_vbus_by_gpio(1);
		mt_usb_dev_disconnect();
		mt_usb_host_connect(0);
//		musb_platform_set_mode(mtk_musb, 1);
//		mt_usb_set_vbus(mtk_musb, 1);
		break;
	case DUAL_ROLE_PROP_DR_DEVICE:
		DBG(0, "switch role to device\n");
		musb->role_switch_state = MSUB_ROLE_SWITCHING_HOS2DEV;
#ifdef CONFIG_ATC_USB_HSRX_DISC
		musb->hcd->flags &= ~(1U << HCD_FLAG_DISC_POLL_RDY);
#endif
		mt_usb_host_disconnect(0);
		mt_usb_connect();
//		musb_platform_set_mode(musb, 2);
//		mt_usb_set_vbus(mtk_musb, 1);
		break;
	default:
		DBG(0, "atc_dual_role_set_prop_dr not support val=%d\n",val);
		break;
	}
	return 0;
}

static int atc_dual_role_set_prop_pr(struct musb *musb,
				const unsigned int val)
{
	DBG(0, "atc_dual_role_set_prop_pr val=%d\n",val);

	switch (val) {
	case DUAL_ROLE_PROP_PR_SRC:
		mt_usb_set_vbus(musb, 1);
		break;
	case DUAL_ROLE_PROP_PR_SNK:
		mt_usb_set_vbus(musb, 0);
		break;
	default:
		DBG(0, "atc_dual_role_set_prop_pr not support val=%d\n",val);
		break;
	}
	return 0;
}
#endif

static int mt_dual_role_set_prop(struct dual_role_phy_instance *dual_role,
			enum dual_role_property prop, const unsigned int *val)
{
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	struct musb *musb = dev_get_drvdata(dual_role->dev.parent);
	DBG(0, "mt_dual_role_set_prop prop=%d val=%d\n",prop,*val);

#ifdef CONFIG_ATC_MACRO_VTS_ENABLE
	DBG(0, "not set prop in vts test\n");
	return 0;
#endif

	switch (prop) {
	case DUAL_ROLE_PROP_PR:
		atc_dual_role_set_prop_pr(musb, *val);
		break;
	case DUAL_ROLE_PROP_DR:
		atc_dual_role_set_prop_dr(musb, *val);
		break;
	default:
		DBG(0, "not support prop=%d\n",prop);
		break;
	}
#endif

	return 0;
}

int mt_usb_dual_role_changed(struct musb *musb)
{
	if (musb->dr_usb)
		dual_role_instance_changed(musb->dr_usb);

	return 0;
}

int mt_usb_dual_role_init(struct musb *musb)
{
	struct dual_role_phy_desc *dual_desc;

	dual_desc = devm_kzalloc(mtk_musb->controller, sizeof(*dual_desc),
				GFP_KERNEL);

	if (!dual_desc)
		return -ENOMEM;

	dual_desc->name = "dual-role-usb20";
	dual_desc->supported_modes = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
	dual_desc->properties = mt_dual_role_props;
	dual_desc->num_properties = ARRAY_SIZE(mt_dual_role_props);
	dual_desc->get_property = mt_dual_role_get_prop;
	dual_desc->set_property = mt_dual_role_set_prop;
	dual_desc->property_is_writeable = mt_dual_role_prop_is_writeable;

	musb->dr_usb = devm_dual_role_instance_register(mtk_musb->controller,
							dual_desc);
	if (IS_ERR(musb->dr_usb)) {
		dev_info(musb->controller, "fail to register dual role usb\n");
		return -EINVAL;
	}
	return 0;
}
#endif /* CONFIG_DUAL_ROLE_USB_INTF */
