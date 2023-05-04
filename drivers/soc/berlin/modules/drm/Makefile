HEADER_PATH += -I$(srctree)/drivers/soc/berlin/modules/avio/vpp \
               -I$(srctree)/drivers/soc/berlin/modules/avio/vpp/ca/include/ \
               -I$(srctree)/drivers/soc/berlin/modules/avio/common/ \
               -I$(srctree)/drivers/soc/berlin/modules/avio/ \
               -I$(srctree)/drivers/soc/berlin/modules/avio/include \
               -I$(srctree)/drivers/soc/berlin/modules/vpp/include \
               -I$(srctree)/drivers/soc/berlin/modules/msgQ/

ifeq ($(CONFIG_AVIO_VS680),y)
HEADER_PATH += -I$(srctree)/drivers/soc/berlin/modules/avio/fw_inc/vs680
ccflags-y += -DUSE_VS680=1
else ifeq ($(CONFIG_AVIO_VS680_A0),y)
HEADER_PATH += -I$(srctree)/drivers/soc/berlin/modules/avio/fw_inc/vs680a0
ccflags-y += -DUSE_VS680=1
else ifeq ($(CONFIG_AVIO_VS640),y)
HEADER_PATH += -I$(srctree)/drivers/soc/berlin/modules/avio/fw_inc/vs640
ccflags-y += -DUSE_VS640=1
endif

ccflags-y += $(HEADER_PATH)
ccflags-y += -Werror

obj-$(CONFIG_SYNA_DRM) +=syna_drm.o
syna_drm-y += drm_syna_crtc.o
syna_drm-y += drm_syna_debugfs.o
syna_drm-y += drm_syna_drv.o
syna_drm-y += drm_syna_dvi.o
syna_drm-y += drm_syna_gem.o
syna_drm-y += drm_syna_modeset.o
syna_drm-y += drm_syna_plane.o
syna_drm-y += drm_syna_tmds.o
syna_drm-y += syna_vpp.o