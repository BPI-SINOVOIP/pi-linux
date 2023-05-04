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

#ifndef _TEE_INTERNAL_CRYPTO_GENERIC_API_H_
#define _TEE_INTERNAL_CRYPTO_GENERIC_API_H_

#include "tee_internal_crypto_common.h"

/** Allocate a handle for a new cryptographic operation.
 *
 * @param operation	reference to generated operation handle
 * @param algorithm	one of the cipher algorithms enumerated in
 *			TEE_CRYPTO_ALGORITHM_ID
 * @param mode		mode for the current operation, as specified by the
 * 			enumeration TEE_OperationMode
 * @param maxKeySize	maximum key size that is in use by the algorithm
 *
 * @retval TEE_SUCCESS			on success
 * @retval EE_ERROR_SHORT_BUFFER	if the output buffer is not large enough
 *					to allocate the operation
 * @retval TEE_ERROR_NOT_SUPPORTED	if the mode is not compatible with the
 * 					algorithm or key size or if the algorithm
 *					is not one of the listed algorithms
 */
TEE_Result TEE_AllocateOperation(TEE_OperationHandle *operation,
				uint32_t algorithm, uint32_t mode,
				uint32_t maxKeySize);

/** Deallocate all resources associated with an operation handle.
 *
 * @param operation	pointer to operation handle
 */
void TEE_FreeOperation(TEE_OperationHandle operation);

/** Fill in the operationInfo structure associated with an operation.
 *
 * @param operation	operation handle
 *
 * @param operationInfo	pointer to a structure filled with the operation information
 */
void TEE_GetOperationInfo(TEE_OperationHandle operation,
				TEE_OperationInfo* operationInfo);

/** Associate an operation with a key.
 *
 * @param operation	operation handle
 * @param key		handle on a key object
 *
 * @return	The only possible return value is TEE_SUCCESS
 */
TEE_Result TEE_SetOperationKey(TEE_OperationHandle operation,
				TEE_ObjectHandle key);

#endif /* _TEE_INTERNAL_CRYPTO_GENERIC_API_H_ */
