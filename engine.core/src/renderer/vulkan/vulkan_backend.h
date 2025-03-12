#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <resources/resource_types.h>

/*
    @brief Инициализация бэкэнда Vulkan.
    @param Указатель на общий интерфейс бэкенда.
    @return True операция прошла успещно, false если есть ошибки.
*/
bool vulkan_renderer_backend_initialize(renderer_backend* backend);

void vulkan_renderer_backend_shutdown(renderer_backend* backend);

void vulkan_renderer_backend_on_resized(renderer_backend* backend, i32 width, i32 height);

bool vulkan_renderer_backend_begin_frame(renderer_backend* backend, f32 delta_time);

bool vulkan_renderer_backend_end_frame(renderer_backend* backend, f32 delta_time);

bool vulkan_renderer_begin_renderpass(renderer_backend* backend, builtin_renderpass renderpass_id);

bool vulkan_renderer_end_renderpass(renderer_backend* backend, builtin_renderpass renderpass_id);

void vulkan_renderer_draw_geometry(geometry_render_data data);

void vulkan_renderer_create_texture(texture* texture, const void* pixels);

void vulkan_renderer_destroy_texture(texture* texture);

bool vulkan_renderer_create_geometry(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);

void vulkan_renderer_destroy_geometry(geometry* geometry);

bool vulkan_renderer_shader_create(struct shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages);

void vulkan_renderer_shader_destroy(struct shader* shader);

bool vulkan_renderer_shader_initialize(struct shader* shader);

bool vulkan_renderer_shader_use(struct shader* shader);

bool vulkan_renderer_shader_bind_globals(struct shader* shader);

bool vulkan_renderer_shader_bind_instance(struct shader* shader, u32 instance_id);

bool vulkan_renderer_shader_apply_globals(struct shader* shader);

bool vulkan_renderer_shader_apply_instance(struct shader* shader, bool needs_update);

bool vulkan_renderer_shader_acquire_instance_resources(struct shader* shader, texture_map** maps, u32* out_instance_id);

bool vulkan_renderer_shader_release_instance_resources(struct shader* shader, u32 instance_id);

bool vulkan_renderer_shader_set_uniform(struct shader* shader, struct shader_uniform* uniform, const void* value);

bool vulkan_renderer_texture_map_acquire_resources(texture_map* map);

void vulkan_renderer_texture_map_release_resources(texture_map* map);
