/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * drivers/staging/android/uapi/ion.h
 *
 * Copyright (C) 2011 Google, Inc.
 */

#ifndef _UAPI_LINUX_ION_H
#define _UAPI_LINUX_ION_H

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * ion_heap_types - list of all possible types of heaps that Android can use
 *
 * @ION_HEAP_TYPE_SYSTEM:        Reserved heap id for ion heap that allocates
 *				 memory using alloc_page(). Also, supports
 *				 deferred free and allocation pools.
* @ION_HEAP_TYPE_DMA:		 Reserved heap id for ion heap that manages
 * 				 single CMA (contiguous memory allocator)
 * 				 region. Uses standard DMA APIs for
 *				 managing memory within the CMA region.
 */
enum ion_heap_type {
	ION_HEAP_TYPE_SYSTEM = 0,
	ION_HEAP_TYPE_DMA = 2,
	/* reserved range for future standard heap types */
	ION_HEAP_TYPE_CUSTOM = 16,
	ION_HEAP_TYPE_BERLIN,
	ION_HEAP_TYPE_BERLIN_NC,
	ION_HEAP_TYPE_BERLIN_SECURE,
	ION_HEAP_TYPE_SYSTEM_CUST,
	ION_HEAP_TYPE_DMA_CUST,
	ION_HEAP_TYPE_MAX = 31,
};

/**
 * ion_heap_id - list of standard heap ids that Android can use
 *
 * @ION_HEAP_SYSTEM		Id for the ION_HEAP_TYPE_SYSTEM
 * @ION_HEAP_DMA_START 		Start of reserved id range for heaps of type
 *				ION_HEAP_TYPE_DMA
 * @ION_HEAP_DMA_END		End of reserved id range for heaps of type
 *				ION_HEAP_TYPE_DMA
 * @ION_HEAP_CUSTOM_START	Start of reserved id range for heaps of custom
 *				type
 * @ION_HEAP_CUSTOM_END		End of reserved id range for heaps of custom
 *				type
 */
enum ion_heap_id {
	ION_HEAP_SYSTEM = (1 << ION_HEAP_TYPE_SYSTEM),
	ION_HEAP_DMA_START = (ION_HEAP_SYSTEM << 1),
	ION_HEAP_DMA_END = (ION_HEAP_DMA_START << 7),
	ION_HEAP_CUSTOM_START = (ION_HEAP_DMA_END << 1),
	ION_HEAP_CUSTOM_END = (ION_HEAP_CUSTOM_START << 22),
};

#define ION_NUM_MAX_HEAPS	(32)

/**
 * allocation flags - the lower 16 bits are used by core ion, the upper 16
 * bits are reserved for use by the heaps themselves.
 */

/*
 * mappings of this buffer should be cached, ion will do cache maintenance
 * when the buffer is mapped for dma
 */
#define ION_FLAG_CACHED			BIT(0)

#define ION_NONCACHED			0
#define ION_CACHED			(ION_FLAG_CACHED)

/**
 * DOC: Ion Userspace API
 *
 * create a client by opening /dev/ion
 * most operations handled via following ioctls
 *
 */

/**
 * struct ion_allocation_data - metadata passed from userspace for allocations
 * @len:		size of the allocation
 * @heap_id_mask:	mask of heap ids to allocate from
 * @flags:		flags passed to heap
 * @handle:		pointer that will be populated with a cookie to use to
 *			refer to this allocation
 *
 * Provided by userspace as an argument to the ioctl
 */
struct ion_allocation_data {
	__u64 len;
	__u32 heap_id_mask;
	__u32 flags;
	__u32 fd;
	__u32 unused;
};

/**
 * struct ion_custom_data - metadata passed to/from userspace for a custom ioctl
 * @cmd:	the custom ioctl function to call
 * @arg:	additional data to pass to the custom ioctl, typically a user
 *		pointer to a predefined structure
 *
 * This works just like the regular cmd and arg fields of an ioctl.
 */

struct ion_custom_data {
	__u32 share_fd;
	__u32 cmd;
	__u64 arg;
};

#define MAX_HEAP_NAME			32

/**
 * struct ion_heap_data - data about a heap
 * @name - first 32 characters of the heap name
 * @type - heap type
 * @heap_id - heap id for the heap
 */
struct ion_heap_data {
	char name[MAX_HEAP_NAME];
	__u32 type;
	__u32 heap_id;
	__u32 reserved0;
	__u32 reserved1;
	__u32 reserved2;
};

/**
 * struct ion_heap_query - collection of data about all heaps
 * @cnt - total number of heaps to be copied
 * @heaps - buffer to copy heap data
 */
struct ion_heap_query {
	__u32 cnt; /* Total number of heaps to be copied */
	__u32 reserved0; /* align to 64bits */
	__u64 heaps; /* buffer to be populated */
	__u32 reserved1;
	__u32 reserved2;
};

#define HEAP_INFO_NAME_LEN		16

struct ion_heap_info {
	__u32 type;
	char name[HEAP_INFO_NAME_LEN];
	__u32 id;
	__u32 base;
	__u32 size;
	__u32 attribute;
};

#define ION_IOC_MAGIC		'I'

/**
 * DOC: ION_IOC_ALLOC - allocate memory
 *
 * Takes an ion_allocation_data struct and returns it with the handle field
 * populated with the opaque handle for the allocation.
 */
#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, \
				      struct ion_allocation_data)

/**
 * DOC: ION_IOC_CUSTOM - call architecture specific ion ioctl
 *
 * Takes the argument of the architecture specific ioctl to call and
 * passes appropriate userdata for that ioctl
 */
#define ION_IOC_CUSTOM		_IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)

/**
 * DOC: ION_IOC_HEAP_QUERY - information about available heaps
 *
 * Takes an ion_heap_query structure and populates information about
 * available Ion heaps.
 */
#define ION_IOC_HEAP_QUERY     _IOWR(ION_IOC_MAGIC, 8, \
					struct ion_heap_query)

/**
 * DOC: ION_IOC_HEAP_ABI_VERSION - return ABI version
 *
 * Returns ABI version for this driver
 */
#define ION_IOC_ABI_VERSION    _IOR(ION_IOC_MAGIC, 9, \
					__u32)

struct ion_berlin_data {
	__u32 cmd;
	__u32 offset;
	__u64 addr;
	__u64 len;
};

#define ION_INVALIDATE_CACHE		1
#define ION_CLEAN_CACHE			2
#define ION_FLUSH_CACHE			3

enum berlin_custom_command {
	ION_BERLIN_PHYS = 0,
	ION_BERLIN_SYNC = 1,
	ION_BERLIN_GETHM = 4,
	ION_BERLIN_GETHI = 5,
};

/* The ion memory pool attribute flag bits */
#define ION_A_FS                0x0001      /* For secure memory */
#define ION_A_NS                0x0002      /* For non-secure memory */
#define ION_A_FC                0x0004      /* For cacheable memory */
#define ION_A_NC                0x0008      /* For non-cacheable memory */
#define ION_A_FD                0x0010      /* For dynamic memory */
#define ION_A_ND                0x0020      /* For static memory */
#define ION_A_CC                0x0100      /* For control (class) memory */
#define ION_A_CV                0x0200      /* For video (class) memory */
#define ION_A_CG                0x0400      /* For graphics (class) memory */
#define ION_A_CO                0x0800      /* For other (class) memory */

#endif /* _UAPI_LINUX_ION_H */
