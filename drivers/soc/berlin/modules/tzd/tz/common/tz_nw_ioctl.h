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

#ifndef _TZ_NW_IOCTL_H_
#define _TZ_NW_IOCTL_H_

/*
 * In order to simplify tz_driver, we only support below ioctl:
 * - register (need further consideration)
 * - mmap (get a physical address continous memory)
 * - va_to_pa (covert the buffer from mmap)
 * - cmd (use same structure as tee_comm)
 */

#include "tz_comm.h"

#define TZ_CLIENT_DEVICE_FULL_PATH	"/dev/tz"
#define TZ_CLIENT_DEVICE_NAME	"tz"

#define TZ_CLIENT_IOC_MAGIC	'T'	/* TrustZone magic number */

struct tz_mem_info {
	void *va;
	void *pa;
	uint32_t attr;
};

struct tz_memmove_param {
	void *dst;
	void *src;
	size_t size;
};

struct tz_cache_param {
	void *start;
	size_t size;
};

struct fastcall_generic_param {
	/*
	 * the first is always the sub command id
	 */
	unsigned int sub_cmd_id;
	unsigned int param_len;
	char param[0];
};

/*
 * we use same data structure between userspace<->kernelspace and
 * kernelspace<->tz. It would be high efficiency.
 * and we add task_id in tee_comm_channel to support it.
 */
#define TZ_CLIENT_IOCTL_CMD \
	_IO(TZ_CLIENT_IOC_MAGIC, 2)

/* it's to alloc the physical address before call mmap() */
#define TZ_CLIENT_IOCTL_ALLOC_MEM \
	_IO(TZ_CLIENT_IOC_MAGIC, 3)

/* it's to free the physical address after call mumap() */
#define TZ_CLIENT_IOCTL_FREE_MEM \
	_IO(TZ_CLIENT_IOC_MAGIC, 4)

#define TZ_CLIENT_IOCTL_GET_MEMINFO \
	_IO(TZ_CLIENT_IOC_MAGIC, 5)

#define TZ_CLIENT_IOCTL_FASTCALL_MEMMOVE \
	_IO(TZ_CLIENT_IOC_MAGIC, 6)

#define TZ_CLIENT_IOCTL_FASTCALL_CACHE_CLEAN \
	_IO(TZ_CLIENT_IOC_MAGIC, 7)

#define TZ_CLIENT_IOCTL_FASTCALL_CACHE_INVALIDATE \
	_IO(TZ_CLIENT_IOC_MAGIC, 8)

#define TZ_CLIENT_IOCTL_FASTCALL_CACHE_FLUSH \
	_IO(TZ_CLIENT_IOC_MAGIC, 9)

struct tz_session_param {
	uint32_t param;
	uint32_t origin;
	uint32_t task_id;
	uint32_t result;
};

struct tz_instance_param {
	uint32_t param;
	uint32_t origin;
	uint32_t result;
};

#define TZ_CLIENT_IOCTL_OPEN_SESSION \
	_IO(TZ_CLIENT_IOC_MAGIC, 10)

#define TZ_CLIENT_IOCTL_CLOSE_SESSION \
	_IO(TZ_CLIENT_IOC_MAGIC, 11)

#define TZ_CLIENT_IOCTL_CREATE_INSTANCE \
	_IO(TZ_CLIENT_IOC_MAGIC, 12)

#define TZ_CLIENT_IOCTL_DESTROY_INSTANCE \
	_IO(TZ_CLIENT_IOC_MAGIC, 13)

#define TZ_CLIENT_IOCTL_FASTCALL_GENERIC_CMD \
	_IO(TZ_CLIENT_IOC_MAGIC, 14)

#endif /* _TZ_NW_IOCTL_H_ */
