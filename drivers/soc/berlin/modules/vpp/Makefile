PWD := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BERLIN_INC := $(srctree)/drivers/soc/berlin/modules/
ION_INC := $(srctree)/drivers/staging/android/

###############################################################################
#
# Common VPP module for FB and DRM
#
###############################################################################
obj-$(CONFIG_BERLIN_VPP) := vpp.o

vpp-objs += \
     source/vpp_api.o		\
     source/vpp.o		\
     source/vpp_mem.o		\
     source/vpp_isr.o		\
     source/vpp_fb.o

###############################################################################
#
# Chip specific section
#
###############################################################################
ifneq ($(filter y, $(CONFIG_AVIO_VS640) $(CONFIG_AVIO_VS680) $(CONFIG_AVIO_VS680_A0)), )
ifeq ($(CONFIG_AVIO_VS640), y)
FIRMWARE_INC_DIR_LOCAL=  $(BERLIN_INC)/avio/fw_inc/vs640/
else ifeq ($(CONFIG_AVIO_VS680), y)
FIRMWARE_INC_DIR_LOCAL=  $(BERLIN_INC)/avio/fw_inc/vs680/
else ifeq ($(CONFIG_AVIO_VS680_A0), y)
FIRMWARE_INC_DIR_LOCAL=  $(BERLIN_INC)/avio/fw_inc/vs680a0/
endif
HEADER_PATH  += -I$(PWD)include/configs/chip-vs6xx/
endif

###############################################################################
#
# Common AVIOUTIL kernel module objects
#
###############################################################################
AVIO_INC_PATH := -I$(BERLIN_INC)/avio/include
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/vpp/ca/include
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/common
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/dhub
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/dhub/include
AVIO_INC_PATH += -I$(BERLIN_INC)/avio/fastcall/include

###############################################################################
HEADER_PATH += -I$(BERLIN_INC)/tzd/include \
               -I$(BERLIN_INC)/tzd \
               -I$(BERLIN_INC)/tzd/tz/common

HEADER_PATH +=  \
              -I$(FIRMWARE_INC_DIR_LOCAL)  \
               $(AVIO_INC_PATH)\
              -I$(PWD)include/common/ \
              -I$(srctree)/drivers/soc/berlin/modules/msgQ

HEADER_PATH  += -I$(PWD)include/
HEADER_PATH  += -I$(ION_INC)

ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror
