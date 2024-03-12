#include "synap_profile.h"
#include "synap_kernel_log.h"
#include "synap_kernel_ca.h"
#include "synap_mem.h"
#include <linux/slab.h>


bool synap_profile_parse_layers(struct ebg_file_public_data *pd,
                                struct synap_network_resources_desc *desc)
{
    u32 i;
    struct ebg_file_layer *layer_data;
    struct synap_profile_layer *layer_profile;
    u32 layer_data_size;

    struct ebg_file_vsi_data vsi_data;
    ebg_file_vsi_data_parse(pd, &vsi_data);

    layer_data_size = sizeof(struct ebg_file_layer) * vsi_data.layer_count;

    layer_data = kzalloc(layer_data_size, GFP_KERNEL);
    if (!layer_data) {
        KLOGE("failed to alloc layers buf");
        return false;
    }

    if (!ebg_file_get_layers(pd, layer_data, vsi_data.layer_count)) {
        KLOGE("failed to get layer data");
        kfree(layer_data);
        return false;
    }

    layer_profile = kzalloc(sizeof(struct synap_profile_layer) * vsi_data.layer_count, GFP_KERNEL);
    if (!layer_profile) {
        KLOGE("failed to alloc layers buf");
        kfree(layer_data);
        return false;
    }
    for (i = 0; i < vsi_data.layer_count; i++) {
        layer_profile[i].uid = layer_data[i].uid;
        strncpy(layer_profile[i].name, layer_data[i].layer_name, strlen(layer_data[i].layer_name));
    }

    desc->layer_profile_buf = layer_profile;
    desc->operation_count = vsi_data.operation_count;
    desc->layer_count = vsi_data.layer_count;
    kfree(layer_data);

    return true;
}

bool synap_profile_get_layers_data(struct synap_network *network)
{
    u32 i, j;
    void *op_va;
    struct synap_profile_layer *layers = network->layer_profile;
    struct dma_buf *dmabuf = network->profile_operation->dmabuf;

    if (!layers || !dmabuf) return false;

    if (dma_buf_begin_cpu_access(dmabuf, DMA_BIDIRECTIONAL) != 0) {
        return false;
    }

    op_va = dma_buf_vmap(dmabuf);
    if (op_va == NULL) {
        KLOGE("op buffer kmap failed.");
        dma_buf_end_cpu_access(dmabuf, DMA_BIDIRECTIONAL);
        return false;
    }

    for (j = 0; j < network->layer_count; j++) {
        layers[j].cycle = 0;
        layers[j].execution_time = 0;
        layers[j].byte_read = 0;
        layers[j].byte_write = 0;
    }

    for (i = 0; i < network->operation_count; i++) {
        struct synap_profile_operation *p = &((struct synap_profile_operation *)op_va)[i];
        if (p->layer_idx < network->layer_count) {
            layers[p->layer_idx].type = p->type;
            layers[p->layer_idx].cycle += p->cycle;
            layers[p->layer_idx].execution_time += p->execution_time;
            layers[p->layer_idx].byte_read += p->total_read_bw;
            layers[p->layer_idx].byte_write += p->total_write_bw;
        }
        else {
            KLOGE("Invalid layer index for op %u: %u", i, p->layer_idx);
        }
    }

    for (j = 0; j < network->layer_count; j++) {
        KLOGD("layer[%d]/[%d] type=%s name=%s cycle=%d execution_time=%d byte_read=%d byte_write=%d",
              j, network->layer_count,
              synap_operation_type2str(layers[j].type), layers[j].name, layers[j].cycle,
              layers[j].execution_time, layers[j].byte_read, layers[j].byte_write);
    }

    dma_buf_vunmap(dmabuf, op_va);
    dma_buf_end_cpu_access(dmabuf, DMA_BIDIRECTIONAL);

    return true;
}

const char *synap_operation_type2str(u32 operation_type)
{
    switch (operation_type) {
    case SYNAP_OPERATION_TYPE_SH:
        return "SH";
    case SYNAP_OPERATION_TYPE_NN:
        return "NN";
    case SYNAP_OPERATION_TYPE_TP:
        return "TP";
    case SYNAP_OPERATION_TYPE_SW:
        return "SW";
    case SYNAP_OPERATION_TYPE_SC:
        return "SC";
    case SYNAP_OPERATION_TYPE_NONE:
        return "--";
    case SYNAP_OPERATION_TYPE_INIT:
        return "INI";
    case SYNAP_OPERATION_TYPE_END:
        return "END";
    default:
        return "N/A";
    }
}

#if (DEBUG_LEVEL == 3)
void synap_profile_operation_dump(struct synap_network *network)
{
    u32 i;
    void *op_va;
    struct dma_buf *dmabuf = network->profile_operation->dmabuf;

    if (!dmabuf) return;

    if (dma_buf_begin_cpu_access(dmabuf, DMA_BIDIRECTIONAL) != 0) {
        return;
    }

    op_va = dma_buf_vmap(dmabuf);
    if (op_va == NULL) {
        KLOGE("op buffer kmap failed.");
        dma_buf_end_cpu_access(dmabuf, DMA_BIDIRECTIONAL);
        return;
    }

    KLOGD("|------------------------------------------------------------|");
    KLOGD("| op_type | op_layer_idx | op_cycle | byte_read | byte_write |");
    for (i = 0; i < network->operation_count; i++) {

        struct synap_profile_operation *p =
            &((struct synap_profile_operation *)op_va)[i];

        KLOGD("|%9s|%14d|%10d|%11d|%12d|",
              synap_operation_type2str(p->type), p->layer_idx,
              p->cycle, p->total_read_bw, p->total_write_bw);
    }
    KLOGD("|------------------------------------------------------------|");

    dma_buf_vunmap(dmabuf, op_va);
    dma_buf_end_cpu_access(dmabuf, DMA_BIDIRECTIONAL);
}

#endif


