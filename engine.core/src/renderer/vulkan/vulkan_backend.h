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

void vulkan_renderer_texture_create(texture* t, const void* pixels);

void vulkan_renderer_texture_create_writable(texture* t);

void vulkan_renderer_texture_resize(texture* t, u32 new_width, u32 new_height);

void vulkan_renderer_texture_write_data(texture* t, u32 offset, u32 size, const void* pixels);

void vulkan_renderer_texture_destroy(texture* t);

bool vulkan_renderer_texture_map_acquire_resources(texture_map* map);

void vulkan_renderer_texture_map_release_resources(texture_map* map);

bool vulkan_renderer_geometry_create(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);

void vulkan_renderer_geometry_destroy(geometry* geometry);

void vulkan_renderer_geometry_draw(geometry_render_data data);

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
