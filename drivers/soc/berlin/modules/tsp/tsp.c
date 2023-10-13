/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "[tsp kernel driver]" fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/clk.h>
#include "drv_msg.h"
#include "tee_client_api.h"

#define     TSP_ISR_MSGQ_SIZE                              32
#define     TSP_ISR_START                                  0x1
#define     TSP_ISR_STOP                                   0x2
#define     TSP_ISR_WAKEUP                                 0x3
/*
 * RA_TspReg_IntReg is 0xF344 on BG5CT
 * not sure if we need to support BG5CT on kernel 4.14
 */
#define     RA_TspReg_Figo1Dtcm                            0x20000
#define     RA_FigoSysBasic_FIGO0                          0xA800
#define     RA_FigoReg_figoRstn                            0x0028
#define     RA_TspReg_IntReg                               0xDB44
#define     RA_TspIntReg_software_int_enable               0x0028
#define     RA_TspIntReg_software_int_status               0x002C
#define     RA_TspIntReg_software_int_set                  0x0030

#define     Figo_STA_RESET                                 0x00
#define     Figo_STA_RELEASE                               0x01

#define     Figo_CMD_RUN                                   0x01
#define     Figo_CMD_STALL                                 0x02

#define     RA_Figo0_cmd                                   0x0018
#define     RA_Figo0_resp                                  0x001C

#define     RA_TspDtcmGlobal_LA                            0x00008
#define     RA_LocalArea_Ext_D0th_                         0x00000
#define     RA_LocalArea_Ext_D2th_                         0x00008

#define TSP_DEVICE_NAME			"tsp"
#define TSP_DEVICE_PATH			("/dev/" TSP_DEVICE_NAME)
#define TSP_MAX_DEVS            2
#define TSP_MINOR               0

#define TSP_IOCTL_CMD_MSG       _IOW('t', 1, int[2])
#define TSP_IOCTL_GET_MSG       _IOR('t', 2, MV_CC_MSG_t)
#define TSP_IOCTL_DISABLE_INT   _IO('t', 3)
#define TSP_IOCTL_ENABLE_INT    _IO('t', 4)
#define TSP_IOCTL_SET_CLK_RATE  _IOW('t', 5, enum clk_setting)

struct tsp_context {
	struct clk *core;
	unsigned long default_rate;
	AMPMsgQ_t hTSPMsgQ;
	struct semaphore tsp_sem;
};

struct tsp_device_t {
	struct tsp_context TspCtx;
	unsigned char *dev_name;
	struct cdev cdev;
	struct class *dev_class;
	int major;
	int minor;
};

enum clk_setting {
	TSP_CLK_LOW,
	TSP_CLK_NORMAL
};

enum {
	TSP_SAVE_HW_CONTEXT = 0x10000,
	TSP_RESTORE_HW_CONTEXT,
	TSP_SET_FIGO_STATE,
	TSP_GET_FIGO_STATE
};

/*
 * Static Variables
 */
static void __iomem *tsp_virt_addr;
static int gTspIsrEnState;
static int tsp_irq;
static struct resource *pTspRes;
static struct tsp_device_t tsp_dev = {
	.dev_name = TSP_DEVICE_NAME,
	.minor = TSP_MINOR,
};

/*
 * Module internal function
 */
#define TSP_REG_WORD32_WRITE(addr, data) \
	writel_relaxed(((unsigned int)(data)), ((addr) + tsp_virt_addr))
#define TSP_REG_WORD32_READ(offset, holder)	\
	(*(holder) = readl_relaxed((offset) + tsp_virt_addr))

#define TSP_FIGO_NUM    2

static const TEEC_UUID ta_tsp_uuid = {0x1316a183, 0x894d, 0x43fe, \
	{0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xe8} };


static irqreturn_t tsp_devices_isr(int irq, void *dev_id)
{
	struct tsp_context *hTspCtx = (struct tsp_context *)dev_id;
	MV_CC_MSG_t msg = { 0 };
	u32 addr, val;
	int rc;

	addr = RA_TspReg_IntReg + RA_TspIntReg_software_int_status;
	TSP_REG_WORD32_READ(addr, &val);

	if (likely(val != 0)) {
		TSP_REG_WORD32_WRITE(addr, val);

		msg.m_MsgID = val & 0xffff;
		rc = AMPMsgQ_Add(&hTspCtx->hTSPMsgQ, &msg);
		if (likely(rc == S_OK))
			up(&hTspCtx->tsp_sem);
	}

	return IRQ_HANDLED;
}

static void drv_tsp_open(struct tsp_context *hTspCtx, unsigned int mask)
{
	u32 addr;

	addr = RA_TspReg_IntReg + RA_TspIntReg_software_int_enable;
	TSP_REG_WORD32_WRITE(addr, mask);

	return;
}

static void drv_tsp_close(struct tsp_context *hTspCtx)
{
	MV_CC_MSG_t msg = { 0 };
	u32 addr;
	int err;

	addr = RA_TspReg_IntReg + RA_TspIntReg_software_int_enable;
	TSP_REG_WORD32_WRITE(addr, 0);
	addr = RA_TspReg_IntReg + RA_TspIntReg_software_int_status;
	TSP_REG_WORD32_WRITE(addr, 0);

	do {
		err = AMPMsgQ_DequeueRead(&hTspCtx->hTSPMsgQ, &msg);
	} while (likely(err == 1));

	sema_init(&hTspCtx->tsp_sem, 0);

	return;
}

#ifdef CONFIG_PM_SLEEP
static bool figo_running[2] = {false, false};
static TEEC_Session session[TSP_FIGO_NUM];
static TEEC_Context context;

static int tz_tsp_errcode_translate(TEEC_Result result)
{
	int ret;

	switch (result) {
	case TEEC_SUCCESS:
		ret = 0;
		break;
	case TEEC_ERROR_ACCESS_DENIED:
		ret = -ENOTSUPP;
		break;
	case TEEC_ERROR_BAD_PARAMETERS:
		ret = -EINVAL;
		break;
	default:
		ret = -EPERM;
		break;
	}
	return ret;
}

static int tz_tsp_check_figo_id(uint32_t id)
{
	if (id > (TSP_FIGO_NUM - 1)) {
		pr_err("Invalid figo id:0x%x\n", id);
		return TEEC_ERROR_BAD_PARAMETERS;
	}
	return TEEC_SUCCESS;
}

static int tz_tsp_initialize(void)
{
	TEEC_Result result = TEEC_SUCCESS;
	uint32_t i;

	/* [1] Connect to TEE */
	result = TEEC_InitializeContext(
				NULL,
				&context);
	if (result != TEEC_SUCCESS) {
		pr_err("TEEC_InitializeContext ret=0x%08x\n", result);
		goto fun_ret;
	} else
		pr_info("TEEC_InitializeContext success\n");

	/* [2] Open session with TEE application */
	for (i = 0; i < TSP_FIGO_NUM; i++) {
		TEEC_Operation operation;

		operation.paramTypes = TEEC_PARAM_TYPES(
				TEEC_VALUE_INPUT,
				TEEC_NONE,
				TEEC_NONE,
				TEEC_NONE);
		operation.params[0].value.a = i;

		result = TEEC_OpenSession(
					&context,
					&session[i],
					&ta_tsp_uuid,
					TEEC_LOGIN_USER,
					NULL,
					&operation,
					NULL);
		if (result != TEEC_SUCCESS) {
			while (i--)
				TEEC_CloseSession(&session[i]);
			TEEC_FinalizeContext(&context);
			pr_err("TEEC_OpenSession ret=0x%08x\n", result);
			goto fun_ret;
		}
		pr_info("TEEC_OpenSession %d success\n", i);
	}

fun_ret:
	return tz_tsp_errcode_translate(result);
}

static void tz_tsp_finalize(void)
{
	uint32_t i;

	for (i = 0; i < TSP_FIGO_NUM; i++)
		TEEC_CloseSession(&session[i]);
	TEEC_FinalizeContext(&context);
}

static int tz_tsp_save_hw_context(uint32_t figo_id)
{
	TEEC_Result result = TEEC_SUCCESS;
	TEEC_Operation operation;

	result = tz_tsp_check_figo_id(figo_id);
	if (result != TEEC_SUCCESS)
		goto fun_ret;

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	result = TEEC_InvokeCommand(
			&session[figo_id],
			TSP_SAVE_HW_CONTEXT,
			&operation,
			NULL);
	if (result != TEEC_SUCCESS)
		pr_err("figo[%d] save HW context error: 0x%x\n",
			figo_id, result);

fun_ret:
	return tz_tsp_errcode_translate(result);
}

static int tz_tsp_restore_hw_context(uint32_t figo_id)
{
	TEEC_Result result = TEEC_SUCCESS;
	TEEC_Operation operation;

	result = tz_tsp_check_figo_id(figo_id);
	if (result != TEEC_SUCCESS)
		goto fun_ret;

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	result = TEEC_InvokeCommand(
			&session[figo_id],
			TSP_RESTORE_HW_CONTEXT,
			&operation,
			NULL);
	if (result != TEEC_SUCCESS)
		pr_err("figo[%d] restore HW context error: 0x%x\n",
			figo_id, result);

fun_ret:
	return tz_tsp_errcode_translate(result);
}

static int tz_tsp_set_figo_state(uint32_t figo_id, uint32_t state)
{
	TEEC_Result result = TEEC_SUCCESS;
	TEEC_Operation operation;

	result = tz_tsp_check_figo_id(figo_id);
	if (result != TEEC_SUCCESS)
		goto fun_ret;

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_INPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);
	operation.params[0].value.a = state;

	result = TEEC_InvokeCommand(
			&session[figo_id],
			TSP_SET_FIGO_STATE,
			&operation,
			NULL);
	if (result != TEEC_SUCCESS)
		pr_err("figo[%d] set %s failed, error code: 0x%x\n",
			figo_id, (state == Figo_STA_RESET) ? "reset" : "release", result);

fun_ret:
	return tz_tsp_errcode_translate(result);
}

static int tz_tsp_get_figo_state(uint32_t figo_id, uint32_t *state)
{
	TEEC_Result result = TEEC_SUCCESS;
	TEEC_Operation operation;

	result = tz_tsp_check_figo_id(figo_id);
	if (result != TEEC_SUCCESS)
		goto fun_ret;

	operation.paramTypes = TEEC_PARAM_TYPES(
			TEEC_VALUE_OUTPUT,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	result = TEEC_InvokeCommand(
			&session[figo_id],
			TSP_GET_FIGO_STATE,
			&operation,
			NULL);
	if (result != TEEC_SUCCESS)
		pr_err("figo[%d] get reset register error: 0x%x\n",
			figo_id, result);
	*state = operation.params[0].value.a;

fun_ret:
	return tz_tsp_errcode_translate(result);
}

static int wait_figo_resp(u32 figo_id, unsigned int cmd)
{
	u32 addr, val, cnt, old, val1, val2;

	addr = RA_Figo0_resp + RA_TspReg_Figo1Dtcm * figo_id;
	TSP_REG_WORD32_READ(addr, &val);
	if (val == cmd)
		return 0;

	addr = RA_Figo0_cmd + RA_TspReg_Figo1Dtcm * figo_id;
	TSP_REG_WORD32_READ(addr, &old);
	TSP_REG_WORD32_WRITE(addr, cmd);
	cnt = 200000;
	addr = RA_Figo0_resp + RA_TspReg_Figo1Dtcm * figo_id;
	while (--cnt) {
		udelay(1);
		TSP_REG_WORD32_READ(addr, &val);
		if (val == cmd && cmd == Figo_CMD_STALL) {
			cnt = 100;
			while (--cnt) {
				udelay(1);
				addr = RA_TspDtcmGlobal_LA +
					RA_LocalArea_Ext_D0th_ +
					RA_TspReg_Figo1Dtcm * figo_id;
				TSP_REG_WORD32_READ(addr, &val1);
				addr = RA_TspDtcmGlobal_LA +
					RA_LocalArea_Ext_D2th_ +
					RA_TspReg_Figo1Dtcm * figo_id;
				TSP_REG_WORD32_READ(addr, &val2);
				if ((u16)(val2 - 1) == (u16)val1)
					break;
			}
			if (cnt == 0) {
				pr_err("figo[%d] data stream DMA not finish\n",
					figo_id);
				pr_err("figo[%d] DsCmdIdRet = %d\n",
					figo_id, val1);
				pr_err("figo[%d] NextDsCmdId = %d\n",
					figo_id, val2);
			}
			break;
		}
		if (val == cmd && cmd == Figo_CMD_RUN)
			break;
	}
	if (cnt == 0) {
		addr = RA_Figo0_cmd + RA_TspReg_Figo1Dtcm * figo_id;
		TSP_REG_WORD32_WRITE(addr, old);
		pr_err("wait_figo[%d]_resp timeout, cmd=%d\n", figo_id, cmd);
		return -ETIMEDOUT;
	}
	return 0;
}

static int tsp_figo_stall(u32 figo_id)
{
	u32 addr, val, figoRstn;
	int res;

	addr = RA_Figo0_resp + RA_TspReg_Figo1Dtcm * figo_id;
	TSP_REG_WORD32_READ(addr, &val);
	pr_info("figo[%d] task status: %x\n", figo_id, val);

	tz_tsp_get_figo_state(figo_id, &figoRstn);
	pr_info("figo[%d] reset status: %x\n", figo_id, figoRstn);

	if (val == Figo_CMD_STALL || !figoRstn) {
		figo_running[figo_id] = false;
		res = 0;
	} else {
		figo_running[figo_id] = true;
		res = wait_figo_resp(figo_id, Figo_CMD_STALL);
		if (!res)
			tz_tsp_set_figo_state(figo_id, Figo_STA_RESET);
	}
	return res;
}

static int tsp_figo_run(u32 figo_id)
{
	if (figo_running[figo_id] == false)
		return 0;

	tz_tsp_set_figo_state(figo_id, Figo_STA_RELEASE);

	return wait_figo_resp(figo_id, Figo_CMD_RUN);
}

static void tsp_disable_irq(void)
{
	/* disable all enabled interrupt */
	if (gTspIsrEnState) {
		/* disable TSP interrupt */
		disable_irq_nosync(tsp_irq);
	}
}

static void tsp_enable_irq(void)
{
	/* disable all enabled interrupt */
	if (gTspIsrEnState) {
		/* disable TSP interrupt */
		enable_irq(tsp_irq);
	}
}

static int berlin_tsp_suspend(struct device *dev)
{
	int ret;

	pr_info("berlin_tsp_suspend\n");

	ret = tz_tsp_initialize();
	if (ret) {
		pr_err("TZ TSP initialize failed.\n");
		return ret;
	}

	tsp_disable_irq();

	ret = tsp_figo_stall(0);
	if (ret) {
		pr_err("figo[0] stall failed.\n");
		goto cleanup1;
	}
	ret = tsp_figo_stall(1);
	if (ret) {
		pr_err("figo[1] stall failed.\n");
		tsp_figo_run(0);
		goto cleanup1;
	}

	ret = tz_tsp_save_hw_context(0);
	if (ret) {
		pr_err("figo[0] save HW context failed.\n");
		goto cleanup2;
	}
	ret = tz_tsp_save_hw_context(1);
	if (ret) {
		pr_err("figo[1] save HW context failed.\n");
		goto cleanup2;
	}

	return ret;

cleanup2:
	tsp_figo_run(0);
	tsp_figo_run(1);
cleanup1:
	tsp_enable_irq();
	tz_tsp_finalize();

	return ret;
}

static int berlin_tsp_resume(struct device *dev)
{
	int ret;

	pr_info("berlin_tsp_resume\n");

	tz_tsp_set_figo_state(0, Figo_STA_RESET);
	tz_tsp_set_figo_state(1, Figo_STA_RESET);

	ret = tz_tsp_restore_hw_context(0);
	if (ret)
		goto cleanup;
	ret = tz_tsp_restore_hw_context(1);
	if (ret)
		goto cleanup;

	ret = tsp_figo_run(0);
	if (ret)
		goto cleanup;
	ret = tsp_figo_run(1);
	if (ret) {
		tsp_figo_stall(0);
		goto cleanup;
	}
	tsp_enable_irq();
	tz_tsp_finalize();
	return ret;

cleanup:
	pr_err("tsp resume failed.\n");
	tz_tsp_finalize();
	return ret;
}
#endif

static int tsp_device_init(struct tsp_device_t *tsp_dev, unsigned int user)
{
	unsigned int err;

	sema_init(&(tsp_dev->TspCtx.tsp_sem), 0);
	err = AMPMsgQ_Init(&(tsp_dev->TspCtx.hTSPMsgQ), TSP_ISR_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		pr_err("drv_tsp_init: hTSPMsgQ init failed, err:%8x\n", err);
		return -1;
	}
	pr_info("tsp_device_init ok\n");

	return S_OK;
}

static int tsp_device_exit(struct tsp_device_t *tsp_dev, unsigned int user)
{
	unsigned int err;

	clk_disable_unprepare(tsp_dev->TspCtx.core);

	err = AMPMsgQ_Destroy(&(tsp_dev->TspCtx.hTSPMsgQ));
	if (unlikely(err != S_OK)) {
		pr_err("drv_tsp_exit: TSP MsgQ Destroy failed, err:%8x\n",
			err);
	}

	pr_info("tsp_device_exit ok");
	return S_OK;
}

/*
 * Module Register API
 */
static atomic_t tsp_dev_refcnt = ATOMIC_INIT(0);
static int tsp_driver_open(struct inode *inode, struct file *filp)
{
	int err = 0;

	pr_info("Start open tsp driver!\n");

	if (atomic_inc_return(&tsp_dev_refcnt) > 1) {
		pr_info("tsp driver reference count %d!\n",
			 atomic_read(&tsp_dev_refcnt));
		return 0;
	}
	err = request_irq(tsp_irq, tsp_devices_isr, IRQF_SHARED,
			  "tsp_module", (void *)&(tsp_dev.TspCtx));
	if (unlikely(err < 0)) {
		pr_info("tsp_irq:%5d, err:%8x\n", tsp_irq, err);
		return err;
	}
	gTspIsrEnState = 1;
	pr_info("tsp_driver_open ok\n");

	return 0;
}

static int tsp_driver_release(struct inode *inode, struct file *filp)
{
	if (atomic_read(&tsp_dev_refcnt) == 0) {
		pr_info("tsp driver already released!\n");
		return 0;
	}

	if (atomic_dec_return(&tsp_dev_refcnt)) {
		pr_info("tsp dev ref cnt after this release: %d!\n",
			 atomic_read(&tsp_dev_refcnt));
		return 0;
	}

	free_irq(tsp_irq, (void *)&(tsp_dev.TspCtx));
	gTspIsrEnState = 0;

	pr_info("tsp_driver_release ok\n");

	return 0;
}

static int tsp_driver_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
	size_t size = vma->vm_end - vma->vm_start;
	unsigned long addr =
	    pTspRes->start + (vma->vm_pgoff << PAGE_SHIFT);

	if (!(addr >= pTspRes->start &&
	      (addr + size) <= pTspRes->start + resource_size(pTspRes))) {
		pr_err("Invalid address, start=0x%lx, end=0x%lx\n", addr,
			addr + size);
		return -EINVAL;
	}
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	ret = remap_pfn_range(vma,
			      vma->vm_start,
			      (pTspRes->start >> PAGE_SHIFT) +
			      vma->vm_pgoff, size, vma->vm_page_prot);
	if (ret)
		pr_info("tsp_driver_mmap failed, ret = %d\n", ret);
	else
		pr_info("tsp_driver_mmap ok\n");
	return ret;
}

static long tsp_driver_ioctl_unlocked(struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	enum clk_setting clk;

	switch (cmd) {
	case TSP_IOCTL_DISABLE_INT:
		{
			if (gTspIsrEnState) {
				/* disable TSP interrupt */
				disable_irq(tsp_irq);
			}
			break;
		}

	case TSP_IOCTL_ENABLE_INT:
		{
			if (gTspIsrEnState) {
				/* enable TSP interrupt */
				enable_irq(tsp_irq);
			}
			break;
		}
	case TSP_IOCTL_CMD_MSG:
		{
			int io_info[2];
			struct tsp_context *pTspCtx;

			pTspCtx = &(tsp_dev.TspCtx);
			if (unlikely(copy_from_user
				     (io_info, (void __user *) arg,
				      2 * sizeof(int)) > 0)) {
				return -EFAULT;
			}
			switch (io_info[0]) {
			case TSP_ISR_START:
				drv_tsp_open(pTspCtx, io_info[1]);
				break;
			case TSP_ISR_STOP:
				drv_tsp_close(pTspCtx);
				break;
			case TSP_ISR_WAKEUP:
				up(&pTspCtx->tsp_sem);
				break;
			default:
				break;
			}
			break;
		}

	case TSP_IOCTL_GET_MSG:
		{
			MV_CC_MSG_t msg = { 0 };
			int rc = S_OK;
			struct tsp_context *pTspCtx;

			pTspCtx = &(tsp_dev.TspCtx);
			rc = down_interruptible(&pTspCtx->tsp_sem);
			if (unlikely(rc < 0))
				return rc;
			rc = AMPMsgQ_ReadTry(&pTspCtx->hTSPMsgQ, &msg);
			if (unlikely(rc != S_OK)) {
				pr_info("TSP read message queue failed\n");
				return -EFAULT;
			}
			AMPMsgQ_ReadFinish(&pTspCtx->hTSPMsgQ);
			if (unlikely(copy_to_user
				     ((void __user *) arg, &msg,
				      sizeof(MV_CC_MSG_t)) > 0)) {
				return -EFAULT;
			}
			break;
		}
	case TSP_IOCTL_SET_CLK_RATE:
		if (!tsp_dev.TspCtx.core) {
			pr_err("tsp clk uncontrollable in kernel\n");
			return -EACCES;
		}
		if (unlikely(copy_from_user(&clk,
					   (void __user *)arg,
					   sizeof(enum clk_setting)) > 0)) {
			return -EFAULT;
		}
		switch (clk) {
		case TSP_CLK_LOW:
			clk_set_rate(tsp_dev.TspCtx.core,
				    tsp_dev.TspCtx.default_rate / 4);
			break;
		case TSP_CLK_NORMAL:
			clk_set_rate(tsp_dev.TspCtx.core,
				    tsp_dev.TspCtx.default_rate);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return 0;
}

static const struct file_operations tsp_ops = {
	.open = tsp_driver_open,
	.release = tsp_driver_release,
	.unlocked_ioctl = tsp_driver_ioctl_unlocked,
	.compat_ioctl = tsp_driver_ioctl_unlocked,
	.mmap = tsp_driver_mmap,
	.owner = THIS_MODULE,
};

static int
tsp_driver_setup_cdev(struct cdev *dev, int major, int minor,
		      const struct file_operations *fops)
{
	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	dev->ops = fops;
	return cdev_add(dev, MKDEV(major, minor), 1);
}

static int tsp_drv_init(struct tsp_device_t *tsp_device)
{
	int res;

	/* Now setup cdevs. */
	res = tsp_driver_setup_cdev(&tsp_device->cdev, tsp_device->major,
				  tsp_device->minor, &tsp_ops);
	if (res) {
		pr_err("tsp_driver_setup_cdev failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}
	pr_info("setup cdevs device minor [%d]\n", tsp_device->minor);

	/* add TSP devices to sysfs */
	tsp_device->dev_class = class_create(THIS_MODULE, tsp_device->dev_name);
	if (IS_ERR(tsp_device->dev_class)) {
		pr_err("class_create failed.\n");
		res = -ENODEV;
		goto err_add_device;
	}

	device_create(tsp_device->dev_class, NULL,
		      MKDEV(tsp_device->major, tsp_device->minor), NULL,
		      tsp_device->dev_name);
	pr_info("create device sysfs [%s]\n", tsp_device->dev_name);

	/* create hw device */
	res = tsp_device_init(tsp_device, 0);
	if (res != 0) {
		pr_err("tsp_int_init failed !!! res = 0x%08X\n",
			res);
		res = -ENODEV;
		goto err_add_device;
	}

	return 0;
err_add_device:
	if (tsp_device->dev_class) {
		device_destroy(tsp_device->dev_class,
			       MKDEV(tsp_device->major, tsp_device->minor));
		class_destroy(tsp_device->dev_class);
	}

	cdev_del(&tsp_device->cdev);

	return res;
}

static int tsp_drv_exit(struct tsp_device_t *tsp_device)
{
	int res;

	pr_info("tsp_drv_exit [%s] enter\n", tsp_device->dev_name);
	/* destroy kernel API */
	res = tsp_device_exit(tsp_device, 0);
	if (res != 0)
		pr_err("dev_exit failed !!! res = 0x%08X\n", res);
	if (tsp_device->dev_class) {
		/* del sysfs entries */
		device_destroy(tsp_device->dev_class,
			       MKDEV(tsp_device->major, tsp_device->minor));
		pr_info("delete device sysfs [%s]\n", tsp_device->dev_name);

		class_destroy(tsp_device->dev_class);
	}
	/* del cdev */
	cdev_del(&tsp_device->cdev);

	return 0;
}

static int berlin_tsp_probe(struct platform_device *pdev)
{
	int res;
	dev_t dev;

	tsp_irq = platform_get_irq(pdev, 0);
	pTspRes = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	tsp_virt_addr = devm_ioremap_resource(&pdev->dev, pTspRes);
	if (IS_ERR(tsp_virt_addr)) {
		res = PTR_ERR(tsp_virt_addr);
		goto err_prob_device_2;
	}

	tsp_dev.TspCtx.core = devm_clk_get_optional(&pdev->dev, "core");
	if (IS_ERR(tsp_dev.TspCtx.core)) {
		res = PTR_ERR(tsp_dev.TspCtx.core);
		pr_err("error in getting core clk handle: %d\n", res);
		goto err_prob_device_2;
	}

	clk_prepare_enable(tsp_dev.TspCtx.core);
	tsp_dev.TspCtx.default_rate = clk_get_rate(tsp_dev.TspCtx.core);

	res = alloc_chrdev_region(&dev, 0, TSP_MAX_DEVS, TSP_DEVICE_NAME);
	tsp_dev.major = MAJOR(dev);
	if (res < 0) {
		pr_err("alloc_chrdev_region() failed for tsp\n");
		goto err_prob_device_1;
	}
	pr_info("register cdev device major [%d]\n", tsp_dev.major);

	res = tsp_drv_init(&tsp_dev);
	if (res < 0) {
		pr_err("tsp_drv_init fail!\n");
		goto err_prob_device_0;
	}
	pr_info("berlin_tsp_probe OK\n");

	return 0;

err_prob_device_0:
	unregister_chrdev_region(MKDEV(tsp_dev.major, 0), TSP_MAX_DEVS);
err_prob_device_1:
	clk_disable_unprepare(tsp_dev.TspCtx.core);
err_prob_device_2:
	pr_info("tsp_probe failed !!! (%d)\n", res);
	return res;
}

static int berlin_tsp_remove(struct platform_device *pdev)
{
	pr_info("tsp_remove\n");
	tsp_drv_exit(&tsp_dev);
	unregister_chrdev_region(MKDEV(tsp_dev.major, 0), TSP_MAX_DEVS);
	pr_info("unregister cdev device major [%d]\n", tsp_dev.major);
	tsp_dev.major = 0;
	pr_info("tsp_remove OK\n");
	return 0;
}

static const struct of_device_id tsp_match[] = {
	{.compatible = "syna,berlin-tsp",},
	{.compatible = "marvell,berlin-tsp",},
	{},
};

static SIMPLE_DEV_PM_OPS(berlin_tsp_pmops, berlin_tsp_suspend,
			 berlin_tsp_resume);

static struct platform_driver berlin_tsp_driver = {
	.probe = berlin_tsp_probe,
	.remove = berlin_tsp_remove,
	.driver = {
		   .name = TSP_DEVICE_NAME,
		   .of_match_table = tsp_match,
		   .pm = &berlin_tsp_pmops,
	},
};
module_platform_driver(berlin_tsp_driver);

MODULE_AUTHOR("marvell");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("tsp module template");
