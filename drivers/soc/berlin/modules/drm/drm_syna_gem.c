// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */
#include <drm/drmP.h>
#include <linux/dma-buf.h>

#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/capability.h>
#include <linux/ion.h>
#include <drm/drm_mm.h>
#include "drm_syna_drv.h"
#include "drm_syna_gem.h"
#include "syna_drm.h"

#define ION_AF_GFX_NONSECURE      (ION_A_NS | ION_A_FC)

int nonSecurePoolID;

static unsigned long ion_dmabuf_get_phy(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct page *page;

	table = buffer->sg_table;
	page = sg_page(table->sgl);
	return (unsigned long)PFN_PHYS(page_to_pfn(page));
}

int syna_gem_init(void)
{
	struct ion_heap_data *hdata;
	int heap_num, i;

	hdata = kmalloc(sizeof(*hdata) * ION_NUM_MAX_HEAPS, GFP_KERNEL);
	if (!hdata) {
		DRM_ERROR("%s alloc mem for ion_heap_data failed\n", __func__);
		return -1;
	}

	heap_num = ion_query_heaps_kernel(hdata, ION_NUM_MAX_HEAPS);

	for (i = 0; i < heap_num; i++) {
		if (!strcmp(hdata[i].name, "CMA-reserved"))
			nonSecurePoolID = 1 << hdata[i].heap_id;
	}

	kfree(hdata);

	if (nonSecurePoolID == -1) {
		DRM_ERROR("Can't find CMA-reserved %d", nonSecurePoolID);
		return -1;
	}

	return 0;
}

void syna_gem_deinit(void)
{
	DRM_DEBUG_PRIME("%s\n", __FUNCTION__);
}

static void syna_gem_free_buf(struct syna_gem_object *syna_obj)
{
	dma_buf_kunmap(syna_obj->dma_buf, 0, syna_obj->kernel_vir_addr);
	dma_buf_end_cpu_access(syna_obj->dma_buf, DMA_BIDIRECTIONAL);
	dma_buf_put(syna_obj->dma_buf);
}

void syna_gem_object_free_priv(struct drm_gem_object *obj)
{
	struct syna_gem_object *syna_obj;

	if (!obj) {
		DRM_ERROR("%s %d  obj is NULL!!\n", __func__, __LINE__);
		return;
	}
	syna_obj = to_syna_obj(obj);
	if (!syna_obj) {
		DRM_ERROR("%s %d  syna obj is NULL!!\n", __func__, __LINE__);
		return;
	}
	drm_gem_free_mmap_offset(obj);

	syna_gem_free_buf(syna_obj);

	drm_gem_object_release(&syna_obj->base);
	kfree(syna_obj);
}

/*
 * Allocate a sg_table for this GEM object.
 * Note: Both the table's contents, and the sg_table itself must be freed by
 *       the caller.
 * Returns a pointer to the newly allocated sg_table, or an ERR_PTR() error.
 */
struct sg_table *syna_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct syna_gem_object *syna_obj = to_syna_obj(obj);
	struct sg_table *sgt;
	int ret;

	if (!syna_obj) {
		DRM_ERROR("%s %d  syna obj is NULL!!\n", __func__, __LINE__);
		return ERR_PTR(-EINVAL);
	}
	if (syna_obj->phyaddr == 0)
		return ERR_PTR(-EINVAL);

	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt) {
		DRM_ERROR("failed to allocate sgt\n");
		return ERR_PTR(-ENOMEM);
	}

	ret = sg_alloc_table(sgt, 1, GFP_KERNEL);
	if (ret) {
		DRM_ERROR("failed to allocate sgt, %d\n", ret);
		kfree(sgt);
		return ERR_PTR(ret);
	}

	sg_set_page(sgt->sgl, pfn_to_page(PFN_DOWN(syna_obj->phyaddr)),
		    obj->size, 0);

	return sgt;
}

static int syna_gem_alloc_buf(struct syna_gem_object *syna_obj, bool alloc_kmap)
{
	struct drm_gem_object *obj = &syna_obj->base;

	if (!obj) {
		DRM_ERROR("%s %d  obj is NULL!!\n", __func__, __LINE__);
		return -EINVAL;
	}

	syna_obj->dma_attrs = DMA_ATTR_WRITE_COMBINE;

	if (!alloc_kmap)
		syna_obj->dma_attrs |= DMA_ATTR_NO_KERNEL_MAPPING;

	syna_obj->dma_buf = ion_alloc(obj->size, nonSecurePoolID, ION_NONCACHED);
	if (IS_ERR(syna_obj->dma_buf)) {
		DRM_ERROR("%s %d  ion alloc failed!\n", __func__, __LINE__);
		return -ENOMEM;
	}

	syna_obj->phyaddr = ion_dmabuf_get_phy(syna_obj->dma_buf);

	if (dma_buf_begin_cpu_access(syna_obj->dma_buf, DMA_FROM_DEVICE) != 0) {
		DRM_ERROR("%s:%d dma_buf_begin_cpu_access fail\n",
		       __func__, __LINE__);
		goto err;
	}

	syna_obj->kernel_vir_addr = dma_buf_kmap(syna_obj->dma_buf, 0);
	if(IS_ERR(syna_obj->kernel_vir_addr)){
		DRM_ERROR("%s %d dma buf kmap failed!\n", __func__, __LINE__);
		goto err;
	}

	DRM_DEBUG_PRIME("Alloc memory for (%d) %lx\n", current->pid,
			syna_obj->phyaddr);

	return 0;

err:
	if (!IS_ERR_OR_NULL(syna_obj->dma_buf)) {
		if (syna_obj->kernel_vir_addr) {
			dma_buf_kunmap(syna_obj->dma_buf, 0, syna_obj->kernel_vir_addr);
			dma_buf_end_cpu_access(syna_obj->dma_buf, DMA_FROM_DEVICE);
		}
		dma_buf_put(syna_obj->dma_buf);
	}
	return -ENOMEM;

}

static struct drm_gem_object *syna_gem_create_object(struct drm_device *drm,
					      unsigned int size,
					      bool alloc_kmap)
{
	struct syna_gem_object *syna_obj;
	struct drm_gem_object *obj;
	int ret;

	size = round_up(size, PAGE_SIZE);

	syna_obj = kzalloc(sizeof(*syna_obj), GFP_KERNEL);
	if (!syna_obj) {
		DRM_ERROR("%s %d  fail to allocate syna obj!!\n",
			  __func__, __LINE__);
		return ERR_PTR(-ENOMEM);
	}

	obj = &syna_obj->base;

	drm_gem_private_object_init(drm, obj, size);

	ret = syna_gem_alloc_buf(syna_obj, 0);
	if (ret) {
		DRM_ERROR("%s %d  fail to allocate buffer!!\n",
			  __func__, __LINE__);
		goto err_free_syna_obj;
	}

	return &syna_obj->base;

err_free_syna_obj:
	kfree(syna_obj);
	return ERR_PTR(ret);
}

int syna_gem_dumb_create(struct drm_file *file,
			 struct drm_device *dev,
			 struct drm_mode_create_dumb *args)
{
	struct drm_gem_object *obj;
	u32 handle;
	u32 pitch;
	size_t size;
	int err;

	pitch = args->width * (ALIGN(args->bpp, 8) >> 3);
	size = PAGE_ALIGN(pitch * args->height);

	obj = syna_gem_create_object(dev, size, 0);
	if (IS_ERR(obj)) {
		DRM_ERROR("%s:%d syna_gem_create_object fail\n",
			__func__, __LINE__);
		return PTR_ERR(obj);
	}

	err = drm_gem_handle_create(file, obj, &handle);
	if (err) {
		DRM_ERROR("%s %d drm_gem_handle_create fail\n",
				__func__, __LINE__);
		syna_gem_object_free_priv(obj);
		return err;
	}

	args->handle = handle;
	args->pitch = pitch;
	args->size = size;
	drm_gem_object_put_unlocked(obj);
	return 0;
}

int syna_drm_gem_object_mmap(struct drm_gem_object *obj,
			     struct vm_area_struct *vma)
{
	int ret;
	size_t size;
	struct syna_gem_object *syna_obj = to_syna_obj(obj);

	if (!syna_obj) {
		DRM_ERROR("%s:%d syna obj is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}
	vma->vm_flags &= ~VM_PFNMAP;

	size = vma->vm_end - vma->vm_start;

	ret = remap_pfn_range(vma,
			      vma->vm_start,
			      (syna_obj->phyaddr >> PAGE_SHIFT) + vma->vm_pgoff,
			      size, vma->vm_page_prot);
	if (ret) {
		DRM_ERROR("%s:%d Map fail\n", __func__, __LINE__);
		return -EAGAIN;
	}

	DRM_DEBUG_PRIME("%s %d PID(%d)  Map %lx to %lx\n", __func__,
			__LINE__, current->pid, syna_obj->phyaddr,
			vma->vm_start);

	return ret;
}

int syna_gem_mmap_buf(struct drm_gem_object *obj, struct vm_area_struct *vma)
{
	int ret;

	ret = drm_gem_mmap_obj(obj, obj->size, vma);
	if (ret) {
		DRM_ERROR("%s:%d drm_gem_mmap_obj fail!!\n",
			  __func__, __LINE__);
		return ret;
	}

	return syna_drm_gem_object_mmap(obj, vma);
}

int syna_gem_dumb_map_offset(struct drm_file *file,
			     struct drm_device *dev,
			     uint32_t handle, uint64_t *offset)
{
	struct drm_gem_object *obj = NULL;
	int err = 0;

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(file, handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		goto exit_unlock;
	}

	err = drm_gem_create_mmap_offset(obj);
	if (err)
		goto exit_obj_unref;

	*offset = drm_vma_node_offset_addr(&obj->vma_node);
	DRM_DEBUG_PRIME("%s (PID:%d) map handle(%d) to %llx\n", __func__,
		 current->pid, handle, *offset);
exit_obj_unref:
	drm_gem_object_put_unlocked(obj);
exit_unlock:
	mutex_unlock(&dev->struct_mutex);
	return err;
}

u64 syna_gem_get_dev_addr(struct drm_gem_object *obj)
{
	struct syna_gem_object *syna_obj = to_syna_obj(obj);

	if (!syna_obj) {
		DRM_ERROR("%s:%d syna obj is NULL!!\n", __func__, __LINE__);
		return 0;
	}
	return syna_obj->dev_addr;
}

int syna_gem_object_create_ioctl_priv(struct drm_device *dev,
				      void *data, struct drm_file *file)
{
	struct drm_syna_gem_create *args = data;
	struct drm_gem_object *obj;
	int err;

	if (args->flags) {
		DRM_ERROR("invalid flags: %#08x\n", args->flags);
		return -EINVAL;
	}

	if (args->handle) {
		DRM_ERROR("invalid handle (this should always be 0)\n");
		return -EINVAL;
	}

	obj = syna_gem_create_object(dev, PAGE_ALIGN(args->size), args->flags);
	if (IS_ERR(obj))
		return PTR_ERR(obj);

	err = drm_gem_handle_create(file, obj, &args->handle);
	if (err) {
		DRM_ERROR("%s %d drm_gem_handle_create fail\n",
			__func__, __LINE__);
		syna_gem_object_free_priv(obj);
		return err;
	}

	drm_gem_object_put_unlocked(obj);
	return err;
}

int syna_gem_object_mmap_ioctl(struct drm_device *dev, void *data,
			       struct drm_file *file)
{
	struct drm_syna_gem_mmap *args = (struct drm_syna_gem_mmap *)data;

	if (args->pad) {
		DRM_ERROR("invalid pad (this should always be 0)\n");
		return -EINVAL;
	}

	if (args->offset) {
		DRM_ERROR("invalid offset (this should always be 0)\n");
		return -EINVAL;
	}

	return syna_gem_dumb_map_offset(file, dev, args->handle, &args->offset);
}

int syna_gem_object_cpu_prep_ioctl(struct drm_device *dev, void *data,
				   struct drm_file *file)
{
	struct drm_syna_gem_cpu_prep *args =
	    (struct drm_syna_gem_cpu_prep *)data;
	struct drm_gem_object *obj;
	struct syna_gem_object *syna_obj;
	int err = 0;

	if (args->flags & ~(SYNA_GEM_CPU_PREP_READ |
				SYNA_GEM_CPU_PREP_WRITE |
				SYNA_GEM_CPU_PREP_NOWAIT)) {
		DRM_ERROR("invalid flags: %#08x\n", args->flags);
		return -EINVAL;
	}

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(file, args->handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		goto exit_unlock;
	}

	syna_obj = to_syna_obj(obj);

	if (!syna_obj) {
		err = -EINVAL;
		goto exit_unref;
	}
	if (syna_obj->cpu_prep) {
		err = -EBUSY;
		goto exit_unref;
	}

	if (!err)
		syna_obj->cpu_prep = true;

exit_unref:
	drm_gem_object_put_unlocked(obj);
exit_unlock:
	mutex_unlock(&dev->struct_mutex);
	return err;
}

int syna_gem_object_cpu_fini_ioctl(struct drm_device *dev, void *data,
				   struct drm_file *file)
{
	struct drm_syna_gem_cpu_fini *args =
	    (struct drm_syna_gem_cpu_fini *)data;
	struct drm_gem_object *obj;
	struct syna_gem_object *syna_obj;
	int err = 0;

	if (args->pad) {
		DRM_ERROR("invalid pad (this should always be 0)\n");
		return -EINVAL;
	}

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(file, args->handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		goto exit_unlock;
	}

	syna_obj = to_syna_obj(obj);

	if (!syna_obj || !syna_obj->cpu_prep) {
		err = -EINVAL;
		goto exit_unref;
	}

	syna_obj->cpu_prep = false;

exit_unref:
	drm_gem_object_put_unlocked(obj);
exit_unlock:
	mutex_unlock(&dev->struct_mutex);
	return err;
}
