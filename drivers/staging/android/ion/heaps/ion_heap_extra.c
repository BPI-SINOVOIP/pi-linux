// SPDX-License-Identifier: GPL-2.0
/*
 * ION heap extra function support
 *
 * Copyright (C) 2021 Synaptics, Inc.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/ion_heap_extra.h>

static LIST_HEAD(heap_extra_list);
static DEFINE_SPINLOCK(heap_extra_lock);

#define CB_NUMS			(ION_HEAP_TYPE_MAX - ION_HEAP_TYPE_CUSTOM + 1)
#define CB_TYPE_DEFAULT		(ION_HEAP_TYPE_MAX - ION_HEAP_TYPE_CUSTOM)
static int (*free_cb_type[CB_NUMS])(struct dma_buf *dmabuf);

int ion_heap_extra_add_heap(struct ion_heap_extra *heap_extra)
{
	int type;

	type = heap_extra->heap->type;
	if ((type >= ION_HEAP_TYPE_MAX) || (type < ION_HEAP_TYPE_CUSTOM))
		return -EINVAL;

	type -= ION_HEAP_TYPE_CUSTOM;
	spin_lock(&heap_extra_lock);

	if (free_cb_type[type])
		heap_extra->free_cb = free_cb_type[type];
	else if (free_cb_type[CB_TYPE_DEFAULT])
		heap_extra->free_cb = free_cb_type[CB_TYPE_DEFAULT];

	list_add(&heap_extra->list, &heap_extra_list);

	spin_unlock(&heap_extra_lock);

	return 0;
}
EXPORT_SYMBOL(ion_heap_extra_add_heap);

int ion_heap_extra_rm_heap(struct ion_heap_extra *heap_extra)
{
	spin_lock(&heap_extra_lock);

	list_del(&heap_extra->list);

	spin_unlock(&heap_extra_lock);

	return 0;
}
EXPORT_SYMBOL(ion_heap_extra_rm_heap);

int ion_heap_extra_reg_free_cb(enum ion_heap_type type,
					int (*free_cb)(struct dma_buf *dmabuf))
{
	struct ion_heap_extra *heap_extra;

	if ((type > ION_HEAP_TYPE_MAX) || (type < ION_HEAP_TYPE_CUSTOM))
		return -EINVAL;

	spin_lock(&heap_extra_lock);

	free_cb_type[type - ION_HEAP_TYPE_CUSTOM] = free_cb;

	if (type == ION_HEAP_TYPE_MAX) {
		list_for_each_entry(heap_extra, &heap_extra_list, list) {
				heap_extra->free_cb = free_cb;
		}
	} else {
		list_for_each_entry(heap_extra, &heap_extra_list, list) {
			if (heap_extra->heap->type == type) {
				heap_extra->free_cb = free_cb;
			}
		}
	}
	spin_unlock(&heap_extra_lock);

	return 0;
}
EXPORT_SYMBOL(ion_heap_extra_reg_free_cb);

static int __init ion_heap_extra_init(void)
{
	return 0;
}

static void __exit ion_heap_extra_exit(void)
{
	struct ion_heap_extra *heap_extra, *n;

	spin_lock(&heap_extra_lock);

	list_for_each_entry_safe(heap_extra, n, &heap_extra_list, list) {
		list_del(&heap_extra->list);
	}

	spin_unlock(&heap_extra_lock);
}

module_init(ion_heap_extra_init);
module_exit(ion_heap_extra_exit);
MODULE_LICENSE("GPL v2");
