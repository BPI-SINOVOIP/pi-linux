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

#ifndef _TEE_INTERNAL_INFO_API_H_
#define _TEE_INTERNAL_INFO_API_H_

#include "tee_internal_common.h"
#include "tee_internal_config.h"

#define TEE_VERSION(major, minor)	(((major) << 16) | ((minor) << 16))
#define TEE_VERSION_STR(major, minor)	#major "." #minor

#define TEE_DESCRIPTION(major, minor)	"TEE SDK v" TEE_VERSION_STR(major, minor)

const char *TEE_GetVersion(uint32_t *major, uint32_t *minor);

const char *TEE_GetDescription(void);

const TEE_UUID *TEE_GetDeviceID(void);

#endif /* _TEE_INTERNAL_INFO_API_H_ */
