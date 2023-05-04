// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */
#ifndef _DRV_HRX_H_
#define _DRV_HRX_H_

#include <linux/version.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include "drv_msg.h"
#include <linux/kdev_t.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/dma-buf.h>
#include <linux/types.h>

#include "avio_type.h"
#include "avio_base.h"
#include "avio_dhub_drv.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "drv_hrx.h"
#include "shm.h"

#define HRX_DEVICE_NAME	"hrx"
#define HRX_MAX_DEVS (1)
#define HRX_MINOR	(0)
#define HRX_ISR_MSGQ_SIZE 128
#define HRX_DEVICE_PROCFILE "hrx_info"

struct aip_shm {
	struct dma_buf *buf;
	void *p;
};

struct aip_dma_cmd {
	u32 addr0;
	u32 size0;
	u32 addr1;
	u32 size1;
};

struct aip_dma_cmd_fifo {
	struct aip_dma_cmd aip_dma_cmd[4][8];
	u32 update_pcm[8];
	u32 takein_size[8];
	u32 size;
	u32 wr_offset;
	u32 rd_offset;
	u32 kernel_rd_offset;
	u32 prev_fifo_overflow_cnt;
	/* used by kernel */
	u32 kernel_pre_rd_offset;
	u32 overflow_buffer;
	u32 overflow_buffer_size;
	u32 fifo_overflow;
	u32 fifo_overflow_cnt;
};

struct hrx_priv {
	const char *dev_name;
	struct cdev cdev;
	struct class *dev_class;
	const struct file_operations *fops;
	struct device *dev;
	int major;
	int minor;
	struct proc_dir_entry *dev_procdir;
	struct shm_client *client;

	spinlock_t aip_spinlock;
	struct semaphore vip_sem;
	struct semaphore hdmirx_sem;
	struct semaphore aip_sem;
	AMPMsgQ_t hVipMsgQ;
	AMPMsgQ_t hHdmirxMsgQ;
	AMPMsgQ_t hAipMsgQ;

	struct aip_shm shm;
	struct aip_dma_cmd_fifo *p_aip_cmdfifo;

	u32 otg_intr;
	u32 hdmirx_intr;
	u32 ytg_intr;
	u32 uvtg_intr;
	u32 itg_intr;
	u32 mic3_intr;
	u32 validate_intrs_mask;
	int aip_source;
	int aip_i2s_pair;

	HDL_dhub *dhub;

	resource_size_t hrx_base;
	struct resource *edid_resource;
	struct gpio_desc *gpiod_hrxhpd;
	struct gpio_desc *gpiod_hrx5v;
};

#define hrx_trace(...)	//pr_info("[hrx] "__VA_ARGS__)
#define hrx_enter_func() hrx_trace("[hrx] enter %s\n", __func__)
#define hrx_error(...)	pr_err("[hrx] "__VA_ARGS__)

void aip_start_cmd(struct hrx_priv *hrx, INT *aip_info, void *param);
void aip_stop_cmd(struct hrx_priv *hrx);
void aip_resume_cmd(struct hrx_priv *hrx);

#endif //_DRV_HRX_H_
