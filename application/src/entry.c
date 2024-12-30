#include <entry.h>
#include "game.h"

bool create_game(struct game* inst)
{
    inst->window_title  = "Game Application";
    inst->window_width  = 1024;
    inst->window_height = 768;

    inst->initialize = game_initialize;
    inst->update     = game_update;
    inst->render     = game_render;
    inst->on_resize  = game_on_resize;

    // TODO: Сделать динамически выделяемым.
    game_context context;
    inst->state = &context;

    return true;
}
