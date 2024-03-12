// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#pragma once

#include <linux/types.h>
#include <linux/cdev.h>
#include <tee_client_api.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>


#ifndef CLASS_NAME
#   define CLASS_NAME     "synap_class"
#endif

#ifndef DEVICE_NAME
#   define DEVICE_NAME    "synap"
#endif


struct synap_ta {
    TEEC_Context teec_context;
    TEEC_Session teec_session;

    /* buffers used by the TA to store initialization sequences */
    struct synap_mem *nonsecure_buf;
    struct synap_mem *secure_buf;
};

struct synap_device {

    /* this is the device that is registered in the DTS */
    struct platform_device *pdev;

    /* this is the character device interface to allow user space to talk to the device */
    struct miscdevice misc_dev;

    /* is the misc device registered ? */
    bool misc_registered;

    /* this is the value of the interrup status register of the NPU */
    u32 irq_status;

    /* a task startes a network execution waits on this queue until it is
       woken up by the interrupt handling task (irq_work) */
    wait_queue_head_t irq_wait_queue;

    /* this is the irq line of where the npu hardware signals completion */
    u32 irq_line;

    /* is the irq handler registered ? */
    bool irq_registered;

    /* global statistics */
    uint64_t inference_count;
    uint64_t inference_time_us;

    /* protects files field */
    struct mutex files_mutex;

    /* list of all the currently open files */
    struct list_head files;

    /* indicates whether the npu is currently reserved by one of the clients */
    struct synap_file* npu_reserved;

    /* protects usage of the npu hardware -- must be locked only while holding files_mutex */
    struct mutex hw_mutex;

    struct synap_ta *ta;

    u32 profile_request_idx;
    u32 profile_request_lines;
};


enum _irq_status {
    IRQ_STATUS_ENABLED  = 0,
    IRQ_STATUS_DISABLED = 1,
};

struct synap_attachment {
    u32 ta_aid;

    /* attachment idr index */
    s32 aidr;

    /* owner */
    struct synap_network *network;

    /* buffer that is attached */
    struct synap_io_buffer *buffer;

};

struct synap_network {

    u32 ta_nid;

    /* network idr index */
    s32 nidr;

    /* memory areas used by the npu hardware for this model */
    struct synap_mem* code;
    struct synap_mem* pool;
    struct synap_mem* page_table;

    /* statistics */
    uint64_t inference_count;
    uint64_t inference_time_us;
    uint32_t last_inference_time_us;
    int32_t io_buff_count;
    int32_t io_buff_size;

    /* profile */
    struct synap_mem* profile_operation;
    struct synap_profile_layer *layer_profile;
    bool is_profile_mode;
    u32 layer_count;
    u32 operation_count;

    /* owner */
    struct synap_file *inst;

    /* all synap_attachment created for this network */
    struct idr attachments;

};

struct synap_io_buffer {

    /* owner */
    struct synap_file *inst;

    /* how many attachments to this buffer exist */
    u32 ref_count;

    /* buffer idr index */
    s32 bidr;

    /* mem_id used by TZK to indetify the buffer (for scattered buffers) */
    u32 mem_id;

    /* id used by the TA to identify this io buffer */
    u32 ta_bid;

    /* backing dmabuf */
    struct synap_mem *mem;

    /* used to store the object in the synap_file struct */
    struct list_head list;
};

struct synap_file {
    /* current process id */
    pid_t pid;

    /* owner */
    struct synap_device *synap_device;

    /* all synap_network created for this file */
    struct idr networks;

    /* all the io buffers created for this file */
    struct idr io_buffers;

    u32 network_count;

    /* used to store the object in the synap_device struct */
    struct list_head list;

    /* protects the synap file fields, its networks and the corresponding buffers,
        must locked only while holding synap_device->files_mutex */
    struct mutex mutex;
};

struct synap_network_resources_desc {
    bool is_secure;
    size_t code_size;
    size_t pool_size;
    size_t page_table_size;

    bool is_profile_mode;
    u32 layer_count;
    u32 operation_count;
    struct synap_profile_layer *layer_profile_buf;
};


int32_t synap_kernel_module_init(void);

void synap_kernel_module_exit(void);

extern const struct attribute_group *synap_device_groups[];
