// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */

#if !defined(__DRM_SYNA_GEM_H__)
#define __DRM_SYNA_GEM_H__

#include <linux/version.h>
#include <linux/dma-buf.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/capability.h>
#include <drm/drmP.h>
#include <drm/drm_mm.h>
#include <drm/drm_gem.h>

#define ion_phys_addr_t unsigned long

struct syna_gem_object {
	struct drm_gem_object base;
	unsigned int flags;
	int buffer_index;
	unsigned long dma_attrs;

	atomic_t pg_refcnt;
	struct page **pages;
	dma_addr_t *addrs;

	bool cpu_prep;
	struct sg_table *sgt;
	phys_addr_t cpu_addr;
	dma_addr_t dev_addr;
	struct dma_buf *dma_buf;
	ion_phys_addr_t phyaddr;
	void *kernel_vir_addr;
};

#define to_syna_obj(obj) container_of(obj, struct syna_gem_object, base)

/* ioctl functions */
int syna_gem_object_create_ioctl_priv(struct drm_device *dev,
				      void *data, struct drm_file *file);
int syna_gem_object_mmap_ioctl(struct drm_device *dev, void *data,
			       struct drm_file *file);
int syna_gem_object_cpu_prep_ioctl(struct drm_device *dev, void *data,
				   struct drm_file *file);
int syna_gem_object_cpu_fini_ioctl(struct drm_device *dev, void *data,
				   struct drm_file *file);

/* drm driver functions */
void syna_gem_object_free_priv(struct drm_gem_object *obj);

int syna_gem_dumb_create(struct drm_file *file,
			 struct drm_device *dev,
			 struct drm_mode_create_dumb *args);
int syna_gem_mmap_buf(struct drm_gem_object *obj, struct vm_area_struct *vma);

int syna_gem_dumb_map_offset(struct drm_file *file, struct drm_device *dev,
			     uint32_t handle, uint64_t *offset);

/* internal interfaces */
u64 syna_gem_get_dev_addr(struct drm_gem_object *obj);
struct sg_table *syna_gem_prime_get_sg_table(struct drm_gem_object *obj);
int syna_gem_init(void);
void syna_gem_deinit(void);
int syna_drm_gem_object_mmap(struct drm_gem_object *obj,
			     struct vm_area_struct *vma);

#endif /* !defined(__DRM_SYNA_GEM_H__) */
