#pragma once

#include <defines.h>

// @brief Код кнопки мышки.
typedef enum button {
    BTN_UNKNOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_MIDDLE,
    BTN_FORWARD,
    BTN_BACKWARD,
    BTNS_MAX
} button;

// @brief Код клавиши клавиатуры.
//        Смотреть https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
typedef enum key {
    KEY_UNKNOWN      = 0x00,    // The key is unknown.
    KEY_BACKSPACE    = 0x08,    // The backspace key.
    KEY_TAB          = 0x09,    // The tab key.
    KEY_ENTER        = 0x0D,    // The enter key.
    KEY_SHIFT        = 0x10,    // The shift key (deprecate).
    KEY_CONTROL      = 0x11,    // The control key (deprecate).
    KEY_ALT          = 0x12,    // The alt key (deprecate),
    KEY_PAUSE        = 0x13,    // The pause key.
    KEY_CAPSLOCK     = 0x14,    // The caps lock key.
    KEY_ESCAPE       = 0x1B,    // The escape key.
    KEY_SPACE        = 0x20,    // The space bar key.
    KEY_PAGEUP       = 0x21,    // The page up key.
    KEY_PAGEDOWN     = 0x22,    // The page down key.
    KEY_END          = 0x23,    // The end key.
    KEY_HOME         = 0x24,    // The home key.
    KEY_LEFT         = 0x25,    // The left arrow key.
    KEY_UP           = 0x26,    // The up arrow key.
    KEY_RIGHT        = 0x27,    // The right arrow key.
    KEY_DOWN         = 0x28,    // The down arrow key.
    KEY_SELECT       = 0x29,
    KEY_PRINT        = 0x2A,
    KEY_EXECUTE      = 0x2B,
    KEY_PRINTSCREEN  = 0x2C,    // The print screen key.
    KEY_INSERT       = 0x2D,    // The insert ley.
    KEY_DELETE       = 0x2E,    // The delete key.
    KEY_HELP         = 0x2F,

    KEY_0            = 0x30,    // The '0' key.
    KEY_1            = 0x31,    // The '1' key.
    KEY_2            = 0x32,    // The '2' key.
    KEY_3            = 0x33,    // The '3' key.
    KEY_4            = 0x34,    // The '4' key.
    KEY_5            = 0x35,    // The '5' key.
    KEY_6            = 0x36,    // The '6' key.
    KEY_7            = 0x37,    // The '7' key.
    KEY_8            = 0x38,    // The '8' key.
    KEY_9            = 0x39,    // The '9' key.

    KEY_A            = 0x41,    // The A key.
    KEY_B            = 0x42,    // The B key.
    KEY_C            = 0x43,    // The C key.
    KEY_D            = 0x44,    // The D key.
    KEY_E            = 0x45,    // The E key.
    KEY_F            = 0x46,    // The F key.
    KEY_G            = 0x47,    // The G key.
    KEY_H            = 0x48,    // The H key.
    KEY_I            = 0x49,    // The I key.
    KEY_J            = 0x4A,    // The J key.
    KEY_K            = 0x4B,    // The K key.
    KEY_L            = 0x4C,    // The L key.
    KEY_M            = 0x4D,    // The M key.
    KEY_N            = 0x4E,    // The N key.
    KEY_O            = 0x4F,    // The O key.
    KEY_P            = 0x50,    // The P key.
    KEY_Q            = 0x51,    // The Q key.
    KEY_R            = 0x52,    // The R key.
    KEY_S            = 0x53,    // The S key.
    KEY_T            = 0x54,    // The T key.
    KEY_U            = 0x55,    // The U key.
    KEY_V            = 0x56,    // The V key.
    KEY_W            = 0x57,    // The W key.
    KEY_X            = 0x58,    // The X key.
    KEY_Y            = 0x59,    // The Y key.
    KEY_Z            = 0x5A,    // The Z key.

    KEY_LSUPER       = 0x5B,    // The left windows/super key.
    KEY_RSUPER       = 0x5C,    // The right windows/super key.
    KEY_APPS         = 0x5D,    // The applications key.

    KEY_SLEEP        = 0x5F,    // The sleep key.

    KEY_NUMPAD0      = 0x60,    // The numberpad '0' key.
    KEY_NUMPAD1      = 0x61,    // The numberpad '1' key.
    KEY_NUMPAD2      = 0x62,    // The numberpad '2' key.
    KEY_NUMPAD3      = 0x63,    // The numberpad '3' key.
    KEY_NUMPAD4      = 0x64,    // The numberpad '4' key.
    KEY_NUMPAD5      = 0x65,    // The numberpad '5' key.
    KEY_NUMPAD6      = 0x66,    // The numberpad '6' key.
    KEY_NUMPAD7      = 0x67,    // The numberpad '7' key.
    KEY_NUMPAD8      = 0x68,    // The numberpad '8' key.
    KEY_NUMPAD9      = 0x69,    // The numberpad '9' key.
    KEY_MULTIPLY     = 0x6A,    // The numberpad multiply key.
    KEY_ADD          = 0x6B,    // The numberpad add key.
    KEY_SEPARATOR    = 0x6C,    // The numberpad separator key.
    KEY_SUBTRACT     = 0x6D,    // The numberpad subtract key.
    KEY_DECIMAL      = 0x6E,    // The numberpad decimal key.
    KEY_DIVIDE       = 0x6F,    // The numberpad divide key.

    KEY_F1           = 0x70,    // The F1  key.
    KEY_F2           = 0x71,    // The F2  key.
    KEY_F3           = 0x72,    // The F3  key.
    KEY_F4           = 0x73,    // The F4  key.
    KEY_F5           = 0x74,    // The F5  key.
    KEY_F6           = 0x75,    // The F6  key.
    KEY_F7           = 0x76,    // The F7  key.
    KEY_F8           = 0x77,    // The F8  key.
    KEY_F9           = 0x78,    // The F9  key.
    KEY_F10          = 0x79,    // The F10 key.
    KEY_F11          = 0x7A,    // The F11 key.
    KEY_F12          = 0x7B,    // The F12 key.
    KEY_F13          = 0x7C,    // The F13 key.
    KEY_F14          = 0x7D,    // The F14 key.
    KEY_F15          = 0x7E,    // The F15 key.
    KEY_F16          = 0x7F,    // The F16 key.
    KEY_F17          = 0x80,    // The F17 key.
    KEY_F18          = 0x81,    // The F18 key.
    KEY_F19          = 0x82,    // The F19 key.
    KEY_F20          = 0x83,    // The F20 key.
    KEY_F21          = 0x84,    // The F21 key.
    KEY_F22          = 0x85,    // The F22 key.
    KEY_F23          = 0x86,    // The F23 key.
    KEY_F24          = 0x87,    // The F24 key.

    KEY_NUMLOCK      = 0x90,    // The number lock key.
    KEY_SCROLLOCK    = 0x91,    // The scroll lock key.
    KEY_NUMPAD_EQUAL = 0x92,    // The numberpad equal key.

    KEY_LSHIFT       = 0xA0,    // The left shift key.
    KEY_RSHIFT       = 0xA1,    // The right shift key.
    KEY_LCONTROL     = 0xA2,    // The left control key.
    KEY_RCONTROL     = 0xA3,    // The right control key.
    KEY_LALT         = 0xA4,    // The left alt key.
    KEY_RALT         = 0xA5,    // The right alt key.

    KEY_SEMICOLON    = 0x3B,    // The semicolon key.

    KEY_APOSTROPHE   = 0xDE,    // The apostroph/single-quote key.
    KEY_QUOTE        = KEY_APPS,
    KEY_EQUAL        = 0xBB,    // The equal/plus key.
    KEY_COMMA        = 0xBC,    // The comma key.
    KEY_MINUS        = 0xBD,    // The minus key.
    KEY_DOT          = 0xBE,    // The dot(period) key.
    KEY_SLASH        = 0xBF,    // The slash key.

    KEY_GRAVE        = 0xC0,    // The grave key.

    KEY_LBRACKET     = 0xDB,    // The left (square) bracket key.
    KEY_PIPE         = 0xDC,    // The pipe/backslash key.
    KEY_BACKSLASH    = KEY_PIPE,
    KEY_RBRACKET     = 0xDD,    // The right (square) bracket key.

    KEYS_MAX         = 0xFF
} key;
