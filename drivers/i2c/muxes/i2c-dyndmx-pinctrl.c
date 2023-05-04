// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define MAX_CHAN	2

struct i2c_dyndmx_chan {
	struct i2c_adapter adap;
	struct i2c_adapter *parent_adap;
	struct i2c_dyndmx_pinctrl_priv *priv;
	struct pinctrl_state *state;
	u32 chan_id;
};

struct i2c_dyndmx_pinctrl_priv {
	struct device *dev;
	struct rt_mutex dmx_lock;
	struct pinctrl *pinctrl;
	struct i2c_algorithm algo;
	struct i2c_dyndmx_chan chan[MAX_CHAN];
};

static int i2c_dyndmx_master_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct i2c_dyndmx_chan *chan = adap->algo_data;
	struct i2c_adapter *parent = chan->parent_adap;
	int ret;

	ret = pinctrl_select_state(chan->priv->pinctrl, chan->state);
	if (ret >= 0)
		ret = i2c_transfer(parent, msgs, num);
	return ret;
}

static u32 i2c_dyndmx_functionality(struct i2c_adapter *adap)
{
	struct i2c_dyndmx_chan *chan = adap->algo_data;
	struct i2c_adapter *parent = chan->parent_adap;

	return parent->algo->functionality(parent);
}

static void i2c_dyndmx_lock_bus(struct i2c_adapter *adap,
				unsigned int flags)
{
	struct i2c_dyndmx_chan *chan = adap->algo_data;
	rt_mutex_lock_nested(&chan->priv->dmx_lock, 1);
}

static int i2c_dyndmx_trylock_bus(struct i2c_adapter *adap,
				  unsigned int flags)
{
	struct i2c_dyndmx_chan *chan = adap->algo_data;
	return rt_mutex_trylock(&chan->priv->dmx_lock);
}

static void i2c_dyndmx_unlock_bus(struct i2c_adapter *adap,
				  unsigned int flags)
{
	struct i2c_dyndmx_chan *chan = adap->algo_data;
	rt_mutex_unlock(&chan->priv->dmx_lock);
}

static const struct i2c_lock_operations i2c_dyndmx_lock_ops = {
	.lock_bus = i2c_dyndmx_lock_bus,
	.trylock_bus = i2c_dyndmx_trylock_bus,
	.unlock_bus = i2c_dyndmx_unlock_bus,
};

static int i2c_dyndmx_setup_chan(struct i2c_dyndmx_pinctrl_priv *priv, int i)
{
	struct i2c_dyndmx_chan *chan = &priv->chan[i];
	struct device_node *np = priv->dev->of_node;
	struct device_node *parent_np, *child = NULL;
	struct i2c_adapter *parent;
	const char *name;
	int ret;
	u32 reg;

	chan->priv = priv;
	chan->chan_id = i;

	ret = of_property_read_string_index(np, "pinctrl-names", i,
					    &name);
	if (ret < 0) {
		dev_err(priv->dev, "Cannot parse pinctrl-names: %d\n", ret);
		return ret;
	}

	chan->state = pinctrl_lookup_state(priv->pinctrl, name);
	if (IS_ERR(chan->state)) {
		ret = PTR_ERR(chan->state);
		dev_err(priv->dev, "Cannot look up pinctrl state %s: %d\n",
			name, ret);
		return ret;
	}

	parent_np = of_parse_phandle(np, "i2c-parent", i);
	if (!parent_np) {
		dev_err(priv->dev, "can't get phandle for parent %d\n", i);
		return -ENODEV;
	}
	parent = of_find_i2c_adapter_by_node(parent_np);
	of_node_put(parent_np);
	if (!parent) {
		dev_err(priv->dev, "Cannot find parent bus %d\n", i);
		return -EPROBE_DEFER;
	}

	snprintf(chan->adap.name, sizeof(chan->adap.name),
		 "i2c-%d-dyndmx (chan_id %d)", i2c_adapter_id(parent), i);
	chan->parent_adap = parent;
	chan->adap.owner = THIS_MODULE;
	chan->adap.algo = &priv->algo;
	chan->adap.algo_data = chan;
	chan->adap.dev.parent = &parent->dev;
	chan->adap.class = parent->class;
	chan->adap.retries = parent->retries;
	chan->adap.timeout = parent->timeout;
	chan->adap.quirks = parent->quirks;
	chan->adap.lock_ops = &i2c_dyndmx_lock_ops;
	for_each_child_of_node(np, child) {
		ret = of_property_read_u32(child, "reg", &reg);
		if (ret)
			continue;
		if (i == reg)
			break;
	}
	chan->adap.dev.of_node = child;

	ret = i2c_add_adapter(&chan->adap);
	if (ret < 0)
		goto err_with_put;

	return 0;

err_with_put:
	i2c_put_adapter(parent);
	dev_err(priv->dev, "failed to setup dyndmx-adapter %d (%d)\n", i, ret);
	return ret;
}

static int i2c_dyndmx_pinctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct i2c_dyndmx_pinctrl_priv *priv;
	int i, j, ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	priv->algo.master_xfer = i2c_dyndmx_master_xfer;
	priv->algo.functionality = i2c_dyndmx_functionality;
	rt_mutex_init(&priv->dmx_lock);

	priv->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(priv->pinctrl))
		return PTR_ERR(priv->pinctrl);

	for (i = 0; i < MAX_CHAN; i++) {
		ret = i2c_dyndmx_setup_chan(priv, i);
		if (ret)
			goto err_rollback;
	}

	platform_set_drvdata(pdev, priv);

	return 0;

err_rollback:
	for (j = 0; j < i; j++) {
		i2c_del_adapter(&priv->chan[j].adap);
		of_node_put(priv->chan[j].adap.dev.of_node);
		i2c_put_adapter(priv->chan[j].parent_adap);
	}

	return ret;
}

static int i2c_dyndmx_pinctrl_remove(struct platform_device *pdev)
{
	struct i2c_dyndmx_pinctrl_priv *priv = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < MAX_CHAN; i++) {
		i2c_del_adapter(&priv->chan[i].adap);
		of_node_put(priv->chan[i].adap.dev.of_node);
		i2c_put_adapter(priv->chan[i].parent_adap);
	}

	return 0;
}

static const struct of_device_id i2c_dyndmx_pinctrl_of_match[] = {
	{ .compatible = "i2c-dyndmx-pinctrl", },
	{},
};
MODULE_DEVICE_TABLE(of, i2c_dyndmx_pinctrl_of_match);

static struct platform_driver i2c_dyndmx_pinctrl_driver = {
	.driver	= {
		.name = "i2c-dyndmx-pinctrl",
		.of_match_table = i2c_dyndmx_pinctrl_of_match,
	},
	.probe	= i2c_dyndmx_pinctrl_probe,
	.remove	= i2c_dyndmx_pinctrl_remove,
};

static int __init i2c_dyndmx_pinctrl_init(void)
{
	return platform_driver_register(&i2c_dyndmx_pinctrl_driver);
}
subsys_initcall(i2c_dyndmx_pinctrl_init);

static void __exit i2c_dyndmx_pinctrl_exit(void)
{
	platform_driver_unregister(&i2c_dyndmx_pinctrl_driver);
}
module_exit(i2c_dyndmx_pinctrl_exit);

MODULE_DESCRIPTION("pinctrl-based I2C dyndmx driver");
MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_LICENSE("GPL v2");
