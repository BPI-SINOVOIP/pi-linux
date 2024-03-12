#pragma once

#include "synap_kernel.h"
#include "ebg_file.h"
#include <linux/types.h>
#include <linux/device.h>
#include "ca/synap_ta_cmd.h"

// only sh/nn/tp valid
enum synap_operation_type {
    SYNAP_OPERATION_TYPE_NONE = 0,
    SYNAP_OPERATION_TYPE_SH = 1,
    SYNAP_OPERATION_TYPE_NN = 2,
    SYNAP_OPERATION_TYPE_TP = 3,
    SYNAP_OPERATION_TYPE_SW = 4,
    SYNAP_OPERATION_TYPE_SC = 5,
    SYNAP_OPERATION_TYPE_END = 0xFFFE,
    SYNAP_OPERATION_TYPE_INIT = 0xFFFF,
};

struct synap_profile_operation {
    u32 type;           /* operation type */
    u32 layer_idx;      /* layer index of operation */
    u32 execution_time; /* execution time in us */
    u32 cycle;          /* operation cycle count */
    u32 total_read_bw;  /* Total read bandwidth in bytes */
    u32 total_write_bw; /* Total write bandwidth in bytes */
    u32 axi_read_bw;    /* axi read bandwidth in bytes */
    u32 axi_write_bw;   /* axi write bandwidth in bytes */
    u32 ddr_read_bw;    /* ddr read bandwidth in bytes */
    u32 ddr_write_bw;   /* ddr write bandwidth in bytes */
};

struct synap_profile_layer {
    u32 uid;
    char name[64];
    u32 type;
    u32 cycle;
    u32 execution_time;
    u32 byte_read;
    u32 byte_write;
};


bool synap_profile_parse_layers(struct ebg_file_public_data *pd,
                                struct synap_network_resources_desc *desc);

bool synap_profile_get_layers_data(struct synap_network *network);

void synap_profile_operation_dump(struct synap_network *network);

const char *synap_operation_type2str(u32 operation_type);

