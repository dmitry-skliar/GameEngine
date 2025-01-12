// Cобственные подключения.
#include "input.h"

// Внутренние подключения.
#include "logger.h"
#include "debug/assert.h"
#include "memory/memory.h"
#include "event.h"

typedef struct keyboard_state {
    bool keys[KEYS_MAX + 1];
} keyboard_state;

typedef struct mouse_state {
    i32 x;
    i32 y;
    bool buttons[BTNS_MAX];
} mouse_state;

typedef struct input_system_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_system_context;

// Указатель на состояние системы ввода.
static input_system_context* context = null;

// Сообщения.
static const char* message_context_not_initialized = "Input system was not initialized. Please first call 'input_system_initialize'.";

bool input_system_initialize()
{
    kassert_debug(context == null, "Trying to call function 'input_system_initialize' more than once!");

    context = kmallocate_t(input_system_context, MEMORY_TAG_SYSTEM);
    if(!context)
    {
        kerror("Memory for input system was not allocated.");
        return false;
    }
    kmzero_tc(context, input_system_context, 1);

    kinfor("Input system started.");
    return true;
}

void input_system_shutdown()
{
    kassert_debug(context != null, message_context_not_initialized);

    kmfree(context);
    context = null;

    kinfor("Input system stopped.");
}

void input_system_update(f64 delta_time)
{
    kassert_debug(context != null, message_context_not_initialized);

    kmcopy_tc(&context->keyboard_previous, &context->keyboard_current, keyboard_state, 1);
    kmcopy_tc(&context->mouse_previous, &context->mouse_current, mouse_state, 1);

    ktrace("Input system updated!");
}

void input_update_keyboard_key(key key, bool pressed)
{
    kassert_debug(context != null, message_context_not_initialized);

    if(key >= KEYS_MAX || key == KEY_UNKNOWN)
    {
        kwarng("Input system: unknown keyboard key code: %X.", key);
        return;
    }

    if(context->keyboard_current.keys[key] != pressed)
    {
        context->keyboard_current.keys[key] = pressed;

        event_context data = { .u32[0] = key };
        event_send(pressed ? EVENT_CODE_KEYBOARD_KEY_PRESSED : EVENT_CODE_KEYBOARD_KEY_RELEASED, null, data);
    }
}

void input_update_mouse_button(button button, bool pressed)
{
    kassert_debug(context != null, message_context_not_initialized);

    if(button >= BTNS_MAX || button == BTN_UNKNOWN)
    {
        kwarng("Input system: unknown mouse button %X code.", button);
        return;
    }

    if(context->mouse_current.buttons[button] != pressed)
    {
        context->mouse_current.buttons[button] = pressed;

        event_context data = { .u32[0] = button };
        event_send(pressed ? EVENT_CODE_MOUSE_BUTTON_PRESSED : EVENT_CODE_MOUSE_BUTTON_RELEASED, null, data);
    }
}

void input_update_mouse_move(i32 x, i32 y)
{
    kassert_debug(context != null, message_context_not_initialized);

    if(context->mouse_current.x != x || context->mouse_current.y != y)
    {
        // NOTE: Включить при отладке!
        // kdebug("Current mouse position %X : %X", x, y);

        context->mouse_current.x = x;
        context->mouse_current.y = y;

        event_context data = { .i32[0] = x, .i32[1] = y };
        event_send(EVENT_CODE_MOUSE_MOVED, null, data);
    }
}

void input_update_mouse_wheel(i32 z_delta)
{
    kassert_debug(context != null, message_context_not_initialized);

    // Нет внутреннего состояния!

    event_context data = { .i32[0] = z_delta };
    event_send(EVENT_CODE_MOUSE_WHEEL, null, data);
}

bool input_is_keyboard_key_down(key key)
{
    kassert_debug(context != null, message_context_not_initialized);
    return context->keyboard_current.keys[key] == true;
}

bool input_is_keyboard_key_up(key key)
{
    kassert_debug(context != null, message_context_not_initialized);
    return context->keyboard_current.keys[key] == false;
}

bool input_was_keyboard_key_down(key key)
{
    kassert_debug(context != null, message_context_not_initialized);
    return context->keyboard_previous.keys[key] == true;
}

bool input_was_keyboard_key_up(key key)
{
    kassert_debug(context != null, message_context_not_initialized);
    return context->keyboard_previous.keys[key] == false;
}

bool input_is_mouse_button_down(button button)
{
    kassert_debug(context != null, message_context_not_initialized);
    return context->mouse_current.buttons[button] == true;
}

bool input_is_mouse_button_up(button button)
{
    kassert_debug(context != null, message_context_not_initialized);
    return context->mouse_current.buttons[button] == false;
}

bool input_was_mouse_button_down(button button)
{
    kassert_debug(context != null, message_context_not_initialized);
    return context->mouse_previous.buttons[button] == true;
}

bool input_was_mouse_button_up(button button)
{
    kassert_debug(context != null, message_context_not_initialized);
    return context->mouse_previous.buttons[button] == false;
}

void input_get_current_mouse_position(i32* x, i32* y)
{
    kassert_debug(context != null, message_context_not_initialized);
    *x = context->mouse_current.x;
    *y = context->mouse_current.y;
}

void input_get_previous_mouse_position(i32* x, i32* y)
{
    kassert_debug(context != null, message_context_not_initialized);
    *x = context->mouse_previous.x;
    *y = context->mouse_previous.y;
}

const char* input_get_keyboard_key_name(key key)
{
    static const char* key_names[KEYS_MAX] = {
        [KEY_UNKNOWN]   = "UNKNOWN",          [KEY_BACKSPACE]   = "BACKSPACE",      [KEY_TAB]          = "TAB",
        [KEY_ENTER]     = "ENTER",            [KEY_PAUSE]       = "PAUSE",          [KEY_CAPSLOCK]     = "CAPSLOCK",
        [KEY_ESCAPE]    = "ESCAPE",           [KEY_SPACE]       = "SPACE",          [KEY_PAGEUP]       = "PAGEUP",
        [KEY_PAGEDOWN]  = "PAGEDOWN",         [KEY_END]         = "END",            [KEY_HOME]         = "HOME",
        [KEY_LEFT]      = "ARROW LEFT",       [KEY_UP]          = "ARROW UP",       [KEY_RIGHT]        = "ARROW RIGHT",
        [KEY_DOWN]      = "ARROW DOWN",       [KEY_SELECT]      = "SELECT",         [KEY_PRINT]        = "PRINT",
        [KEY_EXECUTE]   = "EXECUTE",          [KEY_PRINTSCREEN] = "PRINTSCREEN",    [KEY_INSERT]       = "INSERT",
        [KEY_DELETE]    = "DELETE",           [KEY_HELP]        = "HELP",           [KEY_0]            = "0",
        [KEY_1]         = "1",                [KEY_2]           = "2",              [KEY_3]            = "3",
        [KEY_4]         = "4",                [KEY_5]           = "5",              [KEY_6]            = "6",
        [KEY_7]         = "7",                [KEY_8]           = "8",              [KEY_9]            = "9",
        [KEY_A]         = "A",                [KEY_B]           = "B",              [KEY_C]            = "C",
        [KEY_D]         = "D",                [KEY_E]           = "E",              [KEY_F]            = "F",
        [KEY_G]         = "G",                [KEY_H]           = "H",              [KEY_I]            = "I",
        [KEY_J]         = "J",                [KEY_K]           = "K",              [KEY_L]            = "L",
        [KEY_M]         = "M",                [KEY_N]           = "N",              [KEY_O]            = "O",
        [KEY_P]         = "P",                [KEY_Q]           = "Q",              [KEY_R]            = "R",
        [KEY_S]         = "S",                [KEY_T]           = "T",              [KEY_U]            = "U",
        [KEY_V]         = "V",                [KEY_W]           = "W",              [KEY_X]            = "X",
        [KEY_Y]         = "Y",                [KEY_Z]           = "Z",              [KEY_LSUPER]       = "LSUPER",
        [KEY_RSUPER]    = "RSUPER",           [KEY_APPS]        = "APPS",           [KEY_SLEEP]        = "SLEEP",
        [KEY_NUMPAD0]   = "NUMPAD-0",         [KEY_NUMPAD1]     = "NUMPAD-1",       [KEY_NUMPAD2]      = "NUMPAD-2",
        [KEY_NUMPAD3]   = "NUMPAD-3",         [KEY_NUMPAD4]     = "NUMPAD-4",       [KEY_NUMPAD5]      = "NUMPAD-5",
        [KEY_NUMPAD6]   = "NUMPAD-6",         [KEY_NUMPAD7]     = "NUMPAD-7",       [KEY_NUMPAD8]      = "NUMPAD-8",
        [KEY_NUMPAD9]   = "NUMPAD-9",         [KEY_MULTIPLY]    = "NUMPAD-MUL",     [KEY_ADD]          = "NUMPAD-ADD",
        [KEY_SEPARATOR] = "NUMPAD-SEP",       [KEY_SUBTRACT]    = "NUMPAD-SUB",     [KEY_DIVIDE]       = "NUMPAD-DIV",
        [KEY_F1]        = "F1",               [KEY_F2]          = "F2",             [KEY_F3]           = "F3",
        [KEY_F4]        = "F4",               [KEY_F5]          = "F5",             [KEY_F6]           = "F6",
        [KEY_F7]        = "F7",               [KEY_F8]          = "F8",             [KEY_F9]           = "F9",
        [KEY_F10]       = "F10",              [KEY_F11]         = "F11",            [KEY_F12]          = "F12",
        [KEY_F13]       = "F13",              [KEY_F14]         = "F14",            [KEY_F15]          = "F15",
        [KEY_F16]       = "F16",              [KEY_F17]         = "F17",            [KEY_F18]          = "F18",
        [KEY_F19]       = "F19",              [KEY_F20]         = "F20",            [KEY_F21]          = "F21",
        [KEY_F22]       = "F22",              [KEY_F23]         = "F23",            [KEY_F24]          = "F24",
        [KEY_NUMLOCK]   = "NUMLOCK",          [KEY_SCROLLOCK]   = "SCROLLOCK",      [KEY_NUMPAD_EQUAL] = "NUMPAD-EQUAL",
        [KEY_LSHIFT]    = "LSHIFT",           [KEY_RSHIFT]      = "RSHIFT",         [KEY_LCONTROL]     = "LCONTROL",
        [KEY_RCONTROL]  = "RCONTROL",         [KEY_LALT]        = "LALT",           [KEY_RALT]         = "RALT",
        [KEY_SEMICOLON] = "SEMICOLON",        [KEY_APOSTROPHE]  = "APOSTROPHE",     [KEY_EQUAL]        = "EQUAL/PLUS",
        [KEY_COMMA]     = "COMMA",            [KEY_MINUS]       = "MINUS",          [KEY_DOT]          = "DOT",
        [KEY_SLASH]     = "SLASH",            [KEY_GRAVE]       = "GRAVE",          [KEY_LBRACKET]     = "LBRACKET",
        [KEY_BACKSLASH] = "PIPE/BACKSLASH",   [KEY_RBRACKET]    = "RBRACKET" 
    };

    if(key >= KEYS_MAX)
    {
        return key_names[KEY_UNKNOWN];
    }

    return key_names[key];
}

const char* input_get_mouse_button_name(button button)
{
    static const char* button_names[] = {
        [BTN_UNKNOWN]   = "UNKNOWN",          [BTN_LEFT]        = "BTN_LEFT",       [BTN_RIGHT]        = "BTN_RIGHT",
        [BTN_MIDDLE]    = "BTN_MIDDLE",       [BTN_BACKWARD]    = "BTN_BACKWARD",   [BTN_FORWARD]      = "BTN_FORWARD"
    };

    if(button >= BTNS_MAX)
    {
        return button_names[BTN_UNKNOWN];
    }

    return button_names[button];
}
