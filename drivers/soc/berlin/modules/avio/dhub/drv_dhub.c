// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/io.h>

#include "avio_memmap.h"
#include "avio_dhub_drv.h"
#include "hal_dhub_fastcall_wrap.h"
#include "drv_dhub.h"

#define DHUB_MODULE_NAME "avio_module_dhub"
#define VPP_DHUB_NAME   "vpp_dhub"
#define AG_DHUB_NAME    "ag_dhub"

void __weak drv_dhub_finalize_dhub(void *h_dhub_ctx) { }

static void drv_dhub_read_dhub_cfg(DHUB_CTX *hDhubCtx, void *dev)
{
	struct device_node *np, *iter;
	const char *dhub_node_name = "syna,berlin-dhub";
	u32 frameRate;
	struct platform_device *pdev = (struct platform_device *)dev;
	int nodeFound = 0;

	np = pdev->dev.of_node;
	if (np) {
		np = pdev->dev.of_node;
		for_each_child_of_node(np, iter) {
			if (of_device_is_compatible(iter, dhub_node_name)) {
				nodeFound = 1;
				break;
			}
		}
	}

	if (nodeFound) {
		np = iter;
		if (!of_property_read_u32(np, "frame-rate", &frameRate)) {
			avio_trace("%s:%d: frameRate - %x\n",
				__func__, __LINE__, frameRate);
			hDhubCtx->fastlogo_framerate = frameRate;
		}

		hDhubCtx->irq_num[DHUB_ID_VPP_DHUB] =
			of_irq_get_byname(np, VPP_DHUB_NAME);
		hDhubCtx->irq_num[DHUB_ID_AG_DHUB] =
			of_irq_get_byname(np, AG_DHUB_NAME);

		avio_trace("%s:%d: irq - %s:%x, %s:%x\n",
			__func__, __LINE__,
			VPP_DHUB_NAME, hDhubCtx->irq_num[DHUB_ID_VPP_DHUB],
			AG_DHUB_NAME, hDhubCtx->irq_num[DHUB_ID_AG_DHUB]);
	} else {
		avio_trace("%s:%d: Node not found %s!\n",
			__func__, __LINE__, dhub_node_name);
	}

}

static int drv_dhub_get_dhub_cfg(DHUB_CTX *hDhubCtx, void *dev)
{
	AVIO_CTX *hAvioCtx =
		(AVIO_CTX *)avio_sub_module_get_ctx(AVIO_MODULE_TYPE_AVIO);
	int ret = 0;

	//Get or read any DHUB specific configuratiaon from DTS
	drv_dhub_read_dhub_cfg(hDhubCtx, dev);

	if (!hAvioCtx->avio_base) {
		avio_error("%s:%d: AVIO Base not initialized - %x\n",
			__func__, __LINE__, hAvioCtx->avio_base);
		ret = -1;
	}

	//Note: already DTS configuration is parsed by avio module
	avio_trace("%s:%d: AVIO base : %x/%x\n",
		__func__, __LINE__, hAvioCtx->avio_base, hAvioCtx->avio_size);
	drv_dhub_config_ctx(hDhubCtx, hAvioCtx->avio_base);

	hDhubCtx->isTeeEnabled = hAvioCtx->isTeeEnabled;

	return ret;
}

static void drv_dhub_config(void *h_dhub_ctx, void *dev)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *)h_dhub_ctx;

	memset(hDhubCtx, 0, sizeof(DHUB_CTX));

	drv_dhub_get_dhub_cfg(hDhubCtx, dev);
}

static int drv_dhub_ioctl_unlocked(void *h_dhub_ctx,
			unsigned int cmd, unsigned long arg, long *retVal)
{
	int processedFlag = 1;

	if (!retVal)
		return 0;

	*retVal = 0;

	switch (cmd) {
	case ISR_IOCTL_SET_AFFINITY:
	{
		unsigned int aff_param[2] = { 0, 0 };
		DHUB_ID dhub_id = DHUB_ID_MAX;
		DHUB_CONFIG_INFO *p_dhub_cfg;
		int module_name;
		const struct cpumask *m;

		if (copy_from_user(aff_param,
			(void __user *) arg, 2 * sizeof(unsigned int)))
			AVIO_MODULE_STORE_AND_RETURN(
				retVal, -EFAULT, 1);

		module_name = aff_param[0];
		if (module_name == IRQ_VPP_MODULE)
			dhub_id = DHUB_ID_VPP_DHUB;
		else if (module_name == IRQ_AOUT_MODULE)
			dhub_id = DHUB_ID_AG_DHUB;

		avio_trace("%s:%d: aff_param-%d,%d\n",
			__func__, __LINE__, aff_param[0], aff_param[1]);

		if (module_name == DHUB_ID_MAX)
			return 0;

		p_dhub_cfg = Dhub_GetConfigInfo_ByDhubId(dhub_id);
		if (!p_dhub_cfg || !p_dhub_cfg->IsrRegistered)
			return 0;

		avio_trace("%s:%d: set aff_param to irq - %d\n",
			__func__, __LINE__, p_dhub_cfg->irq_num);

		m = get_cpu_mask(aff_param[1]);
		irq_set_affinity_hint(p_dhub_cfg->irq_num, m);
		break;
	}

	case POWER_IOCTL_DISABLE_INT:
	case POWER_IOCTL_ENABLE_INT:
	{
		int module_name = (int) (arg & 0xFFFF);
		int module_mask = (int) (arg >> 16);
		DHUB_ID dhub_id = DHUB_ID_MAX;
		DHUB_CONFIG_INFO *p_dhub_cfg;

		if (module_name == IRQ_VPP_MODULE)
			dhub_id = DHUB_ID_VPP_DHUB;
		else if (module_name == IRQ_AOUT_MODULE)
			dhub_id = DHUB_ID_AG_DHUB;

		if (module_name == DHUB_ID_MAX)
			return 0;

		p_dhub_cfg = Dhub_GetConfigInfo_ByDhubId(dhub_id);
		if (!p_dhub_cfg ||
			(p_dhub_cfg->IsrRegistered &&
			((VPP_INT_EN & module_mask) == 0)))
			return 0;

		if (cmd == POWER_IOCTL_DISABLE_INT) {
			/* disable VPP interrupt */
			disable_irq(p_dhub_cfg->irq_num);
		} else {
			/* enable VPP interrupt */
			enable_irq(p_dhub_cfg->irq_num);
		}
		break;
	}

	default:
		processedFlag = 0;
		break;
	}

	return processedFlag;
}

static irqreturn_t avio_devices_dhub_isr(int irq, void *dev_id)
{
	DHUB_CONFIG_INFO *p_dhub_cfg = (DHUB_CONFIG_INFO *) dev_id;
	HDL_semaphore *pSemHandle =
			wrap_dhub_semaphore(&p_dhub_cfg->pDhubHandle->dhub);
	DHUB_INTR_HANDLER_INFO *p_int_handler;
	DHUB_HAL_FOPS *fops = &p_dhub_cfg->fops;
	int intr_num, h_intr_num;
	UINT32 instat;
	UINT32 intr_status;

	//Get the interrupt status
	instat = fops->semaphore_chk_full(pSemHandle, -1);

	//Clear & Invoke any registered isr handlers
	while (instat) {
		//get interrupt num from instat
		intr_num = ffs(instat) - 1;

		//get interrupt handler and status
		p_int_handler = &p_dhub_cfg->intrHandler[intr_num];
		h_intr_num = p_int_handler->intrNum;
		intr_status = instat & p_int_handler->intrMask;

		//Clear/Ack interrupt
		fops->semaphore_pop(pSemHandle, h_intr_num, 1);
		fops->semaphore_clr_full(pSemHandle, h_intr_num);

		//Invoke the registered callback
		if (intr_status && p_int_handler->pIntrHandler)
			p_int_handler->pIntrHandler(intr_status,
				p_int_handler->pIntrHandlerArgs);
		else
			avio_error("%s:%d: Spurious interrupt - %d/%x/%x\n",
				__func__, __LINE__, h_intr_num,
				intr_status, p_int_handler->intrMask);

		//Clear the interrupt bit in instat
		instat &= ~(1 << intr_num);
	}

	return IRQ_HANDLED;
}

static void drv_dhub_enable_irq(void *h_dhub_ctx)
{
	DHUB_CONFIG_INFO *p_dhub_cfg;
	int i;

	for (i = 0; i < DHUB_ID_MAX; i++) {
		p_dhub_cfg = Dhub_GetConfigInfo_ByDhubId(i);
		/* disable interrupt */
		if (p_dhub_cfg && p_dhub_cfg->IsrRegistered)
			enable_irq(p_dhub_cfg->irq_num);
	}
}

static void drv_dhub_disable_irq(void *h_dhub_ctx)
{
	DHUB_CONFIG_INFO *p_dhub_cfg;
	int i;

	wrap_DhubEnableAutoPush(false, false, 0);

	for (i = 0; i < DHUB_ID_MAX; i++) {
		p_dhub_cfg = Dhub_GetConfigInfo_ByDhubId(i);
		/* disable interrupt */
		if (p_dhub_cfg && p_dhub_cfg->IsrRegistered)
			disable_irq_nosync(p_dhub_cfg->irq_num);
	}
}

static int drv_dhub_request_irq(unsigned int vec_num,
			  DHUB_ID dhub_id, const char *devname)
{
	int err = -1;
	DHUB_CONFIG_INFO *p_dhub_cfg;

	p_dhub_cfg = Dhub_GetConfigInfo_ByDhubId(dhub_id);
	if (!p_dhub_cfg)
		return err;

	p_dhub_cfg->irq_num = vec_num;
	avio_trace("%s:%d: vec_num = %x\n",
		__func__, __LINE__, p_dhub_cfg->irq_num);

	/* register and enable DHUB ISR */
	err = request_irq(vec_num, avio_devices_dhub_isr, 0,
			  devname, (void *) p_dhub_cfg);
	if (unlikely(err < 0))
		avio_trace("vec_num:%5d, err:%8x\n", vec_num, err);
	else
		p_dhub_cfg->IsrRegistered = 1;

	return err;
}

static int drv_dhub_open(void *h_dhub_ctx)
{
	int err = -1;
	unsigned int vpp_vec_num, vec_num;
	DHUB_CTX *hDhubCtx = (DHUB_CTX *)h_dhub_ctx;

	err = drv_dhub_initialize_dhub(h_dhub_ctx);
	if (err) {
		avio_trace("%s: failed: %x\n", __func__, err);
		return err;
	}

	vpp_vec_num = vec_num = hDhubCtx->irq_num[DHUB_ID_VPP_DHUB];
	if (((int)vec_num) > 0)
		err = drv_dhub_request_irq(vec_num,
					DHUB_ID_VPP_DHUB, VPP_DHUB_NAME);

	vec_num = hDhubCtx->irq_num[DHUB_ID_AG_DHUB];
	if (!err && (((int)vec_num) > 0)) {
		err = drv_dhub_request_irq(vec_num,
					DHUB_ID_AG_DHUB, AG_DHUB_NAME);
		if (unlikely(err < 0))
			goto free_irq_1;
	}

	return err;

free_irq_1:
	free_irq(vpp_vec_num, (void *) NULL);
	return err;
}

static void drv_dhub_close(void *h_dhub_ctx)
{
	DHUB_CONFIG_INFO *p_dhub_cfg;
	int i;

	wrap_DhubEnableAutoPush(false, false, 0);

	for (i = 0; i < DHUB_ID_MAX; i++) {
		p_dhub_cfg = Dhub_GetConfigInfo_ByDhubId(i);
		if (p_dhub_cfg && p_dhub_cfg->IsrRegistered) {
			/* Clean up affinity_hint */
			irq_set_affinity_hint(p_dhub_cfg->irq_num, NULL);
			/* unregister interrupt */
			free_irq(p_dhub_cfg->irq_num, (void *)p_dhub_cfg);
			p_dhub_cfg->IsrRegistered = 0;
		}
	}
	drv_dhub_finalize_dhub(h_dhub_ctx);
}

static const AVIO_MODULE_FUNC_TABLE dhub_drv_fops = {
	.config = drv_dhub_config,
	.ioctl_unlocked	 = drv_dhub_ioctl_unlocked,
	.open	    = drv_dhub_open,
	.close	    = drv_dhub_close,
	.enable_irq		 = drv_dhub_enable_irq,
	.disable_irq		= drv_dhub_disable_irq,
};

int avio_module_drv_dhub_probe(struct platform_device *pdev)
{
	DHUB_CTX *hDhubCtx;

	hDhubCtx = devm_kzalloc(&pdev->dev, sizeof(DHUB_CTX), GFP_KERNEL);
	if (!hDhubCtx)
		return -ENOMEM;

	spin_lock_init(&hDhubCtx->dhub_cfg_spinlock);

	avio_sub_module_register(AVIO_MODULE_TYPE_DHUB,
			DHUB_MODULE_NAME, hDhubCtx, &dhub_drv_fops);

	return 0;
}

