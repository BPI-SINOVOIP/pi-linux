// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/seq_file.h>

#include "isp_driver.h"
#include "ispdhub_sema.h"
#include "ispdhub_cfg.h"

#include "isp_driver.h"
#include "drv_isp.h"
#include "drv_debug.h"
#include "drv_msg.h"
#include "drv_ispdhub.h"
#include "ispDhub.h"

#define ISP_DHUB_MSG_ID 0

#define ISP_DHUB_DEVICE_PROCFILE_CONFIG		"config"
#define ISP_DHUB_DEVICE_PROCFILE_STATUS		"status"
#define ISP_DHUB_DEVICE_PROCFILE_DETAIL		"detail"

static const char *isp_dhub_names[ISP_DHUB_ID_MAX+1] = {
	"isp_tsb_dhub",
	"isp_fwr_dhub",
	"invalid",
};

void __weak isp_drv_dhub_config_ctx(void *h_dhub_ctx, void *ispss_base) { }

static int isp_drv_dhub_config(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	int ret = 0;
	int i;
	struct device_node *np;
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)mod_ctx->mod_prv;
	struct platform_device *pdev = to_platform_device(isp_dev->dev);

	//Get or read any DHUB specific configuratiaon from DTS
	np = pdev->dev.of_node;

	for (i = 0; i < ISP_DHUB_ID_MAX; i++) {
		hDhubCtx->irq_num = of_irq_get_byname(np, isp_dhub_names[i]);

		if(((int)hDhubCtx->irq_num) > 0) {
			hDhubCtx->dhub_id = i;
			break;
		}
	}

	ispdhub_trace("%s:%d: irq - %s:%x\n", __func__, __LINE__,
		isp_dhub_names[i], hDhubCtx->irq_num);

	if (!hDhubCtx->mod_base) {
		ispdhub_error("%s:%d: ISP Base not initialized - %p\n",
			__func__, __LINE__, hDhubCtx->mod_base);
		ret = -1;
	}

	//Note: already DTS configuration is parsed by ISP module
	ispdhub_trace("%s:%d: ISP base : %p\n", __func__, __LINE__,
					hDhubCtx->mod_base);

	isp_drv_dhub_config_ctx(hDhubCtx, hDhubCtx->mod_base);

	return ret;
}

static void isp_drv_dhub_update_intr_period(ISP_DHUB_INTR_HANDLER_INFO *p_int_handler)
{
	struct timespec nows;
	long long time;
	unsigned int period;

	ktime_get_ts(&nows);
	time = timespec_to_ns(&nows);

	if (p_int_handler->last_intr_time != 0) {

		period = (unsigned int)
			abs(time - p_int_handler->last_intr_time);

		if (period > p_int_handler->intr_period_max)
			p_int_handler->intr_period_max = period;
		if (period < p_int_handler->intr_period_min)
			p_int_handler->intr_period_min = period;
	}

	p_int_handler->last_intr_time = time;
}

static irqreturn_t isp_devices_dhub_isr(int irq, void *dev_id)
{
	isp_module_ctx *p_mCtx = (isp_module_ctx*)dev_id;
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)p_mCtx->mod_prv;
	ISP_DHUB_INTR_HANDLER_INFO *p_int_handler;
	ISP_DHUB_HAL_FOPS *fops = &hDhubCtx->fops;
	int intr_num;
	unsigned int instat;
	unsigned int intr_status;

	//Get the interrupt status
	instat = fops->semaphore_chk_full(hDhubCtx, -1);

	//Clear & Invoke all interrupt handlers
	while (instat) {

		//get interrupt num from instat
		intr_num = ffs(instat) - 1;

		//get interrupt handler and status
		p_int_handler = &hDhubCtx->intrHandler[intr_num];
		intr_status = instat & p_int_handler->intrMask;

		//Clear/Ack interrupt
		fops->semaphore_pop(hDhubCtx, intr_num, 1);
		fops->semaphore_clr_full(hDhubCtx, intr_num);

		//Invoke the registered interrupt handlers
		//Otherwise log error only once
		if (intr_status && p_int_handler->pIntrHandler)
			p_int_handler->pIntrHandler(intr_num,
				p_int_handler->pIntrHandlerArgs);
		else if (!p_int_handler->intrCount)
			ispdhub_trace("%s:%d: NoHandler - %d/0x%x\n",
				__func__, __LINE__,
				intr_num, intr_status);

		//Increment interrupt count
		p_int_handler->intrCount++;
		hDhubCtx->intrCount++;

		if (hDhubCtx->enable_intr_monitor)
			isp_drv_dhub_update_intr_period(p_int_handler);

		//Clear the interrupt bit in instat
		instat &= ~(1 << intr_num);
	}

	return IRQ_HANDLED;
}

static void isp_drv_dhub_enable_irq(ISP_DHUB_CTX *hDhubCtx)
{
	/* enable interrupt during resume/power-on */
	if (hDhubCtx->IsrRegistered)
		enable_irq(hDhubCtx->irq_num);
}

static void isp_drv_dhub_disable_irq(ISP_DHUB_CTX *hDhubCtx)
{
	/* disable interrupt during shutdown/power-down */
	if (hDhubCtx->IsrRegistered)
		disable_irq_nosync(hDhubCtx->irq_num);
}

static void isp_drv_dhub_IntrUnRegisterHandler(ISP_DHUB_CTX *hDhubCtx,
			unsigned int intr_num)
{
	ISP_DHUB_INTR_HANDLER_INFO *p_intr_info;
	unsigned int intr_mask = (1 << intr_num);

	if ((intr_num >= ISP_DHUB_INTR_HANDLER_MAX))
		return;

	if ((hDhubCtx->intrMask & intr_mask)) {
		//Already registered, then allow un-registration

		spin_lock(&hDhubCtx->dhub_msg_spinlock);

		//Remove the entry
		p_intr_info = &hDhubCtx->intrHandler[intr_num];
		p_intr_info->pIntrHandler = NULL;
		p_intr_info->pIntrHandlerArgs = NULL;
		p_intr_info->intrMask = 0;
		p_intr_info->intrNum = 0;
		p_intr_info->intrCount = 0;
		hDhubCtx->intrMask &= ~intr_mask;

		//Decrement the ISR count for only removed entry
		hDhubCtx->intrHandlerCount--;

		ispdhub_trace("%s:%d Removed fom ISP_DHUB-/%d : %d / %x\n",
			__func__, __LINE__,
			hDhubCtx->intrHandlerCount, intr_num, intr_mask);

		spin_unlock(&hDhubCtx->dhub_msg_spinlock);
	}
}

static void isp_drv_dhub_IntrRegisterHandler(ISP_DHUB_CTX *hDhubCtx,
			unsigned int intr_num,
			void *pArgs, ISP_DHUB_INTR_HANDLER intrHandler)
{
	ISP_DHUB_INTR_HANDLER_INFO *p_intr_info;
	unsigned int intr_mask = (1 << intr_num);

	if (intr_num >= ISP_DHUB_INTR_HANDLER_MAX)
		return;

	if (!(hDhubCtx->intrMask & intr_mask)) {
		//Not already registered, then allow registration

		spin_lock(&hDhubCtx->dhub_msg_spinlock);

		//Add the entry
		p_intr_info = &hDhubCtx->intrHandler[intr_num];
		p_intr_info->pIntrHandler = intrHandler;
		p_intr_info->pIntrHandlerArgs = pArgs;
		p_intr_info->intrMask |= intr_mask;
		p_intr_info->intrNum = intr_num;
		p_intr_info->intrCount = 0;

		hDhubCtx->intrMask |= intr_mask;

		//increment the ISR count for only new entry
		hDhubCtx->intrHandlerCount++;

		ispdhub_trace("%s:%d Added to ISP_DHUB-/%d : %d / %x\n",
			__func__, __LINE__,
			hDhubCtx->intrHandlerCount, intr_num, intr_mask);

		spin_unlock(&hDhubCtx->dhub_msg_spinlock);
	} else if (!intrHandler) {
		isp_drv_dhub_IntrUnRegisterHandler(hDhubCtx, intr_num);
	}
}

static int isp_drv_dhub_post_msg(ISP_DHUB_CTX *hDhubCtx, unsigned int msgId,
    unsigned int param1, unsigned int param2)
{
	MV_CC_MSG_t msg;
	int ret = 0;

	msg.m_MsgID = msgId;
	msg.m_Param1 = param1;
	msg.m_Param2 = param2;

	spin_lock(&hDhubCtx->dhub_msg_spinlock);
	ret = AMPMsgQ_Add(&hDhubCtx->hDHUBMsgQ, &msg);
	spin_unlock(&hDhubCtx->dhub_msg_spinlock);

	if (ret == 0) {
		//trigger interrupt semaphore
		up(&hDhubCtx->dhub_sem);
	} else {
		if (!atomic_read(&hDhubCtx->isp_dhub_isr_msg_err_cnt))
			ispdhub_error("[isp dhub isr] MsgQ full\n");

		atomic_inc(&hDhubCtx->isp_dhub_isr_msg_err_cnt);
	}

	return ret;
}

static int isp_drv_dhub_pop_msg(ISP_DHUB_CTX *hDhubCtx, MV_CC_MSG_t *p_msg)
{
	int ret = 0;
	unsigned long flags;

	//wait for interrupt semaphore
	ret = down_interruptible(&hDhubCtx->dhub_sem);
	if (ret < 0)
		return -1;

	//check fullness
	ret = AMPMsgQ_Fullness(&hDhubCtx->hDHUBMsgQ);
	if (ret <= 0)
		return -1;

	//Pop one message
	spin_lock_irqsave(&hDhubCtx->dhub_msg_spinlock, flags);
	ret = AMPMsgQ_DequeueRead(&hDhubCtx->hDHUBMsgQ, p_msg);
	spin_unlock_irqrestore(&hDhubCtx->dhub_msg_spinlock, flags);

	//Reset error count after sucessful pop
	if (!atomic_read(&hDhubCtx->isp_dhub_isr_msg_err_cnt))
		atomic_set(&hDhubCtx->isp_dhub_isr_msg_err_cnt, 0);

	return ret;
}

static void isp_drv_dwp_clear_intr(void __iomem *mod_base)
{
	void __iomem *isp_be_dewarp_lgdc_base =
		mod_base + ISP_BE_DWP_REG_BASE + ISP_BE_DWP_LGDC_REG_BASE;

	writel(ISP_BE_CLEAR_DWP_TG, isp_be_dewarp_lgdc_base + ISP_BE_DWP_CFTG);
}

static int isp_drv_dhub_irq_handler(unsigned int intr_num, void *pArgs)
{
	isp_module_ctx *p_mCtx = (isp_module_ctx*)pArgs;
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)p_mCtx->mod_prv;
	int status = hDhubCtx->intrHandler[intr_num].intrMask;
	int ret = 0;

	if(IS_ISP_CORE_INTR(intr_num))
		isp_drv_core_clear_intr(intr_num, &status, hDhubCtx->mod_base);
	else if(IS_ISP_BE_DWP_INTR(intr_num))
		isp_drv_dwp_clear_intr(hDhubCtx->mod_base);

	ret = isp_drv_dhub_post_msg(hDhubCtx, ISP_DHUB_MSG_ID, status, intr_num);

	return ret;
}

static int isp_drv_dhub_exit(ISP_DHUB_CTX *hDhubCtx)
{
	unsigned int err = 0;

	err = AMPMsgQ_Destroy(&hDhubCtx->hDHUBMsgQ);
	if (unlikely(err != S_OK))
		ispdhub_error("%s: hDHUBMsgQ Destroy failed, err:%8x\n",
				__func__, err);

	return err;
}

static int isp_drv_dhub_init(ISP_DHUB_CTX *hDhubCtx)
{
	unsigned int err = 0;

	err = AMPMsgQ_Init(&hDhubCtx->hDHUBMsgQ, ISR_DHUB_MSGQ_SIZE);
	if (unlikely(err != S_OK)) {
		ispdhub_error("%s: hDHUBMsgQ init failed, err:%8x\n",
				__func__, err);
	} else {
		spin_lock_init(&hDhubCtx->dhub_msg_spinlock);
		sema_init(&hDhubCtx->dhub_sem, 0);
	}

	return err;
}

static ssize_t isp_dhub_config_proc_write(struct file *file,
					 const char __user *buffer,
					 size_t count, loff_t *f_pos)
{
	isp_module_ctx *p_mCtx = (isp_module_ctx*)PDE_DATA(file_inode(file));
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)p_mCtx->mod_prv;
	unsigned long config = 0;
	unsigned int i, ascii_code, dec_code;
	unsigned int size =
		count > sizeof(unsigned long) ? sizeof(unsigned long) : count;

	if (copy_from_user(&config, buffer, size))
		return 0;

	//Convert ASCII value to hex digit - max 8 hex digit allowed
	for (i = 0; i < size; i++) {

		dec_code = 0;
		ascii_code = ((config >> (i * 8)) & 0xFF);
		if (ascii_code >= 'a' && ascii_code <= 'f')
			dec_code = 'a' - 10;
		else if (ascii_code >= 'A' && ascii_code <= 'F')
			dec_code = 'A' - 10;
		else if (ascii_code >= '0' && ascii_code <= '9')
			dec_code = '0';

		if (!dec_code)
			break;

		//Initilaize only if valid number is seen
		if (!i)
			hDhubCtx->enable_intr_monitor = 0;

		hDhubCtx->enable_intr_monitor <<= 4;
		hDhubCtx->enable_intr_monitor |= (ascii_code - dec_code);
	}

	ispdhub_info("%s:%d: size:%x, config:%lx->%x\n", __func__, __LINE__,
		size, config, hDhubCtx->enable_intr_monitor);

	return size;
}

static int isp_dhub_config_seq_show(struct seq_file *m, void *v)
{
	unsigned int config = 0;
	isp_module_ctx *p_mCtx = (isp_module_ctx*)m->private;
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)p_mCtx->mod_prv;

	config = hDhubCtx->enable_intr_monitor;

	seq_puts(m, "--------------------------------------\n");
	seq_printf(m, "| intr_monitor |0x%08x           |\n", config);
	seq_puts(m, "--------------------------------------\n");

	return 0;
}

static int isp_dhub_config_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_dhub_config_seq_show, PDE_DATA(inode));
}

static int isp_dhub_status_seq_show(struct seq_file *m, void *v)
{
	isp_module_ctx *p_mCtx = (isp_module_ctx*)m->private;
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)p_mCtx->mod_prv;

	seq_puts(m, "-----------------------------------\n");
	seq_printf(m, "| %s | Mask:%08x, handler_count:%02d, intr_count:%-10d |\n",
		   isp_dhub_names[hDhubCtx->dhub_id], hDhubCtx->intrMask,
		   hDhubCtx->intrHandlerCount, hDhubCtx->intrCount);
	seq_puts(m, "-----------------------------------\n");

	return 0;
}

static int isp_dhub_status_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_dhub_status_seq_show, PDE_DATA(inode));
}

static int isp_dhub_detail_seq_show(struct seq_file *m, void *v)
{
	isp_module_ctx *p_mCtx = (isp_module_ctx*)m->private;
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)p_mCtx->mod_prv;
	int i;
	ISP_DHUB_INTR_HANDLER_INFO *p_intr_info;

	seq_puts(m, "------------------------------------\n");
	seq_puts(m, "|  Intr  |    mask     | count     |");
	if (hDhubCtx->enable_intr_monitor)
		seq_puts(m, " min-period| max-period|");
	seq_puts(m, "\n");

	for (i = 0; i < ISP_DHUB_INTR_HANDLER_MAX; i++) {
		p_intr_info = &hDhubCtx->intrHandler[i];
		if (p_intr_info->pIntrHandler) {
			seq_printf(m, "|   %-2d   |  %-8x   | %-10d|",
				p_intr_info->intrNum, p_intr_info->intrMask,
				p_intr_info->intrCount);
			if (hDhubCtx->enable_intr_monitor)
				seq_printf(m, " %-10d| %-10d|",
					p_intr_info->intr_period_min,
					p_intr_info->intr_period_max);
			seq_printf(m, "\n");
		}
	}

	seq_puts(m, "------------------------------------\n");

	return 0;
}

static int isp_dhub_detail_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_dhub_detail_seq_show, PDE_DATA(inode));
}


static const struct file_operations isp_dhub_driver_config_fops = {
	.owner = THIS_MODULE,
	.open = isp_dhub_config_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = isp_dhub_config_proc_write,
};

static const struct file_operations isp_dhub_driver_status_fops = {
	.owner = THIS_MODULE,
	.open = isp_dhub_status_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations isp_dhub_driver_detail_fops = {
	.owner = THIS_MODULE,
	.open = isp_dhub_detail_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void isp_drv_dhub_create_proc_file(struct proc_dir_entry *dev_procdir, isp_module_ctx *p_mCtx)
{
	struct proc_dir_entry *isp_dhub_driver_config;
	struct proc_dir_entry *isp_dhub_driver_state;
	struct proc_dir_entry *isp_dhub_driver_detail;

	if (dev_procdir) {
		isp_dhub_driver_config =
			proc_create_data(ISP_DHUB_DEVICE_PROCFILE_CONFIG, 0644,
					dev_procdir, &isp_dhub_driver_config_fops, p_mCtx);
		isp_dhub_driver_state =
			proc_create_data(ISP_DHUB_DEVICE_PROCFILE_STATUS, 0644,
					dev_procdir, &isp_dhub_driver_status_fops, p_mCtx);
		isp_dhub_driver_detail =
			proc_create_data(ISP_DHUB_DEVICE_PROCFILE_DETAIL, 0644,
					dev_procdir, &isp_dhub_driver_detail_fops, p_mCtx);
	}
}

static void isp_drv_dhub_remove_proc_file(struct proc_dir_entry *dev_procdir)
{
	if (dev_procdir) {
		remove_proc_entry(ISP_DHUB_DEVICE_PROCFILE_DETAIL, dev_procdir);
		remove_proc_entry(ISP_DHUB_DEVICE_PROCFILE_STATUS, dev_procdir);
		remove_proc_entry(ISP_DHUB_DEVICE_PROCFILE_CONFIG, dev_procdir);
	}
}

static int isp_dhub_mod_init(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	int retVal;
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)mod_ctx->mod_prv;

	//probe/configure ISP DHUB driver
	//Read the configuration needed by ISP DHUB
	retVal = isp_drv_dhub_config(isp_dev, mod_ctx);
	if (retVal)
		return retVal;

	//init ISP DHUB driver
	retVal = isp_drv_dhub_init(hDhubCtx);
	if (retVal)
		return retVal;

	isp_drv_dhub_create_proc_file(isp_dev->dev_procdir, mod_ctx);

	return 0;
}

#ifdef CONFIG_PM
int isp_dhub_mod_suspend(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)mod_ctx->mod_prv;

	ispdhub_semaphore_suspend(hDhubCtx);

	isp_drv_dhub_disable_irq(hDhubCtx);

	return 0;
}

int isp_dhub_mod_resume(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)mod_ctx->mod_prv;

	ispdhub_semaphore_resume(hDhubCtx);

	isp_drv_dhub_enable_irq(hDhubCtx);

	return 0;
}
#else
int isp_dhub_mod_suspend(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	return 0;
}

int isp_dhub_mod_resume(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	return 0;
}
#endif /* CONFIG_PM */

long isp_dhub_mod_ioctl(isp_device *isp_dev, isp_module_ctx *mod_ctx,
			unsigned int cmd, unsigned long arg)
{
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)mod_ctx->mod_prv;
	unsigned int sub_cmd;

	//LSB 8b - Main command, MSB 24b - Sub command
	sub_cmd = cmd >> 8;
	cmd = cmd & 0xFF;

	if ((cmd != ISP_MODULE_IOCTL))
		return -EINVAL;
	else if (!hDhubCtx->IsrRegistered)
		return -EPERM;

	switch (sub_cmd) {
	case ISP_DHUB_IOCTL_SET_AFFINITY:
	{
		unsigned int aff_param[2] = { 0, 0 };
		const struct cpumask *m;

		if (copy_from_user(aff_param,
			(void __user *) arg, 2 * sizeof(unsigned int)))
			return -EFAULT;

		ispdhub_trace("%s:%d: aff_param-%d,%d\n",
			__func__, __LINE__, aff_param[0], aff_param[1]);

		ispdhub_trace("%s:%d: set aff_param to irq - %d\n",
			__func__, __LINE__, hDhubCtx->irq_num);

		m = get_cpu_mask(aff_param[1]);
		irq_set_affinity_hint(hDhubCtx->irq_num, m);
		break;
	}

	case ISP_DHUB_IOCTL_DHUB_INIT:
	{
		if ((atomic_read(&mod_ctx->refcnt) == 1) &&
			(hDhubCtx->intrHandlerCount == 0)) {
			/*Initialize DHUB semaphore for first open
			 * before any intr registration */
			ispdhub_semaphore_config(hDhubCtx);
		}

		break;
	}

	case ISP_DHUB_IOCTL_POWER_DISABLE_INT:
	{
		/* disable DHUB interrupt */
		isp_drv_dhub_disable_irq(hDhubCtx);
		break;
	}

	case ISP_DHUB_IOCTL_POWER_ENABLE_INT:
	{
		/* enable DHUB interrupt */
		isp_drv_dhub_enable_irq(hDhubCtx);
		break;
	}

	case ISP_DHUB_IOCTL_ENABLE_INT:
	{
		unsigned int irq_num;

		if (copy_from_user(&irq_num,
			(void __user *) arg, sizeof(unsigned int)))
			return -EFAULT;

		ispdhub_trace("%s:%d: Enable int - %d / 0x%x\n",
			__func__, __LINE__, irq_num, 1 << irq_num);

		isp_drv_dhub_IntrRegisterHandler(hDhubCtx, irq_num,
			mod_ctx, isp_drv_dhub_irq_handler);

		//Enable the semaphore interrupt
		ispdhub_semaphore_enable_intr(hDhubCtx, irq_num);

		break;
	}

	case ISP_DHUB_IOCTL_DISABLE_INT:
	{
		unsigned int irq_num;

		if (copy_from_user(&irq_num,
			(void __user *) arg, sizeof(unsigned int)))
			return -EFAULT;

		ispdhub_trace("%s:%d: Disable int - %d / 0x%x\n",
			__func__, __LINE__, irq_num, 1 << irq_num);

		isp_drv_dhub_IntrUnRegisterHandler(hDhubCtx, irq_num);

		//Disable the semaphore interrupt
		ispdhub_semaphore_disable_intr(hDhubCtx, irq_num);

		break;
	}

	case ISP_DHUB_IOCTL_WAIT_FOR_INT:
	{
		MV_CC_MSG_t msg = { 0 };
		int ret;

		ret = isp_drv_dhub_pop_msg(hDhubCtx, &msg);

		if ((ret != 1) ||
			copy_to_user((void __user *) arg, &msg,
					sizeof(MV_CC_MSG_t))) {
			return -EFAULT;
		}
		break;
	}

	default:
		return -EINVAL;
	}

	return 0;
}

int isp_dhub_mod_close(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	int i;
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)mod_ctx->mod_prv;

	if (atomic_read(&mod_ctx->refcnt) == 0) {
		ispdhub_trace("isp dhub driver already released!\n");
		return 0;
	}

	if (atomic_dec_return(&mod_ctx->refcnt)) {
		ispdhub_trace(
			"isp dhub driver reference count after this release : %d!\n",
				atomic_read(&mod_ctx->refcnt));
		return 0;
	}

	//Remove all registered handler
	for (i = 0; i < ISP_DHUB_INTR_HANDLER_MAX; i++) {
		isp_drv_dhub_IntrUnRegisterHandler(hDhubCtx, i);
	}

	/* unregister interrupt */
	if (hDhubCtx->IsrRegistered) {
		free_irq(hDhubCtx->irq_num, (void *)mod_ctx);
		hDhubCtx->IsrRegistered = 0;
	}

	return 0;
}

int isp_dhub_mod_open(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)mod_ctx->mod_prv;
	int err = -1;
	unsigned int vec_num;

	if (atomic_inc_return(&mod_ctx->refcnt) > 1) {
		ispdhub_trace("isp dhub driver reference count : %d!\n",
					atomic_read(&mod_ctx->refcnt));
		return 0;
	}

	vec_num = hDhubCtx->irq_num;
	ispdhub_trace("%s:%d: vec_num = %x\n",
		__func__, __LINE__, vec_num);

	if (((int)vec_num) > 0 && (hDhubCtx->dhub_id < ISP_DHUB_ID_MAX)) {
		/* register and enable DHUB ISR */
		err = request_irq(vec_num, isp_devices_dhub_isr, 0,
			  isp_dhub_names[hDhubCtx->dhub_id], (void *) mod_ctx);

		if (unlikely(err < 0))
			ispdhub_trace("vec_num:%5d, err:%8x\n", vec_num, err);
		else
			hDhubCtx->IsrRegistered = 1;
	}

	return err;
}

int isp_dhub_mod_exit(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	int retVal;
	ISP_DHUB_CTX *hDhubCtx = (ISP_DHUB_CTX *)mod_ctx->mod_prv;

	isp_drv_dhub_remove_proc_file(isp_dev->dev_procdir);

	retVal = isp_drv_dhub_exit(hDhubCtx);

	return retVal;
}

int isp_dhub_mod_probe(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	int retVal;
	ISP_DHUB_CTX *hDhubCtx;

	hDhubCtx = devm_kzalloc(isp_dev->dev, sizeof(ISP_DHUB_CTX), GFP_KERNEL);
	if (!hDhubCtx)
		return -ENOMEM;

	mod_ctx->mod_name = "ISP DHUB";
	mod_ctx->mod_prv = hDhubCtx;
	atomic_set(&mod_ctx->refcnt, 0);

	hDhubCtx->dev = isp_dev->dev;
	hDhubCtx->mod_base = isp_dev->reg_base;

	retVal = isp_dhub_mod_init(isp_dev, mod_ctx);

	ispdhub_trace("%s:%d: probe %s\n",
		__func__, __LINE__, isp_module_err_str(retVal));

	return retVal;
}
