// SPDX-License-Identifier: GPL-2.0
/*
 *  Synaptics BERLIN CHIP ID support
 *
 *  Author:	Jisheng Zhang <jszhang@marvell.com>
 *  Copyright (C) 2018 Synaptics Incorporated
 *  Copyright (c) 2014 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>

u32 chip_revision __read_mostly;

static const struct of_device_id chipid_of_match[] __initconst = {
	{ .compatible = "marvell,berlin-chipid", },
	{},
};

static const char * __init berlin_id_to_family(u32 id)
{
	const char *soc_family;

	switch (id) {
	case 0x370:
		soc_family = "Synaptics AS370";
		break;
	case 0x390:
		soc_family = "Synaptics AS390";
		break;
	case 0x470:
		soc_family = "Synaptics AS470";
		break;
	case 0x640:
		soc_family = "Synaptics VS640";
		break;
	case 0x680:
		soc_family = "Synaptics VS680";
		break;
	case 0x3005:
		soc_family = "Marvell Berlin BG2CD";
		break;
	case 0x3006:
		soc_family = "Marvell Berlin BG2CDP";
		break;
	case 0x3012:
		soc_family = "Marvell Berlin BG4DTV";
		break;
	case 0x3017:
		soc_family = "Marvell Berlin BG4CDP";
		break;
	case 0x3100:
		soc_family = "Marvell Berlin BG2";
		break;
	case 0x3108:
		soc_family = "Marvell Berlin BG2CT";
		break;
	case 0x3114:
		soc_family = "Marvell Berlin BG2Q";
		break;
	case 0x3215:
		soc_family = "Marvell Berlin BG2Q4K";
		break;
	case 0x3218:
		soc_family = "Marvell Berlin BG4CT";
		break;
	case 0x3288:
		soc_family = "Marvell Berlin BG5CT";
		break;
	case 0x4006:
		soc_family = "Marvell Berlin BG4CD";
		break;
	default:
		soc_family = "<unknown>";
		break;
	}
	return soc_family;
}

static void __init rev_fixup(u32 id, u32 *rev)
{
	if (id == 0x680) {
		if (*rev == 0xa0)
			*rev = 0;
		if (*rev == 0xb0)
			*rev = 0xa0;
		if (*rev == 0xb1)
			*rev = 0xa1;
	}
}

static int __init berlin_chipid_init(void)
{
	u32 val, rev;
	unsigned long dt_root;
	struct soc_device_attribute *soc_dev_attr;
	struct soc_device *soc_dev;
	struct device_node *np;
	int ret = 0;
	void __iomem *id_base = NULL;

	np = of_find_matching_node(NULL, chipid_of_match);
	if (!np)
		return -ENODEV;

	id_base = of_iomap(np, 0);
	if (!id_base) {
		ret = -ENOMEM;
		goto out_put_node;
	}

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr) {
		ret = -ENOMEM;
		goto out_unmap;
	}

	dt_root = of_get_flat_dt_root();
	soc_dev_attr->machine = of_get_flat_dt_prop(dt_root, "model", NULL);
	if (!soc_dev_attr->machine)
		soc_dev_attr->machine = "<unknown>";

	val = readl_relaxed(id_base);
	val = (val >> 12) & 0xffff;
	soc_dev_attr->family = berlin_id_to_family(val);

	ret = of_property_read_u32(np, "chip-revision", &rev);
	if (ret)
		rev = readl_relaxed(id_base + 4);
	rev_fixup(val, &rev);
	chip_revision = rev;
	soc_dev_attr->revision = kasprintf(GFP_KERNEL, "%X", rev);

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev)) {
		kfree(soc_dev_attr);
		ret = PTR_ERR(soc_dev);
	}

out_unmap:
	iounmap(id_base);
out_put_node:
	of_node_put(np);
	return ret;
}
arch_initcall(berlin_chipid_init);
