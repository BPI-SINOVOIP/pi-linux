/*************************************************************************/ /*
 UVCS Driver (kernel module)

 Copyright (C) 2015 - 2024 Renesas Electronics Corporation

 License        Dual MIT/GPLv2

 The contents of this file are subject to the MIT license as set out below.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 Alternatively, the contents of this file may be used under the terms of
 the GNU General Public License Version 2 ("GPL") in which case the provisions
 of GPL are applicable instead of those above.

 If you wish to allow use of your version of this file only under the terms of
 GPL, and not to allow others to use your version of this file under the terms
 of the MIT license, indicate your decision by deleting the provisions above
 and replace them with the notice and other provisions required by GPL as set
 out in the file called "GPL-COPYING" included in this distribution. If you do
 not delete the provisions above, a recipient may use your version of this file
 under the terms of either the MIT license or GPL.

 This License is also included in this distribution in the file called
 "MIT-COPYING".

 EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 GPLv2:
 If you wish to use this file under the terms of GPL, following terms are
 effective.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/ /*************************************************************************/


/******************************************************************************/
/*                    INCLUDE FILES                                           */
/******************************************************************************/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <linux/sys_soc.h>
#include <linux/reset.h>
#include "uvcs_types.h"
#include "uvcs_cmn.h"
#include "uvcs_lkm_internal.h"
#include "mcvx_types.h"
#include "mcvx_api.h"

#ifdef PSEUDO_DEV
#error
#endif

/******************************************************************************/
/*                    LOCAL TYPES                                             */
/******************************************************************************/

/******************************************************************************/
/*                    VARIABLES                                               */
/******************************************************************************/
static uint ip_option = UVCS_IPOPT_DEFAULT;
static uint print_baa_mode;
static uint print_vcp_info;
static uint independent_mode;
static uint force_vlc_parallel;
module_param(ip_option, uint, 0000);
module_param(print_baa_mode, uint, 0000);
module_param(print_vcp_info, uint, 0000);
module_param(independent_mode, uint, 0000);
module_param(force_vlc_parallel, uint, 0000);

static uint v_serial[UVCS_CMN_MAX_HW_NUM]; /**< debug information. VLC execute counter */
static uint c_serial[UVCS_CMN_MAX_HW_NUM]; /**< debug information. CE execute counter */

static struct uvcs_driver_info *driver_info;

static const struct soc_device_attribute r8a7795[] = {
	{ .soc_id = "r8a7795" },
	{ },
};

static const struct soc_device_attribute r8a7796[] = {
	{ .soc_id = "r8a7796" },
	{ },
};

static const struct soc_device_attribute r8a77965[] = {
	{ .soc_id = "r8a77965" },
	{ },
};

static const struct soc_device_attribute r9a09g055ma3gbg[] = {
	{ .soc_id = "r9a09g055ma3gbg" },
	{ },
};

static const struct soc_device_attribute r9a09g056[] = {
        { .soc_id = "r9a09g056*" },
        { },
};

static const struct soc_device_attribute r9a09g057[] = {
	{ .soc_id = "r9a09g057*" },
	{ },
};

static const struct soc_device_attribute device_es1x[] = {
	{ .revision = "ES1.*" },
	{ },
};

static const struct soc_device_attribute device_es2x[] = {
	{ .revision = "ES2.*" },
	{ },
};

/******************************************************************************/
/*                    FORWARD DECLARATIONS                                    */
/******************************************************************************/
static irqreturn_t uvcs_vlc_int_handler(int irq, void *dev);
static irqreturn_t uvcs_ce_int_handler(int irq, void *dev);
static void uvcs_vlc_timer_handler(struct timer_list *data_ptr);
static void uvcs_ce_timer_handler(struct timer_list *data_ptr);

static int uvcs_vcp_request_irq(struct platform_device *pdev);
static void uvcs_vcp_free_irq(struct platform_device *pdev);
static int uvcs_clock_power_enable(struct platform_device *pdev);
static void uvcs_clock_power_disable(struct platform_device *pdev);
static void uvcs_get_lsi_info(struct uvcs_driver_info *drv);


/******************************************************************************/
/*                    FUNCTIONS                                               */
/******************************************************************************/

/**
 * \brief Interrupt handler for VLC module
 *
 * \retval IRQ_HANDLED	success
 * \retval IRQ_NONE		unhandled
 */
static irqreturn_t uvcs_vlc_int_handler(
	int irq, /**< [in] interrupted IRQ number */
	void *dev /**< [in] platform device */
	)
{
	if (driver_info) {
		struct platform_device *pdev = dev;
		struct timespec64 ts = {0u};
		int i;
		unsigned long flags;

		ktime_get_raw_ts64(&ts);
		for (i = 0; i < driver_info->vcp_devnum; i++) {
			if ((driver_info->vcpinf[i].irq_vlc == irq)
			&&	(driver_info->vcpinf[i].pdev == pdev)) {
				spin_lock_irqsave(&driver_info->vcpinf[i].slock_vlc, flags);
				del_timer(&driver_info->vcpinf[i].timer_vlc);
				iowrite32(0u, driver_info->vcpinf[i].reg_vlc + UVCS_VCPREG_IRQENB);
				uvcs_cmn_vlc_interrupt(driver_info->uvcs_info, i, ts.tv_nsec, UVCS_FALSE);
				spin_unlock_irqrestore(&driver_info->vcpinf[i].slock_vlc, flags);
				return IRQ_HANDLED;
			}
		}
	}

	return IRQ_NONE;
}

/**
 * \brief Interrupt handler for CE module
 *
 * \retval IRQ_HANDLED	success
 * \retval IRQ_NONE		unhandled
 */
static irqreturn_t uvcs_ce_int_handler(
	int irq, /**< [in] interrupted IRQ number */
	void *dev /**< [in] platform device */
	)
{
	if (driver_info) {
		struct platform_device *pdev = dev;
		struct timespec64 ts = {0u};
		int i;
		unsigned long flags;

		ktime_get_raw_ts64(&ts);
		for (i = 0; i < driver_info->vcp_devnum; i++) {
			if ((driver_info->vcpinf[i].irq_ce == irq)
			&&	(driver_info->vcpinf[i].pdev == pdev)) {
				spin_lock_irqsave(&driver_info->vcpinf[i].slock_ce, flags);
				del_timer(&driver_info->vcpinf[i].timer_ce);
				iowrite32(0u, driver_info->vcpinf[i].reg_ce + UVCS_VCPREG_IRQENB);
				uvcs_cmn_ce_interrupt(driver_info->uvcs_info, i, ts.tv_nsec, UVCS_FALSE);
				spin_unlock_irqrestore(&driver_info->vcpinf[i].slock_ce, flags);
				return IRQ_HANDLED;
			}
		}
	}

	return IRQ_NONE;
}

/**
 * \brief Timer handler for VLC module (timeout VCP processing)
 */
static void uvcs_vlc_timer_handler(
	struct timer_list *data_ptr /**< [in] hardware identifier */
	)
{
	struct uvcs_vcp_hwinf *vcpinfo = from_timer(vcpinfo, data_ptr, timer_vlc);
	u64 data = vcpinfo -> timer_vlc_data;
	if (driver_info) {
		struct timespec64 ts = {0u};
		unsigned long flags;

		ktime_get_raw_ts64(&ts);
		if (data < driver_info->vcp_devnum) {
			spin_lock_irqsave(&driver_info->vcpinf[data].slock_vlc, flags);
			del_timer(&driver_info->vcpinf[data].timer_vlc);
			iowrite32(0u, driver_info->vcpinf[data].reg_vlc + UVCS_VCPREG_IRQENB);
			uvcs_cmn_vlc_interrupt(driver_info->uvcs_info, data, ts.tv_nsec, UVCS_TRUE);
			spin_unlock_irqrestore(&driver_info->vcpinf[data].slock_vlc, flags);
		}
	}
}

/**
 * \brief Timer handler for CE module (timeout VCP processing)
 */
static void uvcs_ce_timer_handler(
	struct timer_list *data_ptr /**< [in] hardware identifier */
	)
{
	struct uvcs_vcp_hwinf *vcpinfo = from_timer(vcpinfo, data_ptr, timer_ce);
	u64 data = vcpinfo -> timer_ce_data;
	if (driver_info) {
		struct timespec64 ts = {0u};
		unsigned long flags;

		ktime_get_raw_ts64(&ts);
		if (data < driver_info->vcp_devnum) {
			spin_lock_irqsave(&driver_info->vcpinf[data].slock_ce, flags);
			del_timer(&driver_info->vcpinf[data].timer_ce);
			iowrite32(0u, driver_info->vcpinf[data].reg_ce + UVCS_VCPREG_IRQENB);
			uvcs_cmn_ce_interrupt(driver_info->uvcs_info, data, ts.tv_nsec, UVCS_TRUE);
			spin_unlock_irqrestore(&driver_info->vcpinf[data].slock_ce, flags);
		}
	}
}

/**
 * \brief (Callback function) Register read
 *
 * This function is called from uvcs driver when reading data from target register is needed
 */
static void uvcs_register_read(
		UVCS_PTR udptr, /**< [in] driver control information */
		volatile UVCS_U32 *reg_addr, /**< [in] register address */
		UVCS_U32 *dst_addr, /**< [out] read data */
		UVCS_U32 num_reg /**< [in] number of data to read */
		)
{
	while (num_reg > 0) {
		*dst_addr++ = ioread32(reg_addr++);
		rmb();
		num_reg--;
	}
}

/**
 * \brief (Callback function) Register write
 *
 * This function is called from uvcs driver when writing data to target register is needed.
 */
static void uvcs_register_write(
		UVCS_PTR udptr, /**< [in] driver control information */
		volatile UVCS_U32 *reg_addr, /**< [in] register address */
		UVCS_U32 *src_addr, /**< [in] target data */
		UVCS_U32 num_reg /**< [in] number of data to write */
		)
{
	while (num_reg > 0) {
		iowrite32(*src_addr++, reg_addr++);
		wmb();
		num_reg--;
	}
}

/**
 * \brief (Callback function) Start timing of VCP processing
 *
 * This function is called from uvcs driver when processing of VCP hardware will start
 */
static void uvcs_hw_start(
		UVCS_PTR  udptr, /**< [in] driver control information */
		UVCS_U32  hw_ip_id, /**< [in] hardware identifier */
		UVCS_U32  hw_module_id, /**< [in] hardware module identifier */
		UVCS_U32 *baa /**< [in] debug information */
		)
{
	struct uvcs_driver_info *drv = (struct uvcs_driver_info *)udptr;

	if (hw_ip_id < drv->vcp_devnum) {
		/* set kernel timer */
		if (hw_module_id == UVCS_CMN_BASE_ADDR_VLC) {
			drv->vcpinf[hw_ip_id].timer_vlc.expires = jiffies + UVCS_TIMEOUT_TIMER;
			drv->vcpinf[hw_ip_id].timer_vlc_data = hw_ip_id;
			add_timer(&drv->vcpinf[hw_ip_id].timer_vlc);
		} else {
			drv->vcpinf[hw_ip_id].timer_ce.expires = jiffies + UVCS_TIMEOUT_TIMER;
			drv->vcpinf[hw_ip_id].timer_ce_data = hw_ip_id;
			add_timer(&drv->vcpinf[hw_ip_id].timer_ce);
		}
	}

	/* debug */
	if (hw_module_id == UVCS_CMN_BASE_ADDR_VLC) {
		if ((print_baa_mode & UVCS_DEBUG_PRINT_VLC)
		&&	(baa != NULL)) {
			MCVX_VLC_BAA_T *v = (MCVX_VLC_BAA_T *)baa;

			printk(KERN_INFO "\n");
			printk(KERN_INFO "(v)irp_v_addr        = %p\n",  v->irp_v_addr);
			printk(KERN_INFO "(v)irp_p_addr        = %lx\n", (long)v->irp_p_addr);
			printk(KERN_INFO "(v)list_item_v_addr  = %p\n",  v->list_item_v_addr);
			printk(KERN_INFO "(v)list_item_p_addr  = %lx\n", (long)v->list_item_p_addr);
			printk(KERN_INFO "(v)imc_buff_addr     = %lx\n", (long)v->imc_buff_addr);
			printk(KERN_INFO "(v)imc_buff_size     = %lx\n", (long)v->imc_buff_size);
			printk(KERN_INFO "(v)ims_buff_addr[0]  = %lx\n", (long)v->ims_buff_addr[0]);
			printk(KERN_INFO "(v)ims_buff_size[0]  = %lx\n", (long)v->ims_buff_size[0]);
			printk(KERN_INFO "(v)lm_vlc_mbi_addr   = %lx\n", (long)v->lm_vlc_mbi_addr);
			printk(KERN_INFO "(v)str_es_addr[0]    = %lx\n", (long)v->str_es_addr[0]);
			printk(KERN_INFO "(v)str_es_size[0]    = %lx\n", (long)v->str_es_size[0]);
			printk(KERN_INFO "(v)dec_dp_addr       = %lx\n", (long)v->dec_dp_addr);
			printk(KERN_INFO "(v)dec_bp_addr       = %lx\n", (long)v->dec_bp_addr);
			printk(KERN_INFO "(v)dec_prob_r_addr   = %lx\n", (long)v->dec_prob_r_addr);
			printk(KERN_INFO "(v)dec_prob_w_addr   = %lx\n", (long)v->dec_prob_w_addr);
			printk(KERN_INFO "(v)dec_segm_r_addr   = %lx\n", (long)v->dec_segm_r_addr);
			printk(KERN_INFO "(v)dec_segm_w_addr   = %lx\n", (long)v->dec_segm_w_addr);
			printk(KERN_INFO "(v)dec_mai_addr      = %lx\n", (long)v->dec_mai_addr);
			printk(KERN_INFO "(v)dec_mv_w_addr     = %lx\n", (long)v->dec_mv_w_addr);
			printk(KERN_INFO "(v)dec_mv_r_addr[0]  = %lx\n", (long)v->dec_mv_r_addr[0]);
			printk(KERN_INFO "(v)userid            = %lx\n", (long)ioread32(drv->vcpinf[hw_ip_id].reg_vlc + 0x58));
		}
		if (print_baa_mode & UVCS_DEBUG_PRINT_EXE)
			printk(KERN_INFO "(v)exec serial       = %d\n", v_serial[hw_ip_id]++);

	} else {
		if ((print_baa_mode & UVCS_DEBUG_PRINT_CE)
		&&	(baa != NULL)) {
			MCVX_CE_BAA_T *c = (MCVX_CE_BAA_T *)baa;

			printk(KERN_INFO "(c)irp_v_addr        = %p\n",  c->irp_v_addr);
			printk(KERN_INFO "(c)irp_p_addr        = %lx\n", (long)c->irp_p_addr);
			printk(KERN_INFO "(c)imc_buff_addr     = %lx\n", (long)c->imc_buff_addr);
			printk(KERN_INFO "(c)imc_buff_size     = %lx\n", (long)c->imc_buff_size);
			printk(KERN_INFO "(c)ims_buff_addr[0]  = %lx\n", (long)c->ims_buff_addr[0]);
			printk(KERN_INFO "(c)ims_buff_size[0]  = %lx\n", (long)c->ims_buff_size[0]);
			printk(KERN_INFO "(c)lm_ce_mbi_addr    = %lx\n", (long)c->lm_ce_mbi_addr);
			printk(KERN_INFO "(c)lm_ce_prd_addr    = %lx\n", (long)c->lm_ce_prd_addr);
			printk(KERN_INFO "(c)lm_ce_ovt_addr    = %lx\n", (long)c->lm_ce_ovt_addr);
			printk(KERN_INFO "(c)lm_ce_deb_addr    = %lx\n", (long)c->lm_ce_deb_addr);
			printk(KERN_INFO "(c)img_flt_t(Yp)     = %lx\n", (long)c->img_flt_top.Ypic_addr);
			printk(KERN_INFO "(c)img_flt_t(Cp)     = %lx\n", (long)c->img_flt_top.Cpic_addr);
			printk(KERN_INFO "(c)img_flt_t(Ya)     = %lx\n", (long)c->img_flt_top.Yanc_addr);
			printk(KERN_INFO "(c)img_flt_t(Ca)     = %lx\n", (long)c->img_flt_top.Canc_addr);
			printk(KERN_INFO "(c)img_flt_b(Yp)     = %lx\n", (long)c->img_flt_bot.Ypic_addr);
			printk(KERN_INFO "(c)img_flt_b(Cp)     = %lx\n", (long)c->img_flt_bot.Cpic_addr);
			printk(KERN_INFO "(c)img_flt_b(Ya)     = %lx\n", (long)c->img_flt_bot.Yanc_addr);
			printk(KERN_INFO "(c)img_flt_b(Ca)     = %lx\n", (long)c->img_flt_bot.Canc_addr);
			printk(KERN_INFO "(c)img_ref_num       = %lx\n", (long)c->img_ref_num);
			printk(KERN_INFO "(c)img_ref0_addr(Yp) = %lx\n", (long)c->img_ref[0].Ypic_addr);
			printk(KERN_INFO "(c)img_ref0_addr(Cp) = %lx\n", (long)c->img_ref[0].Cpic_addr);
			printk(KERN_INFO "(c)img_ref0_addr(Ya) = %lx\n", (long)c->img_ref[0].Yanc_addr);
			printk(KERN_INFO "(c)img_ref0_addr(Ca) = %lx\n", (long)c->img_ref[0].Canc_addr);
			printk(KERN_INFO "(c)img_ref1_addr(Yp) = %lx\n", (long)c->img_ref[1].Ypic_addr);
			printk(KERN_INFO "(c)img_ref1_addr(Cp) = %lx\n", (long)c->img_ref[1].Cpic_addr);
			printk(KERN_INFO "(c)img_ref1_addr(Ya) = %lx\n", (long)c->img_ref[1].Yanc_addr);
			printk(KERN_INFO "(c)img_ref1_addr(Ca) = %lx\n", (long)c->img_ref[1].Canc_addr);
			printk(KERN_INFO "(c)stride_dec        = %lx\n", (long)c->stride_dec);
			printk(KERN_INFO "(c)stride_ref        = %lx\n", (long)c->stride_ref);
			printk(KERN_INFO "(c)stride_flt        = %lx\n", (long)c->stride_flt);
			printk(KERN_INFO "(c)userid            = %lx\n", (long)ioread32(drv->vcpinf[hw_ip_id].reg_ce + 0x5c));
		}
		if (print_baa_mode & UVCS_DEBUG_PRINT_EXE)
			printk(KERN_INFO "(c)exec serial       = %d\n", c_serial[hw_ip_id]++);

	}
}

/**
 * \brief (Callback function) Stop timing of VCP processing
 *
 * This function is called from uvcs driver when processing on VCP hardware is finished
 */
static void uvcs_hw_stop(
		UVCS_PTR  udptr, /**< [in] driver control information */
		UVCS_U32  hw_ip_id, /**< [in] hardware identifier */
		UVCS_U32  hw_module_id /**< [in] hardware module identifier */
		)
{
	/* power management code or hw-cache setting code is implemented here */
}

/**
 * \brief (Callback function) Resets VCP hardware
 *
 * This function is called from uvcs driver when resetting of VCP hardware is needed.
 */
static void uvcs_hw_reset(
	UVCS_PTR  udptr, /**< [in] driver control information */
	UVCS_U32  hw_ip_id /**< [in] hardware identifier */
	)
{
	struct uvcs_driver_info *drv = (struct uvcs_driver_info *)udptr;
    int result = -EFAULT;
    if (hw_ip_id < UVCS_CMN_MAX_HW_NUM)
    {
        result = device_reset_optional(&drv->pdev[hw_ip_id]->dev);
        if (result)
        {
            pr_err("reset failed (id = %d)\n", hw_ip_id);
        }
    }
}

/**
 * \brief Iniailize IO layer. Acquire IRQ, Enable power and clock
 *
 * \retval 0		success
 * \retval other	error
 */
int uvcs_io_init(
	struct uvcs_driver_info *drv /**< [in] driver control information */
	)
{
	int result = -EFAULT;
	UVCS_RESULT uvcs_result;
	UVCS_CMN_INIT_PARAM_T *iparam;
	u32 actnum;
	u32 i;

	if (!drv)
		goto err_exit_0;

	/* power */
	for (actnum = 0; actnum < drv->vcp_devnum + drv->fcp_devnum; actnum++) {
		result = uvcs_clock_power_enable(drv->pdev[actnum]);
		if (result) {
			pr_err("pm failed (id = %d)\n", actnum);
			goto err_exit_1;
		}
		result = device_reset_optional(&drv->pdev[actnum]->dev);
		if (result) {
			pr_err("reset failed (id = %d)\n", actnum);
			goto err_exit_1;
		}
	}

	uvcs_get_lsi_info(drv);

	iparam = &drv->uvcs_init_param;
	for (i = 0; i < drv->vcp_devnum; i++) {
		iparam->ip_base_addr[i][UVCS_CMN_BASE_ADDR_VLC] = drv->vcpinf[i].reg_vlc;
		iparam->ip_base_addr[i][UVCS_CMN_BASE_ADDR_CE] = drv->vcpinf[i].reg_ce;
		iparam->ip_arch[i] = drv->vcpinf[i].iparch;
		iparam->ip_group_id[i][UVCS_CMN_BASE_ADDR_VLC] = drv->vcpinf[i].ipgroup;
		iparam->ip_group_id[i][UVCS_CMN_BASE_ADDR_CE] = drv->vcpinf[i].ipgroup;
	}
	for (i = 0; i < drv->fcp_devnum; i++) {
		iparam->fcpc_base_addr[i] = drv->fcpinf[i].reg_fcp;
		iparam->fcpc_arch[i] = drv->fcpinf[i].iparch;
	}

	switch (drv->lsi_info.product_id) {
	case UVCS_LSITYPE_M3W:
		switch (drv->lsi_info.cut_id) {
		case 0x00:
			if (independent_mode) {
				iparam->ip_group_id[0][UVCS_CMN_BASE_ADDR_VLC] = 1u;
				iparam->ip_group_id[0][UVCS_CMN_BASE_ADDR_CE] = 1u;
				iparam->fcpc_indep_mode = UVCS_TRUE;
			}
			break;

		default:
			iparam->ip_group_id[0][UVCS_CMN_BASE_ADDR_VLC] = 1u;
			break;
		}
		break;

	case UVCS_LSITYPE_H3:
		switch (drv->lsi_info.cut_id) {
		case 0x00:
			break;

		case 0x10:
			if (force_vlc_parallel != 0) {
				iparam->ip_group_id[0][UVCS_CMN_BASE_ADDR_VLC] = 1u;
			}
			break;

		default:
			iparam->ip_group_id[0][UVCS_CMN_BASE_ADDR_VLC] = 1u;
			break;
		}
		break;

	case UVCS_LSITYPE_M3N:
	case UVCS_LSITYPE_E3:
	case UVCS_LSITYPE_V2MA:
	case UVCS_LSITYPE_V2H:
	case UVCS_LSITYPE_V2N:
	default:
		break;
	}

	/* init */
	iparam->struct_size    = sizeof(UVCS_CMN_INIT_PARAM_T);
	iparam->udptr          = drv;
	iparam->hw_num         = drv->vcp_devnum;
	iparam->fcpc_hw_num    = drv->fcp_devnum;
	iparam->cb_hw_start    = &uvcs_hw_start;
	iparam->cb_hw_stop     = &uvcs_hw_stop;
	iparam->cb_hw_reset    = &uvcs_hw_reset;
	iparam->cb_proc_done   = &uvcs_hw_processing_done;
	iparam->cb_sem_lock    = &uvcs_semaphore_lock;
	iparam->cb_sem_unlock  = &uvcs_semaphore_unlock;
	iparam->cb_sem_create  = &uvcs_semaphore_create;
	iparam->cb_sem_destroy = &uvcs_semaphore_destroy;
	iparam->cb_thr_event   = &uvcs_thread_event;
	iparam->cb_thr_create  = &uvcs_thread_create;
	iparam->cb_thr_destroy = &uvcs_thread_destroy;
	iparam->cb_reg_read    = &uvcs_register_read;
	iparam->cb_reg_write   = &uvcs_register_write;
	iparam->ip_option      = ip_option;

	uvcs_result = uvcs_cmn_initialize(iparam, &drv->uvcs_info);
	if (uvcs_result != UVCS_RTN_OK) {
		result = -EFAULT;
		pr_err("failed to initialize %d\n", uvcs_result);
		goto err_exit_1;
	}

	drv->ip_info.struct_size = sizeof(UVCS_CMN_IP_INFO_T);
	uvcs_result = uvcs_cmn_get_ip_info(drv->uvcs_info, &drv->ip_info);
	if (uvcs_result != UVCS_RTN_OK) {
		result = -EINVAL;
		pr_err("failed to get vcp information %d\n", uvcs_result);
		goto err_exit_2;
	}

	drv->ip_cap.struct_size = sizeof(UVCS_CMN_IP_CAPABILITY_T);
	uvcs_result = uvcs_cmn_get_ip_capability(drv->uvcs_info, &drv->ip_cap);
	if (uvcs_result != UVCS_RTN_OK) {
		result = -EINVAL;
		pr_err("failed to get vcp capability information %d\n", uvcs_result);
		goto err_exit_2;
	}

	/* enable interrupt */
	for (i = 0; i < drv->vcp_devnum; i++) {
		result = uvcs_vcp_request_irq(drv->vcpinf[i].pdev);
		if (result)
			goto err_exit_3;
	}

	if (print_vcp_info) {
		printk(KERN_INFO "vcp info\n");
		printk(KERN_INFO " product_id = 0x%04x\n", drv->lsi_info.product_id);
		printk(KERN_INFO " cut_id     = 0x%04x\n", drv->lsi_info.cut_id);
		for (i = 0; i < drv->vcp_devnum; i++) {
			printk(KERN_INFO " capa[%d] = 0x%08x\n",
						i, drv->ip_cap.ip_capability[i]);
			printk(KERN_INFO " arch[%d] = %d\n", i, drv->vcpinf[i].iparch);
		}
	}

	driver_info = drv;
	return 0;

err_exit_3:
	for (; i > 0; i--)
		uvcs_vcp_free_irq(drv->vcpinf[i - 1u].pdev);

err_exit_2:
	uvcs_cmn_deinitialize(drv->uvcs_info, UVCS_TRUE);

err_exit_1:
	for (; actnum > 0; actnum--)
		uvcs_clock_power_disable(drv->pdev[actnum - 1u]);

err_exit_0:
	return result;
}


/**
 * \brief Deinitialize driver (release IRQ, Disable power and clock)
 */
void uvcs_io_deinit(
	struct uvcs_driver_info *drv /**< [in] driver information */
	)
{
	u32 i;

	if (drv) {
		for (i = drv->vcp_devnum; i > 0; i--)
			uvcs_vcp_free_irq(drv->vcpinf[i - 1u].pdev);
		uvcs_cmn_deinitialize(drv->uvcs_info, UVCS_TRUE);
		for (i = drv->vcp_devnum + drv->fcp_devnum; i > 0; i--)
			uvcs_clock_power_disable(drv->pdev[i - 1u]);
	}

	driver_info = NULL;
}

/******************************************************************************/
/*              lsi type                                                      */
/******************************************************************************/
/**
 * \brief gets LSI information
 */
static void uvcs_get_lsi_info(
	struct uvcs_driver_info *drv /**< driver information */
	)
{
	if (drv != NULL) {
		drv->lsi_info.struct_size = sizeof(struct UVCS_CMN_LSI_INFO);

		if (soc_device_match(r8a7795)) {
			drv->lsi_info.product_id = UVCS_LSITYPE_H3;
		} else if (soc_device_match(r8a7796)) {
			drv->lsi_info.product_id = UVCS_LSITYPE_M3W;
		} else if (soc_device_match(r9a09g055ma3gbg)) {
			drv->lsi_info.product_id = UVCS_LSITYPE_V2MA;
		} else if (soc_device_match(r9a09g057)) {
			drv->lsi_info.product_id = UVCS_LSITYPE_V2H;
		} else if (soc_device_match(r9a09g056))
		{
			drv->lsi_info.product_id = UVCS_LSITYPE_V2N;
		} else {
			drv->lsi_info.product_id = 0; /* UVCS_LSITYPE_M3N / UVCS_LSITYPE_E3 */
		}

		if (soc_device_match(device_es1x)) {
			drv->lsi_info.cut_id = 0;

		} else if (soc_device_match(device_es2x)) {
			drv->lsi_info.cut_id = 0x10u;

		} else {
			drv->lsi_info.cut_id = 0x20u;
		}
	}
}

/******************************************************************************/
/*              request irq                                                   */
/******************************************************************************/
/**
 * \brief install interrupt handler
 *
 * \retval 0		 success
 * \retval other	 failed
 */
static int uvcs_vcp_request_irq(
	struct platform_device *pdev /**< [in] target platform device */
	)
{
	struct uvcs_platform_data *pdata;
	struct uvcs_vcp_hwinf *vcpinf;
	int result = -EFAULT;

	if ((pdev == NULL)
	||	(pdev->dev.platform_data == NULL))
		goto err_exit_0;

	pdata = pdev->dev.platform_data;
	if (pdata->hwinf == NULL)
		goto err_exit_0;

	vcpinf = (struct uvcs_vcp_hwinf *)pdata->hwinf;
	vcpinf->irq_enable = false;
	result = request_irq(vcpinf->irq_vlc,
					&uvcs_vlc_int_handler,
					IRQF_SHARED, vcpinf->irq_name_vlc,
					pdev);
	if (result) {
		dev_err(&pdev->dev, "failed to request irq %u,v %d\n", pdata->device_id, result);
		goto err_exit_1;
	}

	result = request_irq(vcpinf->irq_ce,
					&uvcs_ce_int_handler,
					IRQF_SHARED, vcpinf->irq_name_ce,
					pdev);
	if (result) {
		dev_err(&pdev->dev, "failed to request irq %u,c %d\n", pdata->device_id, result);
		goto err_exit_2;
	}

	timer_setup(&vcpinf->timer_vlc,uvcs_vlc_timer_handler,0);
	timer_setup(&vcpinf->timer_ce,uvcs_ce_timer_handler,0);
	spin_lock_init(&vcpinf->slock_vlc);
	spin_lock_init(&vcpinf->slock_ce);
	vcpinf->irq_enable = true;
	return 0;

err_exit_2:
	free_irq(vcpinf->irq_vlc, pdev);

err_exit_1:
err_exit_0:
	return result;
}

/**
 * \brief release interrupt
 */
static void uvcs_vcp_free_irq(
	struct platform_device *pdev /**< [in] target platform device */
	)
{
	if ((pdev != NULL)
	&&	(pdev->dev.platform_data != NULL)) {
		struct uvcs_platform_data *pdata = pdev->dev.platform_data;
		struct uvcs_vcp_hwinf *vcpinf = (struct uvcs_vcp_hwinf *)pdata->hwinf;

		if ((vcpinf != NULL)
		&&	(vcpinf->irq_enable)) {
			free_irq(vcpinf->irq_vlc, pdev);
			free_irq(vcpinf->irq_ce, pdev);
			vcpinf->irq_enable = false;
		}
	}
}

/******************************************************************************/
/*              power management                                              */
/******************************************************************************/
/**
 * \brief turn on power and clock
 *
 * \retval 0		success
 * \retval other	failed
 */
static int uvcs_clock_power_enable(
	struct platform_device *pdev /**< [in] target platform device */
	)
{
	struct uvcs_platform_data *pdata;
	int result = -EFAULT;

	if ((pdev == NULL)
	||	(pdev->dev.platform_data == NULL))
		goto err_exit_0;

	pdata = pdev->dev.platform_data;
	if ((pdata == NULL)
	||	(pdata->hwinf == NULL))
		goto err_exit_0;

	result = pm_runtime_get_sync(&pdev->dev);

err_exit_0:
	return result;
}

/**
 * \brief turn off power and clock
 */
static void uvcs_clock_power_disable(
	struct platform_device *pdev /**< [in] target platform device */
	)
{
	struct uvcs_platform_data *pdata;

	if ((pdev == NULL)
	||	(pdev->dev.platform_data == NULL))
		return;

	pdata = pdev->dev.platform_data;
	if (pdata->hwinf == NULL)
		return;

	pm_runtime_put_sync(&pdev->dev);
}

/******************************************************************************/
/*              resource (DT)                                                 */
/******************************************************************************/
/**
 * \brief gets resources for VCP module from device-tree.
 *
 * \retval 0		 success
 * \retval other	 failed
 */
int uvcs_get_vcp_resource(
	struct platform_device *pdev,	/**< [in] platform device for VCP */
	struct uvcs_vcp_hwinf *vcpinf,	/**< [out] VCP control information */
	u32 iparch, /**< [in] architecture type of VCP */
	const char *name /**< [in] device name */
	)
{
	int result = -EFAULT;
	struct resource *res;
	int ch;

#if !(LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0))
	int irq;
#endif

	if ((pdev == NULL) || (vcpinf == NULL))
		goto err_exit_0;

	vcpinf->reg_vlc = NULL;
	vcpinf->reg_ce = NULL;
	result = -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "resource not found (reg_0, %u)\n", iparch);
		goto err_exit_1;
	}
	vcpinf->pa_vlc = (u64)res->start;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res == NULL) {
		dev_err(&pdev->dev, "resource not found (reg_1, %u)\n", iparch);
		goto err_exit_1;
	}
	vcpinf->pa_ce = (u64)res->start;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0))
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "resource not found (irq_0, %u)\n", iparch);
		goto err_exit_1;
	}
	vcpinf->irq_vlc = (int)res->start;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (res == NULL) {
		dev_err(&pdev->dev, "resource not found (irq_1, %u)\n", iparch);
		goto err_exit_1;
	}
	vcpinf->irq_ce = (int)res->start;
#else
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
	{
		dev_err(&pdev->dev, "resource not found (irq_0, %u)\n", iparch);
		goto err_exit_1;
	}
    vcpinf->irq_vlc = irq;

	irq = platform_get_irq(pdev, 1);
	if (irq < 0)
	{
		dev_err(&pdev->dev, "resource not found (irq_1, %u)\n", iparch);
		goto err_exit_1;
	}
    vcpinf->irq_ce = irq;
#endif

	if (of_property_read_u32(pdev->dev.of_node, "renesas,#fcp_ch", &ch) < 0) {
		dev_err(&pdev->dev, "of_property_read_u32() failed #%d, %u\n", __LINE__, iparch);
		goto err_exit_1;
	}
	if (ch >= UVCS_FCP_DEVNUM) {
		result = -EINVAL;
		dev_err(&pdev->dev, "invalid fcp_ch %u\n", iparch);
		goto err_exit_1;
	}

	vcpinf->reg_vlc = ioremap(vcpinf->pa_vlc,
									UVCS_REG_SIZE_VLC);
	if (vcpinf->reg_vlc == NULL) {
		dev_err(&pdev->dev, "failed to remap (reg_0, %u)\n", iparch);
		goto err_exit_1;
	}

	vcpinf->reg_ce = ioremap(vcpinf->pa_ce,
									UVCS_REG_SIZE_CE);
	if (vcpinf->reg_ce == NULL) {
		dev_err(&pdev->dev, "failed to remap (reg_1, %u)\n", iparch);
		goto err_exit_2;
	}

	snprintf(vcpinf->irq_name_vlc, sizeof(vcpinf->irq_name_vlc), "%s %s,v",
		 dev_name(&pdev->dev), name);
	snprintf(vcpinf->irq_name_ce, sizeof(vcpinf->irq_name_ce), "%s %s,c",
		 dev_name(&pdev->dev), name);

	vcpinf->iparch = iparch;
	vcpinf->ipgroup = ch;
	vcpinf->pdev = pdev;

	return 0;

err_exit_2:
	iounmap(vcpinf->reg_vlc);
	vcpinf->reg_vlc = NULL;

err_exit_1:
err_exit_0:
	return result;
}

/**
 * \brief gets resources for FCPC module from device-tree.
 *
 * \retval 0		 success
 * \retval others	 failed
 */
int uvcs_get_fcp_resource(
			struct platform_device *pdev,	/**< [in] platform device for FCPC */
			struct uvcs_fcp_hwinf *fcpinf,	/**< [out] FCPC control information */
			u32 iparch	/**< [in] architecture type of VCP */
			)
{
	int result = -EFAULT;
	struct resource *res;

	if ((pdev == NULL) || (fcpinf == NULL))
		goto err_exit_0;

	fcpinf->reg_fcp = NULL;
	result = -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "resource not found (reg)\n");
		goto err_exit_1;
	}
	fcpinf->pa_fcp = (u64)res->start;

	fcpinf->reg_fcp = ioremap(fcpinf->pa_fcp,
									UVCS_REG_SIZE_FCPC);
	if (fcpinf->reg_fcp == NULL) {
		dev_err(&pdev->dev, "failed to remap (reg)\n");
		goto err_exit_1;
	}

	fcpinf->iparch = iparch;
	fcpinf->pdev = pdev;
	return 0;

err_exit_1:
err_exit_0:
	return result;
}

/**
 * \brief release VCP resource
 */
void uvcs_put_vcp_resource(
			struct uvcs_vcp_hwinf *vcpinf /**< [in] VCP control information */
			)
{
	if ((vcpinf)
	&&	(vcpinf->reg_vlc)) {
		if (vcpinf->reg_ce) {
			iounmap(vcpinf->reg_ce);
			vcpinf->reg_ce = NULL;
		}
		iounmap(vcpinf->reg_vlc);
		vcpinf->reg_vlc = NULL;
	}
}

/**
 * \brief release FCPC resource
 */
void uvcs_put_fcp_resource(
			struct uvcs_fcp_hwinf *fcpinf /**< [in] FCPC control information */
			)
{
	if ((fcpinf)
	&&	(fcpinf->reg_fcp)) {
		iounmap(fcpinf->reg_fcp);
		fcpinf->reg_fcp = NULL;
	}
}
