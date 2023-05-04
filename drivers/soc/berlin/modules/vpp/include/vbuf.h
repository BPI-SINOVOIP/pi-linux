// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef _VBUF_H_
#define _VBUF_H_

#include "vpp_vbuf.h"

typedef struct vbuf_info_t {
	unsigned int m_grp_alloc_flag;
	unsigned int m_buf_size;
	unsigned int m_flags;
	unsigned int m_srcfmt;
	unsigned int m_order;
	unsigned int m_bytes_per_pixel;
	unsigned int m_content_offset;
	unsigned int m_content_width;
	unsigned int m_content_height;
	int m_active_left;
	int m_active_top;
	unsigned int m_active_width;
	unsigned int m_active_height;
	int m_disp_offset;
	unsigned int m_buf_stride;
	void *m_bufferID;
	uintptr_t m_pbuf_start;
	void *hShm;
	VPP_VBUF *pVppVbufInfo_virt;
	VPP_VBUF *pVppVbufInfo_phy;
} VBUF_INFO;
#endif //_VBUF_H_
