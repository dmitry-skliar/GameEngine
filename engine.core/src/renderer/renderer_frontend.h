#pragma once

#include <renderer/renderer_types.h>

bool renderer_initialize(const char* application_name, u32 width, u32 height);

void renderer_shutdown();

void renderer_on_resize(i32 width, i32 height);

bool renderer_draw_frame(render_packet* packet);
