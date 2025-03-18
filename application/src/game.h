#pragma once

#include <entry.h>
#include <application.h>
#include <math/kmath.h>
#include <renderer/camera.h>
#include <renderer/renderer_frontend.h>

typedef struct game_state {
    f32 delta_time;
    camera* world_camera;
} game_state;

bool game_initialize(game* inst);

bool game_update(game* inst, f32 delta_time);

bool game_render(game* inst, f32 delta_time);

void game_on_resize(game* inst, i32 width, i32 height);
