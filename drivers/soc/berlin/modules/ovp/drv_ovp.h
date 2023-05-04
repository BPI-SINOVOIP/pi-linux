// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef _OVP_DRIVER_H_
#define _OVP_DRIVER_H_
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include "drv_msg.h"

#define OVP_ISR_MSGQ_SIZE           8
#define OVP_CC_MSG                  0x00

typedef struct _OVP_CONTEXT_ {
	unsigned int ovp_intr_status;

	AMPMsgQ_t hOVPMsgQ;
	spinlock_t ovp_msg_spinlock;
	struct semaphore ovp_sem;
	struct mutex ovp_mutex;

	int irq_num;

	struct resource *pOvpRes;
	unsigned int ovp_base;
	unsigned int ovp_size;
	void *ovp_virt_base;

	struct clk *ovp_clk;
} OVP_CTX;

struct ovp_device_t {
	unsigned char *dev_name;
	struct cdev cdev;
	struct device *dev;
	struct class *dev_class;
	const struct file_operations *fops;

	int major;
	int minor;

	struct proc_dir_entry *dev_procdir;
	void *private_data;

	int (*dev_init)(struct ovp_device_t*, unsigned int);
	int (*dev_exit)(struct ovp_device_t*, unsigned int);
};

#endif //_OVP_DRIVER_H_
