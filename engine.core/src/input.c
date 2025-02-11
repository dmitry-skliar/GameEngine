// Cобственные подключения.
#include "input.h"

// Внутренние подключения.
#include "logger.h"
#include "event.h"
#include "memory/memory.h"

typedef struct keyboard_state {
    bool keys[KEYS_MAX + 1];
} keyboard_state;

typedef struct mouse_state {
    i32 x;
    i32 y;
    i32 z_delta;
    bool buttons[BTNS_MAX];
} mouse_state;

typedef struct input_system_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_system_state;

static input_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the input system to be initialized. Call 'input_system_initialize' first.";

void input_system_initialize(u64* memory_requirement, void* memory)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once!", __FUNCTION__);
        return;
    }

    *memory_requirement = sizeof(struct input_system_state);

    if(!memory)
    {
        return;
    }

    kzero(memory, *memory_requirement);
    state_ptr = memory;
}

void input_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    state_ptr = null;
}

void input_system_update(f64 delta_time)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    kcopy_tc(&state_ptr->keyboard_previous, &state_ptr->keyboard_current, keyboard_state, 1);
    kcopy_tc(&state_ptr->mouse_previous, &state_ptr->mouse_current, mouse_state, 1);
}

void input_update_keyboard_key(key key, bool pressed)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    if(key >= KEYS_MAX || key == KEY_UNKNOWN)
    {
        kwarng("Input system: unknown keyboard key code: %X.", key);
        return;
    }

    if(state_ptr->keyboard_current.keys[key] != pressed)
    {
        state_ptr->keyboard_current.keys[key] = pressed;

        event_context context = { .u32[0] = key };
        event_send(pressed ? EVENT_CODE_KEYBOARD_KEY_PRESSED : EVENT_CODE_KEYBOARD_KEY_RELEASED, null, &context);
    }
}

void input_update_mouse_button(button button, bool pressed)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    if(button >= BTNS_MAX || button == BTN_UNKNOWN)
    {
        kwarng("Input system: unknown mouse button %X code.", button);
        return;
    }

    if(state_ptr->mouse_current.buttons[button] != pressed)
    {
        state_ptr->mouse_current.buttons[button] = pressed;

        event_context context = { .u32[0] = button };
        event_send(pressed ? EVENT_CODE_MOUSE_BUTTON_PRESSED : EVENT_CODE_MOUSE_BUTTON_RELEASED, null, &context);
    }
}

void input_update_mouse_move(i32 x, i32 y)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    if(state_ptr->mouse_current.x != x || state_ptr->mouse_current.y != y)
    {
        // NOTE: Включить при отладке!
        // kdebug("Current mouse position %X : %X", x, y);

        state_ptr->mouse_current.x = x;
        state_ptr->mouse_current.y = y;

        event_context context = { .i32[0] = x, .i32[1] = y };
        event_send(EVENT_CODE_MOUSE_MOVED, null, &context);
    }
}

void input_update_mouse_wheel(i32 z_delta)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    if(state_ptr->mouse_current.z_delta != z_delta)
    {
        state_ptr->mouse_current.z_delta = z_delta;
    }

    event_context context = { .i32[0] = z_delta };
    event_send(EVENT_CODE_MOUSE_WHEEL, null, &context);
}

bool input_is_keyboard_key_down(key key)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }
    return state_ptr->keyboard_current.keys[key] == true;
}

bool input_is_keyboard_key_up(key key)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return true;
    }
    return state_ptr->keyboard_current.keys[key] == false;
}

bool input_was_keyboard_key_down(key key)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }
    return state_ptr->keyboard_previous.keys[key] == true;
}

bool input_was_keyboard_key_up(key key)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return true;
    }
    return state_ptr->keyboard_previous.keys[key] == false;
}

bool input_is_mouse_button_down(button button)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }
    return state_ptr->mouse_current.buttons[button] == true;
}

bool input_is_mouse_button_up(button button)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return true;
    }
    return state_ptr->mouse_current.buttons[button] == false;
}

bool input_was_mouse_button_down(button button)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }
    return state_ptr->mouse_previous.buttons[button] == true;
}

bool input_was_mouse_button_up(button button)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return true;
    }
    return state_ptr->mouse_previous.buttons[button] == false;
}

void input_current_mouse_position(i32* x, i32* y)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    *x = state_ptr->mouse_current.x;
    *y = state_ptr->mouse_current.y;
}

void input_previous_mouse_position(i32* x, i32* y)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    *x = state_ptr->mouse_previous.x;
    *y = state_ptr->mouse_previous.y;
}

void input_current_mouse_wheel(i32* z_delta)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    *z_delta = state_ptr->mouse_current.z_delta;
}

void input_previous_mouse_wheel(i32* z_delta)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    *z_delta = state_ptr->mouse_previous.z_delta;
}


const char* input_keyboard_key_str(key key)
{
    static const char* names[KEYS_MAX] = {
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
        return names[KEY_UNKNOWN];
    }

    return names[key];
}

const char* input_mouse_button_str(button button)
{
    static const char* names[] = {
        [BTN_UNKNOWN]   = "UNKNOWN",          [BTN_LEFT]        = "BTN_LEFT",       [BTN_RIGHT]        = "BTN_RIGHT",
        [BTN_MIDDLE]    = "BTN_MIDDLE",       [BTN_BACKWARD]    = "BTN_BACKWARD",   [BTN_FORWARD]      = "BTN_FORWARD"
    };

    if(button >= BTNS_MAX)
    {
        return names[BTN_UNKNOWN];
    }

    return names[button];
}
