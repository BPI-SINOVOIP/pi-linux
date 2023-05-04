// SPDX-License-Identifier: GPL-2.0
/*
 * dhub irq controller driver for Berlin SoCs
 *
 * Copyright (C) 2018 Synaptics Incorporated
 */

#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#define SEMAHUB_ARR	0x100
#define SEMAPHORE_CFG	0x0
#define SEMAPHORE_INTR	0x4
#define SEMAPHORE_MASK	0x10

#define TOTAL_SEMA_NUM			32
#define SEMA_CPU_DELTA			8
#define SEMAINTR_MASK_EMPTY		1
#define SEMAINTR_MASK_FULL		(1 << 1)
#define SEMAINTR_MASK_ALMOST_EMPTY	(1 << 2)
#define SEMAINTR_MASK_ALMOST_FULL	(1 << 3)

struct semahub_ctl {
	u32 pop_offset;
	u32 full_offset;
	u32 cell_size;
};

struct semahub_gc_data {
	struct irq_domain *domain;
	u32 cached_ictl_mask;	/* shadow ctrl for mask */
	const struct semahub_ctl *ctl;

	/* context for suspend/resume */
	u32 intr_context[TOTAL_SEMA_NUM];
};

static const struct semahub_ctl berlin_semahub_ctl = {
	.pop_offset = 0x384,
	.full_offset = 0x38c,
	.cell_size = 0x14,
};

static const struct semahub_ctl default_semahub_ctl = {
	.pop_offset = 0x404,
	.full_offset = 0x40c,
	.cell_size = 0x18,
};

static const struct of_device_id semahub_matches[] = {
	{ .compatible = "syna,berlin-dhub-irq", .data = &berlin_semahub_ctl },
	{ .compatible = "syna,as370-dhub-irq", .data = &default_semahub_ctl },
	{ .compatible = "syna,vs680-dhub-irq", .data = &default_semahub_ctl },
	{ .compatible = "syna,as470-dhub-irq", .data = &default_semahub_ctl },
	{ .compatible = "syna,vs640-dhub-irq", .data = &default_semahub_ctl },
	{ }
};

static void berlin_dhub_ictl_irq_handler(struct irq_desc *desc)
{
	struct semahub_gc_data *gc_data = irq_desc_get_handler_data(desc);
	struct irq_domain *d = gc_data->domain;
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct irq_chip_generic *gc = irq_get_domain_generic_chip(d, 0);
	u32 stat;
	unsigned int hwirq, virq;

	chained_irq_enter(chip, desc);

	stat = readl_relaxed(gc->reg_base + gc_data->ctl->full_offset);
	/* get final stat */
	stat = stat & (~gc_data->cached_ictl_mask);
	while (stat) {
		hwirq = ffs(stat) - 1;
		virq = irq_find_mapping(d, hwirq);
		generic_handle_irq(virq);
		stat &= ~(1 << hwirq);
	}

	chained_irq_exit(chip, desc);
}

static inline void semahub_cell_write(const struct irq_chip_generic *gc,
				      u32 idx, u32 offset, u32 val)
{
	const struct semahub_gc_data *gc_data = gc->private;

	if (idx < TOTAL_SEMA_NUM) {
		writel_relaxed(val, gc->reg_base + SEMAHUB_ARR +
			       idx * gc_data->ctl->cell_size + offset);
	}
}

static inline u32 semahub_cell_read(const struct irq_chip_generic *gc,
				    u32 idx, u32 offset)
{
	const struct semahub_gc_data *gc_data = gc->private;

	return readl_relaxed(gc->reg_base + SEMAHUB_ARR +
			     idx * gc_data->ctl->cell_size + offset);
}

static void berlin_dhub_mask_irq(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct semahub_gc_data *gc_data = gc->private;
	irq_hw_number_t hw_irq = irqd_to_hwirq(d);
	u32 mask = 1 << hw_irq;

	irq_gc_lock(gc);
	gc_data->cached_ictl_mask |= mask;
	semahub_cell_write(gc, hw_irq, SEMAPHORE_INTR, 0);
	irq_gc_unlock(gc);
}

static void berlin_dhub_unmask_irq(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct semahub_gc_data *gc_data = gc->private;
	irq_hw_number_t hw_irq = irqd_to_hwirq(d);
	u32 mask = 1 << hw_irq;

	irq_gc_lock(gc);
	gc_data->cached_ictl_mask &= ~mask;
	semahub_cell_write(gc, hw_irq, SEMAPHORE_INTR, SEMAINTR_MASK_FULL);
	irq_gc_unlock(gc);
}

static void berlin_dhub_ack_irq(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct semahub_gc_data *gc_data = gc->private;
	irq_hw_number_t hw_irq = irqd_to_hwirq(d);
	u32 mask = d->mask;
	u32 ack = (1 << SEMA_CPU_DELTA) | hw_irq;

	irq_gc_lock(gc);
	/* CPU pop semaphore */
	writel_relaxed(ack, gc->reg_base + gc_data->ctl->pop_offset);
	/* clear cell full status */
	writel_relaxed(mask, gc->reg_base + gc_data->ctl->full_offset);
	irq_gc_unlock(gc);
}

static void berlin_dhub_irq_config(struct irq_chip_generic *gc)
{
	int i;
	u32 stat;
	struct semahub_gc_data *gc_data = (struct semahub_gc_data *)gc->private;

	/* config and disable all interrupts */
	for (i = 0; i < TOTAL_SEMA_NUM; i++) {
		/* set semaphore depth to 1 */
		semahub_cell_write(gc, i, SEMAPHORE_CFG, 1);
		semahub_cell_write(gc, i, SEMAPHORE_INTR, 0);
	}
	/* clear status if any */
	stat = readl_relaxed(gc->reg_base + gc_data->ctl->full_offset);
	if (stat)
		writel_relaxed(stat, gc->reg_base + gc_data->ctl->full_offset);
	gc_data->cached_ictl_mask = ~0;
}

#ifdef CONFIG_PM
static void berlin_dhub_irq_save_load_context(struct irq_chip_generic *gc,
					      bool save)
{
	int i;
	struct semahub_gc_data *gc_data = gc->private;

	for (i = 0; i < TOTAL_SEMA_NUM; i++) {
		if (save) {
			/* depth is alwasy 1, no need to save */
			gc_data->intr_context[i] =
				semahub_cell_read(gc, i, SEMAPHORE_INTR);
		} else {
			semahub_cell_write(gc, i, SEMAPHORE_CFG, 1);
			semahub_cell_write(gc, i, SEMAPHORE_INTR,
					   gc_data->intr_context[i]);
		}
	}
}

static void berlin_dhub_irq_suspend(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);

	irq_gc_lock(gc);
	berlin_dhub_irq_save_load_context(gc, true);
	irq_gc_unlock(gc);
}

static void berlin_dhub_irq_resume(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);

	irq_gc_lock(gc);
	berlin_dhub_irq_save_load_context(gc, false);
	irq_gc_unlock(gc);
}
#else
#define berlin_dhub_irq_suspend NULL
#define berlin_dhub_irq_resume	NULL
#endif /* CONFIG_PM */

static int __init berlin_dhub_irq_init(struct device_node *np,
				       struct device_node *parent)
{
	unsigned int clr = IRQ_NOREQUEST | IRQ_NOPROBE | IRQ_NOAUTOEN;
	struct resource r;
	const struct of_device_id *match;
	const struct semahub_ctl *semahub_ctl_p;
	struct semahub_gc_data *semahub_gc_data;
	struct irq_domain *domain;
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;
	void __iomem *iobase;
	int ret, parent_irq;

	/* Map the parent interrupt for the chained handler */
	parent_irq = irq_of_parse_and_map(np, 0);
	if (!parent_irq) {
		pr_err("%s: unable to parse irq\n", np->full_name);
		return -EINVAL;
	}

	match = of_match_node(semahub_matches, np);
	if (!match)	/* This shall never happen. */
		return -ENODEV;
	semahub_ctl_p = match->data;

	ret = of_address_to_resource(np, 0, &r);
	if (ret) {
		pr_err("%s: unable to get resource\n", np->full_name);
		return ret;
	}

	semahub_gc_data = kzalloc(sizeof(*semahub_gc_data), GFP_KERNEL);
	if (!semahub_gc_data)
		return -ENOMEM;

	if (!request_mem_region(r.start, resource_size(&r), np->full_name)) {
		pr_err("%s: unable to request mem region\n", np->full_name);
		ret = -ENOMEM;
		goto err_outmem;
	}

	iobase = ioremap(r.start, resource_size(&r));
	if (!iobase) {
		pr_err("%s: unable to map resource\n", np->full_name);
		ret = -ENOMEM;
		goto err_release;
	}
	/* 32 interrupt sources per Dhub */
	domain = irq_domain_add_linear(np, TOTAL_SEMA_NUM,
				       &irq_generic_chip_ops, NULL);
	if (!domain) {
		pr_err("%s: unable to add irq domain\n", np->full_name);
		ret = -ENOMEM;
		goto err_unmap;
	}

	/* Allocate a single Generic IRQ chip for this node */
	ret = irq_alloc_domain_generic_chips(domain, TOTAL_SEMA_NUM, 1,
					     np->full_name, handle_edge_irq,
					     clr, 0, 0);
	if (ret) {
		pr_err("%s: unable to alloc irq domain gc\n", np->full_name);
		goto err_unmap;
	}

	semahub_gc_data->domain = domain;
	semahub_gc_data->ctl = semahub_ctl_p;

	gc = irq_get_domain_generic_chip(domain, 0);

	gc->private = semahub_gc_data;
	gc->reg_base = iobase;
	ct = gc->chip_types;

	ct->chip.irq_ack = berlin_dhub_ack_irq;
	ct->chip.irq_mask = berlin_dhub_mask_irq;
	ct->chip.irq_unmask = berlin_dhub_unmask_irq;
	ct->chip.irq_suspend = berlin_dhub_irq_suspend;
	ct->chip.irq_resume = berlin_dhub_irq_resume;

	/* config the hardware */
	berlin_dhub_irq_config(gc);
	/* Set the IRQ chaining logic */
	irq_set_chained_handler_and_data(parent_irq,
					 berlin_dhub_ictl_irq_handler,
					 semahub_gc_data);

	return 0;

err_unmap:
	iounmap(iobase);
err_release:
	release_mem_region(r.start, resource_size(&r));
err_outmem:
	kfree(semahub_gc_data);
	return ret;
}

IRQCHIP_DECLARE(berlin_dhub_ictl,
		"syna,berlin-dhub-irq", berlin_dhub_irq_init);
IRQCHIP_DECLARE(as370_dhub_ictl,
		"syna,as370-dhub-irq", berlin_dhub_irq_init);
IRQCHIP_DECLARE(vs680_dhub_ictl,
		"syna,vs680-dhub-irq", berlin_dhub_irq_init);
IRQCHIP_DECLARE(as470_dhub_ictl,
		"syna,as470-dhub-irq", berlin_dhub_irq_init);
IRQCHIP_DECLARE(vs640_dhub_ictl,
		"syna,vs640-dhub-irq", berlin_dhub_irq_init);
