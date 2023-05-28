// SPDX-License-Identifier: GPL-2.0
/*
 * dwc3-rockchip-inno.c - DWC3 glue layer for Rockchip devices with Innosilicon based PHY
 *
 * Based on dwc3-of-simple.c
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>

#include <linux/workqueue.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/usb/phy.h>

#include "core.h"
#include "../host/xhci.h"


struct dwc3_rk_inno {
	struct device		*dev;
	struct clk_bulk_data	*clks;
	struct dwc3		*dwc;
	struct usb_phy		*phy;
	struct notifier_block	reset_nb;
	struct work_struct	reset_work;
	struct mutex		lock;
	int			num_clocks;
	struct reset_control	*resets;
};

static int dwc3_rk_inno_host_reset_notifier(struct notifier_block *nb, unsigned long event, void *data)
{
	struct dwc3_rk_inno	*rk_inno = container_of(nb, struct dwc3_rk_inno, reset_nb);

	schedule_work(&rk_inno->reset_work);

	return NOTIFY_DONE;
}

static void dwc3_rk_inno_host_reset_work(struct work_struct *work)
{
	struct dwc3_rk_inno	*rk_inno = container_of(work, struct dwc3_rk_inno, reset_work);
	struct usb_hcd		*hcd = dev_get_drvdata(&rk_inno->dwc->xhci->dev);
	struct usb_hcd		*shared_hcd = hcd->shared_hcd;
	struct xhci_hcd		*xhci = hcd_to_xhci(hcd);
	unsigned int		count = 0;

	mutex_lock(&rk_inno->lock);

	if (hcd->state != HC_STATE_HALT) {
		usb_remove_hcd(shared_hcd);
		usb_remove_hcd(hcd);
	}

	if (rk_inno->phy)
		usb_phy_shutdown(rk_inno->phy);

	while (hcd->state != HC_STATE_HALT) {
		if (++count > 1000) {
			dev_err(rk_inno->dev, "wait for HCD remove 1s timeout!\n");
			break;
		}
		usleep_range(1000, 1100);
	}

	if (hcd->state == HC_STATE_HALT) {
		xhci->shared_hcd = shared_hcd;
		usb_add_hcd(hcd, hcd->irq, IRQF_SHARED);
		usb_add_hcd(shared_hcd, hcd->irq, IRQF_SHARED);
	}

	if (rk_inno->phy)
		usb_phy_init(rk_inno->phy);

	mutex_unlock(&rk_inno->lock);
	dev_dbg(rk_inno->dev, "host reset complete\n");
}

static int dwc3_rk_inno_probe(struct platform_device *pdev)
{
	struct dwc3_rk_inno	*rk_inno;
	struct device		*dev = &pdev->dev;
	struct device_node	*np = dev->of_node, *child, *node;
	struct platform_device	*child_pdev;

	int			ret;

	rk_inno = devm_kzalloc(dev, sizeof(*rk_inno), GFP_KERNEL);
	if (!rk_inno)
		return -ENOMEM;

	platform_set_drvdata(pdev, rk_inno);
	rk_inno->dev = dev;

	rk_inno->resets = of_reset_control_array_get(np, false, true,
						    true);
	if (IS_ERR(rk_inno->resets)) {
		ret = PTR_ERR(rk_inno->resets);
		dev_err(dev, "failed to get device resets, err=%d\n", ret);
		return ret;
	}

	ret = reset_control_deassert(rk_inno->resets);
	if (ret)
		goto err_resetc_put;

	ret = clk_bulk_get_all(rk_inno->dev, &rk_inno->clks);
	if (ret < 0)
		goto err_resetc_assert;

	rk_inno->num_clocks = ret;
	ret = clk_bulk_prepare_enable(rk_inno->num_clocks, rk_inno->clks);
	if (ret)
		goto err_resetc_assert;

	ret = of_platform_populate(np, NULL, NULL, dev);
	if (ret)
		goto err_clk_put;

	child = of_get_child_by_name(np, "dwc3");
	if (!child) {
		dev_err(dev, "failed to find dwc3 core node\n");
		ret = -ENODEV;
		goto err_plat_depopulate;
	}

	child_pdev = of_find_device_by_node(child);
	if (!child_pdev) {
		dev_err(dev, "failed to get dwc3 core device\n");
		ret = -ENODEV;
		goto err_plat_depopulate;
	}

	rk_inno->dwc = platform_get_drvdata(child_pdev);
	if (!rk_inno->dwc || !rk_inno->dwc->xhci) {
		ret = -EPROBE_DEFER;
		goto err_plat_depopulate;
	}

	node = of_parse_phandle(child, "usb-phy", 0);
	INIT_WORK(&rk_inno->reset_work, dwc3_rk_inno_host_reset_work);
	rk_inno->reset_nb.notifier_call = dwc3_rk_inno_host_reset_notifier;
	rk_inno->phy = devm_usb_get_phy_by_node(dev, node, &rk_inno->reset_nb);
	of_node_put(node);
	mutex_init(&rk_inno->lock);

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	return 0;

err_plat_depopulate:
	of_platform_depopulate(dev);

err_clk_put:
	clk_bulk_disable_unprepare(rk_inno->num_clocks, rk_inno->clks);
	clk_bulk_put_all(rk_inno->num_clocks, rk_inno->clks);

err_resetc_assert:
	reset_control_assert(rk_inno->resets);

err_resetc_put:
	reset_control_put(rk_inno->resets);
	return ret;
}

static void __dwc3_rk_inno_teardown(struct dwc3_rk_inno *rk_inno)
{
	of_platform_depopulate(rk_inno->dev);

	clk_bulk_disable_unprepare(rk_inno->num_clocks, rk_inno->clks);
	clk_bulk_put_all(rk_inno->num_clocks, rk_inno->clks);
	rk_inno->num_clocks = 0;

	reset_control_assert(rk_inno->resets);

	reset_control_put(rk_inno->resets);

	pm_runtime_disable(rk_inno->dev);
	pm_runtime_put_noidle(rk_inno->dev);
	pm_runtime_set_suspended(rk_inno->dev);
}

static int dwc3_rk_inno_remove(struct platform_device *pdev)
{
	struct dwc3_rk_inno	*rk_inno = platform_get_drvdata(pdev);

	__dwc3_rk_inno_teardown(rk_inno);

	return 0;
}

static void dwc3_rk_inno_shutdown(struct platform_device *pdev)
{
	struct dwc3_rk_inno	*rk_inno = platform_get_drvdata(pdev);

	__dwc3_rk_inno_teardown(rk_inno);
}

static int __maybe_unused dwc3_rk_inno_runtime_suspend(struct device *dev)
{
	struct dwc3_rk_inno	*rk_inno = dev_get_drvdata(dev);

	clk_bulk_disable(rk_inno->num_clocks, rk_inno->clks);

	return 0;
}

static int __maybe_unused dwc3_rk_inno_runtime_resume(struct device *dev)
{
	struct dwc3_rk_inno	*rk_inno = dev_get_drvdata(dev);

	return clk_bulk_enable(rk_inno->num_clocks, rk_inno->clks);
}

static int __maybe_unused dwc3_rk_inno_suspend(struct device *dev)
{
	struct dwc3_rk_inno *rk_inno = dev_get_drvdata(dev);

	reset_control_assert(rk_inno->resets);

	return 0;
}

static int __maybe_unused dwc3_rk_inno_resume(struct device *dev)
{
	struct dwc3_rk_inno *rk_inno = dev_get_drvdata(dev);

	reset_control_deassert(rk_inno->resets);

	return 0;
}

static const struct dev_pm_ops dwc3_rk_inno_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc3_rk_inno_suspend, dwc3_rk_inno_resume)
	SET_RUNTIME_PM_OPS(dwc3_rk_inno_runtime_suspend,
			dwc3_rk_inno_runtime_resume, NULL)
};

static const struct of_device_id of_dwc3_rk_inno_match[] = {
	{ .compatible = "rockchip,rk3328-dwc3" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_dwc3_rk_inno_match);

static struct platform_driver dwc3_rk_inno_driver = {
	.probe		= dwc3_rk_inno_probe,
	.remove		= dwc3_rk_inno_remove,
	.shutdown	= dwc3_rk_inno_shutdown,
	.driver		= {
		.name	= "dwc3-rk-inno",
		.of_match_table = of_dwc3_rk_inno_match,
		.pm	= &dwc3_rk_inno_dev_pm_ops,
	},
};

module_platform_driver(dwc3_rk_inno_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 Rockchip Innosilicon Glue Layer");
MODULE_AUTHOR("Peter Geis <pgwipeout@gmail.com>");
