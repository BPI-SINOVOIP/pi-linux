/********************************************************************************
 * Marvell GPL License Option
 *
 * If you received this File from Marvell, you may opt to use, redistribute and/or
 * modify this File in accordance with the terms and conditions of the General
 * Public License Version 2, June 1991 (the "GPL License"), a copy of which is
 * available along with the File in the license.txt file or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
 * on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED.  The GPL License provides additional details about this warranty
 * disclaimer.
 ******************************************************************************/

#ifndef _TZ_NW_SYS_CALLBACK_H_
#define _TZ_NW_SYS_CALLBACK_H_

#include "tz_comm.h"

tz_errcode_t tz_nw_register_sys_callback(uint32_t cmd_id,
			tz_cmd_handler handler, void *userdata);

tz_errcode_t tz_nw_unregister_sys_callback(uint32_t cmd_id);

uint32_t tz_nw_sys_callback(void *userdata, uint32_t cmd_id,
				uint32_t param, uint32_t *p_origin);

#endif /* _TZ_NW_SYS_CALLBACK_H_ */