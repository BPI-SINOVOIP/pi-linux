// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */

#ifndef _DRV_ISP_DHUB_SEMA_H_
#define _DRV_ISP_DHUB_SEMA_H_

#include "drv_ispdhub_prv.h"

#define SEMAPHORE_CFG	0x0
#define SEMAPHORE_INTR	0x4

#define SEMA_CPU_DELTA			8
#define SEMAINTR_MASK_EMPTY		1
#define SEMAINTR_MASK_FULL		(1 << 1)
#define SEMAINTR_MASK_ALMOST_EMPTY	(1 << 2)
#define SEMAINTR_MASK_ALMOST_FULL	(1 << 3)

#define ISPDHUB_GET_SEMAPHORE_OFFSET(hDhubCtx, sem_id, offset) \
	hDhubCtx->ctl.cell_offset + (sem_id * hDhubCtx->ctl.cell_size) + offset

void ispdhub_semaphore_pop(void *hdl, int id, int delta);
unsigned int ispdhub_semaphore_chk_full(void *hdl, int id);
void ispdhub_semaphore_clr_full(void *hdl, int id);
void ispdhub_semaphore_disable_intr(ISP_DHUB_CTX *hDhubCtx, int id);
void ispdhub_semaphore_enable_intr(ISP_DHUB_CTX *hDhubCtx, int id);
void ispdhub_semaphore_config(ISP_DHUB_CTX *hDhubCtx);

void ispdhub_semaphore_suspend(ISP_DHUB_CTX *hDhubCtx);
void ispdhub_semaphore_resume(ISP_DHUB_CTX *hDhubCtx);

#endif //_DRV_ISP_DHUB_SEMA_H_
