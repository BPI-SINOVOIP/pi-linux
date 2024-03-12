#pragma once

#include "synap_kernel.h"
#include <linux/types.h>
#include <linux/device.h>
#include "ca/synap_ta_cmd.h"
#include <linux/scatterlist.h>


struct synap_mem {
    struct synap_device *owner;
    struct dma_buf *dmabuf;
    struct sg_table *sg_table;
    struct dma_buf_attachment *dmabuf_attach;
    size_t size;
    u64 offset;
};

typedef enum {
    SYNAP_MEM_POOL = 0,
    SYNAP_MEM_CODE = 1,
    SYNAP_MEM_PAGE_TABLE = 2,
    SYNAP_MEM_DRIVER_BUFFER = 3,
    SYNAP_MEM_IO_BUFFER = 4,
    SYNAP_MEM_PROFILE_OP_BUFFER = 5
} synap_mem_type;

#define SYNAP_MEM_NPU_PAGE_SIZE 4096
#define SYNAP_MEM_ALIGN_SIZE(x) (((x) + 4096 - ((x) % SYNAP_MEM_NPU_PAGE_SIZE == 0 ? 4096 : (x) % SYNAP_MEM_NPU_PAGE_SIZE)))

bool synap_mem_init(void);

void synap_mem_free(struct synap_mem* mem);

struct synap_mem *synap_mem_alloc(struct synap_device* dev, synap_mem_type mem_type,
                                  bool secure, size_t size);

struct synap_mem *synap_mem_wrap_fd(struct synap_device* dev, int fd, size_t size, u64 offset);
