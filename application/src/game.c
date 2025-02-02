#include "game.h"

#include <input.h>
#include <memory/memory.h>

bool game_create(game* inst)
{
    inst->window_title  = "Game Application";
    inst->window_width  = 1024;
    inst->window_height = 768;

    inst->initialize = game_initialize;
    inst->update     = game_update;
    inst->render     = game_render;
    inst->on_resize  = game_on_resize;

    // inst->state = kallocate_tc(game_state, 1, MEMORY_TAG_GAME);

    return true;
}

void game_destroy(game* inst)
{
    // kfree_tc(inst->state, game_state, 1, MEMORY_TAG_GAME);
}

bool game_initialize(game* inst)
{
    return true;
}

bool game_update(game* inst, f32 delta_time)
{
    static u64 alloc_count = 0;
    u64 prev_alloc_count = alloc_count;
    alloc_count = memory_system_alloc_count();

    if(input_keyboard_key_pressed('M'))
    {
        kdebug("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
    }
    
    return true;
}

bool game_render(game* inst, f32 delta_time)
{
    return true;
}

void game_on_resize(game* inst, i32 width, i32 height)
{
    kdebug("Game resized (w/h): %d / %d", width, height);

}
