// SPDX-License-Identifier: GPL-2.0
/*
 * ION Memory Allocator customized system heap exporter, based on ion system heap.
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (C) 2021 Synaptics, Inc.
 */

#include <asm/page.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/ion.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/ion_heap_extra.h>

#include "ion_page_pool.h"

#define NUM_ORDERS ARRAY_SIZE(orders)

/*
 * OOM not triggered when order is bigger than PAGE_ALLOC_COSTLY_ORDER (3)
 * OOM triggered when order is less or equals to PAGE_ALLOC_COSTLY_ORDER (3)
 * with low_order_gfp_flags.
 * With order > 3, only allocation failed warning if no memory.
 * With order = 3, OOM killer and RCU hang happen due to memory fragment even
 * free memory is enough So assign orders that are bigger than 2 as high_order_gfp_flags
 * to avoid OOM killer easily and RCU hang
 */
static gfp_t high_order_gfp_flags = (GFP_HIGHUSER | __GFP_ZERO | __GFP_NOWARN |
				     __GFP_NORETRY) & ~__GFP_RECLAIM;
static gfp_t low_order_gfp_flags  = GFP_HIGHUSER | __GFP_ZERO;
static const unsigned int orders[] = {4, 3, 2, 1, 0};

static int order_to_index(unsigned int order)
{
	int i;

	for (i = 0; i < NUM_ORDERS; i++)
		if (order == orders[i])
			return i;
	BUG();
	return -1;
}

static inline unsigned int order_to_size(int order)
{
	return PAGE_SIZE << order;
}

struct ion_system_cust_heap {
	struct ion_heap heap;
	struct ion_heap_extra heap_extra;
	struct ion_page_pool *pools[NUM_ORDERS];
};

static struct page *alloc_buffer_page(struct ion_system_cust_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long order)
{
	struct ion_page_pool *pool = heap->pools[order_to_index(order)];

	return ion_page_pool_alloc(pool);
}

static void free_buffer_page(struct ion_system_cust_heap *heap,
			     struct ion_buffer *buffer, struct page *page)
{
	struct ion_page_pool *pool;
	unsigned int order = compound_order(page);

	/* go to system */
	if (buffer->private_flags & ION_PRIV_FLAG_SHRINKER_FREE) {
		__free_pages(page, order);
		return;
	}

	pool = heap->pools[order_to_index(order)];

	ion_page_pool_free(pool, page);
}

static struct page *alloc_largest_available(struct ion_system_cust_heap *heap,
					    struct ion_buffer *buffer,
					    unsigned long size,
					    unsigned int max_order)
{
	struct page *page;
	int i;

	for (i = 0; i < NUM_ORDERS; i++) {
		if (size < order_to_size(orders[i]))
			continue;
		if (max_order < orders[i])
			continue;

		page = alloc_buffer_page(heap, buffer, orders[i]);
		if (!page)
			continue;

		return page;
	}

	return NULL;
}

static int ion_system_cust_heap_allocate(struct ion_heap *heap,
				    struct ion_buffer *buffer,
				    unsigned long size,
				    unsigned long flags)
{
	struct ion_system_cust_heap *sys_heap = container_of(heap,
							struct ion_system_cust_heap,
							heap);
	struct sg_table *table;
	struct scatterlist *sg;
	struct list_head pages;
	struct page *page, *tmp_page;
	int i = 0;
	unsigned long size_remaining = PAGE_ALIGN(size);
	unsigned int max_order = orders[0];

	if (size / PAGE_SIZE > totalram_pages() / 2)
		return -ENOMEM;

	INIT_LIST_HEAD(&pages);
	while (size_remaining > 0) {
		page = alloc_largest_available(sys_heap, buffer, size_remaining,
					       max_order);
		if (!page)
			goto free_pages;
		list_add_tail(&page->lru, &pages);
		size_remaining -= page_size(page);
		max_order = compound_order(page);
		i++;
	}
	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		goto free_pages;

	if (sg_alloc_table(table, i, GFP_KERNEL))
		goto free_table;

	sg = table->sgl;
	list_for_each_entry_safe(page, tmp_page, &pages, lru) {
		sg_set_page(sg, page, page_size(page), 0);
		sg = sg_next(sg);
		list_del(&page->lru);
	}

	buffer->sg_table = table;

	ion_buffer_prep_noncached(buffer);

	return 0;

free_table:
	kfree(table);
free_pages:
	list_for_each_entry_safe(page, tmp_page, &pages, lru)
		free_buffer_page(sys_heap, buffer, page);
	return -ENOMEM;
}

static void ion_system_cust_heap_free(struct ion_buffer *buffer)
{
	struct ion_system_cust_heap *sys_heap = container_of(buffer->heap,
							struct ion_system_cust_heap,
							heap);
	struct sg_table *table = buffer->sg_table;
	struct scatterlist *sg;
	int i;

	for_each_sg(table->sgl, sg, table->nents, i)
		free_buffer_page(sys_heap, buffer, sg_page(sg));
	sg_free_table(table);
	kfree(table);
}

static int ion_system_cust_heap_shrink(struct ion_heap *heap, gfp_t gfp_mask,
				  int nr_to_scan)
{
	struct ion_page_pool *pool;
	struct ion_system_cust_heap *sys_heap;
	int nr_total = 0;
	int i, nr_freed;
	int only_scan = 0;

	sys_heap = container_of(heap, struct ion_system_cust_heap, heap);

	if (!nr_to_scan)
		only_scan = 1;

	for (i = 0; i < NUM_ORDERS; i++) {
		pool = sys_heap->pools[i];

		if (only_scan) {
			nr_total += ion_page_pool_shrink(pool,
							 gfp_mask,
							 nr_to_scan);

		} else {
			nr_freed = ion_page_pool_shrink(pool,
							gfp_mask,
							nr_to_scan);
			nr_to_scan -= nr_freed;
			nr_total += nr_freed;
			if (nr_to_scan <= 0)
				break;
		}
	}
	return nr_total;
}

static long ion_system_cust_get_pool_size(struct ion_heap *heap)
{
	struct ion_system_cust_heap *sys_heap;
	long total_pages = 0;
	int i;

	sys_heap = container_of(heap, struct ion_system_cust_heap, heap);
	for (i = 0; i < NUM_ORDERS; i++)
		total_pages += ion_page_pool_nr_pages(sys_heap->pools[i]);

	return total_pages;
}

static void ion_system_cust_heap_destroy_pools(struct ion_page_pool **pools)
{
	int i;

	for (i = 0; i < NUM_ORDERS; i++)
		if (pools[i])
			ion_page_pool_destroy(pools[i]);
}

static int ion_system_cust_heap_create_pools(struct ion_page_pool **pools)
{
	int i;

	for (i = 0; i < NUM_ORDERS; i++) {
		struct ion_page_pool *pool;
		gfp_t gfp_flags = low_order_gfp_flags;

		if (orders[i] > 2)
			gfp_flags = high_order_gfp_flags;

		pool = ion_page_pool_create(gfp_flags, orders[i]);
		if (!pool)
			goto err_create_pool;
		pools[i] = pool;
	}

	return 0;

err_create_pool:
	ion_system_cust_heap_destroy_pools(pools);
	return -ENOMEM;
}

static struct ion_heap_ops system_cust_heap_ops = {
	.allocate = ion_system_cust_heap_allocate,
	.free = ion_system_cust_heap_free,
	.shrink = ion_system_cust_heap_shrink,
	.get_pool_size = ion_system_cust_get_pool_size,
};

static void ion_system_cust_buf_release(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct ion_system_cust_heap *sys_heap;
	int (*free_cb)(struct dma_buf *dmabuf);

	sys_heap = container_of(buffer->heap, struct ion_system_cust_heap, heap);

	free_cb = sys_heap->heap_extra.free_cb;
	if (free_cb) {
		if (!free_cb(dmabuf))
			ion_free(buffer);
	} else
		ion_free(buffer);
}

static struct ion_system_cust_heap system_cust_heap = {
	.heap = {
		.ops = &system_cust_heap_ops,
		.type = ION_HEAP_TYPE_SYSTEM_CUST,
		.name = "ion_system_cust_heap",
	}
};

static int __init ion_system_cust_heap_init(void)
{
	int ret = ion_system_cust_heap_create_pools(system_cust_heap.pools);

	if (ret)
		return ret;

	system_cust_heap.heap.buf_ops.release = ion_system_cust_buf_release;

	ret = ion_device_add_heap(&system_cust_heap.heap);
	if (!ret) {
		system_cust_heap.heap_extra.heap = &system_cust_heap.heap;
		ion_heap_extra_add_heap(&system_cust_heap.heap_extra);
	}

	return ret;
}

static void __exit ion_system_cust_heap_exit(void)
{
	ion_heap_extra_rm_heap(&system_cust_heap.heap_extra);
	ion_device_remove_heap(&system_cust_heap.heap);
	ion_system_cust_heap_destroy_pools(system_cust_heap.pools);
}

module_init(ion_system_cust_heap_init);
module_exit(ion_system_cust_heap_exit);
MODULE_LICENSE("GPL v2");
