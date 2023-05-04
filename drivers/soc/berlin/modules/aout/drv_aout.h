// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */
#ifndef _DRV_AOUT_H_
#define _DRV_AOUT_H_
#include <linux/version.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/kdev_t.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/dma-buf.h>
#include <linux/types.h>

#include "drv_msg.h"
#include "avio_type.h"
#include "avio_base.h"
#include "avio_dhub_drv.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "shm.h"

#define AOUT_DEVICE_NAME	"aout"
#define AOUT_MAX_DEVS		8
#define AOUT_MINOR	(0)
#define AOUT_IOCTL_START_CMD	0xbeef2001
#define AOUT_IOCTL_STOP_CMD		0xbeef2002
#define AOUT_IOCTL_AIO_CMD		0xbeef3006

typedef enum {
	MA0_PATH = 0,
	LoRo_PATH = 1,
	SPDIF_PATH = 2,
	HDMI_PATH = 3,
	MAX_OUTPUT_AUDIO = 4,
} AUDIO_PATH;

typedef struct {
	struct dma_buf *buf;
	void *p;
	int gid;
} AOUT_SHM;

typedef struct aout_dma_info_t {
	u32 addr0;
	u32 size0;
	u32 addr1;
	u32 size1;
} AOUT_DMA_INFO;

/* !!!! Be careful:
the depth must align with user space configuration which is in audio_cfg.h */
#define AOUT_PATH_CMD_DEPTH		16

typedef struct aout_path_cmd_fifo_t {
	AOUT_DMA_INFO aout_dma_info[4][AOUT_PATH_CMD_DEPTH];
	u32 update_pcm[AOUT_PATH_CMD_DEPTH];
	u32 takeout_size[AOUT_PATH_CMD_DEPTH];
	u32 size;
	u32 wr_offset;
	u32 rd_offset;
	u32 kernel_rd_offset;
	u32 zero_buffer;
	u32 zero_buffer_size;
	u32 fifo_underflow;
	u32 aout_pre_read;
	u32 underflow_cnt;
} AOUT_PATH_CMD_FIFO;

#define AOUT_ISR_MSGQ_SIZE		128
#define AOUT_INFO_SIZE		(4)
#define AMP_DEVICE_PROCFILE_AOUT	"aout_info"

struct aout_path {
	AOUT_SHM shm;
	AOUT_PATH_CMD_FIFO *cmd_fifo;
	bool en;
	int irq;
	int int_cnt;
	int write_cnt;
};

struct aout_priv {
	const char *dev_name;
	struct cdev cdev;
	struct class *dev_class;
	const struct file_operations *fops;
	struct shm_client *client;
	struct device *dev;
	int major;
	int minor;
	struct proc_dir_entry *dev_procdir;

	struct aout_path ap[MAX_OUTPUT_AUDIO];

	spinlock_t aout_spinlock;
	struct semaphore aout_sem;
	AMPMsgQ_t hAoutMsgQ;

	HDL_dhub *dhub;
	void *aio_hd;
	struct mutex aio_mutex;
};

#define aout_trace(...)  pr_debug(__VA_ARGS__)
#define aout_enter_func() aout_trace("enter %s\n", __func__)
#define aout_error  pr_err

#endif //_DRV_AOUT_H_
