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

#ifndef _REE_SYS_CALLBACK_CMD_H_
#define _REE_SYS_CALLBACK_CMD_H_

#include "types.h"
#include "tee_comm.h"
#include "tee_internal_api.h"

enum REE_SysCallbackCmd {
	REE_CMD_TIME_GET,
	REE_CMD_TIME_WAIT,
	REE_CMD_THREAD_CREATE,
	REE_CMD_THREAD_DESTROY,
	REE_CMD_LOG_WRITE,
	REE_CMD_DEPRECATED1,
	REE_CMD_DEPRECATED2,
	REE_CMD_DEPRECATED3,
	REE_CMD_DEPRECATED4,
	REE_CMD_DEPRECATED5,
	REE_CMD_DEPRECATED6,
	REE_CMD_DEPRECATED7,
	REE_CMD_DEPRECATED8,
	REE_CMD_DEPRECATED9,
	REE_CMD_DEPRECATED10,
	REE_CMD_DEPRECATED11,
	REE_CMD_FILE_OPEN,
	REE_CMD_FILE_CLOSE,
	REE_CMD_FILE_READ,
	REE_CMD_FILE_WRITE,
	REE_CMD_FILE_SEEK,
	REE_CMD_FOLDER_MAKE,
	REE_CMD_RPMB_READ_COUNTER,
	REE_CMD_RPMB_WRITE_BLOCK,
	REE_CMD_RPMB_READ_BLOCK,
	REE_CMD_MUTEX_EXT_WAIT,
	REE_CMD_MUTEX_EXT_WAKE,
	REE_CMD_MUTEX_EXT_DEL,
	REE_CMD_MAX
};

#ifndef REE_MUTEX_NAME_MAX_LEN
#define REE_MUTEX_NAME_MAX_LEN		32
#endif

#ifndef REE_SEMAPHORE_NAME_MAX_LEN
#define REE_SEMAPHORE_NAME_MAX_LEN	32
#endif

/*
 * file path looks like this /data/tee/uuid2md5/obj_id(<64bytes)
 * 192 is enough
 */
#ifndef REE_FILE_PATH_MAX_LEN
#define REE_FILE_PATH_MAX_LEN		(192 - 1)
#endif

#ifndef REE_FOLDER_PATH_MAX_LEN
#define REE_FOLDER_PATH_MAX_LEN		(192 - 1)
#endif

typedef struct REE_MutexCreateParam
{
	void		*lock;
	char		name[REE_MUTEX_NAME_MAX_LEN + 1];
} REE_MutexCreateParam;

typedef struct REE_SemaphoreCreateParam
{
	void		*sem;
	int		value;
	char		name[REE_SEMAPHORE_NAME_MAX_LEN + 1];
} REE_SemaphoreCreateParam;

typedef struct REE_SemaphoreTimedWaitParam
{
	void		*sem;
	uint32_t	timeout;
} REE_SemaphoreTimedWaitParam;

typedef struct REE_FileOpenParam
{
	char		fileName[REE_FILE_PATH_MAX_LEN + 1];
} REE_FileOpenParam;

typedef struct REE_FileReadWriteParam
{
	uint64_t	filp;
	int32_t		offset;
	int32_t		ret;
	int32_t		size;
	char		buff[0];
} REE_FileReadWriteParam;

typedef struct REE_FileSeekParam
{
	uint64_t	filp;
	int32_t		offset;
	uint32_t	whence;
	uint32_t	pos;
} REE_FileSeekParam;

typedef struct REE_FolderMakeParam
{
	char		folderPath[REE_FOLDER_PATH_MAX_LEN + 1];
} REE_FolderMakeParam;


#endif /* _REE_SYS_CALLBACK_CMD_H_ */
