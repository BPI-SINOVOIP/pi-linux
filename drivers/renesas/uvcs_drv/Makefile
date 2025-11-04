# SPDX-License-Identifier: GPL-2.0-only

obj-$(CONFIG_RENESAS_UVCS_DRV) += uvcs_drv.o
uvcs_drv-objs := lkm/uvcs_lkm.o
uvcs_drv-objs += lkm/uvcs_lkm_uf_semaphore.o
uvcs_drv-objs += lkm/uvcs_lkm_uf_thread.o
uvcs_drv-objs += lkm/uvcs_lkm_uf_other.o
uvcs_drv-objs += lkm/uvcs_lkm_uf_io.o
uvcs_drv-objs += cmn/uvcs_cmn_api.o
uvcs_drv-objs += cmn/uvcs_cmn_dump.o
uvcs_drv-objs += core/mcvx_api.o

EXTRA_CFLAGS += -DUVCS_DEBUG=1

ccflags-y += -I$(srctree)/include
ccflags-y += -I$(srctree)/drivers/renesas/uvcs_drv/core
