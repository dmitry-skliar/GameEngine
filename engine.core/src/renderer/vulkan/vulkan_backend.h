#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <renderer/vulkan/vulkan_types.h>
#include <resources/resource_types.h>

bool vulkan_renderer_backend_initialize(renderer_backend* backend, const renderer_backend_config* config, u8* out_window_render_target_count);

void vulkan_renderer_backend_shutdown(renderer_backend* backend);

void vulkan_renderer_backend_on_resized(i32 width, i32 height);

bool vulkan_renderer_backend_frame_begin(f32 delta_time);

bool vulkan_renderer_backend_frame_end(f32 delta_time);

void vulkan_renderer_renderpass_create(renderpass* out_renderpass, f32 depth, u32 stencil, bool has_prev_pass, bool has_next_pass);

void vulkan_renderer_renderpass_destroy(renderpass* pass);

bool vulkan_renderer_renderpass_begin(renderpass* pass, render_target* target);

bool vulkan_renderer_renderpass_end(renderpass* pass);

renderpass* vulkan_renderer_renderpass_get(const char* name);

void vulkan_renderer_texture_create(texture* t, const void* pixels);

void vulkan_renderer_texture_create_writable(texture* t);

void vulkan_renderer_texture_destroy(texture* t);

void vulkan_renderer_texture_resize(texture* t, u32 new_width, u32 new_height);

void vulkan_renderer_texture_write_data(texture* t, u32 offset, u32 size, const void* pixels);

bool vulkan_renderer_texture_map_acquire_resources(texture_map* map);

void vulkan_renderer_texture_map_release_resources(texture_map* map);

bool vulkan_renderer_geometry_create(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);

void vulkan_renderer_geometry_destroy(geometry* geometry);

void vulkan_renderer_geometry_draw(geometry_render_data* data);

bool vulkan_renderer_shader_create(struct shader* shader, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages);

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

void vulkan_renderer_render_target_create(u8 attachment_count, texture** attachments, renderpass* pass, u32 width, u32 height, render_target* out_target);

void vulkan_renderer_render_target_destroy(render_target* target, bool free_internal_memory);

texture* vulkan_renderer_window_attachment_get(u8 index);

texture* vulkan_renderer_depth_attachment_get();

u8 vulkan_renderer_window_attachment_index_get();
