# SPDX-License-Identifier: GPL-2.0-only

obj-$(CONFIG_RENESAS_VSPM) += vspm.o
vspm-objs := vspm_main.o vspm_sub.o
vspm-objs += frame.o
vspm-objs += manager/vspm_control.o manager/vspm_drv_vsp.o manager/vspm_drv_fdp.o manager/vspm_exec_manager.o manager/vspm_drv_isu.o
vspm-objs += manager/vspm_job_manager.o manager/vspm_lib.o manager/vspm_sort_queue.o manager/vspm_task.o
vspm-objs += vsp/vsp_drv.o vsp/vsp_drv_phy.o vsp/vsp_drv_par.o
vspm-objs += fdp/fdp_drv.o fdp/fdp_drv_l.o
vspm-objs += isu/isu_drv.o isu/isu_drv_phy.o isu/isu_drv_par.o

ccflags-y += -I$(srctree)/drivers/renesas/vspm/
ccflags-y += -I$(srctree)/include
