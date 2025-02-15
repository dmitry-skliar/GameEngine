#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <resources/resource_types.h>

bool vulkan_renderer_backend_initialize(renderer_backend* backend);

void vulkan_renderer_backend_shutdown(renderer_backend* backend);

void vulkan_renderer_backend_on_resized(renderer_backend* backend, i32 width, i32 height);

bool vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time);

void vulkan_renderer_update_global_world_state(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_color, i32 mode);

void vulkan_renderer_update_global_ui_state(mat4 projection, mat4 view, i32 mode);

bool vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time);

bool vulkan_renderer_backend_begin_renderpass(renderer_backend* backend, builtin_renderpass renderpass_id);

bool vulkan_renderer_backend_end_renderpass(renderer_backend* backend, builtin_renderpass renderpass_id);

void vulkan_renderer_backend_draw_geometry(geometry_render_data data);

void vulkan_renderer_backend_create_texture(texture* texture, const void* pixels);

void vulkan_renderer_backend_destroy_texture(texture* texture);

bool vulkan_renderer_backend_create_material(material* material);

void vulkan_renderer_backend_destroy_material(material* material);

bool vulkan_renderer_backend_create_geometry(
    geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count,
    const void* indices
);

void vulkan_renderer_backend_destroy_geometry(geometry* geometry);
