// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated
 *
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _AVIO_IOCTL_H_
#define _AVIO_IOCTL_H_

#include <linux/version.h>

#include "avio_type.h"

#define VPP_IOCTL_VBI_DMA_CFGQ              0xbeef0001
#define VPP_IOCTL_VBI_BCM_CFGQ              0xbeef0002
#define VPP_IOCTL_VDE_BCM_CFGQ              0xbeef0003
#define VPP_IOCTL_GET_MSG                   0xbeef0004
#define VPP_IOCTL_START_BCM_TRANSACTION	    0xbeef0005
#define VPP_IOCTL_BCM_SCHE_CMD              0xbeef0006
#define VPP_IOCTL_INTR_MSG		            0xbeef0007
#define CEC_IOCTL_RX_MSG_BUF_MSG            0xbeef0008
#define VPP_IOCTL_GET_VSYNC                 0xbeef0009
#define HDMITX_IOCTL_UPDATE_HPD             0xbeef000a
#define VPP_IOCTL_INIT                      0xbeef000b
#define ISR_IOCTL_SET_AFFINITY              0xbeef000c
#define VPP_IOCTL_GET_QUIESCENT_FLAG        0xbeef000d
#define VPP_IOCTL_GPIO_WRITE                0xbeef000e
#define VPP_IOCTL_ENABLE_CLK                0xbeef000f
#define VPP_IOCTL_GET_CLK_RATE              0xbeef0010
#define VPP_IOCTL_SET_CLK_RATE              0xbeef0011
#define VPP_IOCTL_GET_VSYNC1                0xbeef0012
#define HDMITX_IOCTL_SET_5V_STATE           0xbeef0013
#define VPP_IOCTL_START                     VPP_IOCTL_VBI_DMA_CFGQ
#define VPP_IOCTL_END                       HDMITX_IOCTL_SET_5V_STATE


#define AOUT_IOCTL_START_CMD            0xbeef2001
#define AOUT_IOCTL_STOP_CMD             0xbeef2002
#define AIP_IOCTL_START_CMD             0xbeef2003
#define AIP_IOCTL_STOP_CMD              0xbeef2004
#define AIP_AVIF_IOCTL_START_CMD        0xbeef2005
#define AIP_AVIF_IOCTL_STOP_CMD	        0xbeef2006
#define AIP_AVIF_IOCTL_SET_MODE         0xbeef2007
#define AIP_IOCTL_GET_MSG_CMD           0xbeef2008
#define AIP_IOCTL_SEMUP_CMD             0xbeef2009
#define AIP_IOCTL_INTR_REG              0xbeef200a
#define APP_IOCTL_INIT_CMD              0xbeef3001
#define APP_IOCTL_START_CMD             0xbeef3002
#define APP_IOCTL_DEINIT_CMD            0xbeef3003
#define APP_IOCTL_GET_MSG_CMD           0xbeef3004
#define AOUT_IOCTL_GET_MSG_CMD          0xbeef3005

#define VIP_IOCTL_GET_MSG               0xbeef4001
#define VIP_IOCTL_VBI_BCM_CFGQ          0xbeef4002
#define VIP_IOCTL_SD_WRE_CFGQ           0xbeef4003
#define VIP_IOCTL_SD_RDE_CFGQ           0xbeef4004
#define VIP_IOCTL_SEND_MSG              0xbeef4005
#define VIP_IOCTL_VDE_BCM_CFGQ          0xbeef4006
#define VIP_IOCTL_INTR_MSG              0xbeef4007
#define VIP_HRX_IOCTL_GET_MSG           0xbeef4008
#define VIP_HRX_IOCTL_SEND_MSG          0xbeef4009
#define VIP_HDCP_IOCTL_GET_MSG          0xbeef400a
#define VIP_HRX_IOCTL_GPIO_WRITE        0xbeef400b
#define VIP_HRX_IOCTL_GPIO_5V_READ      0xbeef400c
#define VIP_MSG_DESTROY_ISR_TASK        1

#define AVIF_IOCTL_GET_MSG              0xbeef6001
#define AVIF_IOCTL_VBI_CFGQ             0xbeef6002
#define AVIF_IOCTL_SD_WRE_CFGQ          0xbeef6003
#define AVIF_IOCTL_SD_RDE_CFGQ          0xbeef6004
#define AVIF_IOCTL_SEND_MSG             0xbeef6005
#define AVIF_IOCTL_VDE_CFGQ             0xbeef6006
#define AVIF_IOCTL_INTR_MSG             0xbeef6007

#define AVIF_HRX_IOCTL_GET_MSG          0xbeef6050
#define AVIF_HRX_IOCTL_SEND_MSG         0xbeef6051
#define AVIF_VDEC_IOCTL_GET_MSG         0xbeef6052
#define AVIF_VDEC_IOCTL_SEND_MSG        0xbeef6053
#define AVIF_HRX_IOCTL_ARC_SET          0xbeef6054
#define AVIF_HDCP_IOCTL_GET_MSG         0xbeef6055
#define AVIF_HDCP_IOCTL_SEND_MSG        0xbeef6056
#define HDMIRX_IOCTL_GET_MSG            0xbeef5001
#define HDMIRX_IOCTL_SEND_MSG           0xbeef5002

#define OVP_IOCTL_INTR                  0xbeef7001
#define OVP_IOCTL_GET_MSG               0xbeef7002
#define OVP_IOCTL_DUMMY_INTR            0xbeef7003
#define OVP_INIT_TA_SESSION             0xbeef7004
#define OVP_KILL_TA_SESSION             0xbeef7005

#define POWER_IOCTL_DISABLE_INT         0xbeef8001
#define POWER_IOCTL_ENABLE_INT          0xbeef8002
#define POWER_IOCTL_WAIT_RESUME         0xbeef8003
#define POWER_IOCTL_START_RESUME        0xbeef8004

#define MAX_INTR_NUM 0x20

#define VPP_CEC_INT_EN      1
#define VPP_INT_EN          2
#define VIP_INT_EN          1

enum {
	IRQ_VPP_MODULE = 0,
	IRQ_AVIN_MODULE,
	IRQ_AOUT_MODULE,
	IRQ_OVP_MODULE,
	IRQ_VIP_MODULE,
	IRQ_MAX_MODULE,
};

typedef enum {
	VPP_CLK_ID_AVIOSYSCLK,
	VPP_CLK_ID_VPPSYSCLK,
	VPP_CLK_ID_BIUCLK,
	VPP_CLK_ID_VPPPIPECLK,
	VPP_CLK_ID_OVPCLK,
	VPP_CLK_ID_FPLL400CLK,
	VPP_CLK_ID_TXESCCLK,
	VPP_CLK_ID_VCLK0,
	VPP_CLK_ID_DPICLK,
	VPP_CLK_ID_AIOSYSCLK,
	VPP_CLK_ID_ESMHPICLK,
	VPP_CLK_ID_ESMCLK,
} VPP_CLK_ID;

typedef struct _VPP_CLOCK_ {
	VPP_CLK_ID clk_id;
	char *clk_name;
	struct clk *vpp_clk;
} VPP_CLOCK;

typedef struct INTR_MSG_T {
	unsigned int DhubSemMap;
	unsigned int Enable;
} INTR_MSG;

typedef struct VPP_CLK_RATE_T {
	VPP_CLK_ID clk_id;
	unsigned int clk_rate_in_hz;
} VPP_CLK_RATE;

typedef struct VPP_CLK_ENABLE_T {
	VPP_CLK_ID clk_id;
	int enable;
} VPP_CLK_ENABLE;

typedef struct {
	unsigned int uiShmOffset;	/**< Not used by kernel */
	unsigned int *unaCmdBuf;
	unsigned int *cmd_buffer_base;
	unsigned int max_cmd_size;
	unsigned int cmd_len;
	unsigned int cmd_buffer_hw_base;
	void *p_cmd_tag;
} APP_CMD_BUFFER;

typedef struct {
	unsigned int uiShmOffset;	/**< Not used by kernel */
	APP_CMD_BUFFER in_coef_cmd[8];
	APP_CMD_BUFFER out_coef_cmd[8];
	APP_CMD_BUFFER in_data_cmd[8];
	APP_CMD_BUFFER out_data_cmd[8];
	unsigned int size;
	unsigned int wr_offset;
	unsigned int rd_offset;
	unsigned int kernel_rd_offset;
/************** used by Kernel *****************/
	unsigned int kernel_idle;
} HWAPP_CMD_FIFO;

#endif //_AVIO_IOCTL_H_
