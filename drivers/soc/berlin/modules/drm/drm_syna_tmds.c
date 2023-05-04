// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include "drm_syna_drv.h"

static void syna_tmds_encoder_helper_dpms(struct drm_encoder *encoder, int mode)
{
}

static bool
syna_tmds_encoder_helper_mode_fixup(struct drm_encoder *encoder,
				    const struct drm_display_mode *mode,
				    struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void syna_tmds_encoder_helper_prepare(struct drm_encoder *encoder)
{
}

static void syna_tmds_encoder_helper_commit(struct drm_encoder *encoder)
{
}

static void
syna_tmds_encoder_helper_mode_set(struct drm_encoder *encoder,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
}

static void syna_tmds_encoder_destroy(struct drm_encoder *encoder)
{
	struct syna_drm_private *dev_priv = encoder->dev->dev_private;

	if (encoder == NULL) {
		DRM_ERROR("%s: encoder is NULL!!\n", __func__);
		return;
	}
	DRM_DEBUG_DRIVER("[ENCODER:%d:%s]\n", encoder->base.id, encoder->name);

	drm_encoder_cleanup(encoder);

	kfree(encoder);
	dev_priv->encoder = NULL;
}

static const struct drm_encoder_helper_funcs syna_tmds_encoder_helper_funcs = {
	.dpms = syna_tmds_encoder_helper_dpms,
	.mode_fixup = syna_tmds_encoder_helper_mode_fixup,
	.prepare = syna_tmds_encoder_helper_prepare,
	.commit = syna_tmds_encoder_helper_commit,
	.mode_set = syna_tmds_encoder_helper_mode_set,
};

static const struct drm_encoder_funcs syna_tmds_encoder_funcs = {
	.destroy = syna_tmds_encoder_destroy,
};

struct drm_encoder *syna_tmds_encoder_create(struct drm_device *dev)
{
	struct drm_encoder *encoder;
	int err;

	encoder = kzalloc(sizeof(*encoder), GFP_KERNEL);
	if (!encoder)
		return ERR_PTR(-ENOMEM);

	err = drm_encoder_init(dev,
			       encoder,
			       &syna_tmds_encoder_funcs,
			       DRM_MODE_ENCODER_TMDS, NULL);
	if (err) {
		DRM_ERROR("Failed to initialise encoder");
		kfree(encoder);
		return ERR_PTR(err);
	}
	drm_encoder_helper_add(encoder, &syna_tmds_encoder_helper_funcs);

	/*
	 * This is a bit field that's used to determine which
	 * CRTCs can drive this encoder.
	 */
	encoder->possible_crtcs = 0x1;

	DRM_DEBUG_DRIVER("[ENCODER:%d:%s]\n", encoder->base.id, encoder->name);

	return encoder;
}
