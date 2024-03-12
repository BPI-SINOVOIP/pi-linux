#
# NDA AND NEED-TO-KNOW REQUIRED
#
# Copyright (C) 2013-2020 Synaptics Incorporated. All rights reserved.
#
# This file contains information that is proprietary to Synaptics
# Incorporated ("Synaptics"). The holder of this file shall treat all
# information contained herein as confidential, shall use the
# information only for its intended purpose, and shall not duplicate,
# disclose, or disseminate any of this information in any manner
# unless Synaptics has otherwise provided express, written
# permission.
#
# Use of the materials may require a license of intellectual property
# from a third party or from Synaptics. This file conveys no express
# or implied licenses to any intellectual property rights belonging
# to Synaptics.
#
# INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
# SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
# INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
# INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
# CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
# OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
# BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
# COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
# DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
# TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.

set(synap_ko_intermediate ${VSSDK_BUILD_TARGET_DIR}/synap/intermediate/ko)
set(synap_ko ${synap_ko_intermediate}/linux/synap.ko)
file(GLOB src src/*.c src/*.h ../common/inc/*.h ../common/src/*.c)

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(MOD_OPTIONS DEBUG=1)
endif()

add_custom_command(
    OUTPUT ${synap_ko}
    # This is just to make sure the module will be rebuilt if the source is changed.
    DEPENDS ${src} Kbuild build.sh
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build.sh ${VSSDK_CONFIG_FILE} ${MOD_OPTIONS}
    WORKING_DIRECTORY ${VSSDK_DIR_ABS}
)

add_custom_target(synapko ALL DEPENDS ${synap_ko})
add_custom_target(synapko_clean COMMAND rm -rf ${synap_ko_intermediate})

if(${VSSDK_CONFIG_ANDROID_OS})
    set(install_subdir "android")
elseif(${CONFIG_LINUX_OS})
    set(install_subdir "linux")
endif()

install(FILES ${synap_ko} DESTINATION kernel/${install_subdir}/${VSSDK_CONFIG_LINUX_SRC_PATH})
