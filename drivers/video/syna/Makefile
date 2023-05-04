PWD := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BERLIN_INC := $(srctree)/drivers/soc/berlin/modules/
BERLIN_VPP_INC := $(srctree)/drivers/soc/berlin/modules/vpp/include/
ION_INC := $(srctree)/drivers/staging/android/

###############################################################################
#
# Common FB module objects
#
###############################################################################

obj-$(CONFIG_SYNA_FBDEV) += synafb.o

synafb-objs += source/fb.o

#Enable for double buffering otherwise tripple buffering
MV_DEFINE += -DMRVL_FB_ENABLE_DOUBLE_BUFFERING

MV_DEFINE += -DMV_SYSTEM_WIDTH_DEFAULT=1280
MV_DEFINE += -DMV_SYSTEM_HEIGHT_DEFAULT=720

MV_DEFINE += -I$(BERLIN_INC)/tzd \
             -I$(BERLIN_INC)/tzd/tz/common \
             -I$(BERLIN_INC)/tzd/include


MV_DEFINE += -I$(BERLIN_INC)/avio/common

HEADER_PATH  += -I$(BERLIN_VPP_INC)
HEADER_PATH  += -I$(ION_INC)

ccflags-y += $(MV_DEFINE)
ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror
