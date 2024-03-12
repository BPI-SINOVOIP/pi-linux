// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2021 Synaptics Incorporated */

#ifndef _UAPI_SYNAP_H
#define _UAPI_SYNAP_H

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 *  magic used for synap.ko ioctls
 */
#define SYNAP_IOCTL 0xBE

/**
 *  parameters for SYNAP_SET_NETWORK_INPUT and SYNAP_SET_NETWORK_OUTPUT
 */
struct synap_set_network_io_data {

    /**
     *  id of the network
     */
    __u32 nid;

    /**
     *  id of the buffer attachment
     */
    __u32 aid;

    /**
     *  index of the network input/output to which the buffer has to be assigned
     */
    __u32 index;

};
struct synap_create_io_buffer_from_dmabuf_data {

    /**
     *  fd of the dmabuf
     */
    __u32 fd;

    /**
     *  size of the area within the SHM area that will be used as buffer
     *
     *  the size must be a multiple of 4K
     */
    __u32 size;

    /**
     *  offset from the start of the dmabuf to use as buffer
     *
     *  the offset must be 4K aligned
     *
     */
    __u32 offset;

    /**
     *  output - bid of the buffer
     */
    __u32 bid;

    /**
     *  output - mem_id of the buffer registration
     */
    __u32 mem_id;
};


/**
 *  SYNAP_CREATE_IO_BUFFER_FROM_DMABUF
 */
struct synap_create_secure_io_buffer_from_dmabuf_data {

    struct synap_create_io_buffer_from_dmabuf_data dmabuf_data;

    /**
     *
     *  buffer security level
     *  1: the buffer should be used as a secure buffer
     *  0: the buffer should be used as non-secure buffer
     *
     */
    __u32 secure;

};


/**
 *  SYNAP_CREATE_IO_BUFFER_FROM_MEM_ID
 */
struct synap_create_io_buffer_from_mem_id_data {

    /**
     *  fd of the dmabuf
     */
    __u32 mem_id;

    /**
     *  size of the area within the SHM area that will be used as buffer
     *
     *  the size must be a multiple of 4K
     */
    __u32 size;

    /**
     *  offset from the start of the dmabuf to use as buffer
     *
     *  the offset must be 4K aligned
     *
     */
    __u32 offset;

    /**
     *  output - bid of the buffer
     */
    __u32 bid;

};

/**
 *  SYNAP_CREATE_IO_BUFFER
 */
struct synap_create_io_buffer_data {

    /**
     *  size of the buffer
     */
    __u32 size;

    /**
     *  output - fd of the buffer
     */
    __u32 fd;

    /**
     *  output - mem_id of the buffer
     */
    __u32 mem_id;

    /**
     *  output - bid of the buffer
     */
    __u32 bid;

};


/**
 *  SYNAP_ATTACH_IO_BUFFER
 */
struct synap_attach_io_buffer_data {

    /**
     *  id of the network for which the buffer will be allocated
     */
    __u32 nid;

    /**
     *  bid of the buffer that needs to be attached
     */
    __u32 bid;

    /**
     *  output - id of the buffer attachment
     */
    __u32 aid;

};

/**
 *  parameters for SYNAP_CREATE_NETWORK
 */
struct synap_create_network_data {

    /**
     *  start address of the network code
     */
    __u64 start;

    /**
     *  size of the network code
     */
    __u64 size;

    /**
     *  output - id of the network
     */
    __u32 nid;

};

/**
 *  parameters about buffer
 */
struct synap_attachment_data {
    /**
     *  id of the network
     */
    __u32 nid;

    /**
     *  id of the attachment
     */
    __u32 aid;
};

/**
 *  associate a registered buffer to the specified input of the network
 */
#define SYNAP_SET_NETWORK_INPUT _IOW(SYNAP_IOCTL, 0, struct synap_set_network_io_data)

/**
 *  associate a registered buffer to the specified output of the network
 */
#define SYNAP_SET_NETWORK_OUTPUT _IOW(SYNAP_IOCTL, 1, struct synap_set_network_io_data)

/**
 *  register memory area within a dmabuf fd as io buffer
 */
#define SYNAP_CREATE_IO_BUFFER_FROM_DMABUF \
    _IOWR(SYNAP_IOCTL, 2, struct synap_create_io_buffer_from_dmabuf_data)

/**
 *  register memory area within a dmabuf fd as io buffer
 */
#define SYNAP_CREATE_SECURE_IO_BUFFER_FROM_DMABUF \
    _IOWR(SYNAP_IOCTL, 3, struct synap_create_secure_io_buffer_from_dmabuf_data)

/**
 *  register mem_id memory area within as io buffer
 */
#define SYNAP_CREATE_IO_BUFFER_FROM_MEM_ID \
    _IOWR(SYNAP_IOCTL, 4, struct synap_create_io_buffer_from_mem_id_data)

/**
 *  destroy a io buffer
 */
#define SYNAP_DESTROY_IO_BUFFER _IOW(SYNAP_IOCTL, 5, __u32)

/**
 *  register memory identified by a TZ mem_id as buffer for the specified network
 */
#define SYNAP_ATTACH_IO_BUFFER _IOWR(SYNAP_IOCTL, 6, struct synap_attach_io_buffer_data)

/**
 *  unregister the specified buffer
 */
#define SYNAP_DETACH_IO_BUFFER _IOW(SYNAP_IOCTL, 7, struct synap_attachment_data)

/**
 *  execute the specified network, the ioctl blocks until the network has finished execution
 */
#define SYNAP_RUN_NETWORK _IOW(SYNAP_IOCTL, 8, __u32)

/**
 *  create a new network that will run the specified code
 */
#define SYNAP_CREATE_NETWORK _IOWR(SYNAP_IOCTL, 9, struct synap_create_network_data)

/**
 *  destroy the specified network and release all the associated resources and buffers
 */
#define SYNAP_DESTROY_NETWORK _IOW(SYNAP_IOCTL, 10, __u32)

/**
 *  lock the hardware for exclusive access
 */
#define SYNAP_LOCK_HARDWARE _IO(SYNAP_IOCTL, 11)

/**
 *  unlock the hardware for exclusive access
 */
#define SYNAP_UNLOCK_HARDWARE _IO(SYNAP_IOCTL, 12)

/**
 *  query the hardware lock status
 */
#define SYNAP_QUERY_HARDWARE_LOCK  _IOR(SYNAP_IOCTL, 13, __u32)

/**
 *  creates a new io buffer
 */
#define SYNAP_CREATE_IO_BUFFER \
    _IOWR(SYNAP_IOCTL, 14, struct synap_create_io_buffer_data)

#endif /* _UAPI_SYNAP_H */
