#include "game.h"
#include <logger.h>
#include <event.h>
#include <input.h>

bool game_on_key(event_code code, void* sender, void* listener, event_context data)
{
    if(code == EVENT_CODE_KEYBOARD_KEY_PRESSED)
    {
        kdebug("Keyboard key '%s' pressed.", input_get_keyboard_key_name(data.u32[0]));
    }
    else
    {
        kdebug("Keyboard key '%s' released.", input_get_keyboard_key_name(data.u32[0]));
    }

    return false;
}

bool game_on_button(event_code code, void* sender, void* listener, event_context data)
{
    if(code == EVENT_CODE_MOUSE_BUTTON_PRESSED)
    {
        kdebug("Mouse button '%s' pressed.", input_get_mouse_button_name(data.u32[0]));
    }
    else
    {
        kdebug("Mouse button '%s' released.", input_get_mouse_button_name(data.u32[0]));
    }

    return false;
}

bool game_initialize(application* inst)
{
    // kdebug("Game init!");
    event_register(EVENT_CODE_KEYBOARD_KEY_PRESSED, null, game_on_key);
    event_register(EVENT_CODE_KEYBOARD_KEY_RELEASED, null, game_on_key);
    event_register(EVENT_CODE_MOUSE_BUTTON_PRESSED, null, game_on_button);
    event_register(EVENT_CODE_MOUSE_BUTTON_RELEASED, null, game_on_button);
    return true;
}

bool game_update(application* inst, f32 delta_time)
{
    // kdebug("Game update!");
    if(input_is_keyboard_key_down(KEY_I) && input_was_keyboard_key_up(KEY_I))
    {
        kdebug("GAME_UPDATE: Get delta time is %.6f", delta_time);
    }
    return true;
}

bool game_render(application* inst, f32 delta_time)
{
    // kdebug("Game render!");
    return true;
}

void game_on_resize(application* inst, i32 width, i32 height)
{
    ktrace("Game: update screen size %d : %d", width, height);
}
