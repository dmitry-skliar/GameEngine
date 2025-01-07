#pragma once

#include <application.h>

typedef struct game_context {
    f32 delta_time;
} game_context;

bool game_initialize(application* inst);

bool game_update(application* inst, f32 delta_time);

bool game_render(application* inst, f32 delta_time);

void game_on_resize(application* inst, i32 width, i32 height);
