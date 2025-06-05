#pragma once

#include <defines.h>
#include <math/math_types.h>
#include <renderer/renderer_types.h>

typedef struct render_view_system_config {
    u16 max_view_count;
} render_view_system_config;

bool render_view_system_initialize(u64* memory_requirement, void* memory, render_view_system_config* config);

void render_view_system_shutdown();

bool render_view_system_create(render_view_config* config);

void render_view_system_on_window_resize(u32 width, u32 height);

render_view* render_view_system_get(const char* name);

bool render_view_system_build_packet(const render_view* view, void* data, render_view_packet* out_packet);

bool render_view_system_on_render(const render_view* view, render_view_packet* packet, u64 frame_number, u64 render_target_index);
