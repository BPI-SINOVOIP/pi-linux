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

#ifndef _TZ_ERRCODE_H_
#define _TZ_ERRCODE_H_

#include "types.h"

/* Here, we use same error code as TEE_ERROR_*.
 * but it's not a must, we can use different error code definitions.
 */
enum {
	TZ_SUCCESS			= 0x00000000,	/* TEEC_SUCCESS			*/
	TZ_ERROR_GENERIC		= 0xFFFF0000,	/* TEEC_ERROR_GENERIC		*/
	TZ_ERROR_ACCESS_DENIED		= 0xFFFF0001,	/* TEEC_ERROR_ACCESS_DENIED	*/
	TZ_ERROR_CANCEL			= 0xFFFF0002,	/* TEEC_ERROR_CANCEL		*/
	TZ_ERROR_ACCESS_CONFLICT	= 0xFFFF0003,	/* TEEC_ERROR_ACCESS_CONFLICT	*/
	TZ_ERROR_EXCESS_DATA		= 0xFFFF0004,	/* TEEC_ERROR_EXCESS_DATA	*/
	TZ_ERROR_BAD_FORMAT		= 0xFFFF0005,	/* TEEC_ERROR_BAD_FORMAT	*/
	TZ_ERROR_BAD_PARAMETERS		= 0xFFFF0006,	/* TEEC_ERROR_BAD_PARAMETERS	*/
	TZ_ERROR_BAD_STATE		= 0xFFFF0007,	/* TEEC_ERROR_BAD_STATE		*/
	TZ_ERROR_ITEM_NOT_FOUND		= 0xFFFF0008,	/* TEEC_ERROR_ITEM_NOT_FOUND	*/
	TZ_ERROR_NOT_IMPLEMENTED	= 0xFFFF0009,	/* TEEC_ERROR_NOT_IMPLEMENTED	*/
	TZ_ERROR_NOT_SUPPORTED		= 0xFFFF000A,	/* TEEC_ERROR_NOT_SUPPORTED	*/
	TZ_ERROR_NO_DATA		= 0xFFFF000B,	/* TEEC_ERROR_NO_DATA		*/
	TZ_ERROR_OUT_OF_MEMORY		= 0xFFFF000C,	/* TEEC_ERROR_OUT_OF_MEMORY	*/
	TZ_ERROR_BUSY			= 0xFFFF000D,	/* TEEC_ERROR_BUSY		*/
	TZ_ERROR_COMMUNICATION		= 0xFFFF000E,	/* TEEC_ERROR_COMMUNICATION	*/
	TZ_ERROR_SECURITY		= 0xFFFF000F,	/* TEEC_ERROR_SECURITY		*/
	TZ_ERROR_SHORT_BUFFER		= 0xFFFF0010,	/* TEEC_ERROR_SHORT_BUFFER	*/
	TZ_PENDING			= 0xFFFF2000,
	TZ_ERROR_TIMEOUT		= 0xFFFF3001,
	TZ_ERROR_OVERFLOW		= 0xFFFF300F,
	TZ_ERROR_TARGET_DEAD		= 0xFFFF3024,	/* TEEC_ERROR_TARGET_DEAD	*/
	TZ_ERROR_STORAGE_NO_SPACE	= 0xFFFF3041,
	TZ_ERROR_MAC_INVALID		= 0xFFFF3071,
	TZ_ERROR_SIGNATURE_INVALID	= 0xFFFF3072,
	TZ_ERROR_TIME_NOT_SET		= 0xFFFF5000,
	TZ_ERROR_TIME_NEEDS_RESET	= 0xFFFF5001,
};

typedef uint32_t	tz_errcode_t;

#endif /* _TZ_ERRCODE_H_ */
