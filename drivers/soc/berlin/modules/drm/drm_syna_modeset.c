// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */

#include "drm_syna_drv.h"
#include <linux/moduleparam.h>
#include <linux/version.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_atomic.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_atomic_helper.h>
#include "syna_vpp.h"

static bool async_flip_enable = true;

module_param(async_flip_enable, bool, 0444);

MODULE_PARM_DESC(async_flip_enable,
		 "Enable support for 'faked' async flipping (default: Y)");
MODULE_LICENSE("Dual MIT/GPL");


/*************************************************************************
 * DRM mode config callbacks
 **************************************************************************/

static struct drm_framebuffer *syna_fb_create(struct drm_device *dev,
					      struct drm_file *file,
					      const struct drm_mode_fb_cmd2
					      *mode_cmd)
{
	struct drm_framebuffer *fb;

	switch (mode_cmd->pixel_format) {
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_NV12:
		break;
	default:
		DRM_ERROR_RATELIMITED
		    ("pixel format not supported (format = %u)\n",
		     mode_cmd->pixel_format);
		return ERR_PTR(-EINVAL);
	}

	if (mode_cmd->flags & DRM_MODE_FB_INTERLACED) {
		DRM_ERROR_RATELIMITED
		    ("interlaced framebuffers not supported\n");
		return ERR_PTR(-EINVAL);
	}

	if (mode_cmd->modifier[0] != DRM_FORMAT_MOD_NONE) {
		DRM_ERROR_RATELIMITED
		    ("format modifier 0x%llx is not supported\n",
		     mode_cmd->modifier[0]);
		return ERR_PTR(-EINVAL);
	}

	fb = drm_gem_fb_create(dev, file, mode_cmd);
	if (IS_ERR(fb))
		goto out;

	DRM_DEBUG_DRIVER("[FB:%d]\n", fb->base.id);

out:
	return fb;
}

static const struct drm_mode_config_funcs syna_mode_config_funcs = {
	.fb_create = syna_fb_create,
	.output_poll_changed = NULL,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static void syna_atomic_commit_tail(struct drm_atomic_state *state)
{
	struct drm_device *dev = state->dev;

	drm_atomic_helper_commit_modeset_disables(dev, state);
	drm_atomic_helper_commit_modeset_enables(dev, state);
	drm_atomic_helper_commit_planes(dev, state,
					DRM_PLANE_COMMIT_ACTIVE_ONLY);
	drm_atomic_helper_commit_hw_done(state);
	drm_atomic_helper_wait_for_vblanks(dev, state);
	drm_atomic_helper_cleanup_planes(dev, state);
}

static struct drm_mode_config_helper_funcs syna_mode_config_helpers = {
	.atomic_commit_tail = syna_atomic_commit_tail,
};

int syna_modeset_early_init(struct syna_drm_private *dev_priv)
{
	struct drm_device *dev = dev_priv->dev;
	int err;
	int index = 0;

	drm_mode_config_init(dev);

	dev->mode_config.funcs = &syna_mode_config_funcs;
	dev->mode_config.min_width = SYNA_WIDTH_MAX / 4;
	dev->mode_config.min_height = SYNA_HEIGHT_MAX / 4;

	dev->mode_config.max_width = SYNA_WIDTH_MAX;
	dev->mode_config.max_height = SYNA_HEIGHT_MAX;

	DRM_INFO("max_width is %d\n", dev->mode_config.max_width);
	DRM_INFO("max_height is %d\n", dev->mode_config.max_height);

	dev->mode_config.fb_base = 0;
	dev->mode_config.async_page_flip = async_flip_enable;
	dev->mode_config.helper_private = &syna_mode_config_helpers;

	DRM_INFO("%s async flip support is %s\n",
		 dev->driver->name, async_flip_enable ? "enabled" : "disabled");

	dev->mode_config.allow_fb_modifiers = true;

	dev_priv->plane = syna_plane_create(dev, DRM_PLANE_TYPE_PRIMARY);
	if (IS_ERR(dev_priv->plane)) {
		DRM_ERROR("failed to create a DRM_PLANE_TYPE_PRIMARY plane\n");
		err = PTR_ERR(dev_priv->plane);
		goto err_config_cleanup;
	}

	dev_priv->crtc = syna_crtc_create(dev, 0, dev_priv->plane);
	if (IS_ERR(dev_priv->crtc)) {
		DRM_ERROR("failed to create a CRTC\n");
		err = PTR_ERR(dev_priv->crtc);
		goto err_config_cleanup;
	}

	index = 1 << drm_crtc_index(dev_priv->crtc);
	dev_priv->overlay_plane =
	    syna_overlay_plane_create(dev, index, DRM_PLANE_TYPE_OVERLAY);
	if (IS_ERR(dev_priv->overlay_plane)) {
		DRM_ERROR("failed to create a DRM_PLANE_TYPE_OVERLAY plane\n");
		err = PTR_ERR(dev_priv->overlay_plane);
		goto err_config_cleanup;
	}

	dev_priv->connector = syna_dvi_connector_create(dev);
	if (IS_ERR(dev_priv->connector)) {
		DRM_ERROR("failed to create a connector\n");
		err = PTR_ERR(dev_priv->connector);
		goto err_config_cleanup;
	}

	dev_priv->encoder = syna_tmds_encoder_create(dev);
	if (IS_ERR(dev_priv->encoder)) {
		DRM_ERROR("failed to create an encoder\n");
		err = PTR_ERR(dev_priv->encoder);
		goto err_config_cleanup;
	}

	err = drm_connector_attach_encoder(dev_priv->connector,
					   dev_priv->encoder);
	if (err) {
		DRM_ERROR
		    ("failed to attach [ENCODER:%d:%s] to [CONNECTOR:%d:%s] (err=%d)\n",
		     dev_priv->encoder->base.id,
		     dev_priv->encoder->name,
		     dev_priv->connector->base.id,
		     dev_priv->connector->name, err);
		goto err_config_cleanup;
	}

	DRM_DEBUG_DRIVER("initialised\n");

	return 0;

err_config_cleanup:
	drm_mode_config_cleanup(dev);

	return err;
}

int syna_modeset_late_init(struct syna_drm_private *dev_priv)
{
	struct drm_device *ddev = dev_priv->dev;

	drm_mode_config_reset(ddev);

	return 0;
}

void syna_modeset_late_cleanup(struct syna_drm_private *dev_priv)
{
	drm_mode_config_cleanup(dev_priv->dev);

	DRM_DEBUG_DRIVER("cleaned up\n");
}
