/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Driver for Renesas RZ/V2N Input Video Control
 *
 * Copyright (C) 2023 Renesas Electronics Corp.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef __RZV2N_IVC__
#define __RZV2N_IVC__

#define N_MAX_IVC_VB2_BUFS   (20)  /* Max # of buffers */
#define N_DEF_IVC_VB2_BUFS   (5)  /* Default # of buffers. Recommended to be equal or larger than cru->num_buf */
#define IVC_IS_DRIVEN_BY_ISP_IRQ  (1) /* If 1, next frame is triggerd by ISP rather than IVC */

#define IVC_WDR_MODE_LINEAR                                   0x00000000
#define IVC_WDR_MODE_FS_LIN                                   0x00000002

struct rzv2n_ivc_sensor_mode {
	int wdr_mode;
	int width;
	int height;
	void *params;
};

int rzv2n_ivc_stream_start(u32 ctx_id, const struct rzv2n_ivc_sensor_mode *mode, int *error, int32_t (*sw_mcfe_external_process_request_v4l2_fr_ss)(uint32_t ctx_id, uint64_t timestamp));
int rzv2n_ivc_stream_stop(u32 ctx_id);
int rzv2n_ivc_notify_isp_config_done(u32 ctx_id);
int rzv2n_ivc_notify_isp_consumed_frame(u32 ctx_id);
int rzv2n_ivc_notify_cru_output_done(u32 ctx_id, u64 timestamp, int error);

#endif
