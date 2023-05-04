/*
 * TrustZone Driver
 *
 * Copyright (C) 2019 Synaptics Ltd.
 * Copyright (C) 2016 Marvell Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#ifdef CONFIG_ION
#include "mem_region_userdata.h"
#include <linux/ion.h>
#include <linux/dma-buf.h>
#endif /* CONFIG_ION */
#include "config.h"
#include "tz_log.h"
#include "tz_driver_private.h"

struct shm_ops {
	int (*init)(void);
	int (*malloc)(struct tzd_shm *tzshm, gfp_t flags);
	int (*free)(struct tzd_shm *tzshm);
	int (*destroy)(void);
};

#ifdef CONFIG_ION

static unsigned int heap_id_mask;

static int tzd_ion_shm_init(void)
{
	int heap_num, i;
	struct ion_heap_data *hdata;

	hdata = kmalloc(sizeof(*hdata) * ION_NUM_MAX_HEAPS, GFP_KERNEL);
	if (!hdata) {
		tz_error("tzd_ion_shm_init alloc mem failed\n");
		return -ENOMEM;
	}

	heap_num = ion_query_heaps_kernel(hdata, ION_NUM_MAX_HEAPS);

	heap_id_mask = 0;
	for (i = 0; i < heap_num; i++) {
		if (hdata[i].type == ION_HEAP_TYPE_DMA) {
			heap_id_mask |= 1 << hdata[i].heap_id;
		}
	}
	kfree(hdata);

	return 0;
}

static unsigned long ion_dmabuf_get_phy(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct page *page;

	table = buffer->sg_table;
	page = sg_page(table->sgl);
	return (unsigned long)PFN_PHYS(page_to_pfn(page));
}

static int tzd_ion_shm_destroy(void)
{
	return 0;
}

static int tzd_ion_shm_alloc(struct tzd_shm *tzshm, gfp_t flags)
{
	void *alloc_addr;
	struct dma_buf *ion_dma_buf;

	ion_dma_buf = ion_alloc(tzshm->len, heap_id_mask, ION_FLAG_CACHED);
	if (IS_ERR(ion_dma_buf)) {
		tz_error("ion alloc failed, size=%d", tzshm->len);
		return -ENOMEM;
	}

	if (dma_buf_begin_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL) != 0) {
		dma_buf_put(ion_dma_buf);
		return -ENOMEM;
	}
	alloc_addr = dma_buf_kmap(ion_dma_buf, 0);
	if (alloc_addr == NULL) {
		tz_error("ion buffer kmap failed.");
		dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(ion_dma_buf);
		return -ENOMEM;
	}

	tzshm->k_addr = alloc_addr;
	tzshm->u_addr = NULL;
	tzshm->userdata = (void *)ion_dma_buf;
	tzshm->p_addr = (void *)ion_dmabuf_get_phy(ion_dma_buf);

	return 0;
}

static int tzd_ion_shm_free(struct tzd_shm *tzshm)
{
	if (tzshm->userdata) {
		struct dma_buf *ion_dma_buf = tzshm->userdata;
		dma_buf_kunmap(ion_dma_buf, 0, tzshm->k_addr);
		dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(ion_dma_buf);
	}
	return 0;
}

static const struct shm_ops ion_shm_ops = {
	.init = tzd_ion_shm_init,
	.malloc = tzd_ion_shm_alloc,
	.free = tzd_ion_shm_free,
	.destroy = tzd_ion_shm_destroy,
};
#endif /* CONFIG_ION */

static int tzd_kmem_shm_init(void)
{
	return 0;
}

static int tzd_kmem_shm_destroy(void)
{
	return 0;
}

static int tzd_kmem_shm_alloc(struct tzd_shm *tzshm, gfp_t flags)
{
	void *alloc_addr;

	alloc_addr = (void *)__get_free_pages(flags,
			get_order(roundup(tzshm->len, PAGE_SIZE)));
	if (unlikely(!alloc_addr)) {
		tz_error("get free pages failed");
		return -ENOMEM;
	}

	tzshm->k_addr = alloc_addr;
	tzshm->u_addr = NULL;
	tzshm->p_addr = (void *)virt_to_phys(alloc_addr);

	return 0;
}

static int tzd_kmem_shm_free(struct tzd_shm *tzshm)
{
	if (likely(tzshm && tzshm->k_addr))
		free_pages((unsigned long)tzshm->k_addr,
			get_order(roundup(tzshm->len, PAGE_SIZE)));
	return 0;
}

static const struct shm_ops kmem_shm_ops = {
	.init = tzd_kmem_shm_init,
	.malloc = tzd_kmem_shm_alloc,
	.free = tzd_kmem_shm_free,
	.destroy = tzd_kmem_shm_destroy,
};

#ifdef CONFIG_ION
# define DEFAULT_TZ_SHM_OPS	(&ion_shm_ops)
#else
# define DEFAULT_TZ_SHM_OPS	(&kmem_shm_ops)
#endif

static const struct shm_ops *tz_shm_ops = DEFAULT_TZ_SHM_OPS;

struct tzd_shm *tzd_shm_new(struct tzd_dev_file *dev, size_t size, gfp_t flags)
{
	struct tzd_shm *shm;

	if (unlikely(!dev || !size))
		return NULL;

	shm = kzalloc(sizeof(*shm), flags);
	if (unlikely(!shm))
		return NULL;
	shm->len = size;
	if (unlikely(tz_shm_ops->malloc(shm, flags) < 0)) {
		tz_error("shm_malloc failure on size (%zu)", size);
		kfree(shm);
		return NULL;
	}

	mutex_lock(&dev->tz_mutex);
	list_add_tail(&shm->head, &dev->dev_shm_head.shm_list);
	dev->dev_shm_head.shm_cnt++;
	mutex_unlock(&dev->tz_mutex);

	return shm;
}

void *tzd_shm_alloc(struct tzd_dev_file *dev, size_t size, gfp_t flags)
{
	struct tzd_shm *shm = tzd_shm_new(dev, size, flags);
	if (!shm)
		return NULL;
	return shm->p_addr;
}

int tzd_shm_free(struct tzd_dev_file *dev, const void *x)
{
	struct tzd_shm *shm, *next;

	if (unlikely(!dev))
		return -ENODEV;

	if (unlikely(!x))
		return -EFAULT;

	mutex_lock(&dev->tz_mutex);
	list_for_each_entry_safe(shm, next,
			&dev->dev_shm_head.shm_list, head) {
		if (shm->p_addr == x) {
			list_del(&shm->head);
			mutex_unlock(&dev->tz_mutex);
			if (likely(shm->k_addr))
				tz_shm_ops->free(shm);
			kfree(shm);
			return 0;
		}
	}
	mutex_unlock(&dev->tz_mutex);

	return 0;
}

int tzd_shm_delete(struct tzd_dev_file *dev)
{
	struct tzd_shm *shm, *next;

	if (unlikely(!dev))
		return -ENODEV;

	mutex_lock(&dev->tz_mutex);
	list_for_each_entry_safe(shm, next,
			&dev->dev_shm_head.shm_list, head) {
		list_del(&shm->head);
		if (likely(shm->k_addr))
			tz_shm_ops->free(shm);
		kfree(shm);
	}
	mutex_unlock(&dev->tz_mutex);

	return 0;
}

int tzd_shm_mmap(struct tzd_dev_file *dev, struct vm_area_struct *vma)
{
	struct tzd_shm *shm, *next;
	unsigned long pgoff;
	unsigned long map_len = vma->vm_end - vma->vm_start;
	int ret = -EINVAL;

	if (unlikely(!dev))
		return -ENODEV;

	mutex_lock(&dev->tz_mutex);
	list_for_each_entry_safe(shm, next,
			&dev->dev_shm_head.shm_list, head) {
		pgoff = (unsigned long)shm->p_addr >> PAGE_SHIFT;
		if ((pgoff == vma->vm_pgoff) && (map_len == PAGE_ALIGN(shm->len))){
			shm->u_addr = (void *)((unsigned long)vma->vm_start);
			ret = 0;
			break;
		}
	}
	mutex_unlock(&dev->tz_mutex);

	return ret;
}

void *tzd_shm_phys_to_virt_nolock(struct tzd_dev_file *dev, void *pa)
{
	struct tzd_shm *shm, *next;

	if (unlikely(!dev))
		return NULL;

	list_for_each_entry_safe(shm, next,
			&dev->dev_shm_head.shm_list, head) {
		if ((pa < shm->p_addr + shm->len) && (pa >= shm->p_addr))
			return shm->k_addr + (pa - shm->p_addr);
	}

	return NULL;
}

void *tzd_shm_phys_to_virt(struct tzd_dev_file *dev, void *pa)
{
	void *va;

	if (unlikely(!dev))
		return NULL;
	mutex_lock(&dev->tz_mutex);
	va = tzd_shm_phys_to_virt_nolock(dev, pa);
	mutex_unlock(&dev->tz_mutex);

	return va;
}

#ifdef CONFIG_ION
#define DEFAULT_SHM_TYPE	"ion"
#else
#define DEFAULT_SHM_TYPE	"kmem"
#endif

#define MAX_TYPE_LENGTH 8

static char shm_type[MAX_TYPE_LENGTH] = DEFAULT_SHM_TYPE;
#ifdef CONFIG_ION
module_param_string(shm_type, shm_type, sizeof (shm_type), 0);
MODULE_PARM_DESC(shm_type,
	"ion: use ion shared mem; kmem: use kmem shared mem");
#endif

int tzd_shm_init(void)
{
#ifdef CONFIG_ION
	if (!strncmp(shm_type, "ion", 3))
		tz_shm_ops = &ion_shm_ops;
	else
#endif
	if (!strncmp(shm_type, "kmem", 4))
		tz_shm_ops = &kmem_shm_ops;

	if (tz_shm_ops->init)
		return tz_shm_ops->init();

	return 0;
}

int tzd_shm_exit(void)
{
	return tz_shm_ops->destroy();
}
