#include "game.h"

#include <logger.h>

bool game_initialize(game* inst)
{
    kdebug("Game init!");
    return true;
}

bool game_update(game* inst, f32 delta_time)
{
    kdebug("Game update!");
    return true;
}

bool game_render(game* inst, f32 delta_time)
{
    kdebug("Game render!");
    return true;
}

void game_on_resize(game* inst, i32 width, i32 height)
{
    kdebug("Game on resize!");
}
