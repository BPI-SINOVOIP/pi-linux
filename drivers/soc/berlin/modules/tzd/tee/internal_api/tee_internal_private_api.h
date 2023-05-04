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

#ifndef _TEE_INTERNAL_PRIVATE_API_H_
#define _TEE_INTERNAL_PRIVATE_API_H_

#include "tee_internal_core_common.h"
#include "tee_internal_storage_generic_api.h"
#include "mem_region.h"

typedef struct mem_region	TEE_MemRegion;
typedef struct reg_region	TEE_RegRegion;

#define TEE_MR_ATTR(perm, reg, zone, mattr, ctrl)		\
	MR_ATTR(perm, reg, zone, mattr, ctrl)

#define TEE_MR_M_PERM			MR_M_PERM
#define TEE_MR_M_ZONE			MR_M_ZONE
#define TEE_MR_M_TYPE			MR_M_TYPE
#define TEE_MR_M_MEM_ATTR		MR_M_MEM_ATTR
#define TEE_MR_M_CACHEABLE		MR_M_CACHEABLE
#define TEE_MR_M_DATA_ATTR		MR_M_DATA_ATTR
#define TEE_MR_M_PREMAPPED		MR_M_PREMAPPED

#define TEE_MR_PERM(mr)			MR_PERM(mr)
#define TEE_MR_ZONE(mr)			MR_ZONE(mr)
#define TEE_MR_WINDOW(mr)		MR_WINDOW(mr)
#define TEE_MR_MEM_ATTR(mr)		MR_MEM_ATTR(mr)
#define TEE_MR_IS_RESTRICTED(mr)	MR_IS_RESTRICTED(mr)
#define TEE_MR_IS_SECURE(mr)		MR_IS_SECURE(mr)
#define TEE_MR_IS_NONSECURE(mr)		MR_IS_NONSECURE(mr)
#define TEE_MR_IS_PREMAPPED(mr)		MR_IS_PREMAPPED(mr)
#define TEE_MR_IS_CACHEABLE(mr)		MR_IS_CACHEABLE(mr)
#define TEE_MR_IS_DATA_ATTR(mr)		MR_IS_DATA_ATTR(mr)
#define TEE_MR_IS_REGISTER(mr)		MR_IS_REGISTER(mr)
#define TEE_MR_IS_MEMORY(mr)		MR_IS_MEMORY(mr)

/** Get the memory region count
 *
 * @param attrMask	attribute mask to filter memory regions.
 * 			can be the flags in TEE_MR_M_*. 0 to get all.
 * 			only (region.attr & attrMask == attrVal) is filtered.
 * @param attrVal	attribute value to filter memory regions.
 * 			use TEE_MR_ATTR() to generate it. ignored if attrMask=0.
 *
 * @return	region number.
 */
uint32_t TEE_GetMemRegionCount(uint32_t attrMask, uint32_t attrVal);

/** Retrieve the memory region list.
 *
 * @param region	buffer to store the retrieved regions.
 *			NULL to return region number, same as
 *			TEE_GetMemRegionCount().
 * @param maxNum	max count can be retrieved (count region).
 * @param attrMask	attribute mask to filter memory regions.
 * 			can be the flags in TEE_MR_M_*. 0 to get all.
 * 			only (region.attr & attrMask == attrVal) is filtered.
 * @param attrVal	attribute value to filter memory regions.
 * 			use TEE_MR_ATTR() to generate it. ignored if attrMask=0.
 *
 * @return	retrieved region number. if maxNum < total matched region
 * 		number, then only maxNum would be copied, and return total
 * 		matched region_number.
 */
uint32_t TEE_GetMemRegionList(TEE_MemRegion *region, uint32_t maxNum,
		uint32_t attrMask, uint32_t attrVal);

/** Find memory region based on the input address.
 *
 * @param region	buffer to store the retrieved region.
 * @param buffer	buffer address to check.
 * @param size		size of the buffer, must not cross 2 regions.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_BAD_PARAMETERS	Parameter error.
 */
TEE_Result TEE_FindMemRegion(TEE_MemRegion *region,
		void *buffer, uint32_t size);

/** Find Read register region based on the input address.
 *
 * @param region	buffer to store the register region.
 * @param addr      address to check.
 *
 * @retval TEE_SUCCESS			    Pass the check.
 * @retval TEE_ERROR_BAD_PARAMETERS	Parameter error.
 * @retval TEE_ERROR_ACCESS_DENIED  address is not in the register region
 */
TEE_Result TEE_FindRegRegion(TEE_RegRegion *region,
		void *addr);

/*
 * Data attributes support to save memory in TrustZone.
 *
 * Data attributes are used to identify the content in secure memory is secure
 * or not.
 * With data attributes, we can store all the secure & non-secure content in
 * secure memory, so the memory size can be saved.
 *
 * In order to use it, the user must set the data buffer to be secure or
 * non-secure before use them.
 *
 * And in order to improve the access performance, read/write lock is used to
 * synchornize the data attributes list.
 */

/** Set data attributes.
 * @note must set before write data to the buffer.
 *
 * @param buffer	start address of the data.
 * @param size		size of the data.
 * @param attr		attributes of the data.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_BAD_PARAMETERS	Parameter error.
 * @retval TEE_ERROR_ACCESS_CONFLICT	The buffer is in use.
 */
TEE_Result TEE_SetDataAttribute(const void *buffer, size_t size, uint32_t attr);

/** Retrieve back data attributes.
 *
 * @param buffer	start address of the data.
 * @param size		size of the data.
 * @param attr		buffer to return attributes of the data.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_BAD_PARAMETERS	Parameter error.
 * @retval TEE_ERROR_BAD_STATE		Data attributes is not set.
 */
TEE_Result TEE_GetDataAttribute(const void *buffer, size_t size, uint32_t *attr);

/** Clear data attributes.
 *
 * @param buffer	start address of the data.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_BAD_PARAMETERS	Parameter error.
 * @retval TEE_ERROR_BAD_STATE		Data attributes is not set.
 */
TEE_Result TEE_ClearDataAttribute(const void *buffer);

/* Memory Transfer Operation */
typedef enum {
	TEE_MT_COPY,
	TEE_MT_ENCRYPT,
	TEE_MT_DECRYPT
} TEE_MemoryOperation;

/** check memory transfer rights.
 *
 * @param dst		destination address.
 * @param dstLen	destination data length
 * @param src		source address.
 * @param srcLen	source data length
 * @param op		Operation, see TEE_MemoryOperation.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_ACCESS_CONFLICT	Can't pass the check, master must not
 *					issue the transfer.
 *
 * Examples:
 * - memory to memory copy:
 *	res = TEE_CheckMemoryTransferRights(dst, dstLen, src, srcLen,
 *			TEE_MT_COPY);
 * - memory to memory decrypt:
 *	res = TEE_CheckMemoryTransferRights(dst, dstLen, src, srcLen,
 *			TEE_MT_DECRYPT);
 */
TEE_Result TEE_CheckMemoryTransferRights(
		const void*		dst,
		uint32_t		dstLen,
		const void*		src,
		uint32_t		srcLen,
		TEE_MemoryOperation	op);

/* memory access permision, see TZ_Sxx_Nxx in tz_perm.h */
#define TEE_NONSECURE				TZ_SRW_NRW
#define TEE_SECURE				TZ_SRW_NNA
#define TEE_RESTRICTED				TZ_SNA_NNA

/** check memory input rights.
 *
 * @param src		source address.
 * @param srcLen	source data length
 * @param dstPerm	desitination memory access permission, can be
 *			TEE_NONSECURE, TEE_SECURE.
 * @param op		Operation, see TEE_MemoryOperation.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_ACCESS_CONFLICT	Can't pass the check, master must not
 *					issue the transfer.
 *
 * Examples:
 * - memory to memory copy:
 *	res = TEE_CheckMemoryInputRights(src, srcLen,
 *			TEE_NONSECURE, TEE_MT_COPY);
 * - memory to memory decrypt:
 *	res = TEE_CheckMemoryTransferRights(src, srcLen,
 *			TEE_SECURE, TEE_MT_DECRYPT);
 */
TEE_Result TEE_CheckMemoryInputRights(
		const void*		src,
		uint32_t		srcLen,
		uint32_t		dstPerm,
		TEE_MemoryOperation	op);

/** check memory output rights.
 *
 * @param dst		destination address.
 * @param dstLen	destination data length
 * @param srcPerm	source memory access permission, can be
 *			TEE_NONSECURE, TEE_SECURE.
 * @param op		Operation, see TEE_MemoryOperation.
 *
 * @retval TEE_SUCCESS			Pass the check.
 * @retval TEE_ERROR_ACCESS_CONFLICT	Can't pass the check, master must not
 *					issue the transfer.
 *
 * Examples:
 * - memory to memory copy:
 *	res = TEE_CheckMemoryOutputRights(dst, dstLen,
 *			TEE_NONSECURE, TEE_MT_COPY);
 * - memory to memory decrypt:
 *	res = TEE_CheckMemoryTransferRights(dst, dstLen,
 *			TEE_SECURE, TEE_MT_DECRYPT);
 */
TEE_Result TEE_CheckMemoryOutputRights(
		const void*		dst,
		uint32_t		dstLen,
		uint32_t		srcPerm,
		TEE_MemoryOperation	op);

/** User level callback to client.
 * Callback to client and execute user registered callback
 *
 * @param commandID	command ID.
 * @param paramTypes	parameter types, see TEE_PARAM_TYPE.
 * @param params	the 4 parameters. here, for memref, it won't
 * 			be converted to client virtual address.
 * @param cancellationRequestTimeout	timeout in ms.
 * @param returnOrigin	return origin of the result.
 *
 * @retval TEE_SUCCESS	Succeed.
 * @retval TEE_ERROR_*	Error code, need check returnOrigin too.
 */
TEE_Result TEE_Callback(
	uint32_t		commandID,
	uint32_t		paramTypes,
	TEE_Param		params[4],
	uint32_t		cancellationRequestTimeout,
	uint32_t*		returnOrigin);

/**System level callback to client.
 * Callback to client and execute system registered callback
 *
 * @param commandID	command ID.
 * @param paramTypes	parameter types, see TEE_PARAM_TYPE.
 * @param params	the 4 parameters. here, for memref, it won't
 * 			be converted to client virtual address.
 * @param param_ext	extent param.
 * @param param_ext_size extent param size.
 * @param returnOrigin	return origin of the result.
 *
 * @retval TEE_SUCCESS	Succeed.
 * @retval TEE_ERROR_*	Error code, need check returnOrigin too.
 */
TEE_Result TEE_SysCallback(
	uint32_t		commandID,
	uint32_t		paramTypes,
	TEE_Param		params[4],
	void			*param_ext,
	uint32_t		param_ext_size,
	uint32_t*		returnOrigin);

/*
 * Physical/Virtual Address Conversion API
 */
#define TEE_PhysToVirt(phys)	(phys)
#define TEE_VirtToPhys(virt)	(virt)


/*
 * Cache Operation
 */

/** invalidate dcache by range.
 *
 * @param start		virtual start address of region
 * @param size		size of the region
 */
TEE_Result TEE_InvalidateCache(void *start, size_t size);

/** clean dcache by range.
 *
 * @param start		virtual start address of region
 * @param size		size of region
 */
TEE_Result TEE_CleanCache(void *start, size_t size);

/** clean & invalidate dcache by range.
 *
 * Ensure that the data held in the page addr is written back
 * to the page in question.
 * @param start		virtual start address of region
 * @param size		size of region
 */
TEE_Result TEE_FlushCache(void *start, size_t size);

/** create mutex.
 *
 * @param lock		return the lock handle.
 * @param name		name of the mutex.
 * 			if different modules use same mutex name,
 * 			they will share same mutex handle.
 * 			if name==NULL, then it would create a new
 * 			anonymous mutex. to avoid duplicate name,
 *			it is suggested to use UUID.
 *
 * @retval TEE_SUCCESS	Success to create the mutex.
 * @retval TEE_ERROR_*	Error code if fail to create the mutex.
 */
TEE_Result TEE_MutexCreate(void **lock, const char *name);

/** destroy mutex.
 *
 * @param lock		mutex handle.
 *
 * @retval TEE_SUCCESS	success to destroy the lock.
 * @retval TEE_ERROR_*	Error code if fail to destroy the mutex.
 */
TEE_Result TEE_MutexDestroy(void *lock);

/** lock mutex.
 *
 * @param lock		mutex handle.
 *
 * @retval TEE_SUCCESS	success to lock the mutex.
 * @retval TEE_ERROR_*	Error code if fail to lock the mutex.
 */
TEE_Result TEE_MutexLock(void *lock);

/** try to lock mutex.
 *
 * @param lock		mutex handle.
 *
 * @retval TEE_SUCCESS	success to trylock the mutex.
 * @retval TEE_ERROR_BAD_STATE	lock is owned by others.
 * @retval TEE_ERROR_*	Error code if fail to trylock the mutex.
 */
TEE_Result TEE_MutexTryLock(void *lock);

/** unlock mutex.
 *
 * @param lock		mutex handle.
 *
 * @retval TEE_SUCCESS	success to unlock the mutex.
 * @retval TEE_ERROR_*	Error code if fail to unlock the mutex.
 */
TEE_Result TEE_MutexUnlock(void *lock);


/** create semaphore.
 *
 * @param sem		return the sem handle.
 * @param value		initial value of the semaphore.
 * @param name		name of the semaphore.
 * 			if different modules use same semaphore name,
 * 			they will share same semaphore handle. and the initial
 * 			value is ignored. to avoid duplicate name, it is suggested
 *			to use UUID.
 * 			if name==NULL, then it would create a new
 * 			anonymous semaphore.
 *
 * @retval TEE_SUCCESS	Success to create the semaphore.
 * @retval TEE_ERROR_*	Error code if fail to create the semaphore.
 */
TEE_Result TEE_SemaphoreCreate(void **sem, int value, const char *name);

/** destroy semaphore.
 *
 * @param sem		semaphore handle.
 *
 * @retval TEE_SUCCESS	success to destroy the sem.
 * @retval TEE_ERROR_*	Error code if fail to destroy the semaphore.
 */
TEE_Result TEE_SemaphoreDestroy(void *sem);

/** wait semaphore, and decrease the count
 *
 * @param sem		semaphore handle.
 *
 * @retval TEE_SUCCESS	success to wait the semaphore.
 * @retval TEE_ERROR_*	Error code if fail to wait the semaphore.
 */
TEE_Result TEE_SemaphoreWait(void *sem);

/** wait semaphore until timeout.
 *
 * @param sem		semaphore handle.
 * @param timeout	timeout in milliseconds.
 *
 * @retval TEE_SUCCESS	success to wait the semaphore.
 * @retval TEE_ERROR_TIMEOUT	timeout reached.
 * @retval TEE_ERROR_*	Error code if fail to wait the semaphore.
 */
TEE_Result TEE_SemaphoreTimedWait(void *sem, uint32_t timeout);

/** try to wait semaphore.
 *
 * it's similar as TEE_SemaphoreTimedWait(sem, 0).
 *
 * @param sem		semaphore handle.
 *
 * @retval TEE_SUCCESS	success to trywait the semaphore.
 * @retval TEE_ERROR_BAD_STATE	sem is owned by others.
 * @retval TEE_ERROR_*	Error code if fail to trywait the semaphore.
 */
TEE_Result TEE_SemaphoreTryWait(void *sem);

/** post semaphore, and increase the count.
 *
 * @param sem		semaphore handle.
 *
 * @retval TEE_SUCCESS	success to post the semaphore.
 * @retval TEE_ERROR_*	Error code if fail to post the semaphore.
 */
TEE_Result TEE_SemaphorePost(void *sem);

typedef enum {
	TEE_BOOT_MODE_NORMAL	= 0,	/* Normal Boot */
	TEE_BOOT_MODE_RECOVERY	= 1,	/* Recovery Boot */
	TEE_BOOT_MODE_MAX
} TEE_BootMode;

/** Get boot mode.
 *
 * @param mode		buffer to return boot mode, see TEE_BootMode.
 *
 * @retval TEE_SUCCESS	success to get the info.
 */
TEE_Result TEE_GetBootMode(TEE_BootMode *mode);

typedef struct {
	uint32_t commVer;		/* Common Version */
	uint32_t custVer;		/* Customer Version */
} TEE_AntiRollbackInfo;

/** Get anti-rollback info.
 *
 * @param info		buffer to return anti-rollback versions.
 *
 * @retval TEE_SUCCESS	success to get the info.
 */
TEE_Result TEE_GetAntiRollbackInfo(TEE_AntiRollbackInfo *info);

/** Verify BCM image.
 *
 * @param in		buffer to keep the enced image.
 * @param inLen		length of the enced image
 * @param out		buffer to keep the decrypted image
 * @param outLen	buffer to keep the return length
 *
 * @retval TEE_SUCCESS			success to get the info.
 * @retval TZ_ERROR_BAD_PARAMETERS	invalid parameters.
 */
TEE_Result TEE_VerifyImage(const void *in, uint32_t inLen, void *out,
		uint32_t *outLen, uint32_t codeType);

/** send command to secure processor and wait until its completion
 *
 * @param cmd_id	command ID
 * @param param		buffer of the input param
 * @param param_len	parameter length
 * @param result	buffer to keep the result
 * @param result_len	result length
 *
 * @retval TEE_SUCCESS	success to get the info.
 */
TEE_Result TEE_ExecCmd(size_t cmd_id, const void *param, size_t param_len,
		const void *result, size_t result_len);

/** allocate shared memory.
 *
 * @param name		shared memory name. NULL for anonymous.
 *			if it's not NULL, it tries to find the named shared
 *			memory first.
 * @param len		shared memory size in bytes.
 * @param align		alignment size.
 * @param shmFlags		reserved for permissions.
 * @param shm		buffer to return the shm handle.
 *
 * @retval TEE_SUCCESS			success.
 * @retval TEE_ERROR_BAD_PARAMETERS	invalid parameters.
 * @retval TEE_ERROR_OUT_OF_MEMORY	out of memory.
 */
TEE_Result TEE_AllocateSharedMemory(const char *name, size_t len,
		size_t align, uint32_t shmFlags, TEE_ObjectHandle *shm);

/** release shared memory.
 *
 * @param shm		shm handle to release.
 *
 * @retval TEE_SUCCESS			success.
 * @retval TEE_ERROR_BAD_PARAMETERS	shm == NULL.
 * @retval TEE_ERROR_ITEM_NOT_FOUND	the shared memory handle is invalid.
 */
TEE_Result TEE_ReleaseSharedMemory(TEE_ObjectHandle shm);

/** retrieve shared memory ID with shared memory handle.
 *
 * @param shm		shm handle.
 * @param shmID		buffer to return shared memory ID, which can be shared
 *			cross TA instances.
 *
 * @retval TEE_SUCCESS			success.
 * @retval TEE_ERROR_BAD_PARAMETERS	shm == NULL or shmID == NULL.
 * @retval TEE_ERROR_ITEM_NOT_FOUND	the shared memory handle is invalid.
 */
TEE_Result TEE_RetrieveSharedMemoryID(TEE_ObjectHandle shm, uint32_t *shmID);

/** register shared memory with shared memory ID.
 * must call TEE_RegisterSharedMemory() to release it after use it.
 *
 * @param shmID		Shared memory ID shared from other TA instance.
 * @param shm		buffer to return the shm handle.
 *
 * @retval TEE_SUCCESS			success.
 * @retval TEE_ERROR_BAD_PARAMETERS	shm == NULL
 * @retval TEE_ERROR_ITEM_NOT_FOUND	the shared memory ID is invalid.
 */
TEE_Result TEE_RegisterSharedMemory(uint32_t shmID, TEE_ObjectHandle *shm);

/** map shared memory to current TA virtual memory space.
 *
 * @param shm		shm handle.
 * @param offset	start offset of the shared memory to map.
 * @param len		length to map. 0 to map whole shm.
 * @param va		buffer to return the mapped virtual address.
 *
 * @retval TEE_SUCCESS			success.
 * @retval TEE_ERROR_BAD_PARAMETERS	shm == NULL or va == NULL.
 * @retval TEE_ERROR_ITEM_NOT_FOUND	the shared memory handle is invalid.
 */
TEE_Result TEE_MapSharedMemory(TEE_ObjectHandle shm, size_t offset,
		size_t len, void **va);

/** unmap shared memory from current TA virtual memory space.
 *
 * @param shm		shm handle.
 * @param va		virtual address to unmap.
 * @param len		length to unmap.
 *
 * @retval TEE_SUCCESS			success.
 * @retval TEE_ERROR_BAD_PARAMETERS	shm == NULL or va == NULL.
 * @retval TEE_ERROR_ITEM_NOT_FOUND	the shared memory handle is invalid.
 */
TEE_Result TEE_UnmapSharedMemory(TEE_ObjectHandle shm, void *va, size_t len);

/** RPMB set key
 *
 * @param key	the rpmb key
 * @retval TEE_SUCCESS	return TEE_SUCCESS
 */
TEE_Result TEE_RPMB_SetKey(uint8_t *key);

/** RPMB read block
 *
 * @param devName	name of rpmb device, eg: /dev/block/rpmb
 * @param addr		the address of rpmb write,unit is 256B
 * @param data		the data read from rpmb
 * @param len		size of data in bytes
 * @retval TEE_SUCCESS	return TEE_SUCCESS
 */
TEE_Result TEE_RPMB_Read(const char *devName,
			uint16_t addr,
			uint8_t *data,
			uint32_t len);
/** RPMB write block
 *
 * @param addr		the address of rpmb write,unit is 256B
 * @param data		the data write to rpmb
 * @parm len		the len of data, size should be <= 256
 * @retval TEE_SUCCESS	return TEE_SUCCESS
 */
TEE_Result TEE_RPMB_Write(const char *devName,
			    uint16_t addr,
			    uint8_t *data,
			    uint32_t len);

#endif /* _TEE_INTERNAL_PRIVATE_API_H_ */
