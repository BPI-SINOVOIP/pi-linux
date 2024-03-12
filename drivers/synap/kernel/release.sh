#!/bin/bash

# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2021 Synaptics Incorporated

source build/header.rc
source build/install.rc

mod_dir=${CONFIG_SYNA_SDK_PATH}/synap/vsi_npu_driver
dst_dir=${CONFIG_SYNA_SDK_REL_PATH}/synap/vsi_npu_driver

# install the sources of the kernel module (only GPL/MIT files)
mkdir -p ${dst_dir}/synap/kernel
rsync -az --exclude={tests,.git} ${mod_dir}/synap/kernel/ ${dst_dir}/synap/kernel/

# install the commone sources of the kernel module (only GPL/MIT files)
mkdir -p ${dst_dir}/synap/common
rsync -az --exclude={.git} ${mod_dir}/synap/common/ ${dst_dir}/synap/common/
