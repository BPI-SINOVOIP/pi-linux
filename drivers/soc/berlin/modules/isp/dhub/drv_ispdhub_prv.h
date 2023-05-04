// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */

#ifndef _DRV_ISP_DHUB_PRV_H_
#define _DRV_ISP_DHUB_PRV_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>

#include "drv_msg.h"

#define ISP_DHUB_INTR_HANDLER_MAX 32
#define ISR_DHUB_MSGQ_SIZE        64

typedef int (*ISP_DHUB_INTR_HANDLER)(unsigned int intrMask, void *pArgs);

typedef enum _ISP_DHUB_ID_ {
	ISP_DHUB_ID_TSB,
	ISP_DHUB_ID_FWR,
	ISP_DHUB_ID_MAX,
} ISP_DHUB_ID;

typedef enum _ISP_DHUB_IOCTL_ {
	ISP_DHUB_IOCTL_SET_AFFINITY,
	ISP_DHUB_IOCTL_POWER_DISABLE_INT,
	ISP_DHUB_IOCTL_POWER_ENABLE_INT,
	ISP_DHUB_IOCTL_ENABLE_INT,
	ISP_DHUB_IOCTL_DISABLE_INT,
	ISP_DHUB_IOCTL_WAIT_FOR_INT,
	ISP_DHUB_IOCTL_DHUB_INIT,
} ISP_DHUB_IOCTL;

typedef struct ISP_DHUB_INTR_HANDLER_INFO_s {
	ISP_DHUB_INTR_HANDLER pIntrHandler;
	void *pIntrHandlerArgs;
	unsigned int intrMask;
	unsigned int intrNum;
	unsigned int intrCount;

	long long last_intr_time;
	unsigned int intr_period_max;
	unsigned int intr_period_min;
} ISP_DHUB_INTR_HANDLER_INFO;

typedef struct _ISP_DHUB_SEMAPHORE_CTRL_ {
	unsigned int pop_offset;
	unsigned int full_offset;
	unsigned int cell_size;
	unsigned int cell_offset;
} ISP_DHUB_SEMAPHORE_CTRL;

typedef struct ISP_DHUB_HAL_FOPS_s {
	unsigned int (*semaphore_chk_full)(void *hdl, int id);
	void (*semaphore_clr_full)(void *hdl, int id);
	void (*semaphore_pop)(void *hdl, int id, int delta);
} ISP_DHUB_HAL_FOPS;

typedef struct _ISP_DHUB_CONTEXT_ {
	unsigned char isTeeEnabled;

	/* Range of physical memory to be mapped for ISP-DHUB*/
	void *mod_base;
	void *dhub_base[ISP_DHUB_ID_MAX];
	void *sram_base[ISP_DHUB_ID_MAX];
	void *tsb_bcm_base;

	ISP_DHUB_HAL_FOPS fops;

	ISP_DHUB_INTR_HANDLER_INFO intrHandler[ISP_DHUB_INTR_HANDLER_MAX];
	unsigned int intrHandlerCount;
	unsigned int intrMask;

	unsigned int irq_num;
	int IsrRegistered;
	ISP_DHUB_ID dhub_id;
	unsigned int intrCount;
	unsigned int enable_intr_monitor;

	/* shadow ctrl for mask */
	unsigned int cached_ictl_mask;
	ISP_DHUB_SEMAPHORE_CTRL ctl;
	/* context for suspend/resume */
	unsigned int intr_context[ISP_DHUB_INTR_HANDLER_MAX];

	atomic_t isp_dhub_isr_msg_err_cnt;
	spinlock_t dhub_msg_spinlock;
	AMPMsgQ_t hDHUBMsgQ;
	struct semaphore dhub_sem;

	struct device *dev;
} ISP_DHUB_CTX;

#endif //_DRV_ISP_DHUB_PRV_H_

