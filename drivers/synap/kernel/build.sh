#!/bin/bash

# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2021 Synaptics Incorporated

# function to print titles in bold
function title() {
	echo -e '\n\033[1m'$1'\033[0m'
}

# directory where this script is located
mod_dir=$(readlink -f "$(dirname "${BASH_SOURCE}")")

# include the common file to parse config and setup variables
source build/header.rc
source build/chip.rc

# directory where the kernel build installs the in-tree modules and modules.dep
opt_outdir_release=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_LINUX_REL_PATH}

# directory where the kernel build installs stores the intermediate files
opt_outdir_intermediate=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_LINUX_REL_PATH}/intermediate

# directory where we want to store our own intermediate files
opt_outdir_synap_intermediate=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/synap/intermediate/ko

# directory containing the kernel sources
opt_linux_src=${topdir}/${CONFIG_LINUX_SRC_PATH}

# sysroot with output of the build
opt_workdir_runtime_sysroot=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_SYNA_SDK_OUT_ROOTFS}

# in case we were called to perform a clean remove the intermediate results
# the clean doesn't remove files from opt_outdir_release and opt_workdir_runtime_sysroot
if [ ${clean} -eq 1 ]; then
    rm -rf ${opt_outdir_synap_intermediate} && echo "Successfully cleaned intermediate files"
    exit 0
fi

# setup the make variables used in case LLVM is used
opt_var_defs="$2"
if [ "is${CONFIG_LINUX_CROSS_COMPILE_LLVM_UTILS}" = "isy" ]; then
    opt_var_defs="${opt_var_defs} LLVM=1"
fi

# set the variables to select the crosscompiler and target architecture
make_extra_vars="ARCH=${CONFIG_LINUX_ARCH} CROSS_COMPILE=${CONFIG_LINUX_CROSS_COMPILE} CC=${CONFIG_LINUX_CROSS_COMPILE_CC}"

# set the O= directory with intermediated files from the kernel build, M= the directory where our intermediate files will 
# be stored, src= the directory with our Kbuild file
make_vars="O=${opt_outdir_intermediate} M=${opt_outdir_synap_intermediate}/linux src=${mod_dir}"

# build the module with output in the opt_outdir_synap_intermediate directory
title "Building kernel module"
mkdir -p ${opt_outdir_intermediate}

# copy Kbuild file to output directory (the second pass needs to find the Kbuild file there to parse KBUILD_EXTRA_SYMBOLS)
mkdir -p ${opt_outdir_synap_intermediate}/linux
cp ${mod_dir}/Kbuild ${opt_outdir_synap_intermediate}/linux

echo make ${make_vars} ${make_extra_vars} ${opt_var_defs} -C ${opt_linux_src} -j ${CONFIG_CPU_NUMBER}

make ${make_vars} ${make_extra_vars} ${opt_var_defs} -C ${opt_linux_src} -j ${CONFIG_CPU_NUMBER}

# install the module in same directory of the modules built in-tree and regenerate modules.dep file of the system
title "Installing kernel module and creating modules.dep"
mkdir -p ${opt_outdir_release}
make ${make_vars} ${make_extra_vars} ${opt_var_defs} -C ${opt_linux_src} INSTALL_MOD_PATH=${opt_outdir_release} modules_install

# update the sysroot with the new collection of modules (that includes both in-tree modules and our module)
title "Updating sysroot with kernel module and modules.dep"
mkdir -p ${opt_workdir_runtime_sysroot}/lib
cp -ad ${opt_outdir_release}/lib/modules ${opt_workdir_runtime_sysroot}/lib/ && echo done
