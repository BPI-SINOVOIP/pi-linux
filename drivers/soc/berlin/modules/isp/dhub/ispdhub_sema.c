// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */

#include "ispdhub_sema.h"

#define IS_DHUB_ID_VALID(DHUB_ID) (DHUB_ID < ISP_DHUB_ID_MAX)

#define ISP_DHUB_GET_DHUB_BASE(DHUBCTX) \
	DHUBCTX->dhub_base[DHUBCTX->dhub_id]

#define ISP_DHUB_GET_SRAM_BASE(DHUBCTX) \
	DHUBCTX->sram_base[DHUBCTX->dhub_id]

static inline void ispdhub_reg_write(const ISP_DHUB_CTX *hDhubCtx, unsigned int offset, unsigned int value)
{
	if (IS_DHUB_ID_VALID(hDhubCtx->dhub_id))
		writel_relaxed(value, ISP_DHUB_GET_DHUB_BASE(hDhubCtx) + offset);
}

static inline unsigned int ispdhub_reg_read(const ISP_DHUB_CTX *hDhubCtx, unsigned int offset)
{
	unsigned int reg_val = 0;

	if (IS_DHUB_ID_VALID(hDhubCtx->dhub_id))
		reg_val = readl_relaxed(ISP_DHUB_GET_DHUB_BASE(hDhubCtx) + offset);

	return reg_val;
}

#ifdef ISPDHUB_GET_SEMAPHORE_OFFSET
static inline void ispdhub_semaphore_reg_write(const ISP_DHUB_CTX *hDhubCtx,
				      unsigned int idx, unsigned int offset, unsigned int val)
{
	unsigned int final_offset;

	if ((idx < ISP_DHUB_INTR_HANDLER_MAX) &&
		IS_DHUB_ID_VALID(hDhubCtx->dhub_id)) {
		final_offset = ISPDHUB_GET_SEMAPHORE_OFFSET(hDhubCtx, idx, offset);
		ispdhub_reg_write(hDhubCtx, final_offset, val);
	}
}

static inline unsigned int ispdhub_semaphore_reg_read(const ISP_DHUB_CTX *hDhubCtx,
				    unsigned int idx, unsigned int offset)
{
	unsigned int final_offset;
	unsigned int reg_val = 0;

	if ((idx < ISP_DHUB_INTR_HANDLER_MAX) &&
		IS_DHUB_ID_VALID(hDhubCtx->dhub_id)) {
		final_offset = ISPDHUB_GET_SEMAPHORE_OFFSET(hDhubCtx, idx, offset);
		reg_val = ispdhub_reg_read(hDhubCtx, final_offset);
	}

	return reg_val;
}
#else //!ISPDHUB_GET_SEMAPHORE_OFFSET
static inline void ispdhub_semaphore_reg_write(const ISP_DHUB_CTX *hDhubCtx,
				      unsigned int idx, unsigned int offset, unsigned int val)
{
	unsigned int final_offset;

	if ((idx < ISP_DHUB_INTR_HANDLER_MAX) &&
		IS_DHUB_ID_VALID(hDhubCtx->dhub_id)) {
		final_offset = hDhubCtx->ctl.cell_offset + idx * hDhubCtx->ctl.cell_size + offset;
		writel_relaxed(val, ISP_DHUB_GET_DHUB_BASE(hDhubCtx) + final_offset);
	}
}


static inline unsigned int ispdhub_semaphore_reg_read(const ISP_DHUB_CTX *hDhubCtx,
				    unsigned int idx, unsigned int offset)
{
	unsigned int final_offset;
	unsigned int reg_val = 0;

	if ((idx < ISP_DHUB_INTR_HANDLER_MAX) &&
		IS_DHUB_ID_VALID(hDhubCtx->dhub_id)) {
		final_offset = hDhubCtx->ctl.cell_offset + idx * hDhubCtx->ctl.cell_size + offset;
		reg_val = readl_relaxed(ISP_DHUB_GET_DHUB_BASE(hDhubCtx) + final_offset);
	}

	return reg_val;
}
#endif //ISPDHUB_GET_SEMAPHORE_OFFSET

void ispdhub_semaphore_pop(void *hdl, int id, int delta) {
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX*)hdl;
	unsigned int ack = (1 << SEMA_CPU_DELTA) | id;

	/* CPU pop semaphore */
	ispdhub_reg_write(hDhubCtx, hDhubCtx->ctl.pop_offset, ack);
}

unsigned int ispdhub_semaphore_chk_full(void *hdl, int id) {
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX*)hdl;
	unsigned int stat;

	stat = ispdhub_reg_read(hDhubCtx, hDhubCtx->ctl.full_offset);

	return stat;
}

void ispdhub_semaphore_clr_full(void *hdl, int id) {
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX*)hdl;
	unsigned int mask = (1 << id);

	/* clear cell full status */
	ispdhub_reg_write(hDhubCtx, hDhubCtx->ctl.full_offset, mask);
}

void ispdhub_semaphore_disable_intr(ISP_DHUB_CTX *hDhubCtx, int id)
{
	unsigned int mask = 1 << id;

	hDhubCtx->cached_ictl_mask |= mask;
	ispdhub_semaphore_reg_write(hDhubCtx, id, SEMAPHORE_INTR, 0);
}

void ispdhub_semaphore_enable_intr(ISP_DHUB_CTX *hDhubCtx, int id)
{
	unsigned int mask = 1 << id;

	hDhubCtx->cached_ictl_mask &= ~mask;
	ispdhub_semaphore_reg_write(hDhubCtx, id, SEMAPHORE_INTR, SEMAINTR_MASK_FULL);
}

void ispdhub_semaphore_config(ISP_DHUB_CTX *hDhubCtx)
{
	int i;
	unsigned int stat;
	unsigned int full_offset;

	/* config and disable all interrupts */
	for (i = 0; i < ISP_DHUB_INTR_HANDLER_MAX; i++) {
		/* set semaphore depth to 1 */
		ispdhub_semaphore_reg_write(hDhubCtx, i, SEMAPHORE_CFG, 1);
		ispdhub_semaphore_reg_write(hDhubCtx, i, SEMAPHORE_INTR, 0);
	}
	/* clear status if any */
	full_offset = hDhubCtx->ctl.full_offset;
	stat = ispdhub_reg_read(hDhubCtx, full_offset);
	if (stat)
		ispdhub_reg_write(hDhubCtx, full_offset, stat);
	hDhubCtx->cached_ictl_mask = ~0;
}

#ifdef CONFIG_PM
static void ispdhub_semaphore_save_load_context(ISP_DHUB_CTX *hDhubCtx,
					      bool save)
{
	int i;

	for (i = 0; i < ISP_DHUB_INTR_HANDLER_MAX; i++) {
		if (save) {
			/* depth is alwasy 1, no need to save */
			hDhubCtx->intr_context[i] =
				ispdhub_semaphore_reg_read(hDhubCtx, i, SEMAPHORE_INTR);
		} else {
			ispdhub_semaphore_reg_write(hDhubCtx, i, SEMAPHORE_CFG, 1);
			ispdhub_semaphore_reg_write(hDhubCtx, i, SEMAPHORE_INTR,
					   hDhubCtx->intr_context[i]);
		}
	}
}

void ispdhub_semaphore_suspend(ISP_DHUB_CTX *hDhubCtx)
{
	ispdhub_semaphore_save_load_context(hDhubCtx, true);
}

void ispdhub_semaphore_resume(ISP_DHUB_CTX *hDhubCtx)
{
	ispdhub_semaphore_save_load_context(hDhubCtx, false);
}
#else
void ispdhub_semaphore_suspend(ISP_DHUB_CTX *hDhubCtx)
{
}

void ispdhub_semaphore_resume(ISP_DHUB_CTX *hDhubCtx)
{
}
#endif /* CONFIG_PM */
