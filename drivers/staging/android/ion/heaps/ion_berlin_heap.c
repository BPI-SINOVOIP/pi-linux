// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/staging/android/ion/ion_berlin_heap.c
 *
 * Copyright (C) 2020 Synaptics Incorporated
 *
 */

#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/ion.h>
#include <linux/debugfs.h>
#include <linux/ion_heap_extra.h>

#include "../ion_private.h"

#define ION_BERLIN_ALLOCATE_FAIL	-1
#define BEST_FIT_ATTRIBUTE		BIT(8)
#define MAX_ATTR_BUF_LEN		32

struct ion_berlin_heap {
	struct ion_heap heap;
	struct ion_heap_extra heap_extra;
	struct gen_pool *pool;
	struct device *dev;
};

struct ion_berlin_info {
	int heap_num;
	struct ion_platform_heap *heaps_data;
	struct ion_heap **heaps;
};

/*
 * This is a copy of ion_buffer_kmap_get() which isn't exported by core ion.
 * To modularize berlin heap, we copy the function here
 */
static void *ion_buffer_kmap_get_internel(struct ion_buffer *buffer)
{
	void *vaddr;

	if (buffer->kmap_cnt) {
		buffer->kmap_cnt++;
		return buffer->vaddr;
	}
	vaddr = ion_heap_map_kernel(buffer->heap, buffer);
	if (WARN_ONCE(!vaddr,
		      "heap->ops->map_kernel should return ERR_PTR on error"))
		return ERR_PTR(-EINVAL);
	if (IS_ERR(vaddr))
		return vaddr;
	buffer->vaddr = vaddr;
	buffer->kmap_cnt++;
	return vaddr;
}

static phys_addr_t ion_berlin_allocate(struct ion_heap *heap,
					unsigned long size)
{
	struct ion_berlin_heap *berlin_heap =
		container_of(heap, struct ion_berlin_heap, heap);
	unsigned long offset = gen_pool_alloc(berlin_heap->pool, size);

	if (!offset)
		return ION_BERLIN_ALLOCATE_FAIL;

	return offset;
}

static void ion_berlin_free(struct ion_heap *heap, phys_addr_t addr,
			    unsigned long size)
{
	struct ion_berlin_heap *berlin_heap =
		container_of(heap, struct ion_berlin_heap, heap);

	if (addr == ION_BERLIN_ALLOCATE_FAIL)
		return;

	gen_pool_free(berlin_heap->pool, addr, size);
}

static int ion_berlin_heap_allocate(struct ion_heap *heap,
				    struct ion_buffer *buffer,
				    unsigned long size,
				    unsigned long flags)
{
	struct sg_table *table;
	phys_addr_t paddr;
	int ret;

	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto err_free;

	paddr = ion_berlin_allocate(heap, size);
	if (paddr == ION_BERLIN_ALLOCATE_FAIL) {
		ret = -ENOMEM;
		goto err_free_table;
	}

	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(paddr)), size, 0);
	buffer->sg_table = table;

	return 0;

err_free_table:
	sg_free_table(table);
err_free:
	kfree(table);
	return ret;
}

static void ion_berlin_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct sg_table *table = buffer->sg_table;
	struct page *page = sg_page(table->sgl);
	phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	ion_berlin_free(heap, paddr, buffer->size);
	sg_free_table(table);
	kfree(table);
}

static int ion_berlin_dma_sync(struct ion_heap *heap,
			       struct ion_buffer *buffer,
			       struct ion_berlin_data *region)
{
	struct ion_berlin_heap *berlin_heap =
		container_of(heap, struct ion_berlin_heap, heap);
	struct device * dev = berlin_heap->dev;
	struct sg_table *table = buffer->sg_table;
	unsigned long offset, region_len, op_len;
	struct scatterlist *sg;
	phys_addr_t phys_addr;
	int i;

	offset = region->offset;
	region_len = region->len;

	switch (region->cmd) {
	case ION_INVALIDATE_CACHE:
	{
		mutex_lock(&buffer->lock);
		for_each_sg(table->sgl, sg, table->nents, i) {
			if (sg->length > offset) {
				phys_addr = PFN_PHYS(page_to_pfn(sg_page(sg)));
				op_len = sg->length - offset;
				op_len = op_len > region_len ? region_len : op_len;
				dma_sync_single_for_cpu(dev, phys_addr + offset,
					op_len, DMA_FROM_DEVICE);
				region_len -= op_len;
				if (!region_len)
					break;
				offset = 0;
			} else {
				offset -= sg->length;
			}
		}
		mutex_unlock(&buffer->lock);
		break;
	}
	case ION_CLEAN_CACHE:
	{
		mutex_lock(&buffer->lock);
		for_each_sg(table->sgl, sg, table->nents, i) {
			if (sg->length > offset) {
				phys_addr = PFN_PHYS(page_to_pfn(sg_page(sg)));
				op_len = sg->length - offset;
				op_len = op_len > region_len ? region_len : op_len;
				dma_sync_single_for_device(dev, phys_addr + offset,
					op_len, DMA_TO_DEVICE);
				region_len -= op_len;
				if (!region_len)
					break;
				offset = 0;
			} else {
				offset -= sg->length;
			}
		}
		mutex_unlock(&buffer->lock);
		break;
	}
	case ION_FLUSH_CACHE:
	{
		mutex_lock(&buffer->lock);
		for_each_sg(table->sgl, sg, table->nents, i) {
			if (sg->length > offset) {
				phys_addr = PFN_PHYS(page_to_pfn(sg_page(sg)));
				op_len = sg->length - offset;
				op_len = op_len > region_len ? region_len : op_len;
				dma_sync_single_for_device(dev, phys_addr + offset,
					op_len, DMA_TO_DEVICE);
				dma_sync_single_for_cpu(dev, phys_addr + offset,
					op_len, DMA_FROM_DEVICE);
				region_len -= op_len;
				if (!region_len)
					break;
				offset = 0;
			} else {
				offset -= sg->length;
			}
		}
		mutex_unlock(&buffer->lock);
		break;
	}
	default:
		pr_err("%s: Unknown cache command %d\n",
			__func__, region->cmd);
		return -EINVAL;
	}

	return 0;
}

static int ion_berlin_get_phy(struct ion_heap *heap,
			      struct ion_buffer *buffer,
			      struct ion_berlin_data *region)
{
	struct sg_table *table;
	struct page *page;

	table = buffer->sg_table;
	page = sg_page(table->sgl);
	region->addr = (unsigned long)PFN_PHYS(page_to_pfn(page));
	region->len = buffer->size;
	return 0;
}

static int ion_berlin_custom_ioctl(struct ion_heap *heap,
				   struct ion_buffer *buffer,
				   struct ion_custom_data *custom_data)
{
	int ret = -EINVAL;
	struct ion_berlin_data region;

	switch (custom_data->cmd) {
		case ION_BERLIN_SYNC:
			if (heap->flags & (ION_HEAP_FLAG_SECURE | ION_HEAP_FLAG_NONCACHED))
				return -EPERM;

			if (copy_from_user(&region, (void __user *)custom_data->arg,
					   sizeof(region)))
				return -EFAULT;
			ret = ion_berlin_dma_sync(heap, buffer, &region);
			break;
		case ION_BERLIN_PHYS:
			memset(&region, 0, sizeof(region));
			ret = ion_berlin_get_phy(heap, buffer, &region);
			if (!ret)
				ret = copy_to_user((void __user *)custom_data->arg, &region,
								sizeof(region));
			break;
		default:
			pr_err("%s: Unknown ioctl %d\n", __FUNCTION__, custom_data->cmd);
			return -EINVAL;
	}
	return ret;
}

static int ion_berlin_heap_get_attr(struct ion_heap *heap, unsigned int *attr)
{
	struct ion_platform_heap *heap_data = heap->heap_data;
	int ret = 0;

	if (heap_data && attr) {
		*attr = heap_data->attribute[1];
	} else
		ret = -EINVAL;

	return ret;
}

static long ion_berlin_get_pool_size(struct ion_heap *heap)
{
	struct ion_platform_heap *heap_data = heap->heap_data;

	return heap_data->size >> PAGE_SHIFT;
}

static struct ion_heap_ops berlin_heap_ops = {
	.allocate = ion_berlin_heap_allocate,
	.free = ion_berlin_heap_free,
	.get_pool_size = ion_berlin_get_pool_size,
	.custom_ioctl = ion_berlin_custom_ioctl,
	.get_heap_attr = ion_berlin_heap_get_attr,
};

static int ion_berlin_heap_debug_show(struct ion_heap *heap,
				      struct seq_file *s, void *unused)
{
	struct ion_platform_heap *heap_data = heap ->heap_data;

	seq_printf(s, "heap %s: base: 0x%08llx, size 0x%lx\n",
				heap->name, heap_data->base,
				heap_data->size);

	seq_printf(s, "attr: [0x%08x 0x%08x]\n",
				heap_data->attribute[0],
				heap_data->attribute[1]);

	return 0;
}

static struct sg_table *ion_berlin_map_dma_buf(struct dma_buf_attachment *attachment,
					enum dma_data_direction direction)
{
	struct ion_buffer *buffer = attachment->dmabuf->priv;
	struct ion_heap *heap = buffer->heap;
	struct ion_dma_buf_attachment *a;
	struct sg_table *table;
	unsigned long attrs = attachment->dma_map_attrs;

	a = attachment->priv;
	table = a->table;

	if (heap->flags & ION_HEAP_FLAG_SECURE)
		return table;

	if (!(buffer->flags & ION_FLAG_CACHED))
		attrs |= DMA_ATTR_SKIP_CPU_SYNC;

	if (!dma_map_sg_attrs(attachment->dev, table->sgl, table->nents,
			      direction, attrs))
		return ERR_PTR(-ENOMEM);

	a->mapped = true;

	return table;
}

static void ion_berlin_unmap_dma_buf(struct dma_buf_attachment *attachment,
				struct sg_table *table,
				enum dma_data_direction direction)
{
	struct ion_dma_buf_attachment *a = attachment->priv;

	if (a->mapped) {
		struct ion_buffer *buffer = attachment->dmabuf->priv;
		unsigned long attrs = attachment->dma_map_attrs;

		a->mapped = false;

		if (!(buffer->flags & ION_FLAG_CACHED))
			attrs |= DMA_ATTR_SKIP_CPU_SYNC;

		dma_unmap_sg_attrs(attachment->dev, table->sgl,
				   table->nents,
				   direction, attrs);
	}
}

static int ion_berlin_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
						enum dma_data_direction direction)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct ion_heap *heap = buffer->heap;
	struct ion_dma_buf_attachment *a;

	if (heap->flags & ION_HEAP_FLAG_SECURE)
		return -EPERM;

	mutex_lock(&buffer->lock);
	if (!(buffer->flags & ION_FLAG_CACHED))
		goto unlock;

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sg_for_cpu(a->dev, a->table->sgl, a->table->nents,
				    direction);
	}

unlock:
	mutex_unlock(&buffer->lock);
	return 0;
}

static int ion_berlin_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
						enum dma_data_direction direction)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct ion_heap *heap = buffer->heap;
	struct ion_dma_buf_attachment *a;

	if (heap->flags & ION_HEAP_FLAG_SECURE)
		return -EPERM;

	mutex_lock(&buffer->lock);
	if (!(buffer->flags & ION_FLAG_CACHED))
		goto unlock;

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sg_for_device(a->dev, a->table->sgl, a->table->nents,
				       direction);
	}

unlock:
	mutex_unlock(&buffer->lock);
	return 0;
}

static void *ion_berlin_dma_buf_map(struct dma_buf *dmabuf, unsigned long offset)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct ion_heap *heap = buffer->heap;
	void *vaddr;

	if (heap->flags & ION_HEAP_FLAG_SECURE) {
		pr_err("<%s>: try to map secure mem.\n", __FUNCTION__);
		dump_stack();
		return ERR_PTR(-EPERM);
	}

	vaddr = ion_buffer_kmap_get_internel(buffer);
	if (IS_ERR(vaddr))
		return vaddr;

	return vaddr + offset * PAGE_SIZE;
}

static int ion_berlin_dma_buf_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct ion_heap *heap = buffer->heap;
	int ret;

	if (heap->flags & ION_HEAP_FLAG_SECURE) {
		pr_err("<%s>: Process <%s> try to map secure mem.\n",
			__FUNCTION__, current->comm);
		return -EPERM;
	}

	mutex_lock(&buffer->lock);
	if (!(buffer->flags & ION_FLAG_CACHED))
		vma->vm_page_prot =
			pgprot_writecombine(vma->vm_page_prot);

	ret = ion_heap_map_user(heap, buffer, vma);
	mutex_unlock(&buffer->lock);

	if (ret)
		pr_err("%s: failure mapping buffer to userspace\n", __func__);

	return ret;
}

static void *ion_berlin_dma_buf_vmap(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct ion_heap *heap = buffer->heap;
	void *vaddr;

	if (heap->flags & ION_HEAP_FLAG_SECURE) {
		pr_err("<%s>: try to map secure mem.\n", __FUNCTION__);
		dump_stack();
		return ERR_PTR(-EPERM);
	}

	mutex_lock(&buffer->lock);
	vaddr = ion_buffer_kmap_get_internel(buffer);
	mutex_unlock(&buffer->lock);

	return vaddr;
}

static void ion_berlin_dma_buf_release(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct ion_berlin_heap *berlin_heap;
	int (*free_cb)(struct dma_buf *dmabuf);

	berlin_heap = container_of(buffer->heap, struct ion_berlin_heap, heap);
	free_cb = berlin_heap->heap_extra.free_cb;
	if (free_cb) {
		if (!free_cb(dmabuf))
			ion_free(buffer);
	} else
		ion_free(buffer);
}

struct ion_heap *ion_berlin_heap_create(struct ion_platform_heap *heap_data,
					struct device *dev)
{
	struct ion_berlin_heap *berlin_heap;

	berlin_heap = kzalloc(sizeof(*berlin_heap), GFP_KERNEL);
	if (!berlin_heap)
		return ERR_PTR(-ENOMEM);

	berlin_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!berlin_heap->pool) {
		kfree(berlin_heap);
		return ERR_PTR(-ENOMEM);
	}

	if (heap_data->attribute[0] & BEST_FIT_ATTRIBUTE)
		gen_pool_set_algo(berlin_heap->pool, gen_pool_best_fit, NULL);

	berlin_heap->dev = dev;
	gen_pool_add(berlin_heap->pool, heap_data->base, heap_data->size,
		     -1);
	berlin_heap->heap.ops = &berlin_heap_ops;
	if (heap_data->attribute[1] & ION_A_FS) {
		berlin_heap->heap.type = ION_HEAP_TYPE_BERLIN_SECURE;
	} else if (heap_data->attribute[1] & ION_A_NC) {
		berlin_heap->heap.type = ION_HEAP_TYPE_BERLIN_NC;
	} else
		berlin_heap->heap.type = ION_HEAP_TYPE_BERLIN;
	berlin_heap->heap.flags = ION_HEAP_FLAG_DEFER_FREE;
	berlin_heap->heap.heap_data = heap_data;
	berlin_heap->heap.name = heap_data->name;
	berlin_heap->heap.buf_ops.map = ion_berlin_dma_buf_map;
	berlin_heap->heap.buf_ops.mmap = ion_berlin_dma_buf_mmap;
	berlin_heap->heap.buf_ops.vmap = ion_berlin_dma_buf_vmap;
	berlin_heap->heap.buf_ops.map_dma_buf = ion_berlin_map_dma_buf;
	berlin_heap->heap.buf_ops.unmap_dma_buf = ion_berlin_unmap_dma_buf;
	berlin_heap->heap.buf_ops.begin_cpu_access =
					ion_berlin_dma_buf_begin_cpu_access;
	berlin_heap->heap.buf_ops.end_cpu_access =
					ion_berlin_dma_buf_end_cpu_access;
	berlin_heap->heap.buf_ops.release = ion_berlin_dma_buf_release;

	if ((heap_data->attribute[1] & ION_A_FC) == 0)
		berlin_heap->heap.flags |= ION_HEAP_FLAG_NONCACHED;

	if (heap_data->attribute[1] & ION_A_FS)
		berlin_heap->heap.flags |= ION_HEAP_FLAG_SECURE;

	berlin_heap->heap_extra.heap = &berlin_heap->heap;
	ion_heap_extra_add_heap(&berlin_heap->heap_extra);

	return &berlin_heap->heap;
}

static void ion_berlin_heap_destroy(struct ion_heap *heap)
{
	struct ion_berlin_heap *berlin_heap =
		container_of(heap, struct  ion_berlin_heap, heap);

	ion_heap_extra_rm_heap(&berlin_heap->heap_extra);
	gen_pool_destroy(berlin_heap->pool);
	kfree(berlin_heap);
}

static void *ion_berlin_malloc_info(struct device *dev, int heap_num)
{
	int size = 0;
	unsigned char *p = NULL;
	struct ion_berlin_info *info = NULL;
	int info_len = sizeof(*info);
	int heaps_data_len = sizeof(struct ion_platform_heap);
	int heaps_len = sizeof(struct ion_heap *);

	size =  info_len + heaps_data_len * heap_num
		+ heaps_len * heap_num;
	p = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!p) {
		pr_err("ion_berlin_malloc_info fail\n");
		return p;
	}

	info = (struct ion_berlin_info *)(p);
	info->heap_num = heap_num;
	info->heaps_data = (struct ion_platform_heap *)(p + info_len);
	info->heaps = (struct ion_heap **)(p + info_len +
			heaps_data_len * heap_num);

	return (void *)(p);
}

static int ion_berlin_get_info(struct device *dev,
			       struct ion_berlin_info **info)
{
	int i, res = -ENODEV;
	int heap_num = 0;
	struct device_node *np;
	struct resource r;
	struct ion_berlin_info *tmp_info;
	int attri_num = 0;
	unsigned int attr_buf[MAX_ATTR_BUF_LEN];

	np = dev->of_node;
	if (!np)
		goto err_node;

	res = of_property_read_u32(np, "pool-num", &heap_num);
	if (res)
		goto err_node;

	tmp_info = (struct ion_berlin_info *)ion_berlin_malloc_info(dev, heap_num);
	if (!tmp_info) {
		res = -ENOMEM;
		goto err_node;
	}

	res = of_property_read_u32(np, "attributes-num-per-pool",
				   &attri_num);
	if (res)
		goto err_node;

	if ((heap_num * attri_num) > MAX_ATTR_BUF_LEN) {
		pr_err("%d * %d exceed attribute buffer\n", heap_num, attri_num);
		goto err_node;
	}

	res = of_property_read_u32_array(np, "pool-attributes",
					 attr_buf, heap_num * attri_num);
	if (res) {
		pr_err("get mrvl,ion-pool-attributes fail\n");
		goto err_node;
	}

	for (i = 0; i < heap_num; i++) {
		res = of_address_to_resource(np, i, &r);
		if (res)
			goto err_node;
		(tmp_info->heaps_data + i)->base = r.start;
		(tmp_info->heaps_data + i)->size = resource_size(&r);
		(tmp_info->heaps_data + i)->name = r.name;
		(tmp_info->heaps_data + i)->attribute[0] = attr_buf[i*attri_num];
		(tmp_info->heaps_data + i)->attribute[1] = attr_buf[i*attri_num + 1];
	}

	*info = tmp_info;
	return 0;

err_node:
	pr_err("ion_berlin_get_info err %d\n", res);
	return res;
}

static int ion_berlin_probe(struct platform_device *pdev)
{
	int res = 0;
	int i = 0;
	struct ion_berlin_info *info;

	res = ion_berlin_get_info(&pdev->dev, &info);
	if (res != 0)
		return res;

	for (i = 0; i < info->heap_num; i++) {
		struct ion_platform_heap *heap_data = (info->heaps_data + i);
		info->heaps[i] = ion_berlin_heap_create(heap_data, &pdev->dev);

		if (IS_ERR_OR_NULL(info->heaps[i])) {
			res = PTR_ERR(info->heaps[i]);
			info->heaps[i] = NULL;
			goto err_create_heap;
		}
		info->heaps[i]->debug_show = ion_berlin_heap_debug_show;
		ion_device_add_heap(info->heaps[i]);
	}
	platform_set_drvdata(pdev, info);
	dev_info(&pdev->dev, "ion_berlin_probe %d heaps done\n", info->heap_num);
	return 0;

err_create_heap:
	for (i = 0; i < info->heap_num; i++) {
		if (info->heaps[i])
			ion_berlin_heap_destroy(info->heaps[i]);
	}

	return res;
}

static int ion_berlin_remove(struct platform_device *pdev)
{
	int i = 0;
	struct ion_berlin_info *info;

	info = (struct ion_berlin_info *)dev_get_drvdata(&pdev->dev);

	for (i = 0; i < info->heap_num; i++) {
		if (info->heaps[i])
			ion_berlin_heap_destroy(info->heaps[i]);
	}

	return 0;
}

static const struct of_device_id ion_berlin_heaps_of_match[] = {
	{ .compatible = "syna,ion-berlin-heaps", },
	{},
};

static struct platform_driver ion_berlin_driver = {
	.probe = ion_berlin_probe,
	.remove = ion_berlin_remove,
	.driver = {
		.name	= "ion-berlin",
		.of_match_table = ion_berlin_heaps_of_match,
	},
};

static int __init ion_berlin_init(void)
{
	return platform_driver_register(&ion_berlin_driver);
}
device_initcall(ion_berlin_init);
MODULE_LICENSE("GPL v2");
