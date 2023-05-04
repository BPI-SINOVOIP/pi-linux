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

#ifndef _TEE_INTERNAL_CORE_COMMON_H_
#define _TEE_INTERNAL_CORE_COMMON_H_

#include "tee_internal_common.h"

#define TEE_PARAM_TYPES(t0, t1, t2, t3)	\
	((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))
#define TEE_PARAM_TYPE_GET(t, i)	(((t) >> (i*4)) & 0xF)

#define TEE_PARAM_IS_VALUE(t)		\
	((t) >= TEE_PARAM_TYPE_VALUE_INPUT && (t) <= TEE_PARAM_TYPE_VALUE_INOUT)
#define TEE_PARAM_IS_MEMREF(t)		\
	((t) >= TEE_PARAM_TYPE_MEMREF_INPUT && (t) <= TEE_PARAM_TYPE_MEMREF_INOUT)

/** The TEE_Identity structure defines the full identity of a Client:
 * @param login		is one of the TEE_LOGIN_XXX constants. (See section 4.2.2.)
 * @param uuid		contains the client UUID or Nil (as defined in [4]) if not applicable.
*/
typedef struct
{
	uint32_t		login;
	TEE_UUID		uuid;
} TEE_Identity;


/* FIXME: to support 32-bit user-land APP and
 * 32-bit EL1 TZ kernel, change void * to uint32_t,
 * change size_t to uint32_t. needs to refine later
 * if to support 64-bit EL1 TZ kernel and 64-bit
 * user-land app.
 */
typedef union TEE_Param
{
	struct
	{
		uint32_t	buffer;
		uint32_t	size;
	} memref;
	struct
	{
		uint32_t	a;
		uint32_t	b;
	} value;
} TEE_Param;


/** Handle on sessions opened by a TA on another TA */
typedef void* TEE_TASessionHandle;

enum {
	TEE_PARAM_TYPE_NONE		= 0,
	TEE_PARAM_TYPE_VALUE_INPUT	= 1,
	TEE_PARAM_TYPE_VALUE_OUTPUT	= 2,
	TEE_PARAM_TYPE_VALUE_INOUT	= 3,
	TEE_PARAM_TYPE_MEMREF_INPUT	= 5,
	TEE_PARAM_TYPE_MEMREF_OUTPUT	= 6,
	TEE_PARAM_TYPE_MEMREF_INOUT	= 7,
};

enum {
	TEE_LOGIN_PUBLIC		= 0x00000000,
	TEE_LOGIN_USER			= 0x00000001,
	TEE_LOGIN_GROUP			= 0x00000002,
	TEE_LOGIN_APPLICATION		= 0x00000004,
	TEE_LOGIN_APPLICATION_USER	= 0x00000005,
	TEE_LOGIN_APPLICATION_GROUP	= 0x00000006,
	TEE_LOGIN_TRUSTED_APP		= 0xF0000000,
};

enum {
	TEE_ORIGIN_API			= 0x00000001,
	TEE_ORIGIN_COMMS		= 0x00000002,
	TEE_ORIGIN_TEE			= 0x00000003,
	TEE_ORIGIN_TRUSTED_APP		= 0x00000004,
};

#endif /* _TEE_INTERNAL_CORE_COMMON_H_ */
