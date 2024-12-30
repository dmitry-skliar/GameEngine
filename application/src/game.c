#include "game.h"

bool game_initialize(game* inst)
{
    KDEBUG("Game init!");
    return true;
}

bool game_update(game* inst, f32 delta_time)
{
    KDEBUG("Game update!");
    return true;
}

bool game_render(game* inst, f32 delta_time)
{
    KDEBUG("Game render!");
    return true;
}

void game_on_resize(game* inst, i32 width, i32 height)
{
    KDEBUG("Game on resize!");
}
