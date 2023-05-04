#ifndef __DRV_ISP_H__
#define __DRV_ISP_H__
#include "reg_if.h"

void isp_drv_core_clear_intr(int intr_num, int *status, void *mod_base);
int isp_core_mod_probe(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_core_mod_close(isp_device *isp_dev, isp_module_ctx *mod_ctx);
long isp_core_mod_ioctl(isp_device *isp_dev, isp_module_ctx *mod_ctx,
			unsigned int cmd, unsigned long arg);

#endif /* __DRV_ISP_H__ */
