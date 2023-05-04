/*
 * Copyright (C) 2014 Marvell Technology Group Ltd.
 *
 * Antoine Tenart <antoine.tenart@free-electrons.com>
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/types.h>

#define BERLIN_MAX_RESETS	32

#define to_berlin_reset_priv(p)		\
	container_of((p), struct berlin_reset_priv, rcdev)

struct berlin_reset_priv {
	spinlock_t			lock;
	void __iomem			*base;
	unsigned int			size;
	struct reset_controller_dev	rcdev;
};

static int berlin_reset_reset(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	int offset = id >> 8;
	int mask = BIT(id & 0x7f);
	int sticky = (id & 0x80);

	if (sticky)
		return -EINVAL;

	writel(mask, priv->base + offset);

	/* let the reset be effective */
	udelay(10);

	return 0;
}

static int berlin_reset_assert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	u32 reg;
	unsigned long flags;
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	int offset = id >> 8;
	int mask = BIT(id & 0x3f);
	int sticky = (id & 0x80);
	int inverted = (id & 0x40);

	if (!sticky)
		return -EINVAL;

	spin_lock_irqsave(&priv->lock, flags);
	reg = readl(priv->base + offset);
	if (inverted)
		reg |= mask;
	else
		reg &= ~mask;
	writel(reg, priv->base + offset);
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static int berlin_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	u32 reg;
	unsigned long flags;
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	int offset = id >> 8;
	int mask = BIT(id & 0x3f);
	int sticky = (id & 0x80);
	int inverted = (id & 0x40);

	if (!sticky)
		return -EINVAL;

	spin_lock_irqsave(&priv->lock, flags);
	reg = readl(priv->base + offset);
	if (inverted)
		reg &= ~mask;
	else
		reg |= mask;
	writel(reg, priv->base + offset);
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static int berlin_reset_status(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	u32 reg;
	unsigned long flags;
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	int offset = id >> 8;
	int mask = BIT(id & 0x3f);
	int sticky = (id & 0x80);
	int inverted = (id & 0x40);

	if (!sticky)
		return -EINVAL;

	spin_lock_irqsave(&priv->lock, flags);
	reg = readl(priv->base + offset);
	spin_unlock_irqrestore(&priv->lock, flags);

	if (inverted)
		return !!(reg & mask);
	else
		return !(reg & mask);
}

static const struct reset_control_ops berlin_reset_ops = {
	.reset		= berlin_reset_reset,
	.assert		= berlin_reset_assert,
	.deassert	= berlin_reset_deassert,
	.status		= berlin_reset_status,
};

static int berlin_reset_xlate(struct reset_controller_dev *rcdev,
			      const struct of_phandle_args *reset_spec)
{
	struct berlin_reset_priv *priv = to_berlin_reset_priv(rcdev);
	unsigned offset, bit, sticky, inverted;

	offset = reset_spec->args[0];
	bit = reset_spec->args[1];
	sticky = reset_spec->args[2];
	inverted = reset_spec->args[3];

	if (offset >= priv->size)
		return -EINVAL;

	if (bit >= BERLIN_MAX_RESETS)
		return -EINVAL;

	return (offset << 8) | (sticky << 7) | (inverted << 6) | bit;
}

static int __berlin_reset_init(struct device_node *np)
{
	struct berlin_reset_priv *priv;
	struct resource res;
	resource_size_t size;
	int ret;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	ret = of_address_to_resource(np, 0, &res);
	if (ret)
		goto err;

	size = resource_size(&res);
	priv->base = ioremap(res.start, size);
	if (!priv->base) {
		ret = -ENOMEM;
		goto err;
	}
	priv->size = size;

	spin_lock_init(&priv->lock);

	priv->rcdev.owner = THIS_MODULE;
	priv->rcdev.ops = &berlin_reset_ops;
	priv->rcdev.of_node = np;
	priv->rcdev.of_reset_n_cells = 4;
	priv->rcdev.of_xlate = berlin_reset_xlate;

	return reset_controller_register(&priv->rcdev);

err:
	kfree(priv);
	return ret;
}

static const struct of_device_id berlin_reset_of_match[] __initconst = {
	{ .compatible = "marvell,berlin2-chip-ctrl" },
	{ .compatible = "marvell,berlin2cd-chip-ctrl" },
	{ .compatible = "marvell,berlin2q-chip-ctrl" },
	{ .compatible = "marvell,berlin4ct-chip-ctrl" },
	{ .compatible = "marvell,berlin4cdp-chip-ctrl" },
	{ },
};
MODULE_DEVICE_TABLE(of, berlin_reset_of_match);

static int __init berlin_reset_init(void)
{
	struct device_node *np;
	int ret;

	for_each_matching_node(np, berlin_reset_of_match) {
		ret = __berlin_reset_init(np);
		if (ret)
			return ret;
	}

	return 0;
}
arch_initcall(berlin_reset_init);
MODULE_DESCRIPTION("Synaptics Berlin reset driver");
MODULE_LICENSE("GPL v2");
