// SPDX-License-Identifier: GPL-2.0-only
/*
 * Rockchip CPUFreq Driver
 *
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Copyright (C) 2022 Collabora Ltd.
 */

#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>

#include "cpufreq-dt.h"

#define RK3588_MEMCFG_HSSPRF_LOW	0x20
#define RK3588_MEMCFG_HSDPRF_LOW	0x28
#define RK3588_MEMCFG_HSDPRF_HIGH	0x2c
#define RK3588_CPU_CTRL			0x30

#define VOLT_RM_TABLE_END		~1

struct volt_rm_table {
	uint32_t volt;
	uint32_t rm;
};

struct rockchip_opp_info {
	const struct rockchip_opp_data *data;
	struct volt_rm_table *volt_rm_tbl;
	struct regmap *grf;
	u32 current_rm;
	u32 reboot_freq;
};

struct rockchip_opp_data {
	int (*set_read_margin)(struct device *dev, struct rockchip_opp_info *opp_info,
			       unsigned long volt);
};

struct cluster_info {
	struct list_head list_head;
	struct rockchip_opp_info opp_info;
	cpumask_t cpus;
};
static LIST_HEAD(cluster_info_list);

static int rk3588_cpu_set_read_margin(struct device *dev, struct rockchip_opp_info *opp_info,
				      unsigned long volt)
{
	bool is_found = false;
	u32 rm;
	int i;

	if (!opp_info->volt_rm_tbl)
		return 0;

	for (i = 0; opp_info->volt_rm_tbl[i].rm != VOLT_RM_TABLE_END; i++) {
		if (volt >= opp_info->volt_rm_tbl[i].volt) {
			rm = opp_info->volt_rm_tbl[i].rm;
			is_found = true;
			break;
		}
	}

	if (!is_found)
		return 0;
	if (rm == opp_info->current_rm)
		return 0;
	if (!opp_info->grf)
		return 0;

	dev_dbg(dev, "set rm to %d\n", rm);
	regmap_write(opp_info->grf, RK3588_MEMCFG_HSSPRF_LOW, 0x001c0000 | (rm << 2));
	regmap_write(opp_info->grf, RK3588_MEMCFG_HSDPRF_LOW, 0x003c0000 | (rm << 2));
	regmap_write(opp_info->grf, RK3588_MEMCFG_HSDPRF_HIGH, 0x003c0000 | (rm << 2));
	regmap_write(opp_info->grf, RK3588_CPU_CTRL, 0x00200020);
	udelay(1);
	regmap_write(opp_info->grf, RK3588_CPU_CTRL, 0x00200000);

	opp_info->current_rm = rm;

	return 0;
}

static const struct rockchip_opp_data rk3588_cpu_opp_data = {
	.set_read_margin = rk3588_cpu_set_read_margin,
};

static const struct of_device_id rockchip_cpufreq_of_match[] = {
	{
		.compatible = "rockchip,rk3588",
		.data = (void *)&rk3588_cpu_opp_data,
	},
	{},
};

static struct cluster_info *rockchip_cluster_info_lookup(int cpu)
{
	struct cluster_info *cluster;

	list_for_each_entry(cluster, &cluster_info_list, list_head) {
		if (cpumask_test_cpu(cpu, &cluster->cpus))
			return cluster;
	}

	return NULL;
}

static int rockchip_cpufreq_set_volt(struct device *dev,
				     struct regulator *reg,
				     struct dev_pm_opp_supply *supply)
{
	int ret;

	ret = regulator_set_voltage_triplet(reg, supply->u_volt_min,
					    supply->u_volt, supply->u_volt_max);
	if (ret)
		dev_err(dev, "%s: failed to set voltage (%lu %lu %lu uV): %d\n",
			__func__, supply->u_volt_min, supply->u_volt,
			supply->u_volt_max, ret);

	return ret;
}

static int rockchip_cpufreq_set_read_margin(struct device *dev,
					    struct rockchip_opp_info *opp_info,
					    unsigned long volt)
{
	if (opp_info->data && opp_info->data->set_read_margin) {
		opp_info->data->set_read_margin(dev, opp_info, volt);
	}

	return 0;
}

static int rk_opp_config_regulators(struct device *dev,
				    struct dev_pm_opp *old_opp, struct dev_pm_opp *new_opp,
				    struct regulator **regulators, unsigned int count)
{
	struct dev_pm_opp_supply old_supplies[2];
	struct dev_pm_opp_supply new_supplies[2];
	struct regulator *vdd_reg = regulators[0];
	struct regulator *mem_reg = regulators[1];
	struct rockchip_opp_info *opp_info;
	struct cluster_info *cluster;
	int ret = 0;
	unsigned long old_freq = dev_pm_opp_get_freq(old_opp);
	unsigned long new_freq = dev_pm_opp_get_freq(new_opp);

	/* We must have two regulators here */
	WARN_ON(count != 2);

	ret = dev_pm_opp_get_supplies(old_opp, old_supplies);
	if (ret)
		return ret;

	ret = dev_pm_opp_get_supplies(new_opp, new_supplies);
	if (ret)
		return ret;

	cluster = rockchip_cluster_info_lookup(dev->id);
	if (!cluster)
		return -EINVAL;
	opp_info = &cluster->opp_info;

	if (new_freq >= old_freq) {
		ret = rockchip_cpufreq_set_volt(dev, mem_reg, &new_supplies[1]);
		if (ret)
			goto error;
		ret = rockchip_cpufreq_set_volt(dev, vdd_reg, &new_supplies[0]);
		if (ret)
			goto error;
		rockchip_cpufreq_set_read_margin(dev, opp_info, new_supplies[0].u_volt);
	} else {
		rockchip_cpufreq_set_read_margin(dev, opp_info, new_supplies[0].u_volt);
		ret = rockchip_cpufreq_set_volt(dev, vdd_reg, &new_supplies[0]);
		if (ret)
			goto error;
		ret = rockchip_cpufreq_set_volt(dev, mem_reg, &new_supplies[1]);
		if (ret)
			goto error;
	}

	return 0;

error:
	rockchip_cpufreq_set_read_margin(dev, opp_info, old_supplies[0].u_volt);
	rockchip_cpufreq_set_volt(dev, mem_reg, &old_supplies[1]);
	rockchip_cpufreq_set_volt(dev, vdd_reg, &old_supplies[0]);
	return ret;
}

static void rockchip_get_opp_data(const struct of_device_id *matches,
				  struct rockchip_opp_info *info)
{
	const struct of_device_id *match;
	struct device_node *node;

	node = of_find_node_by_path("/");
	match = of_match_node(matches, node);
	if (match && match->data)
		info->data = match->data;
	of_node_put(node);
}

static int rockchip_get_volt_rm_table(struct device *dev, struct device_node *np,
				      char *porp_name, struct volt_rm_table **table)
{
	struct volt_rm_table *rm_table;
	const struct property *prop;
	int count, i;

	prop = of_find_property(np, porp_name, NULL);
	if (!prop)
		return -EINVAL;

	if (!prop->value)
		return -ENODATA;

	count = of_property_count_u32_elems(np, porp_name);
	if (count < 0)
		return -EINVAL;

	if (count % 2)
		return -EINVAL;

	rm_table = devm_kzalloc(dev, sizeof(*rm_table) * (count / 2 + 1),
				GFP_KERNEL);
	if (!rm_table)
		return -ENOMEM;

	for (i = 0; i < count / 2; i++) {
		of_property_read_u32_index(np, porp_name, 2 * i,
					   &rm_table[i].volt);
		of_property_read_u32_index(np, porp_name, 2 * i + 1,
					   &rm_table[i].rm);
	}

	rm_table[i].volt = 0;
	rm_table[i].rm = VOLT_RM_TABLE_END;

	*table = rm_table;

	return 0;
}

static int rockchip_cpufreq_reboot(struct notifier_block *notifier, unsigned long event, void *cmd)
{
	struct cluster_info *cluster;
	struct device *dev;
	int freq, ret, cpu;

	if (event != SYS_RESTART)
		return NOTIFY_DONE;

	for_each_possible_cpu(cpu) {
		cluster = rockchip_cluster_info_lookup(cpu);
		if (!cluster)
			continue;

		dev = get_cpu_device(cpu);
		if (!dev)
			continue;

		freq = cluster->opp_info.reboot_freq;

		if (freq) {
			ret = dev_pm_opp_set_rate(dev, freq);
			if (ret)
				dev_err(dev, "Failed setting reboot freq for cpu %d to %d: %d\n",
					cpu, freq, ret);
			dev_pm_opp_remove_table(dev);
		}
	}

	return NOTIFY_DONE;
}

static int rockchip_cpufreq_cluster_init(int cpu, struct cluster_info *cluster)
{
	struct rockchip_opp_info *opp_info = &cluster->opp_info;
	int reg_table_token = -EINVAL;
	int opp_table_token = -EINVAL;
	struct device_node *np;
	struct device *dev;
	const char * const reg_names[] = { "cpu", "mem", NULL };
	int ret = 0;

	dev = get_cpu_device(cpu);
	if (!dev)
		return -ENODEV;

	if (!of_find_property(dev->of_node, "cpu-supply", NULL))
		return -ENOENT;

	np = of_parse_phandle(dev->of_node, "operating-points-v2", 0);
	if (!np) {
		dev_warn(dev, "OPP-v2 not supported\n");
		return -ENOENT;
	}

	ret = dev_pm_opp_of_get_sharing_cpus(dev, &cluster->cpus);
	if (ret) {
		dev_err(dev, "Failed to get sharing cpus\n");
		goto np_err;
	}

	rockchip_get_opp_data(rockchip_cpufreq_of_match, opp_info);
	if (opp_info->data && opp_info->data->set_read_margin) {
		opp_info->current_rm = UINT_MAX;
		opp_info->grf = syscon_regmap_lookup_by_phandle(np, "rockchip,grf");
		if (IS_ERR(opp_info->grf))
			opp_info->grf = NULL;
		rockchip_get_volt_rm_table(dev, np, "rockchip,volt-mem-read-margin", &opp_info->volt_rm_tbl);

		of_property_read_u32(np, "rockchip,reboot-freq", &opp_info->reboot_freq);
	}

	if (of_find_property(dev->of_node, "cpu-supply", NULL) &&
	    of_find_property(dev->of_node, "mem-supply", NULL)) {
		reg_table_token = dev_pm_opp_set_regulators(dev, reg_names);
		if (reg_table_token < 0) {
			ret = reg_table_token;
			dev_err(dev, "Failed to set opp regulators\n");
			goto np_err;
		}
		opp_table_token = dev_pm_opp_set_config_regulators(dev, rk_opp_config_regulators);
		if (opp_table_token < 0) {
			ret = opp_table_token;
			dev_err(dev, "Failed to set opp config regulators\n");
			goto reg_opp_table;
		}
	}

	of_node_put(np);

	return 0;

reg_opp_table:
	if (reg_table_token >= 0)
		dev_pm_opp_put_regulators(reg_table_token);
np_err:
	of_node_put(np);

	return ret;
}

static struct notifier_block rockchip_cpufreq_reboot_notifier = {
	.notifier_call = rockchip_cpufreq_reboot,
	.priority = 0,
};

static int __init rockchip_cpufreq_driver_init(void)
{
	struct cluster_info *cluster, *pos;
	struct cpufreq_dt_platform_data pdata = {0};
	int cpu, ret;

	for_each_possible_cpu(cpu) {
		cluster = rockchip_cluster_info_lookup(cpu);
		if (cluster)
			continue;

		cluster = kzalloc(sizeof(*cluster), GFP_KERNEL);
		if (!cluster) {
			ret = -ENOMEM;
			goto release_cluster_info;
		}

		ret = rockchip_cpufreq_cluster_init(cpu, cluster);
		if (ret) {
			pr_err("Failed to initialize dvfs info cpu%d\n", cpu);
			goto release_cluster_info;
		}
		list_add(&cluster->list_head, &cluster_info_list);
	}

	ret = register_reboot_notifier(&rockchip_cpufreq_reboot_notifier);
	if (ret) {
		pr_err("Failed to register reboot handler\n");
		goto release_cluster_info;
	}

	pdata.have_governor_per_policy = true;
	pdata.suspend = cpufreq_generic_suspend;

	return PTR_ERR_OR_ZERO(platform_device_register_data(NULL, "cpufreq-dt",
			       -1, (void *)&pdata,
			       sizeof(struct cpufreq_dt_platform_data)));

release_cluster_info:
	list_for_each_entry_safe(cluster, pos, &cluster_info_list, list_head) {
		list_del(&cluster->list_head);
		kfree(cluster);
	}
	return ret;
}
module_init(rockchip_cpufreq_driver_init);

MODULE_AUTHOR("Finley Xiao <finley.xiao@rock-chips.com>");
MODULE_DESCRIPTION("Rockchip cpufreq driver");
MODULE_LICENSE("GPL v2");
