// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for Renesas RZ/V2N Input Video Control
 *
 * Copyright (C) 2023 Renesas Electronics Corp.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/wait.h>

#include <media/videobuf2-dma-contig.h>

#include <media/rzv2n-ivc.h>
#include "../rzg2l-cru/rzg2l-cru.h"
#include <media/rzg2l-sensor-settings.h>

#ifdef CONFIG_VIDEO_RZV2N_ISP_IVC_DEBUG
#include <linux/debugfs.h>
#endif

#define CRU_NUM	2
#define PLANES_NUM	1
#define MAX_PLANE_SIZE	(4096 * 2160 * 12 / 8) /* To calculate memory offset */

#define ISP_AXIRX_PLNUM	(0x000)
#define ISP_AXIRX_PXFMT	(0x004)
#define ISP_AXIRX_SADDL_P0	(0x010)
#define ISP_AXIRX_SADDH_P0	(0x014)
#define ISP_AXIRX_SADDL_P1	(0x018)
#define ISP_AXIRX_SADDH_P1	(0x01c)
#define ISP_AXIRX_HSIZE	(0x020)
#define ISP_AXIRX_VSIZE	(0x024)
#define ISP_AXIRX_BLANK	(0x028)
#define ISP_AXIRX_STRD	(0x030)
#define ISP_AXIRX_ISSU	(0x040)

#define ISP_FM_CONTEXT	(0x100)
#define ISP_FM_MCON	(0x104)
#define ISP_FM_FRCON	(0x108)
#define ISP_FM_STOP	(0x10c)
#define ISP_FM_INT_EN	(0x120)
#define ISP_FM_INT_STA	(0x124)

#ifdef CONFIG_VIDEO_RZV2N_ISP_IVC_DEBUG
static struct dentry *debug_entry_dir;
#endif

static int n_ivc_bufs = N_DEF_IVC_VB2_BUFS;
module_param(n_ivc_bufs, int, 0644);
MODULE_PARM_DESC(n_ivc_bufs,
		 "Specify the number of videobuf2 buffers that are managed by IVC and passed to CRU");

static bool cru_timestamp;
module_param(cru_timestamp, bool, 0644);


/**
 * @struct received_src_slot: Represents a source frame buffer received from CRU
 */
struct received_src_slot {
	bool is_ready;
	struct list_head node;
	u32 ctx_id;
	dma_addr_t addr;
	u32 index;	/**< Corresponds to an index in struct v4l2_buffer */
	u64 timestamp;	/**< Timestamp set by CRU */
};

/**
 * @struct used_buf_slot: Represents a used frame buffer which is no more needed
 *  and ready to QBUF again
 */
struct used_buf_slot {
	struct list_head node;
	u32 ctx_id;
	u32 index;
};

/**
 * @struct ivc_context: Contains the informations of an IVC context.
 *  A context is defined by ISP and each context corresponds to a single CRU
 */
struct ivc_context {
	struct rzg2l_cru_dev *cru;
	u8 dtype;	/**< RAW bit depth (for ISP_AXIRX_PXFMT.DTYPE) */
	u16 strd;	/**< Stride bytes (for ISP_AXIRX_STRD.STR) */
	int	n_bufs;	/**< # of intermediate buffers that work as an output of CRU */

	struct v4l2_plane buf_planes[PLANES_NUM];
	int *error;
};

/**
 * @struct rzv2n_ivc_device: Device structure
 */
struct rzv2n_ivc_device {
	/* Hardware settings */
	struct device *dev;
	void __iomem *base;	/**< Base address of IVC registers */
	struct clk *pclk;
	struct reset_control *presetn;

	/* Locks */
	spinlock_t src_lock;	/**< Corresponds to received_src_list */
	spinlock_t used_lock;	/**< Corresponds to used_buf_list */
	spinlock_t state_lock;	/**< Corresponds to current transfer state */
	spinlock_t stream_lock;	/**< Corresponds to bmp_active_contexts */

	/* List structures and their instances */
	struct list_head received_src_list;	/**< List of received source frames */
	struct received_src_slot recv_src_intl[CRU_NUM][N_MAX_IVC_VB2_BUFS];	/**< Internal structure */
	struct list_head used_buf_list;	/**< List of used VB2 buffers */
	struct used_buf_slot used_buf_intl[CRU_NUM][N_MAX_IVC_VB2_BUFS];	/**< Internal structure */

	/* State of current transfer */
	struct {
		bool is_busy;	/**< True when IVC has picked a source frame */
		bool is_reg_prepared;	/**< True when IVC registers are prepared for this frame */
		bool mcfg_done;	/**< True when ISP configuration is set for this frame */
		u32 ctx_id;	/**< Current context ID */
		struct received_src_slot *src;	/**< Current source frame */
	} state;

	/* States of ISP streams */
	u16 bmp_active_contexts;	/**< Bitmap of active contexts */
	struct ivc_context context[CRU_NUM];	/**< Context object corresponding to CRU# */

	/* Wait queues */
	wait_queue_head_t transfer_q;	/**< Wait queue for the transfer thread */
	wait_queue_head_t replenish_q;	/**< Wait queue for the replenish thread */

	/* Monitoring threads */
	struct task_struct *transfer_thread;	/**< Thread that starts transferring */
	struct task_struct *replenish_thread;	/**< Thread that replenish used buffers */
	int32_t (*sw_mcfe_external_process_request_v4l2_fr_ss)(uint32_t ctx_id, uint64_t timestamp);
};

static struct rzv2n_ivc_device *gl_ivc_dev;

/* Table of callback functions that are called from CRU driver */
static const struct rzg2l_cru_callbacks gl_ivc_callbacks_for_cru = {
	.frame_end = rzv2n_ivc_notify_cru_output_done,
};

//extern int32_t sw_mcfe_external_process_request_v4l2_fr_ss(uint32_t ctx_id, uint64_t timestamp); /* sw_mcfe_external_driver.c */

static int transfer_thread_func(void *data);
static int replenish_thread_func(void *data);
static int rzv2n_ivc_remove(struct platform_device *pdev);

#ifdef CONFIG_VIDEO_RZV2N_ISP_IVC_DEBUG

#define INJECT_RAWDATA_COUNT 1

struct dma_buffer {
	dma_addr_t addr;
	void *pointer;
	size_t size;
	gfp_t gfp;
};

struct inject_rawdata_struct {
	u32 count;
	u32 size;
	u32 n_buffer;
	u32 idx;
	struct dma_buffer buffer[INJECT_RAWDATA_COUNT];
};

static struct inject_rawdata_struct inject_rawdata = {
	.count = 0,
	.size = 0xcc8580,
};

static int inject_rawdata_free(void)
{
	uint i;
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;

	for (i = 0; i < INJECT_RAWDATA_COUNT; ++i) {
		struct dma_buffer *buf = &inject_rawdata.buffer[i];

		if (buf->pointer) {
			dev_info(ivc->dev, "inject_rawdata: free buffer cpu: %p dma: %llx, size: %ld\n", buf->pointer, buf->addr, buf->size);
			dma_free_coherent(ivc->dev, buf->size, buf->pointer, buf->addr);
			buf->pointer = NULL;
		}
	}
	return 0;
}

static int inject_rawdata_alloc(uint n)
{
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	uint i;

	inject_rawdata_free();
	for (i = 0; i < INJECT_RAWDATA_COUNT && i < n; ++i) {
		struct dma_buffer *buf = &inject_rawdata.buffer[i];

		dev_info(ivc->dev, "inject_rawdata: alloc buffer %d, %d\n", i, inject_rawdata.size);
		buf->pointer = dma_alloc_coherent(ivc->dev, inject_rawdata.size, &buf->addr, GFP_KERNEL);
		if (buf->pointer) {
			buf->size = inject_rawdata.size;
			buf->gfp = GFP_KERNEL;
			dev_info(ivc->dev, "inject_rawdata: cpu: %p dma: %llx\n", buf->pointer, buf->addr);
		} else {
			dev_info(ivc->dev, "inject_rawdata: fail\n");
			return -ENOMEM;
		}
	}
	inject_rawdata.n_buffer = n;
	return 0;
}

static int inject_rawdata_realloc(void)
{
	uint n = inject_rawdata.count;
	uint i = -1;

	if (inject_rawdata.n_buffer == n) {
		for (i = 0; i < n; ++i) {
			if (!inject_rawdata.buffer[i].pointer || inject_rawdata.buffer[i].size != inject_rawdata.size)
				break;
		}
	}
	inject_rawdata.idx = 0;
	if (i != n) {
		inject_rawdata_free();
		return inject_rawdata_alloc(n);
	}
	return 0;
}

static int ivc_debug_open(struct inode *inode, struct file *file)
{
	int r;
	(void)inode;
	(void)file;
	if (inject_rawdata.size == 0 || inject_rawdata.count > INJECT_RAWDATA_COUNT)
		return -EINVAL;
	r = inject_rawdata_realloc();
	if (!inject_rawdata.buffer[0].pointer || inject_rawdata.count == 0)
		return -ENOENT;
	if (r != 0)
		return r;
	return 0;
}

static ssize_t ivc_debug_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	uint aidx, didx;
	void *dbuf;

	aidx = *f_pos / inject_rawdata.size;
	didx = *f_pos % inject_rawdata.size;
	dev_info(ivc->dev, "inject_rawdata: write %p %ld %lld, [%d].%08x\n", buf, count, *f_pos, aidx, didx);
	if (aidx > INJECT_RAWDATA_COUNT)
		return -EINVAL;
	dbuf = inject_rawdata.buffer[aidx].pointer;
	if (!dbuf)
		return -ENOENT;
	return simple_write_to_buffer(dbuf + didx, inject_rawdata.size - didx, f_pos, buf, count);
}

static const struct file_operations rzv2n_ivc_debug_fops = {
	.owner = THIS_MODULE,
	.open = ivc_debug_open,
	.write = ivc_debug_write,
	.llseek = default_llseek
};

static int ivc_debug_init(struct rzv2n_ivc_device *ivc)
{
	debug_entry_dir = debugfs_create_dir("rzv2n-ivc", NULL);
	if (debug_entry_dir == NULL) {
		dev_err(ivc->dev, "debugfs_create_dir failed\n");
		return -ENOMEM;
	}

	{
		static struct dentry *dir;

		dir = debugfs_create_dir("inject_rawdata", debug_entry_dir);
		if (dir) {
			debugfs_create_u32("count", 0600, dir, &inject_rawdata.count);
			debugfs_create_u32("size", 0600, dir, &inject_rawdata.size);
			debugfs_create_file("data", 0600, dir, NULL, &rzv2n_ivc_debug_fops);
		}
	}
	{
		uint i;

		for (i = 0; i < ARRAY_SIZE(inject_rawdata.buffer); ++i)
			inject_rawdata.buffer->pointer = NULL;
	}
	return 0;
}

static int ivc_debug_remove(struct rzv2n_ivc_device *ivc)
{
	(void)ivc;
	debugfs_remove_recursive(debug_entry_dir);
	return 0;
}
#endif

static int list_empty_with_lock(struct list_head *pnode, spinlock_t *plock)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(plock, flags);
	ret = list_empty(pnode);
	spin_unlock_irqrestore(plock, flags);
	return ret;
}

static void rzv2n_ivc_write(struct rzv2n_ivc_device *ivc, u32 offset, u32 value)
{
	iowrite32(value, ivc->base + offset);
}

/**
 * @brief	Set initial values to struct v4l2_buffer.
 *  Required before using vb2_qbuf/dqbuf or related functions
 * @param[in]	ctx_id	Target context ID
 * @param[out]	v4l2_buf	Output structure
 */
static void set_v4l2_buffer_initial_values(u32 ctx_id, struct v4l2_buffer *v4l2_buf)
{
	struct ivc_context *context = &gl_ivc_dev->context[ctx_id];

	memset(v4l2_buf, 0, sizeof(struct v4l2_buffer));
	v4l2_buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf->memory = V4L2_MEMORY_MMAP;
	v4l2_buf->m.planes = context->buf_planes;
	v4l2_buf->length = context->buf_planes[0].length;
}

/**
 * @brief	Start IVC streaming of the specified context.
 *  Supposed to be called from ISP Driver
 * @param	ctx_id	Context ID determined by ISP Driver
 * @return	0 on success, <0 on error
 */
int rzv2n_ivc_stream_start(u32 ctx_id, const struct rzv2n_ivc_sensor_mode *mode, int *error, int32_t (*sw_mcfe_external_process_request_v4l2_fr_ss)(uint32_t ctx_id, uint64_t timestamp))
{
	bool is_first = false;
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	struct ivc_context *context = &ivc->context[ctx_id];
	struct rzg2l_cru_dev *cru;
	const struct rzg2l_cru_video_format *rzvfmt;
	struct vb2_queue *q = NULL;
	struct v4l2_requestbuffers v4l2_rb;
	const enum v4l2_buf_type buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	unsigned long flags;
	int i, j, ret;

	ivc->sw_mcfe_external_process_request_v4l2_fr_ss = sw_mcfe_external_process_request_v4l2_fr_ss;

	dev_dbg(ivc->dev, "Start streaming (ctx_id:%d, res:%d/%d, wdr:%d)\n", ctx_id, mode->width, mode->height, mode->wdr_mode);
	if (!ivc)
		return -EFAULT;
	if ((ctx_id < 0) || (ctx_id >= CRU_NUM))
		return -EINVAL;
	cru = context->cru;
	if (!cru)
		return -EFAULT;
	rzvfmt = rzg2l_cru_format_from_pixel(cru->format.pixelformat);
	if (!rzvfmt) {
		dev_err(ivc->dev, "CRU#%d has unsupported pixel format\n", ctx_id);
		return -EINVAL;
	}
	if (n_ivc_bufs <= 0) {
		dev_err(ivc->dev, "n_ivc_bufs must be at least 1\n");
		return -EINVAL;
	}

	if (ivc->bmp_active_contexts & (1 << ctx_id)) {
		dev_err(ivc->dev, "Context #%d is busy\n", ctx_id);
		return -EBUSY;
	}

	cru->ctx_id = ctx_id;
	cru->exposure = mode->wdr_mode == IVC_WDR_MODE_FS_LIN ? 2 : 1;
	cru->sensor_settings = *(struct rzg2l_sensor_settings *)mode->params;

	/* Query available number of buffers through videobuf2 */
	memset(&v4l2_rb, 0, sizeof(struct v4l2_requestbuffers));
	v4l2_rb.count = n_ivc_bufs;
	v4l2_rb.type = buf_type;
	v4l2_rb.memory = V4L2_MEMORY_MMAP;

	memset(&context->buf_planes, 0, sizeof(context->buf_planes));
	context->buf_planes[0].m.mem_offset = MAX_PLANE_SIZE * cru->exposure;
	context->buf_planes[0].length = cru->format.sizeimage * cru->exposure;

	q = &cru->queue;
	ret = vb2_reqbufs(q, &v4l2_rb);
	if (ret < 0) {
		dev_err(ivc->dev, "vb2_reqbufs failed\n");
		return -ENOMEM;
	}
	dev_dbg(ivc->dev, "n_ivc_bufs:%d, actually available:%d\n", n_ivc_bufs, v4l2_rb.count);
	if (v4l2_rb.count < cru->num_buf) {
		dev_err(ivc->dev, "vb2_reqbufs returned smaller result than expected\n");
		return -ENOMEM;
	}
	context->n_bufs = v4l2_rb.count;

	/* Beyond this point, you have to run a cleanup on failure */
	/* Note: 0x2e is just a fallback value for safety. Should not be trusted */
	context->dtype = rzvfmt->dtype == 0 ? 0x2e : rzvfmt->dtype;
	context->strd = cru->format.bytesperline & 0xffff;
	context->error = error;

	spin_lock_irqsave(&ivc->stream_lock, flags);
	is_first = ivc->bmp_active_contexts ? false : true;
	ivc->bmp_active_contexts |= 1 << ctx_id;
	spin_unlock_irqrestore(&ivc->stream_lock, flags);
	if (is_first) {
		dev_dbg(ivc->dev, "This is the first stream\n");
		for (i = 0; i < CRU_NUM; i++) {
			for (j = 0; j < N_MAX_IVC_VB2_BUFS; j++) {
				ivc->recv_src_intl[i][j].is_ready = false;
				INIT_LIST_HEAD(&ivc->recv_src_intl[i][j].node);
				INIT_LIST_HEAD(&ivc->used_buf_intl[i][j].node);
			}
		}
		INIT_LIST_HEAD(&ivc->received_src_list);
		INIT_LIST_HEAD(&ivc->used_buf_list);
		rzv2n_ivc_write(ivc, ISP_FM_CONTEXT, 0x2); /* NFRM_STSEL=0, CONSEL=0b10 Multi Context */
		rzv2n_ivc_write(ivc, ISP_FM_INT_EN, 0x2); /* VVAL_IRPE=1 */

		/* Register monitoring threads */
		ivc->transfer_thread = kthread_create(transfer_thread_func, ivc, "IVC transfer thread");
		if (IS_ERR(ivc->transfer_thread)) {
			dev_err(ivc->dev, "Failed to start IVC transfer thread\n");
			ret = -EFAULT;
			goto cleanup;
		}

		ivc->replenish_thread = kthread_create(replenish_thread_func, ivc, "IVC replenish thread");
		if (IS_ERR(ivc->replenish_thread)) {
			dev_err(ivc->dev, "Failed to start IVC replenish thread\n");
			ret = -EFAULT;
			goto cleanup;
		}
		wake_up_process(ivc->transfer_thread);
		wake_up_process(ivc->replenish_thread);
	}

	/* Register callback to CRU */
	if (rzg2l_cru_set_callbacks(cru, &gl_ivc_callbacks_for_cru) < 0) {
		ret = -EFAULT;
		goto cleanup;
	}

	/* Prepare output buffers through videobuf2 */
	for (i = 0; i < context->n_bufs; i++) {
		struct v4l2_buffer v4l2_buf;
		/* Assuming that all of the v4l2_buffer have the same parameters except index */
		set_v4l2_buffer_initial_values(ctx_id, &v4l2_buf);
		v4l2_buf.index = i;
		ret = vb2_querybuf(q, &v4l2_buf);
		if (ret < 0) {
			dev_err(ivc->dev, "vb2_querybuf #%d failed with %d\n", i, ret);
			ret = -EIO;
			goto cleanup;
		}
	}

	for (i = 0; i < context->n_bufs; i++) {
		struct v4l2_buffer v4l2_buf;

		set_v4l2_buffer_initial_values(ctx_id, &v4l2_buf);
		v4l2_buf.index = i;
		ret = vb2_qbuf(q, cru->vdev.v4l2_dev->mdev, &v4l2_buf);
		if (ret < 0) {
			dev_err(ivc->dev, "vb2_qbuf #%d failed with %d\n", i, ret);
			ret = -EIO;
			goto cleanup;
		}
	}

	/*
	 * You must explicitly activate power management for CRU because
	 * IVC uses CRU API without calling rzg2l_cru_mc_open()
	 */
	ret = rzv2n_cru_pm_get(cru);
	if (ret < 0) {
		dev_err(ivc->dev, "rzv2n_cru_pm_get failed with %d\n", ret);
		ret = -EIO;
		goto cleanup;
	}

	/* Propagate .start_streaming API call to downstream (usually CRU) */
	ret = vb2_streamon(q, buf_type);
	if (ret < 0) {
		dev_err(ivc->dev, "vb2_streamon failed with %d\n", ret);
		ret = -EIO;
		goto cleanup;
	}
	return 0;

cleanup:
	if (q)
		vb2_queue_release(q);
	rzv2n_ivc_stream_stop(ctx_id);
	return ret;
}
EXPORT_SYMBOL_GPL(rzv2n_ivc_stream_start);

/**
 * @brief	Stop IVC streaming of the specified context.
 *  Supposed to be called from ISP Driver
 * @param	ctx_id	Context ID determined by ISP Driver
 * @return	0 on success, <0 on error
 */
int rzv2n_ivc_stream_stop(u32 ctx_id)
{
	int i, j;
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	struct rzg2l_cru_dev *cru;
	unsigned long flags;
	bool is_last, is_valid;

	dev_dbg(ivc->dev, "Stop streaming (ctx_id:%d)\n", ctx_id);
	if ((ctx_id < 0) || (ctx_id >= CRU_NUM))
		return -EINVAL;

	cru = ivc->context[ctx_id].cru;
	if (!cru)
		return -EFAULT;

	/*
	 * Note: ISP driver may call this function for the same context multiple times.
	 * So you must safely ignore it
	 */
	spin_lock_irqsave(&ivc->stream_lock, flags);
	is_valid = ivc->bmp_active_contexts & (1 << ctx_id);
	if (is_valid)
		ivc->bmp_active_contexts &= ~(1 << ctx_id);
	is_last = ivc->bmp_active_contexts ? false : true;
	spin_unlock_irqrestore(&ivc->stream_lock, flags);

	if (!is_valid) {
		dev_dbg(ivc->dev, "ctx_id %d is not active!\n", ctx_id);
		return -EINVAL;
	}

	/* Unregister callback from CRU */
	rzg2l_cru_set_callbacks(cru, NULL);

	/* Wait if this is the last stream and IVC is still processing it */
	if (is_last) {
		bool is_still_busy;

		spin_lock_irqsave(&ivc->state_lock, flags);
		is_still_busy = ivc->state.is_busy && ivc->state.ctx_id == ctx_id;
		spin_unlock_irqrestore(&ivc->state_lock, flags);
		if (is_still_busy) {
			int timeout = 10; /* Tentative */

			dev_dbg(ivc->dev, "Waiting until the last frame has been processed\n");
			while (ivc->state.is_busy && ivc->state.ctx_id == ctx_id && timeout-- > 0)
				msleep(20); /* Tentative */
			if (timeout <= 0)
				dev_err(ivc->dev, "Last frame timeout\n");
		}
	}

	/* Propagate .stop_streaming API call to downstream (usually CRU) */
	vb2_streamoff(&cru->queue, V4L2_BUF_TYPE_VIDEO_CAPTURE);

	/* Explicitly deactivate power managment for CRU */
	rzv2n_cru_pm_put(cru);

	if (is_last) {
		dev_dbg(ivc->dev, "This is the last stream\n");
		/*
		 * Note: In this version we don't use ISP_FM_STOP, because this method
		 * works only when IVC AXIRX is active but we have no way to know it
		 */
		rzv2n_ivc_write(ivc, ISP_FM_INT_EN, 0); /* VVAL_IRPE and all=0 */

		/* Stop monitoring threads */
		if (!IS_ERR(ivc->transfer_thread))
			kthread_stop(ivc->transfer_thread);
		if (!IS_ERR(ivc->replenish_thread))
			kthread_stop(ivc->replenish_thread);

		/* Make sure to clear internal states */
		ivc->state.is_busy = false;
		ivc->state.is_reg_prepared = false;
		ivc->state.mcfg_done = false;
		init_waitqueue_head(&ivc->transfer_q);
		init_waitqueue_head(&ivc->replenish_q);

		for (i = 0; i < CRU_NUM; i++) {
			for (j = 0; j < N_MAX_IVC_VB2_BUFS; j++) {
				ivc->recv_src_intl[i][j].is_ready = false;
				INIT_LIST_HEAD(&ivc->recv_src_intl[i][j].node);
				INIT_LIST_HEAD(&ivc->used_buf_intl[i][j].node);
			}
		}
		INIT_LIST_HEAD(&ivc->received_src_list);
		INIT_LIST_HEAD(&ivc->used_buf_list);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(rzv2n_ivc_stream_stop);

/**
 * @brief	Notify IVC that the ISP configuration has been set.
 *  This operation allows IVC to start transferring data from its inner FIFO to ISP.
 *  Supposed to be called from ISP Driver
 * @param	ctx_id	Context ID determined by ISP Driver
 * @return	0 on success
 */
int rzv2n_ivc_notify_isp_config_done(u32 ctx_id)
{
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	unsigned long flags;

	dev_dbg(ivc->dev, "isp_config_done notified (ctx_id:%d)\n", ctx_id);
	spin_lock_irqsave(&ivc->state_lock, flags);
	rzv2n_ivc_write(ivc, ISP_FM_MCON, 0x1); /* MCFG_DONE=1 */
	ivc->state.mcfg_done = true;
	if (ivc->state.is_reg_prepared) {
		dev_dbg(ivc->dev, "then START_AXIRX (for IVC buf idx#%d)\n", ivc->state.src->index);
		rzv2n_ivc_write(ivc, ISP_FM_FRCON, 0x1); /* START_AXIRX=1 */
	}
	spin_unlock_irqrestore(&ivc->state_lock, flags);
	return 0;
}
EXPORT_SYMBOL_GPL(rzv2n_ivc_notify_isp_config_done);

/**
 * @brief	Complete the current transfer and put the last frame into used_buf_list.
 *  Should be called either from IVC IRQ or ISP IRQ
 */
static void complete_current_transfer(void)
{
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	struct used_buf_slot *used_buf_intl;
	unsigned long flags;

	/* Mark the current frame buffer as USED and move it to used_buf_list */
	// dev_dbg(ivc->dev, "%s: Add ctx#%d idx#%d to used_buf_list\n", __func__, ivc->state.src->ctx_id, ivc->state.src->index);
	spin_lock_irqsave(&ivc->used_lock, flags);
	ivc->state.src->is_ready = false;
	/* Maybe we should pick an empty internal slot here, but simple use index instead */
	used_buf_intl = &ivc->used_buf_intl[ivc->state.src->ctx_id][ivc->state.src->index];
	used_buf_intl->ctx_id = ivc->state.src->ctx_id;
	used_buf_intl->index = ivc->state.src->index;
	list_add_tail(&used_buf_intl->node, &ivc->used_buf_list);
	spin_unlock_irqrestore(&ivc->used_lock, flags);
	/* Wake up the replenish thread */
	wake_up_interruptible(&ivc->replenish_q);

	/* Clear the current transfer state */
	spin_lock_irqsave(&ivc->state_lock, flags);
	ivc->state.is_busy = false;
	ivc->state.is_reg_prepared = false;
	ivc->state.mcfg_done = false;
	ivc->state.src = NULL;
	spin_unlock_irqrestore(&ivc->state_lock, flags);
	/* Wake up the transfer thread so that it can process the next frame */
	wake_up_interruptible(&ivc->transfer_q);
}

/**
 * @brief	Notify IVC that ISP has consumed a frame.
 *  This operation allows IVC to discard a frame buffer and replenish it.
 *  Supposed to be called from ISP Frame End IRQ
 * @param	ctx_id	Context ID determined by IVC Driver
 * @return	0 on success
 */
int rzv2n_ivc_notify_isp_consumed_frame(u32 ctx_id)
{
	dev_dbg(gl_ivc_dev->dev, "isp_done notified (ctx_id:%d)\n", ctx_id);
#if IVC_IS_DRIVEN_BY_ISP_IRQ
	complete_current_transfer();
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(rzv2n_ivc_notify_isp_consumed_frame);

/**
 * @brief	Notify IVC that one of the CRU outputs is ready.
 *  This operation inserts the received frame to IVC's source list and
 *  lets the transfer thread fetch it. Usually called from CRU IRQ
 * @param	ctx_id	Context ID determined by ISP Driver
 * @param	timestamp	Timestamp of this frame. Not essential for IVC
 *  but application may use it
 * @return	0 on success, <0 on error
 */
int rzv2n_ivc_notify_cru_output_done(u32 ctx_id, u64 timestamp, int error)
{
	int i, ret, result;
	unsigned long flags;
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	struct ivc_context *context = &ivc->context[ctx_id];
	struct rzg2l_cru_dev *cru = context->cru;
	struct vb2_queue *q = &cru->queue;
	struct received_src_slot *slot = NULL;
	struct v4l2_buffer v4l2_buf;

	if (context->error)
		*context->error = error;
	ret = 0;
	// dev_dbg(ivc->dev, "%s(%d, %d)\n", __func__, ctx_id, cru_slot);

	/* Pick an empty internal slot */
	for (i = 0; i < context->n_bufs; i++) {
		if (!ivc->recv_src_intl[ctx_id][i].is_ready) {
			slot = &ivc->recv_src_intl[ctx_id][i];
			break;
		}
	}
	if (!slot) {
		dev_err(ivc->dev, "No empty received_src_slot (Should not happen)\n");
		return -EBUSY;
	}

	/* Dequeue a buffer from CRU vb2 queue and remember it inside IVC structure */
	set_v4l2_buffer_initial_values(ctx_id, &v4l2_buf);
	result = vb2_dqbuf(q, &v4l2_buf, true);
	if (result != 0) {
		dev_dbg(ivc->dev, "vb2_dqbuf failed with %d. Ignore this frame\n", result);
		return -EBUSY;
	}

	slot->is_ready = true;
	slot->ctx_id = ctx_id;
	slot->addr = vb2_dma_contig_plane_dma_addr(q->bufs[v4l2_buf.index], 0);
	slot->index = v4l2_buf.index;
	slot->timestamp = cru_timestamp ? timestamp : 0;
	dev_dbg(ivc->dev, "DQBUF done (idx#%d, time:%lld)\n", slot->index, slot->timestamp);

	/* Add src to the list and let transfer_thread fetch it (Lock required) */
	spin_lock_irqsave(&ivc->src_lock, flags);
	list_add_tail(&slot->node, &ivc->received_src_list);
	spin_unlock_irqrestore(&ivc->src_lock, flags);
	wake_up_interruptible(&ivc->transfer_q);

	return ret;
}

/**
 * @brief	Prepare IVC registers and start transferring a frame to ISP.
 *  Part of transfer_thread_func()
 * @param	slot	Source frame to be transfered (readonly)
 * @return	0 on success, <0 on error
 */
static int rzv2n_ivc_frame_start(struct received_src_slot *slot)
{
	int result;
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	struct ivc_context *context = &ivc->context[slot->ctx_id];
	struct rzg2l_cru_dev *cru = context->cru;
	unsigned long flags;
	bool is_context_active;

	dev_dbg(ivc->dev, "%s: Sending ctx#%d idx#%d time:%lld\n", __func__, slot->ctx_id, slot->index, slot->timestamp);

	/* Make sure that you don't start a transfer when the target context is stopping */
	spin_lock_irqsave(&ivc->stream_lock, flags);
	is_context_active = ivc->bmp_active_contexts & (1 << slot->ctx_id);
	spin_unlock_irqrestore(&ivc->stream_lock, flags);
	if (!is_context_active) {
		dev_info(ivc->dev, "frame_start, but ctx_id %d is not active!\n", slot->ctx_id);
		return -EBUSY;
	}

	/*
	 * Let ISP Driver prepare the ISP configuration
	 * CHECKME: Maybe you can call this function much earlier to maximize
	 * efficiency, but it's not tested
	 */
	result = ivc->sw_mcfe_external_process_request_v4l2_fr_ss(slot->ctx_id, slot->timestamp);
	if (result == 0) {
		dma_addr_t addr = slot->addr;
#ifdef CONFIG_VIDEO_RZV2N_ISP_IVC_DEBUG
		if (inject_rawdata.count != 0) {
			int idx = inject_rawdata.idx;

			if (inject_rawdata.buffer[idx].pointer && inject_rawdata.buffer[idx].addr)
				addr = inject_rawdata.buffer[idx].addr;
			else
				dev_err(ivc->dev, "bad inject_rawdata_buffer\n");
			idx++;
			if (idx > inject_rawdata.n_buffer)
				idx = 0;
		}
#endif
		rzv2n_ivc_write(ivc, ISP_AXIRX_PLNUM, cru->exposure == 2 ? 1 : 0); // PNUM
#ifndef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
			addr += cru->format.bytesperline * (cru->sensor_settings.pixel_area.vertical.dummy_0 + cru->sensor_settings.pixel_area.vertical.ignored_area_of_effective_pixel_0 + cru->sensor_settings.pixel_area.vertical.effective_margin_for_color_processing_0);
#endif
		if (cru->exposure == 2)	{
			if (cru->sensor_settings.is_hdr) {
				rzv2n_ivc_write(ivc, ISP_AXIRX_SADDH_P1, (addr + cru->format.sizeimage) >> 32);
				rzv2n_ivc_write(ivc, ISP_AXIRX_SADDL_P1, (addr + cru->format.sizeimage) & 0xffffffff);
				rzv2n_ivc_write(ivc, ISP_AXIRX_SADDH_P0, addr >> 32);
				rzv2n_ivc_write(ivc, ISP_AXIRX_SADDL_P0, addr & 0xffffffff);
			} else {
				rzv2n_ivc_write(ivc, ISP_AXIRX_SADDH_P0, (addr + cru->format.sizeimage) >> 32);
				rzv2n_ivc_write(ivc, ISP_AXIRX_SADDL_P0, (addr + cru->format.sizeimage) & 0xffffffff);
				rzv2n_ivc_write(ivc, ISP_AXIRX_SADDH_P1, addr >> 32);
				rzv2n_ivc_write(ivc, ISP_AXIRX_SADDL_P1, addr & 0xffffffff);
			}
		} else {
			rzv2n_ivc_write(ivc, ISP_AXIRX_SADDH_P0, addr >> 32);
			rzv2n_ivc_write(ivc, ISP_AXIRX_SADDL_P0, addr & 0xffffffff);
		}
		rzv2n_ivc_write(ivc, ISP_AXIRX_HSIZE, cru->format.width & 0x1fff);
		rzv2n_ivc_write(ivc, ISP_AXIRX_VSIZE, cru->format.height & 0x1fff);
#ifdef CONFIG_VIDEO_RZG2L_CRU_IMAGE_PROCESSOR
		rzv2n_ivc_write(ivc, ISP_AXIRX_PXFMT, context->dtype | 0x00010000); /* CLFMT=1, DTYPE=value in rzg2l_cru_formats[] */
#else
		rzv2n_ivc_write(ivc, ISP_AXIRX_PXFMT, context->dtype/* | 0x00010000*/); /* CLFMT=1, DTYPE=value in rzg2l_cru_formats[] */
#endif
		rzv2n_ivc_write(ivc, ISP_AXIRX_STRD, context->strd);
		spin_lock_irqsave(&ivc->state_lock, flags);
		ivc->state.is_reg_prepared = true;
		if (ivc->state.mcfg_done) { /* True when sw_mcfe has been done */
			/*
			 * Note: In this version we always set MCFG_DONE first and then START_AXIRX
			 * for safety. But it's OK to set them in opposite order
			 */
			dev_dbg(ivc->dev, "MCFG_DONE is already set. Now START_AXIRX\n");
			rzv2n_ivc_write(ivc, ISP_FM_FRCON, 0x1); /* START_AXIRX=1 */
		}
		spin_unlock_irqrestore(&ivc->state_lock, flags);
	} else {
		/* Failed to configure ISP. Discard this CRU output */
		dev_dbg(ivc->dev, "IVC doesn't start AXIRX (Frame discarded)\n");
		complete_current_transfer();
	}
	return 0;
}

/**
 * @brief	Thread that watches received_src_list and starts transferring
 *  a source frame to ISP.
 * @param	data	unused
 * @return	0 when killed
 */
static int transfer_thread_func(void *data)
{
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	struct received_src_slot *slot = NULL;
	unsigned long flags;
	(void)data;

	while (!kthread_should_stop()) {
		int ret;
		/* Wait until received_src_list has a content and IVC is idle */
		ret = wait_event_interruptible(ivc->transfer_q,
			(!list_empty_with_lock(&ivc->received_src_list, &ivc->src_lock) && !ivc->state.is_busy)
			|| kthread_should_stop());
		if ((ret < 0) || kthread_should_stop()) {
			dev_dbg(ivc->dev, "%s interrupted with %d\n", __func__, ret);
			break;
		}

		/* Get a source from the list and transfer it via IVC */
		spin_lock_irqsave(&ivc->src_lock, flags);
		slot = list_entry(ivc->received_src_list.next, struct received_src_slot, node);
		list_del(&slot->node); /* Removed from the list, but slot is still alive */
		spin_unlock_irqrestore(&ivc->src_lock, flags);

		if (slot) {
			if (ivc->bmp_active_contexts & (1 << slot->ctx_id)) {
				spin_lock_irqsave(&ivc->state_lock, flags);
				ivc->state.is_busy = true;
				ivc->state.ctx_id = slot->ctx_id;
				ivc->state.src = slot;
				ivc->state.is_reg_prepared = false;
				ivc->state.mcfg_done = false;
				spin_unlock_irqrestore(&ivc->state_lock, flags);
				rzv2n_ivc_frame_start(slot);
			} else {
				dev_dbg(ivc->dev, "%s woke up but ctx_id %d is not active\n", __func__, slot->ctx_id);
			}
		}
	}
	dev_dbg(ivc->dev, "%s killed\n", __func__);
	ivc->transfer_thread = NULL;
	return 0;
}

/**
 * @brief	Thread that watches used_buf_list and replenish VB2 buffers by QBUF
 * @param	data	unused
 * @return	0 when killed
 */
static int replenish_thread_func(void *data)
{
	struct rzv2n_ivc_device *ivc = gl_ivc_dev;
	struct list_head *phead, *ptmp;
	unsigned long flags;
	(void)data;

	while (!kthread_should_stop()) {
		int ret;
		/* Wait until used_buf_list has a content */
		ret = wait_event_interruptible(ivc->replenish_q,
			!list_empty_with_lock(&ivc->used_buf_list, &ivc->used_lock)
			|| kthread_should_stop());
		if ((ret < 0) || kthread_should_stop()) {
			dev_dbg(ivc->dev, "%s interrupted with %d\n", __func__, ret);
			break;
		}

		spin_lock_irqsave(&ivc->used_lock, flags);
		/* Process every buffer in the list and replenish it by calling vb2_qbuf */
		list_for_each_safe(phead, ptmp, &ivc->used_buf_list) {
			int result;
			struct rzg2l_cru_dev *cru;
			struct v4l2_buffer v4l2_buf;
			struct used_buf_slot *used_buf_intl = list_entry(phead, struct used_buf_slot, node);

			set_v4l2_buffer_initial_values(used_buf_intl->ctx_id, &v4l2_buf);
			v4l2_buf.index = used_buf_intl->index;
			if (ivc->bmp_active_contexts & (1 << used_buf_intl->ctx_id)) {
				cru = ivc->context[used_buf_intl->ctx_id].cru;
				result = vb2_qbuf(&cru->queue, cru->vdev.v4l2_dev->mdev, &v4l2_buf);
				if (result < 0) {
					dev_err(ivc->dev, "QBUF ctx#%d idx#%d failed with %d\n", used_buf_intl->ctx_id, used_buf_intl->index, result);
					continue;
				}
				dev_dbg(ivc->dev, "QBUF ctx#%d idx#%d done\n", used_buf_intl->ctx_id, used_buf_intl->index);
			} else {
				/* Make sure that you don't call QBUF when the target context is stopping */
				dev_dbg(ivc->dev, "%s woke up but ctx_id %d is not active\n", __func__, used_buf_intl->ctx_id);
			}
			/* Now this buffer is replenished. Remove it from the list */
			list_del(phead);
		}
		spin_unlock_irqrestore(&ivc->used_lock, flags);
	}
	dev_dbg(ivc->dev, "%s killed\n", __func__);
	ivc->replenish_thread = NULL;
	return 0;
}

/**
 * @brief	IVC IRQ handler that wakes up when IVC has completed its transfer.
 *  This function does nothing if IVC_IS_DRIVEN_BY_ISP_IRQ=1.
 *  (In that case, the completion process is done by ISP Frame End IRQ)
 */
static irqreturn_t rzv2n_ivc_irq(int irq, void *data)
{
	struct rzv2n_ivc_device *ivc = data;
	const unsigned int handled = 1; /* always */

	dev_dbg(ivc->dev, "%s\n", __func__);
#if !IVC_IS_DRIVEN_BY_ISP_IRQ
	complete_current_transfer();
#endif
	return IRQ_RETVAL(handled);
}

static int rzv2n_ivc_probe(struct platform_device *pdev)
{
	struct rzv2n_ivc_device *priv;
	struct resource *res;
	int i, irq, ncru, ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;
	gl_ivc_dev = priv;

	priv->dev = &pdev->dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	ncru = of_count_phandle_with_args(priv->dev->of_node, "crus", NULL);
	if (ncru <= 0 || CRU_NUM < ncru) {
		dev_err(priv->dev, "bad CRU number\n");
		return -EINVAL;
	}

	dev_info(priv->dev, "RZ/V2N Input Video Control Driver %pR, ncru = %d\n", res, ncru);
	/* List all CRUs */
	for (i = 0; i < ncru; ++i) {
		struct of_phandle_args args;
		struct resource cru_res;
		struct platform_device *pdev;
		struct rzg2l_cru_dev *cru;

		ret = of_parse_phandle_with_args(priv->dev->of_node, "crus", NULL, i, &args);
		if (ret) {
			dev_err(priv->dev, "cannot find crus[%d] %d\n", i, ret);
			return ret;
		}
		ret = of_address_to_resource(args.np, 0, &cru_res);
		if (ret) {
			dev_err(priv->dev, "cannot find cru[%d] resource %d\n", i, ret);
			of_node_put(args.np);
			return ret;
		}
		dev_info(priv->dev, "cru[%d] = %pR\n", i, &cru_res);

		pdev = of_find_device_by_node(args.np);
		if (!pdev) {
			dev_err(priv->dev, "cannot find cru[%d] device\n", i);
			return -ENOMEM;
		}
		cru = platform_get_drvdata(pdev);
		if (!cru) {
			dev_err(priv->dev, "cannot find cru[%d] drvdata\n", i);
			return -ENOMEM;
		}
		put_device(&pdev->dev);
		priv->context[i].cru = cru;

		of_node_put(args.np);
	}

	/* Initialize locks */
	spin_lock_init(&priv->src_lock);
	spin_lock_init(&priv->used_lock);
	spin_lock_init(&priv->state_lock);
	spin_lock_init(&priv->stream_lock);
	init_waitqueue_head(&priv->transfer_q);
	init_waitqueue_head(&priv->replenish_q);

	/* Register IRQ handler */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(priv->dev, "failed to set up irq\n");
		return irq;
	}
	ret = devm_request_irq(priv->dev, irq, rzv2n_ivc_irq, IRQF_SHARED,
			       KBUILD_MODNAME, priv);
	if (ret) {
		dev_err(priv->dev, "failed to set up irq\n");
		return ret;
	}

	/* Remember clock and reset pins */
	priv->pclk = devm_clk_get(priv->dev, "pclk");
	if (IS_ERR(priv->pclk)) {
		dev_err(priv->dev, "failed to get PCLK clock\n");
		return PTR_ERR(priv->pclk);
	}
	priv->presetn = devm_reset_control_get(&pdev->dev, "presetn");
	if (IS_ERR(priv->presetn)) {
		dev_err(&pdev->dev, "failed to get PRESETN reset\n");
		return PTR_ERR(priv->presetn);
	}
	clk_prepare_enable(priv->pclk);
	reset_control_deassert(priv->presetn);

	pm_runtime_enable(&pdev->dev);

	platform_set_drvdata(pdev, priv);

#ifdef CONFIG_VIDEO_RZV2N_ISP_IVC_DEBUG
	ivc_debug_init(priv);
#endif
	return 0;
}

static int rzv2n_ivc_remove(struct platform_device *pdev)
{
	struct rzv2n_ivc_device *ivc = platform_get_drvdata(pdev);

	/* Stop IRQ */
	rzv2n_ivc_write(ivc, ISP_FM_INT_EN, 0);

	clk_disable_unprepare(ivc->pclk);
	reset_control_assert(ivc->presetn);

	pm_runtime_disable(&pdev->dev);

#ifdef CONFIG_VIDEO_RZV2N_ISP_IVC_DEBUG
	ivc_debug_remove(ivc);
#endif
	return 0;
}

static const struct of_device_id rzv2n_ivc_of_match[] = {
	{ .compatible = "renesas,rzv2n-ivc" },
	{ },
};
MODULE_DEVICE_TABLE(of, rzv2n_ivc_of_match);

static struct platform_driver rzv2n_ivc_platform_driver = {
	.probe		= rzv2n_ivc_probe,
	.remove		= rzv2n_ivc_remove,
	.driver		= {
		.name	= "rzv2n-ivc",
		.of_match_table = rzv2n_ivc_of_match,
		.suppress_bind_attrs = true,
	},
};

module_platform_driver(rzv2n_ivc_platform_driver);

MODULE_ALIAS("rzv2n-ivc");
MODULE_AUTHOR("Maki Hirai <maki.hirai.wt@bp.renesas.com>");
MODULE_DESCRIPTION("Renesas RZ/V2N Input Video Control Driver");
MODULE_LICENSE("GPL");
