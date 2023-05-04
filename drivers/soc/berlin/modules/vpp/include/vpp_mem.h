// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef _VPP_MEM_H_
#define _VPP_MEM_H_

typedef enum {
	FB_PATTERN_CHECKER = 0,
	FB_PATTERN_VERT_COLORBAR,
	FB_PATTERN_CUSTOM,
	FB_PATTERN_CUSTOM_APM,
	MAX_PATTERNS,
	FB_PATTERN_HORZ_COLORBAR,
} FB_PATTERN_TYPE;

typedef enum {
	FB_ION_HEAP_CACHE,
	FB_ION_HEAP_NON_CACHE,
	MAX_FB_ION_HEAP_TYPE,
} FB_ION_HEAP_TYPE;

int MV_VPP_MapMemory(mrvl_vpp_dev *vdev,
			struct vm_area_struct *vma);
int MV_VPP_InitMemory(struct device *fb_dev);
void MV_VPP_DeinitMemory(void);
int MV_VPP_AllocateMemory(mrvl_vpp_dev *vdev, int height, int stride);
void MV_VPP_FreeMemory(mrvl_vpp_dev *vdev);
phys_addr_t VPP_ion_dmabuf_get_phy(struct dma_buf *dmabuf);

#endif //_VPP_MEM_H_
