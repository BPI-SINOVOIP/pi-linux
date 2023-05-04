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

#ifndef _TEE_INTERNAL_MEM_API_H_
#define _TEE_INTERNAL_MEM_API_H_

#include "tee_internal_core_common.h"

enum TEE_AccessFlags {
	TEE_ACCESS_READ			= 0x00000001,
	TEE_ACCESS_WRITE		= 0x00000002,
	TEE_ACCESS_ANY_OWNER		= 0x00000004,
};

enum TEE_MallocHint {
	TEE_MALLOC_ZEROS		= 0x00000000,
	TEE_MALLOC_FLAG_DEFAULT		= TEE_MALLOC_ZEROS,
	TEE_MALLOC_FLAG_SYS_MAX		= 0x7fffffff,
	TEE_MALLOC_FLAG_USER_START	= 0x80000000,
	TEE_MALLOC_FLAG_USER_MAX	= 0xffffffff
};

#define TEE_MallocHintIsUser(hint)	((hint) & 0x80000000)
#define TEE_MallocHintIsSys(hint)	(!TEE_MallocHintIsUser(hint))

TEE_Result TEE_CheckMemoryAccessRights(
	uint32_t		accessFlags,
	void*			buffer,
	size_t			size);

void TEE_SetInstanceData(
	void*			instanceData);

void* TEE_GetInstanceData(void);

void* TEE_Malloc(
	size_t			size,
	uint32_t		hint);

/**
 * @brief
 *
 * @param buffer
 * @param newSize
 *
 * @return
 */
void* TEE_Realloc(
	void*			buffer,
	uint32_t		newSize);

/**
 * @brief
 *
 * @param buffer
 */
void TEE_Free(void*		buffer);

/** Copies size bytes from the object pointed to by src into the object pointed
 * to by dest
 *
 * @param dest		destination buffer
 * @param src		source buffer
 * @param size		number of bytes to be copied
 *
 * @sa memcpy() in C
 */
void TEE_MemMove(
	void*			dest,
	void*			src,
	uint32_t		size);

/** Compares the first size bytes of the object pointed to by buffer1 to the
 * first size bytes of the object pointed to by buffer2.
 *
 * @param buffer1	A pointer to the first buffer
 * @param buffer2	A pointer to the second buffer
 * @param size		The number of bytes to be compared
 *
 * @retval >0		If the first byte that differs is higher in buffer1
 * @retval =0		If the first size bytes of the two buffers are identical
 * @retval <0		If the first byte that differs is higher in buffer2
 *
 * @note buffer1 and buffer2 can reside in any kinds of memory, including
 * shared memory.
 * @sa memcmp() in C
 */
int32_t TEE_MemCompare(
	void*			buffer1,
	void*			buffer2,
	uint32_t		size);

/** Fill memory with byte x.
 *
 * @param buffer	A pointer to the destination buffer
 * @param x		The value to be set. Will convert to uint8_t.
 * @param size		The number of bytes to be set
 *
 * @sa memset() in C
 */
void TEE_MemFill(
	void*			buffer,
	uint32_t		x,
	uint32_t		size);

#endif /* _TEE_INTERNAL_MEM_API_H_ */
