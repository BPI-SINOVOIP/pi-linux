// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */

#include <linux/moduleparam.h>
#include <linux/version.h>
#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include "drm_syna_drv.h"

struct syna_mode_data {
	int hdisplay;
	int vdisplay;
	int vrefresh;
	bool reduced_blanking;
	bool interlaced;
	bool margins;
};

static const struct syna_mode_data syna_extra_modes[] = {
	{
	 .hdisplay = 1280,
	 .vdisplay = 720,
	 .vrefresh = 60,
	 .reduced_blanking = false,
	 .interlaced = false,
	 .margins = false,
	 },
	{
	 .hdisplay = 1920,
	 .vdisplay = 1080,
	 .vrefresh = 60,
	 .reduced_blanking = false,
	 .interlaced = false,
	 .margins = false,
	 },
	{
	 .hdisplay = 960,
	 .vdisplay = 540,
	 .vrefresh = 60,
	 .reduced_blanking = false,
	 .interlaced = false,
	 .margins = false,
	 },
};

static char preferred_mode_name[DRM_DISPLAY_MODE_LEN] = "\0";

module_param_string(dvi_preferred_mode,
		    preferred_mode_name, DRM_DISPLAY_MODE_LEN, 0444);

MODULE_PARM_DESC(dvi_preferred_mode,
		 "Specify the preferred mode (if supported), e.g. 1280x1024.");
MODULE_LICENSE("Dual MIT/GPL");

static int syna_dvi_add_extra_modes(struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	int num_modes;
	int i;

	for (i = 0, num_modes = 0; i < ARRAY_SIZE(syna_extra_modes); i++) {
		mode = drm_cvt_mode(connector->dev,
				    syna_extra_modes[i].hdisplay,
				    syna_extra_modes[i].vdisplay,
				    syna_extra_modes[i].vrefresh,
				    syna_extra_modes[i].reduced_blanking,
				    syna_extra_modes[i].interlaced,
				    syna_extra_modes[i].margins);
		if (mode) {
			drm_mode_probed_add(connector, mode);
			num_modes++;
		}
	}

	return num_modes;
}

static int syna_dvi_connector_helper_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	int num_modes;
	int len = strlen(preferred_mode_name);

	if (len)
		DRM_DEBUG_DRIVER("detected dvi_preferred_mode=%s\n",
				 preferred_mode_name);
	else
		DRM_DEBUG_DRIVER("no dvi_preferred_mode\n");

	num_modes = drm_add_modes_noedid(connector,
					 dev->mode_config.max_width,
					 dev->mode_config.max_height);

	num_modes += syna_dvi_add_extra_modes(connector);
	if (num_modes) {
		struct drm_display_mode *pref_mode = NULL;

		if (len) {
			struct drm_display_mode *mode;
			struct list_head *entry;

			list_for_each(entry, &connector->probed_modes) {
				mode = list_entry(entry,
						  struct drm_display_mode,
						  head);
				if (!strcmp(mode->name, preferred_mode_name)) {
					pref_mode = mode;
					break;
				}
			}
		}

		if (pref_mode)
			pref_mode->type |= DRM_MODE_TYPE_PREFERRED;
		else
			drm_set_preferred_mode(connector,
					       dev->mode_config.max_width,
					       dev->mode_config.max_height);
	}

	drm_mode_sort(&connector->probed_modes);

	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s] found %d modes\n",
			 connector->base.id, connector->name, num_modes);

	return num_modes;
}

static int syna_dvi_connector_helper_mode_valid(struct drm_connector *connector,
						struct drm_display_mode *mode)
{
	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		return MODE_NO_INTERLACE;
	else if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
		return MODE_NO_DBLESCAN;

	switch (mode->hdisplay) {
	case 1920:
		if (mode->vdisplay == 1080)
			return MODE_OK;
		break;
	case 1280:
		if (mode->vdisplay == 720)
			return MODE_OK;
		break;
	case 960:
		if (mode->vdisplay == 540)
			return MODE_OK;
		break;
	default:
		return MODE_ERROR;
	}

	return MODE_ERROR;
}

static void syna_dvi_connector_destroy(struct drm_connector *connector)
{
	struct syna_drm_private *dev_priv = connector->dev->dev_private;

	if (!dev_priv) {
		DRM_ERROR("%s %d  device private is NULL!!\n",
			  __func__, __LINE__);
		return;
	}
	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s]\n",
			 connector->base.id, connector->name);

	drm_connector_cleanup(connector);

	kfree(connector);
	dev_priv->connector = NULL;
}

static void syna_dvi_connector_force(struct drm_connector *connector)
{
}

static struct drm_connector_helper_funcs syna_dvi_connector_helper_funcs = {
	.get_modes = syna_dvi_connector_helper_get_modes,
	.mode_valid = syna_dvi_connector_helper_mode_valid,
};

static const struct drm_connector_funcs syna_dvi_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = syna_dvi_connector_destroy,
	.force = syna_dvi_connector_force,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.dpms = drm_helper_connector_dpms,
};

struct drm_connector *syna_dvi_connector_create(struct drm_device *dev)
{
	struct drm_connector *connector;

	connector = kzalloc(sizeof(*connector), GFP_KERNEL);
	if (!connector)
		return ERR_PTR(-ENOMEM);

	drm_connector_init(dev,
			   connector,
			   &syna_dvi_connector_funcs, DRM_MODE_CONNECTOR_DVID);
	drm_connector_helper_add(connector, &syna_dvi_connector_helper_funcs);

	connector->dpms = DRM_MODE_DPMS_OFF;
	connector->interlace_allowed = false;
	connector->doublescan_allowed = false;
	connector->display_info.subpixel_order = SubPixelHorizontalRGB;

	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s]\n", connector->base.id,
			 connector->name);

	return connector;
}
