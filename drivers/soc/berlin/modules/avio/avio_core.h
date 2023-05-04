// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _AVIO_DRIVER_H_
#define _AVIO_DRIVER_H_
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include "avio_type.h"
#include "avio_ioctl.h"
#include "avio_memmap.h"

typedef struct _AVIO_CTX_ {
	struct resource *pAvioRes;
	UINT32 avio_base;
	UINT32 avio_size;
	void *avio_virt_base;
	UINT32 regmap_handle;

	unsigned char isTeeEnabled;

	struct semaphore resume_sem;
} AVIO_CTX;

struct avio_device_t {
	unsigned char *dev_name;
	struct cdev cdev;
	struct class *dev_class;
	const struct file_operations *fops;
	struct mutex mutex;
	int major;
	int minor;
	struct proc_dir_entry *dev_procdir;
	void *private_data;

	struct mutex avio_mutex;

	int (*dev_init)(struct avio_device_t*, unsigned int);
	int (*dev_exit)(struct avio_device_t*, unsigned int);
};

int avio_module_avio_probe(struct platform_device *pdev);

#endif //_AVIO_DRIVER_H_
