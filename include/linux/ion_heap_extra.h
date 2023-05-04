// SPDX-License-Identifier: GPL-2.0
/*
 * ION heap extra function header
 *
 * Copyright (C) 2021 Synaptics, Inc.
 */

#ifndef _ION_HEAP_EXTRA_H
#define _ION_HEAP_EXTRA_H

#include <linux/list.h>
#include <linux/ion.h>

struct ion_heap_extra {
	struct list_head list;
	struct ion_heap *heap;
	int (*free_cb)(struct dma_buf *dmabuf);
};

int ion_heap_extra_add_heap(struct ion_heap_extra *heap_extra);

int ion_heap_extra_rm_heap(struct ion_heap_extra *heap_extra);

int ion_heap_extra_reg_free_cb(enum ion_heap_type type,
					int (*free_cb)(struct dma_buf *dmabuf));

#endif /* _ION_HEAP_EXTRA_H */
