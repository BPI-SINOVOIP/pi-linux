// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#pragma once

#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#else
#include <linux/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ebg_file_header {
    uint8_t magic[4];
    uint32_t endianness;
    uint32_t version;
    uint32_t security_type;
    uint32_t security_info_length;
    uint32_t metadata_length;
    uint32_t vsi_data_length;
    uint32_t padding_length;
    uint32_t code_length;
};

#define EBG_FILE_HEADER_SIZE ( 9 * 4 )

enum {
    HARDWARE_TYPE_AS370 = 0x80,
    HARDWARE_TYPE_AS371 = 0x80,
    HARDWARE_TYPE_VS680Z1 = 0xA3,
    HARDWARE_TYPE_VS680 = 0xC1,
    HARDWARE_TYPE_VS640 = 0xC2
};

struct ebg_file_metadata {
    uint32_t hardware_type;
    uint8_t network_name[64];
    uint32_t compiler_id;
    uint32_t compiler_version;
    uint32_t execution_mode;
    uint32_t input_count;
    uint32_t output_count;
    uint32_t memory_area_count;
};

#define EBG_FILE_METADATA_SIZE ( 4 + 64 + 6 * 4 )

enum {
    OPERATION_TYPE_SH = 1,
    OPERATION_TYPE_NN = 2,
    OPERATION_TYPE_TP = 3,
};

struct ebg_file_memory_area {
    uint32_t type;
    uint32_t address;
    uint32_t size;
    uint32_t alignment;
    uint32_t page_size;
};

#define EBG_FILE_MEMORY_AREA_SIZE ( 5 * 4 )

#ifndef EBG_FILE_MAX_AREAS
#define EBG_FILE_MAX_AREAS 1024
#endif

struct ebg_file_public_data {
    struct ebg_file_header header;
    struct ebg_file_metadata metadata;
    uint8_t *public_data;
    size_t public_data_len;
    struct ebg_file_memory_area area[EBG_FILE_MAX_AREAS];
};

#define EBG_FILE_REALLOC_TABLE_SIZE ( 4 * 4 )

struct ebg_file_layer {
    uint32_t uid;
    uint8_t layer_name[64];
};

#define EBG_FILE_LAYER_TABLE_SIZE ( 4 + 64 )

struct ebg_file_operation {
    uint32_t layer_idx;
    uint32_t offset;
    uint32_t size;
    uint32_t type;
};

#define EBG_FILE_OPERATION_TABLE_SIZE ( 4 * 4 )

struct ebg_file_vsi_data {
    uint8_t *data;
    uint32_t data_len;
    uint32_t init_realloc_count;
    uint32_t code_realloc_count;
    uint32_t layer_count;
    uint32_t operation_count;
};


/// returns true if the file looks like an EBG (magic matches)
bool ebg_file_is_ebg(uint8_t *ebg, size_t len);

/// parses the first header in the EBG file (no details on the model metadata)
/// returns true in case of success, false otherwise
bool ebg_file_header_parse(uint8_t *ebg, size_t len, struct ebg_file_header* header);

/// parses all the clear information in the EBG file
/// returns true in case of success, false otherwise
bool ebg_file_public_data_parse(uint8_t *ebg, size_t len, struct ebg_file_public_data* clear_data);

/// find the information about the given memory area (idx must be smaller than
/// clear_data->metadata.memory_area_count). This can be used to find code and pool information.
/// returns true in case of success, false otherwise
bool ebg_file_get_memory_area(struct ebg_file_public_data *clear_data, uint32_t idx,
                              struct ebg_file_memory_area** area);

/// find the information about a given input of the network (idx must be smaller than
/// clear_data->metadata.input_count).
/// returns true in case of success, false otherwise
bool ebg_file_get_input_area(struct ebg_file_public_data *clear_data, uint32_t idx,
                                          struct ebg_file_memory_area** area);

/// find the information about a given output of the network (idx must be smaller than
/// clear_data->metadata.output_count).
/// returns true in case of success, false otherwise
bool ebg_file_get_output_area(struct ebg_file_public_data *clear_data, uint32_t idx,
                              struct ebg_file_memory_area** area);


/// parse the VSI reserved data in the model file
/// returns true in case of success, false otherwise
uint8_t* ebg_file_vsi_data_parse(struct ebg_file_public_data *clear_data,
                           struct ebg_file_vsi_data* vsi_data);

/// get the layers in the model file
/// returns true in case of success, false otherwise
bool ebg_file_get_layers(struct ebg_file_public_data *clear_data,
                             struct ebg_file_layer *layers,
                             size_t layer_count);

/// get the operations in the model file
/// returns true in case of success, false otherwise
bool ebg_file_get_operations(struct ebg_file_public_data *clear_data,
                             struct ebg_file_operation *operations,
                             size_t operation_count);

/// find the information about the memory pool required to execute the network
/// returns true in case of success, false otherwise
bool ebg_file_get_pool_area(struct ebg_file_public_data *clear_data,
                            struct ebg_file_memory_area** area);

/// find the information about the code of the model
/// returns true in case of success, false otherwise
bool ebg_file_get_code_area(struct ebg_file_public_data *clear_data,
                            struct ebg_file_memory_area** area);


/// find the number of pages required to store all the page tables (master and secondary)
/// for the given model, this assumes MMU in 4K mode and all areas mapped as 4K pages
bool ebg_file_get_page_table_pages(struct ebg_file_public_data *pd, size_t *pages);

#ifdef __cplusplus
}
#endif
