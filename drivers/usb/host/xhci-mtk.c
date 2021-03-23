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

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "xhci-debugfs.h"
#endif

#include "xhci.h"
#include "xhci-mtk.h"
#include "phy-mtk-ssusb-reg.h"

/* ip_pw_ctrl0 register */
#define CTRL0_IP_SW_RST	BIT(0)

/* ip_pw_ctrl1 register */
#define CTRL1_IP_HOST_PDN	BIT(0)

/* ip_pw_ctrl2 register */
#define CTRL2_IP_DEV_PDN	BIT(0)

/* ip_pw_sts1 register */
#define STS1_IP_SLEEP_STS	BIT(30)
#define STS1_XHCI_RST		BIT(11)
#define STS1_SYS125_RST	BIT(10)
#define STS1_REF_RST		BIT(8)
#define STS1_SYSPLL_STABLE	BIT(0)

/* ip_xhci_cap register */
#define CAP_U3_PORT_NUM(p)	((p) & 0xff)
#define CAP_U2_PORT_NUM(p)	(((p) >> 8) & 0xff)

/* u3_ctrl_p register */
#define CTRL_U3_PORT_HOST_SEL	BIT(2)
#define CTRL_U3_PORT_PDN	BIT(1)
#define CTRL_U3_PORT_DIS	BIT(0)

/* u2_ctrl_p register */
#define CTRL_U2_PORT_HOST_SEL	BIT(2)
#define CTRL_U2_PORT_PDN	BIT(1)
#define CTRL_U2_PORT_DIS	BIT(0)

/* u2_phy_pll register */
#define CTRL_U2_FORCE_PLL_STB	BIT(28)

#define PERI_WK_CTRL0		0x400
#define UWK_CTR0_0P_LS_PE	BIT(8)  /* posedge */
#define UWK_CTR0_0P_LS_NE	BIT(7)  /* negedge for 0p linestate*/
#define UWK_CTL1_1P_LS_C(x)	(((x) & 0xf) << 1)
#define UWK_CTL1_1P_LS_E	BIT(0)

#define PERI_WK_CTRL1		0x404
#define UWK_CTL1_IS_C(x)	(((x) & 0xf) << 26)
#define UWK_CTL1_IS_E		BIT(25)
#define UWK_CTL1_0P_LS_C(x)	(((x) & 0xf) << 21)
#define UWK_CTL1_0P_LS_E	BIT(20)
#define UWK_CTL1_IDDIG_C(x)	(((x) & 0xf) << 11)  /* cycle debounce */
#define UWK_CTL1_IDDIG_E	BIT(10) /* enable debounce */
#define UWK_CTL1_IDDIG_P	BIT(9)  /* polarity */
#define UWK_CTL1_0P_LS_P	BIT(7)
#define UWK_CTL1_IS_P		BIT(6)  /* polarity for ip sleep */

#define PERI_SSUSB_SPM_CTRL		0x514
/*#define PERI_SSUSB_SPM_CTRL		0x510 */
#define RG_SSUSB_SPM_INT_EN		BIT(1)
#define RG_SSUSB_IP_SLEEP_EN	BIT(4)

enum ssusb_wakeup_src {
	SSUSB_WK_IP_SLEEP = 1,
	SSUSB_WK_LINE_STATE = 2,
};

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
#ifdef CONFIG_ATC_USB_HSRX_DISC
bool xhci_disable_disconnect_flag;
EXPORT_SYMBOL_GPL(xhci_disable_disconnect_flag);
#endif

static void u3phywrite32(__le32 *addr, int offset, int mask, int value)
{
	int cur_value;
	int new_value;

	cur_value = readl(addr);
	new_value = (cur_value & (~mask)) | ((value << offset) & mask);
	writel(new_value, addr);
}

void xhci_mtk_u2phy_power_on (struct xhci_hcd_mtk *mtk)
{
	u32 value;
	struct u2_phy_regs __iomem *phy = mtk->u2phy_regs;
    /*
	 * force_uart_en	1'b0		0x68 26
	 * RG_UART_EN		1'b0		0x6c 16
	 * rg_usb20_gpio_ctl	1'b0		0x20 09
	 * usb20_gpio_mode	1'b0		0x20 08
	 * RG_USB20_BC11_SW_EN	1'b0		0x18 23
	 * rg_usb20_dp_100k_mode 1'b1		0x20 18
	 * USB20_DP_100K_EN	1'b0		0x20 16
	 * RG_USB20_DM_100K_EN	1'b0		0x20 17
	 * RG_USB20_OTG_VBUSCMP_EN	1'b1	0x18 20
	 * force_suspendm		1'b0	0x68 18
	*/

	/* force_uart_en, 1'b0 */
	value = readl(&phy->u2_phyd_tm0);
	value &= ~FORCE_UART_EN;
	writel(value, &phy->u2_phyd_tm0);

	/* RG_UART_EN, 1'b0 */
	value = readl(&phy->u2_phyd_tm1);
	value &= ~RG_UART_EN;
	writel(value, &phy->u2_phyd_tm1);

	/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
	value = readl(&phy->u2_phya_cr4);
	value &= ~(USB20_GPIO_MODE | RG_USB20_GPIO_CTL);
	writel(value, &phy->u2_phya_cr4);

	/* RG_USB20_BC11_SW_EN, 1'b0 */
	value = readl(&phy->usb_phya_cr6);
	value &= ~RG_USB20_BC11_SW_EN;
	writel(value, &phy->usb_phya_cr6);

	/* rg_usb20_dp_100k_mode, 1'b1 */
	value = readl(&phy->u2_phya_cr4);
	value |= RG_USB20_DP_100K_MODE;
	writel(value, &phy->u2_phya_cr4);

	/* USB20_DP_100K_EN 1'b0, RG_USB20_DM_100K_EN, 1'b0 */
	value = readl(&phy->u2_phya_cr4);
	value &= ~(USB20_DP_100K_EN | RG_USB20_DM_100K_EN);
	writel(value, &phy->u2_phya_cr4);

	/* RG_USB20_OTG_VBUSCMP_EN, 1'b1 */
	value = readl(&phy->usb_phya_cr6);
	value |= RG_USB20_OTG_VBUSCMP_EN;
	writel(value, &phy->usb_phya_cr6);

	/* force_suspendm, 1'b0 */
	value = readl(&phy->u2_phyd_tm0);
	value &= ~FORCE_SUSPENDM;
	writel(value, &phy->u2_phyd_tm0);

	/*set disconnect threshold to max value*/
	value = readl(&phy->usb_phya_cr6);
	value |= (0xf << 4);
	writel(value, &phy->usb_phya_cr6);

#ifdef CONFIG_ATC_USB_HSRX_DISC
	if (xhci_disable_disconnect_flag) {
		/*disable disconnect interrupt*/
		value = readl(&phy->usb_phya_cr6);
		value &= ~(0x30 << 8);
		value |= (0x20 << 8);
		writel(value, &phy->usb_phya_cr6);
	}
#endif

	/* wait for 800 usec. */
	udelay(800);
}

void xhci_mtk_u2phy_save_current (struct xhci_hcd_mtk *mtk)
{
	struct u2_phy_regs __iomem *u2phy = mtk->u2phy_regs;
	struct u3_phya_regs __iomem *u3phya = mtk->u3phya_regs;

	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_UART_EN_OFST,
		FORCE_UART_EN, 0);
	u3phywrite32(&u2phy->u2_phyd_tm1, RG_UART_EN_OFST,
		RG_UART_EN, 0);
	u3phywrite32(&u2phy->u2_phya_cr4, RG_USB20_GPIO_CTL_OFST,
		RG_USB20_GPIO_CTL, 0);
	u3phywrite32(&u2phy->u2_phya_cr4, USB20_GPIO_MODE_OFST,
		USB20_GPIO_MODE, 0);
	u3phywrite32(&u2phy->usb_phya_cr6, RG_USB20_BC11_SW_EN_OFST,
		RG_USB20_BC11_SW_EN, 0);
	u3phywrite32(&u2phy->usb_phya_cr6,
		 RG_USB20_OTG_VBUSCMP_EN_OFST,
		 RG_USB20_OTG_VBUSCMP_EN, 0);

	u3phywrite32(&u2phy->u2_phyd_tm0, RG_SUSPENDM_OFST,
		RG_SUSPENDM, 1);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_SUSPENDM_OFST,
		FORCE_SUSPENDM, 1);

	udelay(2000);

	u3phywrite32(&u2phy->u2_phyd_tm0, RG_DPPULLDOWN_OFST,
		RG_DPPULLDOWN, 1);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_DMPULLDOWN_OFST,
		RG_DMPULLDOWN, 1);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_XCVRSEL_OFST,
		RG_XCVRSEL, 0x1);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_TERMSEL_OFST,
		RG_TERMSEL, 1);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_DATAIN_OFST,
		RG_DATAIN, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_DP_PULLDOWN_OFST,
		FORCE_DP_PULLDOWN, 1);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_DM_PULLDOWN_OFST,
		FORCE_DM_PULLDOWN, 1);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_XCVRSEL_OFST,
		FORCE_XCVRSEL, 1);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_TERMSEL_OFST,
		FORCE_TERMSEL, 1);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_DATAIN_OFST,
		FORCE_DATAIN, 1);

	udelay(800);

	u3phywrite32(&u2phy->u2_phyd_tm0, RG_SUSPENDM_OFST,
		RG_SUSPENDM, 0);

	udelay(1);

	u3phywrite32(&u2phy->u2_phyd_tm1, RG_VBUSVALID_OFST,
		RG_VBUSVALID, 0);
	u3phywrite32(&u2phy->u2_phyd_tm1, RG_AVALID_OFST,
		RG_AVALID, 0);
	u3phywrite32(&u2phy->u2_phyd_tm1, RG_SESSEND_OFST,
		RG_SESSEND, 1);

	u3phywrite32(&u3phya->u3_phya_reg1, RG_SSUSB_VUSB10_ON_OFST,
		RG_SSUSB_VUSB10_ON, 1);

	udelay(10);
}

void xhci_mtk_u2phy_recovery (struct xhci_hcd_mtk *mtk)
{
	struct u2_phy_regs __iomem *u2phy = mtk->u2phy_regs;
	struct u3_phya_regs __iomem *u3phya = mtk->u3phya_regs;
	u32 value;

	u3phywrite32(&u3phya->u3_phya_reg1, RG_SSUSB_VUSB10_ON_OFST,
		RG_SSUSB_VUSB10_ON, 1);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_UART_EN_OFST,
		FORCE_UART_EN, 0);
	u3phywrite32(&u2phy->u2_phyd_tm1, RG_UART_EN_OFST,
		RG_UART_EN, 0);
	u3phywrite32(&u2phy->u2_phya_cr4, RG_USB20_GPIO_CTL_OFST,
		RG_USB20_GPIO_CTL, 0);
	u3phywrite32(&u2phy->u2_phya_cr4, USB20_GPIO_MODE_OFST,
		USB20_GPIO_MODE, 0);

	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_SUSPENDM_OFST,
		FORCE_SUSPENDM, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_DPPULLDOWN_OFST,
		RG_DPPULLDOWN, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_DMPULLDOWN_OFST,
		RG_DMPULLDOWN, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_XCVRSEL_OFST,
		RG_XCVRSEL, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_TERMSEL_OFST,
		RG_TERMSEL, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_DATAIN_OFST,
		RG_DATAIN, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_DP_PULLDOWN_OFST,
		FORCE_DP_PULLDOWN, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_DM_PULLDOWN_OFST,
		FORCE_DM_PULLDOWN, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_XCVRSEL_OFST,
		FORCE_XCVRSEL, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_TERMSEL_OFST,
		FORCE_TERMSEL, 0);
	u3phywrite32(&u2phy->u2_phyd_tm0, FORCE_DATAIN_OFST,
		FORCE_DATAIN, 0);
	u3phywrite32(&u2phy->usb_phya_cr6, RG_USB20_BC11_SW_EN_OFST,
		RG_USB20_BC11_SW_EN, 0);
	u3phywrite32(&u2phy->usb_phya_cr6, RG_USB20_OTG_VBUSCMP_EN_OFST,
		RG_USB20_OTG_VBUSCMP_EN, 1);

	/*set disconnect threshold to max value*/
	value = readl(&u2phy->usb_phya_cr6);
	value |= (0xf << 4);
	writel(value, &u2phy->usb_phya_cr6);

#ifdef CONFIG_ATC_USB_HSRX_DISC
	if (xhci_disable_disconnect_flag) {
		/*disable disconnect interrupt*/
		value = readl(&u2phy->usb_phya_cr6);
		value &= ~(0x30 << 8);
		value |= (0x20 << 8);
		writel(value, &u2phy->usb_phya_cr6);
	}
#endif

	/* Wait 800 usec */
	udelay(800);

	u3phywrite32(&u2phy->u2_phyd_tm1, RG_VBUSVALID_OFST,
		RG_VBUSVALID, 1);
	u3phywrite32(&u2phy->u2_phyd_tm1, RG_AVALID_OFST,
		RG_AVALID, 1);
	u3phywrite32(&u2phy->u2_phyd_tm1, RG_SESSEND_OFST,
		RG_SESSEND, 0);
}

void xhci_mtk_vbus_on (struct xhci_hcd_mtk *mtk)
{
	if (!gpio_is_valid(mtk->vbus_gpio)) {
		dev_warn(mtk->dev, "%s, gpio is invalid\n", __func__);
		return;
	}

	gpio_direction_output(mtk->vbus_gpio, 1);
	gpio_set_value(mtk->vbus_gpio, 0);
}

void xhci_mtk_vbus_off (struct xhci_hcd_mtk *mtk)
{
	if (!gpio_is_valid(mtk->vbus_gpio)) {
		dev_warn(mtk->dev, "%s, gpio is invalid\n", __func__);
		return;
	}

	gpio_direction_output(mtk->vbus_gpio, 1);
	gpio_set_value(mtk->vbus_gpio, 1);
}

struct xhci_hcd_mtk *hcd_mtk;
void xhci_set_vbus_by_gpio(int is_on)
{
	if (!hcd_mtk)
		return;

	if (is_on) {
		xhci_mtk_vbus_on(hcd_mtk);
	} else {
		xhci_mtk_vbus_off(hcd_mtk);
	}
}

#endif

static int xhci_mtk_host_enable(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value, check_val;
	int ret;
	int i;

	if (!mtk->has_ippc)
		return 0;

	/* power on host ip */
	value = readl(&ippc->ip_pw_ctr1);
	value &= ~CTRL1_IP_HOST_PDN;
	writel(value, &ippc->ip_pw_ctr1);

#ifdef	CONFIG_ATC_AOSP_ENHANCEMENT
	if (USB30 == mtk->speed) {
		/* power on and enable all u3 ports */
		for (i = 0; i < mtk->num_u3_ports; i++) {
			value = readl(&ippc->u3_ctrl_p[i]);
			value &= ~(CTRL_U3_PORT_PDN | CTRL_U3_PORT_DIS);
			value |= CTRL_U3_PORT_HOST_SEL;
			writel(value, &ippc->u3_ctrl_p[i]);
		}
	}
#else
	/* power on and enable all u3 ports */
	for (i = 0; i < mtk->num_u3_ports; i++) {
		value = readl(&ippc->u3_ctrl_p[i]);
		value &= ~(CTRL_U3_PORT_PDN | CTRL_U3_PORT_DIS);
		value |= CTRL_U3_PORT_HOST_SEL;
		writel(value, &ippc->u3_ctrl_p[i]);
	}
#endif

	/* power on and enable all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		value = readl(&ippc->u2_ctrl_p[i]);
		value &= ~(CTRL_U2_PORT_PDN | CTRL_U2_PORT_DIS);
		value |= CTRL_U2_PORT_HOST_SEL;
		writel(value, &ippc->u2_ctrl_p[i]);
	}

	/*
	 * wait for clocks to be stable, and clock domains reset to
	 * be inactive after power on and enable ports
	 */
	check_val = STS1_SYSPLL_STABLE | STS1_REF_RST |
			STS1_SYS125_RST | STS1_XHCI_RST;

	ret = readl_poll_timeout(&ippc->ip_pw_sts1, value,
			  (check_val == (value & check_val)), 100, 20000);
	if (ret) {
		dev_err(mtk->dev, "clocks are not stable (0x%x)\n", value);
		return ret;
	}

	return 0;
}

static int xhci_mtk_host_disable(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value;
	int ret;
	int i;

	if (!mtk->has_ippc)
		return 0;

#ifdef	CONFIG_ATC_AOSP_ENHANCEMENT
	if (USB30 == mtk->speed) {
		/* power down all u3 ports */
		for (i = 0; i < mtk->num_u3_ports; i++) {
			value = readl(&ippc->u3_ctrl_p[i]);
			value |= CTRL_U3_PORT_PDN;
			writel(value, &ippc->u3_ctrl_p[i]);
		}
	}
#else
	/* power down all u3 ports */
	for (i = 0; i < mtk->num_u3_ports; i++) {
		value = readl(&ippc->u3_ctrl_p[i]);
		value |= CTRL_U3_PORT_PDN;
		writel(value, &ippc->u3_ctrl_p[i]);
	}
#endif

	/* power down all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		value = readl(&ippc->u2_ctrl_p[i]);
		value |= CTRL_U2_PORT_PDN;
		writel(value, &ippc->u2_ctrl_p[i]);
	}

	/* power down host ip */
	value = readl(&ippc->ip_pw_ctr1);
	value |= CTRL1_IP_HOST_PDN;
	writel(value, &ippc->ip_pw_ctr1);

	/* wait for host ip to sleep */
	ret = readl_poll_timeout(&ippc->ip_pw_sts1, value,
			  (value & STS1_IP_SLEEP_STS), 100, 100000);
	if (ret) {
		dev_err(mtk->dev, "ip sleep failed!!!\n");
		return ret;
	}
	return 0;
}

static int xhci_mtk_ssusb_config(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 value;

	if (!mtk->has_ippc)
		return 0;

	/* reset whole ip */
	value = readl(&ippc->ip_pw_ctr0);
	value |= CTRL0_IP_SW_RST;
	writel(value, &ippc->ip_pw_ctr0);
	udelay(1);
	value = readl(&ippc->ip_pw_ctr0);
	value &= ~CTRL0_IP_SW_RST;
	writel(value, &ippc->ip_pw_ctr0);

	/*
	 * device ip is default power-on in fact
	 * power down device ip, otherwise ip-sleep will fail
	 */
	value = readl(&ippc->ip_pw_ctr2);
	value |= CTRL2_IP_DEV_PDN;
	writel(value, &ippc->ip_pw_ctr2);

	value = readl(&ippc->ip_xhci_cap);
	mtk->num_u3_ports = CAP_U3_PORT_NUM(value);
	mtk->num_u2_ports = CAP_U2_PORT_NUM(value);
	dev_dbg(mtk->dev, "%s u2p:%d, u3p:%d\n", __func__,
			mtk->num_u2_ports, mtk->num_u3_ports);

	return xhci_mtk_host_enable(mtk);
}

static int xhci_mtk_clks_enable(struct xhci_hcd_mtk *mtk)
{
	int ret;
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	u32	value;

	value = readl(mtk->ssusb_clk);
	value |= (0x1 << 1);
	value &= ~(0x1 << 2);
	writel(value, mtk->ssusb_clk);
	value = readl(mtk->ssusb_clk);
	value |= (0x1 << 4);
	writel(value, mtk->ssusb_clk);
#endif

	ret = clk_prepare_enable(mtk->ref_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable ref_clk\n");
		goto ref_clk_err;
	}

	ret = clk_prepare_enable(mtk->sys_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable sys_clk\n");
		goto sys_clk_err;
	}

	if (mtk->wakeup_src) {
		ret = clk_prepare_enable(mtk->wk_deb_p0);
		if (ret) {
			dev_err(mtk->dev, "failed to enable wk_deb_p0\n");
			goto usb_p0_err;
		}

		ret = clk_prepare_enable(mtk->wk_deb_p1);
		if (ret) {
			dev_err(mtk->dev, "failed to enable wk_deb_p1\n");
			goto usb_p1_err;
		}
	}
	return 0;

usb_p1_err:
	clk_disable_unprepare(mtk->wk_deb_p0);
usb_p0_err:
	clk_disable_unprepare(mtk->sys_clk);
sys_clk_err:
	clk_disable_unprepare(mtk->ref_clk);
ref_clk_err:
	return -EINVAL;
}

static void xhci_mtk_clks_disable(struct xhci_hcd_mtk *mtk)
{
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	u32 value;

	value = readl(mtk->ssusb_clk);
	value &= ~(0x1 << 4);
	writel(value, mtk->ssusb_clk);
#endif

	if (mtk->wakeup_src) {
		clk_disable_unprepare(mtk->wk_deb_p1);
		clk_disable_unprepare(mtk->wk_deb_p0);
	}
	clk_disable_unprepare(mtk->sys_clk);
	clk_disable_unprepare(mtk->ref_clk);
}

#ifndef CONFIG_ATC_AOSP_ENHANCEMENT
/* only clocks can be turn off for ip-sleep wakeup mode */
static void usb_wakeup_ip_sleep_en(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct regmap *pericfg = mtk->pericfg;

	if (mtk->uses_new_wakeup) {
		regmap_read(pericfg, PERI_SSUSB_SPM_CTRL, &tmp);
		tmp |= RG_SSUSB_SPM_INT_EN | RG_SSUSB_IP_SLEEP_EN;
		regmap_write(pericfg, PERI_SSUSB_SPM_CTRL, tmp);
		regmap_read(pericfg, PERI_SSUSB_SPM_CTRL, &tmp);
		dev_dbg(mtk->dev, "%s(): WK_CTRL=%#x\n", __func__, tmp);
	} else {
		regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
		tmp &= ~UWK_CTL1_IS_P;
		tmp &= ~(UWK_CTL1_IS_C(0xf));
		tmp |= UWK_CTL1_IS_C(0x8);
		regmap_write(pericfg, PERI_WK_CTRL1, tmp);
		regmap_write(pericfg, PERI_WK_CTRL1, tmp | UWK_CTL1_IS_E);

		regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
		dev_dbg(mtk->dev, "%s(): WK_CTRL1[P6,E25,C26:29]=%#x\n",
			__func__, tmp);
	}
}

static void usb_wakeup_ip_sleep_dis(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;

	if (mtk->uses_new_wakeup) {
		regmap_read(mtk->pericfg, PERI_SSUSB_SPM_CTRL, &tmp);
		tmp &= ~(RG_SSUSB_SPM_INT_EN | RG_SSUSB_IP_SLEEP_EN);
		regmap_write(mtk->pericfg, PERI_SSUSB_SPM_CTRL, tmp);
	} else {
		regmap_read(mtk->pericfg, PERI_WK_CTRL1, &tmp);
		tmp &= ~UWK_CTL1_IS_E;
		regmap_write(mtk->pericfg, PERI_WK_CTRL1, tmp);
	}
}

/*
* for line-state wakeup mode, phy's power should not power-down
* and only support cable plug in/out
*/
static void usb_wakeup_line_state_en(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct regmap *pericfg = mtk->pericfg;

	if (mtk->uses_new_wakeup)
		return;

	/* line-state of u2-port0 */
	regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
	tmp &= ~UWK_CTL1_0P_LS_P;
	tmp &= ~(UWK_CTL1_0P_LS_C(0xf));
	tmp |= UWK_CTL1_0P_LS_C(0x8);
	regmap_write(pericfg, PERI_WK_CTRL1, tmp);
	regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
	regmap_write(pericfg, PERI_WK_CTRL1, tmp | UWK_CTL1_0P_LS_E);

	/* line-state of u2-port1 */
	regmap_read(pericfg, PERI_WK_CTRL0, &tmp);
	tmp &= ~(UWK_CTL1_1P_LS_C(0xf));
	tmp |= UWK_CTL1_1P_LS_C(0x8);
	regmap_write(pericfg, PERI_WK_CTRL0, tmp);
	regmap_write(pericfg, PERI_WK_CTRL0, tmp | UWK_CTL1_1P_LS_E);
}

static void usb_wakeup_line_state_dis(struct xhci_hcd_mtk *mtk)
{
	u32 tmp;
	struct regmap *pericfg = mtk->pericfg;

	if (mtk->uses_new_wakeup)
		return;

	/* line-state of u2-port0 */
	regmap_read(pericfg, PERI_WK_CTRL1, &tmp);
	tmp &= ~UWK_CTL1_0P_LS_E;
	regmap_write(pericfg, PERI_WK_CTRL1, tmp);

	/* line-state of u2-port1 */
	regmap_read(pericfg, PERI_WK_CTRL0, &tmp);
	tmp &= ~UWK_CTL1_1P_LS_E;
	regmap_write(pericfg, PERI_WK_CTRL0, tmp);
}

static void usb_wakeup_enable(struct xhci_hcd_mtk *mtk)
{
	if (mtk->wakeup_src == SSUSB_WK_IP_SLEEP)
		usb_wakeup_ip_sleep_en(mtk);
	else if (mtk->wakeup_src == SSUSB_WK_LINE_STATE)
		usb_wakeup_line_state_en(mtk);
}

static void usb_wakeup_disable(struct xhci_hcd_mtk *mtk)
{
	if (mtk->wakeup_src == SSUSB_WK_IP_SLEEP)
		usb_wakeup_ip_sleep_dis(mtk);
	else if (mtk->wakeup_src == SSUSB_WK_LINE_STATE)
		usb_wakeup_line_state_dis(mtk);
}
#endif

static int usb_wakeup_of_property_parse(struct xhci_hcd_mtk *mtk,
				struct device_node *dn)
{
	struct device *dev = mtk->dev;

	/*
	* wakeup function is optional, so it is not an error if this property
	* does not exist, and in such case, no need to get relative
	* properties anymore.
	*/
	of_property_read_u32(dn, "mediatek,wakeup-src", &mtk->wakeup_src);
	if (!mtk->wakeup_src)
		return 0;

	mtk->wk_deb_p0 = devm_clk_get(dev, "wakeup_deb_p0");
	if (IS_ERR(mtk->wk_deb_p0)) {
		dev_err(dev, "fail to get wakeup_deb_p0\n");
		return PTR_ERR(mtk->wk_deb_p0);
	}

	mtk->wk_deb_p1 = devm_clk_get(dev, "wakeup_deb_p1");
	if (IS_ERR(mtk->wk_deb_p1)) {
		dev_err(dev, "fail to get wakeup_deb_p1\n");
		return PTR_ERR(mtk->wk_deb_p1);
	}

	mtk->pericfg = syscon_regmap_lookup_by_phandle(dn,
						"mediatek,syscon-wakeup");
	if (IS_ERR(mtk->pericfg)) {
		dev_err(dev, "fail to get pericfg regs\n");
		return PTR_ERR(mtk->pericfg);
	}

	return 0;
}

static int xhci_mtk_setup(struct usb_hcd *hcd);

static const struct xhci_driver_overrides xhci_mtk_overrides = {
	.extra_priv_size = sizeof(struct xhci_hcd),
	.reset = xhci_mtk_setup,
};

static struct hc_driver __read_mostly xhci_mtk_hc_driver;

static int xhci_mtk_phy_init(struct xhci_hcd_mtk *mtk)
{
	int i;
	int ret;

	for (i = 0; i < mtk->num_phys; i++) {
		ret = phy_init(mtk->phys[i]);
		if (ret)
			goto exit_phy;
	}
	return 0;

exit_phy:
	for (; i > 0; i--)
		phy_exit(mtk->phys[i - 1]);

	return ret;
}

static int xhci_mtk_phy_exit(struct xhci_hcd_mtk *mtk)
{
	int i;

	for (i = 0; i < mtk->num_phys; i++)
		phy_exit(mtk->phys[i]);

	return 0;
}

static int xhci_mtk_phy_power_on(struct xhci_hcd_mtk *mtk)
{
	int i;
	int ret;

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	xhci_mtk_u2phy_recovery(mtk);
#endif

	for (i = 0; i < mtk->num_phys; i++) {
		ret = phy_power_on(mtk->phys[i]);
		if (ret)
			goto power_off_phy;
	}
	return 0;

power_off_phy:
	for (; i > 0; i--)
		phy_power_off(mtk->phys[i - 1]);

	return ret;
}

static void xhci_mtk_phy_power_off(struct xhci_hcd_mtk *mtk)
{
	unsigned int i;

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	xhci_mtk_u2phy_save_current (mtk);
#endif

	for (i = 0; i < mtk->num_phys; i++)
		phy_power_off(mtk->phys[i]);
}

static int xhci_mtk_ldos_enable(struct xhci_hcd_mtk *mtk)
{
	int ret;

	ret = regulator_enable(mtk->vbus);
	if (ret) {
		dev_err(mtk->dev, "failed to enable vbus\n");
		return ret;
	}

	ret = regulator_enable(mtk->vusb33);
	if (ret) {
		dev_err(mtk->dev, "failed to enable vusb33\n");
		regulator_disable(mtk->vbus);
		return ret;
	}
	return 0;
}

static void xhci_mtk_ldos_disable(struct xhci_hcd_mtk *mtk)
{
	regulator_disable(mtk->vbus);
	regulator_disable(mtk->vusb33);
}

static void xhci_mtk_quirks(struct device *dev, struct xhci_hcd *xhci)
{
	struct usb_hcd *hcd = xhci_to_hcd(xhci);
	struct xhci_hcd_mtk *mtk = hcd_to_mtk(hcd);

	/*
	 * As of now platform drivers don't provide MSI support so we ensure
	 * here that the generic code does not try to make a pci_dev from our
	 * dev struct in order to setup MSI
	 */
	xhci->quirks |= XHCI_PLAT;
	xhci->quirks |= XHCI_MTK_HOST;
	/*
	 * MTK host controller gives a spurious successful event after a
	 * short transfer. Ignore it.
	 */
	xhci->quirks |= XHCI_SPURIOUS_SUCCESS;
	if (mtk->lpm_support)
		xhci->quirks |= XHCI_LPM_SUPPORT;

	/*
	 * MTK xHCI 0.96: PSA is 1 by default even if doesn't support stream,
	 * and it's 3 when support it.
	 */
	if (xhci->hci_version < 0x100 && HCC_MAX_PSA(xhci->hcc_params) == 4)
		xhci->quirks |= XHCI_BROKEN_STREAMS;
}

/* called during probe() after chip reset completes */
static int xhci_mtk_setup(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	struct xhci_hcd_mtk *mtk = hcd_to_mtk(hcd);
	int ret;

	if (usb_hcd_is_primary_hcd(hcd)) {
		ret = xhci_mtk_ssusb_config(mtk);
		if (ret)
			return ret;
	}

	ret = xhci_gen_setup(hcd, xhci_mtk_quirks);
	if (ret)
		return ret;

	if (usb_hcd_is_primary_hcd(hcd)) {
		mtk->num_u3_ports = xhci->num_usb3_ports;
		mtk->num_u2_ports = xhci->num_usb2_ports;
		ret = xhci_mtk_sch_init(mtk);
		if (ret)
			return ret;
	}

	return ret;
}

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
static void xhci_check_usb_speed(struct xhci_hcd_mtk *mtk)
{
	if (strstr(saved_command_line, "xh=full"))
		mtk->speed = USB11;
	else if (strstr(saved_command_line, "xh=high"))
		mtk->speed = USB20;
	else
		mtk->speed = USB30;

	dev_info(mtk->dev, "current support usb protocol : %s\n", \
		(mtk->speed == USB30) ? "USB3.0" : \
		((mtk->speed == USB20) ? "USB2.0" : "USB1.1"));
}

#ifdef CONFIG_ATC_USB_HSRX_DISC
static void xhci_disconnect_shorcut(struct xhci_hcd_mtk *mtk)
{
	if (strstr(saved_command_line, "U1_Dis=enable"))
		xhci_disable_disconnect_flag = true;
	else
		xhci_disable_disconnect_flag = false;

	dev_info(mtk->dev, "current xhci %s disconnect mechanism\n",
		xhci_disable_disconnect_flag ? "Enable" : "Disable");
}
#endif
#endif

static int xhci_mtk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct xhci_hcd_mtk *mtk;
	const struct hc_driver *driver;
	struct xhci_hcd *xhci;
	struct resource *res;
	struct usb_hcd *hcd;
	struct phy *phy;
	int phy_num;
	int ret = -ENODEV;
	int irq;

	if (usb_disabled())
		return -ENODEV;

	driver = &xhci_mtk_hc_driver;
	mtk = devm_kzalloc(dev, sizeof(*mtk), GFP_KERNEL);
	if (!mtk)
		return -ENOMEM;

	mtk->dev = dev;
	mtk->vbus = devm_regulator_get(dev, "vbus");
	if (IS_ERR(mtk->vbus)) {
		dev_err(dev, "fail to get vbus\n");
		return PTR_ERR(mtk->vbus);
	}

	mtk->vusb33 = devm_regulator_get(dev, "vusb33");
	if (IS_ERR(mtk->vusb33)) {
		dev_err(dev, "fail to get vusb33\n");
		return PTR_ERR(mtk->vusb33);
	}

	mtk->sys_clk = devm_clk_get(dev, "sys_ck");
	if (IS_ERR(mtk->sys_clk)) {
		dev_err(dev, "fail to get sys_ck\n");
		return PTR_ERR(mtk->sys_clk);
	}

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	hcd_mtk = mtk;
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "clk");
	if (res) {
		mtk->ssusb_clk = devm_ioremap_resource(dev, res);
		if (IS_ERR(mtk->ssusb_clk)) {
			ret = PTR_ERR(mtk->ippc_regs);
			return ret;
		}
	}
#endif
	/*
	 * reference clock is usually a "fixed-clock", make it optional
	 * for backward compatibility and ignore the error if it does
	 * not exist.
	 */
	mtk->ref_clk = devm_clk_get(dev, "ref_ck");
	if (IS_ERR(mtk->ref_clk)) {
		if (PTR_ERR(mtk->ref_clk) == -EPROBE_DEFER)
			return -EPROBE_DEFER;

		mtk->ref_clk = NULL;
	}

	mtk->lpm_support = of_property_read_bool(node, "usb3-lpm-capable");
	mtk->uses_new_wakeup = of_property_read_bool(node, "mediatek,new-wakeup");

	ret = usb_wakeup_of_property_parse(mtk, node);
	if (ret)
		return ret;

	mtk->num_phys = of_count_phandle_with_args(node,
			"phys", "#phy-cells");
	if (mtk->num_phys > 0) {
		mtk->phys = devm_kcalloc(dev, mtk->num_phys,
					sizeof(*mtk->phys), GFP_KERNEL);
		if (!mtk->phys)
			return -ENOMEM;
	} else {
		mtk->num_phys = 0;
	}

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	device_enable_async_suspend(dev);

	ret = xhci_mtk_ldos_enable(mtk);
	if (ret)
		goto disable_pm;

	ret = xhci_mtk_clks_enable(mtk);
	if (ret)
		goto disable_ldos;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto disable_clk;
	}

	/* Initialize dma_mask and coherent_dma_mask to 32-bits */
	ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
	if (ret)
		goto disable_clk;

	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;
	else
		dma_set_mask(dev, DMA_BIT_MASK(32));

	hcd = usb_create_hcd(driver, dev, dev_name(dev));
	if (!hcd) {
		ret = -ENOMEM;
		goto disable_clk;
	}

	/*
	 * USB 2.0 roothub is stored in the platform_device.
	 * Swap it with mtk HCD.
	 */
	mtk->hcd = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, mtk);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mac");
	hcd->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(hcd->regs)) {
		ret = PTR_ERR(hcd->regs);
		goto put_usb2_hcd;
	}
	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ippc");
	if (res) {	/* ippc register is optional */
		mtk->ippc_regs = devm_ioremap_resource(dev, res);
		if (IS_ERR(mtk->ippc_regs)) {
			ret = PTR_ERR(mtk->ippc_regs);
			goto put_usb2_hcd;
		}
		mtk->has_ippc = true;
	} else {
		mtk->has_ippc = false;
	}

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "u2phy");
	if (res) {
		mtk->u2phy_regs = devm_ioremap_resource(dev, res);
		if (IS_ERR(mtk->u2phy_regs)) {
			ret = PTR_ERR(mtk->u2phy_regs);
			goto put_usb2_hcd;
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "u3phya");
	if (res) {
		mtk->u3phya_regs = devm_ioremap_resource(dev, res);
		if (IS_ERR(mtk->u3phya_regs)) {
			ret = PTR_ERR(mtk->u3phya_regs);
			goto put_usb2_hcd;
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "u2csr");
	if (res) {
		mtk->u2csr_regs = devm_ioremap_resource(dev, res);
		if (IS_ERR(mtk->u2csr_regs)) {
			ret = PTR_ERR(mtk->u2csr_regs);
			goto put_usb2_hcd;
		}
	}

	mtk->vbus_gpio = of_get_named_gpio(node, "gpio_vbus", 0);
	if (gpio_is_valid(mtk->vbus_gpio)) {
		dev_info(dev, "get gpio vbus is %d\n", mtk->vbus_gpio);
		gpio_request(mtk->vbus_gpio, "USB30_HOST_VBUS_GPIO");
	}

	xhci_check_usb_speed(mtk);
#ifdef CONFIG_ATC_USB_HSRX_DISC
	xhci_disconnect_shorcut(mtk);
#endif
#endif

	for (phy_num = 0; phy_num < mtk->num_phys; phy_num++) {
		phy = devm_of_phy_get_by_index(dev, node, phy_num);
		if (IS_ERR(phy)) {
			ret = PTR_ERR(phy);
			goto put_usb2_hcd;
		}
		mtk->phys[phy_num] = phy;
	}

	ret = xhci_mtk_phy_init(mtk);
	if (ret)
		goto put_usb2_hcd;

	ret = xhci_mtk_phy_power_on(mtk);
	if (ret)
		goto exit_phys;

	device_init_wakeup(dev, true);

	xhci = hcd_to_xhci(hcd);
	xhci->main_hcd = hcd;
	xhci->shared_hcd = usb_create_shared_hcd(driver, dev,
			dev_name(dev), hcd);
	if (!xhci->shared_hcd) {
		ret = -ENOMEM;
		goto power_off_phys;
	}

	ret = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (ret)
		goto put_usb3_hcd;

	if (HCC_MAX_PSA(xhci->hcc_params) >= 4 &&
	    !(xhci->quirks & XHCI_BROKEN_STREAMS))
		xhci->shared_hcd->can_do_streams = 1;

	ret = usb_add_hcd(xhci->shared_hcd, irq, IRQF_SHARED);
	if (ret)
		goto dealloc_usb2_hcd;

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	if (USB30 != mtk->speed) {
		u32 i, value;
		u32 __iomem *addr;

		value = readl(&(mtk->ippc_regs->ip_xhci_cap));
		mtk->num_u3_ports = CAP_U3_PORT_NUM(value);
		for(i = 1; i <= mtk->num_u3_ports; i++) {
			addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((i - 1) & 0xff);
			value = readl(addr);
			value = xhci_port_state_to_neutral(value);
			value &= ~PORT_POWER;
			writel(value, addr);
		}
	}

	if (USB11 == mtk->speed) {
		u32 val;

#define HS_ENABLE BIT(5)
		val = readl(&(mtk->u2csr_regs->power_management));
		val &= (~HS_ENABLE);
		writel(val, &(mtk->u2csr_regs->power_management));
	}

	//writel(0x10340, hcd->regs + 0x420);
	xhci_init_debugfs(mtk);
#endif

	return 0;

dealloc_usb2_hcd:
	usb_remove_hcd(hcd);

put_usb3_hcd:
	xhci_mtk_sch_exit(mtk);
	usb_put_hcd(xhci->shared_hcd);

power_off_phys:
	xhci_mtk_phy_power_off(mtk);
	device_init_wakeup(dev, false);

exit_phys:
	xhci_mtk_phy_exit(mtk);

put_usb2_hcd:
	usb_put_hcd(hcd);

disable_clk:
	xhci_mtk_clks_disable(mtk);

disable_ldos:
	xhci_mtk_ldos_disable(mtk);

disable_pm:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	return ret;
}

static int xhci_mtk_remove(struct platform_device *dev)
{
	struct xhci_hcd_mtk *mtk = platform_get_drvdata(dev);
	struct usb_hcd	*hcd = mtk->hcd;
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	usb_remove_hcd(xhci->shared_hcd);
	xhci_mtk_phy_power_off(mtk);
	xhci_mtk_phy_exit(mtk);
	device_init_wakeup(&dev->dev, false);

	usb_remove_hcd(hcd);
	usb_put_hcd(xhci->shared_hcd);
	usb_put_hcd(hcd);
	xhci_mtk_sch_exit(mtk);
	xhci_mtk_clks_disable(mtk);
	xhci_mtk_ldos_disable(mtk);
	pm_runtime_put_sync(&dev->dev);
	pm_runtime_disable(&dev->dev);

	return 0;
}

/*
 * if ip sleep fails, and all clocks are disabled, access register will hang
 * AHB bus, so stop polling roothubs to avoid regs access on bus suspend.
 * and no need to check whether ip sleep failed or not; this will cause SPM
 * to wake up system immediately after system suspend complete if ip sleep
 * fails, it is what we wanted.
 */
static int __maybe_unused xhci_mtk_suspend(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd *hcd = mtk->hcd;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

#ifndef CONFIG_ATC_AOSP_ENHANCEMENT
	xhci_dbg(xhci, "%s: stop port polling\n", __func__);
	clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	del_timer_sync(&hcd->rh_timer);
	clear_bit(HCD_FLAG_POLL_RH, &xhci->shared_hcd->flags);
	del_timer_sync(&xhci->shared_hcd->rh_timer);
#else
	int ret;

	xhci_info(xhci, "===> %s\n", __func__);

	ret = xhci_suspend(xhci, device_may_wakeup(dev));
	if (ret) {
		dev_err(mtk->dev, "failed to suspend xhci, fail %d\n", ret);
		return ret;
	}
#endif

	xhci_mtk_host_disable(mtk);
	xhci_mtk_phy_power_off(mtk);
	xhci_mtk_clks_disable(mtk);

#ifndef CONFIG_ATC_AOSP_ENHANCEMENT
	usb_wakeup_enable(mtk);
#else
	xhci_mtk_vbus_off(mtk);
#endif
	return 0;
}

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
void usb_xhci_phy_reset(void)
{
	struct u2_phy_regs __iomem *u2phy;
	if (!hcd_mtk)
		return;

	u2phy = hcd_mtk->u2phy_regs;
	/* Reset USB PHY to default state */
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_RESET_OFST,
		RG_RESET, 1);
	mdelay(10);
	u3phywrite32(&u2phy->u2_phyd_tm0, RG_RESET_OFST,
		RG_RESET, 0);
}
#endif

static int __maybe_unused xhci_mtk_resume(struct device *dev)
{
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd *hcd = mtk->hcd;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

	xhci_info(xhci, "===> %s\n", __func__);

#ifndef CONFIG_ATC_AOSP_ENHANCEMENT
	usb_wakeup_disable(mtk);
#else
	xhci_mtk_vbus_on(mtk);
#endif
	xhci_mtk_clks_enable(mtk);
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	xhci_info(xhci, "Reset XHCI PHY\n");
	usb_xhci_phy_reset();
#endif
	xhci_mtk_phy_power_on(mtk);

#ifndef CONFIG_ATC_AOSP_ENHANCEMENT
	xhci_mtk_host_enable(mtk);
	xhci_dbg(xhci, "%s: restart port polling\n", __func__);
	set_bit(HCD_FLAG_POLL_RH, &xhci->shared_hcd->flags);
	usb_hcd_poll_rh_status(xhci->shared_hcd);
	set_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	usb_hcd_poll_rh_status(hcd);
	return 0;
#else
	xhci_mtk_ssusb_config(mtk);
	return xhci_resume(xhci, true);
#endif
}

static const struct dev_pm_ops xhci_mtk_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xhci_mtk_suspend, xhci_mtk_resume)
};
#define DEV_PM_OPS IS_ENABLED(CONFIG_PM) ? &xhci_mtk_pm_ops : NULL

static const struct dev_pm_ops xhci_mtk_phone_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(xhci_mtk_suspend, xhci_mtk_resume)
};
#define DEV_PHONE_PM_OPS (IS_ENABLED(CONFIG_PM) ? &xhci_mtk_phone_pm_ops : NULL)


#ifdef CONFIG_OF
static const struct of_device_id mtk_xhci_of_match[] = {
	{ .compatible = "mediatek,ac8257-xhci"},
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_xhci_of_match);
#endif

static struct platform_driver mtk_xhci_driver = {
	.probe	= xhci_mtk_probe,
	.remove	= xhci_mtk_remove,
	.driver	= {
		.name = "xhci-mtk",
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(mtk_xhci_of_match),
	},
};

MODULE_ALIAS("platform:xhci-mtk");

static int __init xhci_mtk_init(void)
{
	xhci_init_driver(&xhci_mtk_hc_driver, &xhci_mtk_overrides);
	return platform_driver_register(&mtk_xhci_driver);
}
module_init(xhci_mtk_init);

static void __exit xhci_mtk_exit(void)
{
	platform_driver_unregister(&mtk_xhci_driver);
}
module_exit(xhci_mtk_exit);

MODULE_AUTHOR("Chunfeng Yun <chunfeng.yun@mediatek.com>");
MODULE_DESCRIPTION("MediaTek xHCI Host Controller Driver");
MODULE_LICENSE("GPL v2");
