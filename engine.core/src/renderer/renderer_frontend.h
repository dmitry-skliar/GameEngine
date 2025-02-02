#pragma once

#include <renderer/renderer_types.h>
#include <platform/window.h>

bool renderer_initialize(window* window_state);

void renderer_shutdown();

void renderer_on_resize(i32 width, i32 height);

bool renderer_draw_frame(render_packet* packet);
