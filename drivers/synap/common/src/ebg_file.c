// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#include <ebg_file.h>

#ifdef SYNAP_TA
#include <synap_ta_log.h>
#define EBG_FILE_LOG_ENTER() SYNAP_TA_LOG_FUN(LOG_TYPE_ENTER, __func__, __LINE__, "entering")
#define EBG_FILE_LOG_SUCCESS() SYNAP_TA_LOG_FUN(LOG_TYPE_EXIT_SUCCESS, __func__, __LINE__, "exit with success")
#define EBG_FILE_LOG_FAILURE() SYNAP_TA_LOG_FUN(LOG_TYPE_EXIT_FAIL, __func__, __LINE__, "exit with error")
#define EBG_FILE_LOG(format, ...) SYNAP_TA_LOG_FUN(LOG_TYPE_INFO, __func__, __LINE__, format, ##__VA_ARGS__)
#elif defined(__KERNEL__)
#include "synap_kernel_log.h"
#define EBG_FILE_LOG_ENTER() KLOGI("%s():%d entering", __func__, __LINE__)
#define EBG_FILE_LOG_SUCCESS() KLOGI("%s():%d exit with success", __func__, __LINE__)
#define EBG_FILE_LOG_FAILURE() KLOGI("%s():%d exit with failure", __func__, __LINE__)
#define EBG_FILE_LOG(format, ...) KLOGI("%s():%d " format, __func__, __LINE__, ##__VA_ARGS__)
#elif defined(NDEBUG)
#include <stdio.h>
#define EBG_FILE_LOG_ENTER()
#define EBG_FILE_LOG_SUCCESS()
#define EBG_FILE_LOG_FAILURE() printf("%s():%d exit with failure\n", __func__, __LINE__)
#define EBG_FILE_LOG(format, ...)
#else
#include <stdio.h>
#define EBG_FILE_LOG_ENTER() printf("%s():%d entering\n", __func__, __LINE__)
#define EBG_FILE_LOG_SUCCESS() printf("%s():%d exit with success\n", __func__, __LINE__)
#define EBG_FILE_LOG_FAILURE() printf("%s():%d exit with failure\n", __func__, __LINE__)
#define EBG_FILE_LOG(format, ...) printf("%s():%d " format "\n", __func__, __LINE__, ##__VA_ARGS__)
#endif

enum {
    EBG_MEMORY_NONE = 0,
    EBG_MEMORY_POOL = 1,
    EBG_MEMORY_AXI_SRAM = 2,
    EBG_MEMORY_VIP_SRAM = 3,
    EBG_MEMORY_CODE = 4,
    EBG_MEMORY_INIT_CODE = 5,
    EBG_MEMORY_COEF = 6,
    EBG_MEMORY_VSI_RESERVE = 7,
    EBG_MEMORY_INPUT = 8,
    EBG_MEMORY_OUTPUT = 9,
};


static uint8_t *read_uint8_arr(uint8_t *addr, uint8_t *arr, size_t len) {

    size_t i;
    for (i = 0; i < len ; i++) {
        arr[i] = addr[i];
    }

    return addr + len;
}

static uint8_t *read_uint32(uint8_t *addr, uint32_t *data) {
    *data = addr[0] + (addr[1] << 8) + (addr[2] << 16) + (addr[3] << 24);
    return addr + 4;
}

uint8_t *read_area(uint8_t *addr, struct ebg_file_memory_area *area) {

    uint8_t *p = addr;

    p = read_uint32(p, &(area->type));
    p = read_uint32(p, &(area->address));
    p = read_uint32(p, &(area->size));
    p = read_uint32(p, &(area->alignment));
    p = read_uint32(p, &(area->page_size));

    return p;

}

bool read_header(uint8_t *addr, struct ebg_file_header *header, uint8_t **next) {

    uint8_t *p = addr;

    EBG_FILE_LOG_ENTER();

    p = read_uint8_arr(p, header->magic, 4);
    p = read_uint32(p, &(header->endianness));
    p = read_uint32(p, &(header->version));
    p = read_uint32(p, &(header->security_type));
    p = read_uint32(p, &(header->security_info_length));
    p = read_uint32(p, &(header->metadata_length));
    p = read_uint32(p, &(header->vsi_data_length));
    p = read_uint32(p, &(header->padding_length));
    p = read_uint32(p, &(header->code_length));

    if (header->magic[0] != 'E' || header->magic[1] != 'B' ||
            header->magic[2] != 'G' || header->magic[3] != 'X' ) {
        EBG_FILE_LOG("expected magic 0x%08x, found 0x%08x", *((uint32_t *) "EBGX"),
                     *((uint32_t *) header->magic));
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    if ((header->endianness & 0xFF) != 0x0) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    if (header->version != 0x1) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    if (header->security_type != 0x00 && header->security_type != 0x01 ) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    EBG_FILE_LOG("header with:");
    EBG_FILE_LOG("security_info_length %d", header->security_info_length);
    EBG_FILE_LOG("metadata_length %d", header->metadata_length);
    EBG_FILE_LOG("vsi_data_length %d", header->vsi_data_length);
    EBG_FILE_LOG("padding_length %d", header->padding_length);
    EBG_FILE_LOG("code_length %d", header->code_length);

    *next = p;

    EBG_FILE_LOG_SUCCESS();

    return true;
}

uint8_t *read_metadata(uint8_t *addr, struct ebg_file_metadata *metadata) {

    uint8_t *p = addr;

    p = read_uint32(p, &(metadata->hardware_type));
    p = read_uint8_arr(p, metadata->network_name, 64);
    p = read_uint32(p, &(metadata->compiler_id));
    p = read_uint32(p, &(metadata->compiler_version));
    p = read_uint32(p, &(metadata->execution_mode));
    p = read_uint32(p, &(metadata->input_count));
    p = read_uint32(p, &(metadata->output_count));
    p = read_uint32(p, &(metadata->memory_area_count));

    return p;
}

bool ebg_file_is_ebg(uint8_t *ebg, size_t len) {
    EBG_FILE_LOG_ENTER();

    if (len < 4) {
        return false;
    }

    EBG_FILE_LOG_SUCCESS();

    return ebg[0] == 'E' && ebg[1] == 'B' && ebg[2] == 'G' && ebg[3] == 'X';

}

bool ebg_file_header_parse(uint8_t *ebg, size_t len, struct ebg_file_header* header) {

    uint8_t *p = ebg;

    EBG_FILE_LOG_ENTER();

    if (EBG_FILE_HEADER_SIZE > len) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    if (!read_header(p, header, &p)) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    EBG_FILE_LOG_SUCCESS();

    return true;
}


bool ebg_file_public_data_parse(uint8_t *ebg, size_t len, struct ebg_file_public_data* public_data) {

    size_t i;
    uint8_t *p = ebg;
    uint64_t computed_clear_length = 0;
    uint64_t tot_areas = 0;

    EBG_FILE_LOG_ENTER();

    if (p + EBG_FILE_HEADER_SIZE > ebg + len) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    // parse the header

    if (!read_header(p, &(public_data->header), &p)) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    computed_clear_length = public_data->header.padding_length +
                               public_data->header.vsi_data_length +
                               public_data->header.metadata_length +
                               public_data->header.security_info_length +
                               EBG_FILE_HEADER_SIZE;

    if (len < computed_clear_length) {
        EBG_FILE_LOG("expected at least %d bytes, received %d",
                     (uint32_t) computed_clear_length, (uint32_t) len);
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    // skip the security header
    p += public_data->header.security_info_length;

    if (p + EBG_FILE_METADATA_SIZE > ebg + len) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    // parse the metadata header

    p = read_metadata(p, &(public_data->metadata));

    tot_areas = public_data->metadata.input_count +
                         public_data->metadata.output_count +
                         public_data->metadata.memory_area_count;

    if ( tot_areas > EBG_FILE_MAX_AREAS) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    if (tot_areas * EBG_FILE_MEMORY_AREA_SIZE + EBG_FILE_METADATA_SIZE !=
            public_data->header.metadata_length) {
        EBG_FILE_LOG("metadata.input_count: %u", (unsigned)public_data->metadata.input_count);
        EBG_FILE_LOG("metadata.output_count: %u", (unsigned)public_data->metadata.output_count);
        EBG_FILE_LOG("metadata.memory_area_count: %u", (unsigned)public_data->metadata.memory_area_count);
        EBG_FILE_LOG("tot_areas: %u", (unsigned)tot_areas);
        EBG_FILE_LOG("computed size: %u", (unsigned)tot_areas * EBG_FILE_MEMORY_AREA_SIZE + EBG_FILE_METADATA_SIZE);
        EBG_FILE_LOG("size in header: %u", (unsigned)public_data->header.metadata_length);
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    // parse the areas metadata

    for (i = 0; i < tot_areas; i++) {
        p = read_area(p, &(public_data->area[i]));
    }

    // validate the memory areas
    EBG_FILE_LOG("memory areas %d", public_data->metadata.memory_area_count);
    EBG_FILE_LOG("inputs %d", public_data->metadata.input_count);
    EBG_FILE_LOG("outputs %d", public_data->metadata.output_count);

    for (i = 0; i < public_data->metadata.memory_area_count; i++) {
        struct ebg_file_memory_area *hdr;

        if (!ebg_file_get_memory_area(public_data, i, &hdr)) {
            EBG_FILE_LOG_FAILURE();
            return false;
        }

        if (hdr->type > 7) {
            EBG_FILE_LOG_FAILURE();
            return false;
        }

        // FIXME: should we check other fields?

    }

    for (i = 0; i < public_data->metadata.input_count; i++) {
        struct ebg_file_memory_area *hdr;

        if (!ebg_file_get_input_area(public_data, i, &hdr)) {
            EBG_FILE_LOG_FAILURE();
            return false;
        }

        if (hdr->type != 8) {
            EBG_FILE_LOG_FAILURE();
            return false;
        }

        // FIXME: should we check other fields?

    }

    for (i = 0; i < public_data->metadata.output_count; i++) {
        struct ebg_file_memory_area *hdr;

        if (!ebg_file_get_output_area(public_data, i, &hdr)) {
            EBG_FILE_LOG_FAILURE();
            return false;
        }

        if (hdr->type != 9) {
            EBG_FILE_LOG_FAILURE();
            return false;
        }

        // FIXME: should we check other fields?

    }

    public_data->public_data_len = computed_clear_length;
    public_data->public_data = ebg;

    EBG_FILE_LOG_SUCCESS();

    return true;

}



bool ebg_file_get_memory_area(struct ebg_file_public_data* public_data, uint32_t idx,
                              struct ebg_file_memory_area** area) {

    EBG_FILE_LOG_ENTER();

    if (idx > public_data->metadata.memory_area_count) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    *area = &(public_data->area[idx]);

    EBG_FILE_LOG_SUCCESS();

    return true;

}


bool ebg_file_get_input_area(struct ebg_file_public_data* public_data, uint32_t idx,
                                          struct ebg_file_memory_area** area) {

    EBG_FILE_LOG_ENTER();

    if (idx > public_data->metadata.input_count) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    *area = &(public_data->area[idx + public_data->metadata.memory_area_count]);

    EBG_FILE_LOG_SUCCESS();

    return true;

}

bool ebg_file_get_output_area(struct ebg_file_public_data* public_data, uint32_t idx,
                                    struct ebg_file_memory_area** area) {

    EBG_FILE_LOG_ENTER();

    if (idx > public_data->metadata.output_count) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    *area = &(public_data->area[idx + public_data->metadata.memory_area_count + public_data->metadata.input_count]);

    EBG_FILE_LOG_SUCCESS();

    return true;
}

uint8_t* ebg_file_vsi_data_parse(struct ebg_file_public_data *clear_data,
                             struct ebg_file_vsi_data* vsi_data) {
    uint8_t *p;

    size_t data_offset;

    data_offset = EBG_FILE_HEADER_SIZE + clear_data->header.security_info_length +
            clear_data->header.metadata_length;

    vsi_data->data = clear_data->public_data + data_offset;
    vsi_data->data_len = clear_data->header.vsi_data_length;

    p = vsi_data->data;

    if (vsi_data->data_len < sizeof(uint32_t) * 4) {
        EBG_FILE_LOG_FAILURE();
        return NULL;
    }

    p = read_uint32(p, &(vsi_data->init_realloc_count));
    p = read_uint32(p, &(vsi_data->code_realloc_count));
    p = read_uint32(p, &(vsi_data->layer_count));
    p = read_uint32(p, &(vsi_data->operation_count));

    if (sizeof(uint32_t) * 4 +
        (vsi_data->init_realloc_count + vsi_data->code_realloc_count) * EBG_FILE_REALLOC_TABLE_SIZE +
        vsi_data->layer_count * EBG_FILE_LAYER_TABLE_SIZE +
        vsi_data->operation_count * EBG_FILE_OPERATION_TABLE_SIZE != vsi_data->data_len) {

        EBG_FILE_LOG_FAILURE();
        return NULL;
    }

    return p;
}

bool ebg_file_get_layers(struct ebg_file_public_data *clear_data,
                             struct ebg_file_layer *layers,
                             size_t layer_count) {

    uint32_t i;

    struct ebg_file_vsi_data vsi_data;
    uint8_t *p;

    if ((p = ebg_file_vsi_data_parse(clear_data, &vsi_data)) == NULL) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    if (vsi_data.layer_count < 1) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    if (layer_count != vsi_data.layer_count) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    // skip realloc_table
    p += (vsi_data.init_realloc_count + vsi_data.code_realloc_count) * EBG_FILE_REALLOC_TABLE_SIZE;

    for (i = 0; i < vsi_data.layer_count; i++) {
        p = read_uint32(p, &(layers[i].uid));
        p = read_uint8_arr(p, layers[i].layer_name, 64);
    }
    return true;
}

bool ebg_file_get_operations(struct ebg_file_public_data *clear_data,
                             struct ebg_file_operation *operations,
                             size_t operation_count) {

    uint32_t i;

    struct ebg_file_vsi_data vsi_data;
    uint8_t *p;
    if ((p = ebg_file_vsi_data_parse(clear_data, &vsi_data)) == NULL) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    if (vsi_data.operation_count < 1) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    if (operation_count != vsi_data.operation_count) {
        EBG_FILE_LOG_FAILURE();
        return false;
    }

    // skip realloc_table and layer table
    p += (vsi_data.init_realloc_count + vsi_data.code_realloc_count) * EBG_FILE_REALLOC_TABLE_SIZE
        + vsi_data.layer_count * EBG_FILE_LAYER_TABLE_SIZE;

    for (i = 0; i < vsi_data.operation_count; i++) {
        p = read_uint32(p, &(operations[i].layer_idx));
        p = read_uint32(p, &(operations[i].offset));
        p = read_uint32(p, &(operations[i].size));
        p = read_uint32(p, &(operations[i].type));
    }
    return true;
}

static bool get_area_by_type(struct ebg_file_public_data *clear_data,
                            struct ebg_file_memory_area** area, uint32_t type) {
    size_t i;

    EBG_FILE_LOG_ENTER();

    EBG_FILE_LOG("looking for area of type %d", type);

    for (i = 0; i < clear_data->metadata.memory_area_count; i++) {

        if (clear_data->area[i].type == type) {
            *area = &(clear_data->area[i]);
            EBG_FILE_LOG_SUCCESS();
            return true;
        }

    }

    EBG_FILE_LOG_FAILURE();
    return false;

}

bool ebg_file_get_pool_area(struct ebg_file_public_data *clear_data,
                            struct ebg_file_memory_area** area) {
    return get_area_by_type(clear_data, area, EBG_MEMORY_POOL);
}


bool ebg_file_get_code_area(struct ebg_file_public_data *clear_data,
                            struct ebg_file_memory_area** area) {
    return get_area_by_type(clear_data, area, EBG_MEMORY_CODE);
}

// total number of entries in the MTBL, i.e. total number of STBL that may exist
#define NPU_MTBL_ENTRIES_COUNT 1024

// number of entries in a STBL that uses 4k pages when MMU is in 4k mode
#define NPU_STBL_ENTRIES_COUNT 1024

// memory area covered by a STBL that uses 4k pages when MMU is in 4k mode
#define NPU_STBL_AREA_SIZE ( ( 1024 * 4 ) * NPU_STBL_ENTRIES_COUNT )

#ifdef PRECISE_PAGE_COUNT
bool ebg_file_get_page_table_pages(struct ebg_file_public_data *pd, size_t *pages) {

    size_t i;

    // this array is used to keep track which STBL are in use
    u8 mtbl[NPU_MTBL_ENTRIES_COUNT] = {0};

    /* memset(mtbl, 0, sizeof (u8) * NPU_MTBL_ENTRIES_COUNT); */

    // we need one page for the MTBL
    *pages = 1;

    // iterate over all the areas used by the binary graph
    for (i = 0; i < pd->metadata.memory_area_count +
         pd->metadata.input_count + pd->metadata.output_count; i++) {

        s64 remaining_size = 0;
        u32 start_address = 0;
        u32 mtbl_idx = 0;

        struct ebg_file_memory_area *area;

        if (i < pd->metadata.memory_area_count) {
            if (!ebg_file_get_memory_area(pd, i, &area)) {
                return false;
            }
        } else if (i < pd->metadata.memory_area_count + pd->metadata.input_count) {
            if (!ebg_file_get_input_area(pd, i - pd->metadata.memory_area_count, &area)) {
                return false;
            }
        } else {
            if (!ebg_file_get_output_area(pd, i - (pd->metadata.memory_area_count +
                                                   pd->metadata.input_count), &area)) {
                return false;
            }
        }

        KLOGI("checking area %d of type %d - size %d, start address %d", i, area->type,
              area->size, area->address);

        remaining_size = area->size;
        start_address = area->address;

        // find the start address of the STBL that contain the area start address
        if (start_address % NPU_STBL_AREA_SIZE != 0) {
            start_address -= start_address % NPU_STBL_AREA_SIZE;
            remaining_size += start_address % NPU_STBL_AREA_SIZE;
            KLOGI("need to align start to STBL start, new address %d new size %d",
                  start_address, remaining_size);
        }

        // find all the STBL that are used to map the area
        while (remaining_size > 0) {

            // find the STBL that covers the current start address
            mtbl_idx = start_address / NPU_STBL_AREA_SIZE;

            KLOGI("checking area %d", mtbl_idx);

            // in case the STBL is not in use, mark it in use and increase the number of
            // pages we need to store the page tables
            if (mtbl[mtbl_idx] == 0) {
                KLOGI("marking use of area %d", mtbl_idx);
                mtbl[mtbl_idx] = 1;
                (*pages)++;
            }

            remaining_size -= NPU_STBL_AREA_SIZE;
            start_address += NPU_STBL_AREA_SIZE;
        }

    }

    // FIXME: do we need additional entries in the page table for the reset sequence?

    return true;

}
#else
bool ebg_file_get_page_table_pages(struct ebg_file_public_data *pd, size_t *pages) {

    size_t i;

    // we need one page for the MTBL
    *pages = 1;

    // iterate over all the areas used by the binary graph
    for (i = 0; i < pd->metadata.memory_area_count +
         pd->metadata.input_count + pd->metadata.output_count; i++) {

        struct ebg_file_memory_area *area;

        if (i < pd->metadata.memory_area_count) {
            if (!ebg_file_get_memory_area(pd, i, &area)) {
                return false;
            }
        } else if (i < pd->metadata.memory_area_count + pd->metadata.input_count) {
            if (!ebg_file_get_input_area(pd, i - pd->metadata.memory_area_count, &area)) {
                return false;
            }
        } else {
            if (!ebg_file_get_output_area(pd, i - (pd->metadata.memory_area_count +
                                                   pd->metadata.input_count), &area)) {
                return false;
            }
        }

        *pages += area->size / NPU_STBL_AREA_SIZE + 1;

        if (area->size % NPU_STBL_AREA_SIZE != 0) {
            *pages += 1;
        }

    }

    return true;

}
#endif
