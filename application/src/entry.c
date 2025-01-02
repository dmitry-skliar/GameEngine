#include "game.h"

#include <entry.h>

bool create_game(struct game* inst)
{
    inst->window_title  = "Game Application";
    inst->window_width  = 1024;
    inst->window_height = 768;

    inst->initialize = game_initialize;
    inst->update     = game_update;
    inst->render     = game_render;
    inst->on_resize  = game_on_resize;

    inst->state = kmallocate_t(game_context, MEMORY_TAG_GAME);

    return true;
}
