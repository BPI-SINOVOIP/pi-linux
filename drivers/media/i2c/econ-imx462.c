/*
 * Driver for the IMX462 camera imx462.
 *
 */

#include "econ-imx462.h"

#define DEBUG

#define DEBUG_PRINTKx 
#ifndef DEBUG_PRINTK
#define debug_printk(s , ... )
#else
#define debug_printk pr_info
#endif


static const struct v4l2_ctrl_ops imx462_ctrl_ops = {
	.s_ctrl = imx462_s_ctrl,
};

static const struct v4l2_subdev_video_ops imx462_video_ops = {
	.s_stream = imx462_s_stream,
	.s_frame_interval = imx462_s_frame_interval,
	.g_frame_interval = imx462_g_frame_interval,
//	.g_parm = imx462_g_parm,
//	.s_parm = imx462_s_parm,
};

static const struct v4l2_subdev_pad_ops imx462_subdev_pad_ops = {
	.init_cfg = imx462_entity_init_cfg,
	.enum_mbus_code = imx462_enum_mbus_code,
	.enum_frame_size = imx462_enum_framesizes,
	.get_fmt = imx462_get_fmt,
	.set_fmt = imx462_set_fmt,
//	.enum_frame_interval = imx462_enum_frameintervals,
};

static const struct v4l2_subdev_core_ops imx462_core_ops = {
	.s_power = imx462_s_power,
	/*
	.queryctrl = imx462_queryctrl,
	.s_ctrl = imx462_s_ctrl,
	.g_ctrl = imx462_g_ctrl,
	.g_ext_ctrls = imx462_g_ext_ctrls,
        .s_ext_ctrls = imx462_s_ext_ctrls,	
	.querymenu = imx462_querymenu,
	*/
};

static const struct v4l2_subdev_ops imx462_subdev_ops = {
	.core = &imx462_core_ops,
	.video = &imx462_video_ops,
	.pad = &imx462_subdev_pad_ops,
};

#if 0
static int get_isp_logs(struct i2c_client *client)
{
	uint16_t numberOfLogs = 0;
	uint8_t tx_data[8], rx_data[5], logBuf[256];
	int err = 0, ret = 0;
	int counter, i;
	uint8_t length;

	printk(KERN_ERR "-------in isp logs\n");
	err = imx462_write(client, logMode, ARRAY_SIZE(logMode));
	if (err != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, err);
		ret = -1;
		goto exit;
	}

	logStartStop[4] = 0x02;
	err = imx462_write(client, logStartStop, ARRAY_SIZE(logStartStop));
	if (err != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, err);
		ret = -1;
		goto exit;
	}
	err = imx462_write(client, ispLogStringAddr, ARRAY_SIZE(ispLogStringAddr));
	if (err != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, err);
		ret = -1;
		goto exit;
	}

	do {
		msleep(5);
		counter = 300;
		length = 0;
		ispLogCount0[4] = numberOfLogs & 0xFF;
		err = imx462_write(client, ispLogCount0, ARRAY_SIZE(ispLogCount0));
		if (err != 0) {
			dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, err);
			ret = -1;
			goto exit;
		}
		ispLogCount1[4] = numberOfLogs >> 8;
		err = imx462_write(client, ispLogCount1, ARRAY_SIZE(ispLogCount1));
		if (err != 0) {
			dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, err);
			ret = -1;
			goto exit;
		}
		logStartStop[4] = 0x03;
		err = imx462_write(client, logStartStop, ARRAY_SIZE(logStartStop));
		if (err != 0) {
			dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, err);
			ret = -1;
			goto exit;
		}
		do {
			err = imx462_write(client, getlogLength, ARRAY_SIZE(getlogLength));
			if (err != 0) {
				dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
						__LINE__, err);
				ret = -1;
				goto exit;
			}
			err = imx462_read(client, rx_data, 2);
			if (err != 0) {
				dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
						__LINE__, err);
				ret = -1;
				goto exit;
			}
			counter--;
		} while(rx_data[1] == 0xFF && counter != 0);

		if(rx_data[1] == 0xFF || rx_data[1] == 0x00)
			break;

		length = rx_data[1];
		err = imx462_write(client, getlogAddr, ARRAY_SIZE(getlogAddr));
		if (err != 0) {
			dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, err);
			ret = -1;
			goto exit;
		}
		err = imx462_read(client, rx_data, 5);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
					__LINE__, err);
			ret = -1;
			goto exit;
		}

		/* getting log buffer*/
		tx_data[0] = 0x00;
		tx_data[1] = 0x03;
		tx_data[2] = rx_data[1];
		tx_data[3] = rx_data[2];
		tx_data[4] = rx_data[3];
		tx_data[5] = rx_data[4];
		tx_data[6] = 0x00;
		tx_data[7] = length;
		err = imx462_write(client, tx_data, 8);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, err);
			ret = -1;
			goto exit;
		}
		err = imx462_read(client, logBuf, length + 3);
		if (err != 0) {
			dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
					__LINE__, err);
			ret = -1;
			goto exit;
			dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
					__LINE__, err);
			ret = -1;
			goto exit;
		}
		printk("ISP log: ");
		for(i = 3; i < length + 3; i++)
			printk(KERN_CONT "%c", /*(char)*/ logBuf[i]);

		numberOfLogs++;
		printk("\n");
	} while(1);
	logStartStop[4] = 0x01;
	err = imx462_write(client, logStartStop, ARRAY_SIZE(logStartStop));
	if (err != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, err);
		ret = -1;
		goto exit;
	}
exit:
	printk("-- total number of logs taken = %d\n", numberOfLogs);
	return err;
}
#endif

static void toggle_gpio(struct gpio_desc *gpio, int delay)
{
                gpiod_set_value_cansleep(gpio, 0);
		msleep(delay);
		gpiod_set_value_cansleep(gpio, 1);
		msleep(delay);
}



static int romErase(struct i2c_client *client, uint32_t startAddress, bool sectorErase)
{
	uint8_t rx_data[2];
	int ret = 0, retry = 0;

	/* set ROM address */
	setROMaddr[4] = startAddress >> 24;
	setROMaddr[5] = startAddress >> 16;
	setROMaddr[6] = startAddress >> 8;
	setROMaddr[7] = startAddress;
	for(retry = 0; retry <= RETRY_COUNT; retry++) {
		ret = imx462_write(client, setROMaddr, ARRAY_SIZE(setROMaddr));
		if (ret != 0) {
			if(retry != RETRY_COUNT) {
				msleep(100);
				continue;
			} else {
				dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
				return ret;
			}
		}
		break;
	}

	/* Sector/Block erase command */
	for(retry = 0; retry <= RETRY_COUNT; retry++) {
		if(sectorErase == true)
			ret = imx462_write(client, eraseSector, ARRAY_SIZE(eraseSector));
		else
			ret = imx462_write(client, eraseBlock, ARRAY_SIZE(eraseBlock));
		if (ret != 0) {
			if(retry != RETRY_COUNT) {
				msleep(100);
				continue;
			} else {
				dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
				return ret;
			}
		}
		retry = 0;
		break;
	}

	for(;;) {
		//msleep(1);
		ret = imx462_write(client, eraseStatus, ARRAY_SIZE(eraseStatus));
		if (ret != 0) {
			dev_info(&client->dev, "Retry reading erase status\n");
			retry++;
			if(retry == RETRY_COUNT) {
				dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
				break;
			}
			msleep(100);
			continue;
		}
		ret = imx462_read(client, rx_data, 2);
		if (ret != 0) {
			dev_info(&client->dev, "Retry reading erase status\n");
			retry++;
			if(retry == RETRY_COUNT) {
				dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
					__LINE__, ret);
				break;
			}
			msleep(100);
			continue;
		}

		if(rx_data[1] == ERASE_COMPLETED)
			break;
		else if(rx_data[1] == SECTOR_ERASE_INPROGRESS || rx_data[1] == BLOCK_ERASE_INPROGRESS)
			continue;
		else {
			dev_err(&client->dev, "Error in erasing sector %#x\n", startAddress);
			return -EIO;
		}
	}
	return ret;
}

static int romFlash(struct i2c_client *client)
{
	uint8_t rx_data[2] = {0};
	uint32_t startAddress = SECTOR_END_ADDR;
	int ret = 0, sector, retry = 0, retry_flash = 0;

	for(sector = LAST_SECTOR; sector >= FIRST_SECTOR; sector--) {

		/* set ROM address */
		setROMaddr[4] = startAddress >> 24;
		setROMaddr[5] = startAddress >> 16;
		setROMaddr[6] = startAddress >> 8;
		setROMaddr[7] = startAddress;
		for(retry = 0; retry <= RETRY_COUNT; retry++) {
			ret = imx462_write(client, setROMaddr, ARRAY_SIZE(setROMaddr));
			if (ret != 0) {
				if(retry != RETRY_COUNT) {
					msleep(100);
					continue;
				} else {
					dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
						__LINE__, ret);
					return ret;
				}
			}
			break;
		}
		memcpy(&fwSendcmd[8], &bootData[sector*DATA_SIZE], DATA_SIZE);

		/* set Programming size */
		for(retry = 0; retry <= RETRY_COUNT; retry++) {
			ret = imx462_write(client, setFWsize, ARRAY_SIZE(setFWsize));
			if (ret != 0) {
				if(retry != RETRY_COUNT) {
					msleep(100);
					continue;
				} else {
					dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
						__LINE__, ret);
					return ret;
				}
			}
			break;
		}

		/* send firmware */
		for(retry = 0; retry <= RETRY_COUNT; retry++) {
			ret = imx462_write(client, fwSendcmd, ARRAY_SIZE(fwSendcmd));
			if (ret != 0) {
				if(retry != RETRY_COUNT) {
					msleep(100);
					continue;
				} else {
					dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
						__LINE__, ret);
					return ret;
				}
			}
			break;
		}

		/* program Flash ROM */
		for(retry = 0; retry <= RETRY_COUNT; retry++) {
			ret = imx462_write(client, programROM, ARRAY_SIZE(programROM));
			if (ret != 0) {
				if(retry != RETRY_COUNT) {
					msleep(100);
					continue;
				} else {
					dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
						__LINE__, ret);
					return ret;
				}
			}
			retry = 0;
			break;
		}

		// check completion
		for(;;) {
			//msleep(10);
			ret = imx462_write(client, flashStatus, ARRAY_SIZE(flashStatus));
			if (ret != 0) {
				dev_err(&client->dev," %s(%d) ISP Write Error while writing block%d ret - %d \n", __func__,
						__LINE__, sector, ret);
				if(retry < RETRY_COUNT) {
					dev_info(&client->dev, "Retry reading flash status\n");
					msleep(100);
					retry++;
					continue;
				}
				return ret;
			}
			ret = imx462_read(client, rx_data, 2);
			if (ret != 0) {
				dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
						__LINE__, ret);
				return ret;
			}

			if(rx_data[1] == FLASH_INPROGRESS) continue;
			else if(rx_data[1] == FLASH_COMPLETED) {
				retry_flash = 0;
				retry = 0;
				break;
			} else {
				dev_err(&client->dev, "Error in Flash, Retrying this sector\n");
				sector++;
				startAddress += NEXT_SECTOR;
				break;
			}
		}
		startAddress -= PREVIOUS_SECTOR;
	}
	return ret;
}

static int calcChecksum(struct i2c_client *client)
{
	uint8_t rx_data[3];
	int ret = 0, checkSum;

	/* calculate checksum */
	ret = imx462_write(client, calcChkSum, ARRAY_SIZE(calcChkSum));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	/* check status of calculation */
	for(;;) {
		//msleep(5);
		ret = imx462_write(client, calcStatus, ARRAY_SIZE(calcStatus));
		if (ret != 0) {
			dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}
		ret = imx462_read(client, rx_data, 2);
		if (ret != 0) {
			dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}

		if(rx_data[1] == CALC_COMPLETED) {
			ret = imx462_write(client, getChkSum, ARRAY_SIZE(getChkSum));
			if (ret != 0) {
				dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
						__LINE__, ret);
				return ret;
			}
			ret = imx462_read(client, rx_data, 3);
			if (ret != 0) {
				dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
						__LINE__, ret);
				return ret;
			}
			checkSum = rx_data[1];
			checkSum = (checkSum << 8) + rx_data[2];
			break;
		}
		else if(rx_data[1] == CALC_INPROGRESS) continue;
		else
			dev_err(&client->dev, "Error in calculating checksum\n");
		msleep(10);
	}
	return checkSum;
}


static int imx462_brightness(struct i2c_client *client, u8 value){
	int ret = 0;

	ctrlcmd[2] = 0x03;
	ctrlcmd[3] = 0x21;
	ctrlcmd[4] = (uint8_t) value;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	ctrlcmd[3] = 0x0D;
	ctrlcmd[4] = 0x01;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

static int imx462_contrast(struct i2c_client *client, u8 value){
	int ret = 0;
	
	ctrlcmd[2] = 0x02;
	ctrlcmd[3] = 0x20;
	ctrlcmd[4] = (uint8_t) value;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

static int imx462_sharpness(struct i2c_client *client, u8 value){
	int ret = 0;
	
	ctrlcmd[2] = 0x02;
	ctrlcmd[3] = 0x22;
	ctrlcmd[4] = (uint8_t) value;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

static int imx462_denoise(struct i2c_client *client, u8 value){
	int ret = 0;
	
	ctrlcmd[2] = 0x02;
	ctrlcmd[3] = 0x0C;
	if (value == V4L2_DENOISE_MENU_ON)
		ctrlcmd[4] = 0x01;
	else
		ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

static int imx462_gain(struct i2c_client *client, u8 value){
	int ret = 0;
	
	ctrlcmd[2] = 0x03;
	ctrlcmd[3] = 0x9A;
	ctrlcmd[4] = (uint8_t) value;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	ctrlcmd[3] = 0x0D;
	ctrlcmd[4] = 0x01;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

static int imx462_auto_exposure(struct i2c_client *client, u8 value){
	int ret = 0;
	
	ctrlcmd[2] = 0x03;
	ctrlcmd[3] = 0x01;
	if(value == V4L2_EXPOSURE_MENU_AUTO)
		ctrlcmd[4] = 0x01;
	else
		ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	ctrlcmd[3] = 0x0D;
	ctrlcmd[4] = 0x01;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	
	return ret;
}

static int imx462_exposure_absolute(struct i2c_client *client, u16 value){
	int ret = 0;
	
	ctrlcmd[2] = 0x03;
	ctrlcmd[3] = 0x14;
	ctrlcmd[4] = (uint8_t) (value >> 8);
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	ctrlcmd[3] = 0x15;
	ctrlcmd[4] = (uint8_t) (value);
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	ctrlcmd[3] = 0x0D;
	ctrlcmd[4] = 0x01;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	
	return ret;
}

static int imx462_powerline_frequency(struct i2c_client *client, u16 value){
	int ret = 0;

	ctrlcmd[2] = 0x03;
	ctrlcmd[3] = 0x06;
	if(value == V4L2_POWER_LINE_FREQUENCY_MENU_OFF)
		ctrlcmd[4] = 0x04;
	else if(value == V4L2_POWER_LINE_FREQUENCY_MENU_50HZ)
		ctrlcmd[4] = 0x01;
	else if(value == V4L2_POWER_LINE_FREQUENCY_MENU_60HZ)
		ctrlcmd[4] = 0x02;
	else	//V4L2_POWER_LINE_FREQUENCY_MENU_AUTO
		ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	ctrlcmd[3] = 0x0D;
	ctrlcmd[4] = 0x01;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

static int imx462_white_balance(struct i2c_client *client, u8 value){
	int ret = 0;

	ctrlcmd[2] = 0x06;
	ctrlcmd[3] = 0x02;
	if(value == 0x00) {
		ctrlcmd[4] = 0x01;
		ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	} else {
		ctrlcmd[4] = 0x02;
		ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
		ctrlcmd[3] = 0x03;
		ctrlcmd[4] = (uint8_t) value;
		ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	}
	ctrlcmd[3] = 0x0C;
	ctrlcmd[4] = 0x01;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

#if 0
static int imx462_roll(struct i2c_client *client){
	int ret = 0;

	ret = imx462_write(client, setCamOff, ARRAY_SIZE(setCamOff));
	msleep(100);
	ctrlcmd[2] = 0x02;
	ctrlcmd[3] = 0x05;
	ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	msleep(100);
	ctrlcmd[2] = 0x02;
	ctrlcmd[3] = 0x06;
	ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	msleep(100);
	return ret;
}
#endif

static int imx462_zoom(struct i2c_client *client, u8 value){
	int ret = 0;

	ctrlcmd[2] = 0x02;
	ctrlcmd[3] = 0x01;
	ctrlcmd[4] = (uint8_t) value;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

static int imx462_saturation(struct i2c_client *client, u8 value){
	int ret = 0;

	ctrlcmd[2] = 0x02;
	ctrlcmd[3] = 0x21;
	ctrlcmd[4] = (uint8_t) value;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

static int imx462_autofocus(struct i2c_client *client, u8 value){
	int ret = 0;

	ctrlcmd[2] = 0x0A;
	ctrlcmd[3] = 0x02;
	ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	if(value !=  V4L2_MANUAL_FOCUS) {	
		if(value == V4L2_AF_CONTINUOUS) {
			ctrlcmd[3] = 0x00;
			ctrlcmd[4] = 0x06;
			ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
		}
		else {
			ctrlcmd[3] = 0x00;
			ctrlcmd[4] = 0x01;
			ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
		}
		ctrlcmd[3] = 0x02;
		ctrlcmd[4] = 0x01;
		ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	}
	else {	
		ctrlcmd[2] = 0x00;
		ctrlcmd[3] = 0x10;
		ctrlcmd[4] = ~(0x12);
		ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
		ctrlcmd[2] = 0x0A;
		ctrlcmd[3] = 0x01;
		ctrlcmd[4] = 0x06;
		ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	}
	return ret;
}

static int imx462_caf_range(struct i2c_client *client, u8 value){
	int ret = 0;

	ctrlcmd[2] = 0x0A;
	ctrlcmd[3] = 0x1A;
	ctrlcmd[4] = (uint8_t) value;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	return ret;
}

static int imx462_manual_focus_range(struct i2c_client *client, u16 value){
	int ret = 0;

	ctrlcmd[2] = 0x0D;
	ctrlcmd[3] = 0x18;
	ctrlcmd[4] = (uint8_t)(((value * 10)>>8)&0xff);
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	ctrlcmd[2] = 0x0D;
	ctrlcmd[3] = 0x19;
	ctrlcmd[4] = (uint8_t) ((value * 10)&0xff);
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));

	return ret;
}
static int imx462_configure_framerate(struct i2c_client *client, struct imx462 *imx462)
{
	int err=0;

	switch(imx462->streamcap.capturemode){
		case IMX462_MODE_VGA:
			if(imx462->framerate_index == 30) {
				dev_info(&client->dev, "Stream ON - 480p @30fps\n");
				err = imx462_write(client, slctReslnVGA_30fps, ARRAY_SIZE(slctReslnVGA_30fps));
			}
			else {
				dev_info(&client->dev, "Stream ON - 480p @60fps\n");
				err = imx462_write(client, slctReslnVGA_60fps, ARRAY_SIZE(slctReslnVGA_60fps));
			}
			break;
		case IMX462_MODE_HD:
			if(imx462->framerate_index == 30) {
				dev_info(&client->dev, "Stream ON - 720p @30fps\n");
				err = imx462_write(client, slctReslnHD_30fps, ARRAY_SIZE(slctReslnHD_30fps));
			}
			else {
				dev_info(&client->dev, "Stream ON - 720p @60fps\n");
				err = imx462_write(client, slctReslnHD_60fps, ARRAY_SIZE(slctReslnHD_60fps));
			}
			break;
		case IMX462_MODE_FHD:
		default:
			if(imx462->framerate_index == 30) {
				dev_info(&client->dev, "Stream ON - 1080p @30fps\n");
				err = imx462_write(client, slctReslnFHD_30fps, ARRAY_SIZE(slctReslnFHD_30fps));
			}
			else {
				dev_info(&client->dev, "Stream ON - 1080p @60fps\n");
				err = imx462_write(client, slctReslnFHD_60fps, ARRAY_SIZE(slctReslnFHD_60fps));
			}
			break;
	}

	if(err!=0)
	{
		dev_err(&client->dev, "Error Changing Frame rate\n");
		return -EINVAL;
	}
		return 0;

}


static int imx462_read(struct i2c_client *client, u8 * val, u32 count)
{
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.buf = val,
	};

	msg.flags = I2C_M_RD;
	msg.len = count;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0)
		goto err;

	return 0;

 err:
	dev_err(&client->dev, "Failed reading register ret = %d!\n", ret);
	return ret;
}

static int imx462_write(struct i2c_client *client, u8 * val, u32 count)
{
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = count,
		.buf = val,
	};

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "Failed writing register ret = %d!\n",
			ret);
		return ret;
	}

	return 0;
}

static int imx462_reconfigure(struct imx462 *imx462)
{
	int ret = 0;
	toggle_gpio(imx462->rst_gpio, 10);
	ret = isp_start_firmware(imx462->i2c_client);
	if (ret < 0) {
		dev_err(&imx462->i2c_client->dev,"Error in starting the firmware \n");
		return ret;
	}
	msleep(1500);
	ret = imx462_set_defaults(imx462->i2c_client);
	if (ret) {
		dev_err(&imx462->i2c_client->dev,"Error in setting the defaults \n");
		return ret;
	}

	switch(imx462->streamcap.capturemode){
		case IMX462_MODE_VGA:
			if(imx462->framerate_index == 30) {
				dev_info(&imx462->i2c_client->dev, "Stream ON - 480p @30fps\n");
				ret = imx462_write(imx462->i2c_client, slctReslnVGA_30fps, ARRAY_SIZE(slctReslnVGA_30fps));
			}
			else {
				dev_info(&imx462->i2c_client->dev, "Stream ON - 480p @60fps\n");
				ret = imx462_write(imx462->i2c_client, slctReslnVGA_60fps, ARRAY_SIZE(slctReslnVGA_60fps));
			}
			break;
		case IMX462_MODE_HD:
			if(imx462->framerate_index == 30) {
				dev_info(&imx462->i2c_client->dev, "Stream ON - 720p @30fps\n");
				ret = imx462_write(imx462->i2c_client, slctReslnHD_30fps, ARRAY_SIZE(slctReslnHD_30fps));
			}
			else {
				dev_info(&imx462->i2c_client->dev, "Stream ON - 720p @60fps\n");
				ret = imx462_write(imx462->i2c_client, slctReslnHD_60fps, ARRAY_SIZE(slctReslnHD_60fps));
			}
			break;
		case IMX462_MODE_FHD:
		default:
			if(imx462->framerate_index == 30) {
				dev_info(&imx462->i2c_client->dev, "Stream ON - 1080p @30fps\n");
				ret = imx462_write(imx462->i2c_client, slctReslnFHD_30fps, ARRAY_SIZE(slctReslnFHD_30fps));
			}
			else {
				dev_info(&imx462->i2c_client->dev, "Stream ON - 1080p @60fps\n");
				ret = imx462_write(imx462->i2c_client, slctReslnFHD_60fps, ARRAY_SIZE(slctReslnFHD_60fps));
			}
			break;
	}
	if (ret != 0) {
		dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	if(imx462->streaming == true)
		ret = imx462_write(imx462->i2c_client, setCamMode, ARRAY_SIZE(setCamMode));
	else
		ret = imx462_write(imx462->i2c_client, setCamOff, ARRAY_SIZE(setCamOff));
	if (ret != 0) {
		dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ctrlcmd[2] = 0x02;
	ctrlcmd[3] = 0x20;
	ctrlcmd[4] = (uint8_t) imx462->contrast->val;
	ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));		// CONTRAST
	if (ret != 0) {
		dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ctrlcmd[3] = 0x22;
	ctrlcmd[4] = (uint8_t) imx462->sharpness->val;
	ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));		// SHARPNESS
	if (ret != 0) {
		dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ctrlcmd[3] = 0x0C;
	if (imx462->denoise == V4L2_DENOISE_MENU_ON)
		ctrlcmd[4] = 0x01;
	else
		ctrlcmd[4] = 0x00;
	ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));		// DENOISE
	if (ret != 0) {
		dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ctrlcmd[3] = 0x21;
	ctrlcmd[4] = (uint8_t) imx462->saturation->val;
	ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));		// SATURATION
	if (ret != 0) {
		dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ctrlcmd[3] = 0x01;
	ctrlcmd[4] = (uint8_t) imx462->zoom->val;
	ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));		// ZOOM ABSOLUTE
	if (ret != 0) {
		dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ctrlcmd[2] = 0x06;
	ctrlcmd[3] = 0x02;
	if(imx462->wb_mode == 0x00) {
		ctrlcmd[4] = 0x01;
		ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));	// AWB ON
		if (ret != 0) {
			dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}
	} else {
		ctrlcmd[4] = 0x02;
		ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));	// AWB OFF
		ctrlcmd[3] = 0x03;
		ctrlcmd[4] = (uint8_t) imx462->wb_mode;
		ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));	// WB PRESET
		if (ret != 0) {
			dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}
	}

	ctrlcmd[2] = 0x03;
	ctrlcmd[3] = 0x01;
	if(imx462->exposure_mode == V4L2_EXPOSURE_MENU_AUTO) {
		ctrlcmd[4] = 0x01;
		ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));	// AE ON
		if (ret != 0) {
			dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}
	} else {
		ctrlcmd[4] = 0x00;
		ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));	// AE OFF
		if (ret != 0) {
			dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}

		ctrlcmd[3] = 0x14;
		ctrlcmd[4] = (uint8_t) (imx462->exposure_value->val >> 8);
		ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));	// EXP VALUE
		if (ret != 0) {
			dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}
		ctrlcmd[3] = 0x15;
		ctrlcmd[4] = (uint8_t) (imx462->exposure_value->val);
		ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
		if (ret != 0) {
			dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}

		ctrlcmd[3] = 0x06;
		if(imx462->antiBanding_mode == V4L2_POWER_LINE_FREQUENCY_MENU_OFF)
			ctrlcmd[4] = 0x04;
		else if(imx462->antiBanding_mode == V4L2_POWER_LINE_FREQUENCY_MENU_50HZ)
			ctrlcmd[4] = 0x01;
		else if(imx462->antiBanding_mode == V4L2_POWER_LINE_FREQUENCY_MENU_60HZ)
			ctrlcmd[4] = 0x02;
		else
			ctrlcmd[4] = 0x00;
		ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));	// ANTIBANDING
		if (ret != 0) {
			dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}

		ctrlcmd[3] = 0x0D;
		ctrlcmd[4] = 0x01;
		ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
		if (ret != 0) {
			dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
					__LINE__, ret);
			return ret;
		}
	}
	ctrlcmd[3] = 0x21;
	ctrlcmd[4] = (uint8_t) imx462->brightness->val;
	ret = imx462_write(imx462->i2c_client, ctrlcmd, ARRAY_SIZE(ctrlcmd));		// Brightness
	if (ret != 0) {
		dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	return ret;

}


int camera_initialization(struct imx462 *imx462)
{
        struct i2c_client *client = imx462->i2c_client;  
	struct device *dev = &client->dev;
	uint32_t mipi_lane = 0, mipi_clk = 0;
	int ret;
	int retry = 5, fwUpdate_retry;

	ret = of_property_read_u32(dev->of_node, "camera-mipi-clk", &mipi_clk);
	if (ret) {
		dev_err(dev, "could not get camera-mipi-clk\n");
		return ret;
	}

	ret = of_property_read_u32(dev->of_node,"camera-mipi-lanes", &mipi_lane);
	if (ret) {
		dev_err(dev, "could not get camera-mipi-lanes\n");
		return ret;
	}

//      get_isp_logs(client);

	/* Reset ISP */
	for(retry = 0; retry <= RETRY_COUNT; retry++)
	{
         	toggle_gpio(imx462->rst_gpio, 10);
		ret = isp_fw_version_check(client);
		if(ret < 0) {
			if(retry < RETRY_COUNT) {
				dev_info(&client->dev, "Retrying FW version read\n");
				msleep(100);
				continue;
			}
			dev_err(&client->dev, "Error in reading the firmware version\n");
			ret=UPDATE_NEEDED;
		}
		break;
	}

retry_fwUpdate:
	/* ISP FW Update */
	if(ret == UPDATE_NEEDED) {
         	toggle_gpio(imx462->rst_gpio, 10);
		ret = isp_fw_update(client);
         	toggle_gpio(imx462->rst_gpio, 10);
		ret = isp_fw_version_check(client);
		if(ret == UPDATE_NEEDED) {
			if(fwUpdate_retry <= RETRY_COUNT) {
				dev_info(&client->dev, "Retrying fw_update\n");
				msleep(100);
				fwUpdate_retry++;
				goto retry_fwUpdate;
			}
			dev_err(&client->dev, "Error in updating the firmware\n");
			return ret;
		}
	} else
		dev_info(&client->dev, "ISP has the latest firmware\n");
	msleep(100);

//      get_isp_logs(client);

	for(retry = 0; retry <= RETRY_COUNT; retry++)
	{
		ret = isp_start_firmware(client);
		if(ret < 0) {
			if(retry < RETRY_COUNT) {
				dev_info(&client->dev, "Retrying starting the firmware\n");
				msleep(100);
				continue;
			}
			dev_err(&client->dev, "Error in starting the firmware\n");
			return ret;
		}
		break;
	}

	msleep(100);


	ret = imx462_ctrls_init(imx462);
	if (ret)
		return ret;


	v4l2_i2c_subdev_init(&imx462->sd, client, &imx462_subdev_ops);

	imx462->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	imx462->pad.flags = MEDIA_PAD_FL_SOURCE;
	imx462->sd.dev = &client->dev;
	imx462->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	imx462->framerate_index = 60; //default 60 FPS
	imx462->pix.width  = 1920; //default width
	imx462->pix.height = 1080; //default height

	ret = media_entity_pads_init(&imx462->sd.entity, 1, &imx462->pad);
	if (ret < 0) {
		dev_err(dev, "could not register media entity\n");
		goto free_ctrl;
	}

	ret = imx462_s_power(&imx462->sd, true);
	if (ret < 0) {
		dev_err(dev, "could not power up IMX462\n");
		goto free_entity;
	}

	imx462_s_power(&imx462->sd, false);


	ret = v4l2_async_register_subdev(&imx462->sd);
	if (ret < 0) {
		dev_err(dev, "could not register v4l2 device\n");
		goto free_entity;
	}


	for(retry = 0; retry <= RETRY_COUNT; retry++)
	{
		ret = imx462_set_defaults(client);
		if(ret < 0) {
			if(retry < RETRY_COUNT) {
				dev_info(&client->dev, "Retrying setting the default settings\n");
				msleep(100);
				continue;
			}
			dev_err(&client->dev, "Error in setting the defaults\n");
			return ret;
		}
		break;
	}

	imx462_entity_init_cfg(&imx462->sd, NULL);

return 0;

free_entity:
	media_entity_cleanup(&imx462->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&imx462->ctrls);

return -EINVAL;
      
}


static int imx462_set_power_on(struct imx462 *imx462)
{
	int ret;

	ret = regulator_bulk_enable(IMX462_NUM_SUPPLIES, imx462->supplies);
	if (ret < 0)
		return ret;

	ret = clk_prepare_enable(imx462->xclk);
	if (ret < 0) {
		dev_err(imx462->dev, "clk prepare enable failed\n");
		regulator_bulk_disable(IMX462_NUM_SUPPLIES, imx462->supplies);
		return ret;
	}
	msleep(20);

	return 0;
}

static void imx462_set_power_off(struct imx462 *imx462)
{
	clk_disable_unprepare(imx462->xclk);
	regulator_bulk_disable(IMX462_NUM_SUPPLIES, imx462->supplies);
}

static int imx462_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx462 *imx462 = container_of(ctrl->handler,
					     struct imx462, ctrls);
	struct i2c_client *client = imx462->i2c_client;

	int ret = 0;
	u16 mode;
		
	debug_printk("DEBUG: %s ctrl->id=0x%x ctrl->val=%d\n",__func__,ctrl->id,ctrl->val);
	
	/* lock semaphore */
//	mutex_lock(&mcu_i2c_mutex);

	mode = imx462->streamcap.capturemode;
	switch (ctrl->id) {
		case V4L2_CID_CONTRAST:
			if((ctrl->val >= ctrl(CONTRAST, min))
					&& (ctrl->val <= ctrl(CONTRAST, max))) {
				imx462->contrast->val = ctrl->val;
				ret = imx462_contrast(client, imx462->contrast->val);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_SHARPNESS:
			if((ctrl->val >= ctrl(SHARPNESS, min))
					&& (ctrl->val <= ctrl(SHARPNESS, max))) {
				imx462->sharpness->val = ctrl->val;
				ret = imx462_sharpness(client, imx462->sharpness->val);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_DENOISE:
			if ((ctrl->val >= ctrl(DENOISE, min))
					&& (ctrl->val <= ctrl(DENOISE, max))) {
				imx462->denoise->val = ctrl->val;
				ret = imx462_denoise(client, imx462->denoise->val);
				
			} else
				return -EINVAL;
			break;

		case V4L2_CID_GAIN:
			if(imx462->exposure_mode == V4L2_EXPOSURE_MENU_AUTO) {
				printk(KERN_ERR "Gain doesn't work in Auto Exposure mode, change to manual exposure mode\n");
				break;
			}
			if ((ctrl->val >= ctrl(GAIN, min))
					&& (ctrl->val <= ctrl(GAIN, max))) {
				imx462->gain->val = ctrl->val;
				ret = imx462_gain(client, imx462->gain->val);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_EXPOSURE_AUTO:
			if ((ctrl->val >= ctrl(EXPOSURE_AUTO, min)) &&
					(ctrl->val <= ctrl(EXPOSURE_AUTO, max))) {
				imx462->exposure_mode = ctrl->val;
				ret = imx462_auto_exposure(client, imx462->exposure_mode);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_EXPOSURE_ABSOLUTE:
			if(imx462->exposure_mode == V4L2_EXPOSURE_MENU_AUTO) {
				printk(KERN_ERR "In Auto mode, change to manual exposure mode\n");
				break;
			}
			if ((ctrl->val >= ctrl(EXPOSURE_ABSOLUTE, min)) &&
					(ctrl->val <= ctrl(EXPOSURE_ABSOLUTE, max))) {
				imx462->exposure_value->val = ctrl->val;
				ret = imx462_exposure_absolute(client, imx462->exposure_value->val);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_POWER_LINE_FREQUENCY:
			if((ctrl->val >= ctrl(POWER_LINE_FREQUENCY, min)) &&
					(ctrl->val <= ctrl(POWER_LINE_FREQUENCY, max))) {
				imx462->antiBanding_mode = ctrl->val;
				ret = imx462_powerline_frequency(client, imx462->antiBanding_mode);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
			if((ctrl->val >= ctrl(AUTO_N_PRESET_WHITE_BALANCE, min)) &&
					(ctrl->val <= ctrl(AUTO_N_PRESET_WHITE_BALANCE, max))) {
				imx462->wb_mode = ctrl->val;
				ret = imx462_white_balance(client, imx462->wb_mode);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_ZOOM_ABSOLUTE:
			if((ctrl->val >= ctrl(ZOOM_ABSOLUTE, min))
					&& (ctrl->val <= ctrl(ZOOM_ABSOLUTE, max))) {
				imx462->zoom->val = ctrl->val;
				ret = imx462_zoom(client, imx462->zoom->val);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_SATURATION:
			if((ctrl->val >= ctrl(SATURATION, min))
					&& (ctrl->val <= ctrl(SATURATION, max))) {
				imx462->saturation->val = ctrl->val;
				ret = imx462_saturation(client, imx462->saturation->val);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_BRIGHTNESS:
			if((ctrl->val >= ctrl(BRIGHTNESS, min))
					&& (ctrl->val <= ctrl(BRIGHTNESS, max))) {
				imx462->brightness->val = ctrl->val;
				ret = imx462_brightness(client, imx462->brightness->val);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_FOCUS_AUTO:
			if((ctrl->val >= ctrl(FOCUS_AUTO, min)) 
					&& (ctrl->val <= ctrl(FOCUS_AUTO, max))) {
				imx462->auto_focus = ctrl->val;
				ret = imx462_autofocus(client, imx462->auto_focus);
			} else
				return -EINVAL;
			break;

		case V4L2_CID_AUTO_FOCUS_RANGE:
			if(imx462->auto_focus != V4L2_AF_CONTINUOUS) {
				printk(KERN_ERR "Auto Focus Range works only on continuous auto focus, change to continuous auto focus mode\n");
				break;
			}
			if((ctrl->val >= ctrl(AUTO_FOCUS_RANGE, min)) 
					&& (ctrl->val <= ctrl(AUTO_FOCUS_RANGE, max))) {
				imx462->af_range = ctrl->val;
				ret = imx462_caf_range(client, imx462->af_range); 
			} else
				return -EINVAL;
			break;

		case V4L2_CID_FOCUS_ABSOLUTE:
			if((ctrl->val >= ctrl(FOCUS_ABSOLUTE, min)) 
					&& (ctrl->val <= ctrl(FOCUS_ABSOLUTE, max))) {
				imx462->manual_focus = ctrl->val;
				ret = imx462_manual_focus_range(client, imx462->manual_focus); 
			} else
				return -EINVAL;
			break;

		case V4L2_CID_FRAMERATE:
			if((ctrl->val >= ctrl(FRAMERATE, min)) 
					&& (ctrl->val <= ctrl(FRAMERATE, max))) {
				imx462->framerate_index = ctrl->val;
				imx462_configure_framerate(client , imx462);
			} else
				return -EINVAL;
			break;		
		
		default:
			ret = -EINVAL;
	}
	if (ret != 0) {
		dev_err(&imx462->i2c_client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		dev_info(&imx462->i2c_client->dev, "Reconfiguring ISP...\n");
		ret = imx462_reconfigure(imx462);
		if (ret)
			dev_err(&imx462->i2c_client->dev,"Error in restoring the settings\n");
	}

	/* unlock semaphore */
//	mutex_unlock(&mcu_i2c_mutex);
	return ret;
}

#if 0
static int imx462_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int index, ctrl_index, ctrl_id;
	bool next_ctrl = (qc->id & V4L2_CTRL_FLAG_NEXT_CTRL);
	int num_ctrls = IMX462_NUM_CONTROLS;  

	if (sd == NULL || qc == NULL)
		return -EINVAL;

	if(next_ctrl) {
		ctrl_id = qc->id & (~V4L2_CTRL_FLAG_NEXT_CTRL);

		/*
		 * Ignore the V4L2_CTRL_FLAG_NEXT_COMPOUND for now
		 */
		ctrl_id = ctrl_id & (~V4L2_CTRL_FLAG_NEXT_COMPOUND);
	}
	else {
		/*
		 * Assume we've just got the control ID itself
		 * directly.
		 */
		ctrl_id = qc->id;	
	}

	if(ctrl_id) {
		for(index = 0; index < num_ctrls; index++) {
			if(ctrl_list[index].ctrl_id == ctrl_id) {
				ctrl_index = (next_ctrl) ? index + 1 : index;
				//ctrl_index = index;
				break;
			}
		}

		if(index == num_ctrls) {
			/*
			 * We do not know about this control
			 */
			return -EINVAL;
		}
		else if (next_ctrl && index == num_ctrls - 1)
		{
			/*
			 * We've got a request for the control
			 * after the last one.
			 */
			return -EINVAL;
		}
	}
	else if (next_ctrl) {
		ctrl_index = 0;
	}
	else {
		return -EINVAL;
	}

	if(ctrl_list[ctrl_index].ctrl_type == CTRL_STANDARD) {
		qc->id = ctrl_list[ctrl_index].ctrl_id;
		strcpy(qc->name, ctrl_list[ctrl_index].ctrl_ui_info.ctrl_name);
		qc->type = ctrl_list[ctrl_index].ctrl_ui_info.ctrl_ui_type;
		qc->flags = ctrl_list[ctrl_index].ctrl_ui_info.ctrl_ui_flags;

		qc->minimum = ctrl_list[ctrl_index].ctrl_data.std.ctrl_min;
		qc->maximum = ctrl_list[ctrl_index].ctrl_data.std.ctrl_max;
		qc->step = ctrl_list[ctrl_index].ctrl_data.std.ctrl_step;
		qc->default_value = ctrl_list[ctrl_index].ctrl_data.std.ctrl_def;
	}
	else {
		return -EINVAL;
	}

	return 0;
}
#endif

static int imx462_s_power(struct v4l2_subdev *sd, int on)
{
	struct imx462 *imx462 = container_of(sd, struct imx462, sd);
	int ret = 0;


	mutex_lock(&mcu_i2c_mutex);

	if (imx462->power_count == !on) {
		if (on) {
			ret = imx462_set_power_on(imx462);
			if (ret < 0)
				goto exit;
			usleep_range(500, 1000);
		} else {
			imx462_set_power_off(imx462);
		}
	}

	imx462->power_count += on ? 1 : -1;

exit:
	mutex_unlock(&mcu_i2c_mutex);

	return ret;
}


static int imx462_enum_framesizes(struct v4l2_subdev *sd,
			       struct v4l2_subdev_state *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
{

	if (fse->code != MEDIA_BUS_FMT_UYVY8_2X8)
		return -EINVAL;

	if (fse->index >= ARRAY_SIZE(imx462_mode_info_data))
		return -EINVAL;

	fse->min_width = imx462_mode_info_data[fse->index].width;
	fse->max_width = imx462_mode_info_data[fse->index].width;
	fse->min_height = imx462_mode_info_data[fse->index].height;
	fse->max_height = imx462_mode_info_data[fse->index].height;

	return 0;
}

static int imx462_enum_mbus_code(struct v4l2_subdev *sd,
			    struct v4l2_subdev_state *cfg,
			    struct v4l2_subdev_mbus_code_enum *code)
{

	if (code->index > 0)
		return -EINVAL;

	code->code = MEDIA_BUS_FMT_UYVY8_2X8;

	return 0;
}

static struct v4l2_mbus_framefmt *
__imx462_get_pad_format(struct imx462 *imx462,
			struct v4l2_subdev_state *cfg,
			unsigned int pad,
			enum v4l2_subdev_format_whence which)
{
	switch (which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		return v4l2_subdev_get_try_format(&imx462->sd, cfg, pad);
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		return &imx462->fmt;
	default:
		return NULL;
	}
}


static int imx462_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx462 *imx462 = container_of(sd, struct imx462, sd);

	if (format->pad)
		return -EINVAL;

	memset(mf, 0, sizeof(struct v4l2_mbus_framefmt));

	mf->code = imx462_colour_fmts[0].code;
	mf->colorspace = imx462_colour_fmts[0].colorspace;
//	mf->colorspace = V4L2_COLORSPACE_SRGB;
	mf->width = imx462->pix.width;
	mf->height = imx462->pix.height;
	mf->field = V4L2_FIELD_NONE;

	dev_info(&client->dev, "%s code=0x%x, w/h=(%d,%d), colorspace=%d, field=%d\n",
		__func__, mf->code, mf->width, mf->height, mf->colorspace, mf->field);

	return 0;
}


static struct v4l2_rect *
__imx462_get_pad_crop(struct imx462 *imx462, struct v4l2_subdev_state *cfg,
		      unsigned int pad, enum v4l2_subdev_format_whence which)
{
	switch (which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		return v4l2_subdev_get_try_crop(&imx462->sd, cfg, pad);
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		return &imx462->crop;
	default:
		return NULL;
	}
}


static int imx462_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *cfg,
		struct v4l2_subdev_format *format)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx462 *imx462 = container_of(sd, struct imx462, sd);
	int ret = 0, err = 0;
        const struct imx462_mode_info *new_mode;
	struct v4l2_mbus_framefmt *__format;
	struct v4l2_rect *__crop;

	/* lock semaphore */
//	mutex_lock(&mcu_i2c_mutex);

	__crop = __imx462_get_pad_crop(imx462, cfg, format->pad,
			format->which);

	new_mode = v4l2_find_nearest_size(imx462_mode_info_data,
			       ARRAY_SIZE(imx462_mode_info_data),
			       width, height,
			       format->format.width, format->format.height);

	__crop->width = new_mode->width;
	__crop->height = new_mode->height;

	if (format->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
	
		ret = v4l2_ctrl_s_ctrl_int64(imx462->pixel_clock,
					     new_mode->pixel_clock);
		if (ret < 0)
			return ret;

		ret = v4l2_ctrl_s_ctrl(imx462->link_freq,
				       new_mode->link_freq);
		if (ret < 0)
			return ret;

		imx462->current_mode = new_mode;
	}


	imx462->pix.width  = new_mode->width;
	imx462->pix.height = new_mode->height;

	__format = __imx462_get_pad_format(imx462, cfg, format->pad,
			format->which);
	__format->width = __crop->width;
	__format->height = __crop->height;
	__format->code = MEDIA_BUS_FMT_UYVY8_2X8;
	__format->field = V4L2_FIELD_NONE;
	__format->colorspace = V4L2_COLORSPACE_SRGB;
	 format->format = *__format;


	if(new_mode->width == imx462_frmsizes[IMX462_MODE_VGA].width) {
		imx462->streamcap.capturemode = IMX462_MODE_VGA;
	}
	else if(new_mode->width == imx462_frmsizes[IMX462_MODE_FHD].width) {
		imx462->streamcap.capturemode = IMX462_MODE_FHD;
	}
	else {
		imx462->streamcap.capturemode = IMX462_MODE_HD;
	}

	switch(imx462->streamcap.capturemode){
		case IMX462_MODE_VGA:
			if(imx462->framerate_index == 30) {
				dev_info(&client->dev, "Stream ON - 480p @30fps\n");
				err = imx462_write(client, slctReslnVGA_30fps, ARRAY_SIZE(slctReslnVGA_30fps));
			}
			else {
				dev_info(&client->dev, "Stream ON - 480p @60fps\n");
				err = imx462_write(client, slctReslnVGA_60fps, ARRAY_SIZE(slctReslnVGA_60fps));
			}
			break;
		case IMX462_MODE_HD:
			if(imx462->framerate_index == 30) {
				dev_info(&client->dev, "Stream ON - 720p @30fps\n");
				err = imx462_write(client, slctReslnHD_30fps, ARRAY_SIZE(slctReslnHD_30fps));
			}
			else {
				dev_info(&client->dev, "Stream ON - 720p @60fps\n");
				err = imx462_write(client, slctReslnHD_60fps, ARRAY_SIZE(slctReslnHD_60fps));
			}
			break;
		case IMX462_MODE_FHD:
		default:
			if(imx462->framerate_index == 30) {
				dev_info(&client->dev, "Stream ON - 1080p @30fps\n");
				err = imx462_write(client, slctReslnFHD_30fps, ARRAY_SIZE(slctReslnFHD_30fps));
			}
			else {
				dev_info(&client->dev, "Stream ON - 1080p @60fps\n");
				err = imx462_write(client, slctReslnFHD_60fps, ARRAY_SIZE(slctReslnFHD_60fps));
			}
			break;
	}

	if (err != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		dev_info(&imx462->i2c_client->dev, "Reconfiguring ISP\n");
		ret = imx462_reconfigure(imx462);
		if (ret) {
			dev_err(&imx462->i2c_client->dev,"Error in restoring the settings\n");
			return ret;
		}
	}

	/* unlock semaphore */
//	mutex_unlock(&mcu_i2c_mutex);

	return ret;
}

static int imx462_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
{
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;

	return 0;
}

#if 0
static int imx462_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx462 *imx462 = to_imx462(client);
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	u32 tgt_fps;
	int mode = a->parm.capture.capturemode;
	int ret = 0;

	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		/* Check that the new frame rate is allowed. */
		if ((timeperframe->numerator == 0) ||
		    (timeperframe->denominator == 0)) {
			timeperframe->denominator = DEFAULT_FPS;
			timeperframe->numerator = 1;
		}

		tgt_fps = timeperframe->denominator /
			  timeperframe->numerator;

		if (tgt_fps > MAX_FPS) {
			timeperframe->denominator = MAX_FPS;
			timeperframe->numerator = 1;
		} else if (tgt_fps < MIN_FPS) {
			timeperframe->denominator = MIN_FPS;
			timeperframe->numerator = 1;
		}
		switch(a->parm.capture.timeperframe.denominator) {
			case MIN_FPS:
				imx462->framerate_index = 30;
				//imx462_write(client, slctFrate30, ARRAY_SIZE(slctFrate30));
				break;
			case MAX_FPS:
				imx462->framerate_index = 60;
				//imx462_write(client, slctFrate60, ARRAY_SIZE(slctFrate60));
				break;
		}

		if (mode > IMX462_MODE_MAX || mode < IMX462_MODE_MIN) {
			pr_err("The camera mode[%d] is not supported!\n", mode);
			return -EINVAL;
		}

		imx462->streamcap.capturemode = mode;
		imx462->streamcap.timeperframe = *timeperframe;
		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		pr_debug("   type is not V4L2_BUF_TYPE_VIDEO_CAPTURE but %d\n",
					a->type);
		ret = -EINVAL;
		break;

	default:
		pr_debug("   type is unknown - %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int imx462_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx462 *imx462 = to_imx462(client);
	struct v4l2_captureparm *cparm = &a->parm.capture;
        int ret = 0;

        switch (a->type) {
        /* This is the only case currently handled. */
        case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
                memset(a, 0, sizeof(*a));
                a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                cparm->capability = imx462->streamcap.capability;
                cparm->timeperframe = imx462->streamcap.timeperframe;
                cparm->capturemode = imx462->streamcap.capturemode;
                ret = 0;
                break;

        /* These are all the possible cases. */
        case V4L2_BUF_TYPE_VIDEO_OUTPUT:
        case V4L2_BUF_TYPE_VIDEO_OVERLAY:
        case V4L2_BUF_TYPE_VBI_CAPTURE:
        case V4L2_BUF_TYPE_VBI_OUTPUT:
        case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
        case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
                ret = -EINVAL;
                break;

        default:
                pr_debug("   type is unknown - %d\n", a->type);
                ret = -EINVAL;
                break;
        }
        return ret;
}
#endif

static int imx462_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
        debug_printk("DEBUG: %s",__func__);

	return 0;
}

static int imx462_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
        debug_printk("DEBUG: %s",__func__);

	return 0;
}

static int imx462_s_stream(struct v4l2_subdev *sd, int enable)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct imx462 *imx462 = container_of(sd, struct imx462, sd);
	int ret = 0;

	/* lock semaphore */
//	mutex_lock(&mcu_i2c_mutex);

	imx462->streaming = enable;

	ret = v4l2_ctrl_handler_setup(&imx462->ctrls);
	if (ret < 0) {
		dev_err(&client->dev, "could not sync v4l2 controls\n");
		return ret;
	}

	if (enable) {

		if (ret != 0)
			goto exit_streamConfig;

		/* Stream ON */
		ret = imx462_write(client, setCamMode, ARRAY_SIZE(setCamMode));
		if (ret != 0)
			goto exit_streamConfig;
		if(imx462->antiBanding_mode == V4L2_POWER_LINE_FREQUENCY_MENU_AUTO)
			imx462_write(client, flickerDetectCmd, ARRAY_SIZE(flickerDetectCmd));

	} else {
		/* Stream OFF */
		dev_info(&client->dev, "Stream OFF\n");
		ret = imx462_write(client, setCamOff, ARRAY_SIZE(setCamOff));
		if (ret != 0)
			goto exit_streamConfig;
		goto exit_sStream;
	}

exit_streamConfig:

	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		dev_info(&client->dev, "Reconfiguring ISP\n");
		ret = imx462_reconfigure(imx462);
		if (ret) {
			dev_err(&client->dev,"Error in restoring the settings\n");
			goto exit_sStream;
		}
	}
exit_sStream:
	/* unlock semaphore */
//	mutex_unlock(&mcu_i2c_mutex);

	dev_dbg(&client->dev, "%s--\n", __func__);
	return ret;

}

# if 0
static int imx462_set_retain_ctrls(struct i2c_client *client)
{
	int ret = 0;
	struct imx462 *imx462 = to_imx462(client);

	printk("default camera ctrls setting\n");
	ctrlcmd[2] = 0x0B;
	ctrlcmd[3] = 0x2C;
	ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	ret = imx462_brightness(client, imx462->brightness->val);
	ret = imx462_contrast(client, imx462->contrast->val);
	ret = imx462_saturation(client, imx462->saturation->val);
	ret = imx462_sharpness(client, imx462->sharpness->val);
	ret = imx462_white_balance(client, imx462->wb_mode);
	ret = imx462_gain(client, imx462->gain->val);
	ret = imx462_auto_exposure(client, imx462->exposure_mode);
	ret = imx462_zoom(client, imx462->zoom->val);
	ret = imx462_roll(client);

	ctrlcmd[2] = 0x03;
	ctrlcmd[3] = 0x00;
	ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ctrlcmd[3] = 0x0D;
	ctrlcmd[4] = 0x01;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));	// EXP VALUE
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ret = imx462_powerline_frequency(client, imx462->antiBanding_mode);

	ctrlcmd[2] = 0x06;
	ctrlcmd[3] = 0x00;
	ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
			__LINE__, ret);
		return ret;
	}

	ctrlcmd[3] = 0x0C;
	ctrlcmd[4] = 0x00;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
			__LINE__, ret);
		return ret;
	}
	ret = imx462_autofocus(client, imx462->auto_focus);
	ret = imx462_caf_range(client, imx462->af_range);

	return ret;
}
#endif

static int imx462_set_defaults(struct i2c_client *client)
{

	int ret = 0;

	ret = imx462_write(client, slctSensr, ARRAY_SIZE(slctSensr));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ret = imx462_write(client, slctFrate60, ARRAY_SIZE(slctFrate60));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ret = imx462_write(client, rawPreviewOFF, ARRAY_SIZE(rawPreviewOFF));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ret = imx462_write(client, slctFmt, ARRAY_SIZE(slctFmt));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	ctrlcmd[2] = 0x02;
	ctrlcmd[3] = 0x20;
	ctrlcmd[4] = 0x04;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	ctrlcmd[3] = 0x21;
	ctrlcmd[4] = 0x28;
	ret = imx462_write(client, ctrlcmd, ARRAY_SIZE(ctrlcmd));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	return ret;
}

static int imx462_ctrls_init(struct imx462 *imx462)
{
	struct i2c_client *client = imx462->i2c_client;
	int numctrls = IMX462_NUM_CONTROLS;

	v4l2_ctrl_handler_init(&imx462->ctrls, numctrls);

	imx462->brightness = v4l2_ctrl_new_std(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_BRIGHTNESS,
			0, 255, 1, 110);

	imx462->contrast = v4l2_ctrl_new_std(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_CONTRAST,
			0, 10, 1, 5);

	imx462->saturation = v4l2_ctrl_new_std(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_SATURATION,
			1, 63, 1, 32);

	imx462->gain = v4l2_ctrl_new_std(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_GAIN,
			0, 255, 1, 128);

	/*have to revisit this. V4l2 standing mapping of antibanding  and our firmware mapping are not matching. auto and disabled are swapped*/
	v4l2_ctrl_new_std_menu(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_POWER_LINE_FREQUENCY,
			V4L2_CID_POWER_LINE_FREQUENCY_AUTO,0,V4L2_CID_POWER_LINE_FREQUENCY_50HZ);

	imx462->sharpness = v4l2_ctrl_new_std(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_SHARPNESS,
			0, 4, 1, 2);

	v4l2_ctrl_new_std_menu(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_EXPOSURE_AUTO,
			V4L2_EXPOSURE_MANUAL,0,V4L2_EXPOSURE_AUTO);

	imx462->exposure_value = v4l2_ctrl_new_std(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_EXPOSURE_ABSOLUTE,
			2, 100000, 1, 156);

	imx462->zoom = v4l2_ctrl_new_std(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_ZOOM_ABSOLUTE,
			1, 30, 1, 1);

	/*have to revisit this. V4l2 standing mapping of WB enum and our firmware mapping are not matching*/
	v4l2_ctrl_new_std_menu(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			7,0,V4L2_WHITE_BALANCE_AUTO);

       /*Non Standard V4L2 Controls  */
	imx462->denoise = v4l2_ctrl_new_custom(&imx462->ctrls,
			&imx462_custom_controls[0],NULL);

	imx462->framerate = v4l2_ctrl_new_custom(&imx462->ctrls,
			&imx462_custom_controls[1],NULL);

	imx462->pixel_clock = v4l2_ctrl_new_std(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_PIXEL_RATE,
			1, INT_MAX, 1, 1);

	imx462->link_freq = v4l2_ctrl_new_int_menu(&imx462->ctrls,
			&imx462_ctrl_ops,
			V4L2_CID_LINK_FREQ,
			ARRAY_SIZE(link_freq) - 1,
			0, link_freq);

	if (imx462->link_freq)
		imx462->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx462->sd.ctrl_handler = &imx462->ctrls;

	if(imx462->ctrls.error)
	{
		dev_err(&client->dev, "%s: control initialization error %d\n",
				__func__, imx462->ctrls.error);
		return -EINVAL;
	}

	return 0;

}

static int isp_start_firmware(struct i2c_client *client)
{

	uint8_t rx_data[2];
	int ret = 0;

	/* intrFactorRd */
	ret = imx462_write(client, irFactor2, ARRAY_SIZE(irFactor2));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	ret = imx462_read(client, rx_data, 2);
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	if(rx_data[1] != 0x00) {
		dev_err(&client->dev," %s(%d) Interrupt not received - %d \n", __func__,
				__LINE__, ret);
		return -EIO;
	}

	/* glRdFwStartIntr */
	ret = imx462_write(client, glRdFwStartIntr, ARRAY_SIZE(glRdFwStartIntr));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	ret = imx462_read(client, rx_data, 2);	// O/P :- 0x02,0x00
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	if(rx_data[1] != 1 || rx_data[0] != 2) {
		dev_err(&client->dev, "Bad interrupt start value - %d -- %d\n", rx_data[1], rx_data[0]);
		return -EIO;
	}

	/* Int start */
	ret = imx462_write(client, intStartcmd, ARRAY_SIZE(intStartcmd));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	return ret;
}


static int isp_fw_update(struct i2c_client *client)
{
	uint8_t rx_data[2];
	uint32_t startAddress = 0;
	int ret = 0, sector = 0, block = 0;
	bool isSectorErase;

	/* Check for Boot interrupt */
	ret = imx462_write(client, irFactor1, ARRAY_SIZE(irFactor1));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	ret = imx462_read(client, rx_data, 2);
	if(ret == 0) {
		if(rx_data[1] != 0x01) {
			dev_info(&client->dev, "%d: Failure in reading irFactor1 interrupt\n", __LINE__);
			return -EIO;
		}
	}
	else {
		dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	/* Set PLL value */
	ret = imx462_write(client, setPLL, ARRAY_SIZE(setPLL));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	/* Start RAM */
	ret = imx462_write(client, startRAM, ARRAY_SIZE(startRAM));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	/* Check for SRAM interrupt */
	msleep(2);
	ret = imx462_write(client, irFactor1, ARRAY_SIZE(irFactor1));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	ret = imx462_read(client, rx_data, 2);
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	if(rx_data[1] != 0x04) {
		dev_info(&client->dev, "%d: Failure in reading irFactor1 interrupt\n", __LINE__);
		return -EIO;
	}

	/* Sector erase */
	isSectorErase = true;
	startAddress = SECTOR_START_ADDR;
	for(sector = FIRST_SECTOR; sector < TOTAL_SECTORS; sector++) {
		ret = romErase(client, startAddress, isSectorErase);
		if(ret < 0) {
			dev_err(&client->dev, "Error in erasing the sector%d\n", sector);
			return ret;
		}
		startAddress += SECTOR_START_ADDR;
	}

	/* Block erase */
	isSectorErase = false;
	startAddress = BLOCK_START_ADDR;
	for(block = FIRST_BLOCK; block < TOTAL_BLOCKS; block++) {
		ret = romErase(client, startAddress, isSectorErase);
		if(ret < 0) {
			dev_err(&client->dev, "Error in erasing the block%d\n", block);
			return ret;
		}
		startAddress += BLOCK_START_ADDR;
	}

	/* Flash the ROM */
	dev_info(&client->dev, "Updating ISP Firmware......\n");
	ret = romFlash(client);
	if(ret < 0) {
		dev_err(&client->dev, "Error in flashing the ROM\n");
		return ret;
	}
	dev_info(&client->dev, "FW Successfully Updated\n");
	ret = calcChecksum(client);
	if(ret < 0) {
		dev_err(&client->dev, "Error in calculating the checksum\n");
		return ret;
	}
	dev_info(&client->dev, "CheckSum = %d\n", ret);
	return ret;
}

static int isp_fw_version_check(struct i2c_client *client)
{
	uint8_t fw_version[VERSION_SIZE];
	int ret = 0;

	ret = imx462_write(client, intROMcmd1, ARRAY_SIZE(intROMcmd1));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	ret = imx462_write(client, fwReadcmd, ARRAY_SIZE(fwReadcmd));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}
	ret = imx462_read(client, fw_version, VERSION_SIZE);
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Read Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	dev_info(&client->dev, "ISP Firmware Version - %d.%d\n", fw_version[27], fw_version[28]);
	ret = imx462_write(client, intROMcmd2, ARRAY_SIZE(intROMcmd2));
	if (ret != 0) {
		dev_err(&client->dev," %s(%d) ISP Write Error - %d \n", __func__,
				__LINE__, ret);
		return ret;
	}

	if(fw_version[27] == bootData[24] && fw_version[28] == bootData[25])
		return NO_UPDATE;
		//return UPDATE_NEEDED;
	else {
		dev_info(&client->dev, "New ISP firmware(%d.%d) available\n", bootData[24], bootData[25]);
		return UPDATE_NEEDED;
	}
}


static int imx462_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct device_node *endpoint;
	struct imx462 *imx462;
	int i, ret;

	pr_info("DEBUG:  %s\n",__func__);

	imx462 = devm_kzalloc(dev, sizeof(struct imx462), GFP_KERNEL);
	if (!imx462)
		return -ENOMEM;

	imx462->i2c_client = client;
	imx462->dev = dev;

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!endpoint) {
		dev_err(dev, "endpoint node not found\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(endpoint),
					 &imx462->ep);
	if (ret < 0) {
		dev_err(dev, "parsing endpoint node failed\n");
		return ret;
	}

	of_node_put(endpoint);
	if (ret < 0) {
		dev_err(dev, "parsing endpoint node failed\n");
		return ret;
	}

	if (imx462->ep.bus_type != V4L2_MBUS_CSI2_DPHY) {
		dev_err(dev, "invalid bus type, must be CSI2\n");
		return -EINVAL;
	}

	for (i = 0; i < IMX462_NUM_SUPPLIES; i++)
		imx462->supplies[i].supply = imx462_supply_name[i];

	ret = devm_regulator_bulk_get(dev, IMX462_NUM_SUPPLIES,
				      imx462->supplies);
	if (ret < 0)
		return ret;

	imx462->pwn_gpio = devm_gpiod_get(dev, "pwdn", GPIOD_OUT_HIGH);
	if (IS_ERR(imx462->pwn_gpio)) {
		dev_err(dev, "cannot get pwn-gpios\n");
		return PTR_ERR(imx462->pwn_gpio);
	}

	imx462->rst_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(imx462->rst_gpio)) {
		dev_err(dev, "cannot get reset-gpios\n");
		return PTR_ERR(imx462->rst_gpio);
	}

	mutex_init(&mcu_i2c_mutex);

	ret = camera_initialization(imx462);
	if (ret < 0)
            goto exit;

       pr_info("Imx462 probed successfully\n");

       return 0;

exit:
	imx462_s_power(&imx462->sd, false);
	mutex_destroy(&mcu_i2c_mutex);

	return ret;
}

static void imx462_remove(struct i2c_client *client)
{
	struct imx462 *imx462 = to_imx462(client);

	v4l2_async_unregister_subdev(&imx462->sd);
	media_entity_cleanup(&imx462->sd.entity);
	v4l2_ctrl_handler_free(&imx462->ctrls);
	mutex_destroy(&mcu_i2c_mutex);
}

static const struct i2c_device_id imx462_id[] = {
	{ "imx462", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, imx462_id);

static const struct of_device_id imx462_of_match[] = {
	{ .compatible = "imx462" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx462_of_match);

static struct i2c_driver imx462_driver = {
	.driver = {
		.of_match_table = of_match_ptr(imx462_of_match),
		.name  = "imx462",
	},
	.probe_new = imx462_probe,
	.remove = imx462_remove,
	.id_table = imx462_id,
};

module_i2c_driver(imx462_driver);

MODULE_DESCRIPTION("IMX462 Camera Driver");
MODULE_AUTHOR("e-consystems.com>");
MODULE_LICENSE("GPL v2");
