# Berlin Platform Support

ccflags-y += -I$(srctree)/sound/soc/syna/inc
ccflags-y += -Werror

ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/include
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/common
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/dhub
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/dhub/include
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/aio
ifeq ($(CONFIG_AVIO_VS680),y)
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/fw_inc/vs680
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/aio/vs6xx/680
endif
ifeq ($(CONFIG_AVIO_VS640),y)
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/fw_inc/vs640
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/aio/vs6xx/640
else ifeq ($(CONFIG_AVIO_VS680_A0),y)
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/fw_inc/vs680a0
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/aio/vs6xx/680_a0
endif
ifeq ($(CONFIG_AVIO_AS370),y)
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/fw_inc/as370
ccflags-y += -I$(srctree)/drivers/soc/berlin/modules/avio/aio/as370
endif

snd-soc-berlin-asoc-objs   += berlin_asoc.o
snd-soc-berlin-pcm-objs    += berlin_pcm.o \
			      berlin_capture.o \
			      berlin_util.o \
			      berlin_playback.o \
			      berlin_spdif.o
snd-soc-berlin-dmic-objs   += dmic.o
snd-soc-berlin-outdai-objs += i2s_spdif_outdai.o
snd-soc-berlin-sec-objs    += i2s_sec.o
snd-soc-berlin-pdmi-objs   += pdm_pdmi.o
snd-soc-berlin-mic1-objs   += i2s_mic1.o
snd-soc-berlin-mic2-objs   += i2s_mic2.o
snd-soc-berlin-mic6-objs   += i2s_mic6.o
snd-soc-berlin-spdifi-objs += spdif_spdifi.o
snd-soc-berlin-pri-loopback-objs += i2s_pri_loopback.o
snd-soc-berlin-hdmi-loopback-objs += i2s_hdmi_loopback.o

obj-$(CONFIG_SND_SOC_BERLIN_PCM)    += snd-soc-berlin-pcm.o
obj-$(CONFIG_SND_SOC_BERLIN_DMIC)   += snd-soc-berlin-dmic.o
obj-$(CONFIG_SND_SOC_BERLIN_OUTDAI) += snd-soc-berlin-outdai.o
obj-$(CONFIG_SND_SOC_BERLIN_SEC)    += snd-soc-berlin-sec.o
obj-$(CONFIG_SND_SOC_BERLIN_PDMI)   += snd-soc-berlin-pdmi.o
obj-$(CONFIG_SND_SOC_BERLIN_MIC1)   += snd-soc-berlin-mic1.o
obj-$(CONFIG_SND_SOC_BERLIN_MIC2)   += snd-soc-berlin-mic2.o
obj-$(CONFIG_SND_SOC_BERLIN_MIC6)   += snd-soc-berlin-mic6.o
obj-$(CONFIG_SND_SOC_BERLIN_SPDIFI) += snd-soc-berlin-spdifi.o
obj-$(CONFIG_SND_SOC_BERLIN_PRI_LOOPBACK)  += snd-soc-berlin-pri-loopback.o
obj-$(CONFIG_SND_SOC_BERLIN_HDMI_LOOPBACK) += snd-soc-berlin-hdmi-loopback.o
obj-$(CONFIG_SND_SOC_BERLIN_ASOC)   += snd-soc-berlin-asoc.o
