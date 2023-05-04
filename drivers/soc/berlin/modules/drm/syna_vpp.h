// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */
#if !defined(__SYNA_VPP_H__)
#define __SYNA_VPP_H__

#include <linux/device.h>
#include <linux/types.h>
#include "drm_syna_gem.h"

extern long device_rotate;
extern int nonSecurePoolID;

#define VPP_PLANE_GFX            2
#define VPP_PLANE_PIP            1
#define VPP_PLANE_MAIN           0

#ifdef USE_VS680
#define DRM_OVERLAY_PLANE        VPP_PLANE_PIP
#else
#define DRM_OVERLAY_PLANE        VPP_PLANE_MAIN
#endif


#define SYNA_WIDTH_MAX           1920
#define SYNA_HEIGHT_MAX          1080

bool syna_vpp_clocks_set(struct device *dev,
			 void __iomem *syna_reg, u32 clock_in_mhz,
			 u32 hdisplay, u32 vdisplay);

void syna_vpp_set_updates_enabled(struct device *dev, void __iomem *syna_reg,
				  bool enable);

void syna_vpp_set_syncgen_enabled(struct device *dev, void __iomem *syna_reg,
				  bool enable);

void syna_vpp_set_powerdwn_enabled(struct device *dev, void __iomem *syna_reg,
				   bool enable);

void syna_vpp_set_vblank_enabled(struct device *dev, void __iomem *syna_reg,
				 bool enable);

bool syna_vpp_check_and_clear_vblank(struct device *dev,
				     void __iomem *syna_reg);

void syna_vpp_set_plane_enabled(struct device *dev, void __iomem *syna_reg,
				u32 plane, bool enable);

void syna_vpp_reset_planes(struct device *dev, void __iomem *syna_reg);

void syna_vpp_set_surface(struct device *dev, void __iomem *syna_reg,
			  u32 plane, u64 address,
			  u32 posx, u32 posy,
			  u32 width, u32 height, u32 stride,
			  u32 format, u32 alpha, bool blend);

void syna_vpp_mode_set(struct device *dev, void __iomem *syna_reg,
		       u32 h_display, u32 v_display,
		       u32 hbps, u32 ht, u32 has,
		       u32 hlbs, u32 hfps, u32 hrbs,
		       u32 vbps, u32 vt, u32 vas,
		       u32 vtbs, u32 vfps, u32 vbbs, bool nhsync, bool nvsync);

void syna_vpp_exit(void);
void syna_vpp_wait_vsync(void);

#endif /* __SYNA_VPP_H__ */
