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

#ifndef _TEE_CLIENT_API_H_
#define _TEE_CLIENT_API_H_

#include "tee_client_config.h"
#include "tee_client_const.h"
#include "tee_client_type.h"

#define TEEC_PARAM_TYPES(t0, t1, t2, t3)	\
	((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))
#define TEEC_PARAM_TYPE_GET(t, i)	(((t) >> (i*4)) & 0xF)

#define TEEC_PARAM_IS_VALUE(t)			\
	((t) >= TEEC_VALUE_INPUT && (t) <= TEEC_VALUE_INOUT)
#define TEEC_PARAM_IS_MEMREF(t)			\
	((t) >= TEEC_MEMREF_TEMP_INPUT && (t) <= TEEC_MEMREF_PARTIAL_INOUT)

/** Initialize Context
 *
 * This function initializes a new TEE Context, forming a connection between
 * this Client Application and the TEE identified by the string identifier
 * name.
 * The Client Application MAY pass a NULL name, which means that the
 * Implementation MUST select a default TEE to connect to.
 * The supported name strings, the mapping of these names to a specific TEE,
 * and the nature of the default TEE are implementation-defined.
 * The caller MUST pass a pointer to a valid TEEC Context in context.
 * The Implementation MUST assume that all fields of the TEEC_Context structure
 * are in an undefined state.
 *
 * \b Programmer \b Error
 * The following usage of the API is a programmer error:
 * Attempting to initialize the same TEE Context structure concurrently
 * from multiple threads. Multi-threaded Client Applications must use
 * platform-provided locking mechanisms to ensure that this case
 * does not occur.
 * \b Implementers’ \b Notes
 * It is valid Client Application behavior to concurrently initialize
 * different TEE Contexts, so the Implementation MUST support this.
 *
 * @param name		A zero-terminated string that describes the TEE to
 *			connect to. If this parameter is set to NULL the
 *			Implementation MUST select a default TEE.
 *
 * @param context	A TEEC_Context structure that MUST be initialized by the
 *			Implementation.
 *
 * @retval TEEC_SUCCESS	The initialization was successful.
 * @retval TEEC_ERROR_*	An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_InitializeContext(
	const char*		name,
	TEEC_Context*		context);


/** Finalizes an initialized TEE context.
 *
 * This function finalizes an initialized TEE Context,
 * closing the connection between the Client Application and the TEE.
 * The Client Application MUST only call this function when all Sessions
 * inside this TEE Context have been closed and all
 * Shared Memory blocks have been released.
 * The implementation of this function MUST NOT be able to fail:
 * after this function returns the Client Application must be able to
 * consider that the Context has been closed.
 * The function implementation MUST do nothing if context is NULL.
 *
 * \b Programmer \b Error
 * The following usage of the API is a programmer error:
 *  - Calling with a context which still has sessions opened.
 *  - Calling with a context which contains unreleased Shared Memory blocks.
 *  - Attempting to finalize the same TEE Context structure concurrently
 *    from multiple threads.
 *  - Attempting to finalize the same TEE Context structure more than once,
 *    without an intervening call to TEEC_InitalizeContext.
 *
 * @param context	An initialized TEEC_Context structure which is to be
 *			finalized.
 */
void TEEC_FinalizeContext(
	TEEC_Context*		context);


/** Register a allocated shared memory block.
 *
 * This function registers a block of existing Client Application memory as a
 * block of Shared Memory within the scope of the specified TEE Context,
 * in accordance with the parameters which have been set by the
 * Client Application inside the \a sharedMem structure.
 *
 * The parameter \a context MUST point to an initialized TEE Context.
 *
 * The parameter \a sharedMem MUST point to the Shared Memory structure
 * defining the memory region to register.
 * The Client Application MUST have populated the following fields of the
 * Shared Memory structure before calling this function:
 * The \a buffer field MUST point to the memory region to be shared,
 * and MUST not be NULL.
 * The \a size field MUST contain the size of the buffer, in bytes.
 * Zero is a valid length for a buffer.
 * The \a flags field indicates the intended directions of data flow
 * between the Client Application and the TEE.
 * The Implementation MUST assume that all other fields in the Shared Memory
 * structure have undefined content.
 *
 * An Implementation MAY put a hard limit on the size of a single
 * Shared Memory block, defined by the constant TEEC_CONFIG_SHAREDMEM_MAX_SIZE.
 * However note that this function may fail to register a
 * block smaller than this limit due to a low resource condition
 * encountered at run-time.
 *
 * \b Programmer \b Error
 * The following usage of the API is a programmer error:
 * -  Calling with a \a context which is not initialized.
 * -  Calling with a \a sharedMem which has not be correctly populated
 *    in accordance with the specification.
 * -  Attempting to initialize the same Shared Memory structure concurrently
 *    from multiple threads.Multi-threaded Client Applications must use
 *    platform-provided locking mechanisms to ensure that
 *    this case does not occur.
 *
 * \b Implementor's \b Notes
 * This design allows a non-NULL buffer with a size of 0 bytes to allow
 * trivial integration with any implementations of the C library malloc,
 * in which is valid to allocate a zero byte buffer and receive a non-
 * NULL pointer which may not be de-referenced in return.
 * Once successfully registered, the Shared Memory block can be used for
 * efficient data transfers between the Client Application and the
 * Trusted Application. The TEE Client API implementation and the underlying
 * communications infrastructure SHOULD attempt to transfer data in to the
 * TEE without using copies, if this is possible on the underlying
 * implementation, but MUST fall back on data copies if zero-copy cannot be
 * achieved. Client Application developers should be aware that,
 * if the Implementation requires data copies,
 * then Shared Memory registration may allocate a block of memory of the
 * same size as the block being registered.
 *
 * @param context	A pointer to an initialized TEE Context
 * @param sharedMem	A pointer to a Shared Memory structure to register:
 *			the \a buffer, \a size, and \a flags fields of the
 *			sharedMem structure MUST be set in accordance with
 *			the specification described above
 *
 * @retval TEEC_SUCCESS	The registration was successful.
 * @retval TEEC_ERROR_OUT_OF_MEMORY	the registration could not be
 *			completed because of a lack of resources
 * @retval TEEC_ERROR_*	An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_RegisterSharedMemory(
	TEEC_Context*      	context,
	TEEC_SharedMemory* 	sharedMem);


/** Allocate a shared memory block.
 *
 * This function allocates a new block of memory as a block of Shared Memory
 * within the scope of the specified TEE Context, in accordance with the
 * parameters which have been set by the Client Application inside the
 * \a sharedMem structure.
 *
 * The parameter \a context MUST point to an initialized TEE Context.
 *
 * The \a sharedMem parameter MUST point to the Shared Memory structure
 * defining the region to allocate.
 * Client Application MUST have populated the following fields of the
 * Shared Memory structure:
 * The \a size field MUST contain the desired size of the buffer, in bytes.
 * The size is allowed to be zero. In this case memory is allocated and
 * the pointer written in to the buffer field on return MUST not be NULL
 * but MUST never be de-referenced by the Client Application. In this case
 * however, the Shared Memory block can be used in
 * Registered Memory References.
 * The \a flags field indicates the allowed directions of data flow
 * between the Client Application and the TEE.
 * The Implementation MUST assume that all other fields in the Shared Memory
 * structure have undefined content.
 *
 * An Implementation MAY put a hard limit on the size of a single
 * Shared Memory block, defined by the constant
 * \a TEEC_CONFIG_SHAREDMEM_MAX_SIZE.
 * However note that this function may fail to allocate a
 * block smaller than this limit due to a low resource condition
 * encountered at run-time.
 *
 * If this function returns any code other than \a TEEC_SUCCESS
 * the Implementation MUST have set the \a buffer field of \a sharedMem to NULL.
 *
 *
 * \b Programmer \b Error
 * The following usage of the API is a programmer error:
 * -  Calling with a \a context which is not initialized.
 * -  Calling with a \a sharedMem which has not be correctly populated
 *    in accordance with the specification.
 * -  Attempting to initialize the same Shared Memory structure concurrently
 *    from multiple threads.Multi-threaded Client Applications must use
 *    platform-provided locking mechanisms to ensure that
 *    this case does not occur.
 *
 * \b Implementor's \b Notes
 * Once successfully allocated the Shared Memory block can be used for
 * efficient data transfers between the Client Application and the
 * Trusted Application. The TEE Client API and the underlying communications
 * infrastructure should attempt to transfer data in to the TEE
 * without using copies, if this is possible on the underlying implementation,
 * but may have to fall back on data copies if zero-copy cannot be achieved.
 * The memory buffer allocated by this function must have sufficient
 * alignment to store any fundamental C data type at a natural alignment.
 * For most platforms this will require the memory buffer to have 8-byte
 * alignment, but refer to the Application Binary Interface (ABI) of the
 * target platform for details.
 *
 * @param context	A pointer to an initialized TEE Context
 * @param sharedMem	A pointer to a Shared Memory structure to allocate:
 *			- Before calling this function, the Client Application
 *			  MUST have set the \a size, and \a flags fields.
 *			- On return, for a successful allocation the
 *			  Implementation MUST have set the pointer buffer to
 *			  the address of the allocated block, otherwise it
 *			  MUST set buffer to NULL.
 *
 * @retval TEEC_SUCCESS	The allocation was successful.
 * @retval TEEC_ERROR_OUT_OF_MEMORY	the allocation could not be completed
 *			due to resource constraints
 * @retval TEEC_ERROR_*	An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_AllocateSharedMemory(
	TEEC_Context*		context,
	TEEC_SharedMemory*	sharedMem);


/** Release a shared memory block.
 *
 * This function deregisters or deallocates a previously initialized block of
 * Shared Memory.
 * For a memory buffer allocated using \a TEEC_AllocateSharedMemory the
 * Implementation MUST free the underlying memory and the Client Application
 * MUST NOT access this region after this function has been called.
 * In this case the Implementation MUST set the \a buffer and \a size fields
 * of the \a sharedMem structure to NULL and 0 respectively before returning.
 *
 * For memory registered using \a TEEC_RegisterSharedMemory
 * the Implementation MUST deregister the underlying memory from the TEE,
 * but the memory region will stay available to the Client Application for
 * other purposes as the memory is owned by it.
 *
 * The Implementation MUST do nothing if the \a sharedMem parameter is \a NULL.
 *
 * \b Programmer \b Error
 * The following usage of the API is a programmer error:
 * -  Attempting to release Shared Memory which is used by a
 *    pending operation.
 * -  Attempting to release the same Shared Memory structure concurrently
 *    from multiple threads. Multi-threaded Client Applications
 *    must use platform-provided locking mechanisms to ensure that
 *    this case does not occur.
 *
 * @param sharedMem:  A pointer to a valid Shared Memory structure
 *
 */
void TEEC_ReleaseSharedMemory(
	TEEC_SharedMemory*	sharedMem);


/** Opens a new session between client and trusted application
 *
 * This function opens a new Session between the Client Application and
 * the specified Trusted Application.
 *
 * The Implementation MUST assume that all fields of this \a session structure
 * are in an \a undefined state. When this function returns \a TEEC_SUCCESS
 * the Implementation MUST have populated this structure with any information
 * necessary for subsequent operations within the Session.
 *
 * The target Trusted Application is identified by a UUID passed in the
 * parameter destination.
 *
 * The Session MAY be opened using a specific connection method that can carry
 * additional connection data, such as data about the user or user-group running
 * the Client Application, or about the Client Application itself.
 * This allows the Trusted Application to implement access control methods
 * which separate functionality or data accesses for different actors
 * in the rich environment outside of the TEE. The additional data associated
 * with each connection method is passed in via the pointer \a connectionData.
 * For the core login types the following connection data is required:
 *
 * \a TEEC_LOGIN_PUBLIC - \a connectionData SHOULD be \a NULL.
 * \a TEEC_LOGIN_USER - \a connectionData SHOULD be \a NULL.
 * \a TEEC_LOGIN_GROUP - \a connectionData MUST point to a uint32_t
 * which contains the group which this Client Application wants to connect as.
 * The Implementation is responsible for securely ensuring that the
 * Client Application instance is actually a member of this group.
 * \a TEEC_LOGIN_APPLICATION - \a connectionData SHOULD be \a NULL.
 * \a TEEC_LOGIN_USER_APPLICATION - \a connectionData SHOULD be \a NULL.
 * \a TEEC_LOGIN_GROUP_APPLICATION - \a connectionData MUST point to a uint32_t
 * which contains the group which this Client Application wants to connect as.
 * The Implementation is responsible for securely ensuring that the
 * Client Application instance is actually a member of this group.
 *
 * An open-session operation MAY optionally carry an Operation Payload,
 * and MAY also be cancellable. When the payload is present the parameter
 * \a operation MUST point to a \a TEEC_Operation structure populated by the
 * Client Application. If \a operation is NULL then no data buffers are
 * exchanged with the Trusted Application, and the operation cannot be
 * cancelled by the Client Application.
 *
 * The result of this function is returned both in the function
 * \a TEEC_Result return code and the return origin, stored in the variable
 * pointed to by \a returnOrigin:
 * If the return origin is different from \a TEEC_ORIGIN_TRUSTED_APP,
 * then the return code MUST be  one of the defined error codes .
 * If the return code is \a TEEC_ERROR_CANCEL then it means that the
 * operation was cancelled before it reached the Trusted Application.
 * If the return origin is \a TEEC_ORIGIN_TRUSTED_APP, the meaning of the
 * return code depends on the protocol between the Client Application
 * and the Trusted Application. However, if \a TEEC_SUCCESS is returned,
 * it always means that the session was successfully opened and if the
 * function returns a code different from \a TEEC_SUCCESS,
 * it means that the session opening failed.
 *
 * \b Programmer \b Error
 * The following usage of the API is a programmer error:
 * - Calling with a \a context which is not yet initialized.
 * - Calling with a connectionData set to NULL if connection data is
 *   required by the specified connection method.
 * - Calling with an operation containing an invalid paramTypes field,
 *   i.e., containing a reserved parameter type or where a parameter type
 *   that conflicts with the parent Shared Memory.
 * - Encoding Registered Memory References which refer to
 *   Shared Memory blocks allocated within the scope of a different TEE Context.
 * - Attempting to open a Session using the same Session structure
 *   concurrently from multiple threads. Multi-threaded Client Applications
 *   must use platform-provided locking mechanisms, to ensure that this
 *   case does not occur.
 * - Using the same Operation structure for multiple concurrent operations.
 *
 * @param context	A pointer to an initialized TEE Context.
 * @param session	A pointer to a Session structure to open.
 * @param destination	A pointer to a structure containing the UUID of the
 *			destination Trusted Application
 * @param connectionMethod	The method of connection to use
 * @param connectionData	Any necessary data required to support the
 *				connection method chosen.
 * @param operation	A pointer to an Operation containing a set of Parameters
 *			to exchange with the Trusted Application, or \a NULL if
 *			no Parameters are to be exchanged or if the operation
 *			cannot be cancelled
 * @param returnOrigin	A pointer to a variable which will contain the return
 *			origin. This field may be \a NULL if the return origin
 *			is not needed.
 *
 * if (returnOrigin == TEEC_ORIGIN_TRUSTED_APP),
 * @retval TEEC_SUCCESS	The session was successfully opened.
 * @retval TEEC_ERROR_*	An error defined by the protocol between the Client
 *			Application and the Trusted Application.
 * else (returnOrigin != TEEC_ORIGIN_TRUSTED_APP)
 * @retval TEEC_ERROR_*	An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_OpenSession (
	TEEC_Context*		context,
	TEEC_Session*		session,
	const TEEC_UUID*	destination,
	uint32_t		connectionMethod,
	const void*		connectionData,
	TEEC_Operation*		operation,
	uint32_t*		returnOrigin);


/** Close a opened session between client and trusted application
 *
 *
 * This function closes a Session which has been opened with a
 * Trusted Application.
 *
 * All Commands within the Session MUST have completed before
 * calling this function.
 *
 * The Implementation MUST do nothing if the session parameter is NULL.
 *
 * The implementation of this function MUST NOT be able to fail:
 * after this function returns the Client Application must be able to
 * consider that the Session has been closed.
 *
 * \b Programmer \b Error
 * The following usage of the API is a programmer error:
 *  - Calling with a session which still has commands running.
 *  - Attempting to close the same Session concurrently from multiple
 * threads.
 *  - Attempting to close the same Session more than once.
 *
 * @param session	Session to close
 */
void TEEC_CloseSession (
	TEEC_Session*		session);


/** Invokes a command within the session
 *
 *
 * This function invokes a Command within the specified Session.
 *
 * The parameter \a session MUST point to a valid open Session.
 *
 * The parameter \a commandID is an identifier that is used to indicate
 * which of the exposed Trusted Application functions should be invoked.
 * The supported command identifiers are defined by the Trusted Application‟s
 * protocol.
 *
 * \b Operation \b Handling
 * A Command MAY optionally carry an Operation Payload.
 * When the payload is present the parameter \a operation MUST point to a
 * \a TEEC_Operation structure populated by the Client Application.
 * If \a operation is NULL then no parameters are exchanged with the
 * Trusted Application, and only the Command ID is exchanged.
 *
 * The \a operation structure is also used to manage cancellation of the
 * Command. If cancellation is required then \a the operation pointer MUST be
 * \a non-NULL and the Client Application MUST have zeroed the \a started
 * field of the \a operation structure before calling this function.
 * The \a operation structure MAY contain no Parameters if no data payload
 * is to be exchanged.
 *
 * The Operation Payload is handled as described by the following steps,
 * which are executed sequentially:
 * 1. Each Parameter in the Operation Payload is examined.
 * If the parameter is a Temporary Memory Reference, then it is registered
 * for the duration of the Operation in accordance with the fields set in
 * the \a TEEC_TempMemoryReference structure and the data flow direction
 * specified in the parameter type. Refer to the \a TEEC_RegisterSharedMemory
 * function for error conditions which can be triggered during
 * temporary registration of a memory region.
 * 2. The contents of all the Memory Regions which are exchanged
 * with the TEE are synchronized
 * 3. The fields of all Value Parameters tagged as input are read by the
 * Implementation. This applies to Parameters of type \a TEEC_VALUE_INPUT or
 * \a TEEC_VALUE_INOUT.
 * 4. The Operation is issued to the Trusted Application.
 * During the execution of the Command, the Trusted Application may read
 * the data held within the memory referred to by input Memory References.
 * It may also write data in to the memory referred to by
 * output Memory References, but these modifications are not guaranteed
 * to be observable by the Client Application until the command completes.
 * 5. After the Command has completed, the Implementation MUST update the
 * \a size field of the Memory Reference structures flagged as output:
 *
 * a. For Memory References that are non-null and marked as output,
 * the updated size field MAY be less than or equal to original size field.
 * In this case this indicates the number of bytes actually written by the
 * Trusted Application, and the Implementation MUST synchronize this region
 * with the Client Application memory space.
 * b. For all Memory References marked as output, the updated size
 * field MAY be larger than the original size field.
 * For null Memory References, a required buffer size MAY be specified by
 * the Trusted Application. In these cases the passed output buffer was
 * too small or absent, and the returned size indicates the size of the
 * output buffer which is necessary for the operation to succeed.
 * In these cases the Implementation SHOULD NOT synchronize any
 * shared data with the Client Application.\n
 *
 * 6. When the Command completes, the Implementation MUST update the fields
 * of all Value Parameters tagged as output,
 * i.e., of type \a TEEC_VALUE_OUTPUT or \a TEEC_VALUE_INOUT.
 * 7. All memory regions that were temporarily registered at the
 * beginning of the function are deregistered as if the function
 * \a TEEC_ReleaseSharedMemory was called on each of them.
 * 8. Control is passed back to the calling Client Application code.
 * \b Programmer \b Error \n.
 *
 * The result of this function is returned both in the function
 * \a TEEC_Result return code and the return origin, stored in the
 * variable pointed to by \a returnOrigin:
 *       If the return origin is different from \a TEEC_ORIGIN_TRUSTED_APP,
 * then the return code MUST be one of the error codes.
 * If the return code is TEEC_ERROR_CANCEL then it means that the operation
 * was cancelled before it reached the Trusted Application.
 * If the return origin is \a TEEC_ORIGIN_TRUSTED_APP, then the
 * meaning of the return code is determined by the protocol exposed by the
 * Trusted Application. It is recommended that the Trusted Application
 * developer chooses TEEC_SUCCESS (0) to indicate success in their protocol,
 * as this means that it is possible for the Client Application developer
 * to determine success or failure without looking at the return origin.
 *
 * \b Programmer \n Error
 * The following usage of the API is a programmer error:
 *       Calling with a \a session which is not an open session.
 *       Calling with invalid content in the \a paramTypes field of the
 * \a operation structure. This invalid behavior includes types which are
 * \a Reserved for future use or which conflict with the \a flags
 * of the parent Shared Memory block.
 *       Encoding Registered Memory References which refer to
 * Shared Memory blocks allocated or registered within the scope of a
 * different TEE Context.
 *       Using the same operation structure concurrently for
 * multiple operations, whether open Session operations or Command invocations.
 *
 * @param session	The open Session in which the command will be invoked.
 * @param commandID	The identifier of the Command within the Trusted
 *			Application to invoke. The meaning of each Command
 *			Identifier must be defined in the protocol exposed
 *			by the Trusted Application
 * @param operation	A pointer to a Client Application initialized
 *			\a TEEC_Operation structure, or NULL if there is no
 *			payload to send or if the Command does not need to
 *			support cancellation.
 * @param returnOrigin	A pointer to a variable which will contain the
 *			return origin. This field may be \a NULL if the return
 *			origin is not needed.
 *
 * @retval TEEC_SUCCESS	The command was successfully invoked.
 * @retval TEEC_ERROR_*	An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_InvokeCommand(
	TEEC_Session*		session,
	uint32_t		commandID,
	TEEC_Operation*		operation,
	uint32_t*		returnOrigin);


/** Request cancellation of pending open session or command invocation.
 *
 * This function requests the cancellation of a pending open Session operation
 * or a Command invocation operation. As this is a synchronous API,
 * this function must be called from a thread other than the one executing the
 * \a TEEC_OpenSession or \a TEEC_InvokeCommand function.
 *
 * This function just sends a cancellation signal to the TEE and returns
 * immediately; the operation is not guaranteed to have been cancelled
 * when this function returns. In addition, the cancellation request is just
 * a hint; the TEE or the Trusted Application MAY ignore the
 * cancellation request.
 *
 * It is valid to call this function using a \a TEEC_Operation structure
 * any time after the Client Application has set the \a started field of an
 * Operation structure to zero. In particular, an operation can be
 * cancelled before it is actually invoked, during invocation, and
 * after invocation. Note that the Client Application MUST reset
 * the started field to zero each time an Operation structure is used
 * or re-used to open a Session or invoke a Command if the new operation
 * is to be cancellable.
 *
 * Client Applications MUST NOT reuse the Operation structure for another
 * Operation until the cancelled command has actually returned in the thread
 * executing the \a TEEC_OpenSession or \a TEEC_InvokeCommand function.
 *
 * \b Detecting \b cancellation
 * In many use cases it will be necessary for the Client Application
 * to detect whether the operation was actually cancelled, or whether it
 * completed normally.
 * In some implementations it MAY be possible for part of the infrastructure
 * to cancel the operation before it reaches the Trusted Application.
 * In these cases the return origin returned by \a TEEC_OpenSession or
 * \a TEEC_InvokeCommand MUST be either or \a TEEC_ORIGIN_API,
 * \a TEEC_ORIGIN_COMMS, \a TEEC_ORIGIN_TEE, and the return code MUST be
 * \a TEEC_ERROR_CANCEL.
 * If the cancellation request is handled by the Trusted Application itself
 * then the return origin returned by \a TEEC_OpenSession or
 * \a TEEC_InvokeCommand MUST be \a TEE_ORIGIN_TRUSTED_APP,
 * and the return code is defined by the Trusted Application‟s protocol.
 * If possible, Trusted Applications SHOULD use \a TEEC_ERROR_CANCEL
 * for their return code, but it is accepted that this is not always
 * possible due to conflicts with existing return code definitions in
 * other standards.
 *
 * @param operation	A pointer to a Client Application instantiated
 *			Operation structure.
 */
void TEEC_RequestCancellation(
	TEEC_Operation*		operation);

/** Returns error string.
 *
 * This function returns the error string value based on error number and
 * return origin.
 *
 * @param error		Error number.
 * @param returnOrigin	Origin of the return.
 *
 * @return const char*	Error string info
 */
const char* TEEC_GetError(
	uint32_t		error,
	uint32_t		returnOrigin);

/** Resiter a TA.
 *
 * The TA binary is in a memref or tmpref in param.
 */
TEEC_Result TEEC_RegisterTA(
		TEEC_Context*		context,
		TEEC_Parameter*		taBin,
		uint32_t		paramType);

/** Resiter a TA with the perperty in property.
 *
 * The TA binary is in a memref or tmpref in param.
 */
TEEC_Result TEEC_RegisterTAExt(
		TEEC_Context*		context,
		TEEC_Parameter*		taBin,
		uint32_t		paramType,
		TEEC_Property*	property);

/** Load a file into the given buffer of size bytes
 * @param filename	The file path
 * @param buf		The buffer to store the data
 * @param size		The size of the data to read
 *
 * @retval TEEC_SUCCESS	The command was successfully invoked.
 * @retval TEEC_ERROR_*	An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_LoadFile(
		const char*		filename,
		void*			buf,
		int			size);

/** Load a TA.
 *
 * It loads a TA with specified filename.
 *
 * @param context	TEE context.
 * @param filename	the path of TA to load.
 *
 * @sa TEEC_RegisterTA()
 */
TEEC_Result TEEC_LoadTA(
		TEEC_Context*		context,
		const char*		filename);

/** Load a TA.
 *
 * It loads a TA with specified filename.
 *
 * @param context	TEE context.
 * @param filename	the path of TA to load.
 * @param property	the property of TA to load
 *
 * @sa TEEC_RegisterTAExt()
 */
TEEC_Result TEEC_LoadTAExt(
		TEEC_Context*		context,
		const char*		filename,
		TEEC_Property*		property);


/** Register Callback Routine.
 *
 * @param session	session handle.
 * @param callback	callback routine.
 * @param userdata	userdata for callback routine.
 *
 * @retval TEEC_SUCCESS	The command was successfully invoked.
 * @retval TEEC_ERROR_*	An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_RegisterCallback(
		TEEC_Session*	session,
		TEEC_Callback	callback,
		void *		userdata);

/** Unregister Callback Routine.
 *
 * @param session	session handle.
 *
 * @retval TEEC_SUCCESS	The command was successfully invoked.
 * @retval TEEC_ERROR_*	An implementation-defined error code for any other error.
 */
TEEC_Result TEEC_UnregisterCallback(
		TEEC_Session*	session);

/** Clean secure cache.
 *
 * @note it can only handle the physical continous secure memory.
 *
 * @param context	TEE context.
 * @param phyAddr	Start physical address.
 * @param len		Memory size in bytes.
 *
 * @retval TEEC_SUCCESS			Succeed.
 * @retval TEEC_ERROR_BAD_PARAMETERS	Bad parameters.
 * @retval TEEC_ERROR_ACCESS_DENIED	Operation is not allowed.
 */
TEEC_Result TEEC_SecureCacheClean(
		TEEC_Context*		context,
		void*			phyAddr,
		size_t			len);

/** Invalidate secure cache.
 *
 * @note it can only handle the physical continous secure memory.
 *
 * @param context	TEE context.
 * @param phyAddr	Start physical address.
 * @param len		Memory size in bytes.
 *
 * @retval TEEC_SUCCESS			Succeed.
 * @retval TEEC_ERROR_BAD_PARAMETERS	Bad parameters.
 * @retval TEEC_ERROR_ACCESS_DENIED	Operation is not allowed.
 */
TEEC_Result TEEC_SecureCacheInvalidate(
		TEEC_Context*		context,
		void*			phyAddr,
		size_t			len);

/** Clean and invalidate secure cache.
 *
 * @note it can only handle the physical continous secure memory.
 *
 * @param context	TEE context.
 * @param phyAddr	Start physical address.
 * @param len		Memory size in bytes.
 *
 * @retval TEEC_SUCCESS			Succeed.
 * @retval TEEC_ERROR_BAD_PARAMETERS	Bad parameters.
 * @retval TEEC_ERROR_ACCESS_DENIED	Operation is not allowed.
 */
TEEC_Result TEEC_SecureCacheFlush(
		TEEC_Context*		context,
		void*			phyAddr,
		size_t			len);

/** Fast memory move with physical address.
 *
 * @note it can only handle the physical continous memory.
 *
 * @param context	TEE context.
 * @param dstPhyAddr	Destination memory address, can be secure or non-secure.
 * @param srcPhyAddr	Source memory address, either secure or non-secire.
 * @param len		Bytes to move.
 *
 * @retval TEEC_SUCCESS			Succeed to move the data.
 * @retval TEEC_ERROR_BAD_PARAMETERS	Any parameter is invalid.
 * @retval TEEC_ERROR_OUT_OF_MEMORY	Destination buffer size is too small.
 * @retval TEEC_ERROR_ACCESS_CONFLICT	Operation is not allowed.
 *
 * @sa TEEC_FastSharedMemMove()
 */
TEEC_Result TEEC_FastMemMove(
		TEEC_Context*		context,
		void*			dstPhyAddr,
		void*			srcPhyAddr,
		size_t			len);

/** Fast move in shared memory.
 *
 * @param context	TEE context.
 * @param dst		Destination shared memory, can be secure or non-secure.
 * @param src		Source shared memory, either secure or non-secire.
 * @param dstOffset	Offset to store the data in dst shared memory.
 * @param srcOffset	Offset of the data to move in src shared memory.
 * @param len		Bytes to move.
 *
 * @retval TEEC_SUCCESS			Succeed to move the data.
 * @retval TEEC_ERROR_BAD_PARAMETERS	Any parameter is invalid.
 * @retval TEEC_ERROR_OUT_OF_MEMORY	Destination buffer size is too small.
 * @retval TEEC_ERROR_ACCESS_CONFLICT	Operation is not allowed.
 *
 * @sa TEEC_FastMemMove()
 */
TEEC_Result TEEC_FastSharedMemMove(
		TEEC_Context*		context,
		TEEC_SharedMemory*	dst,
		TEEC_SharedMemory*	src,
		size_t			dstOffset,
		size_t			srcOffset,
		size_t			len);

/** Add TA type to property
 * @param property	TEEC property.
 * @param type		TA type, currently can be "MVLTA" or "CUSTA".
 *
 * @retval TEEC_SUCCESS			Succeed to add type to property.
 * @retval TEEC_ERROR_BAD_PARAMETERS	Any parameter is invalid.
 */
TEEC_Result TEEC_AddPropertyTAType(
		TEEC_Property*		property,
		const char*		type);

/** Get TA type from property
 * @param property	TEEC property.
 * @retval type		TA type, 4 means "MVLTA", 6 means "CUSTA".
 *
 * @retval TEEC_SUCCESS			Succeed to get TA type in property
 * @retval TEEC_ERROR_BAD_PARAMETERS	Any parameter is invalid.
 */
TEEC_Result TEEC_GetPropertyTAType(
		TEEC_Property*		property,
		uint32_t*		type);

/** Load feature control certificate
 * @param context	TEE context.
 *
 * @retval TEEC_SUCCESS			Succeed to load feature control certificate
 * @retval TEEC_ERROR_OUT_OF_MEMORY	Fail to alloc communication channel.
 */
TEEC_Result TEEC_LoadFeatureCert(TEEC_Context *context);

#endif /* _TEE_CLIENT_API_H_ */
