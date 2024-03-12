#include "synap_mem.h"
#include <linux/ion.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include "synap_kernel_log.h"
#include <linux/fs.h>
#include "synap_kernel_ca.h"


static u32 ion_ns_cont_heap_id_mask;
static u32 ion_s_cont_heap_id_mask;
static u32 ion_scatter_heap_id_mask;

bool synap_mem_init(void)
{

    int heap_num, i;
    struct ion_heap_data *hdata;

    hdata = kmalloc(sizeof(*hdata) * ION_NUM_MAX_HEAPS, GFP_KERNEL);
    if (!hdata) {
        KLOGE("unable to look up heaps");
        return false;
    }

    heap_num = ion_query_heaps_kernel(hdata, ION_NUM_MAX_HEAPS);

    ion_ns_cont_heap_id_mask = 0;
    ion_s_cont_heap_id_mask = 0;
    ion_scatter_heap_id_mask = 0;

    for (i = 0; i < heap_num; i++) {
        if (hdata[i]. type == ION_HEAP_TYPE_DMA_CUST) {
            ion_ns_cont_heap_id_mask |= 1 << hdata[i].heap_id;
        }
        if (hdata[i]. type == ION_HEAP_TYPE_BERLIN_SECURE) {
            ion_s_cont_heap_id_mask |= 1 << hdata[i].heap_id;
        }
        if (hdata[i].type == ION_HEAP_TYPE_SYSTEM_CUST) {
            ion_scatter_heap_id_mask |= 1 << hdata[i].heap_id;
        }
    }

    if (ion_ns_cont_heap_id_mask == 0) {
        KLOGE("unable to find ION_HEAP_TYPE_DMA_CUST heap");
        return false;
    }

    if (ion_s_cont_heap_id_mask == 0) {
        KLOGE("unable to find ION_HEAP_TYPE_BERLIN_SECURE heap");
        return false;
    }

    if (ion_ns_cont_heap_id_mask == 0) {
        KLOGE("unable to find ION_HEAP_TYPE_DMA_CUST heap");
        return false;
    }

    kfree(hdata);

    KLOGI("heap mask scatter=0x%x s_cont=0x%x ns_cont=0x%x",
          ion_scatter_heap_id_mask, ion_s_cont_heap_id_mask, ion_ns_cont_heap_id_mask);

    return true;
}


void synap_mem_free(struct synap_mem* mem)
{

    KLOGI("freeing ref=%d", mem->dmabuf->file->f_count);

    if (!IS_ERR_OR_NULL(mem->sg_table)) {
        dma_buf_unmap_attachment(mem->dmabuf_attach, mem->sg_table, DMA_BIDIRECTIONAL);
    }

    if (!IS_ERR_OR_NULL(mem->dmabuf_attach)) {
        dma_buf_detach(mem->dmabuf, mem->dmabuf_attach);
    }

    if (!IS_ERR_OR_NULL(mem->dmabuf)) {
        dma_buf_put(mem->dmabuf);
    }

    kfree(mem);

}

static struct synap_mem *synap_mem_wrap_dmabuf(struct synap_device* dev,
                                           struct dma_buf *dmabuf, size_t size, u64 offset) {

    struct synap_mem *mem = NULL;
    u32 dma_buf_size = 0;
    struct scatterlist *sg = NULL;
    size_t i;
    struct device *r_dev = &dev->pdev->dev;

    // we use the MMU of the NPU to map the buffer with the offset so we can only map at
    // page boundries
    if (offset % SYNAP_MEM_NPU_PAGE_SIZE != 0) {
        KLOGE("offset must be multiple of npu page size");
        return NULL;
    }

    if ((mem = kzalloc(sizeof(struct synap_mem), GFP_KERNEL)) == NULL) {
        KLOGE("kzalloc synap_mem failed");
        return NULL;
    }

    KLOGI("wrapping buffer %p", dmabuf);

    mem->size = size;
    mem->offset = offset;
    mem->dmabuf = dmabuf;

    mem->dmabuf_attach = dma_buf_attach(mem->dmabuf, r_dev);

    if (IS_ERR(mem->dmabuf_attach)) {
        KLOGE("attach dmabuf failed when register memory");
        synap_mem_free(mem);
        return NULL;
    }

    // FIXME: we could be more efficient if we knew the direction
    mem->sg_table = dma_buf_map_attachment(mem->dmabuf_attach, DMA_BIDIRECTIONAL);
    if (IS_ERR(mem->sg_table)) {
        KLOGE("get areas from sgtable failed");
        synap_mem_free(mem);
        return NULL;
    }

    for_each_sg(mem->sg_table->sgl, sg, mem->sg_table->nents, i) {
        dma_buf_size += sg->length;
    }

    // the backing dma buffer must have enough space to fit the whole buffer
    // including the padding till the end of the page after the end of the
    // buffer otherwise we may map unexpected things in the NPU page table
    if (SYNAP_MEM_ALIGN_SIZE(size) + offset > dma_buf_size) {
        KLOGE("specified range doesn't fit within the dmabuf");
        synap_mem_free(mem);
        return NULL;
    }

    if (size == 0) {
        KLOGE("zero size buffer not supported");
        synap_mem_free(mem);
        return NULL;
    }

    return mem;
}

#if (DEBUG_LEVEL >= 2)
static const char* synap_mem_type_to_str(synap_mem_type type)
{
#define TO_STR(x) case x : return #x;
    switch (type) {
        TO_STR(SYNAP_MEM_POOL);
        TO_STR(SYNAP_MEM_CODE);
        TO_STR(SYNAP_MEM_PAGE_TABLE);
        TO_STR(SYNAP_MEM_IO_BUFFER);
        TO_STR(SYNAP_MEM_DRIVER_BUFFER);
        default: return "UNKNOWN TYPE";
    }
}
#endif

struct synap_mem *synap_mem_alloc(struct synap_device* dev, synap_mem_type mem_type,
                                  bool secure, size_t size)
{
    struct synap_mem *mem = NULL;
    struct dma_buf *dmabuf;

    u32 heap_mask = ion_scatter_heap_id_mask;

    /* page table for the moment requires contiguous,
       driver buffers will always require contiguous memory*/
    if (mem_type == SYNAP_MEM_PAGE_TABLE || mem_type == SYNAP_MEM_DRIVER_BUFFER) {
        if (secure) {
            heap_mask = ion_s_cont_heap_id_mask;
        } else {
            heap_mask = ion_ns_cont_heap_id_mask;
        }
    }

    dmabuf = ion_alloc(size, heap_mask, ION_CACHED);

    if (IS_ERR_OR_NULL(dmabuf)) {
        KLOGE("ion alloc failed, size=%d", size);
        return NULL;
    }

    mem = synap_mem_wrap_dmabuf(dev, dmabuf, size, 0);

    if (!mem) {
        dma_buf_put(dmabuf);
        return NULL;
    }

    KLOGI("allocate mem type=%s size=%d %ssecure",
          synap_mem_type_to_str(mem_type), size, secure ? "" : "non");

    return mem;
}

struct synap_mem *synap_mem_wrap_fd(struct synap_device* dev, int fd, size_t size, u64 offset) {
    struct synap_mem *mem = NULL;
    struct dma_buf *dmabuf;

    dmabuf = dma_buf_get(fd);

    if (IS_ERR_OR_NULL(dmabuf)) {
        KLOGE("fd get failed for dmabuf, fd=%d size=%d", fd, size);
        return NULL;
    }

    mem = synap_mem_wrap_dmabuf(dev, dmabuf, size, offset);

    if (!mem) {
        KLOGE("failed to wrap dmabuf")
        dma_buf_put(dmabuf);
        return NULL;
    }

    return mem;

}

