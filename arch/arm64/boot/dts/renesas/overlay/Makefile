# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_RENESAS) += \
	bpi-ai2n-i2c1.dtbo \
	bpi-ai2n-i2c1-flash.dtbo \
	bpi-ai2n-i2c1-rtc.dtbo \
	bpi-ai2n-i2c2.dtbo \
	bpi-ai2n-uart0.dtbo \
	bpi-ai2n-uart2.dtbo \
	bpi-ai2n-pwm.dtbo \
	bpi-ai2n-pwmfan.dtbo \
	bpi-ai2n-spi0.dtbo \
	bpi-ai2n-spi2.dtbo \
	bpi-ai2n-spi2-flash.dtbo \
	bpi-ai2n-spi2-waveshare-st7789v.dtbo \
	bpi-ai2n-spi2-waveshare-tft24.dtbo \
	bpi-ai2n-spi2-waveshare-tft35c.dtbo \
	bpi-ai2n-i2s3-nxez-pcm512x.dtbo \
	bpi-ai2n-i2s3-waveshare-wm8960.dtbo \
	bpi-ai2n-i2c0-mux.dtbo \
	bpi-ai2n-cam0-imx415.dtbo

dtbo-$(CONFIG_ARCH_RENESAS) += \
	rzv2n-evk-cam-imx415.dtbo \
	rzv2n-evk-cam-imx415-dual.dtbo \
	rzv2n-evk-cam-waveshare-imx415.dtbo \
	rzv2n-evk-cam-luckfox-imx415.dtbo \
	rzv2n-evk-cam-econ-imx462.dtbo \
	rzv2n-evk-cam-ov5640.dtbo

dtbo-$(CONFIG_ARCH_RENESAS) += \
	bpi-ai2n-waveshare-jetson-io-cam0-ov5640.dtbo \
	bpi-ai2n-waveshare-jetson-io-cam0-imx415.dtbo \
	bpi-ai2n-waveshare-jetson-io-cam0-waveshare-imx415.dtbo \
	bpi-ai2n-waveshare-jetson-io-cam1-waveshare-imx415.dtbo \
	bpi-ai2n-waveshare-jetson-io-cam0-luckfox-imx415.dtbo

scr-$(CONFIG_ARCH_RENESAS) += \
	rzv2n-fixup.scr

dtbotxt-$(CONFIG_ARCH_RENESAS) += \
	README.rzv2n-overlays

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always-y       := $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files    := *.dtbo *.scr
