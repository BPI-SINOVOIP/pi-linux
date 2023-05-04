// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef _OVP_IOCTL_H_
#define _OVP_IOCTL_H_

#define OVP_IOCTL_INTR	0xbeef7001
#define OVP_IOCTL_GET_MSG	0xbeef7002
#define OVP_IOCTL_DUMMY_INTR	0xbeef7003
#define OVP_INIT_TA_SESSION	0xbeef7004
#define OVP_KILL_TA_SESSION	0xbeef7005
#define OVP_IOCTL_CLKCONTROL	0xbeef7006

typedef struct INTR_MSG_T {
	unsigned int DhubSemMap;
	unsigned int Enable;
} INTR_MSG;

#endif //_OVP_IOCTL_H_
