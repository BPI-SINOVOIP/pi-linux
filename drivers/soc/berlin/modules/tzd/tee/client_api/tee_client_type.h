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

#ifndef _TEE_CLIENT_TYPE_H_
#define _TEE_CLIENT_TYPE_H_

#include "tee_common.h"

/** Return Code. */
typedef TEE_Result TEEC_Result;

/** Universally Unique IDentifier (UUID) type as defined in [RFC4122].A
 *
 * UUID is the mechanism by which a service (Trusted Application) is
 * identified.
 */
typedef TEE_UUID TEEC_UUID;

typedef struct TEEC_Session	TEEC_Session;
typedef struct TEEC_Operation	TEEC_Operation;

typedef TEEC_Result (*TEEC_Callback)(
	TEEC_Session*		session,
	uint32_t		commandID,
	TEEC_Operation*		operation,
	void*			userdata);

/** TEEC_Context is the main logical container linking a Client Application
 * (Service) with a particular TEE.
 *
 */
typedef struct TEEC_Context
{
/*! Implementation-defined variables */
/*! Device identifier */
	uint32_t		fd;
/*! Sessions count of the device */
	int			sessionCount;
/*! Shared memory counter which got created for this context */
	uint32_t		sharedMemCount;
/*! Shared memory list */
	//struct list sharedMemList;
/*! Error number from the client driver */
	int			err;

/*! User-defined variables, can only be set after InitializeContext */
	void *			userdata;
} TEEC_Context;

/** TEEC_Session is the logical container linking a Client Application
 * (Client) with a particular Trusted Application (Service).
 */
struct TEEC_Session
{
/*! Implementation-defined variables */
/*! Reference count of operations */
	int			operationCount;
/*! Task id obtained for the session */
	uint32_t		taskId;
/*! Session id obtained for the session */
	uint32_t		sessionId;
/*! Unique service id */
	TEEC_UUID		serviceId;
/*! Device context */
	TEEC_Context*		device;
/*! Service error number */
	int			err;
/*! Callback Routine, only tee handler is valid */
	TEEC_Callback		Callback;
	void*			callbackUserdata;
/*! Communication Channel */
	struct tee_comm*	comm;
/*! mutex to avoid mutiple access */
	void *			lock;

/*! User-defined variables, can only be set after OpenSession */
	void *			userdata;
};

/** TEEC_SharedMemory denotes a Shared Memory block that is mapped between
 * the client and the service.
 */
typedef struct TEEC_SharedMemory
{
/*! The pointer to the memory buffer shared with TEE. */
	void*			buffer;
/*! The size of the shared memory buffer in bytes. Should not be zero */
	size_t			size;
/*! flags is a bit-vector which can contain the following flags:
 *  - TEEC_MEM_INPUT: the memory can be used to transfer data from the
 *  Client Application to the TEE.
 *  - TEEC_MEM_OUTPUT: The memory can be used to transfer data from the
 *  TEE to the Client Application.
 *  - All other bits in this field SHOULD be set to zero, and are reserved for
 *  future use.
 */
	uint32_t		flags;

/*! Implementation defined fields. */
/*! Device context */
	TEEC_Context*		context;
/*! Operation count */
	int			operationCount;
/*! Shared memory type */
	bool			allocated;
/*! physical address.
 * for allocated shared memory, buffer is virtual address, and phyAddr is
 * physical address.
 * for registered shared memory, both buffer and phyAddr are physical address.
 */
	void*			phyAddr;
/*! List head used by Context */
	//struct list head_ref;
/*! Service error number */
	int			err;
	uint32_t		attr;

/*! User defined fields. */
	void*			userdata;
} TEEC_SharedMemory;

/** Temporary shared memory reference
 *
 * It is used as a TEEC_Operation parameter when the corresponding parameter
 * type is one of  TEEC_MEMREF_TEMP_INPUT,  TEEC_MEMREF_TEMP_OUTPUT, or
 * TEEC_MEMREF_TEMP_INOUT.
 */
typedef struct TEEC_TempMemoryReference
{
/*! buffer is a pointer to the first byte of a region of memory which needs
 * to be temporarily registered for the duration of the Operation.
 * This field can be NULL to specify a null Memory Reference.
 */
	void*			buffer;
/*! size of the referenced memory region. When the operation completes, and
 * unless the parameter type is TEEC_MEMREF_TEMP_INPUT, the Implementation
 * must update this field to reflect the actual or required size of the output:
 * - If the Trusted Application has actually written some data in the output
 *   buffer, then the Implementation MUST update the size field with the actual
 *   number of bytes written.
 * - If the output buffer was not large enough to contain the whole output,
 *   or if it is null, the Implementation MUST update the size field with
 *   the size of the output buffer requested by the Trusted Application.
 *   In this case, no data has been written into the output buffer
 */
	size_t			size;
} TEEC_TempMemoryReference;

/** Registered memory reference
 *
 * A pre-registered or pre-allocated Shared Memory block.
 * It is used as a TEEC_Operation parameter when the corresponding parameter
 * type is one of TEEC_MEMREF_WHOLE, TEEC_MEMREF_PARTIAL_INPUT,
 * TEEC_MEMREF_PARTIAL_OUTPUT, or TEEC_MEMREF_PARTIAL_INOUT.
 */
typedef struct TEEC_RegisteredMemoryReference
{
/*! Pointer to the shared memory structure.
 *
 * The memory reference refers either to the whole Shared Memory or
 * to a partial region within the Shared Memory block, depending of the
 * parameter type. The data flow direction of the memory reference
 * must be consistent with the flags defined in the parent Shared Memory Block.
 * Note that the parent field MUST NOT be NULL. To encode a null
 * Memory Reference, the Client Application must use a Temporary Memory
 * Reference with the buffer field set to NULL. */
	TEEC_SharedMemory*	parent;

/*! Size of the referenced memory region, in bytes.
 *
 * - The Implementation MUST only interpret this field if the Memory Reference
 * type in the operation structure is not TEEC_MEMREF_WHOLE. Otherwise,
 * the size is read from the parent Shared Memory structure.
 * - When an operation completes, and if the Memory Reference is
 * tagged as "output", the Implementation must update this field to reflect
 * the actual or required size of the output. This applies even if the
 * parameter type is TEEC_MEMREF_WHOLE:
 *   o If the Trusted Application has actually written some data in the
 * output buffer, then the Implementation MUST update the size field with the
 * actual number of bytes written.
 *   o If the output buffer was not large enough to contain the whole output,
 * the Implementation MUST update the size field with the size of  the output
 * buffer requested by the Trusted Application. In this case, no data has been
 * written into the output buffer.
 */
	size_t			size;

/*! Offset from the start of allocated Shared memory for reference, in bytes.
 *
 * The Implementation MUST only interpret this field if the
 * Memory Reference type in the operation structure is not TEEC_MEMREF_WHOLE.
 * Otherwise, the Implementation MUST use the base address of the
 * Shared Memory block.
 */
	size_t			offset;
} TEEC_RegisteredMemoryReference;


/** Small raw data value for operation
 *
 * This type defines a parameter that is not referencing shared memory,
 * but carries instead small raw data passed by value.
 * It is used as a TEEC_Operation parameter when the corresponding
 * parameter type is one of
 * TEEC_VALUE_INPUT, TEEC_VALUE_OUTPUT, or TEEC_VALUE_INOUT.
 */
typedef struct TEEC_Value
{
/*! The two fields of this structure do not have a particular meaning.
 * It is up to the protocol between the Client Application and
 * the Trusted Application to assign a semantic to those two integers.
 */
   uint32_t			a;
   uint32_t			b;
} TEEC_Value;


/** Parameter of a TEEC_Operation
 *
 * It can be a Temporary Memory Reference, a Registered Memory Reference,
 * or a Value Parameter.
 */
typedef union TEEC_Parameter
{
/*! Temporary Memory Reference parameter.
 * For parameter type:
 * - TEEC_MEMREF_TEMP_INPUT
 * - TEEC_MEMREF_TEMP_OUTPUT
 * - TEEC_MEMREF_TEMP_INOUT
 */
	TEEC_TempMemoryReference	tmpref;

/*! Registered Memory Reference parameter.
 * For parameter type:
 * - TEEC_MEMREF_WHOLE
 * - TEEC_MEMREF_PARTIAL_INPUT
 * - TEEC_MEMREF_PARTIAL_OUTPUT
 * - TEEC_MEMREF_PARTIAL_INOUT
 */
	TEEC_RegisteredMemoryReference	memref;

/*! Small Raw Value parameter.
 * For parameter type:
 * - TEEC_VALUE_INPUT
 * - TEEC_VALUE_OUTPUT
 * - TEEC_VALUE_INOUT
 */
	TEEC_Value			value;
} TEEC_Parameter;

/** Payload of either an open Session operation or an invoke Command operation
 * with TEE.
 *
 * It is also used for cancellation of operations, which may be desirable even
 * if no payload is passed.
 */
struct TEEC_Operation
{
/*! Indicator for whehter the operation is started.
 * This field which MUST be initialized to zero by the Client Application
 * before each use in an operation if the Client Application may need to
 * cancel the operation about to be performed.
 */
	uint32_t		started;

/*! paramTypes field encodes the type of each of the Parameters in the
 * operation. The layout of these types within a 32-bit integer is
 * implementation-defined and the Client Application MUST use the
 * macro TEEC_PARAMS_TYPE to construct a constant value for this field.
 * As a special case, if the Client Application sets paramTypes to 0,
 * then the Implementation MUST interpret it as meaning that the type for each
 * Parameter is set to TEEC_NONE.
 * The type of each Parameter can take one of the following values
 * - TEEC_NONE
 * - TEEC_VALUE_INPUT
 * - TEEC_VALUE_OUTPUT
 * - TEEC_VALUE_INOUT
 * - TEEC_MEMREF_TEMP_INPUT
 * - TEEC_MEMREF_TEMP_OUTPUT
 * - TEEC_MEMREF_TEMP_INOUT
 * - TEEC_MEMREF_WHOLE
 * - TEEC_MEMREF_PARTIAL_INPUT
 * - TEEC_MEMREF_PARTIAL_OUTPUT
 * - TEEC_MEMREF_PARTIAL_INOUT
 */
	uint32_t		paramTypes;

/*! params is an array of four Parameters. For each parameter, one of the
 * memref, tmpref, or value fields must be used depending on the corresponding
 * parameter type passed in paramTypes as described in the specification
 * of TEEC_Parameter
 */
	TEEC_Parameter		params[4];
/*! Implementation defined fields. */
/*! session of the operaction */
	TEEC_Session*		session;

/*! User defined fields. */
	void*			userdata;
};

/*
 * TEEC property
 */
typedef struct TEEC_Property TEEC_Property;

#endif /* _TEE_CLIENT_TYPE_H_ */
