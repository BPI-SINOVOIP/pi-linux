// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */
#include <drm/drmP.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include "drm_syna_drv.h"
#include "drm_syna_gem.h"
#include "syna_vpp.h"

static void syna_plane_set_surface(struct drm_crtc *crtc, struct drm_plane *plane,
			    struct drm_framebuffer *fb,
			    const uint32_t src_x, const uint32_t src_y)
{
	struct syna_crtc *syna_crtc = to_syna_crtc(crtc);
	struct syna_framebuffer *syna_fb = to_syna_framebuffer(fb);
	unsigned int pitch = fb->pitches[0];
	uint64_t address = syna_gem_get_dev_addr(syna_fb->obj[0]);
	unsigned int planeID = 0;

	if (!syna_crtc || !syna_fb) {
		DRM_ERROR("%s %d  syna crtc or fb is NULL!!\n",
			  __func__, __LINE__);
		return;
	}

	/*
	 * User space specifies 'x' and 'y' and this is used to tell the display
	 * to scan out from part way through a buffer.
	 */
	address += ((src_y * pitch) + (src_x * (syna_drm_fb_cpp(fb))));

	switch (syna_drm_fb_format(fb)) {
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_NV12:
		break;
	default:
		DRM_ERROR("unsupported pixel format (format = %d)\n",
			  syna_drm_fb_format(fb));
		return;
	}

	if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		planeID = VPP_PLANE_GFX;
	} else if (plane->type == DRM_PLANE_TYPE_OVERLAY) {
		planeID = DRM_OVERLAY_PLANE;
	} else {
		DRM_ERROR("unsupported plane type:%d\n", plane->type);
		WARN_ON(1);
		return;
	}

	syna_vpp_set_surface(plane->dev->dev,
			     syna_crtc->syna_reg,
			     planeID,
			     (u64) syna_fb->obj[0],
			     src_x, src_y,
			     fb->width, fb->height, pitch,
			     syna_drm_fb_format(fb), 255, false);
}

static int syna_plane_helper_atomic_check(struct drm_plane *plane,
					  struct drm_plane_state *state)
{
	struct drm_crtc_state *crtc_new_state;

	if (!state->crtc)
		return 0;

	crtc_new_state = drm_atomic_get_new_crtc_state(state->state,
						       state->crtc);

	return drm_atomic_helper_check_plane_state(state, crtc_new_state,
						   DRM_PLANE_HELPER_NO_SCALING,
						   DRM_PLANE_HELPER_NO_SCALING,
						   false, true);
}

static void syna_plane_helper_atomic_update(struct drm_plane *plane,
					    struct drm_plane_state *old_state)
{
	struct drm_plane_state *plane_state = plane->state;
	struct drm_framebuffer *fb = plane_state->fb;

	if (fb) {
		DRM_DEBUG_ATOMIC
		    ("%s:%d %d %d %d %d -> %d %d %d %d  rotation=%d\n",
		     __func__, __LINE__, plane_state->src_x,
		     plane_state->src_y, plane_state->src_w, plane_state->src_h,
		     plane_state->crtc_x, plane_state->crtc_y,
		     plane_state->crtc_w, plane_state->crtc_h,
		     plane_state->rotation);
		syna_plane_set_surface(plane_state->crtc, plane, fb,
				       plane_state->src_x, plane_state->src_y);
	}
}

static const struct drm_plane_helper_funcs syna_plane_helper_funcs = {
	.prepare_fb = drm_gem_fb_prepare_fb,
	.atomic_check = syna_plane_helper_atomic_check,
	.atomic_update = syna_plane_helper_atomic_update,
};

static const struct drm_plane_funcs syna_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_primary_helper_destroy,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

struct drm_plane *syna_plane_create(struct drm_device *dev,
				    enum drm_plane_type type)
{
	struct drm_plane *plane;
	const uint32_t supported_formats[] = {
		DRM_FORMAT_XRGB8888,
		DRM_FORMAT_ARGB8888,
		DRM_FORMAT_XBGR8888,
		DRM_FORMAT_ABGR8888,
		DRM_FORMAT_NV12,
	};
	int err;

	plane = kzalloc(sizeof(*plane), GFP_KERNEL);
	if (!plane) {
		err = -ENOMEM;
		goto err_exit;
	}

	err = drm_universal_plane_init(dev, plane, 0, &syna_plane_funcs,
				       supported_formats,
				       ARRAY_SIZE(supported_formats),
				       NULL, type, "VPP-GFX");
	if (err)
		goto err_plane_free;

	drm_plane_helper_add(plane, &syna_plane_helper_funcs);

	DRM_DEBUG_DRIVER("[PLANE] type:  %d\n", plane->type);
	DRM_DEBUG_DRIVER("[PLANE] index: %d\n", plane->index);
	DRM_DEBUG_DRIVER("[PLANE] name:  %s\n", plane->name);
	DRM_DEBUG_DRIVER("num_total_plane %d\n",
			 dev->mode_config.num_total_plane);

	return plane;

err_plane_free:
	kfree(plane);
err_exit:
	return ERR_PTR(err);
}

struct drm_plane *syna_overlay_plane_create(struct drm_device *dev,
					    int crtc_index,
					    enum drm_plane_type type)
{
	struct drm_plane *plane;
	const uint32_t supported_formats[] = {
		DRM_FORMAT_NV12,
	};
	int err;

	plane = kzalloc(sizeof(*plane), GFP_KERNEL);
	if (!plane) {
		err = -ENOMEM;
		goto err_exit;
	}
	err =
	    drm_universal_plane_init(dev, plane, crtc_index, &syna_plane_funcs,
				     supported_formats,
				     ARRAY_SIZE(supported_formats), NULL, type,
				     "VPP-PIP");

	if (err) {
		DRM_ERROR("%s %d  fail to create plane\n", __func__, __LINE__);
		goto err_plane_free;
	}

	drm_plane_helper_add(plane, &syna_plane_helper_funcs);
	DRM_DEBUG_ATOMIC("%s %d\n", __func__, __LINE__);

	DRM_DEBUG_DRIVER("[PLANE] type:  %d\n", plane->type);
	DRM_DEBUG_DRIVER("[PLANE] index: %d\n", plane->index);
	DRM_DEBUG_DRIVER("[PLANE] name:  %s\n", plane->name);
	DRM_DEBUG_DRIVER("num_total_plane %d\n",
			 dev->mode_config.num_total_plane);

	return plane;

err_plane_free:
	kfree(plane);
err_exit:
	return ERR_PTR(err);
}
