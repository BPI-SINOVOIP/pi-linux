#pragma once

#ifndef __KERNEL__
#include <stdint.h>
#endif

#define SYNAP_TA_PAGE_SIZE 4096

/**
  *  UUID of the TA
  */
#define SYNAP_TA_UUID {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x2F}}

#define SYNAP_TA_DRIVER_BUFFER_SIZE 32768
#define SYNAP_TA_DRIVER_BUFFER_ALIGNEMENT 4096

/**
 *   Open session parameters: when the CA opens a session it must pass the following parameters to
 *   the TA:
 *
 *    PARAM 0: TEEC_MEMREF_TEMP_INPUT driver buffers - array of two struct synap_ta_memory_area
 *             first is used by driver in non-secure mode, second in secure mode these buffers must
 *             be available while the session is open. The two buffers must be contiguous, have
 *             size SYNAP_TA_DRIVER_BUFFER_SIZE and alignement SYNAP_TA_DRIVER_BUFFER_ALIGNEMENT.
 *
 */

/**
 *  command codes used to invoke the TA
 */
enum {
    /**
      *  create a new network with the given code
      *
      *  PARAM 0 : TEEC_MEMREF_TEMP_INPUT network code, this buffer can be released after the call
      *  PARAM 1 : TEEC_MEMREF_TEMP_INPUT resources used to run the network -
      *            struct synap_ta_network_resources, the areas listed must be available until
      *            SYNAP_TA_CMD_DESTROY_NETWORK is called
      *  PARAM 2 : TEEC_VALUE_OUTPUT a = nid
      *
      */
    SYNAP_TA_CMD_CREATE_NETWORK = 0,

    /**
      *  destroy the given network
      *
      *  PARAM 0 : TEEC_VALUE_INPUT a = nid
      *
      */
    SYNAP_TA_CMD_DESTROY_NETWORK = 1,

    /**
      *  attach the given memory for use with the given network
      *
      *  PARAM 0 : TEEC_VALUE_INPUT a = bid of buffer used as input/output for the network,
      *                                 the corresponding buffer has to be available until
      *                                 SYNAP_TA_CMD_DETACH_BUFFER is called or
      *                                 the corresponding network is destroyed with
      *                                 SYNAP_TA_CMD_DESTROY_NETWORK
      *                             b = nid
      *  PARAM 1 : TEEC_VALUE_OUTPUT a = aid
      *
      *
      */
    SYNAP_TA_CMD_ATTACH_IO_BUFFER = 2,

    /**
      *  detach memory from network
      *
      *  PARAM 0 : TEEC_VALUE_INPUT a = nid, b = aid
      *
      */
    SYNAP_TA_CMD_DETACH_IO_BUFFER = 3,

    /**
      *  set the n-th input of the network to the given buffer
      *
      *  PARAM 0 : TEEC_VALUE_INPUT a = nid , b = aid
      *  PARAM 1 : TEEC_VALUE_INPUT a = input index
      *
      */
    SYNAP_TA_CMD_SET_INPUT = 4,

    /**
      *  set the n-th output of the network to the given buffer
      *
      *  PARAM 0 : TEEC_VALUE_INPUT a = nid , b = aid
      *  PARAM 1 : TEEC_VALUE_INPUT a = output index
      *
      */
    SYNAP_TA_CMD_SET_OUTPUT = 5,

    /**
      *  start execution of the given network, activates the NPU in the correct mode if necessary
      *
      *  PARAM 0 : TEEC_VALUE_INPUT a = nid
      *
      */
    SYNAP_TA_CMD_START_NETWORK = 6,

    /**
      *  ceates a buffer using the given physical memory
      *
      *  PARAM 0 : TEEC_MEMREF_TEMP_INPUT array of struct synap_ta_memory_area, the corresponding
      *            buffer will be in use until SYNAP_TA_CMD_DESTROY_IO_BUFFER is called
      *  PARAM 1 : TEEC_VALUE_INPUT a = 0 : buffer must be used as non-secure buffer, 1: secure
      *  PARAM 2 : TEEC_VALUE_OUTPUT a = bid, b = mem_id (if sg is in system memory)
      *
      */
    SYNAP_TA_CMD_CREATE_IO_BUFFER_FROM_SG = 7,


    /**
      *  ceates a buffer using the given existing mem_id
      *
      *  PARAM 0 : TEEC_VALUE_INPUT a = mem_id : the corresponding buffer will be in use until
      *                             SYNAP_TA_CMD_DESTROY_IO_BUFFER is called
      *  PARAM 1 : TEEC_VALUE_INPUT a = offset, b = size
      *  PARAM 2 : TEEC_VALUE_OUTPUT a = bid
      *
      */
    SYNAP_TA_CMD_CREATE_IO_BUFFER_FROM_MEM_ID = 8,


    /**
      *  destroys the buffer that was previous created
      *
      *  PARAM 0 : TEEC_VALUE_INPUT a = mem_id of the buffer to release
      *
      *  NOTE: only buffers that are attached to no network can be destroyed, otherwise the call
      *  will fail
      *
      */
    SYNAP_TA_CMD_DESTROY_IO_BUFFER = 9,


    /**
      *  read the interrupt register of the NPU and stop execution on the npu
      *
      *  PARAM 0 : TEEC_VALUE_OUTPUT a = register value
      *
      */
    SYNAP_TA_CMD_READ_INTERRUPT_REGISTER = 10,

    /**
      *  activate the NPU
      *
      *  PARAM 0 : TEEC_VALUE_INPUT a = 1: secure mode 0: non-secure
      *
      */
    SYNAP_TA_CMD_ACTIVATE_NPU = 11,

    /**
      *  shuts down the NPU
      *
      */
    SYNAP_TA_CMD_DEACTIVATE_NPU = 12,

    /**
      *
      * dump the state of the NPU to serial console. Available only in debug builds.
      *
      */
    SYNAP_TA_CMD_DUMP_STATE = 13

};


struct synap_ta_memory_area {
    /** physical address of the start of the memory area,
     *  must be a multiple of SYNAP_TA_PAGE_SIZE */
    uint64_t addr;

    /** numer of pages of the area */
    uint64_t npage;
};

struct synap_ta_network_resources {
    /** number of areas in the areas array assigned to code */
    uint32_t code_areas_count;

    /** number of areas in the areas array assigned to pool */
    uint32_t pool_areas_count;

    /** number of areas in the areas array assigned to the page table */
    uint32_t page_table_areas_count;

    /** number of areas profile operations */
    uint32_t profile_operation_areas_count;

    /** areas, first code_areas_count are for code, then pool_areas_count are for pool,
     *  finally page_table_areas_count areas are for the page table
     */
    struct synap_ta_memory_area areas[];
};
