#ifndef __UAPI_SYNA_BM__
#define __UAPI_SYNA_BM__

#include <linux/ioctl.h>
#include <linux/types.h>

struct bm_pt_param {
	__u64 phy_addr;
	__u64 len;
	__u64 mem_id;
};

struct bm_uv_align_param {
	__u32 y_size;
};

/* extra parameter for frame buffer */
struct bm_fb_param {
	__u32 fb_type;
	struct bm_uv_align_param uva_param;
};

struct bm_fb_data {
	__u32 fd;
	__u32 flags;
	struct bm_fb_param fb_param;
	struct bm_pt_param pt_param;
};

struct bm_pt_data {
	__u32 fd;
	struct bm_pt_param pt_param;
};

#define BM_IOC_ALLOC_PT		_IOWR('B', 1, struct bm_fb_data)
#define BM_IOC_GET_PT		_IOWR('B', 2, struct bm_pt_data)
#define BM_IOC_PUT_PT		_IOW('B', 3, __u32)

#endif
