#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/irq.h>
#include <linux/uaccess.h>

#include "isp_driver.h"
#include "reg_if.h"
#include "drv_isp_prv.h"
#include "drv_isp.h"
#include "drv_debug.h"
#include "ispDhub.h"

typedef struct isp_irq_params_t {
	isp_interrupt_types intr_type;
	unsigned int intr_status;
} ISP_EVENT_PARAMS;

static void isp_drv_handle_intr(ISP_CORE_CTX *hCoreCtx, int irq_bit,
	int irq_num, int intr_index)
{
	unsigned intstat = 0;
	int sig_en;

	isp_drv_core_clear_intr(irq_num, &intstat, hCoreCtx->mod_base);

	spin_lock(&hCoreCtx->intrs[intr_index].lock);
	sig_en = hCoreCtx->intrs[intr_index].intr_status ? 0 : 1;
	hCoreCtx->intrs[intr_index].intr_status |= intstat;
	spin_unlock(&hCoreCtx->intrs[intr_index].lock);

	if (hCoreCtx->intrs[intr_index].enabled && sig_en)
		up(&(hCoreCtx->intrs[intr_index].sem));

	writel(irq_bit, (hCoreCtx->mod_base)
			+ ISP_GLOBAL_BASE + ISP_INT_STATUS);
}

static irqreturn_t isp_irq_handler(int irq, void *dev_id)
{
	isp_module_ctx *mod_ctx = (isp_module_ctx *)dev_id;
	ISP_CORE_CTX *hCoreCtx = (ISP_CORE_CTX *)mod_ctx->mod_prv;
	unsigned int ra_intstat;

	ra_intstat = readl((hCoreCtx->mod_base)
				+ ISP_GLOBAL_BASE + ISP_INT_STATUS);

	if (ra_intstat & MI_IRQ_BIT)
		isp_drv_handle_intr(hCoreCtx, MI_IRQ_BIT, ispDhubSemMap_TSB_miIntr, ISP_INTR_MI);
	if (ra_intstat & ISP_IRQ_BIT)
		isp_drv_handle_intr(hCoreCtx, ISP_IRQ_BIT, ispDhubSemMap_TSB_ispIntr, ISP_INTR_ISP);

	return IRQ_HANDLED;
}

void isp_drv_core_clear_intr(int intr_num, int *status, void *mod_base)
{
	unsigned int intstat = 0;
	void *isp_core_base;

	if ((status == NULL) || (mod_base == NULL))
		return;

	isp_core_base = mod_base + ISP_CORE_BASE;
	if (intr_num == ispDhubSemMap_TSB_ispIntr) {
		intstat = readl(isp_core_base + ISP_ISP_INTR_STS_REG);
		writel(intstat, isp_core_base + ISP_ISP_INTR_CLR_REG);
	} else if (intr_num == ispDhubSemMap_TSB_miIntr) {
		intstat = readl(isp_core_base + ISP_MI_INTR_STS_REG);
		writel(intstat, isp_core_base + ISP_MI_INTR_CLR_REG);
	}
	*status = intstat;
}

static int isp_drv_core_probe(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	struct resource *r;
	ISP_CORE_CTX *hCoreCtx = (ISP_CORE_CTX *)mod_ctx->mod_prv;
	struct platform_device *pdev = to_platform_device(isp_dev->dev);

	r = platform_get_resource_byname(pdev,
			IORESOURCE_IRQ, ISP_CORE_SUB_MODULE);

	if (!r) {
		ispcore_error(
			"Failed to get interrupt for ISP Core Sub Module\n");
		return -EINVAL;
	}

	hCoreCtx->irq_num = r->start;

	return 0;
}

static int isp_drv_core_init(isp_module_ctx *mod_ctx)
{
	int i;
	ISP_CORE_CTX *hCoreCtx = (ISP_CORE_CTX *)mod_ctx->mod_prv;

	for (i = 0; i < ISP_INTR_MAX; i++) {
		hCoreCtx->intrs[i].intr_status = 0;
		sema_init(&hCoreCtx->intrs[i].sem, 0);
		spin_lock_init(&hCoreCtx->intrs[i].lock);
		hCoreCtx->intrs[i].enabled = false;
	}

	return 0;
}

static int isp_core_mod_init(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	int retVal;
	ISP_CORE_CTX *hCoreCtx = (ISP_CORE_CTX *)mod_ctx->mod_prv;

	//probe/configure ISP CORE driver
	retVal = isp_drv_core_probe(isp_dev, mod_ctx);
	if (retVal)
		return retVal;

	//init ISP CORE driver
	retVal = isp_drv_core_init(mod_ctx);
	if (retVal)
		return retVal;

	ispcore_trace("ISP Core Sub Module intitalized\n");

	return 0;
}

static int isp_core_mod_open(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	int ret;
	ISP_CORE_CTX *hCoreCtx = (ISP_CORE_CTX *)mod_ctx->mod_prv;

	if (atomic_inc_return(&mod_ctx->refcnt) > 1) {
		ispcore_trace("isp core driver reference count : %d!\n",
					atomic_read(&mod_ctx->refcnt));
		return 0;
	}

	ret = request_irq(hCoreCtx->irq_num, isp_irq_handler, 0,
				ISP_CORE_SUB_MODULE, mod_ctx);
	if (ret)
		ispcore_error(
			"Interrupt registration failed for ISP Core Sub Module\n");
	else
		hCoreCtx->IsrRegistered = 1;

	return ret;
}

int isp_core_mod_close(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	ISP_CORE_CTX *hCoreCtx = (ISP_CORE_CTX *)mod_ctx->mod_prv;

	if (atomic_read(&mod_ctx->refcnt) == 0) {
		ispcore_trace("isp driver already released!\n");
		return 0;
	}

	if (atomic_dec_return(&mod_ctx->refcnt)) {
		ispcore_trace(
			"isp driver reference count after this release : %d!\n",
				atomic_read(&mod_ctx->refcnt));
		return 0;
	}

	if (hCoreCtx->IsrRegistered) {
		free_irq(hCoreCtx->irq_num, (void *)mod_ctx);
		hCoreCtx->IsrRegistered = 0;
	}

	return 0;
}

long isp_core_mod_ioctl(isp_device *isp_dev, isp_module_ctx *mod_ctx,
			unsigned int cmd, unsigned long arg)
{
	ISP_CORE_CTX *hCoreCtx = (ISP_CORE_CTX *)mod_ctx->mod_prv;
	void __user *pInArg      = NULL;
	void __user *pOutArg     = NULL;

	pInArg = (void __user *)arg;
	pOutArg = (void __user *)arg;

	if ((cmd == ISP_MODULE_CONFIG))
		return isp_core_mod_open(isp_dev, mod_ctx);
	else if (!hCoreCtx->IsrRegistered)
		return -EPERM;

	switch (cmd) {
	case ISP_WAIT_FOR_EVENT:
	{
		unsigned long flags;
		ISP_EVENT_PARAMS wait;

		if (!copy_from_user(&wait, pInArg, sizeof(wait))) {
			if (down_interruptible(
			&hCoreCtx->intrs[wait.intr_type].sem))
				return -EINTR;

			spin_lock_irqsave(&hCoreCtx->intrs[wait.intr_type].lock, flags);
			wait.intr_status =
				hCoreCtx->intrs[wait.intr_type].intr_status;
			hCoreCtx->intrs[wait.intr_type].intr_status = 0;
			spin_unlock_irqrestore(&hCoreCtx->intrs[wait.intr_type].lock, flags);

			if (copy_to_user(pOutArg, &wait, sizeof(wait))) {
				ispcore_error("IOCTL: wait release param failed...!\n");
				return -EINVAL;
			}
		} else {
			ispcore_error("IOCTL: wait param failed...!\n");
			return -EINVAL;
		}
	}
	break;

	case ISP_EVENT_ENABLE:
	{
		ISP_EVENT_PARAMS start;

		if (!copy_from_user(&start, pInArg, sizeof(start))) {
			hCoreCtx->intrs[start.intr_type].enabled = true;
		} else {
			ispcore_error("IOCTL: event enable param failed...!\n");
			return -EINVAL;
		}
	}
	break;

	case ISP_EVENT_DISABLE:
	{
		unsigned long flags;
		ISP_EVENT_PARAMS stop;

		if (!copy_from_user(&stop, pInArg, sizeof(stop))) {
			spin_lock_irqsave(&hCoreCtx->intrs[stop.intr_type].lock, flags);
			hCoreCtx->intrs[stop.intr_type].enabled = false;
			hCoreCtx->intrs[stop.intr_type].intr_status = 0;
			spin_unlock_irqrestore(&hCoreCtx->intrs[stop.intr_type].lock, flags);
			if (down_trylock(&hCoreCtx->intrs[stop.intr_type].sem))
				up(&hCoreCtx->intrs[stop.intr_type].sem);
		} else {
			ispcore_error("IOCTL: event disable failed...!\n");
			return -EINVAL;
		}
	}
	break;

	default:
		return -EINVAL;
	}

	return 0;
}

int isp_core_mod_probe(isp_device *isp_dev, isp_module_ctx *mod_ctx)
{
	ISP_CORE_CTX *hCoreCtx;
	int ret;

	hCoreCtx = devm_kzalloc(isp_dev->dev, sizeof(ISP_CORE_CTX), GFP_KERNEL);
	if (!hCoreCtx)
		return -ENOMEM;

	mod_ctx->mod_name = "ISP_DRV";
	mod_ctx->mod_prv = hCoreCtx;
	atomic_set(&mod_ctx->refcnt, 0);

	hCoreCtx->dev = isp_dev->dev;
	hCoreCtx->mod_base = isp_dev->reg_base;

	ret = isp_core_mod_init(isp_dev, mod_ctx);

	ispcore_trace("%s:%d: probe %s\n",
		__func__, __LINE__, isp_module_err_str(ret));

	return ret;
}
