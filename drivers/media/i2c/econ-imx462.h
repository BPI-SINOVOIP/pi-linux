#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>


#define	IMX462_VERSION		"2.0.2-g158899"
#define IMX462_DEFAULT_DATAFMT	MEDIA_BUS_FMT_UYVY8_2X8
#define IMX462_DEFAULT_MODE	IMX462_MODE_FHD
#define IMX462_DEFAULT_WIDTH	1920
#define IMX462_DEFAULT_HEIGHT	1080
#define MIN_FPS 		30
#define MAX_FPS 		60
#define DEFAULT_FPS 		60
#define VERSION_SIZE		35
#define UPDATE_NEEDED		1
#define NO_UPDATE		0
#define SECTOR_START_ADDR	0x00001000
#define	SECTOR_END_ADDR		0x001FF000
#define FIRST_SECTOR		0
#define LAST_SECTOR		510
#define TOTAL_SECTORS		15
#define FIRST_BLOCK		0
#define TOTAL_BLOCKS		31
#define BLOCK_START_ADDR	0x00010000
#define NEXT_SECTOR		0x00001000
#define PREVIOUS_SECTOR		0x00001000

#define FLASH_COMPLETED		0x00
#define FLASH_INPROGRESS	0x01
#define ERASE_COMPLETED		0x00
#define BLOCK_ERASE_INPROGRESS	0x04
#define SECTOR_ERASE_INPROGRESS	0x01
#define	CALC_COMPLETED		0x00
#define CALC_INPROGRESS		0x04

#define DATA_SIZE		4*1024
#define RETRY_COUNT		5
#define IMX462_MIN_GAIN		1
#define IMX462_MAX_GAIN		50
#define IMX462_DEFAULT_GAIN	1

#define MAX_CTRL_INFO           40
#define MAX_CTRL_DATA_LEN 	100
#define MAX_CTRL_UI_STRING_LEN 	32
#define MAX_CTRL_MENU_ELEM      20

#define V4L2_CID_DENOISE	0x00980922
#define V4L2_CID_FRAMERATE	0x00980923

enum imx462_frame_rate {
	IMX462_30_FPS,
	IMX462_60_FPS,
	IMX462_NUM_FRAMERATES,
};

static const int imx462_framerates[] = {
	[IMX462_30_FPS] = 30,
	[IMX462_60_FPS] = 60,
};

struct imx462_pixfmt {
	u32 code;
	u32 colorspace;
};

static const struct imx462_pixfmt imx462_formats[] = {
	{ MEDIA_BUS_FMT_UYVY8_2X8, V4L2_COLORSPACE_SRGB, },
};


/* regulator supplies */
static const char * const imx462_supply_name[] = {
	"vdddo", /* Digital I/O (1.8V) supply */
	"vdda",  /* Analog (2.8V) supply */
	"vddd",  /* Digital Core (1.5V) supply */
};

#define IMX462_NUM_SUPPLIES ARRAY_SIZE(imx462_supply_name)

struct reg_value {
	u16 reg;
	u8 val;
};

struct imx462_mode_info {
	u32 width;
	u32 height;
	u32 pixel_clock;
	u32 link_freq;
	u32 max_fps;
};

typedef struct _isp_ctrl_ui_info {
	struct {
		char ctrl_name[MAX_CTRL_UI_STRING_LEN];
		uint8_t ctrl_ui_type;
		uint8_t ctrl_ui_flags;
	} ctrl_ui_info;

	/* This Struct is valid only if ctrl_ui_type = 0x03 */
	struct {
		uint8_t num_menu_elem;
		char **menu;
	} ctrl_menu_info;
} ISP_CTRL_UI_INFO;

/* Stream and Control Info Struct */
typedef struct _isp_stream_info {
	uint32_t fmt_fourcc;
	uint16_t width;
	uint16_t height;
	uint8_t frame_rate_type;
	union {
		struct {
			uint16_t frame_rate_num;
			uint16_t frame_rate_denom;
		} disc;
		struct {
			uint16_t frame_rate_min_num;
			uint16_t frame_rate_min_denom;
			uint16_t frame_rate_max_num;
			uint16_t frame_rate_max_denom;
			uint16_t frame_rate_step_num;
			uint16_t frame_rate_step_denom;
		} cont;
	} frame_rate;
} ISP_STREAM_INFO;


struct imx462 {

	struct v4l2_subdev subdev;
	struct v4l2_ctrl_handler ctrls;
	struct i2c_client *i2c_client;
	struct device *dev;
	struct v4l2_fwnode_endpoint ep;
	struct regulator_bulk_data supplies[IMX462_NUM_SUPPLIES];
	struct gpio_desc *pwn_gpio;
	struct gpio_desc *rst_gpio;
	struct v4l2_pix_format pix;
	struct v4l2_captureparm streamcap;
	struct v4l2_subdev sd;
	struct v4l2_ctrl *pixel_clock;
	struct v4l2_ctrl *link_freq;
	struct media_pad pad;
	struct v4l2_mbus_framefmt fmt;
	struct v4l2_rect crop;

	struct clk *xclk;
	const struct imx462_mode_info	*current_mode;
	int power_count;
	int framerate_index;
	int numctrls;

	//Control values
	struct v4l2_ctrl *contrast;
	struct v4l2_ctrl *sharpness;
	struct v4l2_ctrl *gain;
	u8 wb_mode;
	s16 antiBanding_mode;
	u8 exposure_mode;
	struct v4l2_ctrl *exposure_value;
	struct v4l2_ctrl *denoise;
	struct v4l2_ctrl *zoom;
	u8 hflip;
	u8 vflip;
	struct v4l2_ctrl *saturation;
	u8 test_pattern;
	u8 lock;
	u8 wdr_level;
	u8 gamma;
	struct v4l2_ctrl *brightness;
	u8 auto_focus;
	u8 af_range;
	u16 manual_focus;
	struct v4l2_ctrl *framerate;

	u8 streaming;
	struct mutex mcu_i2c_mutex;
	struct v4l2_ctrl *ctrl[];
};

static const s64 link_freq[] = {
	420000000,
	420000000,
	420000000
};

static const struct imx462_mode_info imx462_mode_info_data[] = {
	{
		.width = 640,
		.height = 480,
		.pixel_clock = 210000000,   
		.link_freq = 0, /* an index in link_freq[] */
		.max_fps = IMX462_60_FPS
	},
	{
		.width = 1280,
		.height = 720,
		.pixel_clock = 210000000, 
		.link_freq = 1, /* an index in link_freq[] */
		.max_fps = IMX462_60_FPS
	},
	{
		.width = 1920,
		.height = 1080,
		.pixel_clock = 210000000,
		.link_freq = 2, /* an index in link_freq[] */
		.max_fps = IMX462_60_FPS
	},
};


#define FREE_SAFE(dev, ptr) \
	if(ptr) { \
		devm_kfree(dev, ptr); \
	}

static struct imx462 *to_imx462(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct imx462, subdev);
}

DEFINE_MUTEX(mcu_i2c_mutex);


enum {
	V4L2_CTRL_CONTRAST,
	V4L2_CTRL_AUTO_N_PRESET_WHITE_BALANCE,
	V4L2_CTRL_GAIN,
	V4L2_CTRL_SHARPNESS,
	V4L2_CTRL_EXPOSURE_AUTO,
	V4L2_CTRL_EXPOSURE_ABSOLUTE,
	V4L2_CTRL_POWER_LINE_FREQUENCY,
	V4L2_CTRL_DENOISE,
	V4L2_CTRL_ZOOM_ABSOLUTE,
	V4L2_CTRL_SATURATION,
	V4L2_CTRL_BRIGHTNESS,
	V4L2_CTRL_FOCUS_AUTO,
	V4L2_CTRL_AUTO_FOCUS_RANGE,
	V4L2_CTRL_FOCUS_ABSOLUTE,
	V4L2_CTRL_FRAMERATE,
	IMX462_NUM_CONTROLS
};

enum {
	IMX462_MODE_MIN = 0,
	IMX462_MODE_VGA = 0,
	IMX462_MODE_HD = 1,
	IMX462_MODE_FHD = 2,
	IMX462_MODE_MAX = 2,
};

enum {
	V4L2_EXPOSURE_MENU_AUTO = 0,
	V4L2_EXPOSURE_MENU_MANUAL,
	NUM_MENU_ELEM_EXPOSURE,
};

enum {
	V4L2_DENOISE_MENU_ON = 0,
	V4L2_DENOISE_MENU_OFF,
	NUM_MENU_ELEM_DENOISE,
};

enum {
	V4L2_POWER_LINE_FREQUENCY_MENU_AUTO = 0,
	V4L2_POWER_LINE_FREQUENCY_MENU_50HZ,
	V4L2_POWER_LINE_FREQUENCY_MENU_60HZ,
	V4L2_POWER_LINE_FREQUENCY_MENU_OFF,
	NUM_MENU_ELEM_POWER_LINE_FREQUENCY,
};

enum {
	V4L2_WB_AUTO = 0,
	V4L2_WB_CLOUDY,
	V4L2_WB_DAYLIGHT,
	V4L2_WB_FLASH,
	V4L2_WB_FLUORESCENCT,
	V4L2_WB_TUNGSTEN,
	V4L2_WB_CANDLELIGHT,
	V4L2_WB_HORIZON,
	NUM_MENU_ELEM_WB,
};

enum {
	V4L2_AF_CONTINUOUS = 0,
	V4L2_AF_ONE_SHOT,
	V4L2_MANUAL_FOCUS,
	NUM_MENU_ELEM_AF,
};

enum {
	V4L2_CAF_MACRO_RANGE = 0,
	V4L2_CAF_NORMAL_RANGE,
	V4L2_CAF_FULL_RANGE,
	V4L2_CAF_INFINITY_RANGE,
	V4L2_CAF_HYPERFOCAL_RANGE,
	NUM_MENU_ELEM_CAF,
};
	
enum {
	V4L2_WDR_OFF = 0,
	V4L2_WDR_LOW,
	V4L2_WDR_MIDDLE,
	V4L2_WDR_HIGH,
	NUM_MENU_ELEM_WDR,
};

enum {
	V4L2_TEST_PATTERN_OFF = 0,
	V4L2_TEST_PATTERN_ON,
	NUM_MENU_ELEM_TEST_PATTERN,
};

static const int imx462_30fps_60fps[] = {
	30,
	60,
};


struct imx462_datafmt {
	u32 code;
	enum v4l2_colorspace colorspace;
};

static const struct imx462_datafmt imx462_colour_fmts[] = {
	{MEDIA_BUS_FMT_UYVY8_2X8, V4L2_COLORSPACE_JPEG},
};

struct camera_common_frmfmt {
	struct v4l2_frmsize_discrete	size;
	const int	*framerates;
	int	num_framerates;
	bool	hdr_en;
	int	mode;
};

static const struct v4l2_frmsize_discrete imx462_frmsizes[] = {
	{640, 480},
	{1280, 720},
	{1920, 1080},
};

static const struct camera_common_frmfmt imx462_frmfmt[] = {
	{{640, 480}, imx462_30fps_60fps, 2, 0, IMX462_MODE_VGA},
	{{1280, 720}, imx462_30fps_60fps, 2, 0, IMX462_MODE_HD},
	{{1920, 1080}, imx462_30fps_60fps, 2, 0, IMX462_MODE_FHD},
};

typedef struct _isp_ctrl_info_std {
	uint32_t ctrl_id;
	uint8_t ctrl_type;
	union {
		struct {
			int32_t ctrl_min;
			int32_t ctrl_max;
			int32_t ctrl_def;
			int32_t ctrl_step;
		}std;
		struct {
			uint8_t val_type;
			uint32_t val_length;
			uint64_t ctrl_min;
			uint64_t ctrl_max;
			uint64_t ctrl_def;
			uint64_t ctrl_step;
			/* This size may vary according to ctrl types */
			uint8_t val_data[MAX_CTRL_DATA_LEN];
		}ext;
	}ctrl_data;

	struct {
		char ctrl_name[MAX_CTRL_UI_STRING_LEN];
		uint8_t ctrl_ui_type;
		uint8_t ctrl_ui_flags;		
	}ctrl_ui_info;
	
	/* This Struct is valid only if ctrl_ui_type = 0x03 */
	struct {
		uint8_t num_menu_elem;
		char menu[MAX_CTRL_MENU_ELEM][MAX_CTRL_UI_STRING_LEN];
	}ctrl_menu_info;
} ISP_CTRL_INFO;


struct {
	s32 ctrl_min;
	s32 ctrl_max;
	u32 ctrl_step;
	s32 ctrl_def;
} imx462_ctrl[] = {
	[V4L2_CTRL_CONTRAST] = {0, 10, 1, 5},
	[V4L2_CTRL_AUTO_N_PRESET_WHITE_BALANCE] = {V4L2_WB_AUTO,
		V4L2_WB_HORIZON, 0,
		V4L2_WB_AUTO},
	[V4L2_CTRL_GAIN] = {0, 255, 1, 128},
	[V4L2_CTRL_SHARPNESS] = {0, 4, 1, 2},
	[V4L2_CTRL_EXPOSURE_AUTO] = {V4L2_EXPOSURE_MENU_AUTO,
		V4L2_EXPOSURE_MENU_MANUAL, 0,
		V4L2_EXPOSURE_MENU_AUTO},
	[V4L2_CTRL_DENOISE] = {V4L2_DENOISE_MENU_ON,
		V4L2_DENOISE_MENU_OFF, 0,
		V4L2_DENOISE_MENU_ON},
	[V4L2_CTRL_EXPOSURE_ABSOLUTE] =	{2, 100000, 1, 156},
	[V4L2_CTRL_POWER_LINE_FREQUENCY] = {V4L2_POWER_LINE_FREQUENCY_MENU_AUTO,
		V4L2_POWER_LINE_FREQUENCY_MENU_OFF, 0,
		V4L2_POWER_LINE_FREQUENCY_MENU_AUTO},
	[V4L2_CTRL_ZOOM_ABSOLUTE] = {1, 30, 1, 1},
	[V4L2_CTRL_SATURATION] = {0, 63, 1, 32},
	[V4L2_CTRL_BRIGHTNESS] = {0, 255, 1, 110},
	[V4L2_CTRL_FOCUS_AUTO] = {V4L2_AF_CONTINUOUS, V4L2_MANUAL_FOCUS, 0, V4L2_AF_CONTINUOUS},
	[V4L2_CTRL_AUTO_FOCUS_RANGE] = {V4L2_CAF_MACRO_RANGE, V4L2_CAF_HYPERFOCAL_RANGE, 0, V4L2_CAF_FULL_RANGE},
	[V4L2_CTRL_FOCUS_ABSOLUTE] = {0, 102, 1, 0},
	[V4L2_CTRL_FRAMERATE] = {30,60,30,60},
};


#define ctrl(nctrl, def) imx462_ctrl[V4L2_CTRL_##nctrl].ctrl_##def

#define ADD_CUSTOM_CTRL(name,min,max,step,def,menu_name,type,flag) [V4L2_CTRL_##name] = {	\
	.ctrl_id = V4L2_CID_##name,	\
	.ctrl_type = CTRL_STANDARD,	\
	.ctrl_data = {	\
			.std = {	\
				.ctrl_min = min,	\
				.ctrl_max = max,	\
				.ctrl_def = def,	\
				.ctrl_step = step,	\
			}	\
		},	\
		.ctrl_ui_info = {	\
				.ctrl_name = #menu_name,	\
				.ctrl_ui_type = type,	\
				.ctrl_ui_flags = flag,	\
		},		\
	},	\

static const struct v4l2_ctrl_ops imx462_ctrl_ops;

static const struct v4l2_ctrl_config imx462_custom_controls[]={
	{
		.ops = &imx462_ctrl_ops,
		.id = V4L2_CID_DENOISE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "denoise",
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
		.flags = 0,	
	},
	{
		.ops = &imx462_ctrl_ops,
		.id = V4L2_CID_FRAMERATE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "framerate",
		.min = 30,
		.max = 60,
		.step = 30,
		.def = 60,
		.flags = 0,	
	},
};


static const char * const white_balance_menu[]={
	"Auto",
	"Cloudy",
	"Daylight",
	"Flash",
	"Cool_white_fluorescent",
	"Tungsten",
	"Candlelight",
	"Horozon",
};

static const char * const exposure_mode[]={
	"exposure_auto",
	"exposure_manual",
};

static const char * const denoise_mode[]={
	"denoise_on",
	"denoise_off",
};

static const char * const antibanding_mode[]={
	"Auto",
	"50HZ",
	"60HZ",
	"Off",
};

static int imx462_s_ctrl(struct v4l2_ctrl *ctrl);
extern int camera_initialization(struct imx462 *);
static void toggle_gpio(struct gpio_desc *gpio, int delay);
static int imx462_set_defaults(struct i2c_client *client);
static int imx462_reconfigure(struct imx462 *priv);
static int imx462_zoom(struct i2c_client *client, u8 value);
static int isp_fw_version_check(struct i2c_client *client);
static int isp_fw_update(struct i2c_client *client);
static int isp_start_firmware(struct i2c_client *client);
static int imx462_ctrls_init(struct imx462 *sensor);
static int imx462_write(struct i2c_client *client, u8 * val, u32 count);
static int imx462_read(struct i2c_client *client, u8 * val, u32 count);
static int imx462_reconfigure(struct imx462 *sensor);
//static int get_isp_logs(struct i2c_client *client);
static int imx462_s_power(struct v4l2_subdev *sd, int on);
static int imx462_s_stream(struct v4l2_subdev *sd, int enable);
static int imx462_entity_init_cfg(struct v4l2_subdev *subdev,struct v4l2_subdev_state *cfg);
static int imx462_set_fmt(struct v4l2_subdev *sd,struct v4l2_subdev_state *cfg,struct v4l2_subdev_format *format);
static int imx462_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *cfg, struct v4l2_subdev_format *format);
static int imx462_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_state *cfg, struct v4l2_subdev_mbus_code_enum *code);
static int imx462_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_subdev_state *cfg, struct v4l2_subdev_frame_size_enum *fse);
static int imx462_s_frame_interval(struct v4l2_subdev *sd,struct v4l2_subdev_frame_interval *fi);
static int imx462_g_frame_interval(struct v4l2_subdev *sd,struct v4l2_subdev_frame_interval *fi);

/* ISP Command Structure */
static u8 intROMcmd1[5]		= {0x05, 0x02, 0x0F, 0x57, 0x01};
static u8 intROMcmd2[5]		= {0x05, 0x02, 0x0F, 0x57, 0x00};
static u8 fwReadcmd[8]		= {0x00, 0x03, 0x18, 0x00, 0x10, 0x00, 0x00, 0x20};
static u8 irFactor1[5]		= {0x05, 0x01, 0x0F, 0x10, 0x01};
static u8 irFactor2[5]		= {0x05, 0x01, 0x00, 0x1C, 0x01};
static u8 glRdFwStartIntr[5]	= {0x05, 0x01, 0x0F, 0x10, 0x01};
static u8 setPLL[10]		= {0x0A, 0x02, 0x0F, 0x1C, 0x00, 0x1B, 0x01, 0x7C, 0x11, 0xA5};
static u8 startRAM[5]		= {0x05, 0x02, 0x0F, 0x4A, 0x01};
static u8 setROMaddr[8]		= {0x08, 0x02, 0x0F, 0x00};
static u8 eraseSector[5]	= {0x05, 0x02, 0x0F, 0x06, 0x01};
static u8 eraseBlock[5]		= {0x05, 0x02, 0x0F, 0x06, 0x04};
static u8 eraseStatus[5]	= {0x05, 0x01, 0x0F, 0x06, 0x01};
static u8 setFWsize[6]		= {0x06, 0x02, 0x0F, 0x04, 0x10, 0x00};
static u8 fwSendcmd[4104]	= {0x00, 0x04, 0x40, 0x00, 0x00, 0x00, 0x10, 0x00};
static u8 programROM[5]		= {0x05, 0x02, 0x0F, 0x07, 0x01};
static u8 flashStatus[5]	= {0x05, 0x01, 0x0F, 0x07, 0x01};
static u8 calcChkSum[5]		= {0x05, 0x02, 0x0F, 0x09, 0x04};
static u8 calcStatus[5] 	= {0x05, 0x01, 0x0F, 0x09, 0x01};
static u8 getChkSum[5]		= {0x05, 0x01, 0x0F, 0x0A, 0x02};
static u8 intStartcmd[5]	= {0x05, 0x02, 0x0F, 0x12, 0x01};
static u8 setCamMode[5]		= {0x05, 0x02, 0x00, 0x0B, 0x02};
static u8 setCamOff[5]		= {0x05, 0x02, 0x00, 0x0B, 0x01};
static u8 slctFmt[5]		= {0x05, 0x02, 0x01, 0x07, 0x03};
static u8 slctSensr[5]		= {0x05, 0x02, 0x00, 0x17, 0x00};
static u8 slctFrate60[5]	= {0x05, 0x02, 0x01, 0x02, 0x08};
static u8 slctReslnFHD_30fps[5]	= {0x05, 0x02, 0x01, 0x01, 0x67};
static u8 slctReslnFHD_60fps[5]	= {0x05, 0x02, 0x01, 0x01, 0x28};
static u8 slctReslnHD_30fps[5] 	= {0x05, 0x02, 0x01, 0x01, 0x25};
static u8 slctReslnHD_60fps[5] 	= {0x05, 0x02, 0x01, 0x01, 0x21};
static u8 slctReslnVGA_30fps[5] = {0x05, 0x02, 0x01, 0x01, 0x68};
static u8 slctReslnVGA_60fps[5] = {0x05, 0x02, 0x01, 0x01, 0x17};
static u8 rawPreviewOFF[5]	= {0x05, 0x02, 0x02, 0x12, 0x00};
static u8 ctrlcmd[5]		= {0x05, 0x02};
static u8 flickerDetectCmd[5]	= {0x05, 0x02, 0x03, 0x06, 0x05};


static const u8 bootData[] = {
#include "imx462_firmware.h"
};


