###############################################################################
#
# Common avio kernel module objects
#
###############################################################################
AVIO_ABS_PATH := $(srctree)/drivers/soc/berlin/modules/avio
AVIO_INC_PATH := -I$(AVIO_ABS_PATH)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/include
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/common

AVIO_COMMON_API_OBJS := avio_core.o \
						avio_util.o \
						avio_sub_module.o \


###############################################################################
# Fastcall - avio kernel sub-module objects
###############################################################################
AVIO_FASTCALL_API_OBJS := fastcall/hal_dhub_fastcall_wrap.o
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/fastcall/include

AVIO_INC_PATH += -I$(srctree)/drivers/soc/berlin/modules/tzd/tee/client_api \
				 -I$(srctree)/drivers/soc/berlin/modules/tzd/tee/common \
				 -I$(srctree)/drivers/soc/berlin/modules/tzd/tz/common \
				 -I$(srctree)/drivers/soc/berlin/modules/tzd/include \
				 -I$(srctree)/drivers/soc/berlin/modules/tzd

###############################################################################
# AIO - aio kernel sub-module objects
###############################################################################
AVIO_AIO_API_OBJS := aio/drv_aio.o
AVIO_AIO_API_OBJS += aio/aio_common.o
AVIO_AIO_API_OBJS += aio/i2s_common.o
AVIO_AIO_API_OBJS += aio/pdm_common.o
AVIO_AIO_API_OBJS += aio/spdifi_common.o
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/aio

ifeq ($(CONFIG_AVIO_AS370),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/aio/as370
AVIO_AIO_API_OBJS += aio/as370/i2s.o
AVIO_AIO_API_OBJS += aio/as370/pdm.o
endif

ifeq ($(CONFIG_AVIO_AS470),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/aio/as470
AVIO_AIO_API_OBJS += aio/as470/i2s.o
AVIO_AIO_API_OBJS += aio/as470/pdm.o
AVIO_AIO_API_OBJS += aio/as470/dmic.o
AVIO_AIO_API_OBJS += aio/as470/misc.o
endif

ifneq ($(filter y, $(CONFIG_AVIO_VS640) $(CONFIG_AVIO_VS680) $(CONFIG_AVIO_VS680_A0)), )
ifeq ($(CONFIG_AVIO_VS640),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/aio/vs6xx/640
endif
ifeq ($(CONFIG_AVIO_VS680),y)
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/aio/vs6xx/680
else
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/aio/vs6xx/680_a0
endif

ifeq ($(CONFIG_AVIO_VS640),y)
AVIO_AIO_API_OBJS += aio/vs6xx/640/aio_priv.o
endif
# vs680z1 share the same aio_priv.c with vs640
ifeq ($(CONFIG_AVIO_VS680),y)
AVIO_AIO_API_OBJS += aio/vs6xx/640/aio_priv.o
endif
ifeq ($(CONFIG_AVIO_VS680_A0),y)
AVIO_AIO_API_OBJS += aio/vs6xx/680_a0/aio_priv.o
AVIO_AIO_API_OBJS += aio/vs6xx/earc.o
endif

AVIO_AIO_API_OBJS += aio/vs6xx/i2s.o
AVIO_AIO_API_OBJS += aio/vs6xx/pdm.o
AVIO_AIO_API_OBJS += aio/vs6xx/dmic.o
AVIO_AIO_API_OBJS += aio/vs6xx/misc.o
endif

###############################################################################
# VPP - avio kernel sub-module objects
###############################################################################
AVIO_VPP_API_OBJS := vpp/drv_vpp.o
AVIO_VPP_API_OBJS += vpp/ca/source/tee_ca_vpp.o
AVIO_VPP_API_OBJS += vpp/ca/source/hal_vpp_tz.o
AVIO_VPP_API_OBJS += vpp/ca/source/hal_vpp_wrap.o

AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/vpp/ca/include
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/vpp

###############################################################################
# DHUB - avio kernel sub-module objects
###############################################################################
AVIO_DHUB_API_OBJS := dhub/drv_dhub.o
AVIO_DHUB_API_OBJS += dhub/avio_dhub_drv.o
AVIO_DHUB_API_OBJS += dhub/hal/hal_dhub_wrap.o
AVIO_DHUB_API_OBJS += dhub/hal/hal_dhub.o
AVIO_DHUB_API_OBJS += dhub/ca/tee_ca_dhub.o

AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub/ca
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub/hal
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/dhub/include

###############################################################################
# HDMITX - avio kernel sub-module objects
###############################################################################
AVIO_HDMITX_API_OBJS := hdmitx/drv_hdmitx.o
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/hdmitx

###############################################################################
# regmap - avio kernel sub-module objects
###############################################################################
AVIO_REGMAP_API_OBJS := common/regmap/regmap_base.o
AVIO_REGMAP_API_OBJS += common/regmap/avio_regmap.o
AVIO_INC_PATH += -I$(AVIO_ABS_PATH)/common/regmap

###############################################################################

AVIO_API_OBJS := $(AVIO_COMMON_API_OBJS) $(AVIO_VPP_API_OBJS) $(AVIO_DHUB_API_OBJS)
AVIO_API_OBJS += $(AVIO_FASTCALL_API_OBJS) $(AVIO_REGMAP_API_OBJS)
AVIO_API_OBJS += $(AVIO_AIO_API_OBJS) $(AVIO_HDMITX_API_OBJS)

###############################################################################
#
# Chip specific section
#
###############################################################################
ifeq ($(CONFIG_AVIO_AS370),y)
AVIO_CFG_PATH += -I$(AVIO_ABS_PATH)/fw_inc/as370
AVIO_DHUB_CFG_OBJS += dhub/configs/as370/avio_dhub_cfg.o
endif

ifeq ($(CONFIG_AVIO_AS470),y)
AVIO_CFG_PATH += -I$(AVIO_ABS_PATH)/fw_inc/as470
AVIO_DHUB_CFG_OBJS += dhub/configs/as470/avio_dhub_cfg.o
endif

ifneq ($(filter y, $(CONFIG_AVIO_VS640) $(CONFIG_AVIO_VS680) $(CONFIG_AVIO_VS680_A0)), )
ifeq ($(CONFIG_AVIO_VS640),y)
AVIO_CFG_PATH += -I$(AVIO_ABS_PATH)/fw_inc/vs640
AVIO_DHUB_CFG_OBJS += dhub/configs/vs6xx/640/dhub_priv.o
else ifneq ($(filter y, $(CONFIG_AVIO_VS680) $(CONFIG_AVIO_VS680_A0)), )
ifeq ($(CONFIG_AVIO_VS680),y)
AVIO_CFG_PATH += -I$(AVIO_ABS_PATH)/fw_inc/vs680
else ifeq ($(CONFIG_AVIO_VS680_A0),y)
AVIO_CFG_PATH += -I$(AVIO_ABS_PATH)/fw_inc/vs680a0
endif
AVIO_DHUB_CFG_OBJS += dhub/configs/vs6xx/680/dhub_priv.o
endif
AVIO_DHUB_CFG_OBJS += vpp/configs/vs6xx/drv_vpp_cfg.o
AVIO_DHUB_CFG_OBJS += dhub/configs/vs6xx/avio_dhub_cfg.o
AVIO_DHUB_CFG_OBJS += dhub/configs/vs6xx/hal_dhub_ext.o
endif

AVIO_CFG_OBJS := $(AVIO_DHUB_CFG_OBJS)

###############################################################################
#
# Common MsgQ kernel module objects
#
###############################################################################
MSGQ_INC_PATH := -I$(srctree)/drivers/soc/berlin/modules/msgQ

EXTERN_INC_PATH := $(MSGQ_INC_PATH)


###############################################################################
#
# Common macro/defines
#
###############################################################################

###############################################################################
#Final part of the Makefile
###############################################################################

AVIO_ALL_OBJS := $(AVIO_API_OBJS) $(AVIO_CFG_OBJS)

avio-objs += $(AVIO_ALL_OBJS) \


HEADER_PATH +=  $(AVIO_INC_PATH)    \
				$(EXTERN_INC_PATH)  \
				$(AVIO_CFG_PATH)    \


ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror

obj-$(CONFIG_BERLIN_AVIO) += avio.o
