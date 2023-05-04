// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#include <linux/slab.h>
#include "vpp_api.h"
#include "vpp_defines.h"

static void MV_VPP_DsiCopyFrameBuffer(VBUF_INFO *vbufinfo, mrvl_vpp_dev *vdev, int y,
			int width, int height, int stride, int bpp)
{
	int i, j;
	unsigned char *ptr = vdev->ionvaddr;
	unsigned int *last_row = (uint32_t *)(vbufinfo->m_bufferID + (height * (width - 1) * bpp));
	uint32_t *src_p = (uint32_t *)(ptr + y * stride);
	uint32_t *dst_p;

	for (i = 0; i < height; i++) {
		dst_p = last_row + i;
		for (j = 0; j < width; j++, src_p++) {
			*dst_p = *src_p;
			dst_p -= height;
		}
	}
}

void MV_VPP_CopyFrameBuffer(VBUF_INFO *vbufinfo, mrvl_vpp_dev *vdev, int y,
			int width, int height, int stride, int bpp, int disp_width, int disp_height)
{
	if (disp_width > disp_height) {
		unsigned char *ptr = vdev->ionvaddr;

		memcpy(vbufinfo->m_bufferID, ptr + y * stride, height * stride);
	} else {
		MV_VPP_DsiCopyFrameBuffer(vbufinfo, vdev, y, width, height, stride, bpp);
	}
}


