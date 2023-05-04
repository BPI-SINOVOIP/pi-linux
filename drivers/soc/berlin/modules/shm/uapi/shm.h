#ifndef __UAPI_SYNA_SHM__
#define __UAPI_SYNA_SHM__

#include <linux/ioctl.h>
#include <linux/types.h>

enum shm_allocation_flag {
	/* the buffer allocated is a cached buffer */
	SHM_FLAG_CACHED       = 0x1,
	/* the buffer allocated is a non-cached buffer */
	SHM_FLAG_NONCACHED    = 0x2,
};

struct shm_allocation_data {
	__u64 len;
	__u32 fd;
	__u32 gid;
	__u32 flags;
	__u32 ion_heap_id_mask;
};

struct shm_fd_data {
	__u32 fd;
	__u32 gid;
};

struct shm_misc_data {
	__u32 gid;
	__u64 phy_addr;
	__u64 len;
};

enum shm_sync_op {
	SHM_INVALIDATE_CACHE,
	SHM_CLEAN_CACHE,
	SHM_FLUSH_CACHE
};

struct shm_sync_data {
	__u32 gid;
	__u32 op;
	__u64 addr;
	__u64 offset;
	__u64 len;
};

#define SHM_IOC_ALLOC           _IOWR('s', 1, struct shm_allocation_data)
#define SHM_IOC_GET_PA          _IOWR('s', 2, struct shm_misc_data)
#define SHM_IOC_GET_FD          _IOWR('s', 3, struct shm_fd_data)
#define SHM_IOC_PUT             _IOW('s', 4, struct shm_fd_data)
#define SHM_IOC_ATTACH          _IOWR('s', 5, struct shm_fd_data)
#define SHM_IOC_DETACH          _IOW('s', 6, __u32)
#define SHM_IOC_SYNC            _IOW('s', 7, struct shm_sync_data)

#endif
