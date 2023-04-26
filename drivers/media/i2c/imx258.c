// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Intel Corporation

#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <asm/unaligned.h>

#define IMX258_REG_VALUE_08BIT		1
#define IMX258_REG_VALUE_16BIT		2

#define IMX258_REG_MODE_SELECT		0x0100
#define IMX258_MODE_STANDBY		0x00
#define IMX258_MODE_STREAMING		0x01

/* Chip ID */
#define IMX258_REG_CHIP_ID		0x0016
#define IMX258_CHIP_ID			0x0258

/* V_TIMING internal */
#define IMX258_VTS_30FPS		0x0c50
#define IMX258_VTS_30FPS_2K		0x0638
#define IMX258_VTS_30FPS_VGA		0x034c
#define IMX258_VTS_MAX			0xffff

/*Frame Length Line*/
#define IMX258_FLL_MIN			0x08a6
#define IMX258_FLL_MAX			0xffff
#define IMX258_FLL_STEP			1
#define IMX258_FLL_DEFAULT		0x0c98

/* HBLANK control - read only */
#define IMX258_PPL_DEFAULT		5352

/* Exposure control */
#define IMX258_REG_EXPOSURE		0x0202
#define IMX258_EXPOSURE_MIN		4
#define IMX258_EXPOSURE_STEP		1
#define IMX258_EXPOSURE_DEFAULT		0x640
#define IMX258_EXPOSURE_MAX		65535

/* Analog gain control */
#define IMX258_REG_ANALOG_GAIN		0x0204
#define IMX258_ANA_GAIN_MIN		0
#define IMX258_ANA_GAIN_MAX		480
#define IMX258_ANA_GAIN_STEP		1
#define IMX258_ANA_GAIN_DEFAULT		0x0

/* Digital gain control */
#define IMX258_REG_GR_DIGITAL_GAIN	0x020e
#define IMX258_REG_R_DIGITAL_GAIN	0x0210
#define IMX258_REG_B_DIGITAL_GAIN	0x0212
#define IMX258_REG_GB_DIGITAL_GAIN	0x0214
#define IMX258_DGTL_GAIN_MIN		0
#define IMX258_DGTL_GAIN_MAX		4096	/* Max = 0xFFF */
#define IMX258_DGTL_GAIN_DEFAULT	1024
#define IMX258_DGTL_GAIN_STEP		1

/* HDR control */
#define IMX258_REG_HDR			0x0220
#define IMX258_HDR_ON			BIT(0)
#define IMX258_REG_HDR_RATIO		0x0222
#define IMX258_HDR_RATIO_MIN		0
#define IMX258_HDR_RATIO_MAX		5
#define IMX258_HDR_RATIO_STEP		1
#define IMX258_HDR_RATIO_DEFAULT	0x0

/* Test Pattern Control */
#define IMX258_REG_TEST_PATTERN		0x0600

/* Orientation */
#define REG_MIRROR_FLIP_CONTROL		0x0101
#define REG_CONFIG_MIRROR_FLIP		0x00
#define REG_CONFIG_FLIP_TEST_PATTERN	0x00

/* Input clock frequency in Hz */
#define IMX258_INPUT_CLOCK_FREQ_MIN	24000000
#define IMX258_INPUT_CLOCK_FREQ		24000000
#define IMX258_INPUT_CLOCK_FREQ_MAX	24000000

#define IMX258_MBUS_FORMAT		MEDIA_BUS_FMT_SRGGB10_1X10

/* regs */
#define PLL_MULT_DRIV                  0x0310
#define IVTPXCK_DIV                    0x0301
#define IVTSYCK_DIV                    0x0303
#define PREPLLCK_VT_DIV                0x0305
#define IOPPXCK_DIV                    0x0309
#define IOPSYCK_DIV                    0x030b
#define PREPLLCK_OP_DIV                0x030d
#define PHASE_PIX_OUTEN                0x3030
#define PDPIX_DATA_RATE                0x3032
#define SCALE_MODE                     0x0401
#define SCALE_MODE_EXT                 0x3038
#define AF_WINDOW_MODE                 0x7bcd
#define FRM_LENGTH_CTL                 0x0350
#define CSI_LANE_MODE                  0x0114
#define X_EVN_INC                      0x0381
#define X_ODD_INC                      0x0383
#define Y_EVN_INC                      0x0385
#define Y_ODD_INC                      0x0387
#define BINNING_MODE                   0x0900
#define BINNING_TYPE_V                 0x0901
#define FORCE_FD_SUM                   0x300d
#define HDR_MODE                       0x0220
#define MODE_SEL                       0x0100
#define DIG_CROP_X_OFFSET              0x0408
#define DIG_CROP_Y_OFFSET              0x040a
#define DIG_CROP_IMAGE_WIDTH           0x040c
#define DIG_CROP_IMAGE_HEIGHT          0x040e
#define SCALE_M                        0x0404
#define X_OUT_SIZE                     0x034c
#define Y_OUT_SIZE                     0x034e
#define X_ADD_STA                      0x0344
#define Y_ADD_STA                      0x0346
#define X_ADD_END                      0x0348
#define Y_ADD_END                      0x034a
#define EXCK_FREQ                      0x0136
#define CSI_DT_FMT                     0x0112
#define LINE_LENGTH_PCK                0x0342
#define FRM_LENGTH_LINES               0x0340
#define SCALE_M_EXT                    0x303a
#define COARSE_INTEG_TIME              0x0202
#define FINE_INTEG_TIME                0x0200
#define ANA_GAIN_GLOBAL                0x0204
#define PLL_IVT_MPY                    0x0306
#define PLL_IOP_MPY                    0x030e
#define REQ_LINK_BIT_RATE_MBPS_H       0x0820
#define REQ_LINK_BIT_RATE_MBPS_L       0x0822

#define REG8(a, v) { a, v }
#define REG16(a, v) { a, ((v) >> 8) & 0xff }, { (a) + 1, (v) & 0xff }

struct imx258_reg {
	u16 address;
	u8 val;
};

struct imx258_reg_list {
	u32 num_of_regs;
	const struct imx258_reg *regs;
};

/* Link frequency config */
struct imx258_link_freq_config {
	u32 pixels_per_line;

	/* PLL registers for this link frequency */
	struct imx258_reg_list reg_list;
};

/* Mode : resolution and related config&values */
struct imx258_mode {
	/* Frame width */
	u32 width;
	/* Frame height */
	u32 height;

	/* V-timing */
	u32 vts_def;
	u32 vts_min;

	/* Index of Link frequency config to be used */
	u32 link_freq_index;
	/* Default register values */
	struct imx258_reg_list reg_list;
};

static const struct imx258_reg common_regs[] = {
	REG8(EXCK_FREQ, 24),
	REG8(EXCK_FREQ+1, 0),

	REG8(0x3051, 0x00),

	REG8(0x3052, 0x00), // extra in mainline
	REG8(0x4e21, 0x14), // extra in mainline

	REG8(0x6b11, 0xcf),
	REG8(0x7ff0, 0x08),
	REG8(0x7ff1, 0x0f),
	REG8(0x7ff2, 0x08),
	REG8(0x7ff3, 0x1b),
	REG8(0x7ff4, 0x23),
	REG8(0x7ff5, 0x60),
	REG8(0x7ff6, 0x00),
	REG8(0x7ff7, 0x01),
	REG8(0x7ff8, 0x00),
	REG8(0x7ff9, 0x78),
	REG8(0x7ffa, 0x01), // 1 in rk, 0 in mainline
	REG8(0x7ffb, 0x00),
	REG8(0x7ffc, 0x00),
	REG8(0x7ffd, 0x00),
	REG8(0x7ffe, 0x00),
	REG8(0x7fff, 0x03),
	REG8(0x7f76, 0x03),
	REG8(0x7f77, 0xfe),
	REG8(0x7fa8, 0x03),
	REG8(0x7fa9, 0xfe),
	REG8(0x7b24, 0x81),
	REG8(0x7b25, 0x01), // 1 in rk, 0 in mainline
	REG8(0x6564, 0x07),
	REG8(0x6b0d, 0x41),
	REG8(0x653d, 0x04),
	REG8(0x6b05, 0x8c),
	REG8(0x6b06, 0xf9),
	REG8(0x6b08, 0x65),
	REG8(0x6b09, 0xfc),
	REG8(0x6b0a, 0xcf),
	REG8(0x6b0b, 0xd2),
	REG8(0x6700, 0x0e),
	REG8(0x6707, 0x0e), // extra in mainline
	REG8(0x9104, 0x00), // extra in mainline
	REG8(0x4648, 0x7f), // extra in mainline
	REG8(0x7420, 0x00), // extra in mainline
	REG8(0x7421, 0x1c), // extra in mainline
	REG8(0x7422, 0x00), // extra in mainline
	REG8(0x7423, 0xd7), // extra in mainline
	REG8(0x5f04, 0x00),
	REG8(0x5f05, 0xed),

	// extra in rk bsp driver (pixel defect correction)
	{0x94c7, 0xff},
	{0x94c8, 0xff},
	{0x94c9, 0xff},
	{0x95c7, 0xff},
	{0x95c8, 0xff},
	{0x95c9, 0xff},
	{0x94c4, 0x3f},
	{0x94c5, 0x3f},
	{0x94c6, 0x3f},
	{0x95c4, 0x3f},
	{0x95c5, 0x3f},
	{0x95c6, 0x3f},
	{0x94c1, 0x02},
	{0x94c2, 0x02},
	{0x94c3, 0x02},
	{0x95c1, 0x02},
	{0x95c2, 0x02},
	{0x95c3, 0x02},
	{0x94be, 0x0c},
	{0x94bf, 0x0c},
	{0x94c0, 0x0c},
	{0x95be, 0x0c},
	{0x95bf, 0x0c},
	{0x95c0, 0x0c},
	{0x94d0, 0x74},
	{0x94d1, 0x74},
	{0x94d2, 0x74},
	{0x95d0, 0x74},
	{0x95d1, 0x74},
	{0x95d2, 0x74},
	{0x94cd, 0x2e},
	{0x94ce, 0x2e},
	{0x94cf, 0x2e},
	{0x95cd, 0x2e},
	{0x95ce, 0x2e},
	{0x95cf, 0x2e},
	{0x94ca, 0x4c},
	{0x94cb, 0x4c},
	{0x94cc, 0x4c},
	{0x95ca, 0x4c},
	{0x95cb, 0x4c},
	{0x95cc, 0x4c},
	{0x900e, 0x32},
	{0x94e2, 0xff},
	{0x94e3, 0xff},
	{0x94e4, 0xff},
	{0x95e2, 0xff},
	{0x95e3, 0xff},
	{0x95e4, 0xff},
	{0x94df, 0x6e},
	{0x94e0, 0x6e},
	{0x94e1, 0x6e},
	{0x95df, 0x6e},
	{0x95e0, 0x6e},
	{0x95e1, 0x6e},
	{0x7fcc, 0x01},
	{0x7b78, 0x00},
	{0x9401, 0x35},
	{0x9403, 0x23},
	{0x9405, 0x23},
	{0x9406, 0x00},
	{0x9407, 0x31},
	{0x9408, 0x00},
	{0x9409, 0x1b},
	{0x940a, 0x00},
	{0x940b, 0x15},
	{0x940d, 0x3f},
	{0x940f, 0x3f},
	{0x9411, 0x3f},
	{0x9413, 0x64},
	{0x9415, 0x64},
	{0x9417, 0x64},
	{0x941d, 0x34},
	{0x941f, 0x01},
	{0x9421, 0x01},
	{0x9423, 0x01},
	{0x9425, 0x23},
	{0x9427, 0x23},
	{0x9429, 0x23},
	{0x942b, 0x2f},
	{0x942d, 0x1a},
	{0x942f, 0x14},
	{0x9431, 0x3f},
	{0x9433, 0x3f},
	{0x9435, 0x3f},
	{0x9437, 0x6b},
	{0x9439, 0x7c},
	{0x943b, 0x81},
	{0x9443, 0x0f},
	{0x9445, 0x0f},
	{0x9447, 0x0f},
	{0x9449, 0x0f},
	{0x944b, 0x0f},
	{0x944d, 0x0f},
	{0x944f, 0x1e},
	{0x9451, 0x0f},
	{0x9453, 0x0b},
	{0x9455, 0x28},
	{0x9457, 0x13},
	{0x9459, 0x0c},
	{0x945d, 0x00},
	{0x945e, 0x00},
	{0x945f, 0x00},
	{0x946d, 0x00},
	{0x946f, 0x10},
	{0x9471, 0x10},
	{0x9473, 0x40},
	{0x9475, 0x2e},
	{0x9477, 0x10},
	{0x9478, 0x0a},
	{0x947b, 0xe0},
	{0x947c, 0xe0},
	{0x947d, 0xe0},
	{0x947e, 0xe0},
	{0x947f, 0xe0},
	{0x9480, 0xe0},
	{0x9483, 0x14},
	{0x9485, 0x14},
	{0x9487, 0x14},
	{0x9501, 0x35},
	{0x9503, 0x14},
	{0x9505, 0x14},
	{0x9507, 0x31},
	{0x9509, 0x1b},
	{0x950b, 0x15},
	{0x950d, 0x1e},
	{0x950f, 0x1e},
	{0x9511, 0x1e},
	{0x9513, 0x64},
	{0x9515, 0x64},
	{0x9517, 0x64},
	{0x951d, 0x34},
	{0x951f, 0x01},
	{0x9521, 0x01},
	{0x9523, 0x01},
	{0x9525, 0x14},
	{0x9527, 0x14},
	{0x9529, 0x14},
	{0x952b, 0x2f},
	{0x952d, 0x1a},
	{0x952f, 0x14},
	{0x9531, 0x1e},
	{0x9533, 0x1e},
	{0x9535, 0x1e},
	{0x9537, 0x6b},
	{0x9539, 0x7c},
	{0x953b, 0x81},
	{0x9543, 0x0f},
	{0x9545, 0x0f},
	{0x9547, 0x0f},
	{0x9549, 0x0f},
	{0x954b, 0x0f},
	{0x954d, 0x0f},
	{0x954f, 0x15},
	{0x9551, 0x0b},
	{0x9553, 0x08},
	{0x9555, 0x1c},
	{0x9557, 0x0d},
	{0x9559, 0x08},
	{0x955d, 0x00},
	{0x955e, 0x00},
	{0x955f, 0x00},
	{0x956d, 0x00},
	{0x956f, 0x10},
	{0x9571, 0x10},
	{0x9573, 0x40},
	{0x9575, 0x2e},
	{0x9577, 0x10},
	{0x9578, 0x0a},
	{0x957b, 0xe0},
	{0x957c, 0xe0},
	{0x957d, 0xe0},
	{0x957e, 0xe0},
	{0x957f, 0xe0},
	{0x9580, 0xe0},
	{0x9583, 0x14},
	{0x9585, 0x14},
	{0x9587, 0x14},
	{0x7f78, 0x00},
	{0x7f89, 0x00},
	{0x7f93, 0x00},
	{0x924b, 0x1b},
	{0x924c, 0x0a},
	{0x9304, 0x04},
	{0x9315, 0x04},
	{0x9250, 0x50},
	{0x9251, 0x3c},
	{0x9252, 0x14},

	REG8(0x94dc, 0x20),
	REG8(0x94dd, 0x20),
	REG8(0x94de, 0x20),
	REG8(0x95dc, 0x20),
	REG8(0x95dd, 0x20),
	REG8(0x95de, 0x20),
	REG8(0x7fb0, 0x00),
	REG8(0x9010, 0x3e),
	REG8(0x9419, 0x50),
	REG8(0x941b, 0x50),
	REG8(0x9519, 0x50),
	REG8(0x951b, 0x50),

	// common per-mode settings
	REG16(ANA_GAIN_GLOBAL, 0),
	REG8(0x20e, 0x01),
	REG8(0x20f, 0x00),
	REG8(0x210, 0x01),
	REG8(0x211, 0x00),
	REG8(0x212, 0x01),
	REG8(0x213, 0x00),
	REG8(0x214, 0x01),
	REG8(0x215, 0x00),
	REG8(AF_WINDOW_MODE, 0),
	REG8(PHASE_PIX_OUTEN, 0x00),
	REG8(PDPIX_DATA_RATE, 0x00),
	REG8(HDR_MODE, 0x00),
};

static const struct imx258_reg mode_4208x3120_regs[] = {
	REG16(CSI_DT_FMT, 0x0a0a),
	REG8(CSI_LANE_MODE, 0x03),
	REG16(LINE_LENGTH_PCK, 5352),
	REG16(FRM_LENGTH_LINES, 3224),
	REG16(X_ADD_STA, 0),
	REG16(Y_ADD_STA, 0),
	REG16(X_ADD_END, 4207),
	REG16(Y_ADD_END, 3119),
	REG8(X_EVN_INC, 1),
	REG8(X_ODD_INC, 1),
	REG8(Y_EVN_INC, 1),
	REG8(Y_ODD_INC, 1),
	REG8(BINNING_MODE, 0x00),
	REG8(BINNING_TYPE_V, 0x11),
	REG8(SCALE_MODE, 0x00),
	REG16(SCALE_M, 16),
	REG16(DIG_CROP_X_OFFSET, 0),
	REG16(DIG_CROP_Y_OFFSET, 0),
	REG16(DIG_CROP_IMAGE_WIDTH, 4208),
	REG16(DIG_CROP_IMAGE_HEIGHT, 3120),
	REG8(SCALE_MODE_EXT, 0x00),
	REG16(SCALE_M_EXT, 16),
	REG8(FORCE_FD_SUM, 0x00),
	REG16(X_OUT_SIZE, 4208),
	REG16(Y_OUT_SIZE, 3120),
	REG8(FRM_LENGTH_CTL, 0x01),
	REG16(COARSE_INTEG_TIME, 3184),
};

static const struct imx258_reg mode_4032x3024_regs[] = {
	REG16(CSI_DT_FMT, 0x0a0a),
	REG8(CSI_LANE_MODE, 0x03),
	REG16(LINE_LENGTH_PCK, 5352),
	REG16(FRM_LENGTH_LINES, 3224),
	REG16(X_ADD_STA, 0),
	REG16(Y_ADD_STA, 0),
	REG16(X_ADD_END, 4207),
	REG16(Y_ADD_END, 3119),
	REG8(X_EVN_INC, 1),
	REG8(X_ODD_INC, 1),
	REG8(Y_EVN_INC, 1),
	REG8(Y_ODD_INC, 1),
	REG8(BINNING_MODE, 0x00),
	REG8(BINNING_TYPE_V, 0x11),
	REG8(SCALE_MODE, 0x00),
	REG16(SCALE_M, 16),
	REG16(DIG_CROP_X_OFFSET, 0), //(4208-4032)/2),
	REG16(DIG_CROP_Y_OFFSET, 0), //(3120-3024)/2),
	REG16(DIG_CROP_IMAGE_WIDTH, 4032),
	REG16(DIG_CROP_IMAGE_HEIGHT, 3024),
	REG8(SCALE_MODE_EXT, 0),
	REG16(SCALE_M_EXT, 16),
	REG8(FORCE_FD_SUM, 0x00),
	REG16(X_OUT_SIZE, 4032),
	REG16(Y_OUT_SIZE, 3024),
	REG8(FRM_LENGTH_CTL, 0x01),
	REG16(COARSE_INTEG_TIME, 3184),
};

static const struct imx258_reg mode_2104_1560_regs[] = {
	REG16(CSI_DT_FMT, 0x0a0a),
	REG8(CSI_LANE_MODE, 0x03),
	REG16(LINE_LENGTH_PCK, 5352),
	REG16(FRM_LENGTH_LINES, 1592),
	REG16(X_ADD_STA, 0),
	REG16(Y_ADD_STA, 0),
	REG16(X_ADD_END, 4207),
	REG16(Y_ADD_END, 3119),
	REG8(X_EVN_INC, 1),
	REG8(X_ODD_INC, 1),
	REG8(Y_EVN_INC, 1),
	REG8(Y_ODD_INC, 1),
	REG8(BINNING_MODE, 0x01),
	REG8(BINNING_TYPE_V, 0x12),
	REG8(SCALE_MODE, 1),
	REG16(SCALE_M, 32),
	REG16(DIG_CROP_X_OFFSET, 0),
	REG16(DIG_CROP_Y_OFFSET, 0),
	REG16(DIG_CROP_IMAGE_WIDTH, 4208),
	REG16(DIG_CROP_IMAGE_HEIGHT, 1560),
	REG8(SCALE_MODE_EXT, 0x00),
	REG16(SCALE_M_EXT, 16),
	REG8(FORCE_FD_SUM, 0x00),
	REG16(X_OUT_SIZE, 2104),
	REG16(Y_OUT_SIZE, 1560),
	REG8(FRM_LENGTH_CTL, 0x01),
	REG16(COARSE_INTEG_TIME, 1582),
};

static const struct imx258_reg mode_1048_780_regs[] = {
	REG16(CSI_DT_FMT, 0x0a0a),
	REG8(CSI_LANE_MODE, 0x03),
	REG16(LINE_LENGTH_PCK, 5352),
	REG16(FRM_LENGTH_LINES, 844),
	REG16(X_ADD_STA, 0),
	REG16(Y_ADD_STA, 0),
	REG16(X_ADD_END, 4191),
	REG16(Y_ADD_END, 3119),
	REG8(X_EVN_INC, 1),
	REG8(X_ODD_INC, 1),
	REG8(Y_EVN_INC, 1),
	REG8(Y_ODD_INC, 1),
	REG8(BINNING_MODE, 0x01),
	REG8(BINNING_TYPE_V, 0x14),
	REG8(SCALE_MODE, 0x01),
	REG16(SCALE_M, 64),
	REG16(DIG_CROP_X_OFFSET, 0),
	REG16(DIG_CROP_Y_OFFSET, 0),
	REG16(DIG_CROP_IMAGE_WIDTH, 4192),
	REG16(DIG_CROP_IMAGE_HEIGHT, 780),
	REG8(SCALE_MODE_EXT, 0x00),
	REG16(SCALE_M_EXT, 16),
	REG8(FORCE_FD_SUM, 0x00),
	REG16(X_OUT_SIZE, 1048),
	REG16(Y_OUT_SIZE, 780),
	REG8(FRM_LENGTH_CTL, 0x01),
	REG16(COARSE_INTEG_TIME, 834),
};

static const char * const imx258_test_pattern_menu[] = {
	"Disabled",
	"Solid Colour",
	"Eight Vertical Colour Bars",
	"Colour Bars With Fade to Grey",
	"Pseudorandom Sequence (PN9)",
};

enum {
	IMX258_LINK_FREQ_1224MBPS,
	IMX258_LINK_FREQ_642MBPS,
};

/* Menu items for LINK_FREQ V4L2 control */
static const s64 link_freq_menu_items[] = {
	612000000ULL,
	321000000ULL,
};

/* 4032*3024 needs 1224 Mbps/lane, 4 lanes */
static const struct imx258_reg mipi_data_rate_1224mbps[] = {
	REG8(IVTPXCK_DIV, 5),
	REG8(IVTSYCK_DIV, 2),
	REG8(PREPLLCK_VT_DIV, 4),
	REG16(PLL_IVT_MPY, 204), // 1224 MHz
	REG8(IOPPXCK_DIV, 10), // or 8
	REG8(IOPSYCK_DIV, 1),
	REG8(PREPLLCK_OP_DIV, 2),
	REG16(PLL_IOP_MPY, 216),
	REG8(PLL_MULT_DRIV, 0),
	REG16(REQ_LINK_BIT_RATE_MBPS_H, 1224*4),
	REG16(REQ_LINK_BIT_RATE_MBPS_L, 0),
};

static const struct imx258_reg mipi_data_rate_642mbps[] = {
	REG8(IVTPXCK_DIV, 5),
	REG8(IVTSYCK_DIV, 2),
	REG8(PREPLLCK_VT_DIV, 4),
	REG16(PLL_IVT_MPY, 107),
	REG8(IOPPXCK_DIV, 10),
	REG8(IOPSYCK_DIV, 1),
	REG8(PREPLLCK_OP_DIV, 2),
	REG16(PLL_IOP_MPY, 216),
	REG8(PLL_MULT_DRIV, 0),
	REG16(REQ_LINK_BIT_RATE_MBPS_H, 2568),
	REG16(REQ_LINK_BIT_RATE_MBPS_L, 0),
};

/* Link frequency configs */
static const struct imx258_link_freq_config link_freq_configs[] = {
	[IMX258_LINK_FREQ_1224MBPS] = {
		.pixels_per_line = IMX258_PPL_DEFAULT,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mipi_data_rate_1224mbps),
			.regs = mipi_data_rate_1224mbps,
		}
	},
	[IMX258_LINK_FREQ_642MBPS] = {
		.pixels_per_line = IMX258_PPL_DEFAULT,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mipi_data_rate_642mbps),
			.regs = mipi_data_rate_642mbps,
		}
	},
};

/* Mode configs */
static const struct imx258_mode supported_modes[] = {
	{
		.width = 4208,
		.height = 3120,
		.vts_def = IMX258_VTS_30FPS,
		.vts_min = IMX258_VTS_30FPS,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_4208x3120_regs),
			.regs = mode_4208x3120_regs,
		},
		.link_freq_index = IMX258_LINK_FREQ_1224MBPS,
	},
	{
		.width = 4032,
		.height = 3024,
		.vts_def = IMX258_VTS_30FPS,
		.vts_min = IMX258_VTS_30FPS,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_4032x3024_regs),
			.regs = mode_4032x3024_regs,
		},
		.link_freq_index = IMX258_LINK_FREQ_1224MBPS,
	},
	{
		.width = 2104,
		.height = 1560,
		.vts_def = IMX258_VTS_30FPS_2K,
		.vts_min = IMX258_VTS_30FPS_2K,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_2104_1560_regs),
			.regs = mode_2104_1560_regs,
		},
		.link_freq_index = IMX258_LINK_FREQ_642MBPS,
	},
	{
		.width = 1048,
		.height = 780,
		.vts_def = IMX258_VTS_30FPS_VGA,
		.vts_min = IMX258_VTS_30FPS_VGA,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_1048_780_regs),
			.regs = mode_1048_780_regs,
		},
		.link_freq_index = IMX258_LINK_FREQ_642MBPS,
	},
};

/*
 * pixel_rate = link_freq * data-rate * nr_of_lanes / bits_per_sample
 * data rate => double data rate; number of lanes => 4; bits per pixel => 10
 */
static u64 link_freq_to_pixel_rate(u64 f)
{
	f *= 2 * 4;
	do_div(f, 10);

	return f;
}

/* regulator supplies */
static const char * const imx258_supply_names[] = {
	"vana", /* Analog (2.8V) supply */
	"vdig", /* Digital Core (1.5V) supply */
	"vif",  /* Digital I/O (1.8V) supply */
	"i2c",  /* I2C BUS I/O (1.8V) supply */
};

#define IMX258_SUPPLY_COUNT ARRAY_SIZE(imx258_supply_names)

struct imx258 {
	struct v4l2_subdev sd;
	struct media_pad pad;

	struct v4l2_ctrl_handler ctrl_handler;
	/* V4L2 Controls */
	struct v4l2_ctrl *link_freq;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *exposure;

	struct gpio_desc *pwdn_gpio;
	struct gpio_desc *reset_gpio;
	struct regulator_bulk_data supplies[IMX258_SUPPLY_COUNT];

	/* Current mode */
	const struct imx258_mode *cur_mode;

	/*
	 * Mutex for serialized access:
	 * Protect sensor module set pad format and start/stop streaming safely.
	 */
	struct mutex mutex;

	/* Streaming on/off */
	bool streaming;

	struct clk *clk;
};

static inline struct imx258 *to_imx258(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct imx258, sd);
}

/* Read registers up to 2 at a time */
static int imx258_read_reg(struct imx258 *imx258, u16 reg, u32 len, u32 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx258->sd);
	struct i2c_msg msgs[2];
	u8 addr_buf[2] = { reg >> 8, reg & 0xff };
	u8 data_buf[4] = { 0, };
	int ret;

	if (len > 4)
		return -EINVAL;

	/* Write register address */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = ARRAY_SIZE(addr_buf);
	msgs[0].buf = addr_buf;

	/* Read data from register */
	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = &data_buf[4 - len];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = get_unaligned_be32(data_buf);

	return 0;
}

/* Write registers up to 2 at a time */
static int imx258_write_reg(struct imx258 *imx258, u16 reg, u32 len, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx258->sd);
	u8 buf[6];

	if (len > 4)
		return -EINVAL;

	put_unaligned_be16(reg, buf);
	put_unaligned_be32(val << (8 * (4 - len)), buf + 2);
	if (i2c_master_send(client, buf, len + 2) != len + 2)
		return -EIO;

	return 0;
}

/* Write a list of registers */
static int imx258_write_regs(struct imx258 *imx258,
			     const struct imx258_reg *regs, u32 len)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx258->sd);
	unsigned int i;
	int ret;

	for (i = 0; i < len; i++) {
		ret = imx258_write_reg(imx258, regs[i].address, 1,
					regs[i].val);
		if (ret) {
			dev_err_ratelimited(
				&client->dev,
				"Failed to write reg 0x%4.4x. error = %d\n",
				regs[i].address, ret);

			return ret;
		}
	}

	return 0;
}

/* Open sub-device */
static int imx258_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct v4l2_mbus_framefmt *try_fmt =
		v4l2_subdev_get_try_format(sd, fh->state, 0);

	/* Initialize try_fmt */
	try_fmt->width = supported_modes[0].width;
	try_fmt->height = supported_modes[0].height;
	try_fmt->code = IMX258_MBUS_FORMAT;
	try_fmt->field = V4L2_FIELD_NONE;

	return 0;
}

static int imx258_update_digital_gain(struct imx258 *imx258, u32 len, u32 val)
{
	int ret;

	ret = imx258_write_reg(imx258, IMX258_REG_GR_DIGITAL_GAIN,
				IMX258_REG_VALUE_16BIT,
				val);
	if (ret)
		return ret;
	ret = imx258_write_reg(imx258, IMX258_REG_GB_DIGITAL_GAIN,
				IMX258_REG_VALUE_16BIT,
				val);
	if (ret)
		return ret;
	ret = imx258_write_reg(imx258, IMX258_REG_R_DIGITAL_GAIN,
				IMX258_REG_VALUE_16BIT,
				val);
	if (ret)
		return ret;
	ret = imx258_write_reg(imx258, IMX258_REG_B_DIGITAL_GAIN,
				IMX258_REG_VALUE_16BIT,
				val);
	if (ret)
		return ret;
	return 0;
}

static int imx258_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx258 *imx258 =
		container_of(ctrl->handler, struct imx258, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&imx258->sd);
	int ret = 0;

	/*
	 * Applying V4L2 control value only happens
	 * when power is up for streaming
	 */
	if (pm_runtime_get_if_in_use(&client->dev) == 0)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_ANALOGUE_GAIN:
		ret = imx258_write_reg(imx258, IMX258_REG_ANALOG_GAIN,
				IMX258_REG_VALUE_16BIT,
				ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = imx258_write_reg(imx258, IMX258_REG_EXPOSURE,
				IMX258_REG_VALUE_16BIT,
				ctrl->val);
		break;
	case V4L2_CID_DIGITAL_GAIN:
		ret = imx258_update_digital_gain(imx258, IMX258_REG_VALUE_16BIT,
				ctrl->val);
		break;
	case V4L2_CID_TEST_PATTERN:
		ret = imx258_write_reg(imx258, IMX258_REG_TEST_PATTERN,
				IMX258_REG_VALUE_16BIT,
				ctrl->val);
		ret = imx258_write_reg(imx258, REG_MIRROR_FLIP_CONTROL,
				IMX258_REG_VALUE_08BIT,
				!ctrl->val ? REG_CONFIG_MIRROR_FLIP :
				REG_CONFIG_FLIP_TEST_PATTERN);
		break;
	case V4L2_CID_WIDE_DYNAMIC_RANGE:
		if (!ctrl->val) {
			ret = imx258_write_reg(imx258, IMX258_REG_HDR,
					       IMX258_REG_VALUE_08BIT,
					       IMX258_HDR_RATIO_MIN);
		} else {
			ret = imx258_write_reg(imx258, IMX258_REG_HDR,
					       IMX258_REG_VALUE_08BIT,
					       IMX258_HDR_ON);
			if (ret)
				break;
			ret = imx258_write_reg(imx258, IMX258_REG_HDR_RATIO,
					       IMX258_REG_VALUE_08BIT,
					       BIT(IMX258_HDR_RATIO_MAX));
		}
		break;
	default:
		dev_info(&client->dev,
			 "ctrl(id:0x%x,val:0x%x) is not handled\n",
			 ctrl->id, ctrl->val);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(&client->dev);

	return ret;
}

static const struct v4l2_ctrl_ops imx258_ctrl_ops = {
	.s_ctrl = imx258_set_ctrl,
};

static int imx258_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *sd_state,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	/* Only one bayer order(GRBG) is supported */
	if (code->index > 0)
		return -EINVAL;

	code->code = IMX258_MBUS_FORMAT;

	return 0;
}

static int imx258_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *sd_state,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index >= ARRAY_SIZE(supported_modes))
		return -EINVAL;

	if (fse->code != IMX258_MBUS_FORMAT)
		return -EINVAL;

	fse->min_width = supported_modes[fse->index].width;
	fse->max_width = fse->min_width;
	fse->min_height = supported_modes[fse->index].height;
	fse->max_height = fse->min_height;

	return 0;
}

static void imx258_update_pad_format(const struct imx258_mode *mode,
				     struct v4l2_subdev_format *fmt)
{
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.code = IMX258_MBUS_FORMAT;
	fmt->format.field = V4L2_FIELD_NONE;
}

static int __imx258_get_pad_format(struct imx258 *imx258,
				   struct v4l2_subdev_state *sd_state,
				   struct v4l2_subdev_format *fmt)
{
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		fmt->format = *v4l2_subdev_get_try_format(&imx258->sd,
							  sd_state,
							  fmt->pad);
	else
		imx258_update_pad_format(imx258->cur_mode, fmt);

	return 0;
}

static int imx258_get_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_format *fmt)
{
	struct imx258 *imx258 = to_imx258(sd);
	int ret;

	mutex_lock(&imx258->mutex);
	ret = __imx258_get_pad_format(imx258, sd_state, fmt);
	mutex_unlock(&imx258->mutex);

	return ret;
}

static int imx258_set_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_format *fmt)
{
	struct imx258 *imx258 = to_imx258(sd);
	const struct imx258_mode *mode;
	struct v4l2_mbus_framefmt *framefmt;
	s32 vblank_def;
	s32 vblank_min;
	s64 h_blank;
	s64 pixel_rate;
	s64 link_freq;

	mutex_lock(&imx258->mutex);

	/* Only one raw bayer(GBRG) order is supported */
	fmt->format.code = IMX258_MBUS_FORMAT;

	mode = v4l2_find_nearest_size(supported_modes,
		ARRAY_SIZE(supported_modes), width, height,
		fmt->format.width, fmt->format.height);
	imx258_update_pad_format(mode, fmt);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, sd_state, fmt->pad);
		*framefmt = fmt->format;
	} else {
		imx258->cur_mode = mode;
		__v4l2_ctrl_s_ctrl(imx258->link_freq, mode->link_freq_index);

		link_freq = link_freq_menu_items[mode->link_freq_index];
		pixel_rate = link_freq_to_pixel_rate(link_freq);
		__v4l2_ctrl_s_ctrl_int64(imx258->pixel_rate, pixel_rate);
		/* Update limits and set FPS to default */
		vblank_def = imx258->cur_mode->vts_def -
			     imx258->cur_mode->height;
		vblank_min = imx258->cur_mode->vts_min -
			     imx258->cur_mode->height;
		__v4l2_ctrl_modify_range(
			imx258->vblank, vblank_min,
			IMX258_VTS_MAX - imx258->cur_mode->height, 1,
			vblank_def);
		__v4l2_ctrl_s_ctrl(imx258->vblank, vblank_def);
		h_blank =
			link_freq_configs[mode->link_freq_index].pixels_per_line
			 - imx258->cur_mode->width;
		__v4l2_ctrl_modify_range(imx258->hblank, h_blank,
					 h_blank, 1, h_blank);
	}

	mutex_unlock(&imx258->mutex);

	return 0;
}

/* Start streaming */
static int imx258_start_streaming(struct imx258 *imx258)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx258->sd);
	const struct imx258_reg_list *reg_list;
	int ret, link_freq_index;

	/* Common registers */
	ret = imx258_write_regs(imx258, common_regs, ARRAY_SIZE(common_regs));
	if (ret) {
		dev_err(&client->dev, "%s failed to set common registers\n", __func__);
		return ret;
	}

	/* Setup PLL */
	link_freq_index = imx258->cur_mode->link_freq_index;
	reg_list = &link_freq_configs[link_freq_index].reg_list;
	ret = imx258_write_regs(imx258, reg_list->regs, reg_list->num_of_regs);
	if (ret) {
		dev_err(&client->dev, "%s failed to set plls\n", __func__);
		return ret;
	}

	/* Apply default values of current mode */
	reg_list = &imx258->cur_mode->reg_list;
	ret = imx258_write_regs(imx258, reg_list->regs, reg_list->num_of_regs);
	if (ret) {
		dev_err(&client->dev, "%s failed to set mode\n", __func__);
		return ret;
	}

	/* Set Orientation be 180 degree */
	ret = imx258_write_reg(imx258, REG_MIRROR_FLIP_CONTROL,
			       IMX258_REG_VALUE_08BIT, REG_CONFIG_MIRROR_FLIP);
	if (ret) {
		dev_err(&client->dev, "%s failed to set orientation\n",
			__func__);
		return ret;
	}

	/* Apply customized values from user */
	ret =  __v4l2_ctrl_handler_setup(imx258->sd.ctrl_handler);
	if (ret)
		return ret;

	/* set stream on register */
	return imx258_write_reg(imx258, IMX258_REG_MODE_SELECT,
				IMX258_REG_VALUE_08BIT,
				IMX258_MODE_STREAMING);
}

/* Stop streaming */
static int imx258_stop_streaming(struct imx258 *imx258)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx258->sd);
	int ret;

	/* set stream off register */
	ret = imx258_write_reg(imx258, IMX258_REG_MODE_SELECT,
		IMX258_REG_VALUE_08BIT, IMX258_MODE_STANDBY);
	if (ret)
		dev_err(&client->dev, "%s failed to set stream\n", __func__);

	/*
	 * Return success even if it was an error, as there is nothing the
	 * caller can do about it.
	 */
	return 0;
}

static int imx258_power_on(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct imx258 *imx258 = to_imx258(sd);
	u32 val = 0;
	int ret;

	if (imx258->clk) {
		ret = clk_set_rate(imx258->clk, IMX258_INPUT_CLOCK_FREQ);
		if (ret < 0)
			dev_warn(dev, "Failed to set clk rate\n");

		val = clk_get_rate(imx258->clk);
		if (val < IMX258_INPUT_CLOCK_FREQ_MIN ||
		    val > IMX258_INPUT_CLOCK_FREQ_MAX) {
			dev_err(dev, "clk mismatched, expecting %u, got %u Hz\n",
				 IMX258_INPUT_CLOCK_FREQ, val);
			return -EINVAL;
		}
	}

	ret = regulator_bulk_enable(IMX258_SUPPLY_COUNT, imx258->supplies);
	if (ret) {
		dev_err(dev, "failed to enable regulators\n");
		return ret;
	}

	mdelay(20);

	gpiod_set_value_cansleep(imx258->pwdn_gpio, 0);

	mdelay(5);

	ret = clk_prepare_enable(imx258->clk);
	if (ret) {
		dev_err(dev, "failed to enable clock\n");
		gpiod_set_value_cansleep(imx258->pwdn_gpio, 1);
		regulator_bulk_disable(IMX258_SUPPLY_COUNT, imx258->supplies);
		return ret;
	}

	usleep_range(1000, 2000);

	gpiod_set_value_cansleep(imx258->reset_gpio, 0);

	usleep_range(400, 500);

	return 0;
}

static int imx258_power_off(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct imx258 *imx258 = to_imx258(sd);

	clk_disable_unprepare(imx258->clk);

	gpiod_set_value_cansleep(imx258->reset_gpio, 1);
	gpiod_set_value_cansleep(imx258->pwdn_gpio, 1);

	regulator_bulk_disable(IMX258_SUPPLY_COUNT, imx258->supplies);

	return 0;
}

static int imx258_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct imx258 *imx258 = to_imx258(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	mutex_lock(&imx258->mutex);
	if (imx258->streaming == enable) {
		mutex_unlock(&imx258->mutex);
		return 0;
	}

	if (enable) {
		ret = pm_runtime_resume_and_get(&client->dev);
		if (ret < 0)
			goto err_unlock;

		/*
		 * Apply default & customized values
		 * and then start streaming.
		 */
		ret = imx258_start_streaming(imx258);
		if (ret)
			goto err_rpm_put;
	} else {
		imx258_stop_streaming(imx258);
		pm_runtime_put(&client->dev);
	}

	imx258->streaming = enable;
	mutex_unlock(&imx258->mutex);

	return ret;

err_rpm_put:
	pm_runtime_put(&client->dev);
err_unlock:
	mutex_unlock(&imx258->mutex);

	return ret;
}

static int __maybe_unused imx258_suspend(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct imx258 *imx258 = to_imx258(sd);

	if (imx258->streaming)
		imx258_stop_streaming(imx258);

	return 0;
}

static int __maybe_unused imx258_resume(struct device *dev)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct imx258 *imx258 = to_imx258(sd);
	int ret;

	if (imx258->streaming) {
		ret = imx258_start_streaming(imx258);
		if (ret)
			goto error;
	}

	return 0;

error:
	imx258_stop_streaming(imx258);
	imx258->streaming = 0;
	return ret;
}

/* Verify chip ID */
static int imx258_identify_module(struct imx258 *imx258)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx258->sd);
	int ret;
	u32 val;

	ret = imx258_read_reg(imx258, IMX258_REG_CHIP_ID,
			      IMX258_REG_VALUE_16BIT, &val);
	if (ret) {
		dev_err(&client->dev, "failed to read chip id %x\n",
			IMX258_CHIP_ID);
		return ret;
	}

	if (val != IMX258_CHIP_ID) {
		dev_err(&client->dev, "chip id mismatch: %x!=%x\n",
			IMX258_CHIP_ID, val);
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int imx258_g_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct imx258 *imx258 = to_imx258(sd);
	u32 val = 0;
	int ret;

	if (reg->reg > 0xffff)
		return -EINVAL;

	reg->size = 1;

	mutex_lock(&imx258->mutex);
	ret = imx258_read_reg(imx258, reg->reg, 1, &val);
	mutex_unlock(&imx258->mutex);
	if (ret)
		return -EIO;

	reg->val = val;
	return 0;
}

static int imx258_s_register(struct v4l2_subdev *sd,
			     const struct v4l2_dbg_register *reg)
{
	struct imx258 *imx258 = to_imx258(sd);
	int ret;

	if (reg->reg > 0xffff || reg->val > 0xff)
		return -EINVAL;

	mutex_lock(&imx258->mutex);
	ret = imx258_write_reg(imx258, reg->reg, 1, reg->val);
	mutex_unlock(&imx258->mutex);

	return ret;
}
#endif

static const struct v4l2_subdev_core_ops imx258_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = imx258_g_register,
	.s_register = imx258_s_register,
#endif
};

static const struct v4l2_subdev_video_ops imx258_video_ops = {
	.s_stream = imx258_set_stream,
};

static const struct v4l2_subdev_pad_ops imx258_pad_ops = {
	.enum_mbus_code = imx258_enum_mbus_code,
	.get_fmt = imx258_get_pad_format,
	.set_fmt = imx258_set_pad_format,
	.enum_frame_size = imx258_enum_frame_size,
};

static const struct v4l2_subdev_ops imx258_subdev_ops = {
	.core = &imx258_core_ops,
	.video = &imx258_video_ops,
	.pad = &imx258_pad_ops,
};

static const struct v4l2_subdev_internal_ops imx258_internal_ops = {
	.open = imx258_open,
};

/* Initialize control handlers */
static int imx258_init_controls(struct imx258 *imx258)
{
	struct i2c_client *client = v4l2_get_subdevdata(&imx258->sd);
	struct v4l2_ctrl_handler *ctrl_hdlr;
	s64 vblank_def;
	s64 vblank_min;
	s64 pixel_rate_min;
	s64 pixel_rate_max;
	int ret;

	ctrl_hdlr = &imx258->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 8);
	if (ret)
		return ret;

	mutex_init(&imx258->mutex);
	ctrl_hdlr->lock = &imx258->mutex;
	imx258->link_freq = v4l2_ctrl_new_int_menu(ctrl_hdlr,
				&imx258_ctrl_ops,
				V4L2_CID_LINK_FREQ,
				ARRAY_SIZE(link_freq_menu_items) - 1,
				0,
				link_freq_menu_items);

	if (imx258->link_freq)
		imx258->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	pixel_rate_max = link_freq_to_pixel_rate(link_freq_menu_items[0]);
	pixel_rate_min = link_freq_to_pixel_rate(
		link_freq_menu_items[ARRAY_SIZE(link_freq_menu_items) - 1]);
	/* By default, PIXEL_RATE is read only */
	imx258->pixel_rate = v4l2_ctrl_new_std(ctrl_hdlr, &imx258_ctrl_ops,
				V4L2_CID_PIXEL_RATE,
				pixel_rate_min, pixel_rate_max,
				1, pixel_rate_max);


	vblank_def = imx258->cur_mode->vts_def - imx258->cur_mode->height;
	vblank_min = imx258->cur_mode->vts_min - imx258->cur_mode->height;
	imx258->vblank = v4l2_ctrl_new_std(
				ctrl_hdlr, &imx258_ctrl_ops, V4L2_CID_VBLANK,
				vblank_min,
				IMX258_VTS_MAX - imx258->cur_mode->height, 1,
				vblank_def);

	if (imx258->vblank)
		imx258->vblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx258->hblank = v4l2_ctrl_new_std(
				ctrl_hdlr, &imx258_ctrl_ops, V4L2_CID_HBLANK,
				IMX258_PPL_DEFAULT - imx258->cur_mode->width,
				IMX258_PPL_DEFAULT - imx258->cur_mode->width,
				1,
				IMX258_PPL_DEFAULT - imx258->cur_mode->width);

	if (imx258->hblank)
		imx258->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx258->exposure = v4l2_ctrl_new_std(
				ctrl_hdlr, &imx258_ctrl_ops,
				V4L2_CID_EXPOSURE, IMX258_EXPOSURE_MIN,
				IMX258_EXPOSURE_MAX, IMX258_EXPOSURE_STEP,
				IMX258_EXPOSURE_DEFAULT);

	v4l2_ctrl_new_std(ctrl_hdlr, &imx258_ctrl_ops, V4L2_CID_ANALOGUE_GAIN,
				IMX258_ANA_GAIN_MIN, IMX258_ANA_GAIN_MAX,
				IMX258_ANA_GAIN_STEP, IMX258_ANA_GAIN_DEFAULT);

	v4l2_ctrl_new_std(ctrl_hdlr, &imx258_ctrl_ops, V4L2_CID_DIGITAL_GAIN,
				IMX258_DGTL_GAIN_MIN, IMX258_DGTL_GAIN_MAX,
				IMX258_DGTL_GAIN_STEP,
				IMX258_DGTL_GAIN_DEFAULT);

	v4l2_ctrl_new_std(ctrl_hdlr, &imx258_ctrl_ops, V4L2_CID_WIDE_DYNAMIC_RANGE,
				0, 1, 1, IMX258_HDR_RATIO_DEFAULT);

	v4l2_ctrl_new_std_menu_items(ctrl_hdlr, &imx258_ctrl_ops,
				V4L2_CID_TEST_PATTERN,
				ARRAY_SIZE(imx258_test_pattern_menu) - 1,
				0, 0, imx258_test_pattern_menu);

	if (ctrl_hdlr->error) {
		ret = ctrl_hdlr->error;
		dev_err(&client->dev, "%s control init failed (%d)\n",
				__func__, ret);
		goto error;
	}

	imx258->sd.ctrl_handler = ctrl_hdlr;

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdlr);
	mutex_destroy(&imx258->mutex);

	return ret;
}

static void imx258_free_controls(struct imx258 *imx258)
{
	v4l2_ctrl_handler_free(imx258->sd.ctrl_handler);
	mutex_destroy(&imx258->mutex);
}

static int imx258_probe(struct i2c_client *client)
{
	struct imx258 *imx258;
	int ret, i;
	u32 val = 0;

	imx258 = devm_kzalloc(&client->dev, sizeof(*imx258), GFP_KERNEL);
	if (!imx258)
		return -ENOMEM;

	imx258->clk = devm_clk_get_optional(&client->dev, NULL);
	if (IS_ERR(imx258->clk))
		return dev_err_probe(&client->dev, PTR_ERR(imx258->clk),
				     "error getting clock\n");
	if (!imx258->clk) {
		dev_dbg(&client->dev,
			"no clock provided, using clock-frequency property\n");

		device_property_read_u32(&client->dev, "clock-frequency", &val);
	} else {
		val = clk_get_rate(imx258->clk);
	}

	//XXX: the driver just checked for the clock to be as expected here
	// but we now just configure the clock to expected value before power on
	// if possible

	/*
	 * Check that the device is mounted upside down. The driver only
	 * supports a single pixel order right now.
	 */
	ret = device_property_read_u32(&client->dev, "rotation", &val);
	if (ret || val != 180)
		return -EINVAL;

	for (i = 0; i < IMX258_SUPPLY_COUNT; i++)
		imx258->supplies[i].supply = imx258_supply_names[i];
	ret = devm_regulator_bulk_get(&client->dev,
				      IMX258_SUPPLY_COUNT,
				      imx258->supplies);
	if (ret)
		return dev_err_probe(&client->dev, ret, "Failed to get supplies\n");

	/* request optional power down pin */
	imx258->pwdn_gpio = devm_gpiod_get_optional(&client->dev, "powerdown",
						    GPIOD_OUT_HIGH);
	if (IS_ERR(imx258->pwdn_gpio))
		return PTR_ERR(imx258->pwdn_gpio);

	/* request optional reset pin */
	imx258->reset_gpio = devm_gpiod_get_optional(&client->dev, "reset",
						    GPIOD_OUT_HIGH);
	if (IS_ERR(imx258->reset_gpio))
		return PTR_ERR(imx258->reset_gpio);

	/* Initialize subdev */
	v4l2_i2c_subdev_init(&imx258->sd, client, &imx258_subdev_ops);

	/* Will be powered off via pm_runtime_idle */
	ret = imx258_power_on(&client->dev);
	if (ret)
		return ret;

	/* Check module identity */
	ret = imx258_identify_module(imx258);
	if (ret)
		goto error_identify;

	/* Set default mode to max resolution */
	imx258->cur_mode = &supported_modes[0];

	ret = imx258_init_controls(imx258);
	if (ret)
		goto error_identify;

	/* Initialize subdev */
	imx258->sd.internal_ops = &imx258_internal_ops;
	imx258->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	imx258->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

	/* Initialize source pad */
	imx258->pad.flags = MEDIA_PAD_FL_SOURCE;

	ret = media_entity_pads_init(&imx258->sd.entity, 1, &imx258->pad);
	if (ret)
		goto error_handler_free;

	ret = v4l2_async_register_subdev_sensor(&imx258->sd);
	if (ret < 0)
		goto error_media_entity;

	pm_runtime_set_active(&client->dev);
	pm_runtime_enable(&client->dev);
	pm_runtime_idle(&client->dev);

	return 0;

error_media_entity:
	media_entity_cleanup(&imx258->sd.entity);

error_handler_free:
	imx258_free_controls(imx258);

error_identify:
	imx258_power_off(&client->dev);

	return ret;
}

static void imx258_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx258 *imx258 = to_imx258(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	imx258_free_controls(imx258);

	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		imx258_power_off(&client->dev);
	pm_runtime_set_suspended(&client->dev);
}

static const struct dev_pm_ops imx258_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(imx258_suspend, imx258_resume)
	SET_RUNTIME_PM_OPS(imx258_power_off, imx258_power_on, NULL)
};

#ifdef CONFIG_ACPI
static const struct acpi_device_id imx258_acpi_ids[] = {
	{ "SONY258A" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(acpi, imx258_acpi_ids);
#endif

static const struct of_device_id imx258_dt_ids[] = {
	{ .compatible = "sony,imx258" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx258_dt_ids);

static struct i2c_driver imx258_i2c_driver = {
	.driver = {
		.name = "imx258",
		.pm = &imx258_pm_ops,
		.acpi_match_table = ACPI_PTR(imx258_acpi_ids),
		.of_match_table	= imx258_dt_ids,
	},
	.probe_new = imx258_probe,
	.remove = imx258_remove,
};

module_i2c_driver(imx258_i2c_driver);

MODULE_AUTHOR("Yeh, Andy <andy.yeh@intel.com>");
MODULE_AUTHOR("Chiang, Alan");
MODULE_AUTHOR("Chen, Jason");
MODULE_DESCRIPTION("Sony IMX258 sensor driver");
MODULE_LICENSE("GPL v2");
