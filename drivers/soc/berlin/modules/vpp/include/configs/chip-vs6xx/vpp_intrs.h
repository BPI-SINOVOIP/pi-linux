// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef _VPP_INTRS_H_
#define _VPP_INTRS_H_

#include "avioDhub.h"

static unsigned int vpp_intrs [] = {
	//VOP1/DP1 - VBI, VDE
	avioDhubSemMap_vpp128b_vpp_inr0,
	avioDhubSemMap_vpp128b_vpp_inr6,
	//VOP2/DP2 - VOP_TG, EoF
	avioDhubSemMap_vpp128b_vpp_inr13,
	avioDhubSemMap_vpp128b_vpp_inr14,
	//OVP - uv,y
	avioDhubSemMap_vpp128b_vpp_inr11,
	avioDhubSemMap_vpp128b_vpp_inr12,
	avioDhubSemMap_vpp128b_vpp_inr2,
	avioDhubSemMap_vpp128b_vpp_inr3,
	avioDhubSemMap_vpp128b_vpp_inr4,
	avioDhubSemMap_vpp128b_vpp_inr5,
	avioDhubSemMap_vpp128b_vpp_inr15
};

#endif //_VPP_INTRS_H_
