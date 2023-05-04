// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include <linux/slab.h>

#include "vpp_defines.h"
#include "vpp_api.h"
#include "vpp_mem.h"

#define NW_SHM_POOL_ATTR	(ION_A_FC | ION_A_NS)
#define NC_SHM_POOL_ATTR	(ION_A_NC | ION_A_NS)

static struct device *berlin_ion_dev;
static unsigned int heap_id_mask;
static unsigned int heap_id_mask_nc;

static int VPP_get_heap_mask_by_attr(unsigned int heap_attr, unsigned int *heap_id_mask)
{
	int heap_num, i;
	struct ion_heap_data *hdata;
	unsigned int heap_type;

	hdata = kmalloc(sizeof(*hdata) * ION_NUM_MAX_HEAPS, GFP_KERNEL);
	if (!hdata) {
		pr_err("%s alloc mem failed\n", __func__);
		return -ENOMEM;
	}

	heap_type = ((heap_attr & NW_SHM_POOL_ATTR) == NW_SHM_POOL_ATTR) ?
			ION_HEAP_TYPE_DMA : ION_HEAP_TYPE_BERLIN_NC;
	heap_num = ion_query_heaps_kernel(hdata, ION_NUM_MAX_HEAPS);

	*heap_id_mask = 0;
	for (i = 0; i < heap_num; i++) {
		if (hdata[i].type == heap_type) {
			*heap_id_mask |= 1 << hdata[i].heap_id;
		}
	}
	kfree(hdata);

	return MV_VPP_OK;
}

phys_addr_t VPP_ion_dmabuf_get_phy(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct page *page;

	table = buffer->sg_table;
	page = sg_page(table->sgl);
	return (phys_addr_t)PFN_PHYS(page_to_pfn(page));
}

static int VPP_InitIonMem(int index)
{
	int ret;

	ret = VPP_get_heap_mask_by_attr(NW_SHM_POOL_ATTR, &heap_id_mask);
	if (ret < 0) {
		pr_err("vpp_driver: VPP_get_heap_mask_by_attr failed\n");
		return ret;
	}
	return MV_VPP_OK;
}

static int VPP_InitIonMem_NonCached(int index)
{
	int ret;

	ret = VPP_get_heap_mask_by_attr(NC_SHM_POOL_ATTR, &heap_id_mask_nc);
	if (ret < 0) {
		pr_err("vpp_driver: VPP_get_heap_mask_by_attr failed\n");
		return ret;
	}
	return MV_VPP_OK;
}

static void VPP_DeinitIonMem(int index)
{
}

int VPP_AllocateMemory(void **shm_handle, unsigned int type, unsigned int size,
	unsigned int align)
{
	struct dma_buf *ion_dma_buf;

	type = VPP_SHM_VIDEO_FB;
	ion_dma_buf = ion_alloc(size, heap_id_mask, ION_FLAG_CACHED);

	if (IS_ERR(ion_dma_buf)) {
		pr_err("vpp_mem: ion alloc failed, size=%d", size);
		return MV_VPP_ENOMEM;
	}

	if (dma_buf_begin_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL) != 0) {
		pr_err("vpp_mem: dma_buf_begin_cpu_access failed\n");
		dma_buf_put(ion_dma_buf);
		return -ENOMEM;
	}
	*shm_handle = (void *)ion_dma_buf;
	return MV_VPP_OK;
}

int VPP_AllocateMemory_NC(void **shm_handle, unsigned int type, unsigned int size,
	unsigned int align)
{
	struct dma_buf *ion_dma_buf;

	type = VPP_SHM_VIDEO_FB;
	ion_dma_buf = ion_alloc(size, heap_id_mask_nc, 0);

	if (IS_ERR(ion_dma_buf)) {
		pr_err("vpp_mem: ion alloc failed, size=%d", size);
		return MV_VPP_ENOMEM;
	}

	if (dma_buf_begin_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL) != 0) {
		pr_err("vpp_mem: dma_buf_begin_cpu_access failed\n");
		dma_buf_put(ion_dma_buf);
		return -ENOMEM;
	}
	*shm_handle = (void *)ion_dma_buf;
	return MV_VPP_OK;
}

int VPP_Memory_for_ta(bool bAllocate, unsigned int *pPhyAddr)
{
	int res = 0;
	static phys_addr_t pVppMem_phy = 0;
	struct dma_buf *ion_dma_buf = NULL;
	void *shm_handle_vbuf = NULL;
	void *virt_ptr = NULL;
	unsigned int phys_ptr = 0;

	if (bAllocate) {
		if (pVppMem_phy) {
			*pPhyAddr = pVppMem_phy;
		} else {
			res = VPP_AllocateMemory_NC(&shm_handle_vbuf, VPP_SHM_VIDEO_FB,
								SHM_SHARE_SZ, PAGE_SIZE);
			if (res == MV_VPP_ENOMEM) {
				pr_err("%s:%d: Failed to allocate memory NC\n", __func__, __LINE__);
				return res;
			}
			ion_dma_buf = (struct dma_buf *) shm_handle_vbuf;
			virt_ptr = dma_buf_kmap(ion_dma_buf, 0);
			if (IS_ERR(virt_ptr)) {
				res = MV_VPP_ENOMEM;
				pr_err("%s:%d: Failed to get virtual address NC\n", __func__, __LINE__);
				return res;
			}
			phys_ptr = (unsigned int) VPP_ion_dmabuf_get_phy(ion_dma_buf);
			if (phys_ptr == 0x0) {
				res = MV_VPP_ENOMEM;
				pr_err("%s:%d: Failed to get physical address NC\n", __func__, __LINE__);
				return res;
			}
			*pPhyAddr = (unsigned int) phys_ptr;
			pr_debug("%s:%d:NC:  virt: 0x%llx, phys: 0x%llx\n, pPhyAddr: 0x%x",
				__func__, __LINE__,(unsigned long long) virt_ptr,
				(unsigned long long) phys_ptr, (unsigned int) *pPhyAddr);
			pVppMem_phy = *pPhyAddr;
		}
	} else if (pVppMem_phy) {
		if (shm_handle_vbuf) {
			ion_dma_buf = (struct dma_buf *) shm_handle_vbuf;
			if (virt_ptr)
				dma_buf_kunmap(ion_dma_buf, 0, virt_ptr);
			dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
			dma_buf_put(ion_dma_buf);
		}
	}

	return res;
}

unsigned int VPP_SHM_MALLOC(unsigned int	eMemType, unsigned int  uiSize, unsigned int  uiAlign,
					 SHM_HANDLE  *phShm)
{
	struct dma_buf *ion_dma_buf;

	ion_dma_buf = ion_alloc(uiSize, heap_id_mask, ION_FLAG_CACHED);

	if (IS_ERR(ion_dma_buf)) {
		pr_err("vpp_mem: ion alloc failed, size=%d", uiSize);
		return MV_VPP_ENOMEM;
	}

	if (dma_buf_begin_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL) != 0) {
		pr_err("vpp_mem: dma_buf_begin_cpu_access failed\n");
		dma_buf_put(ion_dma_buf);
		return -ENOMEM;
	}
	*phShm = (void *)ion_dma_buf;
	return MV_VPP_OK;
}

int VPP_SHM_GetVirtualAddress(SHM_HANDLE phShm, unsigned int offset,
		unsigned int **virt)
{
	struct dma_buf *ion_dma_buf = (struct dma_buf *) phShm;
	*virt = NULL;
	*virt = dma_buf_kmap(ion_dma_buf, 0);
	if (*virt == NULL) {
		pr_err("vpp_mem: ion buffer kmap failed.");
		return MV_VPP_EBADCALL;
	}
	*virt = *virt + offset;
	return MV_VPP_OK;
}

int VPP_SHM_GetPhysicalAddress(SHM_HANDLE  phShm, unsigned int offset,
		uintptr_t *phy)
{
	struct dma_buf *ion_dma_buf = (struct dma_buf *) phShm;

	phy = (void *)VPP_ion_dmabuf_get_phy(ion_dma_buf);
	*phy = *phy + offset;
	return MV_VPP_OK;
}

void FlushCache(dma_addr_t phys_start, unsigned int size)
{
	dma_sync_single_for_device(berlin_ion_dev, phys_start,
				size, DMA_TO_DEVICE);
	dma_sync_single_for_cpu(berlin_ion_dev, phys_start,
				size, DMA_FROM_DEVICE);
}

unsigned int VPP_SHM_CleanCache(uintptr_t phy_addr, unsigned int size)
{
	FlushCache((dma_addr_t)phy_addr, size);
	return 0;
}

unsigned int VPP_SHM_Free(SHM_HANDLE  phShm, unsigned int *virt)
{
	if (virt) {
		struct dma_buf *ion_dma_buf = (struct dma_buf *) phShm;

		dma_buf_kunmap(ion_dma_buf, 0, virt);
		dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(ion_dma_buf);
	}
	return 0;
}

/* ION MEMORY APIs */
int MV_VPP_MapMemory(mrvl_vpp_dev *vdev,
		  struct vm_area_struct *vma)
{
	size_t size;

	size = vma->vm_end - vma->vm_start;
	if (remap_pfn_range(vma,
				vma->vm_start,
				(vdev->ionpaddr >> PAGE_SHIFT) +
				vma->vm_pgoff, size,
				vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

int MV_VPP_InitMemory(struct device *fb_dev)
{
	int ret;
	berlin_ion_dev = fb_dev;

	ret = VPP_InitIonMem(0);
	if (ret != MV_VPP_OK) {
		pr_err("VPP_InitIonMem FAILED ..!!!\n");
		return ret;
	}

	ret = VPP_InitIonMem_NonCached(0);
	if (ret != MV_VPP_OK) {
		pr_err("VPP_InitIonMem_NonCached FAILED ..!!!\n");
		return ret;
	}

	return MV_VPP_OK;
}

void MV_VPP_DeinitMemory(void)
{
	VPP_DeinitIonMem(0);
}

int MV_VPP_AllocateMemory(mrvl_vpp_dev *vdev, int height, int stride)
{
	int retval;
	void *shm = NULL;
	struct dma_buf *ion_dma_buf;

	vdev->phy_size = 0;

	retval = VPP_AllocateMemory(&shm, VPP_SHM_VIDEO_FB,
				(stride * height), PAGE_SIZE);
	if (retval != MV_VPP_OK) {
		pr_err("VPP_AllocateMemory failed\n");
		return MV_VPP_ENOMEM;
	}

	ion_dma_buf = (struct dma_buf *) shm;
	vdev->ionvaddr = dma_buf_kmap(ion_dma_buf, 0);
	if (IS_ERR_OR_NULL(vdev->ionvaddr)) {
		pr_err("ion memory mapping failed - %ld\n",
				PTR_ERR(vdev->ionvaddr));
		goto err;
	}
	vdev->ionhandle = shm;
	vdev->ionpaddr = VPP_ion_dmabuf_get_phy(ion_dma_buf);
	if (vdev->ionpaddr == 0) {
		pr_err("get physical address failed %x\n", retval);
		goto err;
	}
	return MV_VPP_OK;

err:
	if (shm) {
		ion_dma_buf = (struct dma_buf *) shm;
		if (vdev->ionvaddr)
			dma_buf_kunmap(ion_dma_buf, 0, vdev->ionvaddr);
		dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(ion_dma_buf);
	}
	return MV_VPP_ENOMEM;
}

void MV_VPP_FreeMemory(mrvl_vpp_dev *vdev)
{
	if (!IS_ERR_OR_NULL(vdev->ionhandle)) {
		struct dma_buf *ion_dma_buf = (struct dma_buf *) vdev->ionhandle;

		if (!IS_ERR_OR_NULL(vdev->ionvaddr))
			dma_buf_kunmap(ion_dma_buf, 0, vdev->ionvaddr);
		dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(ion_dma_buf);
	}

	vdev->ionhandle = NULL;
}

unsigned int VPP_FreeMemory_Gen(void *handle, unsigned int *virt)
{
	if (!IS_ERR_OR_NULL(handle)) {
		struct dma_buf *ion_dma_buf = (struct dma_buf *) handle;

		if (virt)
			dma_buf_kunmap(ion_dma_buf, 0, virt);
		dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		dma_buf_put(ion_dma_buf);
	}
	return 0;
}

unsigned int VPP_AllocateMemory_Gen(unsigned int  uiSize, unsigned int  uiAlign, FB_ION_HEAP_TYPE ionHeapType, void **pHandle, unsigned int **virt, phys_addr_t *phy)
{
	struct dma_buf *ion_dma_buf = NULL;
	void *alloc_addr = NULL;

	ion_dma_buf = ion_alloc(uiSize, heap_id_mask, ION_FLAG_CACHED);

	if (IS_ERR(ion_dma_buf)) {
		pr_err("ion alloc failed, size=%d", uiSize);
		return MV_VPP_EBADCALL;
	}

	if (dma_buf_begin_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL) != 0) {
		pr_err("failed dma_buf_begin_cpu_access\n");
		goto err;
	}

	alloc_addr = dma_buf_kmap(ion_dma_buf, 0);

	if (alloc_addr == NULL) {
		pr_err("ion buffer kmap failed.");
		goto err;
	}

	*virt = alloc_addr;

	phy = (void *)VPP_ion_dmabuf_get_phy(ion_dma_buf);

	return MV_VPP_OK;

err:
	if (!IS_ERR_OR_NULL(ion_dma_buf)) {
		if (alloc_addr) {
			dma_buf_kunmap(ion_dma_buf, 0, alloc_addr);
			dma_buf_end_cpu_access(ion_dma_buf, DMA_BIDIRECTIONAL);
		}
		dma_buf_put(ion_dma_buf);
	}
	return MV_VPP_EBADCALL;
}

