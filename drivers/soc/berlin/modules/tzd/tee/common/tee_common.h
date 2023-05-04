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

#ifndef _TEE_COMMON_H_
#define _TEE_COMMON_H_

#include "types.h"

/** Return Code.
 *
 * It's same as TEEC_Result.
 */
typedef uint32_t TEE_Result;

/** Universally Unique IDentifier (UUID) type as defined in [RFC4122].A
 *
 * UUID is the mechanism by which a service (Trusted Application) is
 * identified.
 * It's same as TEEC_UUID.
 */
typedef struct
{
	uint32_t		timeLow;
	uint16_t		timeMid;
	uint16_t		timeHiAndVersion;
	uint8_t			clockSeqAndNode[8];
} TEE_UUID;

#endif /* _TEE_COMMON_H_ */
