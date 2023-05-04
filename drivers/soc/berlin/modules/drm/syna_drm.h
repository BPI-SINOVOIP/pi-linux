// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */
#if !defined(__SYNA_DRM_H__)
#define __SYNA_DRM_H__
#include <drm/drm.h>


#define SYNA_VERSION_MAJ               1U
#define SYNA_VERSION_MIN               0U
#define SYNA_VERSION_BUILD             0U

struct drm_syna_gem_create {
	__u64 size;		/* in */
	__u32 flags;		/* in */
	__u32 handle;		/* out */
};

struct drm_syna_gem_mmap {
	__u32 handle;		/* in */
	__u32 pad;
	__u64 offset;		/* out */
};

#define SYNA_GEM_CPU_PREP_READ	(1 << 0)
#define SYNA_GEM_CPU_PREP_WRITE	(1 << 1)
#define SYNA_GEM_CPU_PREP_NOWAIT	(1 << 2)

struct drm_syna_gem_cpu_prep {
	__u32 handle;		/* in */
	__u32 flags;		/* in */
};

struct drm_syna_gem_cpu_fini {
	__u32 handle;		/* in */
	__u32 pad;
};


/*
 * DRM command numbers, relative to DRM_COMMAND_BASE.
 * These defines must be prefixed with "DRM_".
 */
#define DRM_SYNA_GEM_CREATE		0x00
#define DRM_SYNA_GEM_MMAP		0x01
#define DRM_SYNA_GEM_CPU_PREP		0x02
#define DRM_SYNA_GEM_CPU_FINI		0x03

/* These defines must be prefixed with "DRM_IOCTL_". */
#define DRM_IOCTL_SYNA_GEM_CREATE \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_SYNA_GEM_CREATE, \
		 struct drm_syna_gem_create)

#define DRM_IOCTL_SYNA_GEM_MMAP\
	DRM_IOWR(DRM_COMMAND_BASE + DRM_SYNA_GEM_MMAP, \
		 struct drm_syna_gem_mmap)

#define DRM_IOCTL_SYNA_GEM_CPU_PREP \
	DRM_IOW(DRM_COMMAND_BASE + DRM_SYNA_GEM_CPU_PREP, \
		struct drm_syna_gem_cpu_prep)

#define DRM_IOCTL_SYNA_GEM_CPU_FINI \
	DRM_IOW(DRM_COMMAND_BASE + DRM_SYNA_GEM_CPU_FINI, \
		struct drm_syna_gem_cpu_fini)
#endif /* defined(__SYNA_DRM_H__) */
