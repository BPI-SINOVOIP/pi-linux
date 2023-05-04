/*
 * TrustZone Driver
 *
 * Copyright (C) 2016 Marvell Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _TZ_DRIVER_H_
#define _TZ_DRIVER_H_

#include "tz_nw_ioctl.h"

int tz_get_meminfo(struct tz_mem_info *info);
int tzd_kernel_alloc_mem(void **va, void **pa, uint32_t alloc_len);
int tzd_kernel_free_mem(void *pa);
void *tzd_phys_to_virt(void *call_info, void *pa);
void *tzd_get_kernel_dev_file(void);
int tzd_kernel_create_instance(struct tz_instance_param *create_param);
int tzd_kernel_destroy_instance(struct tz_instance_param *destroy_param);
int tzd_kernel_open_session(struct tz_session_param *open_param);
int tzd_kernel_close_session(struct tz_session_param *close_param);
extern int tz_invoke_command(int tzc, int task_id, uint32_t cmd_id,
			uint32_t param, uint32_t *origin,
			tz_cmd_handler callback, void *userdata, void *call_info);

#endif /* _TZ_DRIVER_H_ */
