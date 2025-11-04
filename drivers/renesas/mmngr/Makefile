# SPDX-License-Identifier: GPL-2.0-only

obj-$(CONFIG_RENESAS_MMNGR) += mmngr.o
mmngr-objs := mmngr_drv.o

ccflags-y += -DMMNGR_SALVATORX
ccflags-y += -DMMNGR_SSP_DISABLE
ccflags-y += -DIPMMU_MMU_DISABLE
ccflags-y += -I$(srctree)/include
