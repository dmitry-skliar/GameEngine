#include "game.h"
#include <logger.h>
#include <event.h>

bool game_on_key(event_code code, void* sender, void* listener, event_context data)
{
    kdebug("Keyboard code %d", data.u32[0]);
    return false;
}

bool game_initialize(application* inst)
{
    // kdebug("Game init!");
    event_register(EVENT_CODE_KEYBOARD_KEY_PRESSED, null, game_on_key);
    event_register(EVENT_CODE_KEYBOARD_KEY_RELEASED, null, game_on_key);
    return true;
}

bool game_update(application* inst, f32 delta_time)
{
    // kdebug("Game update!");
    return true;
}

bool game_render(application* inst, f32 delta_time)
{
    // kdebug("Game render!");
    return true;
}

void game_on_resize(application* inst, i32 width, i32 height)
{
    kdebug("Update screen size %d:%d", width, height);
}
