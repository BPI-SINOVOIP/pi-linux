// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Synaptics Incorporated
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/sort.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <soc/berlin/sm.h>

/*
 * IR key definition
 */
typedef enum {
	MV_IR_KEY_NULL = 0, /* no key */

	MV_IR_KEY_POWER = 0x0200,
	MV_IR_KEY_OPENCLOSE,
	MV_IR_KEY_BEGIN_PAIRING,
	MV_IR_KEY_RECOVERY, /* special Sony reboot to the recovery mode */

	/* digital keys */
	MV_IR_KEY_DIGIT_1 = 0x1000,
	MV_IR_KEY_DIGIT_2,
	MV_IR_KEY_DIGIT_3,
	MV_IR_KEY_DIGIT_4,
	MV_IR_KEY_DIGIT_5,
	MV_IR_KEY_DIGIT_6,
	MV_IR_KEY_DIGIT_7,
	MV_IR_KEY_DIGIT_8,
	MV_IR_KEY_DIGIT_9,
	MV_IR_KEY_DIGIT_0,

	/* for BD */
	MV_IR_KEY_INFO = 0x2000,
	MV_IR_KEY_SETUPMENU,
	MV_IR_KEY_CANCEL, /* no key */

	MV_IR_KEY_DISCMENU,
	MV_IR_KEY_TITLEMENU,
	MV_IR_KEY_SUBTITLE,
	MV_IR_KEY_ANGLE,
	MV_IR_KEY_AUDIO,
	MV_IR_KEY_SEARCH,
	MV_IR_KEY_ZOOM,
	MV_IR_KEY_DISPLAY,

	MV_IR_KEY_REPEAT,
	MV_IR_KEY_REPEAT_AB,
	MV_IR_KEY_PIP,
	MV_IR_KEY_EXIT,
#define MV_IR_KEY_RED		MV_IR_KEY_A
#define MV_IR_KEY_GREEN		MV_IR_KEY_B
#define MV_IR_KEY_YELLOW	MV_IR_KEY_C
#define MV_IR_KEY_BLUE		MV_IR_KEY_D
	MV_IR_KEY_A,
	MV_IR_KEY_B,
	MV_IR_KEY_C,
	MV_IR_KEY_D,

	/* IR misc around ENTER */
	MV_IR_KEY_CLEAR = 0x3000,
	MV_IR_KEY_MEDIA_MODE,
	MV_IR_KEY_STEP,
	MV_IR_KEY_RETURN,
	MV_IR_KEY_TITLE,

	/* up down left right enter */
	MV_IR_KEY_UP = 0x4000,
	MV_IR_KEY_DOWN,
	MV_IR_KEY_LEFT,
	MV_IR_KEY_RIGHT,
	MV_IR_KEY_ENTER,

	/* for BD */
	MV_IR_KEY_SLOW,
	MV_IR_KEY_PAUSE,
	MV_IR_KEY_PLAY,
	MV_IR_KEY_STOP,
	MV_IR_KEY_PLAY_PAUSE, /* no key */

	MV_IR_KEY_SKIP_BACKWARD,
	MV_IR_KEY_SKIP_FORWARD,
	MV_IR_KEY_SLOW_BACKWARD, /* no key */
	MV_IR_KEY_SLOW_FORWARD,  /* no key */
	MV_IR_KEY_FAST_BACKWARD,
	MV_IR_KEY_FAST_FORWARD,

	/* bottom keys */
	MV_IR_KEY_F1 = 0x5000,
	MV_IR_KEY_F2,
	MV_IR_KEY_F3,
	MV_IR_KEY_F4,
	MV_IR_KEY_F5,
	MV_IR_KEY_F6,
	MV_IR_KEY_F7,
	MV_IR_KEY_F8,

	/* for future */
	MV_IR_KEY_VOL_PLUS = 0x6000, /* no key */
	MV_IR_KEY_VOL_MINUS, /* no key */
	MV_IR_KEY_VOL_MUTE, /* no key */
	MV_IR_KEY_CHANNEL_PLUS, /* no key */
	MV_IR_KEY_CHANNEL_MINUS, /* no key */
	MV_IR_KEY_HOME, /* for some vendor */
	MV_IR_KEY_MUSIC_ID, /* for some vendor */
	MV_IR_KEY_HOMEPAGE,

	/* obsoleted keys */
	MV_IR_KEY_MENU,
	MV_IR_KEY_INPUTSEL,
	MV_IR_KEY_ANYNET,
	MV_IR_KEY_TELEVISION,
	MV_IR_KEY_CHANNEL_LIST,
	MV_IR_KEY_TVPOWER,
	MV_IR_KEY_MARKER,
	MV_IR_KEY_VIDEO_FORMAT,
	MV_IR_KEY_GUIDE,
	MV_IR_KEY_VIZIO,
	MV_IR_KEY_LAST,
	MV_IR_KEY_NUMBER_SWITCH,
	MV_IR_KEY_WIDE,
	MV_IR_KEY_RECODER,
	MV_IR_KEY_CLOSEDCAPTION,
	MV_IR_KEY_QUICK,
	MV_IR_KEY_INPUT,
	MV_IR_KEY_AMAZON,
	MV_IR_KEY_NETFLIX,
	MV_IR_KEY_VUDU,
	MV_IR_KEY_DASH,
	MV_IR_KEY_M_GO,
#define MV_IR_KEY_LIVE_TV	MV_IR_KEY_TELEVISION
#define MV_IR_KEY_VIA		MV_IR_KEY_HOME
	MV_IR_KEY_TV_INPUT,
	MV_IR_KEY_LIST,
	MV_IR_KEY_REC,
	MV_IR_KEY_BACK,

	/* qwerty keyboards */
	MV_IR_KB_ESC = 0x8000,
#define MV_IR_KB_1 		MV_IR_KEY_DIGIT_1
#define MV_IR_KB_2 		MV_IR_KEY_DIGIT_2
#define MV_IR_KB_3 		MV_IR_KEY_DIGIT_3
#define MV_IR_KB_4 		MV_IR_KEY_DIGIT_4
#define MV_IR_KB_5 		MV_IR_KEY_DIGIT_5
#define MV_IR_KB_6 		MV_IR_KEY_DIGIT_6
#define MV_IR_KB_7 		MV_IR_KEY_DIGIT_7
#define MV_IR_KB_8 		MV_IR_KEY_DIGIT_8
#define MV_IR_KB_9 		MV_IR_KEY_DIGIT_9
#define MV_IR_KB_0 		MV_IR_KEY_DIGIT_0
	MV_IR_KB_MINUS,
	MV_IR_KB_EQUAL,
	MV_IR_KB_BACKSPACE,
	MV_IR_KB_TAB,
	MV_IR_KB_Q,
	MV_IR_KB_W,
	MV_IR_KB_E,
	MV_IR_KB_R,
	MV_IR_KB_T,
	MV_IR_KB_Y,
	MV_IR_KB_U,
	MV_IR_KB_I,
	MV_IR_KB_O,
	MV_IR_KB_P,
	MV_IR_KB_LEFTBRACE,
	MV_IR_KB_RIGHTBRACE,
#define MV_IR_KB_ENTER		MV_IR_KEY_ENTER
	MV_IR_KB_LEFTCTRL,
	MV_IR_KB_A,
	MV_IR_KB_S,
	MV_IR_KB_D,
	MV_IR_KB_F,
	MV_IR_KB_G,
	MV_IR_KB_H,
	MV_IR_KB_J,
	MV_IR_KB_K,
	MV_IR_KB_L,
	MV_IR_KB_SEMICOLON,
	MV_IR_KB_APOSTROPHE,
	MV_IR_KB_GRAVE,
	MV_IR_KB_LEFTSHIFT,
	MV_IR_KB_BACKSLASH,
	MV_IR_KB_Z,
	MV_IR_KB_X,
	MV_IR_KB_C,
	MV_IR_KB_V,
	MV_IR_KB_B,
	MV_IR_KB_N,
	MV_IR_KB_M,
	MV_IR_KB_COMMA,
	MV_IR_KB_DOT,
	MV_IR_KB_SLASH,
	MV_IR_KB_RIGHTSHIFT,
	MV_IR_KB_LEFTALT,
	MV_IR_KB_SPACE,
	MV_IR_KB_CAPSLOCK,
	MV_IR_KB_F1,
	MV_IR_KB_F2,
	MV_IR_KB_F3,
	MV_IR_KB_F4,
	MV_IR_KB_F5,
	MV_IR_KB_F6,
	MV_IR_KB_F7,
	MV_IR_KB_F8,
	MV_IR_KB_F9,
	MV_IR_KB_F10,
	MV_IR_KB_NUMLOCK,
	MV_IR_KB_SCROLLLOCK,

	MV_IR_KB_ZENKAKUHANKAKU,
	MV_IR_KB_102ND,
	MV_IR_KB_F11,
	MV_IR_KB_F12,
	MV_IR_KB_RO,
	MV_IR_KB_KATAKANA,
	MV_IR_KB_HIRAGANA,
	MV_IR_KB_HENKAN,
	MV_IR_KB_KATAKANAHIRAGANA,
	MV_IR_KB_MUHENKAN,
	MV_IR_KB_RIGHTCTRL,
	MV_IR_KB_SYSRQ,
	MV_IR_KB_RIGHTALT,
	MV_IR_KB_LINEFEED,
#define MV_IR_KB_HOME		MV_IR_KEY_HOME
#define MV_IR_KB_UP		MV_IR_KEY_UP
	MV_IR_KB_PAGEUP,
#define MV_IR_KB_LEFT		MV_IR_KEY_LEFT
#define MV_IR_KB_RIGHT		MV_IR_KEY_RIGHT
	MV_IR_KB_END,
#define MV_IR_KB_DOWN		MV_IR_KEY_DOWN
	MV_IR_KB_PAGEDOWN,
	MV_IR_KB_INSERT,
	MV_IR_KB_DELETE,
	MV_IR_KB_MACRO,
#define MV_IR_KB_MUTE		MV_IR_KEY_MUTE
#define MV_IR_KB_VOLUMEDOWN	MV_IR_KEY_VOL_MINUS
#define MV_IR_KB_VOLUMEUP	MV_IR_KEY_VOL_PLuS
	MV_IR_KB_POWER,
	MV_IR_KB_PAUSE,
	MV_IR_KB_SCALE,

	MV_IR_KB_HANGEUL,
	MV_IR_KB_HANGUEL,
	MV_IR_KB_HANJA,
	MV_IR_KB_YEN,
	MV_IR_KB_LEFTMETA,
	MV_IR_KB_RIGHTMETA,
	MV_IR_KB_COMPOSE,

	/* Keypad keys */
	MV_IR_KB_KP0 = 0x8100,
	MV_IR_KB_KP1,
	MV_IR_KB_KP2,
	MV_IR_KB_KP3,
	MV_IR_KB_KP4,
	MV_IR_KB_KP5,
	MV_IR_KB_KP6,
	MV_IR_KB_KP7,
	MV_IR_KB_KP8,
	MV_IR_KB_KP9,
	MV_IR_KB_KPSLASH,
	MV_IR_KB_KPJPCOMMA,
	MV_IR_KB_KPCOMMA,
	MV_IR_KB_KPASTERISK,
	MV_IR_KB_KPMINUS,
	MV_IR_KB_KPPLUS,
	MV_IR_KB_KPPLUSMINUS,
	MV_IR_KB_KPENTER,
	MV_IR_KB_KPDOT,
	MV_IR_KB_KPEQUAL,
	MV_IR_KB_KPLEFTPAREN,
	MV_IR_KB_KPRIGHTPAREN,

	/* Game PAD */
	MV_IR_BTN_GAMEPAD = 0x8200,
	MV_IR_BTN_A,
	MV_IR_BTN_B,
	MV_IR_BTN_C,
	MV_IR_BTN_X,
	MV_IR_BTN_Y,
	MV_IR_BTN_Z,
	MV_IR_BTN_TL,
	MV_IR_BTN_TR,
	MV_IR_BTN_TL2,
	MV_IR_BTN_TR2,
	MV_IR_BTN_SELECT,
	MV_IR_BTN_START,
	MV_IR_BTN_MODE,
	MV_IR_BTN_THUMBL,
	MV_IR_BTN_THUMBR,

	/* Direction Pad (DPAD) */
	MV_IR_BTN_UP,
	MV_IR_BTN_DOWN,
	MV_IR_BTN_LEFT,
	MV_IR_BTN_RIGHT,

	/* Compose keys */
	MV_IR_KB_UPPER_A = 0x8300,
	MV_IR_KB_UPPER_B,
	MV_IR_KB_UPPER_C,
	MV_IR_KB_UPPER_D,
	MV_IR_KB_UPPER_E,
	MV_IR_KB_UPPER_F,
	MV_IR_KB_UPPER_G,
	MV_IR_KB_UPPER_H,
	MV_IR_KB_UPPER_I,
	MV_IR_KB_UPPER_J,
	MV_IR_KB_UPPER_K,
	MV_IR_KB_UPPER_L,
	MV_IR_KB_UPPER_M,
	MV_IR_KB_UPPER_N,
	MV_IR_KB_UPPER_O,
	MV_IR_KB_UPPER_P,
	MV_IR_KB_UPPER_Q,
	MV_IR_KB_UPPER_R,
	MV_IR_KB_UPPER_S,
	MV_IR_KB_UPPER_T,
	MV_IR_KB_UPPER_U,
	MV_IR_KB_UPPER_V,
	MV_IR_KB_UPPER_W,
	MV_IR_KB_UPPER_X,
	MV_IR_KB_UPPER_Y,
	MV_IR_KB_UPPER_Z,

	MV_IR_KB_TILDE,         // ~
	MV_IR_KB_FACTORIAL,     // !
	MV_IR_KB_AT,            // @
	MV_IR_KB_SHARP,         // #
	MV_IR_KB_DOLLAR,        // $
	MV_IR_KB_PERCENTAGE,    // %
	MV_IR_KB_CARET,         // ^
	MV_IR_KB_AND,           // &
	MV_IR_KB_ASTERISK,      // *
	MV_IR_KB_LEFTRBRACKET,  // (
	MV_IR_KB_RIGHTRBRACKET, // )
	MV_IR_KB_UNDERSCORE,    // _
	MV_IR_KB_PLUS,          // +
	MV_IR_KB_LEFTCBRACKET,  // {
	MV_IR_KB_RIGHTCBRACKET, // }
	MV_IR_KB_OR,            // |
	MV_IR_KB_COLON,         // :
	MV_IR_KB_QUOTATION,     // "
	MV_IR_KB_LEFTABRACKET,  // <
	MV_IR_KB_RIGHTABRACKET, // >
	MV_IR_KB_QUESTION,      // ?

	/* Cotehill-Specific keys */
	MV_IR_KEY_FACTORY_RESET = 0x8400,
	MV_IR_KEY_BLASTER_FACTORY_TEST,

	/* Keys in front panel */
#define MV_FP_KEY_POWER MV_IR_KEY_POWER
	MV_FP_KEY_VOL_PLUS = 0x8500,
	MV_FP_KEY_VOL_MINUS,
	MV_FP_KEY_CHANNEL_PLUS,
	MV_FP_KEY_CHANNEL_MINUS,
	MV_FP_KEY_HOME,
	MV_FP_KEY_INPUT,

	MV_HARD_KEY_FACTORY_RESET = 0x9400,

	/* Customization Keys */
	MV_IR_KEY_CUSTOM0 = 0xC000,
	MV_IR_KEY_CUSTOM1,
	MV_IR_KEY_CUSTOM2,
	MV_IR_KEY_CUSTOM3,
	MV_IR_KEY_CUSTOM4,
	MV_IR_KEY_CUSTOM5,
	MV_IR_KEY_CUSTOM6,
	MV_IR_KEY_CUSTOM7,
	MV_IR_KEY_CUSTOM8,
	MV_IR_KEY_CUSTOM9,
	MV_IR_KEY_CUSTOM10,
	MV_IR_KEY_CUSTOM11,
	MV_IR_KEY_CUSTOM12,
	MV_IR_KEY_CUSTOM13,
	MV_IR_KEY_CUSTOM14,
	MV_IR_KEY_CUSTOM15,
	MV_IR_KEY_CUSTOM16,
	MV_IR_KEY_CUSTOM17,
	MV_IR_KEY_CUSTOM18,
	MV_IR_KEY_CUSTOM19,
	MV_IR_KEY_CUSTOM20,
	MV_IR_KEY_CUSTOM21,
	MV_IR_KEY_CUSTOM22,
	MV_IR_KEY_CUSTOM23,
	MV_IR_KEY_CUSTOM24,
	MV_IR_KEY_CUSTOM25,
	MV_IR_KEY_CUSTOM26,
	MV_IR_KEY_CUSTOM27,
	MV_IR_KEY_CUSTOM28,
	MV_IR_KEY_CUSTOM29,
	MV_IR_KEY_CUSTOM30,
	MV_IR_KEY_CUSTOM31,

	MV_IR_KEY_MAX

} MV_IR_KEY_CODE_t;

static struct input_dev *ir_input;
static bool ir_hw_repeat;

struct ir_keymap {
	MV_IR_KEY_CODE_t ircode;
	__u32 keycode;
};

#define MAKE_KEYCODE(modifier, scancode) ((modifier<<16)|scancode)

#define MAKE_IR_KEYMAP(i, m, s)		\
	{.ircode = i, .keycode = MAKE_KEYCODE(m, s) }

static inline int galois_ir_get_modifier(struct ir_keymap *keymap)
{
	return (keymap->keycode >> 16) & 0xffff;
}

static inline int galois_ir_get_scancode(struct ir_keymap *keymap)
{
	return keymap->keycode & 0xffff;
}

static int galois_ir_cmp(const void *x1, const void *x2)
{
	const struct ir_keymap *ir1 = (const struct ir_keymap *)x1;
	const struct ir_keymap *ir2 = (const struct ir_keymap *)x2;

	return ir1->ircode - ir2->ircode;
}

static struct ir_keymap *
galois_ir_bsearch(struct ir_keymap *keymap,
		  int nr_keymap,
		  MV_IR_KEY_CODE_t ircode)
{
	int i, j, m;

	i = 0;
	j = nr_keymap - 1;
	while (i <= j) {
		m = (i + j) / 2;
		if (keymap[m].ircode == ircode)
			return &(keymap[m]);
		else if (keymap[m].ircode > ircode)
			j = m - 1;
		else
			i = m + 1;
	}
	return NULL;
}

/*
 * Default IR Key mapping
 */
static struct ir_keymap keymap_tab[] = {
	MAKE_IR_KEYMAP(MV_IR_KEY_NULL,          0, KEY_RESERVED), /* no key */
	MAKE_IR_KEYMAP(MV_IR_KEY_POWER,	        0, KEY_POWER),
	MAKE_IR_KEYMAP(MV_IR_KEY_OPENCLOSE,     0, KEY_OPEN),
	MAKE_IR_KEYMAP(MV_IR_KEY_BEGIN_PAIRING, 0, KEY_BLUETOOTH),

	/* digital keys */
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_1,       0, KEY_1),
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_2,       0, KEY_2),
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_3,       0, KEY_3),
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_4,       0, KEY_4),
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_5,       0, KEY_5),
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_6,       0, KEY_6),
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_7,       0, KEY_7),
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_8,       0, KEY_8),
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_9,       0, KEY_9),
	MAKE_IR_KEYMAP(MV_IR_KEY_DIGIT_0,       0, KEY_0),

	/* for BD */
	MAKE_IR_KEYMAP(MV_IR_KEY_HOME,          0, KEY_HOMEPAGE),
	MAKE_IR_KEYMAP(MV_IR_KEY_BACK,          0, KEY_BACK),
	MAKE_IR_KEYMAP(MV_IR_KEY_INFO,          0, KEY_INFO),
	MAKE_IR_KEYMAP(MV_IR_KEY_SETUPMENU,     0, KEY_HOME),
	MAKE_IR_KEYMAP(MV_IR_KEY_CANCEL,        0, KEY_CANCEL), /* no key */
	MAKE_IR_KEYMAP(MV_IR_KEY_HOMEPAGE,      0, KEY_HOMEPAGE),

	MAKE_IR_KEYMAP(MV_IR_KEY_DISCMENU,      0, KEY_CONTEXT_MENU),
	MAKE_IR_KEYMAP(MV_IR_KEY_TITLEMENU,     0, KEY_TITLE),
	MAKE_IR_KEYMAP(MV_IR_KEY_SUBTITLE,      0, KEY_SUBTITLE),
	MAKE_IR_KEYMAP(MV_IR_KEY_ANGLE,         0, KEY_ANGLE),
	MAKE_IR_KEYMAP(MV_IR_KEY_AUDIO,         0, KEY_AUDIO),
	MAKE_IR_KEYMAP(MV_IR_KEY_SEARCH,        0, KEY_SEARCH),
	MAKE_IR_KEYMAP(MV_IR_KEY_ZOOM,          0, KEY_ZOOM),
	MAKE_IR_KEYMAP(MV_IR_KEY_DISPLAY,       0, KEY_SCREEN),

	MAKE_IR_KEYMAP(MV_IR_KEY_REPEAT,        0, KEY_MEDIA_REPEAT),
	MAKE_IR_KEYMAP(MV_IR_KEY_REPEAT_AB,     0, KEY_AB),
	MAKE_IR_KEYMAP(MV_IR_KEY_EXIT,          0, KEY_EXIT),
	MAKE_IR_KEYMAP(MV_IR_KEY_A,             0, KEY_RED),
	MAKE_IR_KEYMAP(MV_IR_KEY_B,             0, KEY_GREEN),
	MAKE_IR_KEYMAP(MV_IR_KEY_C,             0, KEY_YELLOW),
	MAKE_IR_KEYMAP(MV_IR_KEY_D,             0, KEY_BLUE),

	/* IR misc around ENTER */
	MAKE_IR_KEYMAP(MV_IR_KEY_CLEAR,         0, KEY_HOMEPAGE),
	MAKE_IR_KEYMAP(MV_IR_KEY_VIDEO_FORMAT,  0, KEY_VIDEO),
	MAKE_IR_KEYMAP(MV_IR_KEY_RETURN,        0, KEY_ESC),
	MAKE_IR_KEYMAP(MV_IR_KEY_MEDIA_MODE,    0, KEY_MODE),

	/* up down left right enter */
	MAKE_IR_KEYMAP(MV_IR_KEY_UP,            0, KEY_UP),
	MAKE_IR_KEYMAP(MV_IR_KEY_DOWN,          0, KEY_DOWN),
	MAKE_IR_KEYMAP(MV_IR_KEY_LEFT,          0, KEY_LEFT),
	MAKE_IR_KEYMAP(MV_IR_KEY_RIGHT,         0, KEY_RIGHT),
	MAKE_IR_KEYMAP(MV_IR_KEY_ENTER,         0, KEY_ENTER),

	/* for BD */
	MAKE_IR_KEYMAP(MV_IR_KEY_SLOW,          0, KEY_SLOW),
	MAKE_IR_KEYMAP(MV_IR_KEY_PAUSE,         0, KEY_PAUSE),
	MAKE_IR_KEYMAP(MV_IR_KEY_PLAY,          0, KEY_PLAYPAUSE),
	MAKE_IR_KEYMAP(MV_IR_KEY_STOP,          0, KEY_STOP),
	MAKE_IR_KEYMAP(MV_IR_KEY_PLAY_PAUSE,    0, KEY_PLAYPAUSE), /* no key */

	MAKE_IR_KEYMAP(MV_IR_KEY_SKIP_BACKWARD, 0, KEY_CHANNELDOWN),
	MAKE_IR_KEYMAP(MV_IR_KEY_SKIP_FORWARD,  0, KEY_CHANNELUP),
	MAKE_IR_KEYMAP(MV_IR_KEY_SLOW_BACKWARD, 0, KEY_RESERVED),  /* no key */
	MAKE_IR_KEYMAP(MV_IR_KEY_SLOW_FORWARD,  0, KEY_RESERVED),  /* no key */
	MAKE_IR_KEYMAP(MV_IR_KEY_FAST_BACKWARD, 0, KEY_REWIND),
	MAKE_IR_KEYMAP(MV_IR_KEY_FAST_FORWARD,  0, KEY_FORWARD),

	/* bottom keys */
	MAKE_IR_KEYMAP(MV_IR_KEY_F1,            0, KEY_F1),
	MAKE_IR_KEYMAP(MV_IR_KEY_F2,            0, KEY_F2),
	MAKE_IR_KEYMAP(MV_IR_KEY_F3,            0, KEY_F3),
	MAKE_IR_KEYMAP(MV_IR_KEY_F4,            0, KEY_ASSISTANT),
	MAKE_IR_KEYMAP(MV_IR_KEY_F5,            0, KEY_F5),
	MAKE_IR_KEYMAP(MV_IR_KEY_F6,            0, KEY_VOLUMEDOWN),
	MAKE_IR_KEYMAP(MV_IR_KEY_F7,            0, KEY_VOLUMEUP),
	MAKE_IR_KEYMAP(MV_IR_KEY_F8,            0, KEY_F8),
	MAKE_IR_KEYMAP(MV_IR_KEY_TITLE,         0, KEY_TITLE),

	/* for future */
	MAKE_IR_KEYMAP(MV_IR_KEY_VOL_PLUS,      0, KEY_VOLUMEUP),   /* no key */
	MAKE_IR_KEYMAP(MV_IR_KEY_VOL_MINUS,     0, KEY_VOLUMEDOWN), /* no key */
	MAKE_IR_KEYMAP(MV_IR_KEY_VOL_MUTE,      0, KEY_MUTE),       /* no key */
	MAKE_IR_KEYMAP(MV_IR_KEY_CHANNEL_PLUS,  0, KEY_CHANNELUP),  /* no key */
	MAKE_IR_KEYMAP(MV_IR_KEY_CHANNEL_MINUS, 0, KEY_CHANNELDOWN),/* no key */

	/* obsoleted keys */
	MAKE_IR_KEYMAP(MV_IR_KEY_MENU,          0, KEY_HOMEPAGE),
	MAKE_IR_KEYMAP(MV_IR_KEY_INPUTSEL,      0, KEY_RESERVED),
	MAKE_IR_KEYMAP(MV_IR_KEY_ANYNET,        0, KEY_RESERVED),
	MAKE_IR_KEYMAP(MV_IR_KEY_TELEVISION,    0, KEY_TV),
	MAKE_IR_KEYMAP(MV_IR_KEY_CHANNEL_LIST,  0, KEY_RESERVED),
	MAKE_IR_KEYMAP(MV_IR_KEY_TVPOWER,       0, KEY_RESERVED),
};

static int ir_input_open(struct input_dev *dev)
{
	int msg = MV_SM_IR_Linuxready;

	bsm_msg_send(MV_SM_ID_IR, &msg, 4);
	bsm_msg_send(MV_SM_ID_POWER, &msg, 4);

	return 0;
}

static void girl_sm_int(int ir_key)
{
	struct ir_keymap *km;

	km = galois_ir_bsearch(keymap_tab,
			ARRAY_SIZE(keymap_tab), (ir_key & 0xffff));
	if (km) {
		int pressed;
		int scancode = galois_ir_get_scancode(km);

		if (ir_key & 0x80000000) {
			pressed = ir_hw_repeat ? 2 : 1;
			input_event(ir_input, EV_KEY, scancode, pressed);
		} else {
			int modifier = galois_ir_get_modifier(km);
			pressed = !(ir_key & 0x8000000);

			if (pressed && modifier)
				input_event(ir_input, EV_KEY,
						modifier, pressed);

			input_event(ir_input, EV_KEY, scancode, pressed);

			if (!pressed && modifier)
				input_event(ir_input, EV_KEY,
						modifier, pressed);
		}
		input_sync(ir_input);
	} else
		printk(KERN_WARNING "lack of key mapping for 0x%x\n", ir_key);
}

static struct sm_ir_handler ir_handler = {
	.fn = girl_sm_int,
};

static int berlin_ir_probe(struct platform_device *pdev)
{
	int i, error, scancode, modifier;

	ir_input = input_allocate_device();
	if (!ir_input) {
		printk("error: failed to alloc input device for IR.\n");
		return -ENOMEM;
	}

	ir_hw_repeat = of_property_read_bool(pdev->dev.of_node, "hw-repeat");
	if (!ir_hw_repeat)
		set_bit(EV_REP, ir_input->evbit);

	ir_input->name = "Infra-Red";
	ir_input->phys = "Infra-Red/input0";
	ir_input->id.bustype = BUS_HOST;
	ir_input->id.vendor = 0x0001;
	ir_input->id.product = 0x0001;
	ir_input->id.version = 0x0100;

	ir_input->open = ir_input_open;

	/* sort the keymap in order */
	sort(keymap_tab, ARRAY_SIZE(keymap_tab),
		sizeof(struct ir_keymap), galois_ir_cmp, NULL);

	for (i = 0; i < ARRAY_SIZE(keymap_tab); i++) {
		scancode = galois_ir_get_scancode(&(keymap_tab[i]));
		modifier = galois_ir_get_modifier(&(keymap_tab[i]));
		__set_bit(scancode, ir_input->keybit);
		if (modifier)
			__set_bit(modifier, ir_input->keybit);
	}
	__set_bit(EV_KEY, ir_input->evbit);

	error = input_register_device(ir_input);
	if (error) {
		printk("error: failed to register input device for IR\n");
		input_free_device(ir_input);
		return error;
	}

	register_sm_ir_handler(&ir_handler);

	return 0;
}

static int berlin_ir_remove(struct platform_device *pdev)
{
	input_unregister_device(ir_input);
	input_free_device(ir_input);
	unregister_sm_ir_handler(&ir_handler);

	return 0;
}

#ifdef CONFIG_PM
static int ir_resume(struct device *dev)
{
	int msg = MV_SM_IR_Linuxready;

	bsm_msg_send(MV_SM_ID_IR, &msg, sizeof(msg));
	bsm_msg_send(MV_SM_ID_POWER, &msg, sizeof(msg));
	return 0;
}

static struct dev_pm_ops ir_pm_ops = {
	.resume		= ir_resume,
};
#endif

static const struct of_device_id berlin_ir_of_match[] = {
	{ .compatible = "marvell,berlin-ir", },
	{},
};
MODULE_DEVICE_TABLE(of, berlin_ir_of_match);

static struct platform_driver berlin_ir_driver = {
	.probe		= berlin_ir_probe,
	.remove		= berlin_ir_remove,
	.driver	= {
		.name	= "berlin-ir",
		.owner  = THIS_MODULE,
		.of_match_table = berlin_ir_of_match,
#ifdef CONFIG_PM
		.pm	= &ir_pm_ops,
#endif
	},
};
module_platform_driver(berlin_ir_driver);

MODULE_AUTHOR("Marvell-Galois");
MODULE_DESCRIPTION("Galois Infra-Red Driver");
MODULE_LICENSE("GPL");
