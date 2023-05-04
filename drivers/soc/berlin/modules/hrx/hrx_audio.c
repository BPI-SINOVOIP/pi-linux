// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */
#include "drv_hrx.h"

#define AIP_SOUECE_SPDIF 1

static void *AIPFifoGetKernelPreRdDMAInfo(
	struct aip_dma_cmd_fifo *p_aip_cmd_fifo, INT pair)
{
	void *pHandle;
	INT rd_offset = p_aip_cmd_fifo->kernel_pre_rd_offset;

	if (rd_offset > p_aip_cmd_fifo->size || rd_offset < 0) {
		INT i = 0, fifo_cmd_size = sizeof(struct aip_dma_cmd_fifo) >> 2;
		INT *temp = (INT *) p_aip_cmd_fifo;
		(void)temp;
		hrx_trace("rd_offset = %d fifo_cmd_size = %d :\n",
			rd_offset, fifo_cmd_size);

		hrx_trace("memory %p is corrupted! corrupted data :\n",
			  p_aip_cmd_fifo);
		for (i = 0; i < fifo_cmd_size; i++)
			hrx_trace("0x%x\n", *temp++);

		rd_offset = 0;
	}
	pHandle = &(p_aip_cmd_fifo->aip_dma_cmd[pair][rd_offset]);
	return pHandle;
}

static void AIPFifoKernelPreRdUpdate(
	struct aip_dma_cmd_fifo *p_aip_cmd_fifo, INT adv)
{
	p_aip_cmd_fifo->kernel_pre_rd_offset += adv;
	p_aip_cmd_fifo->kernel_pre_rd_offset %= p_aip_cmd_fifo->size;
}

static void AIPFifoKernelRdUpdate(
	struct aip_dma_cmd_fifo *p_aip_cmd_fifo, INT adv)
{
	p_aip_cmd_fifo->kernel_rd_offset += adv;
	p_aip_cmd_fifo->kernel_rd_offset %= p_aip_cmd_fifo->size;
}

static INT AIPFifoCheckKernelFullness(struct aip_dma_cmd_fifo *p_aip_cmd_fifo)
{
	INT full;

	full = p_aip_cmd_fifo->wr_offset - p_aip_cmd_fifo->kernel_pre_rd_offset;
	if (full < 0)
		full += p_aip_cmd_fifo->size;
	return full;
}

void aip_start_cmd(struct hrx_priv *hrx, INT *aip_info, void *param)
{
	INT *p = aip_info;
	INT chanId;
	HDL_dhub *dhub = NULL;
	struct aip_dma_cmd *p_dma_cmd;
	struct aip_dma_cmd_fifo *pCmdFifo = NULL;
	UINT32 rc;

	if (!hrx) {
		hrx_error("%s: null handler!\n", __func__);
		return;
	}

	hrx->aip_source = aip_info[2];
	hrx->p_aip_cmdfifo = pCmdFifo = (struct aip_dma_cmd_fifo *) param;

	pCmdFifo = hrx->p_aip_cmdfifo;
	if (!pCmdFifo) {
		hrx_trace("%s: p_aip_fifo is NULL\n", __func__);
		return;
	}

	dhub = hrx->dhub;
	if (*p == 1) {
		hrx->aip_i2s_pair = 1;
		p_dma_cmd = (struct aip_dma_cmd *)
			AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, 0);

		if (hrx->aip_source == AIP_SOUECE_SPDIF) {
			/* To be added */
		} else {
			chanId = avioDhubChMap_aio64b_MIC3_CH_W;
			rc = dhub_channel_write_cmd(dhub, chanId,
				p_dma_cmd->addr0,
				p_dma_cmd->size0, 0, 0, 0, 1, 0, 0);

			AIPFifoKernelPreRdUpdate(hrx->p_aip_cmdfifo, 1);

			/* push 2nd dHub command */
			p_dma_cmd =	(struct aip_dma_cmd *)
				AIPFifoGetKernelPreRdDMAInfo(
							hrx->p_aip_cmdfifo, 0);
			rc = dhub_channel_write_cmd(dhub, chanId,
				p_dma_cmd->addr0,
				p_dma_cmd->size0, 0, 0, 0, 1, 0, 0);
			AIPFifoKernelPreRdUpdate(hrx->p_aip_cmdfifo, 1);
		}
	} else if (*p == 4) {
		UINT pair;

		hrx->aip_i2s_pair = 4;
		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd = (struct aip_dma_cmd *)
				AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
			chanId = avioDhubChMap_aio64b_MIC3_CH_W +
			pair;
			dhub_channel_write_cmd(dhub, chanId,
				p_dma_cmd->addr0,
				p_dma_cmd->size0, 0, 0, 0, 1, 0,
				0);
		}

		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);

		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd = (struct aip_dma_cmd *)
				AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
			chanId = avioDhubChMap_aio64b_MIC3_CH_W +
						pair;
			dhub_channel_write_cmd(dhub, chanId,
				p_dma_cmd->addr0,
				p_dma_cmd->size0, 0, 0, 0, 1, 0,
				0);
		}
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
	}
}

void aip_stop_cmd(struct hrx_priv *hrx)
{
	if (!hrx) {
		hrx_error("%s: null handler!\n", __func__);
		return;
	}
	hrx->p_aip_cmdfifo = NULL;
}

void aip_resume_cmd(struct hrx_priv *hrx)
{
	struct aip_dma_cmd *p_dma_cmd;
	HDL_dhub *dhub = NULL;
	UINT chanId;
	INT pair;
	struct aip_dma_cmd_fifo *pCmdFifo;

	if (!hrx) {
		hrx_error("%s: null handler!\n", __func__);
		return;
	}

	pCmdFifo = hrx->p_aip_cmdfifo;
	if (!pCmdFifo) {
		hrx_trace("%s::p_aip_fifo is NULL\n", __func__);
		return;
	}

	spin_lock(&hrx->aip_spinlock);

	if (!pCmdFifo->fifo_overflow)
		AIPFifoKernelRdUpdate(pCmdFifo, 1);

	dhub = hrx->dhub;

	if (AIPFifoCheckKernelFullness(pCmdFifo)) {
		pCmdFifo->fifo_overflow = 0;
		for (pair = 0; pair < hrx->aip_i2s_pair; pair++) {
			p_dma_cmd = (struct aip_dma_cmd *)
				AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
			if (hrx->aip_source == AIP_SOUECE_SPDIF) {
			/* To be added */
			} else {
				chanId = avioDhubChMap_aio64b_MIC3_CH_W	+ pair;
				dhub_channel_write_cmd(dhub, chanId,
					p_dma_cmd->addr0,
					p_dma_cmd->size0, 0, 0,
					0,
					p_dma_cmd->addr1 ? 0 : 1,
					0, 0);
				if (p_dma_cmd->addr1) {
					dhub_channel_write_cmd(dhub,
						chanId,
						p_dma_cmd->addr1,
						p_dma_cmd->size1,
						0, 0, 0, 1, 0,
						0);
				}
			}
		}
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
	} else {
		pCmdFifo->fifo_overflow = 1;
		pCmdFifo->fifo_overflow_cnt++;
		hrx_trace("%s: fifo_overflow = %d fifo_overflow_cnt = %d\n",
					__func__, pCmdFifo->fifo_overflow,
					pCmdFifo->fifo_overflow_cnt);
		for (pair = 0; pair < hrx->aip_i2s_pair; pair++) {
			/* FIXME:
			 *chanid should be changed if 4 pair is supported
			 */
			if (hrx->aip_source == AIP_SOUECE_SPDIF) {
			/* To be added */
			} else {
				chanId = avioDhubChMap_aio64b_MIC3_CH_W	+ pair;
				dhub_channel_write_cmd(dhub, chanId,
					pCmdFifo->overflow_buffer,
					pCmdFifo->overflow_buffer_size, 0,
					0, 0, 1, 0, 0);
			}
		}
	}

	spin_unlock(&hrx->aip_spinlock);
}
