// SPDX-License-Identifier: GPL-2.0
/*
 * PCIe host controller driver for Marvell Berlin SoCs
 *
 * Copyright (C) 2018 Synaptics Incorporated
 * Copyright (C) 2015 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>

#include "../../pci.h"
#include "pcie-designware.h"

#define APPS_PM_XMT_TURNOFF	BIT(18)

#define LTSSM_STATE_MASK	GENMASK(10, 5)
#define LTSSM_STATE_L2_IDLE	FIELD_PREP(LTSSM_STATE_MASK, 0x15)

#define SYMBOL_TIMER_FILTER_1	0x71c
#define MASK_RADM_1		GENMASK(31, 16)

#define to_berlin_pcie(x)	dev_get_drvdata((x)->dev)

#define BERLIN_ATU_OUTB_UNR_REG_OFFSET(region) (0x1000 + (region << 9))

struct berlin_pcie_data {
	u32 intr_status;
	u32 intr_status1;
	u32 intr_mask;
	u32 intr_mask1;
	u32 intr_mask_val;
	u32 ctrl;
	int (*host_init)(struct pcie_port *pp);
};

struct berlin_pcie {
	struct dw_pcie		pci;
	void __iomem		*ctrl;
	const struct berlin_pcie_data *data;
	u32			pcie_cap_base;
	struct gpio_desc	*reset_gpio;
	struct gpio_descs	*power_gpios;
	struct gpio_desc	*enable_gpio;
	struct clk		*clk;
	struct reset_control	*rc_rst;
	struct phy		*phy;
	int			link_gen;
};

static void berlin_pcie_enable_irq_pulse(struct berlin_pcie *priv)
{
	u32 val;

	writel(0xffffffff, priv->ctrl + priv->data->intr_mask);
	if (priv->data->intr_mask1)
		writel(0xffffffff, priv->ctrl + priv->data->intr_mask1);

	val = readl(priv->ctrl + priv->data->intr_status);
	writel(val, priv->ctrl + priv->data->intr_status);
	if (priv->data->intr_status1) {
		val = readl(priv->ctrl + priv->data->intr_status1);
		writel(val, priv->ctrl + priv->data->intr_status1);
	}

	val = priv->data->intr_mask_val;
	/* enable INTX interrupt */
	writel(val, priv->ctrl + priv->data->intr_mask);
	return;
}

static irqreturn_t berlin_pcie_irq_handler(int irq, void *arg)
{
	u32 val;
	struct berlin_pcie *priv = arg;

	val = readl(priv->ctrl + priv->data->intr_status);
	writel(val, priv->ctrl + priv->data->intr_status);
	return IRQ_HANDLED;
}

static void berlin_pcie_set_gpios_value(struct gpio_descs *gpios, int value)
{
	if (!IS_ERR_OR_NULL(gpios)) {
		unsigned long *values;
		int nvalues = gpios->ndescs;

		values = bitmap_alloc(nvalues, GFP_KERNEL);
		if (!values)
			return;

		if (value)
			bitmap_fill(values, nvalues);
		else
			bitmap_zero(values, nvalues);

		gpiod_set_array_value_cansleep(nvalues, gpios->desc,
					       gpios->info, values);
		kfree(values);
	}
}

static void dw_pcie_link_set_max_speed(struct dw_pcie *pci, u32 link_gen)
{
	u32 cap, ctrl2, link_speed;
	u8 offset = dw_pcie_find_capability(pci, PCI_CAP_ID_EXP);

	cap = dw_pcie_readl_dbi(pci, offset + PCI_EXP_LNKCAP);
	ctrl2 = dw_pcie_readl_dbi(pci, offset + PCI_EXP_LNKCTL2);
	ctrl2 &= ~PCI_EXP_LNKCTL2_TLS;

	if (link_gen >= 1 && link_gen <= 5) {
		link_speed = link_gen;
	} else {
		/* Use hardware capability */
		link_speed = FIELD_GET(PCI_EXP_LNKCAP_SLS, cap);
		ctrl2 &= ~PCI_EXP_LNKCTL2_HASD;
	}

	dw_pcie_writel_dbi(pci, offset + PCI_EXP_LNKCTL2, ctrl2 | link_speed);

	cap &= ~((u32)PCI_EXP_LNKCAP_SLS);
	dw_pcie_writel_dbi(pci, offset + PCI_EXP_LNKCAP, cap | link_speed);
}

static int berlin_pcie_host_init(struct pcie_port *pp)
{
	u32 val;
	int ret;
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct berlin_pcie *priv = to_berlin_pcie(pci);

	/* reset RC */
	reset_control_assert(priv->rc_rst);
	udelay(100);

	/* disable link training */
	val = readl(priv->ctrl + priv->data->ctrl);
	val &= ~(1 << 5);
	writel(val, priv->ctrl + priv->data->ctrl);

	/* power up EP */
	berlin_pcie_set_gpios_value(priv->power_gpios, 1);
	msleep(10);

	gpiod_set_value_cansleep(priv->enable_gpio, 1);
	msleep(10);

	reset_control_deassert(priv->rc_rst);
	udelay(200);

	val = readl(priv->ctrl + priv->data->ctrl);
	val &= ~(1 << 21);
	writel(val, priv->ctrl + priv->data->ctrl);
	val = readl(priv->ctrl + priv->data->ctrl);

	readl(priv->ctrl + 0x2c);

	/* power on phy */
	ret = phy_init(priv->phy);
	if (ret)
		return ret;

	val = readl(priv->ctrl + 0x28);
	val &= ~(1 << 0);
	val |= (1 << 7);
	writel(val, priv->ctrl + 0x28);

	val = readl(priv->ctrl + 0x28);
	val &= ~(7 << 8);
	val |= (7 << 8);
	writel(val, priv->ctrl + 0x28);

	ret = phy_power_on(priv->phy);
	if (ret)
		return ret;

	/* reset EP */
	gpiod_set_value_cansleep(priv->reset_gpio, 1);
	msleep(100);
	gpiod_set_value_cansleep(priv->reset_gpio, 0);
	msleep(100);

	if (priv->link_gen > 0)
		dw_pcie_link_set_max_speed(pci, priv->link_gen);

	dw_pcie_setup_rc(pp);

	priv->pcie_cap_base = dw_pcie_find_capability(pci, PCI_CAP_ID_EXP);

	/* enable link training */
	val = readl(priv->ctrl + priv->data->ctrl);
	val |= (1 << 5);
	writel(val, priv->ctrl + priv->data->ctrl);

	/* wait for link up */
	ret = dw_pcie_wait_for_link(pci);
	if (ret)
		return ret;

	berlin_pcie_enable_irq_pulse(priv);
	if (IS_ENABLED(CONFIG_PCI_MSI))
		dw_pcie_msi_init(pp);

	return 0;
}

static int vs680_pcie_host_init(struct pcie_port *pp)
{
	u32 val;
	int ret;
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct berlin_pcie *priv = to_berlin_pcie(pci);

	/* reset RC */
	reset_control_assert(priv->rc_rst);
	udelay(100);
	reset_control_deassert(priv->rc_rst);

	/* disable link training */
	val = readl(priv->ctrl + priv->data->ctrl);
	val &= ~(1 << 5);
	writel(val, priv->ctrl + priv->data->ctrl);

	/* power up EP */
	berlin_pcie_set_gpios_value(priv->power_gpios, 0);
	msleep(100);
	berlin_pcie_set_gpios_value(priv->power_gpios, 1);

	msleep(10);
	gpiod_set_value_cansleep(priv->enable_gpio, 1);

	/* app_dbi_ro_wr_disable pin */
	val = readl(priv->ctrl + priv->data->ctrl);
	val &= ~(1 << 21);
	writel(val, priv->ctrl + priv->data->ctrl);

	ret = phy_init(priv->phy);
	if (ret)
		return ret;

	ret = phy_power_on(priv->phy);
	if (ret)
		return ret;

	/* reset EP */
	gpiod_set_value_cansleep(priv->reset_gpio, 1);
	msleep(100);
	gpiod_set_value_cansleep(priv->reset_gpio, 0);
	msleep(100);

	val = dw_pcie_readl_dbi(pci, SYMBOL_TIMER_FILTER_1);
	val &= ~MASK_RADM_1;
	dw_pcie_writel_dbi(pci, SYMBOL_TIMER_FILTER_1, val);

	dw_pcie_setup_rc(pp);

	priv->pcie_cap_base = dw_pcie_find_capability(pci, PCI_CAP_ID_EXP);

	/* enable link training */
	val = readl(priv->ctrl + priv->data->ctrl);
	val |= (1 << 5);
	writel(val, priv->ctrl + priv->data->ctrl);

	/* wait for link up */
	dw_pcie_wait_for_link(pci);

	berlin_pcie_enable_irq_pulse(priv);
	if (IS_ENABLED(CONFIG_PCI_MSI))
		dw_pcie_msi_init(pp);

	return 0;
}

static struct dw_pcie_host_ops berlin_pcie_host_ops;

static int berlin_add_pcie_port(struct berlin_pcie *priv,
				struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct pcie_port *pp = &priv->pci.pp;

	pp->irq = platform_get_irq(pdev, 0);
	if (pp->irq < 0)
		return pp->irq;

	ret = devm_request_irq(dev, pp->irq, berlin_pcie_irq_handler,
			       IRQF_SHARED | IRQF_NO_THREAD,
			       "berlin-pcie", priv);
	if (ret) {
		dev_err(dev, "failed to request irq\n");
		return ret;
	}

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		pp->msi_irq = platform_get_irq(pdev, 1);
		if (pp->msi_irq < 0)
			return pp->msi_irq;
	}

	pp->root_bus_nr = 0;
	berlin_pcie_host_ops.host_init = priv->data->host_init;
	pp->ops = &berlin_pcie_host_ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "failed to initialize host\n");
		return ret;
	}

	return 0;
}

static int berlin_pcie_link_up(struct dw_pcie *pci)
{
	struct berlin_pcie *priv = to_berlin_pcie(pci);
	u32 val = dw_pcie_readw_dbi(pci, priv->pcie_cap_base + PCI_EXP_LNKSTA);

	return !!(val & PCI_EXP_LNKSTA_DLLLA);
}

static const struct dw_pcie_ops dw_pcie_ops = {
	.link_up = berlin_pcie_link_up,
};

static const struct berlin_pcie_data berlin_pcie = {
	.intr_status = 0xC,
	.intr_mask = 0x10,
	.intr_mask_val = ~((1 << 17) | (1 << 18) | (1 << 19) | (1 << 20)),
	.ctrl = 0x14,
	.host_init = &berlin_pcie_host_init,
};

static const struct berlin_pcie_data as370_pcie = {
	.intr_status = 0xC,
	.intr_status1 = 0x10,
	.intr_mask = 0x14,
	.intr_mask1 = 0x18,
	.intr_mask_val = ~((1 << 19) | (1 << 20) | (1 << 21) | (1 << 22)),
	.ctrl = 0x1C,
	.host_init = &berlin_pcie_host_init,
};

static const struct berlin_pcie_data vs680_pcie = {
	.intr_status = 0xC,
	.intr_status1 = 0x14,
	.intr_mask = 0x10,
	.intr_mask1 = 0x18,
	.intr_mask_val = ~((1 << 19) | (1 << 20) | (1 << 21) | (1 << 22)),
	.ctrl = 0x1C,
	.host_init = &vs680_pcie_host_init,
};

static const struct of_device_id berlin_pcie_of_match[] = {
	{
		.compatible = "marvell,berlin-pcie",
		.data = &berlin_pcie
	},
	{
		.compatible = "syna,as370-pcie",
		.data = &as370_pcie
	},
	{
		.compatible = "syna,vs680-pcie",
		.data = &vs680_pcie
	},
	{},
};
MODULE_DEVICE_TABLE(of, berlin_pcie_of_match);

#ifdef CONFIG_PM_SLEEP
static void berlin_pcie_wait_l2(struct berlin_pcie *priv)
{
	int ret;
	u32 val;

	if (!dw_pcie_link_up(&priv->pci))
		return;

	val = readl(priv->ctrl + priv->data->ctrl);
	val |= APPS_PM_XMT_TURNOFF;
	writel(val, priv->ctrl + priv->data->ctrl);

	ret = readl_poll_timeout(priv->ctrl + priv->data->ctrl + 4, val,
				 (val & LTSSM_STATE_MASK) == LTSSM_STATE_L2_IDLE,
				 20, 500000);
	if (ret)
		dev_err(priv->pci.dev, "PCIe link enter L2 timeout: %x!\n", val);
}

static int berlin_pcie_suspend_noirq(struct device *dev)
{
	struct berlin_pcie *priv = dev_get_drvdata(dev);

	writel(0xffffffff, priv->ctrl + priv->data->intr_mask);
	if (priv->data->intr_mask1)
		writel(0xffffffff, priv->ctrl + priv->data->intr_mask1);

	berlin_pcie_wait_l2(priv);

	phy_power_off(priv->phy);
	phy_exit(priv->phy);
	reset_control_assert(priv->rc_rst);
	clk_disable_unprepare(priv->clk);
	gpiod_set_value_cansleep(priv->enable_gpio, 0);
	berlin_pcie_set_gpios_value(priv->power_gpios, 0);

	return 0;
}

static int berlin_pcie_resume_noirq(struct device *dev)
{
	struct berlin_pcie *priv = dev_get_drvdata(dev);
	struct pcie_port *pp = &priv->pci.pp;

	clk_prepare_enable(priv->clk);

	return priv->data->host_init(pp);
}

static const struct dev_pm_ops berlin_pcie_pmops = {
	.resume_noirq = berlin_pcie_resume_noirq,
	.suspend_noirq = berlin_pcie_suspend_noirq,
};
#endif

static int berlin_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct berlin_pcie *priv;
	struct dw_pcie *pci;
	struct pcie_port *pp;
	struct resource *res;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	pci = &priv->pci;
	pci->dev = dev;
	pci->ops = &dw_pcie_ops;
	pp = &pci->pp;
	priv->data = of_device_get_match_data(dev);

	priv->rc_rst = devm_reset_control_get_optional(dev, NULL);
	if (IS_ERR(priv->rc_rst))
		return PTR_ERR(priv->rc_rst);

	priv->reset_gpio = devm_gpiod_get_optional(dev, "reset",
							  GPIOD_OUT_LOW);
	if (IS_ERR(priv->reset_gpio))
		return PTR_ERR(priv->reset_gpio);

	priv->power_gpios = devm_gpiod_get_array_optional(dev, "power",
							  GPIOD_OUT_LOW);
	if (IS_ERR(priv->power_gpios))
		return PTR_ERR(priv->power_gpios);

	priv->enable_gpio = devm_gpiod_get_optional(dev, "enable",
							   GPIOD_OUT_LOW);
	if (IS_ERR(priv->enable_gpio))
		return PTR_ERR(priv->enable_gpio);

	priv->phy = devm_phy_get(dev, "pcie-phy");
	if (IS_ERR(priv->phy))
		return PTR_ERR(priv->phy);

	priv->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		dev_err(dev, "Failed to get pcie rc clock\n");
		return PTR_ERR(priv->clk);
	}
	ret = clk_prepare_enable(priv->clk);
	if (ret)
		return ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
	pci->dbi_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(pci->dbi_base)) {
		ret = PTR_ERR(pci->dbi_base);
		goto fail_clk;
	}

	pci->atu_base = pci->dbi_base + 0x1000;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ctrl");
	priv->ctrl = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->ctrl)) {
		ret = PTR_ERR(priv->ctrl);
		goto fail_clk;
	}

	platform_set_drvdata(pdev, priv);

	priv->link_gen = of_pci_get_max_link_speed(dev->of_node);

	ret = berlin_add_pcie_port(priv, pdev);
	if (ret < 0)
		goto fail_clk;

	return 0;

fail_clk:
	clk_disable_unprepare(priv->clk);
	return ret;
}

static struct platform_driver berlin_pcie_driver = {
	.probe = berlin_pcie_probe,
	.driver = {
		.name = "berlin-pcie",
		.suppress_bind_attrs = true,
		.of_match_table = berlin_pcie_of_match,
#ifdef CONFIG_PM_SLEEP
		.pm = &berlin_pcie_pmops,
#endif
	},
};
builtin_platform_driver(berlin_pcie_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_DESCRIPTION("Synaptics PCIe host controller driver");
MODULE_LICENSE("GPL v2");
