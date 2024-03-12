// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#include <linux/types.h>
#include <tee_client_api.h>
#include <ca/synap_ta_cmd.h>
#include <linux/dma-buf.h>


TEEC_Result synap_ca_create_session(TEEC_Context *context,
                                    TEEC_Session *session,
                                    struct sg_table *secure_buf,
                                    struct sg_table *non_secure_buf);

TEEC_Result synap_ca_set_input(TEEC_Session *session, u32 nid, u32 bid, u32 index);

TEEC_Result synap_ca_set_output(TEEC_Session *session, u32 nid, u32 bid, u32 index);

TEEC_Result synap_ca_activate_npu(TEEC_Session *session, u8 mode);

TEEC_Result synap_ca_deactivate_npu(TEEC_Session *session);

TEEC_Result synap_ca_create_network(TEEC_Session *session, TEEC_SharedMemory *data,
                                    struct sg_table *code, struct sg_table *page_table,
                                    struct sg_table *pool, struct sg_table *profile_data,
                                    u32 *nid);

TEEC_Result synap_ca_destroy_network(TEEC_Session *session, u32 nid);

TEEC_Result synap_ca_start_network(TEEC_Session *session, u32 nid);

TEEC_Result synap_ca_read_interrupt_register(TEEC_Session *session, volatile u32 *reg_value);

TEEC_Result synap_ca_dump_state(TEEC_Session *session);

TEEC_Result synap_ca_create_io_buffer_from_sg(TEEC_Session *session, struct sg_table *buf,
                                              u32 offset, u32 size, bool secure, u32 *bid,
                                              u32 *mem_id);

TEEC_Result synap_ca_create_io_buffer_from_mem_id(TEEC_Session *session, u32 mem_id,
                                                  u32 offset, u32 size, u32 *bid);

TEEC_Result synap_ca_destroy_io_buffer(TEEC_Session *session, u32 bid);

TEEC_Result synap_ca_attach_io_buffer(TEEC_Session *session, u32 nid, u32 bid, u32 *aid);

TEEC_Result synap_ca_detach_io_buffer(TEEC_Session *session, u32 nid, u32 aid);

TEEC_Result synap_ca_destroy_session(TEEC_Session *session);
