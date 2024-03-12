// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#include <linux/string.h>

#include "synap_kernel_ca.h"
#include "synap_kernel_log.h"
#include "tee_client_const.h"

static const char *synap_ta_log_code_to_str(TEEC_Result code) {
    switch (code) {
        case TEEC_SUCCESS: return "SUCCESS";
        case TEEC_ERROR_GENERIC: return "GENERIC";
        case TEEC_ERROR_ACCESS_DENIED: return "ACCESS_DENIED";
        case TEEC_ERROR_CANCEL: return "CANCEL";
        case TEEC_ERROR_ACCESS_CONFLICT: return "ACCESS_CONFLICT";
        case TEEC_ERROR_EXCESS_DATA: return "EXCESS_DATA";
        case TEEC_ERROR_BAD_FORMAT: return "BAD_FORMAT/INCOMPATIBLE_HW";
        case TEEC_ERROR_BAD_PARAMETERS: return "BAD_PARAMETERS";
        case TEEC_ERROR_BAD_STATE: return "BAD_STATE";
        case TEEC_ERROR_ITEM_NOT_FOUND: return "ITEM_NOT_FOUND";
        case TEEC_ERROR_NOT_IMPLEMENTED: return "NOT_IMPLEMENTED";
        case TEEC_ERROR_NOT_SUPPORTED: return "NOT_SUPPORTED";
        case TEEC_ERROR_NO_DATA: return "NO_DATA";
        case TEEC_ERROR_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case TEEC_ERROR_BUSY: return "BUSY";
        case TEEC_ERROR_COMMUNICATION: return "COMMUNICATION";
        case TEEC_ERROR_SECURITY: return "SECURITY";
        case TEEC_ERROR_SHORT_BUFFER: return "SHORT_BUFFER";
        default: return "Unknown";
    }
}


TEEC_Result synap_ca_create_session(TEEC_Context *context,
                                    TEEC_Session *session,
                                    struct sg_table *secure_buf,
                                    struct sg_table *non_secure_buf)
{
    TEEC_SharedMemory shm;
    TEE_Result result;
    struct page *pg = NULL;
    TEEC_Operation operation;
    struct synap_ta_memory_area* areas;
    const TEEC_UUID uuid = SYNAP_TA_UUID;

    LOG_ENTER();

    memset(&operation, 0, sizeof(TEEC_Operation));

    if (secure_buf->nents != 1 || non_secure_buf->nents != 1) {
        KLOGE("only contiguous driver buffers are supported");
        return TEEC_ERROR_BAD_PARAMETERS;
    }

    shm.size = 2 * sizeof(struct synap_ta_memory_area);
    shm.flags = TEEC_MEM_INPUT;

    if ((result = TEEC_AllocateSharedMemory(context, &shm)) != TEEC_SUCCESS) {
        KLOGE("cannot allocate shared memory");
        return result;
    }

    areas = (struct synap_ta_memory_area *) shm.buffer;

    pg = sg_page(non_secure_buf->sgl);
    areas[0].addr = PFN_PHYS(page_to_pfn(pg));
    areas[0].npage = non_secure_buf->sgl->length / PAGE_SIZE;

    pg = sg_page(secure_buf->sgl);
    areas[1].addr = PFN_PHYS(page_to_pfn(pg));
    areas[1].npage = secure_buf->sgl->length / PAGE_SIZE;

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT, TEEC_NONE,
                                            TEEC_NONE, TEEC_NONE);

    operation.params[0].memref.parent = &shm;
    operation.params[0].memref.size = shm.size;
    operation.params[0].memref.offset = 0;

    result = TEEC_OpenSession(context, session, &uuid, TEEC_LOGIN_USER, NULL,
                           &operation, (uint32_t *) NULL);

    if (result != TEEC_SUCCESS) {
        KLOGE("error while opening session ret=0x%08x, shm.phyAddr=0x%08x", result, shm.phyAddr);
    }

    TEEC_ReleaseSharedMemory(&shm);

    return result;
}

TEEC_Result synap_ca_activate_npu(TEEC_Session *session, u8 mode)
{
    TEEC_Operation operation;
    TEEC_Result result;
    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.params[0].value.a = mode > 0 ? 1 : 0;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_ACTIVATE_NPU, &operation, NULL);
    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }

    return result;
}


TEEC_Result synap_ca_deactivate_npu(TEEC_Session *session)
{
    TEEC_Operation operation;
    TEEC_Result result;
    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_DEACTIVATE_NPU, &operation, NULL);
    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }

    return result;
}



TEEC_Result synap_ca_set_input(TEEC_Session *session, u32 nid, u32 aid, u32 index)
{
    TEEC_Operation operation;
    TEEC_Result result;
    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                                            TEEC_NONE, TEEC_NONE);
    operation.params[0].value.a = nid;
    operation.params[0].value.b = aid;
    operation.params[1].value.a = index;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_SET_INPUT, &operation, NULL);
    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }

    return result;
}

TEEC_Result synap_ca_set_output(TEEC_Session *session, u32 nid, u32 aid, u32 index)
{
    TEEC_Operation operation;
    TEEC_Result result;
    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                                            TEEC_NONE, TEEC_NONE);
    operation.params[0].value.a = nid;
    operation.params[0].value.b = aid;
    operation.params[1].value.a = index;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_SET_OUTPUT, &operation, NULL);
    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }

    return result;
}

TEEC_Result synap_ca_create_network(TEEC_Session *session, TEEC_SharedMemory *data,
                                    struct sg_table *code, struct sg_table *page_table,
                                    struct sg_table *pool,
                                    struct sg_table *profile_op,
                                    u32 *nid)
{

    TEEC_SharedMemory shm;
    struct scatterlist *sg = NULL;
    struct page *pg = NULL;
    u32 i = 0;
    u32 offset = 0;

    TEEC_Operation operation;
    TEEC_Result result;
    struct synap_ta_network_resources *res;

    LOG_ENTER();

    memset(&operation, 0, sizeof(TEEC_Operation));

    shm.size = sizeof(struct synap_ta_network_resources);

    shm.size += sizeof(struct synap_ta_memory_area) * code->nents;
    shm.size += sizeof(struct synap_ta_memory_area) * page_table->nents;

    if (pool != NULL) {
        shm.size += sizeof(struct synap_ta_memory_area) * pool->nents;
    }

    if (profile_op != NULL) {
        shm.size += sizeof(struct synap_ta_memory_area) * profile_op->nents;
    }

    shm.flags = TEEC_MEM_INPUT;

    if ((result = TEEC_AllocateSharedMemory(session->device, &shm)) != TEEC_SUCCESS) {
        KLOGE("cannot allocate shared memory");
        return result;
    }

    res = shm.buffer;

    res->code_areas_count = code->nents;
    res->page_table_areas_count = page_table->nents;

    if (pool != NULL) {
        res->pool_areas_count = pool->nents;
    } else {
        res->pool_areas_count = 0;
    }

    if (profile_op != NULL) {
        res->profile_operation_areas_count = profile_op->nents;
    } else {
        res->profile_operation_areas_count = 0;
    }

    for_each_sg(code->sgl, sg, code->nents, i) {
        pg = sg_page(sg);
        res->areas[i].addr = PFN_PHYS(page_to_pfn(pg));
        res->areas[i].npage = sg->length / PAGE_SIZE;
        KLOGI("code sg 0x%08x %d", res->areas[i].addr, res->areas[i].npage);
    }

    offset = res->code_areas_count;

    for_each_sg(page_table->sgl, sg, page_table->nents, i) {
        pg = sg_page(sg);
        res->areas[i + offset].addr = PFN_PHYS(page_to_pfn(pg));
        res->areas[i + offset].npage = sg->length / PAGE_SIZE;
        KLOGI("pt sg 0x%08x %d", res->areas[i + offset].addr, res->areas[i + offset].npage);
    }

    offset += res->page_table_areas_count;

    if (pool != NULL) {
        for_each_sg(pool->sgl, sg, pool->nents, i) {
            pg = sg_page(sg);
            res->areas[i + offset].addr = PFN_PHYS(page_to_pfn(pg));
            res->areas[i + offset].npage = sg->length / PAGE_SIZE;
            KLOGI("pool sg 0x%08x %d", res->areas[i + offset].addr, res->areas[i+ offset].npage);
        }
    }

    offset += res->pool_areas_count;

    if (profile_op != NULL) {
        for_each_sg(profile_op->sgl, sg, profile_op->nents, i) {
            pg = sg_page(sg);
            res->areas[i + offset].addr = PFN_PHYS(page_to_pfn(pg));
            res->areas[i + offset].npage = sg->length / PAGE_SIZE;
            KLOGI("profile sg 0x%08x %d", res->areas[i + offset].addr, res->areas[i+ offset].npage);
        }
    }

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT, TEEC_MEMREF_PARTIAL_INPUT,
                                            TEEC_VALUE_OUTPUT, TEEC_NONE);

    operation.params[0].memref.parent = data;
    operation.params[0].memref.size = data->size;

    operation.params[1].memref.parent = &shm;
    operation.params[1].memref.size =  shm.size;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_CREATE_NETWORK, &operation,
                                (uint32_t *) NULL);

    if (result != TEEC_SUCCESS) {
        // SECURITY error here normally means that the network has been crypted/signed using a different key set
        KLOGE("Error=0x%08x (%s) data.phyAddr=0x%08x size %d shm.phyAddr=0x%08x size %d code count %d pt count %d pool count %d",
              result, synap_ta_log_code_to_str(result), data->phyAddr, data->size, shm.phyAddr, shm.size, code->nents, page_table->nents, pool ? pool->nents: -1);
    }

    TEEC_ReleaseSharedMemory(&shm);

    *nid = operation.params[2].value.a;
    return result;
}

TEEC_Result synap_ca_destroy_network(TEEC_Session *session, u32 nid)
{
    TEEC_Operation operation;
    TEEC_Result result;
    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.params[0].value.a = nid;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_DESTROY_NETWORK, &operation, NULL);
    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }
    return result;
}

TEEC_Result synap_ca_start_network(TEEC_Session *session, u32 nid)
{
    TEEC_Operation operation;
    TEEC_Result result;
    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.params[0].value.a = nid;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_START_NETWORK, &operation, NULL);
    if (result != TEEC_SUCCESS) {
        // BAD_FORMAT here normally means that the network has been compiled for an incompatible HW
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }
    return result;
}

static TEE_Result synap_ca_create_areas(TEEC_Session *session,
                                        struct sg_table *buf, u32 offset, u32 size,
                                        TEEC_SharedMemory *shm) {

    struct scatterlist *sg = NULL;
    struct page *pg = NULL;
    u32 i = 0;

    struct synap_ta_memory_area* areas;

    u64 mem_start, mem_end;
    size_t area_idx = 0;
    u32 areas_count = 0;
    u32 current_offset = 0;

    LOG_ENTER();

    if (offset % PAGE_SIZE != 0) {
        KLOGE("offset is not a multiple of PAGE_SIZE");
        return TEEC_ERROR_BAD_PARAMETERS;
    }

    mem_start = offset;
    mem_end = offset + size;

    if (buf->nents == 0) {
        KLOGE("sg list has no entries");
        return TEEC_ERROR_BAD_PARAMETERS;
    }

    // Count the number of segments that fall (completely or partially)
    // in the range [offset, offset + size)
    for_each_sg(buf->sgl, sg, buf->nents, i) {

        if (current_offset < mem_end && current_offset + sg->length > mem_start) {
            areas_count++;
        }

        current_offset += sg->length;
    }

    shm->size = areas_count * sizeof(struct synap_ta_memory_area);
    shm->flags = TEEC_MEM_INPUT;

    if (TEEC_AllocateSharedMemory(session->device, shm) != TEEC_SUCCESS) {
        KLOGE("cannot allocate shared memory");
        return TEEC_ERROR_OUT_OF_MEMORY;
    }

    areas = (struct synap_ta_memory_area *) shm->buffer;

    current_offset = 0;

    // Select and cut the areas so that they don't go outside the range [offset, offset + size)
    for_each_sg(buf->sgl, sg, buf->nents, i) {

        u64 sg_start;
        u64 sg_end;

        pg = sg_page(sg);

        KLOGI("processing sg addr 0x%08x pages %d", (uint32_t) PFN_PHYS(page_to_pfn(pg)),
              sg->length / PAGE_SIZE);

        sg_start = current_offset;
        sg_end = current_offset + sg->length;

        if (sg_end > mem_start && sg_start < mem_end ) {

            u64 range_start = sg_start > mem_start ? sg_start: mem_start;
            u64 range_end = sg_end < mem_end ? sg_end : mem_end;

            areas[area_idx].addr = PFN_PHYS(page_to_pfn(pg)) + (range_start - current_offset);
            areas[area_idx].npage = (range_end - range_start) / PAGE_SIZE;

            if ((range_end - range_start) % PAGE_SIZE != 0) {
                areas[area_idx].npage += 1;
            }

            KLOGI("created sg addr=0x%x len=%d", areas[area_idx].addr, areas[area_idx].npage);

            area_idx++;
        }

        current_offset += sg->length;

    }

    return TEEC_SUCCESS;

}


TEEC_Result synap_ca_create_io_buffer_from_mem_id(TEEC_Session *session, u32 mem_id,
                                                  u32 offset, u32 size, u32 *bid) {
    TEEC_Operation operation;
    TEEC_Result result;

    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                                            TEEC_VALUE_OUTPUT, TEEC_NONE);

    operation.params[0].value.a = mem_id;

    operation.params[1].value.a = offset;
    operation.params[1].value.b = size;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_CREATE_IO_BUFFER_FROM_MEM_ID,
                                &operation, (uint32_t *) NULL);

    *bid = operation.params[2].value.a;

    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }
    return result;
}


TEEC_Result synap_ca_create_io_buffer_from_sg(TEEC_Session *session, struct sg_table *buf,
                                              u32 offset, u32 size, bool secure, u32 *bid,
                                              u32 *mem_id) {
    TEEC_SharedMemory shm;
    TEE_Result result;

    TEEC_Operation operation;

    LOG_ENTER();

    memset(&operation, 0, sizeof(TEEC_Operation));

    if ((result = synap_ca_create_areas(session, buf, offset, size, &shm)) != TEEC_SUCCESS) {
        return result;
    }

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT, TEEC_VALUE_INPUT,
                                            TEEC_VALUE_OUTPUT, TEEC_NONE);
    operation.params[0].memref.parent = &shm;
    operation.params[0].memref.size = shm.size;

    operation.params[1].value.a = secure ? 1 : 0;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_CREATE_IO_BUFFER_FROM_SG,
                             &operation, (uint32_t *) NULL);

    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s), shm.phyAddr=0x%08x", result, synap_ta_log_code_to_str(result), shm.phyAddr);
    }

    TEEC_ReleaseSharedMemory(&shm);

    if (result == TEEC_SUCCESS) {
        *bid = operation.params[2].value.a;
        *mem_id = operation.params[2].value.b;
    }

    return result;
}

TEEC_Result synap_ca_attach_io_buffer(TEEC_Session *session, u32 nid,
                                      u32 bid, u32 *aid) {
    TEEC_Operation operation;
    TEEC_Result result;
    LOG_ENTER();

    memset(&operation, 0, sizeof(TEEC_Operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_OUTPUT,
                                            TEEC_NONE, TEEC_NONE);

    operation.params[0].value.a = bid;
    operation.params[0].value.b = nid;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_ATTACH_IO_BUFFER,
                                &operation, (uint32_t *) NULL);
    *aid = operation.params[1].value.a;

    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }
    return result;
}



TEEC_Result synap_ca_detach_io_buffer(TEEC_Session *session, u32 nid, u32 aid)
{
    TEEC_Operation operation;
    TEEC_Result result;
    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.params[0].value.a = nid;
    operation.params[0].value.b = aid;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_DETACH_IO_BUFFER, &operation, NULL);
    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }
    return result;
}


TEEC_Result synap_ca_destroy_io_buffer(TEEC_Session *session, u32 bid)
{
    TEEC_Operation operation;
    TEEC_Result result;
    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    operation.params[0].value.a = bid;

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_DESTROY_IO_BUFFER, &operation, NULL);
    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }
    return result;
}


TEEC_Result synap_ca_read_interrupt_register(TEEC_Session *session, volatile u32 *reg_value)
{
    TEEC_Operation operation;
    TEEC_Result result;

    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_READ_INTERRUPT_REGISTER, &operation, NULL);
    *reg_value = operation.params[0].value.a;

    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }
    return result;
}

TEEC_Result synap_ca_dump_state(TEEC_Session *session) {
    TEEC_Operation operation;
    TEEC_Result result;

    LOG_ENTER();

    memset(&operation, 0, sizeof(operation));

    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    result = TEEC_InvokeCommand(session, SYNAP_TA_CMD_DUMP_STATE, &operation, NULL);

    if (result != TEEC_SUCCESS) {
        KLOGE("Error=0x%08x (%s)", result, synap_ta_log_code_to_str(result));
    }
    return result;
}


TEEC_Result synap_ca_destroy_session(TEEC_Session *session)
{
    LOG_ENTER();

    TEEC_CloseSession(session);
    return TEEC_SUCCESS;
}

