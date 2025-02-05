#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>

bool vulkan_renderer_backend_initialize(renderer_backend* backend);

void vulkan_renderer_backend_shutdown(renderer_backend* backend);

void vulkan_renderer_backend_on_resized(renderer_backend* backend, i32 width, i32 height);

bool vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time);

void vulkan_renderer_update_global_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_color, i32 mode);

bool vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time);
