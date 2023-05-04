// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef dsp_def_h
#define dsp_def_h

/*NOTE: this definition should be same copy as the one used in AMP*/

typedef enum {
	DSP_IOCTL_SEND_CMD = 10, /*no use. keep it to align the position*/
	/* including either new msg or ack msg */
	DSP_IOCTL_RECEIVE_NOTIFIER,
	DSP_IOCTL_DISABLE_INT,
	DSP_IOCTL_ENABLE_INT,
	DSP_IOCTL_UNBLOCK_USER_ISR,
} DSP_IOCTL_TYPE;

/*msg send to terminate ISR polling thread in user-space*/
#define UNBLOCK_USER_ISR  1

#endif  /*dsp_def_h*/
