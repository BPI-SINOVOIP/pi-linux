// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#include "synap_kernel.h"
#include "synap_mem.h"
#include "synap_kernel_log.h"
#include "synap_kernel_ca.h"
#include "ebg_file.h"
#include "synap_profile.h"

#include "uapi/synap.h"

#include <asm/atomic.h>

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqflags.h>
#include <linux/miscdevice.h>
#include <linux/pagemap.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/moduleparam.h>

#include <asm/uaccess.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/errno.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/kernel_read_file.h>
#endif

#include <linux/mod_devicetable.h>
#include <linux/ion.h>

#define SYNAPTA_NAME "libsynap.ta"
static char *ta_path;
module_param(ta_path, charp, 0000);
MODULE_PARM_DESC(ta_path, "ta path");

//ta_path string len refer to string param max len
//param_set_charp() in kernel/params.c
#define TA_PATH_LEN_MAX 1024

#define SYNAP_HARDWARE_IRQ_TIMEOUT 40000

union synap_ioctl_arg {
    struct synap_set_network_io_data set_data;
    struct synap_attach_io_buffer_data attach_io_buffer_data;
    struct synap_create_secure_io_buffer_from_dmabuf_data create_secure_io_buffer_from_dmabuf_data;
    struct synap_create_io_buffer_from_dmabuf_data create_io_buffer_from_dmabuf_data;
    struct synap_create_io_buffer_from_mem_id_data create_io_buffer_from_mem_id_data;
    struct synap_create_io_buffer_data create_io_buffer_data;
    struct synap_create_network_data create_network_data;
    struct synap_attachment_data attachment_data;
    u32 locked;
    u32 nid;
    u32 bid;
};

static const struct of_device_id synap_dev_match[] = {
    {
        .compatible = "syna,synap"
    },
    { },
};

static u64 synap_get_microsec(void)
{
    struct timespec64 tv;
    ktime_get_real_ts64(&tv);
    return (u64)((tv.tv_sec * 1000000ULL) + (tv.tv_nsec / 1000));
}

#if (DEBUG_LEVEL >= 2)
#define CREATE_CASE(x) case x : return #x;

static const char* cmd_to_str(u32 cmd)
{
    switch (cmd) {
        CREATE_CASE(SYNAP_SET_NETWORK_INPUT);
        CREATE_CASE(SYNAP_SET_NETWORK_OUTPUT);
        CREATE_CASE(SYNAP_CREATE_IO_BUFFER_FROM_DMABUF);
        CREATE_CASE(SYNAP_CREATE_IO_BUFFER);
        CREATE_CASE(SYNAP_CREATE_SECURE_IO_BUFFER_FROM_DMABUF);
        CREATE_CASE(SYNAP_CREATE_IO_BUFFER_FROM_MEM_ID);
        CREATE_CASE(SYNAP_DESTROY_IO_BUFFER);
        CREATE_CASE(SYNAP_ATTACH_IO_BUFFER);
        CREATE_CASE(SYNAP_DETACH_IO_BUFFER);
        CREATE_CASE(SYNAP_CREATE_NETWORK);
        CREATE_CASE(SYNAP_RUN_NETWORK);
        CREATE_CASE(SYNAP_DESTROY_NETWORK);
        default: return "UNKNOWN CMD";
    }
}
#endif

static bool synap_free_attachment(struct synap_attachment* attachment)
{

    if (!attachment) return false;

    LOG_ENTER();

    KLOGI("freeing attachment id=%d", attachment->aidr);

    if (attachment->aidr >= 0) {
        idr_remove(&attachment->network->attachments, attachment->aidr);
    }

    kfree(attachment);

    return true;
}

static struct synap_attachment* synap_alloc_attachment(struct synap_network *net)
{
    struct synap_attachment *attachment = NULL;

    LOG_ENTER();

    if (!net) return NULL;

    attachment = kzalloc(sizeof(struct synap_attachment), GFP_KERNEL);

    if (!attachment) {
        KLOGE("alloc attachment failed");
        return NULL;
    }

    attachment->network = net;

    attachment->aidr = idr_alloc(&net->attachments, attachment, 1, 1 << 31, GFP_KERNEL);

    if (attachment->aidr < 0) {
        KLOGE("alloc attachment idr failed");
        synap_free_attachment(attachment);
        return NULL;
    }

    KLOGI("alloc attachment id=%d", attachment->aidr);

    return attachment;
}

static bool synap_free_network(struct synap_network* net)
{

    LOG_ENTER();

    if (!net) return false;

    KLOGI("free network id=%d", net->nidr);

    idr_destroy(&net->attachments);

    if (net->page_table) synap_mem_free(net->page_table);
    if (net->code) synap_mem_free(net->code);
    if (net->pool) synap_mem_free(net->pool);
    if (net->profile_operation) synap_mem_free(net->profile_operation);
    if (net->layer_profile) kfree(net->layer_profile);

    if (net->nidr >= 0) {
        idr_remove(&net->inst->networks, net->nidr);
    }

    net->inst->network_count--;

    kfree(net);

    return true;
}

static struct synap_network* synap_alloc_network(struct synap_file *inst,
                                                 struct synap_network_resources_desc *desc)
{
    struct synap_network *net;
    struct synap_device *synap_device;

    LOG_ENTER();

    if (!inst || !desc) return NULL;

    synap_device = inst->synap_device;

    net = kzalloc(sizeof(struct synap_network), GFP_KERNEL);

    if (!net) {
        KLOGE("alloc network failed");
        return NULL;
    }

    idr_init(&net->attachments);

    net->inst = inst;

    net->nidr = idr_alloc(&inst->networks, net, 1, 1 << 31, GFP_KERNEL);

    if (net->nidr < 0) {
        KLOGE("alloc network idr failed");
        synap_free_network(net);
        return NULL;
    }

    inst->network_count++;

    KLOGI("allocating %s buffers of sizes page table=%d, code=%d, pool= %d",
            desc->is_secure ? "secure" : "non secure",
            desc->page_table_size, desc->code_size, desc->pool_size);

    KLOGI("allocating page table buffer");

    net->page_table = synap_mem_alloc(synap_device, SYNAP_MEM_PAGE_TABLE,
                                      desc->is_secure, desc->page_table_size);

    if (!net->page_table) {
        KLOGE("alloc page table mem failed");
        synap_free_network(net);
        return NULL;
    }

    KLOGI("allocating code buffer");

    net->code = synap_mem_alloc(synap_device, SYNAP_MEM_CODE,
                                desc->is_secure, desc->code_size);

    if (!net->code) {
        KLOGE("alloc code area mem failed");
        synap_free_network(net);
        return NULL;
    }

    if (desc->pool_size) {

        KLOGI("allocating pool buffer");

        net->pool = synap_mem_alloc(synap_device, SYNAP_MEM_POOL,
                                    desc->is_secure, desc->pool_size);

        if (!net->pool) {
            KLOGE("alloc pool_area_mem failed");
            synap_free_network(net);
            return NULL;
        }
    }

    // profile
    net->is_profile_mode = desc->is_profile_mode;

    if (net->is_profile_mode) {
        net->layer_count = desc->layer_count;
        net->operation_count = desc->operation_count;
        // layer profile partially assign during ebg header parse
        net->layer_profile = desc->layer_profile_buf;
        net->profile_operation = synap_mem_alloc(synap_device,
                                                 SYNAP_MEM_PROFILE_OP_BUFFER,
                                                 false,
                                                 desc->operation_count * sizeof(struct synap_profile_operation));
        if (!net->profile_operation) {
            KLOGE("failed to alloc profile operation mem");
            synap_free_network(net);
            return NULL;
        }
    }

    KLOGI("alloc network id=%d", net->nidr);
    return net;
}

static bool synap_load_ta(struct synap_ta *ta)
{
    TEEC_Result ret = TEEC_SUCCESS;
    TEEC_SharedMemory teeShm;
    TEEC_Parameter parameter;

    mm_segment_t old_fs;
    struct kstat stat;
    char synap_ta_path[TA_PATH_LEN_MAX] = {0};
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
    loff_t offset = 0;

    enum kernel_read_file_id id = READING_FIRMWARE_PREALLOC_BUFFER;
#else
    enum kernel_read_file_id id = READING_FIRMWARE;
#endif

    LOG_ENTER();

    if (!ta) return false;

    if (!ta_path || !strlen(ta_path) ||
        (strlen(ta_path) >= (TA_PATH_LEN_MAX - strlen(SYNAPTA_NAME) - 1))) {
        KLOGE("failed to load ta from %s", ta_path);
        return false;
    }

    strcat(synap_ta_path, ta_path);
    strcat(synap_ta_path, "/");
    strcat(synap_ta_path, SYNAPTA_NAME);

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    if (vfs_stat((const char __user *)synap_ta_path, &stat) < 0) {
        KLOGE("stat %s failed", synap_ta_path);
        set_fs(old_fs);
        return false;
    }

    KLOGI("libsyapta size=%d", stat.size);

    teeShm.size = stat.size;
    teeShm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
    ret = TEEC_AllocateSharedMemory(
                   &ta->teec_context,
                   &teeShm);
    if (ret != TEEC_SUCCESS || (NULL == teeShm.buffer)) {
        KLOGE("alloc teeshm failed ret=0x%x", ret);
        set_fs(old_fs);
        return false;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    if (kernel_read_file_from_path(synap_ta_path, 0, &teeShm.buffer, teeShm.size, NULL, id) != 0) {
#else
    if (kernel_read_file_from_path(synap_ta_path, &teeShm.buffer, &offset, teeShm.size, id) != 0) {
#endif
        KLOGE("read SyNAP TA failed");
        set_fs(old_fs);
        TEEC_ReleaseSharedMemory(&teeShm);
        return false;
    }

    parameter.memref.parent = &teeShm;
    parameter.memref.size = teeShm.size;
    parameter.memref.offset = 0;

    ret = TEEC_RegisterTA(&ta->teec_context, &parameter, TEEC_MEMREF_PARTIAL_INPUT);
    if (TEEC_SUCCESS == ret) {
        KLOGH("SyNAP TA loaded successfully");
    } else if (TEEC_ERROR_ACCESS_CONFLICT == ret) {
        KLOGE("SyNAP TA has been registered");
        ret = TEEC_SUCCESS;
    } else {
        KLOGE("SyNAP TA loading failed, ret=0x%x", ret);
    }

    TEEC_ReleaseSharedMemory(&teeShm);
    set_fs(old_fs);

    return true;
}

static bool synap_connect_ta(struct synap_ta* ta)
{

    LOG_ENTER();

    if (!ta) return false;

    if (ta->secure_buf->sg_table->nents != 1) {
        KLOGE("Invalid secure area");
        return false;
    }

    if (ta->nonsecure_buf->sg_table->nents  != 1) {
        KLOGE("Invalid non secure area");
        return false;
    }

    if (synap_ca_create_session(&ta->teec_context, &ta->teec_session,
                                ta->secure_buf->sg_table,
                                ta->nonsecure_buf->sg_table) != TEEC_SUCCESS) {
        KLOGE("TEE create session failed");
        return false;
    }

    return true;

}

static void synap_finalize_ta(struct synap_ta *ta)
{

    LOG_ENTER();

    if (!ta) return;

    KLOGI("finalize TA");

    if (synap_ca_deactivate_npu(&ta->teec_session) != TEEC_SUCCESS) {
        KLOGE("TEE npu deactivation failed.");
    }

    if (synap_ca_destroy_session(&ta->teec_session) != TEEC_SUCCESS) {
        KLOGE("TEE destroy session failed.");
    }

    if (ta->secure_buf) synap_mem_free(ta->secure_buf);
    if (ta->nonsecure_buf) synap_mem_free(ta->nonsecure_buf);

    TEEC_FinalizeContext(&ta->teec_context);

    kfree(ta);

}

static struct synap_ta * synap_init_ta(struct synap_device *dev)
{

    struct synap_ta * ta;

    LOG_ENTER();

    if (!dev) return NULL;

    ta = kzalloc(sizeof(struct synap_ta), GFP_KERNEL);

    if (!ta) {
        KLOGE("alloc ta struct failed");
        return NULL;
    }

    if (TEEC_InitializeContext(NULL, &ta->teec_context) != TEEC_SUCCESS) {
        KLOGE("TEE initialize failed");
        return NULL;
    }

    if (!synap_load_ta(ta)) {
        synap_finalize_ta(ta);
        return NULL;
    }

    ta->nonsecure_buf = synap_mem_alloc(dev, SYNAP_MEM_DRIVER_BUFFER,
                                        false, SYNAP_TA_DRIVER_BUFFER_SIZE);
    if (!ta->nonsecure_buf) {
        KLOGE("alloc nonsecure mem failed");
        synap_finalize_ta(ta);
        return NULL;
    }

    ta->secure_buf = synap_mem_alloc(dev, SYNAP_MEM_DRIVER_BUFFER,
                                     true, SYNAP_TA_DRIVER_BUFFER_SIZE);

    if (!ta->secure_buf) {
        KLOGE("alloc secure mem failed");
        synap_finalize_ta(ta);
        return NULL;
    }

    if (!synap_connect_ta(ta)) {
        KLOGE("cannot connect to ta");
        synap_finalize_ta(ta);
        return NULL;
    }

    KLOGI("init synapta successfully");

    return ta;
}

static irqreturn_t synap_irq_handler(int irq, void *data)
{

    TEE_Result ret;

    struct synap_device *synap_device = data;

    LOG_ENTER();

    // read the interrupt value so that the interrupt is cleared on the NPU
    // this is a threaded irq handler so there is no problem to call a blocking function
    ret = synap_ca_read_interrupt_register(&synap_device->ta->teec_session,
                                           &synap_device->irq_status);

    if (ret == TEEC_ERROR_CANCEL) {
        KLOGE("NPU was reset becuse it was not idle when we received the interrupt");
    } else if (ret != TEEC_SUCCESS) {
        KLOGE("error while reading npu status register");
    }

    KLOGI("interrupt with status 0x%08x", synap_device->irq_status);

    // wakeup automatically ensures a memory barrier that ensures irq_status
    // is readable by the code waiting for the status
    wake_up_interruptible(&synap_device->irq_wait_queue);

    return IRQ_HANDLED;
}

static bool do_synap_hardware_lock(struct synap_file *inst)
{

    LOG_ENTER();

    if (!inst) return false;

    if (inst->synap_device->npu_reserved != NULL) {
        KLOGE("npu already reserved");
        return false;
    }

    inst->synap_device->npu_reserved = inst;

    return true;
}

static bool do_synap_hardware_unlock(struct synap_file *inst)
{

    LOG_ENTER();

    if (!inst) return false;

    if (inst->synap_device->npu_reserved != inst) {
        KLOGE("npu not reserved by this task");
        return false;
    }

    inst->synap_device->npu_reserved = NULL;

    return true;
}

static bool do_synap_query_hardware_lock(struct synap_file *inst, u32 *data)
{

    LOG_ENTER();

    if (!inst || !data) return false;

    *data = inst->synap_device->npu_reserved != NULL;

    return true;
}

static bool do_synap_set_input_data(struct synap_file *inst,
                                 struct synap_set_network_io_data *data)
{
    struct synap_network *n;
    struct synap_attachment *b;

    LOG_ENTER();

    if (!inst || !data) return false;

    n = idr_find(&inst->networks, data->nid);

    if (!n) {
        KLOGE("failed to get network from nidr=%d", data->nid);
        return false;
    }

    b = idr_find(&n->attachments, data->aid);

    if (!b) {
        KLOGE("failed to get attachment from aidr=%d", data->aid);
        return false;
    }

    if(synap_ca_set_input(&inst->synap_device->ta->teec_session,
                            n->ta_nid, b->ta_aid, data->index) != TEEC_SUCCESS) {
        return false;
    }


    return true;
}

static bool do_synap_set_output_data(struct synap_file *inst,
                                 struct synap_set_network_io_data *data)
{
    struct synap_network *n;
    struct synap_attachment *b;

    LOG_ENTER();

    if (!inst || !data) return false;

    n = idr_find(&inst->networks, data->nid);

    if (!n) {
        KLOGE("failed to get network from nidr=%d", data->nid);
        return false;
    }

    b = idr_find(&n->attachments, data->aid);

    if (!b) {
        KLOGE("failed to get attachment from aidr=%d", data->aid);
        return false;
    }

    if(synap_ca_set_output(&inst->synap_device->ta->teec_session,
                            n->ta_nid, b->ta_aid, data->index) != TEEC_SUCCESS) {
        return false;
    }

    return true;
}

static void synap_free_io_buffer(struct synap_io_buffer *buf) {

    LOG_ENTER();

    if (buf->mem) {
        synap_mem_free(buf->mem);
    }

    if (buf->bidr >= 0) {
        idr_remove(&buf->inst->io_buffers, buf->bidr);
    }

    kfree(buf);

}

static struct synap_io_buffer * synap_alloc_io_buffer(struct synap_file *inst) {

    struct synap_io_buffer *buf;

    LOG_ENTER();

    buf = kzalloc(sizeof(struct synap_io_buffer), GFP_KERNEL);

    if (!buf) {
        KLOGE("failed to allocate buffer");
        return NULL;
    }

    buf->inst = inst;

    buf->bidr = idr_alloc(&inst->io_buffers, buf, 1, 1 << 31, GFP_KERNEL);

    if (buf->bidr < 0) {
        KLOGE("alloc buffer idr failed");
        kfree(buf);
        return NULL;
    }

    return buf;

}

static bool do_synap_create_io_buffer(struct synap_file *inst,
                                      struct synap_create_io_buffer_data *data)
{

    struct synap_io_buffer *buf = NULL;

    LOG_ENTER();

    if (!inst || !data || data->size == 0) return false;

    buf = synap_alloc_io_buffer(inst);

    if (!buf) {
        KLOGE("failed to allocate buffer");
        return false;
    }

    buf->mem = synap_mem_alloc(inst->synap_device, SYNAP_MEM_IO_BUFFER, false, data->size);

    if (!buf->mem) {
        KLOGE("failed to alloc ion buffer");
        synap_free_io_buffer(buf);
        return false;
    }

    KLOGI("registering new buffer (size %d)", data->size);

    if (synap_ca_create_io_buffer_from_sg(&inst->synap_device->ta->teec_session,
                                  buf->mem->sg_table, 0,
                                  data->size, false, &buf->ta_bid, &buf->mem_id) != TEEC_SUCCESS) {
        synap_free_io_buffer(buf);
        return false;
    }

    // take a reference that we will transfer to the caller
    get_dma_buf(buf->mem->dmabuf);

    data->mem_id = buf->mem_id;
    data->fd = dma_buf_fd(buf->mem->dmabuf, O_CLOEXEC);
    data->bid = buf->bidr;

    KLOGI("created io buffer successfully bid=%d, mem_id=%d, fd=%d (refcount=%d)", data->bid,
          data->mem_id, data->fd, buf->mem->dmabuf->file->f_count);

    return true;

}



static bool do_synap_create_io_buffer_from_dmabuf(struct synap_file *inst,
                    struct synap_create_io_buffer_from_dmabuf_data *data, bool secure)
{

    struct synap_io_buffer *buf = NULL;

    LOG_ENTER();

    if (!inst || !data || data->size == 0) return false;

    buf = synap_alloc_io_buffer(inst);

    if (!buf) {
        KLOGE("failed to allocate buffer");
        return false;
    }

    buf->mem = synap_mem_wrap_fd(inst->synap_device, data->fd, data->size, data->offset);

    if (!buf->mem) {
        KLOGE("failed to wrap fd");
        synap_free_io_buffer(buf);
        return false;
    }

    KLOGI("registering fd %d (offset %d size %d)", data->fd, data->offset, data->size);

    if (synap_ca_create_io_buffer_from_sg(&inst->synap_device->ta->teec_session,
                                  buf->mem->sg_table, data->offset,
                                  data->size, secure, &buf->ta_bid, &buf->mem_id) != TEEC_SUCCESS) {
        synap_free_io_buffer(buf);
        return false;
    }

    data->mem_id = buf->mem_id;
    data->bid = buf->bidr;

    KLOGI("created io buffer from dmabuf successfully bid=%d, mem_id=%d", data->bid, data->mem_id);

    return true;

}

static bool do_synap_create_io_buffer_from_mem_id(struct synap_file *inst,
                    struct synap_create_io_buffer_from_mem_id_data *data)
{

    struct synap_io_buffer *buf = NULL;

    LOG_ENTER();

    if (!inst || !data || data->size == 0) return false;

    buf = synap_alloc_io_buffer(inst);

    if (!buf) {
        KLOGE("failed to allocate buffer");
        return false;
    }

    buf->mem_id = data->mem_id;

    KLOGI("registering mem_id %d (offset %d size %d)", data->mem_id, data->offset, data->size);

    if (synap_ca_create_io_buffer_from_mem_id(&inst->synap_device->ta->teec_session,
                                              buf->mem_id, data->offset,
                                              data->size, &buf->ta_bid) != TEEC_SUCCESS) {
        synap_free_io_buffer(buf);
        return false;
    }

    data->mem_id = buf->mem_id;
    data->bid = buf->bidr;

    KLOGI("created io buffer from mem_id %d successfully bid=%d", data->mem_id, data->bid);

    return true;

}

static bool do_synap_attach_io_buffer(
                struct synap_file *inst,
                struct synap_attach_io_buffer_data *data)
{

    struct synap_network* network = NULL;
    struct synap_io_buffer* buffer = NULL;
    struct synap_attachment *attachment = NULL;

    LOG_ENTER();

    KLOGI("creating attachment for bid=%d to nid=%d", data->bid, data->nid);

    if (!inst || !data ) return false;

    network = idr_find(&inst->networks, data->nid);

    if (network == NULL) {
        KLOGE("failed to get network from nidr=%d", data->nid);
        return false;
    }

    buffer = idr_find(&inst->io_buffers, data->bid);

    if (buffer == NULL) {
        KLOGE("failed to get io buffer from bidr=%d", data->bid);
        return false;
    }

    attachment = synap_alloc_attachment(network);

    if (!attachment) {
        KLOGE("failed to allocate attachment");
        return false;
    }

    KLOGI("attaching bid %d", data->bid);

    if (synap_ca_attach_io_buffer(&inst->synap_device->ta->teec_session,
                                  network->ta_nid, buffer->ta_bid,
                                  &attachment->ta_aid) != TEEC_SUCCESS) {
        synap_free_attachment(attachment);
        return false;
    }

    buffer->ref_count++;

    attachment->buffer = buffer;

    // update statistics
    ++network->io_buff_count;
    if (attachment->buffer->mem != NULL) {
        network->io_buff_size += attachment->buffer->mem->size;
    }

    data->aid = attachment->aidr;

    KLOGI("created attachment successfully aid=%d", data->aid);

    return true;

}

static bool do_synap_destroy_io_buffer(struct synap_file *inst, u32 bid)
{
    struct synap_io_buffer* buf;

    LOG_ENTER();

    KLOGI("destroying io buffer bid=%d", bid);

    if (!inst) {
        return false;
    }

    buf = idr_find(&inst->io_buffers, bid);

    if (buf == NULL) {
        KLOGE("failed to get io buffer from bidr=%d", bid);
        return false;
    }

    if (buf->ref_count > 0) {
        KLOGE("cannot delete buffer currently attached to networks bidr=%d", bid);
        return false;
    }

    if (synap_ca_destroy_io_buffer(&inst->synap_device->ta->teec_session,
                                   buf->ta_bid) != TEEC_SUCCESS) {
        KLOGE("error while releasing  bidr=%d", bid);
    }

    synap_free_io_buffer(buf);

    return true;

}


static bool do_synap_detach_io_buffer(struct synap_file *inst, u32 nid, u32 aid)
{
    struct synap_network* net;
    struct synap_attachment* attachment;

    LOG_ENTER();

    KLOGI("detaching attachment aid=%d from network nid=%d", aid, nid);

    if (!inst) {
        return false;
    }

    net = idr_find(&inst->networks, nid);

    if (!net) {
        KLOGE("failed to get network from nidr=%d", nid);
        return false;
    }

    attachment = idr_find(&net->attachments, aid);

    if (!attachment) {
        KLOGE("failed to get attachment from aidr=%d", aid);
        return false;
    }

    // update statistics
    --net->io_buff_count;
    if (attachment->buffer->mem != NULL) {
        net->io_buff_size -= attachment->buffer->mem->size;
    }

    synap_ca_detach_io_buffer(&inst->synap_device->ta->teec_session, net->ta_nid,
                              attachment->ta_aid);

    attachment->buffer->ref_count--;

    synap_free_attachment(attachment);

    return true;
}

static bool synap_get_network_resources_desc(void* model_buffer, size_t model_size,
        struct synap_network_resources_desc *desc)
{

    struct ebg_file_public_data *pd;
    struct ebg_file_memory_area *code_area;
    struct ebg_file_memory_area *pool_area;
    size_t page_tables_pages;

    LOG_ENTER();

    if (!model_size || !desc || !model_buffer) return false;

    pd = kmalloc(sizeof(struct ebg_file_public_data), GFP_KERNEL);

    if (!pd) return false;

    if (!ebg_file_public_data_parse((u8 *)model_buffer, model_size, pd)) {
        KLOGE("ebg parse failed");
        kfree(pd);
        return false;
    }

    KLOGI("ebg parse done, mem_area_cnt=%d", pd->metadata.memory_area_count);

    if(!ebg_file_get_pool_area(pd, &pool_area)) {
        KLOGI("ebg network has no pool");
        pool_area = NULL;
    }

    if(!ebg_file_get_code_area(pd, &code_area)) {
        KLOGE("ebg get code_area failed");
        kfree(pd);
        return false;
    }

    if (!ebg_file_get_page_table_pages(pd, &page_tables_pages)) {
        KLOGE("unable to compute pages required to store page tables");
        kfree(pd);
        return false;
    }

    if (pool_area) {
        desc->pool_size = SYNAP_MEM_ALIGN_SIZE(pool_area->size);
    } else {
        desc->pool_size = 0;
    }

    desc->code_size = SYNAP_MEM_ALIGN_SIZE(code_area->size);
    desc->page_table_size = SYNAP_MEM_ALIGN_SIZE(1024 * 4 * page_tables_pages);
    desc->is_secure = pd->header.security_type != 0;
    desc->is_profile_mode = pd->metadata.execution_mode != 0;

    if (desc->is_profile_mode) {
        if (!synap_profile_parse_layers(pd, desc)) {
            KLOGE("failed to parse layers");
            kfree(pd);
            return false;
        }
    }

    kfree(pd);
    return true;

}

static bool do_synap_create_network(struct synap_file *inst,
                                           struct synap_create_network_data *data)
{
    TEEC_SharedMemory nbg_shm;
    struct synap_network *network = NULL;
    struct synap_network_resources_desc desc;

    LOG_ENTER();

    if (!inst || !data) return false;

    nbg_shm.buffer = NULL;
    nbg_shm.size = data->size;
    nbg_shm.flags = TEEC_MEM_INPUT;

    if (TEEC_AllocateSharedMemory(&inst->synap_device->ta->teec_context,
                                  &nbg_shm) != TEEC_SUCCESS) {
        KLOGE("failed to allocate shared memory");
        return false;
    }

    if (copy_from_user(nbg_shm.buffer, (void __user *)data->start, data->size) != 0) {
        KLOGE("copy network failed");
        TEEC_ReleaseSharedMemory(&nbg_shm);
        return false;
    }

    if (!synap_get_network_resources_desc(nbg_shm.buffer, nbg_shm.size, &desc)) {
        KLOGE("alloc network failed");
        TEEC_ReleaseSharedMemory(&nbg_shm);
        return false;
    }

    if ((network = synap_alloc_network(inst, &desc)) == NULL) {
        KLOGE("alloc network failed");
        TEEC_ReleaseSharedMemory(&nbg_shm);
        return false;
    }

    if (synap_ca_create_network(&inst->synap_device->ta->teec_session,
                                &nbg_shm,
                                network->code->sg_table,
                                network->page_table->sg_table,
                                network->pool != NULL ? network->pool->sg_table : NULL,
                                network->profile_operation != NULL ? network->profile_operation->sg_table : NULL,
                                &network->ta_nid) != TEEC_SUCCESS) {
        KLOGE("create network failed");
        synap_free_network(network);
        TEEC_ReleaseSharedMemory(&nbg_shm);

        return false;
    };

    TEEC_ReleaseSharedMemory(&nbg_shm);

    KLOGI("created new network nid=%d ta_nid=%d", network->nidr, network->ta_nid);

    data->nid = network->nidr;

    return true;

}


static bool do_synap_start_network(struct synap_file *inst, u32 nid)
{
    u64 start, end, inference_time;
    unsigned long jiffies;
    int wait_result;
    struct synap_network* network;

    bool success = false;

    LOG_ENTER();

    KLOGI("starting network nid=%d", nid);

    network = idr_find(&inst->networks, nid);

    if (!network) {
        KLOGE("failed to get network from nidr=%d", nid);
        return false;
    }

    start = synap_get_microsec();

    if (inst->synap_device->npu_reserved != NULL &&
        inst->synap_device->npu_reserved != inst) {
        KLOGE("cannot execute model because the NPU is reserved by another user");
        return false;
    }

    if (synap_ca_start_network(&inst->synap_device->ta->teec_session, network->ta_nid) != TEEC_SUCCESS) {
        KLOGE("start network failed");
        return false;
    }

    jiffies = msecs_to_jiffies(SYNAP_HARDWARE_IRQ_TIMEOUT);

    KLOGI("wait for interrupt\n");

    // the wait_event_interruptible_timeout automatically makes sure there is a memory barrier
    // that ensures we can read irq_status as written by the irq handler
    wait_result = wait_event_interruptible_timeout(inst->synap_device->irq_wait_queue,
                                                    inst->synap_device->irq_status, jiffies);

    if (wait_result == 0) {
        KLOGE("npu timeout\n");
    } else if (inst->synap_device->irq_status == 0) {
        KLOGE("invalid irq");
    } else if (inst->synap_device->irq_status & 0x80000000) {
        KLOGE("AXI bus error");
    } else if (inst->synap_device->irq_status & 0x40000000) {
        KLOGE("MMU exception");
    } else if (inst->synap_device->irq_status > 0) {
        success = true;
    }

    // in case of error we ensure the NPU is shut down to reset its state
    if (!success) {
        KLOGE("resetting NPU after error")
        synap_ca_dump_state(&inst->synap_device->ta->teec_session);
        synap_ca_deactivate_npu(&inst->synap_device->ta->teec_session);
    }

    inst->synap_device->irq_status = 0;

    end = synap_get_microsec();
    inference_time = end - start;
    KLOGI("execution: %lu us", inference_time);

    if (network->is_profile_mode) {
        synap_profile_get_layers_data(network);
        #if (DEBUG_LEVEL == 3)
        synap_profile_operation_dump(network);
        #endif
    }

    // update statistics
    ++inst->synap_device->inference_count;
    inst->synap_device->inference_time_us += inference_time;

    ++network->inference_count;
    network->inference_time_us += inference_time;
    network->last_inference_time_us = inference_time;

    return success;

}

static bool do_synap_destroy_network(struct synap_file *inst, u32 nid)
{
    struct synap_attachment *b;
    s32 aid;
    struct synap_network *network;

    LOG_ENTER();

    if (!inst) return false;

    KLOGI("destroying network nid=%d", nid);

    network = idr_find(&inst->networks, nid);

    if (!network) {
        KLOGE("failed to get network from nidr=%d", nid);
        return false;
    }

    idr_for_each_entry(&network->attachments, b, aid) {
        do_synap_detach_io_buffer(inst, network->nidr, aid);
    }

    synap_ca_destroy_network(&inst->synap_device->ta->teec_session, network->ta_nid);

    synap_free_network(network);

    return true;
}

static long synap_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    unsigned int dir;
    union synap_ioctl_arg data;
    struct synap_file *inst = NULL;
    bool ret;

    LOG_ENTER();

    if ((inst = (struct synap_file *)filp->private_data) == NULL) {
        KLOGE("get synap file data failed");
        return -EFAULT;
    }

    dir = _IOC_DIR(cmd);
    if (_IOC_SIZE(cmd) > sizeof(data))
        return -EINVAL;

    if (!(dir & _IOC_WRITE))
        memset(&data, 0, sizeof(data));
    else if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
        return -EFAULT;

    mutex_lock(&inst->synap_device->files_mutex);

    // acquire the lock to have exclusive access to this file
    mutex_lock(&inst->mutex);

    // if the command is to run a network, acquire the lock for the hardware
    if (cmd == SYNAP_RUN_NETWORK) {
        mutex_lock(&inst->synap_device->hw_mutex);
    }

    // if we don't need to access the npu reservation we can already free the
    // files_mutex
    if (cmd != SYNAP_LOCK_HARDWARE &&
        cmd != SYNAP_UNLOCK_HARDWARE &&
        cmd != SYNAP_QUERY_HARDWARE_LOCK) {
        mutex_unlock(&inst->synap_device->files_mutex);
    }

    KLOGI("ioctl cmd %s", cmd_to_str(cmd));

    switch (cmd) {
    case SYNAP_SET_NETWORK_INPUT:
        ret = do_synap_set_input_data(inst, &data.set_data);
        break;
    case SYNAP_SET_NETWORK_OUTPUT:
        ret = do_synap_set_output_data(inst, &data.set_data);
        break;
    case SYNAP_CREATE_IO_BUFFER:
        ret = do_synap_create_io_buffer(inst, &data.create_io_buffer_data);
        break;
    case SYNAP_CREATE_IO_BUFFER_FROM_DMABUF:
        ret = do_synap_create_io_buffer_from_dmabuf(inst,
                                &data.create_io_buffer_from_dmabuf_data, false);
        break;
    case SYNAP_CREATE_SECURE_IO_BUFFER_FROM_DMABUF:
        ret = do_synap_create_io_buffer_from_dmabuf(inst,
                                &data.create_secure_io_buffer_from_dmabuf_data.dmabuf_data,
                                data.create_secure_io_buffer_from_dmabuf_data.secure);
        break;
    case SYNAP_CREATE_IO_BUFFER_FROM_MEM_ID:
        ret = do_synap_create_io_buffer_from_mem_id(inst, &data.create_io_buffer_from_mem_id_data);
        break;
    case SYNAP_DESTROY_IO_BUFFER:
        ret = do_synap_destroy_io_buffer(inst, data.bid);
        break;
    case SYNAP_ATTACH_IO_BUFFER:
        ret = do_synap_attach_io_buffer(inst, &data.attach_io_buffer_data);
        break;
    case SYNAP_DETACH_IO_BUFFER:
        ret = do_synap_detach_io_buffer(inst, data.attachment_data.nid, data.attachment_data.aid);
        break;
    case SYNAP_CREATE_NETWORK:
        ret = do_synap_create_network(inst, &data.create_network_data);
        break;
    case SYNAP_RUN_NETWORK:
        ret = do_synap_start_network(inst, data.nid);
        break;
    case SYNAP_DESTROY_NETWORK:
        ret = do_synap_destroy_network(inst, data.nid);
        break;
    case SYNAP_LOCK_HARDWARE:
        ret = do_synap_hardware_lock(inst);
        break;
    case SYNAP_UNLOCK_HARDWARE:
        ret = do_synap_hardware_unlock(inst);
        break;
    case SYNAP_QUERY_HARDWARE_LOCK:
        ret = do_synap_query_hardware_lock(inst, &data.locked);
        break;
    default:
        return -EINVAL;
    }

    // if the command is to run a network, we can now release the hardware lock
    if (cmd == SYNAP_RUN_NETWORK) {
        mutex_unlock(&inst->synap_device->hw_mutex);
    }

    // we needed access the npu reservation data we need now to unlock the files_mutex
    if (cmd == SYNAP_LOCK_HARDWARE ||
        cmd == SYNAP_UNLOCK_HARDWARE ||
        cmd == SYNAP_QUERY_HARDWARE_LOCK) {
        mutex_unlock(&inst->synap_device->files_mutex);
    }

    // we are done with our file, we can unlock the mutex
    mutex_unlock(&inst->mutex);

    if (dir & _IOC_READ) {
        if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd)))
            return -EFAULT;
    }

    KLOGI("ioctl cmd %s (pid %d) -- %s", cmd_to_str(cmd), current->pid, ret ? "success" : "failure");

    return ret ? 0 : -EFAULT;
}

ssize_t synap_drv_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
    LOG_ENTER();

    return -EFAULT;
}

static int32_t synap_power_on(struct platform_device *pdev);

static int32_t synap_drv_open(struct inode * inode, struct file * file)
{

    struct synap_device* synap_device;
    struct synap_file *inst = NULL;
    struct miscdevice *misc;

    LOG_ENTER();

    KLOGI("opening npu device");

    /* this is set by the open wrapper of the misc driver */
    misc = file->private_data;

    synap_device = container_of(misc, struct synap_device, misc_dev);

    inst = kzalloc(sizeof(struct synap_file), GFP_KERNEL);

    if (inst == NULL) {
        KLOGE("synap kernel create instance failed");
        return -ENOMEM;
    }

    inst->synap_device = synap_device;
    inst->pid = task_pid_nr(current->group_leader);

    idr_init(&inst->networks);
    mutex_init(&inst->mutex);

    mutex_lock(&inst->synap_device->files_mutex);

    // if this is the first opened file power on the NPU
    if (list_empty(&inst->synap_device->files)) {
        if (synap_power_on(inst->synap_device->pdev) < 0) {
            KLOGE("failed to power on when init");
            mutex_unlock(&inst->synap_device->files_mutex);
            idr_destroy(&inst->networks);
            kfree(inst);
            return -EFAULT;
        }
    }

    idr_init(&inst->io_buffers);

    list_add(&inst->list, &inst->synap_device->files);

    mutex_unlock(&inst->synap_device->files_mutex);

    file->private_data = inst;

    return 0;
}

static int32_t synap_power_off(struct platform_device *pdev);

static int32_t synap_drv_release(struct inode * inode, struct file * file)
{
    struct synap_file *inst = file->private_data;
    struct synap_network *n;
    struct synap_io_buffer *buf;
    s32 nid, bid;

    LOG_ENTER();

    KLOGI("releasing npu device");

    mutex_lock(&inst->synap_device->files_mutex);

    // make sure there is no-one using this file
    mutex_lock(&inst->mutex);

    if (inst->synap_device->npu_reserved == inst) {
        KLOGI("releasing npu reservation");
        inst->synap_device->npu_reserved = NULL;
    }

    list_del(&inst->list);

    if (list_empty(&inst->synap_device->files)) {
        synap_ca_deactivate_npu(&inst->synap_device->ta->teec_session);
        synap_power_off(inst->synap_device->pdev);
    }

    mutex_unlock(&inst->synap_device->files_mutex);

    idr_for_each_entry(&inst->networks, n, nid) {
        do_synap_destroy_network(inst, nid);
    }

    idr_for_each_entry(&inst->io_buffers, buf, bid) {
         do_synap_destroy_io_buffer(inst, bid);
    }

    // we can unlock since nobody will be able to find this file anymore
    mutex_unlock(&inst->mutex);

    idr_destroy(&inst->networks);
    idr_destroy(&inst->io_buffers);

    kfree(inst);

    return 0;
}

static struct file_operations file_operations =
{
    .owner          = THIS_MODULE,
    .open           = synap_drv_open,
    .release        = synap_drv_release,
    .read           = synap_drv_read,
    .unlocked_ioctl = synap_drv_ioctl,
#ifdef HAVE_COMPAT_IOCTL
    .compat_ioctl   = synap_drv_ioctl,
#endif
};


static int32_t synap_power_on(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct clk *sysclk;
    struct clk *coreclk;

    LOG_ENTER();

    if (NULL == pdev) {
        KLOGH("platform device is NULL \n");
        return -1;
    }

    coreclk = devm_clk_get(dev, "core");
    if (IS_ERR(coreclk)) {
        KLOGH("devm_clk_get failed, clk=%p!\n", coreclk);
        return -1;
    }
    else {
        clk_prepare_enable(coreclk);
    }

    sysclk = devm_clk_get(dev, "sys");
    if (IS_ERR(sysclk)) {
        KLOGH("devm_clk_get failed, clk=%p!\n", sysclk);
        return -1;
    }
    else {
        clk_prepare_enable(sysclk);
    }
    KLOGI("power on....");

    return 0;
}

static int32_t synap_power_off(struct platform_device *pdev)
{
    struct clk *sysclk;
    struct clk *coreclk;
    struct device *dev = &pdev->dev;

    LOG_ENTER();

    if (NULL == pdev) {
        KLOGH("platform device is NULL \n");
        return -1;
    }

    coreclk = devm_clk_get(dev, "core");
    if (IS_ERR(coreclk)) {
        KLOGH("devm_clk_get failed in uninit, clk=%p", coreclk);
        return -1;
    }
    else {
        clk_disable_unprepare(coreclk);
    }

    sysclk = devm_clk_get(dev, "sys");
    if (IS_ERR(sysclk)) {
        KLOGH("devm_clk_get failed in uninit, clk=%p!", sysclk);
        return -1;
    }
    else {
        clk_disable_unprepare(sysclk);
    }

    KLOGI("power off....");

    return 0;
}

static int32_t synap_platform_remove(struct platform_device *pdev) {

    struct synap_device *synap_device;

    LOG_ENTER();

    synap_device = platform_get_drvdata(pdev);

    if (!synap_device) {
        return -EFAULT;
    }

    if (synap_device->misc_registered) {
        misc_deregister(&synap_device->misc_dev);
    }

    if (synap_device->irq_registered) {
        free_irq(synap_device->irq_line, synap_device);
    }

    if (synap_device->ta) synap_finalize_ta(synap_device->ta);

    mutex_destroy(&synap_device->hw_mutex);
    mutex_destroy(&synap_device->files_mutex);

    return 0;
}

static int32_t synap_platform_probe(struct platform_device *pdev)
{

    struct synap_device *synap_device;

    LOG_ENTER();

    KLOGI("initializing device");

    // here we request memory that will be automatically deallocated when the device
    // is removed
    synap_device = devm_kzalloc(&pdev->dev, sizeof(struct synap_device), GFP_KERNEL);

    if (synap_device == NULL) {
        KLOGE("alloc failed for driver device");
        return -1;
    }

    synap_device->pdev = pdev;

    platform_set_drvdata(pdev, synap_device);

    /* find the irq of this device */
    synap_device->irq_line = platform_get_irq(pdev, 0);

    if (synap_device->irq_line < 0) {
        KLOGE("platform_get_irq failed");
        synap_platform_remove(pdev);
        return -1;
    }

    KLOGH("irq line from dts = %d\n", synap_device->irq_line);

    /* setup the TA */

    synap_device->ta = synap_init_ta(synap_device);

    if (!synap_device->ta) {
        KLOGE("failed to init synap ta\n");
        synap_platform_remove(pdev);
        return -1;
    }

    /* configure the structure */

    mutex_init(&synap_device->hw_mutex);
    mutex_init(&synap_device->files_mutex);
    INIT_LIST_HEAD(&synap_device->files);

    init_waitqueue_head(&synap_device->irq_wait_queue);

    /* install IRQ handler */

    // here we request a threaded interrupt handler so we can read the status register
    // with a blocking TA call
    if (request_threaded_irq(synap_device->irq_line, NULL, synap_irq_handler,
                             IRQF_ONESHOT, DEVICE_NAME, synap_device)) {
        KLOGE("request_irq failed");
        synap_platform_remove(pdev);
        return -1;
    }

    synap_device->irq_registered = 1;

    KLOGI("enabled ISR for interrupt line=%d\n", synap_device->irq_line);

    /* register a character device to control this device */

    synap_device->misc_dev.name = DEVICE_NAME;
    synap_device->misc_dev.groups = synap_device_groups;
    synap_device->misc_dev.parent = &pdev->dev;
    synap_device->misc_dev.fops = &file_operations;
    synap_device->misc_dev.minor = MISC_DYNAMIC_MINOR;

    if (misc_register(&synap_device->misc_dev) < 0) {
        KLOGE("cannot register character device");
        synap_platform_remove(pdev);
        return -1;
    }

    synap_device->misc_registered = true;

    KLOGH("driver initialize successfully");

    return 0;

}


static int32_t synap_platform_suspend(struct platform_device *pdev,
                                      pm_message_t state)
{
    int32_t ret = 0;
    struct synap_device *synap_device;

    LOG_ENTER();

    synap_device = (struct synap_device *) platform_get_drvdata(pdev);

    mutex_lock(&synap_device->files_mutex);

    // return directly if no opened files
    if (list_empty(&synap_device->files)) {
        mutex_unlock(&synap_device->files_mutex);
        KLOGH("suspend");
        return 0;
    }

    mutex_lock(&synap_device->hw_mutex);

    if ((ret = synap_ca_deactivate_npu(&synap_device->ta->teec_session)) != TEEC_SUCCESS) {
        KLOGE("TEE npu deactivation failed. ret=0x%08x\n", ret);
        mutex_unlock(&synap_device->hw_mutex);
        mutex_unlock(&synap_device->files_mutex);
        return -EFAULT;
    }

    synap_power_off(pdev);

    mutex_unlock(&synap_device->hw_mutex);
    mutex_unlock(&synap_device->files_mutex);

    KLOGH("suspend");

    return 0;
}

static int32_t synap_platform_resume(struct platform_device *pdev)
{
    struct synap_device *synap_device;

    LOG_ENTER();

    synap_device = (struct synap_device *) platform_get_drvdata(pdev);

    mutex_lock(&synap_device->files_mutex);
    mutex_lock(&synap_device->hw_mutex);

    if (!list_empty(&synap_device->files)) {
        if (synap_power_on(pdev) < 0) {
            KLOGE("failed to power on...");
            mutex_lock(&synap_device->hw_mutex);
            mutex_unlock(&synap_device->files_mutex);
            return -EFAULT;
        }
    }

    /* we don't activate the npu yet as we don't know the mode, it will be automatically
       activated at the next inference */

    mutex_unlock(&synap_device->files_mutex);
    mutex_unlock(&synap_device->hw_mutex);

    KLOGH("resume");

    return 0;
}

static struct platform_driver synap_platform_driver = {
    .probe      = synap_platform_probe,
    .remove     = synap_platform_remove,
    .suspend    = synap_platform_suspend,
    .resume     = synap_platform_resume,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = DEVICE_NAME,
        .of_match_table = synap_dev_match
    }
};

int32_t synap_kernel_module_init(void)
{

    LOG_ENTER();

    KLOGI("initalizing synap module");

    if (!synap_mem_init()) {
        KLOGE("platform driver register failed.");
        return -ENODEV;
    }

    if (platform_driver_register(&synap_platform_driver) < 0) {
        KLOGE("platform driver register failed.");
        return -ENODEV;
    }

    return 0;
}

void synap_kernel_module_exit(void)
{
    LOG_ENTER();

    platform_driver_unregister(&synap_platform_driver);
}

