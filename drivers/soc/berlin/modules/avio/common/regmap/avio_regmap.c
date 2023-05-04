// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/platform_device.h>

#include "avio_regmap.h"
#include "avio_sub_module.h"

/*******************************************************************************
 * IO remap function
 */

/*
 * Remap all berlin physical address space.
 *
 * Return 0 if ioremap success, call avio_destroy_deviounmap only ONCE to
 *  claim no need for berlin remapped virture address anymore;
 * Return non-zero if ioremap fail, DONOT call avio_destroy_deviounmap
 *  at this case.
 */
int avio_create_devioremap(struct platform_device *pdev)
{
	AVIO_CTX *hAvioCtx = (AVIO_CTX *)avio_sub_module_get_ctx(AVIO_MODULE_TYPE_AVIO);
	int ret = 0;

	hAvioCtx->avio_virt_base = NULL;

	if (!hAvioCtx->avio_base && pdev) {
		hAvioCtx->pAvioRes = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		hAvioCtx->avio_base = hAvioCtx->pAvioRes->start;
		hAvioCtx->avio_size = resource_size(hAvioCtx->pAvioRes);
	}

	if (pdev)
		hAvioCtx->avio_virt_base = devm_ioremap(&pdev->dev, hAvioCtx->pAvioRes->start,
					resource_size(hAvioCtx->pAvioRes));
	else
		hAvioCtx->avio_virt_base = ioremap(hAvioCtx->avio_base, hAvioCtx->avio_size);

	//Try to remap using regmap_add_entry atleast
	hAvioCtx->avio_virt_base = regmap_add_entry(hAvioCtx->avio_base,
			hAvioCtx->avio_size, hAvioCtx->avio_virt_base);

	if (IS_ERR_OR_NULL(hAvioCtx->avio_virt_base)) {
		avio_error("Fail to map address before it is used!\n");
		ret = -1;
		goto err_ioremap_failed;
	}
	hAvioCtx->regmap_handle = regmap_get_handle_by_phy_addr(hAvioCtx->avio_base);

err_ioremap_failed:
	avio_error("ioremap %s: vir_addr: %p, size: 0x%x, phy_addr: %x!\n",
			ret?"failed":"success",
		  hAvioCtx->avio_virt_base, hAvioCtx->avio_size, hAvioCtx->avio_base);

	return ret;
}

/*
 * Unmap all berlin physical address space.
 *
 * Current reference counter can't cover corner case of unmatch call.
 * Need user to call pair with avio_create_devioremap:
 *  Only call avio_destroy_deviounmap ONCE if avio_create_devioremap
 *   success;
 *  DONOT call avio_destroy_deviounmap if avio_create_devioremap fail.
 */
int avio_destroy_deviounmap(void)
{
	AVIO_CTX *hAvioCtx = (AVIO_CTX *)avio_sub_module_get_ctx(AVIO_MODULE_TYPE_AVIO);

	if (hAvioCtx->avio_virt_base) {
		//Let devm take care of releasing Memory IORESOURCE_MEM
		regmap_remove_entry(hAvioCtx->avio_base);
		hAvioCtx->avio_virt_base = NULL;
	}

	return 0;
}

#ifdef AVIO_VALIDATE_AND_ACCESS_MEMMAP_REGION

void *avio_memmap_phy_to_vir(unsigned int phyaddr)
{
	AVIO_CTX *hAvioCtx = (AVIO_CTX *)avio_sub_module_get_ctx(AVIO_MODULE_TYPE_AVIO);

	if (hAvioCtx->avio_virt_base &&
		REGMAP_IS_ADDR_IN_RANGE(phyaddr, hAvioCtx->avio_base, hAvioCtx->avio_size))
		return regmap_phy_to_virt_by_handle(hAvioCtx->regmap_handle, phyaddr);

	avio_error("Fail to map phy_addr:0x%x, phyaddr: 0x%x, size: 0x%x, Base vir: %p!\n",
		 phyaddr, hAvioCtx->avio_base, hAvioCtx->avio_size, hAvioCtx->avio_virt_base);
	return 0;
}

unsigned int avio_memmap_vir_to_phy(void *v_addr)
{
	AVIO_CTX *hAvioCtx = (AVIO_CTX *)avio_sub_module_get_ctx(AVIO_MODULE_TYPE_AVIO);

	if (hAvioCtx->avio_virt_base &&
		REGMAP_IS_ADDR_IN_RANGE(v_addr, hAvioCtx->avio_virt_base, hAvioCtx->avio_size))
		return regmap_virt_to_phy_by_handle(hAvioCtx->regmap_handle, v_addr);

	avio_error("Fail to map virt_addr:%p, phyaddr: 0x%x, size: 0x%x, Base vir: %p!\n",
		v_addr, hAvioCtx->avio_base, hAvioCtx->avio_size, hAvioCtx->avio_virt_base);
	return 0;
}

#endif //AVIO_VALIDATE_AND_ACCESS_MEMMAP_REGION


int avio_driver_memmap(struct file *file, struct vm_area_struct *vma)
{
	AVIO_CTX *hAvioCtx = (AVIO_CTX *)avio_sub_module_get_ctx(AVIO_MODULE_TYPE_AVIO);
	int ret = 0;
	size_t request_size;
	size_t mem_maped_size;
	unsigned long request_addr;

	mem_maped_size = hAvioCtx->avio_base + hAvioCtx->avio_size;
	request_size = vma->vm_end - vma->vm_start;
	request_addr = hAvioCtx->avio_base + (vma->vm_pgoff << PAGE_SHIFT);

	if (!(request_addr >= hAvioCtx->avio_base &&
		(request_addr + request_size) <= mem_maped_size)) {
		avio_error("Invalid request_address, start=0x%lx, end=0x%lx\n",
			  request_addr, request_addr + request_size);
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	ret = remap_pfn_range(vma, vma->vm_start,
				  (hAvioCtx->avio_base >> PAGE_SHIFT) +
				  vma->vm_pgoff, request_size, vma->vm_page_prot);

	return ret;
}

#ifdef AVIO_VALIDATE_AND_ACCESS_MEMMAP_REGION
EXPORT_SYMBOL(avio_memmap_phy_to_vir);
EXPORT_SYMBOL(avio_memmap_vir_to_phy);
#endif //AVIO_VALIDATE_AND_ACCESS_MEMMAP_REGION
