// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/jiffies.h>

#include "drv_aout.h"
#include "aio_hal.h"
#include "avio_common.h"
#include "aio.h"

#define CPU_ID 0
#define APLL_RATE_32K (16384000 * 8)
#define APLL_RATE_44_1K (22579200 * 8)
#define APLL_RATE_48K (24576000 * 8)
#define INVALID_CHAN_ID (avioDhubChMap_aio64b_SPDIF_W + 1)

#define MAX_CMD_PARAMETER_NUMBER 6
struct function_table {
	char *name;
	int number;
	union {
		void *f;
		int (*f0)(struct aout_priv *aout);
		int (*f1)(struct aout_priv *aout, u32 p1);
		int (*f2)(struct aout_priv *aout, u32 p1, u32 p2);
		int (*f3)(struct aout_priv *aout, u32 p1, u32 p2, u32 p3);
		int (*f4)(struct aout_priv *aout, u32 p1, u32 p2, u32 p3, u32 p4);
		int (*f5)(struct aout_priv *aout, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5);
		int (*f6)(struct aout_priv *aout, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6);
	};
};
static int aout_help(struct aout_priv *aout);
static u8 aout_irq_name[MAX_OUTPUT_AUDIO][64];
static u32 spdif_apll_id = 1; /* APLL id being used. 0: AIO_APLL_0, 1: AIO_APLL_1 */
static u32 hdmi_apll_id = 1;  /* APLL id being used. 0: AIO_APLL_0, 1: AIO_APLL_1 */

#define FUNC_ITEM(func, num) \
		{.name = (#func), .number = (num), .f = (void *)(func)}

static const char *get_path_name(u32 path_id)
{
	const char *name[MAX_OUTPUT_AUDIO] = {"ma0", "loro", "spdif", "hdmi"};

	if (path_id > MAX_OUTPUT_AUDIO)
		return NULL;
	return name[path_id];
}

static inline struct aout_priv *get_aout_priv(struct device *dev)
{
	return (struct aout_priv *)dev_get_drvdata(dev);
}

static int get_dhub_space(struct aout_priv *aout, int chanId)
{
	return hbo_queue_getspace(&aout->dhub->hbo,
		dhub_id2hbo_cmdQ(chanId));
}

static int get_dhub_depth(struct aout_priv *aout, int chanId)
{
	return hbo_queue_getdepth(&aout->dhub->hbo,
			dhub_id2hbo_cmdQ(chanId));
}

static u32 channel_write(struct aout_priv *aout,
					s32 id,
					u32 addr,
					s32 size,
					s32 semOnMTU,
					s32 chkSemId,
					s32 updSemId,
					s32 interrupt,
					T64b cfgQ[],
					u32 *ptr)
{
	return dhub_channel_write_cmd(aout->dhub,
					 id, addr, size,
					 semOnMTU, chkSemId, updSemId,
					 interrupt, cfgQ, ptr);
}

static void *AoutFifoGetKernelRdDMAInfoByIndex(AOUT_PATH_CMD_FIFO
				*p_aout_cmd_fifo, int pair)
{
	void *pHandle;

	pHandle =
		&(p_aout_cmd_fifo->aout_dma_info[pair]
		  [p_aout_cmd_fifo->aout_pre_read]);

	return pHandle;
}

static void *AoutFifoGetKernelRdDMAInfo(AOUT_PATH_CMD_FIFO
				*p_aout_cmd_fifo, int pair)
{
	void *pHandle;
	int rd_offset = p_aout_cmd_fifo->kernel_rd_offset;

	if (rd_offset > p_aout_cmd_fifo->size || rd_offset < 0) {
		int i = 0, fifo_cmd_size = sizeof(AOUT_PATH_CMD_FIFO) >> 2;
		int *temp = (int *) p_aout_cmd_fifo;

		aout_trace
			("AOUT FIFO memory %p is corrupted! corrupted data :\n",
			 p_aout_cmd_fifo);
		for (i = 0; i < fifo_cmd_size; i++)
			aout_trace("0x%x\n", *temp++);
		rd_offset = 0;
	}
	pHandle = &(p_aout_cmd_fifo->aout_dma_info[pair][rd_offset]);

	return pHandle;
}

static void
AoutFifoKernelRdUpdate(AOUT_PATH_CMD_FIFO *p_aout_cmd_fifo, int adv)
{
	p_aout_cmd_fifo->kernel_rd_offset += adv;
	if (p_aout_cmd_fifo->kernel_rd_offset >= p_aout_cmd_fifo->size)
		p_aout_cmd_fifo->kernel_rd_offset -= p_aout_cmd_fifo->size;
}

static int AoutFifoCheckKernelFullness(
	AOUT_PATH_CMD_FIFO *p_aout_cmd_fifo)
{
	int full;

	full = p_aout_cmd_fifo->wr_offset - p_aout_cmd_fifo->kernel_rd_offset;
	if (full < 0)
		full += p_aout_cmd_fifo->size;
	return full;
}

static int AoutFifoCheckKernelPreFullness(AOUT_PATH_CMD_FIFO *p_aout_cmd_fifo)
{
	int full;

	full = p_aout_cmd_fifo->wr_offset - p_aout_cmd_fifo->aout_pre_read;
	if (full < 0)
		full += p_aout_cmd_fifo->size;
	return full;
}

static u32 aout_map_path(struct aout_priv *aout, int irq)
{
	u32 i;

	for (i = 0; i < MAX_OUTPUT_AUDIO; i++)
		if (irq == aout->ap[i].irq)
			return i;

	return MAX_OUTPUT_AUDIO;
}

static inline u32 get_chanId(struct aout_priv *aout, u32 channel)
{
	return irqd_to_hwirq(irq_get_irq_data(channel));
}

static void aout_start_cmd(struct aout_priv *aout,
				int *aout_info, void *param)
{
	int path_id = *aout_info;
	int chanId = 0, chanId0 = 0;
	AOUT_DMA_INFO *p_dma_info;
	AOUT_PATH_CMD_FIFO *p_path_fifo = NULL;

	if (path_id >= MAX_OUTPUT_AUDIO) {
		aout_error("path_id == %d > MAX_OUTPUT_AUDIO!\n", path_id);
		return;
	}

	if (!aout->ap[path_id].en) {
		aout_trace("path %d disabled\n", path_id);
		return;
	}
	aout_trace("path %d start\n", path_id);

	p_path_fifo = (AOUT_PATH_CMD_FIFO *)param;
	if (p_path_fifo->aout_pre_read != 0) {
		aout_trace("path %d: ", path_id);
		aout_trace("aout_pre_read = %d ",
					p_path_fifo->aout_pre_read);
		aout_trace("kener_rd_offset:%d rd_offset:%d wr_offset:%d\n",
		p_path_fifo->kernel_rd_offset,
		p_path_fifo->rd_offset,
		p_path_fifo->wr_offset);

		p_path_fifo->aout_pre_read = 0;
	}
	p_path_fifo->aout_pre_read++;
	p_path_fifo->aout_pre_read %= p_path_fifo->size;

	aout->ap[path_id].cmd_fifo = (AOUT_PATH_CMD_FIFO *)param;
	chanId0 = get_chanId(aout, aout->ap[path_id].irq);
	p_dma_info = (AOUT_DMA_INFO *) AoutFifoGetKernelRdDMAInfo(
			aout->ap[path_id].cmd_fifo, 0);
	chanId = chanId0;
	channel_write(aout, chanId,
			p_dma_info->addr0,
			p_dma_info->size0, 0, 0, 0,
					   1, 0, 0);
}

static void aout_stop_cmd(struct aout_priv *aout, int path_id)
{
	if (path_id >= MAX_OUTPUT_AUDIO) {
		aout_error("path_id == %d > MAX_OUTPUT_AUDIO!\n", path_id);
		return;
	}
	aout->ap[path_id].cmd_fifo  = NULL;
	aout_trace("%s(%d)\n", __func__, path_id);
}

static inline AOUT_DMA_INFO *get_dma_info(
	AOUT_PATH_CMD_FIFO *p_path_fifo,
	u32 index)
{
	return (AOUT_DMA_INFO *)
		AoutFifoGetKernelRdDMAInfoByIndex(p_path_fifo, index);
}

static void write_pre_full(struct aout_priv *aout,
	 u32 path_id,
	 u32 chanId,
	 int kernel_pre_full,
	 AOUT_PATH_CMD_FIFO *fifo)
{
	unsigned int cnt;
	u32 chanId_real;
	unsigned int dhub_space;
	AOUT_DMA_INFO *p_dma_info;
	u32 total = kernel_pre_full;

	while (kernel_pre_full > 0) {
		chanId_real = chanId;
		dhub_space = get_dhub_space(aout, chanId_real);
		p_dma_info = get_dma_info(fifo, 0);
		if (p_dma_info->size1) {
			if (dhub_space < 2)
				goto exit;
		} else {
			if (dhub_space < 1)
				goto exit;
		}
		cnt = channel_write(aout,
				chanId_real,
				p_dma_info->addr0,
				p_dma_info->size0, 0, 0, 0,
				(p_dma_info->size1 == 0) ? 1 : 0,
				0, 0);
		if (p_dma_info->size1) {
			cnt = channel_write(aout,
					chanId_real,
					p_dma_info->addr1,
					p_dma_info->size1, 0, 0, 0,
					1, 0, 0);
			if (cnt == 0)
				pr_info("frame not alignedn");
		}

		if (cnt != 0) {
			kernel_pre_full--;
			fifo->aout_pre_read++;
			fifo->aout_pre_read %= fifo->size;
			fifo->fifo_underflow = 0;
		} else {
			pr_info("command buffer full\n");
			goto exit;
		}
	} //end while
exit:
	aout->ap[path_id].write_cnt += (total - kernel_pre_full);
}

static void write_zero_data(struct aout_priv *aout,
	u32 path_id,
	u32 chanId,
	AOUT_PATH_CMD_FIFO *fifo)
{
	unsigned int write_cmd_cnt;
	AOUT_DMA_INFO dma_info = { 0 };

	write_cmd_cnt = channel_write(aout,
			chanId,
			fifo->zero_buffer,
			fifo->zero_buffer_size, 0, 0,
			0, 1, 0, 0);
	dma_info.addr0 = fifo->zero_buffer;
	dma_info.size0 = fifo->zero_buffer_size;
}

static void aout_resume_cmd(struct aout_priv *aout, u32 path_id, u32 chanId)
{
	int kernel_pre_full, kernel_full;
	unsigned int dhub_space, dhub_depth;
	AOUT_PATH_CMD_FIFO *p_path_fifo = NULL;

	if (path_id >= MAX_OUTPUT_AUDIO) {
		aout_error("path_id == %d > MAX_OUTPUT_AUDIO!\n", path_id);
		return;
	}

	if (!aout) {
		aout_error("aout == NULL!\n");
		return;
	}

	if (!aout->ap[path_id].en) {
		aout_trace("path %d disabled\n", path_id);
		return;
	}

	aout->ap[path_id].int_cnt++;

	p_path_fifo = aout->ap[path_id].cmd_fifo;

	if (!p_path_fifo) {
		aout_trace("path_id=%d not exist", path_id);
		return;
	}

	kernel_pre_full = AoutFifoCheckKernelPreFullness(p_path_fifo);
	kernel_full = AoutFifoCheckKernelFullness(p_path_fifo);
	dhub_space = get_dhub_space(aout, chanId);
	dhub_depth = get_dhub_depth(aout, chanId);

	/*
	 * WORKAROUND:
	 * dHub completion IRQ doesn't arise as expected sometimes.
	 * Especially when there's many kernel log printing.
	 * Root cause isn't fully clear now and further investigation is needed.
	 */
	if (kernel_full > kernel_pre_full) {
		int unit_remain, cmd_dhub;

		/*
		 * "full > prefull"
		 * implies that valid (nonzero) samples have been
		 * pushed to dHub before this ISR.
		 * Advance the RD index by 1 anyway.
		 */
		AoutFifoKernelRdUpdate(p_path_fifo, 1);

		/*
		 * Check whether sync is lost between
		 * "Unit FIFO" and "dHub Queue".
		 * Normal Cases:
		 *  a, unit_remain == cmd_dhub     (No boundary unit exists)
		 *  b, unit_remain == cmd_dhub - 1 (boundary unit exists)
		 * Abnormal Cases:
		 *  c, unit_remain == cmd_dhub     (boundary unit exists)
		 *  d, unit_remain >  cmd_dhub
		 *
		 * "a" and "c" can NOT be distinguished,
		 * but it's OK to only deal with case "d"
		 * When the 2 consecutive dHub commands
		 * for boundary unit are consumed,
		 * case "d" will occur again
		 * And then the obseleted "Unit" will be advanced.
		 */
		unit_remain = kernel_full - kernel_pre_full - 1;
		cmd_dhub = dhub_depth - dhub_space;

		if (unit_remain > cmd_dhub)
			AoutFifoKernelRdUpdate(p_path_fifo,
				unit_remain - cmd_dhub);
	}

	if (kernel_pre_full) {
		write_pre_full(aout,
				path_id,
				chanId,
				kernel_pre_full,
				p_path_fifo);
	} else if (dhub_space >= dhub_depth) {
		p_path_fifo->fifo_underflow = 1;
		p_path_fifo->underflow_cnt++;
		write_zero_data(aout,
				path_id,
				chanId,
				p_path_fifo);
	}
}

static irqreturn_t devices_aout_isr(int irq, void *dev_id)
{
	u32 chanId;
	irq_hw_number_t hw_irq;
	struct aout_priv *aout = (struct aout_priv *) dev_id;
	HRESULT rc;
	int path_id;
	MV_CC_MSG_t msg = { 0, 0, 0 };

	hw_irq = irqd_to_hwirq(irq_get_irq_data(irq));
	chanId = hw_irq;
	path_id = aout_map_path(aout, irq);

	if (path_id < MAX_OUTPUT_AUDIO) {
		aout_resume_cmd(aout, path_id, chanId);
		msg.m_MsgID = 1 << chanId;
		rc = AMPMsgQ_Add(&aout->hAoutMsgQ, &msg);
		if (rc == S_OK)
			up(&aout->aout_sem);
	}
	return IRQ_HANDLED;
}

static int drv_request_irq(struct aout_priv *aout, u32 path)
{
	int err = 0;
	u32 irq;
	u8 *name;

	aout_enter_func();

	irq = aout->ap[path].irq;
	if (irq < 0)
		return -EINVAL;

	name = &aout_irq_name[path][0];
	snprintf(name, sizeof(aout_irq_name)/MAX_OUTPUT_AUDIO, "AUDIO DHUB %d",
		(u32)irqd_to_hwirq(irq_get_irq_data(irq)));
	err = request_irq(irq, devices_aout_isr,
			 IRQF_SHARED, name, aout);
	if (unlikely(err < 0))
		pr_err("register %s irq error", name);
	else
		pr_info("register %s irq succcessfully", name);
	return err;
}

static s32 user_getchannelfrompath(
	struct aout_priv *aout, u32 path)
{
	u32 irq = aout->ap[path].irq;

	if (path >= MAX_OUTPUT_AUDIO) {
		aout_error("undefined path %d\n", path);
		return INVALID_CHAN_ID;
	}

	if (irq >= 0)
		return get_chanId(aout, irq);
	else
		return INVALID_CHAN_ID;
}

static u32 AOUT_HAL_GetdHubChanId(struct aout_priv *aout,
	u32 aio_id, u32 tsd)
{
	u32 chanId = INVALID_CHAN_ID;

	switch (aio_id) {
	case AIO_ID_PRI_TX:
		chanId = user_getchannelfrompath(aout, MA0_PATH);
		chanId += tsd;
		break;
	case AIO_ID_SEC_TX:
		chanId = user_getchannelfrompath(aout, LoRo_PATH);
		break;
	case AIO_ID_SPDIF_TX:
		chanId = user_getchannelfrompath(aout, SPDIF_PATH);
		break;
	case AIO_ID_HDMI_TX:
		chanId = user_getchannelfrompath(aout, HDMI_PATH);
		break;
	default:
		aout_error("invalid aio ID %d\n", aio_id);
	}
	return chanId;
}

static int user_aout_hal_channelenable(struct aout_priv *aout,
			u32 path, u32 enable, u32 effect_pair)
{
	u32 chanId;
	u32 uiInstate;
	HDL_semaphore *pSemHandle = NULL;
	u32 aio_id;
	u32 tsd, tsd_start, tsd_end, mask;

	if (path == MA0_PATH) {
		aio_id = AIO_ID_PRI_TX;
		tsd_start = TSD_START;
		tsd_end = TSD_END;
	} else if (path == LoRo_PATH) {
		aio_id = AIO_ID_SEC_TX;
		tsd_start = TSD_START;
		tsd_end = TSD_START;
	} else if (path == SPDIF_PATH) {
		aio_id = AIO_ID_SPDIF_TX;
		tsd_start = TSD_START;
		tsd_end = TSD_START;
	} else if (path == HDMI_PATH) {
		aio_id = AIO_ID_HDMI_TX;
		tsd_start = TSD_START;
		tsd_end = TSD_START;
	} else {
		return -EINVAL;
	}

	for (tsd = tsd_start; tsd <= tsd_end; tsd++) {
		mask = 1 << tsd;
		if (!(effect_pair & mask))
			continue;

		chanId = AOUT_HAL_GetdHubChanId(aout, aio_id, tsd);
		if (chanId == INVALID_CHAN_ID)
			return -EINVAL;
		pSemHandle = dhub_semaphore(aout->dhub);
		/**> check and clear dhub channel interrupt state*/
		if (!enable) {
			uiInstate = semaphore_chk_full(pSemHandle, chanId);
			if (bTST(uiInstate, chanId)) {
				semaphore_pop(pSemHandle, chanId, 1);
				semaphore_clr_full(pSemHandle, chanId);
			}
		}
		/**< enable/disable interrupt*/
		semaphore_intr_enable(pSemHandle,
				  chanId, 0,
				  enable ? 1 : 0,
				  0,
				  0,
				  CPU_ID
			);
	}
	return 0;
}

static int user_aout_hal_clearintr(struct aout_priv *aout,
						u32 path, u32 pair)
{
	u32 chanId;
	HDL_dhub *dhub;
	HDL_semaphore *pSemHandle;
	u32 aio_id;
	u32 tsd;
	/* warning clear, init */
	aio_id = 0;
	if (path == MA0_PATH)
		aio_id = AIO_ID_PRI_TX;
	else if (path == LoRo_PATH)
		aio_id = AIO_ID_SEC_TX;
	else if (path == HDMI_PATH)
		aio_id = AIO_ID_HDMI_TX;
	else if (path == SPDIF_PATH)
		aio_id = AIO_ID_SPDIF_TX;
	else
		return -EINVAL;

	tsd = TSD_START + pair;
	chanId = AOUT_HAL_GetdHubChanId(aout, aio_id, tsd);
	if (chanId == INVALID_CHAN_ID)
		return -EINVAL;
	dhub = aout->dhub;
	pSemHandle = dhub_semaphore(dhub);
	semaphore_pop(pSemHandle, chanId, 1);
	semaphore_clr_full(pSemHandle, chanId);
	return 0;
}

static int user_aout_hal_cleardhubchannel(
	struct aout_priv *aout, u32 path, u32 effect_pair)
{
	u32 chanId;
	u32 aio_id;
	u32 tsd, tsd_start, tsd_end, mask;
	HDL_dhub *dhub = NULL;

	if (path == MA0_PATH) {
		aio_id = AIO_ID_PRI_TX;
		tsd_start = TSD_START;
		tsd_end = TSD_START;
	} else if (path == LoRo_PATH) {
		aio_id = AIO_ID_SEC_TX;
		tsd_start = TSD_START;
		tsd_end = TSD_START;
	} else if (path == SPDIF_PATH) {
		aio_id = AIO_ID_SPDIF_TX;
		tsd_start = TSD_START;
		tsd_end = TSD_START;
	} else if (path == HDMI_PATH) {
		aio_id = AIO_ID_HDMI_TX;
		tsd_start = TSD_START;
		tsd_end = TSD_START;
	} else {
		return -EINVAL;
	}

	dhub = aout->dhub;
	for (tsd = tsd_start; tsd <= tsd_end; tsd++) {
		mask = 1 << tsd;
		if (effect_pair & mask) {
			chanId = AOUT_HAL_GetdHubChanId(aout, aio_id, tsd);
			if (chanId == INVALID_CHAN_ID)
				continue;
			DhubChannelClear(dhub, chanId, NULL);
		}
	}
	return 0;
}

static int user_aout_hal_dhubdataquery(
	struct aout_priv *aout, u32 path)
{
	u32 chanId;
	HDL_dhub *dhub;
	u32 aio_id;
	u32 ret, ptr;
	/* warning clear, init */
	aio_id = 0;
	if (path == MA0_PATH)
		aio_id = AIO_ID_PRI_TX;
	else if (path == LoRo_PATH)
		aio_id = AIO_ID_SEC_TX;
	else if (path == HDMI_PATH)
		aio_id = AIO_ID_HDMI_TX;
	else if (path == SPDIF_PATH)
		aio_id = AIO_ID_SPDIF_TX;
	else
		return 0;

	chanId = AOUT_HAL_GetdHubChanId(aout, aio_id, 0);
	if (chanId == INVALID_CHAN_ID)
		return 0;
	dhub = aout->dhub;
	ret = dhub_data_query(dhub, chanId, 0, &ptr);
	return ret;
}

static int user_aio_set_apll(struct aout_priv *aout, u32 id, u32 fs)
{
	unsigned long apll;
	u32 apll_id;

	switch (fs) {
	case 11025:
	case 22050:
	case 44100:
	case 88200:
	case 176400:
		apll = APLL_RATE_44_1K;
		break;
	case 8000:
	case 16000:
	case 32000:
	case 64000:
		apll = APLL_RATE_32K;
		break;
	case 12000:
	case 24000:
	case 48000:
	case 96000:
	case 192000:
	case 384000:
		apll = APLL_RATE_48K;
		break;
	default:
		apll = APLL_RATE_48K;
		break;
	}

	switch (id) {
	default:
		pr_info("set id(%d) to fs (%d) not supported!", id, fs);
		return -EINVAL;
	case AIO_ID_PRI_TX:
	case AIO_ID_SEC_TX:
		apll_id = AIO_APLL_0;
		break;
	case AIO_ID_SPDIF_TX:
		apll_id = spdif_apll_id;
		break;
	case AIO_ID_HDMI_TX:
		apll_id = hdmi_apll_id;
		break;
	}

	aio_clk_enable(aout->aio_hd, apll_id, false);
	aio_set_clk_rate(aout->aio_hd, apll_id, apll);
	pr_info("id(%d) set apll%d to %lu, fs %d", id, apll_id, apll, fs);
	aio_clk_enable(aout->aio_hd, apll_id, true);
	return 0;
}

static int user_aio_enableprimaryport(struct aout_priv *aout, u32 enable)
{
	return aio_enabletxport(aout->aio_hd, AIO_ID_PRI_TX, enable);
}

static int user_aio_enabletxport(struct aout_priv *aout, u32 id, u32 enable)
{
	return aio_enabletxport(aout->aio_hd, id, enable);
}

static int user_aio_reset(struct aout_priv *aout)
{
	return aio_reset(aout->aio_hd);
}

static int user_aio_setclkdiv(struct aout_priv *aout, u32 id, u32 div)
{
	return aio_setclkdiv(aout->aio_hd, id, div);
}

static int user_aio_setspdifclk(struct aout_priv *aout, u32 div)
{
	return aio_setspdifclk(aout->aio_hd, div);
}

static int user_aio_setspdif_en(struct aout_priv *aout, u32 enable)
{
	return aio_setspdif_en(aout->aio_hd, enable);
}

static int user_aio_setirq(struct aout_priv *aout,
					u32 pri, u32 sec,
					u32 mic, u32 spdif, u32 hdmi)
{
	return aio_setirq(aout->aio_hd, pri, sec, mic, spdif, hdmi);
}

static int user_aio_getirqsts(struct aout_priv *aout)
{
	return aio_getirqsts(aout->aio_hd);
}

static int user_aio_selhdport(struct aout_priv *aout,
						 u32 sel)
{
	return aio_selhdport(aout->aio_hd, sel);
}

static int user_aio_selhdsource(struct aout_priv *aout,
						   u32 sel)
{
	return aio_selhdsource(aout->aio_hd, sel);
}

static int user_aio_setaudchmute(struct aout_priv *aout,
				u32 id, u32 tsd, u32 mute)
{
	aio_set_aud_ch_mute(aout->aio_hd, id, tsd, mute);
	return 0;
}

static int user_aio_setaudchen(struct aout_priv *aout,
				u32 id, u32 tsd, u32 enable)
{
	aio_set_aud_ch_en(aout->aio_hd, id, tsd, enable);
	return 0;
}

static int user_aio_setaudchflush(struct aout_priv *aout,
			 u32 id, u32 tsd, u32 on)
{
	aio_set_aud_ch_flush(aout->aio_hd, id, tsd, on);
	return 0;
}

static int user_aio_setctl(struct aout_priv *aout,
		 u32 id, u32 data_fmt, u32 width_word, u32 width_sample)
{
	aio_set_ctl(aout->aio_hd, id,
			data_fmt, width_word, width_sample);
	return 0;
}

static int user_aio_i2s_set_clock(struct aout_priv *aout,
	 u32 id, u32 clkSwitch, u32 clkD3Switch,
	 u32 clkSel, u32 pllUsed, u32 en)
{
	return aio_i2s_set_clock(aout->aio_hd, id, clkSwitch,
	 clkD3Switch, clkSel, pllUsed, en);
}

static int user_aio_set_data_fmt(struct aout_priv *aout, u32 id, u32 data_fmt)
{
	aio_set_data_fmt(aout->aio_hd, id, data_fmt);
	return 0;
}

static int user_aio_set_width_word(struct aout_priv *aout,
			 u32 id, u32 width_word)
{
	aio_set_width_word(aout->aio_hd, id, width_word);
	return 0;
}

static int user_aio_set_width_sample(struct aout_priv *aout,
			 u32 id, u32 width_sample)
{
	aio_set_width_sample(aout->aio_hd, id, width_sample);
	return 0;
}

static int user_aio_set_bclkinvert(struct aout_priv *aout, u32 id, u32 invclk)
{
	aio_set_invclk(aout->aio_hd, id, (bool)invclk);
	return 0;
}

static int user_aio_set_lrclkinvert(struct aout_priv *aout, u32 id, u32 invfs)
{
	aio_set_invfs(aout->aio_hd, id, (bool)invfs);
	return 0;
}

static int user_aio_set_tdm(struct aout_priv *aout,
			u32 id, u32 en, u32 chcnt, u32 wshigh)
{
	aio_set_tdm(aout->aio_hd, id, (bool)en, chcnt, wshigh);
	return 0;
}

static int user_aio_set_interleaved_mode(struct aout_priv *aout,
			 u32 id, u32 src, u32 ch_map)
{
	aio_set_interleaved_mode(aout->aio_hd, id, src, ch_map);
	return 0;
}

static void set_aio_clock(struct aout_priv *aout, u32 path)
{
	switch (path) {
	case MA0_PATH:
		aio_i2s_set_clock(aout->aio_hd, AIO_ID_PRI_TX,
			1, 0, 4, AIO_APLL_0, 1);
		break;
	case LoRo_PATH:
		aio_i2s_set_clock(aout->aio_hd, AIO_ID_SEC_TX,
			1, 0, 4, AIO_APLL_0, 1);
		break;
	case SPDIF_PATH:
		aio_i2s_set_clock(aout->aio_hd, AIO_ID_SPDIF_TX,
			1, 0, 4, spdif_apll_id, 1);
		break;
	case HDMI_PATH:
		aio_i2s_set_clock(aout->aio_hd, AIO_ID_HDMI_TX,
			1, 0, 4, hdmi_apll_id, 1);
		break;
	default:
		pr_err("unsuported path id %d\n", path);
		break;
	}
}

static int user_aout_setpath(struct aout_priv *aout,
		u32 p1, u32 p2, u32 p3, u32 p4)
{
	u8 i;
	int err = 0;
	const char *path_name;

	aout->ap[MA0_PATH].en = p1;
	aout->ap[LoRo_PATH].en = p2;
	aout->ap[SPDIF_PATH].en = p3;
	aout->ap[HDMI_PATH].en = p4;

	for (i = 0 ; i < MAX_OUTPUT_AUDIO; i++) {
		path_name = get_path_name(i);
		if (!path_name)
			continue;
		aout_trace("%s - %s\n", path_name,
			aout->ap[i].en ? "Enable":"Disable");

		if (aout->ap[i].en) {
			err = drv_request_irq(aout, i);
			if (err < 0) {
				aout->ap[i].en = 0;
				pr_err("%s request irq fail\n",
					path_name);
				goto error;
			}
			set_aio_clock(aout, i);
		}
	}
	return 0;
error:
	for (; i >= 0; i--)
		if (aout->ap[i].en) {
			aout->ap[i].en = 0;
			free_irq(aout->ap[i].irq, (void *) aout);
		}
	return err;
}

static int user_aio_enable_audio_timer(struct aout_priv *aout, u32 en)
{
	return aio_enable_audio_timer(aout->aio_hd, (bool)en);
}

static int user_aio_get_audio_timer(struct aout_priv *aout)
{
	u32 val;

	aio_get_audio_timer(aout->aio_hd, &val);
	return val;
}

static int user_aio_enable_sampinfo(struct aout_priv *aout, u32 idx, u32 en)
{
	return aio_enable_sampinfo(aout->aio_hd, idx, (bool)en);
}

static int user_aio_set_sampinfo_req(struct aout_priv *aout, u32 idx, u32 en)
{
	return aio_set_sampinfo_req(aout->aio_hd, idx, (bool)en);
}

static int user_aio_get_audio_counter(struct aout_priv *aout, u32 idx)
{
	u32 c;

	aio_get_audio_counter(aout->aio_hd, idx, &c);
	return c;
}

static int user_aio_get_audio_timestamp(struct aout_priv *aout, u32 idx)
{
	u32 t;

	aio_get_audio_timestamp(aout->aio_hd, idx, &t);
	return t;
}

static int user_aio_spdifi_sel(struct aout_priv *aout,
	u32 clk_sel, u32 data_sel)
{
	aio_spdifi_src_sel(aout->aio_hd, clk_sel, data_sel);
	return 0;
}

static int user_aio_spdifi_set_srm(struct aout_priv *aout, u32 idx, u32 margin)
{
	aio_spdifi_set_srm(idx, margin);
	return 0;
}

static int user_aio_spdifi_sw_reset(struct aout_priv *aout)
{
	aio_spdifi_sw_reset(aout->aio_hd);
	return 0;
}

static struct function_table f_table[] = {
	FUNC_ITEM(user_aout_hal_cleardhubchannel, 2),
	FUNC_ITEM(user_aout_hal_clearintr, 2),
	FUNC_ITEM(user_aout_hal_channelenable, 3),
	FUNC_ITEM(user_getchannelfrompath, 1),
	FUNC_ITEM(user_aout_hal_dhubdataquery, 1),
	FUNC_ITEM(user_aout_setpath, 4),
	FUNC_ITEM(user_aio_set_apll, 2),
	FUNC_ITEM(user_aio_setctl, 4),
	FUNC_ITEM(user_aio_reset, 0),
	FUNC_ITEM(user_aio_setclkdiv, 2),
	FUNC_ITEM(user_aio_setaudchen, 3),
	FUNC_ITEM(user_aio_setaudchmute, 3),
	FUNC_ITEM(user_aio_setaudchflush, 3),
	FUNC_ITEM(user_aio_selhdsource, 1),
	FUNC_ITEM(user_aio_selhdport, 1),
	FUNC_ITEM(user_aio_setirq, 5),
	FUNC_ITEM(user_aio_setspdifclk, 1),
	FUNC_ITEM(user_aio_setspdif_en, 1),
	FUNC_ITEM(user_aio_enableprimaryport, 1),
	FUNC_ITEM(user_aio_enabletxport, 2),
	FUNC_ITEM(user_aio_getirqsts, 0),
	FUNC_ITEM(user_aio_i2s_set_clock, 6),
	FUNC_ITEM(user_aio_set_data_fmt, 2),
	FUNC_ITEM(user_aio_set_width_word, 2),
	FUNC_ITEM(user_aio_set_width_sample, 2),
	FUNC_ITEM(user_aio_set_bclkinvert, 2),
	FUNC_ITEM(user_aio_set_lrclkinvert, 2),
	FUNC_ITEM(user_aio_set_tdm, 4),
	FUNC_ITEM(user_aio_set_interleaved_mode, 3),
	FUNC_ITEM(user_aio_enable_audio_timer, 1),
	FUNC_ITEM(user_aio_get_audio_timer, 0),
	FUNC_ITEM(user_aio_enable_sampinfo, 2),
	FUNC_ITEM(user_aio_set_sampinfo_req, 2),
	FUNC_ITEM(user_aio_get_audio_counter, 1),
	FUNC_ITEM(user_aio_get_audio_timestamp, 1),
	FUNC_ITEM(user_aio_spdifi_sel, 2),
	FUNC_ITEM(user_aio_spdifi_set_srm, 2),
	FUNC_ITEM(user_aio_spdifi_sw_reset, 0),
	FUNC_ITEM(aout_help, 0),
};

static int aout_help(struct aout_priv *aout)
{
	int i;

	pr_info("fucntion name, parameters\n");
	pr_info("-------------|-----------\n");
	for (i = 0; i < ARRAY_SIZE(f_table); i++)
		if (f_table[i].f)
			pr_info("%s, %d\n", f_table[i].name, f_table[i].number);
		else
			break;
	pr_info("-------------|-----------\n");
	return 0;
}

static inline bool eq_ignore_case(char c1, char c2)
{
	if (c1 <= 90 && c1 >= 65)
		c1 = c1 + 32;

	if (c2 <= 90 && c2 >= 65)
		c2 = c2 + 32;

	return c1 == c2;
}

static bool sysfs_streq_ignorecase(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2 || eq_ignore_case(*s1, *s2))) {
		s1++;
		s2++;
	}

	if (*s1 == *s2)
		return true;
	if (!*s1 && *s2 == '\n' && !s2[1])
		return true;
	if (*s1 == '\n' && !s1[1] && !*s2)
		return true;
	return false;
}

static int aio_cmd_parse(char *cmd_str,
			char **name, u32 *p, int *num)
{
	char *token;
	int i, idx;
	int rc = 0;
	char *cmd_array[MAX_CMD_PARAMETER_NUMBER + 1];

	idx = 0;
	while ((token = strsep(&cmd_str, " ")) &&
			  idx <= (MAX_CMD_PARAMETER_NUMBER + 1)) {
		cmd_array[idx++] = token;
	}

	if (idx > (MAX_CMD_PARAMETER_NUMBER + 1) || idx == 0) {
		pr_err("cmd can't be parsed, %d\n", idx);
		return -1;
	}

	*name = skip_spaces(cmd_array[0]);

	for (i = 0; i < (idx - 1); i++) {
		rc = kstrtou32(cmd_array[1 + i], 0, &p[i]);
		if (rc < 0)
			return rc;
	}
	*num = idx - 1;
	return 0;
}

static int aio_cmd_trigger(struct aout_priv *aout,
			struct function_table *ft, u32 *p, int num)
{
	int ret = 0;

	switch (ft->number) {
	case 0:
		ret = ft->f0(aout);
		break;
	case 1:
		ret = ft->f1(aout, p[0]);
		break;
	case 2:
		ret = ft->f2(aout, p[0], p[1]);
		break;
	case 3:
		ret = ft->f3(aout, p[0], p[1], p[2]);
		break;
	case 4:
		ret = ft->f4(aout, p[0], p[1], p[2], p[3]);
		break;
	case 5:
		ret = ft->f5(aout, p[0], p[1], p[2], p[3], p[4]);
		break;
	case 6:
		ret = ft->f6(aout, p[0], p[1], p[2], p[3], p[4], p[5]);
		break;
	default:
		pr_err("parameter number(%d) more than %d\n",
			ft->number,
			MAX_CMD_PARAMETER_NUMBER);
		break;
	}
	return ret;
}

static int aout_aio_cmd(struct aout_priv *aout, char *cmd_str)
{
	char *name;
	int ret = 0;
	int i, n;
	u32 p[MAX_CMD_PARAMETER_NUMBER] = {0};

	if (!aout->aio_hd) {
		pr_err("aout->aio_hd is NULL\n");
		return 0;
	}

	aout_trace("aio_cmd %s\n", cmd_str);
	if (aio_cmd_parse(cmd_str, &name, p, &n) < 0) {
		pr_err("cmd parse fail\n");
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(f_table); i++) {
		if (sysfs_streq_ignorecase(name, f_table[i].name)) {
			if (f_table[i].number == n) {
				ret = aio_cmd_trigger(aout, &f_table[i], p, n);
				goto end;
			} else {
				pr_err("%s should has %d para, but %d\n",
				f_table[i].name, f_table[i].number, n);
			}
		}
	}
	pr_err("function %s not supported\n", name);
end:
	return ret;
}

static int aio_init(struct aout_priv *aout)
{
	aout_trace("%d aio fucntions initialized\n", (u32)ARRAY_SIZE(f_table));

	aout->aio_hd = open_aio(AOUT_DEVICE_NAME);
	if (IS_ERR_OR_NULL(aout->aio_hd)) {
		pr_err("get aio hd fail\n");
		aout->aio_hd = NULL;
		return -EINVAL;
	}
	aout_trace("get aio hd %p\n", aout->aio_hd);
	mutex_init(&aout->aio_mutex);

	aio_clk_enable(aout->aio_hd, AIO_APLL_0, true);
	aio_set_clk_rate(aout->aio_hd, AIO_APLL_0, APLL_RATE_48K);
	aio_clk_enable(aout->aio_hd, AIO_APLL_1, true);
	aio_set_clk_rate(aout->aio_hd, AIO_APLL_1, APLL_RATE_48K);
	return 0;
}

static int aio_exit(struct aout_priv *aout)
{
	close_aio(aout->aio_hd);
	aout->aio_hd = NULL;
	return 0;
}

static int aout_status_seq_show(struct seq_file *m, void *v)
{
	struct aout_priv *aout = m->private;
	u32 i;
	AOUT_PATH_CMD_FIFO *p;

	if (!aout)
		return 0;

	seq_puts(m,
		   "----------------------------------------------------------------------\n");
	seq_puts(m,
		   "|   Path  | FIFO Info                   | Zero Buffer     | Underflow |\n");
	seq_puts(m,
		   "|         | <Size> < RD > < WR > <RD-K> | < Addr > <Size> | Counter   |\n");
	seq_puts(m,
		   "----------------------------------------------------------------------\n");

	for (i = 0; i < MAX_OUTPUT_AUDIO; i++) {
		p = aout->ap[i].cmd_fifo;
		if (p)
			seq_printf(m,
			   "| %6s  |  %4u   %4u   %4u   %4u  | %08X  %5u | %7u %u |\n",
			   get_path_name(i),
			   p->size,
			   p->rd_offset,
			   p->wr_offset,
			   p->kernel_rd_offset,
			   p->zero_buffer,
			   p->zero_buffer_size,
			   p->underflow_cnt,
			   p->fifo_underflow);
		else
			seq_printf(m,
				   "| %6s  |                          CLOSED                           |\n",
				get_path_name(i));
	}

	seq_puts(m,
		   "----------------------------------------------------------------------\n");
	seq_puts(m, "|   path  | interrupt count |  write_cnt  |\n");
	seq_puts(m,
		   "|-----------------------------------------|\n");
	for (i = 0; i < MAX_OUTPUT_AUDIO; i++)
		if (aout->ap[i].en)
			seq_printf(m, "| %6s  |    %10d   | %10d  |\n",
			   get_path_name(i),
			   aout->ap[i].int_cnt,
			   aout->ap[i].write_cnt);
	seq_puts(m, "-------------------------------------------\n");
	return 0;
}

static int aout_status_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, aout_status_seq_show, PDE_DATA(inode));
}

static const struct file_operations amp_driver_aout_fops = {
	.open = aout_status_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t aio_cmd_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t retval;

	pr_info("function help\n");
	retval = scnprintf(buf, PAGE_SIZE, "%s\n", "show function help");

	return retval;
}

static ssize_t aio_cmd_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct aout_priv *aout = get_aout_priv(dev);
	ssize_t retval = count;
	char *kbuf;
	int rc = 0;

	if (count > PAGE_SIZE || !buf) {
		pr_err("invalid parameter\n");
		return -EINVAL;
	}

	kbuf = kmemdup_nul(buf, count, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;
	kbuf[count - 1] = '\0';
	pr_info("%s\n", kbuf);
	mutex_lock(&aout->aio_mutex);
	rc = aout_aio_cmd(aout, kbuf);
	mutex_unlock(&aout->aio_mutex);
	pr_info("%s return 0x%x\n", kbuf, rc);
	kfree(kbuf);
	return retval;
}

static struct device_attribute dev_attr_aio_cmd =
	__ATTR(aio_cmd, (0600), aio_cmd_show, aio_cmd_store);

static void wrap_aout_start_cmd(struct aout_priv *aout,
				 int *aout_info, void *param)
{
	unsigned long aoutirq = 0;

	spin_lock_irqsave(&aout->aout_spinlock, aoutirq);
	aout_start_cmd(aout, aout_info, param);
	spin_unlock_irqrestore(&aout->aout_spinlock, aoutirq);
}

static void drv_aout_release_shm(struct aout_priv *aout, int path_id)
{
	if (IS_ERR_OR_NULL(aout->ap[path_id].shm.buf))
		return;

	aout_trace("aout->shm.buf[%d] = %p\n", path_id,
		 aout->ap[path_id].shm.buf);
	aout_trace("aout->shm.p[%d] = %p\n", path_id,
		 aout->ap[path_id].shm.p);
	aout_trace("aout->shm.gid[%d] = %d\n", path_id,
		 aout->ap[path_id].shm.gid);
	dma_buf_end_cpu_access(
		aout->ap[path_id].shm.buf, DMA_BIDIRECTIONAL);
	dma_buf_kunmap(aout->ap[path_id].shm.buf, 0,
					aout->ap[path_id].shm.p);
	shm_put(aout->client, aout->ap[path_id].shm.gid);
	aout->ap[path_id].shm.gid = 0;
	aout->ap[path_id].shm.p = NULL;
	aout->ap[path_id].shm.buf = NULL;
}

static void wrap_aout_stop_cmd(struct aout_priv *aout, int path_id)
{
	unsigned long aoutirq = 0;

	spin_lock_irqsave(&aout->aout_spinlock, aoutirq);
	aout_stop_cmd(aout, path_id);
	spin_unlock_irqrestore(&aout->aout_spinlock, aoutirq);
}

static LONG drv_aout_ioctl_unlocked(struct file *filp, u32 cmd, ULONG arg)
{
	struct aout_priv *aout = filp->private_data;
	int aout_info[AOUT_INFO_SIZE];
	int gid = 0, ret;
	struct dma_buf *buf = NULL;
	void *param = NULL;
	u32 path_id;

	switch (cmd) {
	/**************************************
	 * AOUT IOCTL
	 **************************************/
	case AOUT_IOCTL_START_CMD:
		if (copy_from_user(aout_info,
			(void __user *)arg, AOUT_INFO_SIZE * sizeof(int)))
			return -EFAULT;
		path_id = aout_info[0];
		if (path_id >= MAX_OUTPUT_AUDIO) {
			aout_error("output %d out of range\r\n",
				   path_id);
			break;
		}

		if (!aout->ap[path_id].en) {
			aout_error("output %d is not enabled\r\n", path_id);
			break;
		}

		gid = aout_info[1];
		aout_trace("IOCTRL: gid(%d)\n", gid);
		if (gid) {
			buf = shm_get_dma_buf(aout->client, gid);
			if (IS_ERR(buf)) {
				aout->ap[path_id].shm.buf = NULL;
				aout_trace("gid = %d, dma_buf = %p\n",
							gid, buf);
				return -EFAULT;
			}

			aout->ap[path_id].shm.buf = buf;
		} else {
			aout_error("invalid gid\n");
			return -EFAULT;
		}

		aout_trace("gid = %d, dma_buf size = %zx\n", gid, buf->size);
		ret = dma_buf_begin_cpu_access(buf, DMA_BIDIRECTIONAL);
		if (ret) {
			aout_error("error in beginning cpu access: %d\n", ret);
			goto error;
		}
		param = dma_buf_kmap(buf, 0);
		aout_trace("buf(%p), mapping to %p\n", buf, param);
		aout->ap[path_id].shm.p = param;
		aout->ap[path_id].shm.gid = gid;
		wrap_aout_start_cmd(aout, aout_info, param);
		break;
error:
		shm_put(aout->client, gid);
		break;

	case AOUT_IOCTL_STOP_CMD:
		if (copy_from_user
			(aout_info, (void __user *) arg, 2 * sizeof(int)))
			return -EFAULT;

		path_id = aout_info[0];
		if (path_id >= MAX_OUTPUT_AUDIO) {
			aout_error("output %d out of range\r\n",
				   path_id);
			break;
		}
		if (!aout->ap[path_id].en) {
			aout_error("output %d is not enabled\r\n", path_id);
			break;
		}

		wrap_aout_stop_cmd(aout, path_id);
		drv_aout_release_shm(aout, path_id);
		break;

	case AOUT_IOCTL_GET_MSG_CMD:
		{
			HRESULT rc = S_OK;
			MV_CC_MSG_t msg = { 0 };

			rc = down_interruptible(&aout->aout_sem);
			if (rc < 0)
				return rc;
			rc = AMPMsgQ_ReadTry(&aout->hAoutMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				aout_trace("AOUT read message queue failed\n");
				return -EFAULT;
			}

			AMPMsgQ_ReadFinish(&aout->hAoutMsgQ);
			if (copy_to_user((void __user *) arg,
					 &msg, sizeof(MV_CC_MSG_t))) {
				return -EFAULT;
			}
			break;
		}
	case AOUT_IOCTL_AIO_CMD:
		{
			struct io_cmd_t {
				int len;
				int ret;
				char cmd_str[256];
			} io_cmd;

			if (copy_from_user(&io_cmd,
				(void __user *) arg,
				sizeof(struct io_cmd_t))) {
				return -EFAULT;
			}
			mutex_lock(&aout->aio_mutex);
			io_cmd.ret = aout_aio_cmd(aout, io_cmd.cmd_str);
			mutex_unlock(&aout->aio_mutex);
			if (copy_to_user((void __user *) arg,
				&io_cmd, sizeof(struct io_cmd_t))) {
				return -EFAULT;
			}
		}
		break;
	default:
		break;
	}
	return 0;
}
static int init_dhub(struct aout_priv *aout)
{
	aout->dhub = Dhub_GetDhubHandle_ByDhubId(DHUB_ID_AG_DHUB);
	if (unlikely(aout->dhub == NULL)) {
		aout_error("aout->dhub: get failed\n");
		return -ENODEV;
	}

	return 0;
}

static int drv_aout_init(struct aout_priv *aout)
{
	int ret;

	spin_lock_init(&aout->aout_spinlock);

	sema_init(&aout->aout_sem, 0);
	ret = AMPMsgQ_Init(&aout->hAoutMsgQ, AOUT_ISR_MSGQ_SIZE);
	if (unlikely(ret != S_OK)) {
		aout_error("hAoutMsgQ init: failed, err:%8x\n", ret);
		return -ENOMEM;
	}

	ret = init_dhub(aout);
	if (unlikely(ret != S_OK)) {
		aout_error("init_dhub: failed, err:%8x\n", ret);
		goto error;
	}

	device_create_file(aout->dev, &dev_attr_aio_cmd);

	ret = aio_init(aout);
	if (ret < 0) {
		aout_error("aio_func_table: failed, err:%8x\n", ret);
		goto error;
	}
	aout_trace("%s ok\n", __func__);
	return 0;
error:
	ret = AMPMsgQ_Destroy(&aout->hAoutMsgQ);
	if (unlikely(ret != S_OK))
		aout_error("aout MsgQ Destroy: failed, err:%8x\n", ret);
	return ret;
}

static atomic_t aout_dev_refcnt = ATOMIC_INIT(0);
static int drv_aout_open(struct inode *inode, struct file *file)
{
	int res;
	struct aout_priv *aout =
		container_of(inode->i_cdev, struct aout_priv, cdev);

	if (atomic_inc_return(&aout_dev_refcnt) > 1) {
		pr_info("aout driver reference count %d!\n",
			 atomic_read(&aout_dev_refcnt));
		return 0;
	}

	file->private_data = (void *)aout; /*for other methods*/

	aout->client =
		shm_client_create("aout_client");
	if (IS_ERR_OR_NULL(aout->client)) {
		aout_error("error in creating shm client: %ld\n",
				   PTR_ERR(aout->client));
		return -EFAULT;
	}

	res = drv_aout_init(aout);
	if (res != 0) {
		aout_error("failed !!! res = 0x%08X\n", res);
		goto fail_aout_init;
	}
	return 0;
fail_aout_init:
	shm_client_destroy(aout->client);
	return -ENODEV;
}

static int drv_aout_deinit(struct aout_priv *aout)
{
	u32 err;
	u8 i;

	for (i = 0; i < MAX_OUTPUT_AUDIO; i++) {
		if (aout->ap[i].en)/* path_en, has irq*/
			free_irq(aout->ap[i].irq, (void *) aout);
		wrap_aout_stop_cmd(aout, i);
		drv_aout_release_shm(aout, i);
	}

	if (!IS_ERR_OR_NULL(aout->client)) {
		shm_client_destroy(aout->client);
		aout->client = NULL;
	}

	aio_exit(aout);
	aout->dhub = NULL;
	device_remove_file(aout->dev, &dev_attr_aio_cmd);

	err = AMPMsgQ_Destroy(&aout->hAoutMsgQ);
	if (unlikely(err != S_OK))
		aout_error("aout MsgQ Destroy: failed, err:%8x\n", err);

	return 0;
}

static int drv_aout_release(struct inode *inode, struct file *filp)
{
	struct aout_priv *aout = filp->private_data;
	int res;

	res = drv_aout_deinit(aout);
	if (res < 0)
		aout_error("%s failed\n", __func__);

	return 0;
}

static int drv_aout_config(struct platform_device *pdev)
{
	int irq;
	u8 i;
	struct aout_priv *aout = get_aout_priv(&pdev->dev);
	const char *name;

	aout_enter_func();

	for (i = 0; i < MAX_OUTPUT_AUDIO; i++) {
		name = get_path_name(i);
		if (!name)
			continue;
		irq = platform_get_irq_byname(pdev, name);

		if (irq < 0) {
			aout_error("fail to get irq(%s) for node %s\n",
				   name, pdev->name);
		} else {
			aout_trace("irq(%s) = %d\n", name, irq);
			aout_trace("irqd_to_hwirq(irq_get_irq_data(%d)) = %d\n",
				irq, (u32)irqd_to_hwirq(irq_get_irq_data(irq)));
		}
		aout->ap[i].irq = irq;
	}

	return 0;
}

static int amp_major;
static int
amp_driver_setup_cdev(struct cdev *dev, int major, int minor,
			  const struct file_operations *fops)
{
	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	return cdev_add(dev, MKDEV(major, minor), 1);
}

static int drv_aout_suspend(struct device *dev)
{
	struct aout_priv *aout = get_aout_priv(dev);
	u8 i;

	if (unlikely(!aout)) {
		aout_error("aout is NULL\n");
		return -EINVAL;
	}

	if (unlikely(!aout->client)) {
		aout_trace("aout client NULL, suspend not done\n");
		return 0;
	}

	if (unlikely(!aout->dhub)) {
		aout_trace("dhub init fail, suspend not done\n");
		return 0;
	}

	for (i = 0 ; i < MAX_OUTPUT_AUDIO; i++) {
		if (aout->ap[i].en) {
			disable_irq_nosync(aout->ap[i].irq);
			DhubChannelClear(aout->dhub,
				user_getchannelfrompath(aout, i), 0);
			aout_trace("%s interrupt disabled\n",
				get_path_name(i));
		}
	}
	return 0;
}

static int drv_aout_resume(struct device *dev)
{
	struct aout_priv *aout = get_aout_priv(dev);
	u8 i;
	u32 chanId;

	if (unlikely(!aout)) {
		aout_error("aout is NULL\n");
		return -EINVAL;
	}

	if (unlikely(!aout->client)) {
		aout_trace("aout client NULL, resume not done\n");
		return 0;
	}

	if (unlikely(!aout->dhub)) {
		aout_trace("dhub init fail, resume not done\n");
		return 0;
	}

	for (i = 0 ; i < MAX_OUTPUT_AUDIO; i++) {
		if (aout->ap[i].en) {
			enable_irq(aout->ap[i].irq);
			user_aout_hal_channelenable(aout, i, 1, 1);
			chanId = get_chanId(aout, aout->ap[i].irq);
			aout_resume_cmd(aout, i, chanId);
			aout_trace("kick off %s\n",
				get_path_name(i));
		}
	}

	return 0;
}

static int aout_init(struct aout_priv *aout)
{
	int res;

	/* Now setup cdevs. */
	res =
		amp_driver_setup_cdev(&aout->cdev, aout->major,
				  aout->minor, aout->fops);
	if (res) {
		aout_error("amp_driver_setup_cdev failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}
	aout_trace("setup cdevs device minor [%d]\n", aout->minor);

	/* add PE devices to sysfs */
	aout->dev_class = class_create(THIS_MODULE, aout->dev_name);
	if (IS_ERR(aout->dev_class)) {
		aout_error("class_create failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	device_create(aout->dev_class, NULL,
			  MKDEV(aout->major, aout->minor), NULL,
			  aout->dev_name);
	aout_trace("create device sysfs [%s]\n", aout->dev_name);

	aout_trace("%s ok\n", __func__);
	/* create PE device proc file */
	aout->dev_procdir = proc_mkdir(aout->dev_name, NULL);
	if (aout->dev_procdir) {
		proc_create_data(AMP_DEVICE_PROCFILE_AOUT, 0644,
						aout->dev_procdir,
						&amp_driver_aout_fops, aout);
	}

	return 0;
err_add_device:
	if (aout->dev_class) {
		device_destroy(aout->dev_class,
				   MKDEV(aout->major, aout->minor));
		class_destroy(aout->dev_class);
	}

	cdev_del(&aout->cdev);

	return res;
}

static int aout_exit(struct aout_priv *aout)
{
	aout_trace("%s[%s] enter\n", __func__,
			   aout->dev_name);

	if (aout->dev_procdir) {
		/* remove PE device proc file */
		remove_proc_entry(AMP_DEVICE_PROCFILE_AOUT,
				  aout->dev_procdir);
		remove_proc_entry(aout->dev_name, NULL);
	}

	if (aout->dev_class) {
		/* del sysfs entries */
		device_destroy(aout->dev_class,
				   MKDEV(aout->major, aout->minor));
		aout_trace("delete device sysfs [%s]\n",
				   aout->dev_name);

		class_destroy(aout->dev_class);
	}
	/* del cdev */
	cdev_del(&aout->cdev);

	return 0;
}

static const struct file_operations aout_ops = {
	.open = drv_aout_open,
	.release = drv_aout_release,
	.unlocked_ioctl = drv_aout_ioctl_unlocked,
	.compat_ioctl = drv_aout_ioctl_unlocked,
	.owner = THIS_MODULE,
};

static int aout_probe(struct platform_device *pdev)
{
	int ret;
	struct aout_priv *aout;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	dev_t pedev;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	aout_enter_func();
	aout = devm_kzalloc(dev, sizeof(struct aout_priv),
				  GFP_KERNEL);
	if (!aout) {
		aout_error("no memory for aout\n");
		return -ENOMEM;
	}

	dev_set_drvdata(dev, (void *)aout);
	aout->dev = dev;
	device_rename(dev, AOUT_DEVICE_NAME);
	aout->dev_name = dev_name(dev);
	aout_trace("aout device name is %s\n", dev_name(dev));
	aout->fops = &aout_ops;

	ret = of_property_read_u32(np, "spdif-apll-id", &spdif_apll_id);
	if (ret)
		spdif_apll_id = 1;

	ret = of_property_read_u32(np, "hdmi-apll-id", &hdmi_apll_id);
	if (ret)
		hdmi_apll_id = 1;

	ret = drv_aout_config(pdev);
	if (ret < 0)
		goto err_config;

	ret = alloc_chrdev_region(&pedev, 0,
				AOUT_MAX_DEVS, AOUT_DEVICE_NAME);
	if (ret < 0) {
		aout_error("alloc_chrdev_region() failed for amp\n");
		goto err_alloc_chrdev;
	}
	amp_major = MAJOR(pedev);
	aout_trace("register cdev device major [%d]\n", amp_major);
	aout->major = amp_major;
	aout->minor = AOUT_MINOR;
	/*
	 * aout depends on avio but this dependency isn't reflected in DT.
	 * This may results in a wrong order in dpm_list when aout is "M"
	 * call device_move() to reorder the dpm_list. It's better to use
	 * device_pm_move_to_tail() instead, but the later function isn't
	 * exported. We want to avoid breaking GKI
	 */
	ret = device_move(dev, dev->parent, DPM_ORDER_DEV_LAST);
	if (ret)
		goto err_drv_init;

	ret = aout_init(aout);
	if (ret)
		goto err_drv_init;

	pr_info("%s ok\n", __func__);
	return 0;
err_drv_init:
	unregister_chrdev_region(MKDEV(amp_major, 0), AOUT_MAX_DEVS);
err_alloc_chrdev:
err_config:
	aout_trace("%s failed\n", __func__);
	return -1;
}

static int aout_remove(struct platform_device *pdev)
{
	struct aout_priv *aout = get_aout_priv(&pdev->dev);

	aout_exit(aout);
	unregister_chrdev_region(MKDEV(amp_major, 0), AOUT_MAX_DEVS);
	aout_trace("unregister cdev device major [%d]\n", amp_major);
	amp_major = 0;
	aout_trace("aout removed OK\n");
	return 0;
}

static const struct of_device_id aout_match[] = {
	{
		.compatible = "syna,berlin-aout",
	},
	{},
};

static SIMPLE_DEV_PM_OPS(aout_pmops, drv_aout_suspend,
			 drv_aout_resume);

static struct platform_driver aout_driver = {
	.probe = aout_probe,
	.remove = aout_remove,
	.driver = {
		.name = AOUT_DEVICE_NAME,
		.of_match_table = aout_match,
		.pm = &aout_pmops,
	},
};
module_platform_driver(aout_driver);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AOUT module driver");
