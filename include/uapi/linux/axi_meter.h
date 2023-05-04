// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
*
* Author: Benson Gui <Benson.Gui@synaptics.com>
*
*/
#ifndef _UAPI_AXI_METER_H
#define _UAPI_AXI_METER_H

#include <linux/ioctl.h>

#define MC_IOC_MAGIC	'M'

#define MC_IOC_GET_EVT		_IO(MC_IOC_MAGIC, 1)
#define MC_IOC_SET_EVT		_IO(MC_IOC_MAGIC, 2)
#define MC_IOC_START_CNT	_IO(MC_IOC_MAGIC, 3)
#define MC_IOC_STOP_CNT		_IO(MC_IOC_MAGIC, 4)
#define MC_IOC_CLR_CNT		_IO(MC_IOC_MAGIC, 5)
#define MC_IOC_GET_CNT		_IO(MC_IOC_MAGIC, 6)
#define MC_IOC_GET_CLR_CNT	_IO(MC_IOC_MAGIC, 7)
#define MC_IOC_AXI_GET_CNT	_IO(MC_IOC_MAGIC, 8)
#define MC_IOC_AXI_CLR_CNT	_IO(MC_IOC_MAGIC, 9)
#define MC_IOC_AXI_GET_CLR	_IO(MC_IOC_MAGIC, 10)
#define MC_IOC_AXI_START_CNT	_IO(MC_IOC_MAGIC, 11)
#define MC_IOC_AXI_STOP_CNT	_IO(MC_IOC_MAGIC, 12)
#define MC_IOC_AXI_GET_MASK	_IO(MC_IOC_MAGIC, 13)
#define MC_IOC_AXI_SET_MASK	_IO(MC_IOC_MAGIC, 14)
#define MC_IOC_AXI_GET_INFO	_IO(MC_IOC_MAGIC, 15)
#define MC_IOC_AXI_GET_CLR_ALL	_IO(MC_IOC_MAGIC, 16)

#define MC_SET_ALL_CNT		0xffff
#define AS370_MC_CNT_NUM	8
#define AS470_MC_CNT_NUM	8
#define VS640_MC_CNT_NUM	8
#define VS680_MC_CNT_NUM	16
#define MAX_MC_CNT_NUM		16
#define MAX_AXI_CNT_NUM		12
#define MC_EVT_INVALID		0xffff
#define CNT_NUM_PER_SET		8
#define CNT_SETS_NUM		(MAX_MC_CNT_NUM / CNT_NUM_PER_SET)

#define OF_BIT_TOT		0
#define OF_BIT_ARWAIT		1		/* arwait */
#define OF_BIT_RWAIT		2		/* rwait */
#define OF_BIT_RIDLE		3		/* ridle */
#define OF_BIT_RD		4		/* rdata */
#define OF_BIT_AWWAIT		5		/* awwait */
#define OF_BIT_WWAIT		6		/* wwait */
#define OF_BIT_WIDLE		7		/* widle */
#define OF_BIT_WDATA		8		/* wdata */
#define OF_BIT_AWDATA		9		/* awdata */
#define OF_BIT_ARDATA		10		/* ardata */

enum mc_counter_type {
	AS370_AXI_MC_TYPE,
	VS680_AXI_MC_TYPE,
	AS470_AXI_MC_TYPE,
	VS640_AXI_MC_TYPE,
	MAX_AXI_MC_TYPE,
};

/*   events defined in spec    */
/************************************
* 0x0 : Clock cycles
* 0x1 : perf_hif_rd_or_wr
* 0x2 : perf_hif_wr
* 0x3 : perf_hif_rd
* 0x4 : perf_hif_rmw
* 0x5 : perf_hif_hi_pri_rd
* 0x6 : perf_dfi_wr_data_cycles
* 0x7 : perf_dfi_rd_data_cycles
* 0x8 : perf_hpr_xact_when_critical
* 0x9 : perf_lpr_xact_when_critical
* 0xa : perf_wr_xact_when_critical
* 0xb : perf_op_is_activate
* 0xc : perf_op_is_rd_or_wr
* 0xd : perf_op_is_rd_activate
* 0xe : perf_op_is_rd
* 0xf : perf_op_is_wr
* 0x10 : perf_op_is_precharge
* 0x11 : perf_precharge_for_rdwr
* 0x12 : perf_precharge_for_other
* 0x13 : perf_rdwr_transitions
* 0x14 : perf_write_combine
* 0x15 : perf_war_hazard
* 0x16 : perf_raw_hazard
* 0x17 : perf_waw_hazard
* 0x18 : perf_op_is_enter_selfref
* 0x19 : perf_op_is_enter_powerdown
* 0x1a : perf_selfref_mode
* 0x1b : perf_op_is_refresh
* 0x1c : perf_op_is_crit_ref
* 0x1d : perf_op_is_spec_ref
* 0x1e : perf_op_is_load_mode
* 0x1f : perf_op_is_zqcl
* 0x20 : perf_op_is_zqcs
* 0x21 : perf_hpr_req_with_nocredit
* 0x22 : perf_lpr_req_with_nocredit
*************************************/
enum syna_mc_evt_t {
	mc_clk_cyc = 0,
	mc_hif_rd_or_wr,
	mc_hif_wr,
	mc_hif_rd,
	mc_hif_rmw,
	mc_perf_hif_hi_pri_rd,			/* 5 */
	mc_dfi_wr_data_cycles,
	mc_dfi_rd_data_cycles,
	mc_hpr_xact_when_critical,
	mc_lpr_xact_when_critical,
	mc_wr_xact_when_critical,		/* 10 */
	mc_op_is_activate,
	mc_op_is_rd_or_wr,
	mc_op_is_rd_activate,
	mc_op_is_rd,
	mc_op_is_wr,				/* 15 */
	mc_op_is_precharge,
	mc_precharge_for_rdwr,
	mc_precharge_for_other,
	mc_rdwr_transitions,
	mc_write_combine,			/* 20 */
	mc_war_hazard,
	mc_raw_hazard,
	mc_waw_hazard,
	mc_op_is_enter_selfref,
	mc_op_is_enter_powerdown,		/* 25 */
	mc_selfref_mode,
	mc_op_is_refresh,
	mc_op_is_crit_ref,
	mc_op_is_spec_ref,
	mc_op_is_load_mode,			/* 30 */
	mc_op_is_zqcl,
	mc_op_is_zqcs,
	mc_hpr_req_with_nocredit,
	mc_lpr_req_with_nocredit,
	mc_evt_max,
};

enum syna_mc_cnt_type {
	CNT_TYPE_MC = 0,
	CNT_TYPE_AXI,
	CNT_TYPE_INV,
};


struct mc_set_evt_param {
	__u32 cnt;
	__u32 evt;
};

struct mc_get_cnt_param {
	__u32 cnt_num;
	__u32 cnts[MAX_MC_CNT_NUM];
	__u32 overflow[CNT_SETS_NUM];
};

struct mc_get_clr_param {
	__u32 cnt_idx;
	__u32 cnt_val;
	__u32 overflow;
};

struct axi_get_cnt_param {
	__u32 cnt;
	__u32 total;
	__u32 rdata;
	__u32 ardata;
	__u32 wdata;
	__u32 awdata;
	__u32 status;
};

struct axi_get_info {
	__u32 type;
	__u32 axi_cnt_num;
	__u32 mc_cnt_num;
};

struct axi_set_mask {
	__u32 cnt;
	__u32 mask_id;
};

#endif /* _UAPI_AXI_METER_H */
