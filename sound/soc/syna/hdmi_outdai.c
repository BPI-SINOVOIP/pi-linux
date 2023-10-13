// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <sound/soc.h>

#include "berlin_pcm.h"
#include "berlin_util.h"
#include "berlin_spdif.h"
#include "aio_hal.h"
#include "avio_common.h"
#include "hal_vpp_wrap.h"
#include "vpp_cmd.h"

#define HDMI_PLAYBACK_RATES   (SNDRV_PCM_RATE_8000_384000)
#define HDMI_PLAYBACK_FORMATS (SNDRV_PCM_FMTBIT_S16_LE \
				| SNDRV_PCM_FMTBIT_S24_LE \
				| SNDRV_PCM_FMTBIT_S32_LE) \

struct hdmi_priv {
	struct device *dev;
	const char *dev_name;
	unsigned int irq;
	u32 chid;
	bool requested;
	void *aio_handle;
	u32 channels;
	u32 chan_mask;
	u32 sample_rate;
	u32 frame_rate;/* frame is not same as sample_rate when hbr */
	u32 hbr;
	u32 bit_depth;
	u32 i2s_dfm;
	u32 i2s_cfm;
	u32 fmt;
	u32 format;
	u8  ch_map[8];
	struct workqueue_struct *wq;
	struct delayed_work trigger_work;
	struct snd_pcm_substream *ss;
};

#define pr_item(a) snd_printd(#a" = %d\n", a)

typedef struct VPP_HDMI_AUDIO_CFG_T {
	int numChannels;
	int portNum;
	int sampFreq;
	int sampSize;
	int mClkFactor;
	int audioFmt; /* VPP_HDMI_AUDIO_FMT_T */
	int hbrAudio; /* True/False for controlling HBR audio */
} VPP_HDMI_AUDIO_CFG;

typedef struct VPP_HDMI_SPKR_ALLOC_T {
	unsigned char   FlFr  : 1; // FrontLeft/Front Right
	unsigned char   Lfe   : 1; // Low Frequency Effect
	unsigned char   Fc    : 1; // Front Center
	unsigned char   RlRr  : 1; // Rear Left/Rear Right
	unsigned char   Rc    : 1; // Rear Center
	unsigned char   FlcFrc: 1; // Front Left Center/Front Right Center
	unsigned char   RlcRrc: 1; // Rear Left Center /Rear Right Center
	unsigned char   Res   : 1;
} VPP_HDMI_SPKR_ALLOC;

// Type of input audio stream
typedef enum {
	DD_AC3 = 0,
	DD_PLUS,
	DD_TRUE_HD,
	DTS_HD,
	MPG_AUDIO,
	MP3,
	HE_AAC,
	WMA,
	WMA_PRO,
	WMA_LOSSLESS,
	RAW_PCM,
	LPCM_SD,
	LPCM_HD,
	LPCM_BD,
	LPCM_SESF,
	PL2,
	DD_DCV,
	DTS,
	DTS_MA,
	DTS_HRES,
	DTS_LBR,
	DV_SD,
	VORBIS,
	FLAC,
	RAW_AAC,
	REAL_AUD8,
	REAL_AAC,
	ADPCM,
	SPDIF_IN,
	G711A,
	G711U,
	NEO6,
	AMRNB,
	AMRWB,
	M4A_LATM,
	SRS,
	IEC61937,
	DOLBY_MAT,
	MAX_AUDIO,

	// invalid audio type; it's default of initial setting
	UNKNOW_AUDIO = -1,
} eAudioType;

enum {
	eAUD_CHMASK_UNDEFINED		= 0,

	eAUD_CHMASK_LEFT			= 0x00000001,
	eAUD_CHMASK_RGHT			= 0x00000002,
	eAUD_CHMASK_CNTR			= 0x00000004,
	eAUD_CHMASK_SRRD_LEFT		= 0x00000008,
	eAUD_CHMASK_SRRD_RGHT		= 0x00000010,
	eAUD_CHMASK_LFE 			= 0x00000020,
	eAUD_CHMASK_SRRD_CNTR		= 0x00000040,
	eAUD_CHMASK_LEFT_REAR		= 0x00000080,
	eAUD_CHMASK_RGHT_REAR		= 0x00000100,
	eAUD_CHMASK_LEFT_SIDE		= 0x00000200,
	eAUD_CHMASK_RGHT_SIDE		= 0x00000400,
	eAUD_CHMASK_LEFT_CNTR		= 0x00000800,
	eAUD_CHMASK_RGHT_CNTR		= 0x00001000,
	eAUD_CHMASK_HIGH_LEFT		= 0x00002000,
	eAUD_CHMASK_HIGH_CNTR		= 0x00004000,
	eAUD_CHMASK_HIGH_RGHT		= 0x00008000,
	eAUD_CHMASK_LEFT_WIDE		= 0x00010000,
	eAUD_CHMASK_RGHT_WIDE		= 0x00020000,
	eAUD_CHMASK_TOP_CNTR		= 0x00040000,
	eAUD_CHMASK_HIGH_SIDE_LEFT  = 0x00080000,
	eAUD_CHMASK_HIGH_SIDE_RGHT  = 0x00100000,
	eAUD_CHMASK_HIGH_REAR_CNTR  = 0x00200000,
	eAUD_CHMASK_HIGH_REAR_LEFT  = 0x00400000,
	eAUD_CHMASK_HIGH_REAR_RGHT  = 0x00800000,
	eAUD_CHMASK_LOW_FRONT_CNTR  = 0x01000000,
	eAUD_CHMASK_LOW_FRONT_LEFT  = 0x02000000,
	eAUD_CHMASK_LOW_FRONT_RGHT  = 0x04000000,
	eAUD_CHMASK_LFE2			= 0x08000000,

	eAUD_CHMASK_DUAL_MONO_A 	= 0x10000000,
	eAUD_CHMASK_DUAL_MONO_B 	= 0x20000000,

	eAUD_CHMASK_TOP_MIDDLE_LEFT = 0x40000000,
	eAUD_CHMASK_TOP_MIDDLE_RGHT = 0x80000000,

	eAUD_CHMASK_MONO			= 0x00000004,
	eAUD_CHMASK_MSUR			= 0x00000008,
	eAUD_CHMASK_LEFT_TOTAL  	= 0x00000001,
	eAUD_CHMASK_RGHT_TOTAL  	= 0x00000002,
};

typedef enum VPP_HDMI_AUDIO_FMT_T {
	VPP_HDMI_AUDIO_FMT_UNDEF = 0x00,
	VPP_HDMI_AUDIO_FMT_PCM   = 0x01,
	VPP_HDMI_AUDIO_FMT_AC3,
	VPP_HDMI_AUDIO_FMT_MPEG1,
	VPP_HDMI_AUDIO_FMT_MP3,
	VPP_HDMI_AUDIO_FMT_MPEG2,
	VPP_HDMI_AUDIO_FMT_AAC,
	VPP_HDMI_AUDIO_FMT_DTS,
	VPP_HDMI_AUDIO_FMT_ATRAC,
	VPP_HDMI_AUDIO_FMT_ONE_BIT_AUDIO,
	VPP_HDMI_AUDIO_FMT_DOLBY_DIGITAL_PLUS,
	VPP_HDMI_AUDIO_FMT_DTS_HD,
	VPP_HDMI_AUDIO_FMT_MAT,
	VPP_HDMI_AUDIO_FMT_DST,
	VPP_HDMI_AUDIO_FMT_WMA_PRO,
} VPP_HDMI_AUDIO_FMT;

#define HDMI_AUDIOCFG_PARAM_SIZE	 (sizeof(VPP_HDMI_AUDIO_CFG) + sizeof(int))
#define HDMI_AUDIOVUCCFG_PARAM_SIZE  (sizeof(VPP_HDMI_AUDIO_VUC_CFG) + sizeof(int))
#define HDMI_AUDIO_INFO_SIZE		 (sizeof(VPP_HDMI_SPKR_ALLOC) + 2 * sizeof(int))

static u32 GET_PEAudioType2HDMIType(u32 audioFmt, int *DecType)
{
	switch (audioFmt) {
	case DD_AC3:
		*DecType = VPP_HDMI_AUDIO_FMT_AC3;
		break;

	case DD_PLUS:
		*DecType = VPP_HDMI_AUDIO_FMT_DOLBY_DIGITAL_PLUS;
		break;

	case DD_TRUE_HD:
		*DecType = VPP_HDMI_AUDIO_FMT_MAT;
		break;

	case LPCM_SD:
		*DecType = VPP_HDMI_AUDIO_FMT_PCM;
		break;

	case LPCM_BD:
		*DecType = VPP_HDMI_AUDIO_FMT_PCM;
		break;

	case LPCM_HD:
		*DecType = VPP_HDMI_AUDIO_FMT_PCM;
		break;

	case DTS:
		*DecType = VPP_HDMI_AUDIO_FMT_DTS;
		break;

	case DTS_HD:
	case DTS_HRES:
		*DecType = VPP_HDMI_AUDIO_FMT_DTS_HD;
		break;

	case MPG_AUDIO:
		*DecType = VPP_HDMI_AUDIO_FMT_MPEG2;
		break;

	case MP3:
		*DecType = VPP_HDMI_AUDIO_FMT_MP3;
		break;

	case HE_AAC:
	case RAW_AAC:
		*DecType = VPP_HDMI_AUDIO_FMT_AAC;
		break;

	case WMA:
		*DecType = VPP_HDMI_AUDIO_FMT_WMA_PRO;
		break;

	case WMA_PRO:
		*DecType = VPP_HDMI_AUDIO_FMT_WMA_PRO;
		break;

	case WMA_LOSSLESS:
		*DecType = VPP_HDMI_AUDIO_FMT_WMA_PRO;
		break;

	case RAW_PCM:
		*DecType = VPP_HDMI_AUDIO_FMT_PCM;
		break;

	case DD_DCV:
		*DecType = VPP_HDMI_AUDIO_FMT_DOLBY_DIGITAL_PLUS;
		break;

	case DOLBY_MAT:
		*DecType = VPP_HDMI_AUDIO_FMT_MAT;
		break;

	default:
		*DecType = VPP_HDMI_AUDIO_FMT_UNDEF;
		snd_printk("not support audio fmt %d\n", audioFmt);
		return -EINVAL;
	}
	return 0;
}

static u32 GET_SndChMap2VppChMask(u32 ch_map)
{
	switch (ch_map) {
	case SNDRV_CHMAP_FL:
		return eAUD_CHMASK_LEFT;
	case SNDRV_CHMAP_FR:
		return eAUD_CHMASK_RGHT;
	case SNDRV_CHMAP_RL:
		return eAUD_CHMASK_SRRD_LEFT;
	case SNDRV_CHMAP_RR:
		return eAUD_CHMASK_SRRD_RGHT;
	case SNDRV_CHMAP_FC:
		return eAUD_CHMASK_CNTR;
	case SNDRV_CHMAP_LFE:
		return eAUD_CHMASK_LFE;
	case SNDRV_CHMAP_SL:
		return eAUD_CHMASK_LEFT_SIDE;
	case SNDRV_CHMAP_SR:
		return eAUD_CHMASK_RGHT_SIDE;
	case SNDRV_CHMAP_RC:
		return eAUD_CHMASK_SRRD_CNTR;
	case SNDRV_CHMAP_RLC:
		return eAUD_CHMASK_LEFT_REAR;
	case SNDRV_CHMAP_RRC:
		return eAUD_CHMASK_RGHT_REAR;
	default:
		snd_printk("not support ch map %d\n", ch_map);
		break;
	}

	return 0;
}

static u32 hdmi_get_div(u32 fs)
{
	u32 div;

	switch (fs) {
	case eAUD_Freq_8K:
	case eAUD_Freq_11K:
	case eAUD_Freq_12K:
		div = AIO_DIV32;
		break;
	case eAUD_Freq_16K:
	case eAUD_Freq_22K:
	case eAUD_Freq_24K:
		div = AIO_DIV16;
		break;
	case eAUD_Freq_32K:
	case eAUD_Freq_44K:
	case eAUD_Freq_48K:
		div = AIO_DIV8;
		break;
	case eAUD_Freq_64K:
	case eAUD_Freq_88K:
	case eAUD_Freq_96K:
		div = AIO_DIV4;
		break;
	case eAUD_Freq_176K:
	case eAUD_Freq_192K:
		div = AIO_DIV2;
		break;
	default:
		div = AIO_DIV8;
		break;
	}

	return div;
}

static void hdmi_set_channel_mask(struct hdmi_priv *hdmi)
{
	//use the default FL|FR
	if (hdmi->channels <= 2)
		return;

	/* ch_map[hdmi->channels - 1]!=0 means the channel mapping has been upated*/
	if (hdmi->ch_map[hdmi->channels - 1]) {
		int i;

		hdmi->chan_mask = 0;
		for (i = 0; i < hdmi->channels; i++)
			hdmi->chan_mask |= GET_SndChMap2VppChMask(hdmi->ch_map[i]);
		snd_printd("Update channel mask to %#x\n", hdmi->chan_mask);
	} else {
		/* use the default channel mask,  currently only support the below
		 * continuous channel mappings:
		 * 1) 2 channel: FL + FR
		 * 2) 3 channel: FL + FR + LFE
		 * 3) 4 channel: FL + FR + LFE + FC
		 * 4) 5 channel: FL + FR + LFE + FC + BC
		 * 5) 6 channel: FL + FR + LFE + FC + LS + RS
		 * 6) 7 channel: FL + FR + LFE + FC + LS + RS + BC
		 * 6) 8 channel: FL + FR + LFE + FC + LS + RS + RLC + RRC
		 * if want to support more input mapping, it needs to transfer the input channel
		 * mapping to our hdmi channel mapping and adjust the pcm channels
		 */
		if (hdmi->channels >= 3)
			hdmi->chan_mask |= eAUD_CHMASK_LFE;
		if (hdmi->channels >= 4)
			hdmi->chan_mask |= eAUD_CHMASK_CNTR;
		if (hdmi->channels == 5 || hdmi->channels == 7)
			hdmi->chan_mask |= eAUD_CHMASK_SRRD_CNTR;
		if (hdmi->channels >= 6)
			hdmi->chan_mask |= (eAUD_CHMASK_SRRD_LEFT | eAUD_CHMASK_SRRD_RGHT);
		if (hdmi->channels == 8)
			hdmi->chan_mask |= (eAUD_CHMASK_LEFT_REAR | eAUD_CHMASK_RGHT_REAR);
	}
}


void hdmi_set_samplerate(struct hdmi_priv *hdmi, int freq, int hbr)
{
	int div;

	berlin_set_pll(hdmi->aio_handle, AIO_APLL_1, freq);
	aio_i2s_set_clock(hdmi->aio_handle, AIO_ID_HDMI_TX,
		 1, AIO_CLK_D3_SWITCH_NOR, AIO_CLK_SEL_D8, AIO_APLL_1, 1);

	div = hdmi_get_div(freq);
	snd_printd("aio_setclkdiv hdmi. div %d", div);
	aio_setclkdiv(hdmi->aio_handle, AIO_ID_HDMI_TX, div);
}

static int Disp_SetHdmiAudioFmt(int enable, VPP_HDMI_AUDIO_CFG *pAudioCfg)
{
	u32 pAudioCfgData[HDMI_AUDIOCFG_PARAM_SIZE];
	int ret = 0;
	snd_printd("%s\n", __func__);

	pAudioCfgData[0] = enable;
	memcpy(&pAudioCfgData[1], pAudioCfg, sizeof(VPP_HDMI_AUDIO_CFG));
	ret= wrap_MV_VPPOBJ_InvokePassShm_Helper(&pAudioCfgData,
				SET_HDMI_AUDIOCFG_PARAM, HDMI_AUDIOCFG_PARAM_SIZE);
	if (ret < 0) {
		snd_printk("set hdmi audio fmt fail %d\n", ret);
		ret = -EINVAL;
	}
	return ret;
}

static int Disp_SetHdmiAudioInfo(void *pHDMISpkrMap, u32 lvlShiftVal,
						   u32 downMixInhFlag)
{
	u32 pSpkrMapData[HDMI_AUDIO_INFO_SIZE];
	int ret;

	snd_printd("%s\n", __func__);

	pSpkrMapData[0] = lvlShiftVal;
	pSpkrMapData[1] = downMixInhFlag;
	memcpy(&pSpkrMapData[2], pHDMISpkrMap, sizeof(VPP_HDMI_SPKR_ALLOC));
	ret = wrap_MV_VPPOBJ_InvokePassShm_Helper(&pSpkrMapData,
				SET_HDMI_AUDIO_INFO, HDMI_AUDIO_INFO_SIZE);

	if (ret < 0) {
		snd_printk("set hdmi audio info fail %d\n", ret);
		ret = -EINVAL;
	}
	return ret;
}

static void set_portfmt(struct hdmi_priv *out,
	u32 audio_format, u32 data_fmt, u32 width_word,
	u32 width_sample, u32 uiChanNum, u32 uiSampleSize)
{
	/*< HBR Audio source selection
	 * 00 : HD data, compressed audio(2 samples {2pair of Left & Right} per 64-bit data from DDR) transmitted in 2 LRCK
	 * 01: L-PCM data (1 Sample {one pair of Left &Right} per 64-bit data from DDR) transmitted in 1 LRCK
	 * 10: HD data (4 samples {4pair of Left & Right } per 128-bit data from DDR) transmitter in 1 LRCK
	 * 11: 8 channel LPCM data (4 samples {4pair of Left & Right } per 256-bit data from DDR) transmitted in 1 LRCK
	 */
	#define AIO_HDMI_HD_SRC_64b 	0b00
	#define AIO_HDMI_LPCM_SRC_64b   0b01
	#define AIO_HDMI_HD_SRC_128b	0b10
	#define AIO_HDMI_LPCM_SRC_256b  0b11

	aio_selhdport(out->aio_handle, 0);
	aio_set_ctl(out->aio_handle, AIO_ID_HDMI_TX,
		data_fmt, width_word, width_sample);

	if (audio_format == RAW_PCM) {
		if (uiChanNum > HDMI_STEREO_CHANNEL_NUM) {
			if (uiSampleSize == 16)
				aio_selhdsource(out->aio_handle, AIO_HDMI_HD_SRC_128b);
			else
				aio_selhdsource(out->aio_handle, AIO_HDMI_LPCM_SRC_256b);
		} else {
			if (uiSampleSize == 16)
				aio_selhdsource(out->aio_handle, AIO_HDMI_HD_SRC_64b);
			else
				aio_selhdsource(out->aio_handle, AIO_HDMI_LPCM_SRC_64b);
		}
	} else {
		if (uiChanNum > HDMI_STEREO_CHANNEL_NUM)
			aio_selhdsource(out->aio_handle, AIO_HDMI_LPCM_SRC_256b);
		else
			aio_selhdsource(out->aio_handle, AIO_HDMI_LPCM_SRC_64b);
	}
}

static int hdmi_audioformat_en(bool en)
{
	int ret = 0;

	ret = wrap_MV_VPPOBJ_EnableHdmiAudioFmt(en);
	if (ret < 0) {
		snd_printk("VPP_CA_EnableHdmiAudioFmt failed\n");
		ret = -EINVAL;
	}
	return ret;
}

int hdmi_set_audioformat(u32 channel_numbers, u32 channel_mask,
	 u32 fs, u32 samp_size, u32 clkFactor, u32 audioFmt, u32 hbrAudio, u32 enable)
{
	u8 FlFr = 0, Lfe = 0, Fc = 0, RlRr = 0, Rc = 0, FlcFrc = 0, RlcRrc = 0;
	int i, nChans, mask;
	VPP_HDMI_AUDIO_CFG HDMIAudioCfg;
	int ret;

	nChans = 0;
	mask = 0x1;
	for (i = 0; i < 32; i++) {
		mask = channel_mask & (1 << i);
		if (mask) {
			switch (mask) {
			case eAUD_CHMASK_LEFT:
			case eAUD_CHMASK_RGHT:
				FlFr = 1;
				break;
			case eAUD_CHMASK_CNTR:
				Fc = 1;
				FlFr = 1;
				break;
			case eAUD_CHMASK_SRRD_LEFT:
			case eAUD_CHMASK_SRRD_RGHT:
				RlRr = 1;
				break;
			case eAUD_CHMASK_LFE:
			case eAUD_CHMASK_LFE2:
				Lfe = 1;
				break;
			case eAUD_CHMASK_SRRD_CNTR:
				Rc = 1;
				break;
			case eAUD_CHMASK_LEFT_REAR:
			case eAUD_CHMASK_RGHT_REAR:
				RlcRrc = 1;
				break;
			default:
				break;
			}

			nChans++;
			if (nChans >= channel_numbers)
				break;
		}
	}

	// Fix channel map as 3/2.1 except for L/R case
	if (!(FlFr && !Lfe && !Fc && !RlRr && !RlcRrc))
		FlFr = Lfe = Fc = RlRr = 1;

	channel_numbers = 0;
	if (FlFr)
		channel_numbers += 2;
	if (Lfe)
		channel_numbers += 1;
	if (Fc)
		channel_numbers += 1;
	if (RlRr)
		channel_numbers += 2;
	if (Rc)
		channel_numbers += 1;
	if (FlcFrc)
		channel_numbers += 2;
	if (RlcRrc)
		channel_numbers += 2;
	/* for compress data, channel number is set to 8 */
	switch (audioFmt) {
	default:
	case VPP_HDMI_AUDIO_FMT_UNDEF:
	case VPP_HDMI_AUDIO_FMT_PCM:
		break;
	case VPP_HDMI_AUDIO_FMT_AC3:
	case VPP_HDMI_AUDIO_FMT_MPEG1:
	case VPP_HDMI_AUDIO_FMT_MP3:
	case VPP_HDMI_AUDIO_FMT_MPEG2:
	case VPP_HDMI_AUDIO_FMT_AAC:
	case VPP_HDMI_AUDIO_FMT_DTS:
	case VPP_HDMI_AUDIO_FMT_ATRAC:
	case VPP_HDMI_AUDIO_FMT_ONE_BIT_AUDIO:
	case VPP_HDMI_AUDIO_FMT_DOLBY_DIGITAL_PLUS:
	case VPP_HDMI_AUDIO_FMT_DST:
		channel_numbers = 2;
		FlFr = 1;
		Lfe = Fc = RlRr = Rc = FlcFrc = RlcRrc = 0;
		hbrAudio = false;
		break;
	case VPP_HDMI_AUDIO_FMT_DTS_HD:
	case VPP_HDMI_AUDIO_FMT_MAT:
	case VPP_HDMI_AUDIO_FMT_WMA_PRO:
		hbrAudio = true;
		channel_numbers = 2;
		FlFr = 1;
		Lfe = Fc = RlRr = Rc = FlcFrc = RlcRrc = 0;
		break;
	}

	snd_printd("%s:channel numbers %d, fs %d, samp_size %d, clkFactor %d, audioFmt %d, "
		 "hbr %d,FlFr %d, Lfe %d, Fc %d, RlRr %d, Rc %d, FlcFrc %d RlcRrc %d\n",
		 __func__, channel_numbers, fs, samp_size, clkFactor, audioFmt,
		 hbrAudio, FlFr, Lfe, Fc, RlRr, Rc, FlcFrc, RlcRrc);
	HDMIAudioCfg.numChannels = channel_numbers;
	HDMIAudioCfg.portNum = 0;
	HDMIAudioCfg.sampFreq = fs;
	HDMIAudioCfg.sampSize = samp_size;
	HDMIAudioCfg.mClkFactor = clkFactor;
	HDMIAudioCfg.audioFmt = audioFmt;
	HDMIAudioCfg.hbrAudio = hbrAudio;
	ret = Disp_SetHdmiAudioFmt(enable, &HDMIAudioCfg);
	if (ret < 0)
		return ret;

	if (enable) {
		VPP_HDMI_SPKR_ALLOC HDMISpkrMap;

		HDMISpkrMap.FlFr = FlFr;
		HDMISpkrMap.Lfe = Lfe;
		HDMISpkrMap.Fc = Fc;
		HDMISpkrMap.RlRr = RlRr;
		HDMISpkrMap.Rc = Rc;
		HDMISpkrMap.FlcFrc = FlcFrc;
		HDMISpkrMap.RlcRrc = RlcRrc;
		ret = Disp_SetHdmiAudioInfo(&HDMISpkrMap, 0, 0);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int set_hdmi_audio_fmt(struct hdmi_priv *hdmi)
{
	u32 fs, samp_size, clk_factor, mfs;
	u32 i2s_mode, i2s_DFM, i2s_CFM, channel_numbers, uiBitDepth;
	u32 audioFmt;
	int ret = 0;
	/*
	 * HDMI IP changed,
	 * this clk_factor is bclk_factor,
	 * bclk = bclk_factor * fs;
	 * normal I2S , fix it to 64,
	 * means only support 64 bitclk in 1 LRCLK
	 * means only support 32 frame width
	 */
	clk_factor = 64;

	fs = hdmi->sample_rate;
	channel_numbers = hdmi->channels;
	uiBitDepth = hdmi->bit_depth;
	mfs = hdmi->frame_rate;

	if (hdmi->hbr || hdmi->fmt == DD_PLUS) {
		if (4 * fs != mfs) {
			snd_printk("wrong fs %d, mfs %d\n", fs, mfs);
			return -EINVAL;
		}
	}

	if (hdmi->fmt != RAW_PCM) {
		i2s_mode = AIO_I2S_MODE;
		i2s_DFM = AIO_32DFM;
		i2s_CFM = AIO_32CFM;
		samp_size = 21;
		channel_numbers = hdmi->hbr ? 8 : 2;
	} else {
		if (channel_numbers <= HDMI_PCM_MAX_CHANNEL_NUM) {
			i2s_mode = AIO_I2S_MODE;
			switch (uiBitDepth) {
			case 16:
				i2s_DFM = AIO_16DFM;
				i2s_CFM = AIO_32CFM;
				samp_size = 16;
				break;
			case 24:
			case 32:
				i2s_DFM = AIO_32DFM;
				i2s_CFM = AIO_32CFM;
				samp_size = 32;
				break;
			default:
				snd_printk("HDMI PCM channelNum[%u] not support the bit depth[%u] !",
						  channel_numbers, uiBitDepth);
				return -EINVAL;
			}
		} else {
			snd_printd("HDMI PCM channelNum[%u]! use defaulted stereo", channel_numbers);
			channel_numbers = HDMI_STEREO_CHANNEL_NUM;
			i2s_mode = AIO_I2S_MODE;
			i2s_DFM = AIO_24DFM;
			i2s_CFM = AIO_32CFM;
			samp_size = 24;
		}
		snd_printd("HDMI PCM: ChanNr[%u], BitDepth[%u], I2SMode[%u], I2SDFM[%u], I2SCFM[%u], SampSize[%d]\n",
				 channel_numbers, uiBitDepth, i2s_mode, i2s_DFM, i2s_CFM, samp_size);
	}
	hdmi->i2s_dfm = i2s_DFM;
	hdmi->i2s_cfm = i2s_CFM;

	hdmi_set_samplerate(hdmi, mfs, hdmi->hbr);
	snd_printd("HDMI: ChanNr[%u], BitDepth[%u], I2SMode[%u], I2SDFM[%u], I2SCFM[%u], SampSize[%d]\n",
			 channel_numbers, uiBitDepth, i2s_mode, i2s_DFM, i2s_CFM, samp_size);
	set_portfmt(hdmi, hdmi->fmt, i2s_mode, i2s_CFM, i2s_DFM, channel_numbers, samp_size);

	GET_PEAudioType2HDMIType(hdmi->fmt, &audioFmt);
	ret = hdmi_set_audioformat(channel_numbers, hdmi->chan_mask,
							fs, samp_size, clk_factor, audioFmt,
							hdmi->hbr, true);
	if (ret < 0)
		return ret;

	//Enable HDMI TX audio
	ret = hdmi_audioformat_en(true);

	return ret;
}

static struct snd_kcontrol_new berlin_outdai_ctrls[] = {
	//TODO: add dai control here
};

static int berlin_outdai_startup(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	return 0;
}

static void berlin_outdai_shutdown(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{

}

static int berlin_outdai_setfmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct hdmi_priv *outdai = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	case SND_SOC_DAIFMT_IB_NF:
		break;
	case SND_SOC_DAIFMT_IB_IF:
		break;
	default:
		dev_err(outdai->dev, "MASK not found in fmt 0x%x\n", fmt);
		ret = -EINVAL;
	}

	return ret;
}

struct hdmi_priv *g_hdmi = NULL;
void hdmi_port_int_enable(void)
{
	struct hdmi_priv *hdmi = g_hdmi;
	struct snd_pcm_substream *substream = hdmi->ss;

	aio_set_aud_ch_flush(hdmi->aio_handle, AIO_ID_HDMI_TX, AIO_TSD0, AUDCH_CTRL_FLUSH_OFF);
	aio_set_aud_ch_en(hdmi->aio_handle, AIO_ID_HDMI_TX, AIO_TSD0, AUDCH_CTRL_ENABLE_ENABLE);
	/* wait for hw done */
	usleep_range(1000, 1500);
	aio_set_aud_ch_mute(hdmi->aio_handle, AIO_ID_HDMI_TX, AIO_TSD0, AUDCH_CTRL_MUTE_MUTE_OFF);
	berlin_pcm_hdmi_bitstream_start(substream);
}
EXPORT_SYMBOL(hdmi_port_int_enable);

static void trigger_hdmi(struct work_struct *work)
{
	struct hdmi_priv *hdmi =
		container_of(work, struct hdmi_priv, trigger_work.work);
	struct snd_pcm_substream *substream = hdmi->ss;
	u32 data_type;
	int ret;

	snd_printd("%s: dainame %s, starting...\n", __func__, hdmi->dev_name);
	if (berline_pcm_passthrough_check(substream, &data_type)) {
		snd_printd("IEC61937 data type %d\n", data_type);
		switch (data_type) {
		case BURST_AC3:
			snd_printd("AC3 stream");
			hdmi->fmt = DD_AC3;
			hdmi->sample_rate = hdmi->frame_rate;
			break;
		case BURST_DDP:
			snd_printd("DDP stream");
			hdmi->fmt = DD_PLUS;
			hdmi->sample_rate = hdmi->frame_rate >> 2;
			break;
		case BURST_MAT:
			snd_printd("MAT stream");
			hdmi->fmt = DOLBY_MAT;
			hdmi->sample_rate = hdmi->frame_rate >> 2;
			hdmi->hbr = true;
			break;
		default:
			snd_printk("burst type %d not supported\n", data_type);
			break;
		}
	} else {
		snd_printd("PCM stream");
		hdmi->fmt = RAW_PCM;
		hdmi_set_channel_mask(hdmi);
	}

	ret = set_hdmi_audio_fmt(hdmi);
	if (ret < 0) {
		dev_err(hdmi->dev, "hdmi audio fmt set fail\n");
		return;
	}
	g_hdmi = hdmi;
	pr_item(hdmi->irq);
	pr_item(hdmi->chid);
	pr_item(hdmi->channels);
	pr_item(hdmi->chan_mask);
	pr_item(hdmi->sample_rate);
	pr_item(hdmi->frame_rate);/* frame is not same as sample_rate when hbr */
	pr_item(hdmi->hbr);
	pr_item(hdmi->bit_depth);
	pr_item(hdmi->i2s_dfm);
	pr_item(hdmi->i2s_cfm);
	pr_item(hdmi->fmt);
	snd_printd("kick off hdmi...\n");
}

static int berlin_outdai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct hdmi_priv *hdmi = snd_soc_dai_get_drvdata(dai);
	u32 fs = params_rate(params);
	int ret = 0;
	struct berlin_ss_params ssparams;

	ssparams.irq_num = 1;
	ssparams.chid_num = 1;
	ssparams.mode = HDMIO_MODE;
	ssparams.irq = &hdmi->irq;
	ssparams.dev_name = hdmi->dev_name;
	ret = berlin_pcm_request_dma_irq(substream, &ssparams);
	if (ret == 0)
		hdmi->requested = true;
	else
		return ret;
	aio_set_aud_ch_mute(hdmi->aio_handle, AIO_ID_HDMI_TX, AIO_TSD0, 1);


	hdmi->channels = params_channels(params);
	INIT_DELAYED_WORK(&hdmi->trigger_work, trigger_hdmi);
	hdmi->wq = alloc_workqueue("berlin_hdmi_dai_que", WQ_HIGHPRI | WQ_UNBOUND, 1);
	if (hdmi->wq == NULL) {
		ret = -ENOMEM;
		return ret;
	}

	pr_info("%s:%s:sample_rate:%d channels:%d format:%s, %d\n", __func__,
		 hdmi->dev_name, params_rate(params), params_channels(params),
		   snd_pcm_format_name(params_format(params)), params_format(params));

	hdmi->sample_rate = fs;
	hdmi->bit_depth = params_width(params);
	hdmi->format = params_format(params);
	hdmi->frame_rate = fs;
	memset(hdmi->ch_map, 0, sizeof(hdmi->ch_map));
	hdmi->ch_map[0] = SNDRV_CHMAP_FL;
	hdmi->ch_map[1] = SNDRV_CHMAP_FR;
	hdmi->chan_mask = eAUD_CHMASK_LEFT | eAUD_CHMASK_RGHT;
	hdmi->fmt = RAW_PCM;
	/* Disable HDMI TX audio output then delay to avoid pop noise */
	aio_i2s_set_clock(hdmi->aio_handle, AIO_ID_HDMI_TX,
		1, AIO_CLK_D3_SWITCH_NOR, AIO_CLK_SEL_D8, AIO_APLL_1, 1);
	ret = hdmi_audioformat_en(false);
	if (ret < 0) {
		dev_err(hdmi->dev, "hdmi audio fmt set fail\n");
		return ret;
	}
	usleep_range(5000, 5100);
	return ret;
}

static int berlin_outdai_hw_free(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct hdmi_priv *outdai = snd_soc_dai_get_drvdata(dai);

	aio_set_aud_ch_flush(outdai->aio_handle, AIO_ID_HDMI_TX, AIO_TSD0, 1);
	aio_set_aud_ch_en(outdai->aio_handle, AIO_ID_HDMI_TX, AIO_TSD0, 0);
	aio_set_aud_ch_mute(outdai->aio_handle, AIO_ID_HDMI_TX, AIO_TSD0, AUDCH_CTRL_MUTE_MUTE_ON);
	usleep_range(5000, 5100);

	if (outdai->requested && outdai->irq >= 0) {
		berlin_pcm_free_dma_irq(substream, 1, &outdai->irq);
		outdai->requested = false;
	}

	if (outdai->wq) {
		cancel_delayed_work_sync(&outdai->trigger_work);
		destroy_workqueue(outdai->wq);
		outdai->wq = NULL;
	}
	return 0;
}

static int berlin_outdai_trigger(struct snd_pcm_substream *substream,
				 int cmd, struct snd_soc_dai *dai)
{
	struct hdmi_priv *hdmi = snd_soc_dai_get_drvdata(dai);

	snd_printd("%s: dainame %s, cmd: %d\n", __func__, hdmi->dev_name, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		hdmi->ss = substream;
		/* berlin_outdai_trigger run in isr context, TA function call failed
		 * so use work queque to set hdmi format
		 */
		queue_delayed_work(hdmi->wq, &hdmi->trigger_work, 0);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		hdmi->fmt = RAW_PCM;
		hdmi->sample_rate = hdmi->frame_rate;
		hdmi->hbr = false;
		aio_set_aud_ch_flush(hdmi->aio_handle, AIO_ID_HDMI_TX, AIO_TSD0, AUDCH_CTRL_FLUSH_ON);
		aio_set_aud_ch_en(hdmi->aio_handle, AIO_ID_HDMI_TX, AIO_TSD0, AUDCH_CTRL_ENABLE_DISABLE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int berlin_outdai_dai_probe(struct snd_soc_dai *dai)
{
	snd_soc_add_dai_controls(dai, berlin_outdai_ctrls,
				 ARRAY_SIZE(berlin_outdai_ctrls));
	return 0;
}

static int berlin_outdai_set_channel_map(struct snd_soc_dai *dai,
				   unsigned int tx_num, unsigned int *tx_slot,
				   unsigned int rx_num, unsigned int *rx_slot)
{
	struct hdmi_priv *outdai = snd_soc_dai_get_drvdata(dai);
	int i;

	for (i = 0; i < tx_num; i++)
		outdai->ch_map[i] = tx_slot[i];
	snd_printd("%s: tx_num(%u): %u %u %u %u %u %u %u %u \n", __func__, tx_num,
		tx_slot[0], tx_slot[1], tx_slot[2], tx_slot[3],
		tx_slot[4], tx_slot[5], tx_slot[6], tx_slot[7]);

	return 0;
}

static int berlin_outdai_get_channel_map(struct snd_soc_dai *dai,
				   unsigned int *tx_num, unsigned int *tx_slot,
				   unsigned int *rx_num, unsigned int *rx_slot)
{
	struct hdmi_priv *outdai = snd_soc_dai_get_drvdata(dai);
	int i;

	for (i = 0; i < 8; i++)
		tx_slot[i] = outdai->ch_map[i];
	*tx_num = 8;
	snd_printd("%s: %u %u %u %u %u %u %u %u \n", __func__,
		tx_slot[0], tx_slot[1], tx_slot[2], tx_slot[3],
		tx_slot[4], tx_slot[5], tx_slot[6], tx_slot[7]);

	return 0;
}

static struct snd_soc_dai_ops berlin_dai_outdai_ops = {
	.startup   = berlin_outdai_startup,
	.set_fmt   = berlin_outdai_setfmt,
	.hw_params = berlin_outdai_hw_params,
	.hw_free   = berlin_outdai_hw_free,
	.trigger   = berlin_outdai_trigger,
	.shutdown  = berlin_outdai_shutdown,
	.set_channel_map = berlin_outdai_set_channel_map,
	.get_channel_map = berlin_outdai_get_channel_map,
};

static struct snd_soc_dai_driver berlin_outdai_dai = {
	.name = "berlin-hdmi-outdai",
	.probe = berlin_outdai_dai_probe,
	.playback = {
		.stream_name = "HdmiPlayback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = HDMI_PLAYBACK_RATES,
		.formats = HDMI_PLAYBACK_FORMATS,
	},
	.ops = &berlin_dai_outdai_ops,
};

static const struct snd_soc_component_driver berlin_outdai_component = {
	.name = "hdmi-outdai",
};

static int hdmi_outdai_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hdmi_priv *outdai;
	int irq, ret;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	outdai = devm_kzalloc(dev, sizeof(struct hdmi_priv),
				  GFP_KERNEL);
	if (!outdai)
		return -ENOMEM;
	outdai->dev_name = dev_name(dev);
	outdai->dev = dev;

	/*open aio handle for alsa*/
	outdai->aio_handle = open_aio(outdai->dev_name);
	if (unlikely(outdai->aio_handle == NULL)) {
		snd_printk("aio_handle:%p  get failed\n", outdai->aio_handle);
		return -EBUSY;
	}
	dev_set_drvdata(dev, outdai);

	irq = platform_get_irq_byname(pdev, "hdmi");
	if (irq > 0) {
		outdai->irq = irq;
		outdai->chid = irqd_to_hwirq(irq_get_irq_data(irq));
		snd_printd("get hdmi irq %d for node %s\n",
			   irq, pdev->name);
	}

	ret = devm_snd_soc_register_component(dev,
						  &berlin_outdai_component,
						  &berlin_outdai_dai, 1);
	if (ret) {
		snd_printk("failed to register DAI: %d\n", ret);
		return ret;
	}
	snd_printd("%s: done i2s [%d %d]\n", __func__,
		   outdai->irq, outdai->chid);
	return ret;
}

static int hdmi_outdai_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hdmi_priv *outdai;

	outdai = (struct hdmi_priv *)dev_get_drvdata(dev);

	/*close aio handle of alsa if have opened*/
	if (outdai && outdai->aio_handle) {
		close_aio(outdai->aio_handle);
		outdai->aio_handle = NULL;
	}

	return 0;
}

static const struct of_device_id hdmi_outdai_dt_ids[] = {
	{ .compatible = "syna,vs640-hdmi",  },
	{ .compatible = "syna,vs680-hdmi",  },
	{}
};
MODULE_DEVICE_TABLE(of, hdmi_outdai_dt_ids);

static struct platform_driver hdmi_outdai_driver = {
	.probe = hdmi_outdai_probe,
	.remove = hdmi_outdai_remove,
	.driver = {
		.name = "syna-hdmi-outdai",
		.of_match_table = hdmi_outdai_dt_ids,
	},
};
module_platform_driver(hdmi_outdai_driver);

MODULE_DESCRIPTION("Synaptics HDMI ALSA output dai");
MODULE_ALIAS("platform:hdmi-outdai");
MODULE_LICENSE("GPL v2");
