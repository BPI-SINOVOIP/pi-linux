// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _AVIO_SUB_MODULE_H_
#define _AVIO_SUB_MODULE_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/io.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/platform_device.h>

#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/gpio.h>

#include <linux/mm.h>
#include <linux/kdev_t.h>
#include <asm/page.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/clk.h>
#include <linux/uaccess.h>

#include "avio_type.h"
#include "avio_core.h"
#include "avio_ioctl.h"

#include "avio_debug.h"

#define AVIO_MODULE_STORE_AND_RETURN(pPtrStore, ptrVal, retVal) do { \
	if (pPtrStore) \
		*pPtrStore = ptrVal; \
	return retVal; } while (0)

typedef enum AVIO_MODULE_TYPE_T {
	AVIO_MODULE_TYPE_VPP,
	AVIO_MODULE_TYPE_AIO,
	AVIO_MODULE_TYPE_AOUT,
	AVIO_MODULE_TYPE_AIP,
	AVIO_MODULE_TYPE_VIP,
	AVIO_MODULE_TYPE_AVIF,
	AVIO_MODULE_TYPE_APP,
	AVIO_MODULE_TYPE_OVP,
	AVIO_MODULE_TYPE_CEC,
	AVIO_MODULE_TYPE_DHUB,
	AVIO_MODULE_TYPE_AVIO,
	AVIO_MODULE_TYPE_MAX
} AVIO_MODULE_TYPE;

//open : request_irq(irq_num, isr, IRQF_SHARED, module_name, pCtxData)
//close: free_irq(irq_num, pCtxData)
typedef struct AVIO_MODULE_FUNC_TABLE_T {
	int (*ioctl_unlocked)(void *h_ctx_data, unsigned int cmd,
		unsigned long arg, long *retVal);
	void (*create_proc_file)(void *h_ctx_data,
		struct proc_dir_entry *dev_procdir);
	void (*remove_proc_file)(void *h_ctx_data,
		struct proc_dir_entry *dev_procdir);

	int (*init)(void *h_ctx_data);
	void (*config)(void *h_ctx_data, void *dev);
	int (*open)(void *h_ctx_data);
	void (*close)(void *h_ctx_data);
	void (*enable_irq)(void *h_ctx_data);
	void (*disable_irq)(void *h_ctx_data);
	void (*exit)(void *h_ctx_data);
	int (*save_state)(void *h_ctx_data);
	int (*restore_state)(void *h_ctx_data);
} AVIO_MODULE_FUNC_TABLE;

typedef struct AVIO_MODULE_T {
	AVIO_MODULE_FUNC_TABLE func;
	char is_probed;
	char *module_name;
	void *pCtxData;
} AVIO_MODULE;

int avio_sub_module_probe(struct platform_device *pdev);
int avio_sub_module_config(struct platform_device *pdev);
int avio_sub_module_init(void);
int avio_sub_module_exit(void);
int avio_sub_module_save_state(void);
int avio_sub_module_restore_state(void);
int avio_sub_module_ioctl_unlocked(unsigned int cmd,
		unsigned long arg, long *pRetVal);
void avio_sub_module_create_proc_file(struct proc_dir_entry *dev_procdir);
void avio_sub_module_remove_proc_file(struct proc_dir_entry *dev_procdir);


void avio_sub_module_enable_irq(void);
void avio_sub_module_disable_irq(void);
int avio_sub_module_open(void);
int avio_sub_module_close(void);

void avio_sub_module_register(AVIO_MODULE_TYPE sub_module, char *module_name,
		void *pCtxData, const AVIO_MODULE_FUNC_TABLE *p_func_table);
void avio_sub_module_unregister(AVIO_MODULE_TYPE sub_module);
void *avio_sub_module_get_ctx(AVIO_MODULE_TYPE sub_module);
int avio_sub_module_dhub_init(void);
#endif //_AVIO_SUB_MODULE_H_

