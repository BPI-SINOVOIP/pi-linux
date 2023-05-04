// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _CEC_DRIVER_H_
#define _CEC_DRIVER_H_

#define CEC_CC_MSG_TYPE 1

#define CEC_ISR_MSGQ_SIZE   32

/* ioctl commands */
#define CEC_IOCTL_INTR_MSG      0xbeef0001
#define CEC_IOCTL_GET_MSG       0xbeef0002
#define CEC_IOCTL_RX_MSG_BUF    0xbeef0003

/* CEC interrrupt status */
#define BE_CEC_INTR_TX_SFT_FAIL         0x2008
#define BE_CEC_INTR_TX_FAIL             0x200F
#define BE_CEC_INTR_TX_COMPLETE         0x0010
#define BE_CEC_INTR_RX_COMPLETE         0x0020
#define BE_CEC_INTR_RX_FAIL             0x00C0
#define CEC_INTR_STATUS0_REG_ADDR       0x0058
#define CEC_INTR_STATUS1_REG_ADDR       0x0059
#define CEC_INTR_ENABLE0_REG_ADDR       0x0048
#define CEC_INTR_ENABLE1_REG_ADDR       0x0049
#define CEC_RDY_ADDR                    0x0008
#define CEC_RX_BUF_READ_REG_ADDR        0x0068
#define CEC_RX_EOM_READ_REG_ADDR        0x0069
#define CEC_TOGGLE_FOR_READ_REG_ADDR    0x0004
#define CEC_RX_RDY_ADDR                 0x000c
#define CEC_TX_FIFO_RESET_ADDR          0x0010
#define CEC_RX_FIFO_RESET_ADDR          0x0014
#define CEC_RX_FIFO_DPTR                0x0087
#define CEC_TX_PRESENT_STATE_REG_ADDR   0x0078

typedef struct CEC_RX_MSG_BUF_T {
	unsigned char buf[16];
	unsigned char len;
} CEC_RX_MSG_BUF;

struct cec_context {
	AMPMsgQ_t hCECMsgQ;
	struct semaphore cec_sem;
	struct mutex cec_mutex;
	int gCecIsrEnState;
	int cec_irq;
	struct resource *pCecRes;
	CEC_RX_MSG_BUF rx_buf;
	atomic_t irq_stat;
};

struct cec_device_t {
	struct cec_context CecCtx;
	void __iomem *cec_virt_addr;
	unsigned char *dev_name;
	struct cdev cdev;
	struct class *dev_class;
	int major;
	int minor;
};

#endif      //_CEC_DRIVER_H_
