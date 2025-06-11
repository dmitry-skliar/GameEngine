#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <renderer/vulkan/vulkan_types.h>
#include <resources/resource_types.h>

bool vulkan_backend_initialize(renderer_backend* backend, const renderer_backend_config* config, u8* out_window_render_target_count);
void vulkan_backend_shutdown(renderer_backend* backend);
void vulkan_backend_on_resized(i32 width, i32 height);
bool vulkan_backend_frame_begin(f32 delta_time);
bool vulkan_backend_frame_end(f32 delta_time);
bool vulkan_backend_is_multithreaded();

void vulkan_renderpass_create(renderpass* out_renderpass, f32 depth, u32 stencil, bool has_prev_pass, bool has_next_pass);
void vulkan_renderpass_destroy(renderpass* pass);
bool vulkan_renderpass_begin(renderpass* pass, render_target* target);
bool vulkan_renderpass_end(renderpass* pass);
renderpass* vulkan_renderpass_get(const char* name);

void vulkan_texture_create(texture* t, const void* pixels);
void vulkan_texture_create_writable(texture* t);
void vulkan_texture_destroy(texture* t);
void vulkan_texture_resize(texture* t, u32 new_width, u32 new_height);
void vulkan_texture_write_data(texture* t, u32 offset, u32 size, const void* pixels);
bool vulkan_texture_map_acquire_resources(texture_map* map);
void vulkan_texture_map_release_resources(texture_map* map);

bool vulkan_geometry_create(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);
void vulkan_geometry_destroy(geometry* geometry);
void vulkan_geometry_draw(geometry_render_data* data);

bool vulkan_shader_create(struct shader* s, const shader_config* config, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages);
void vulkan_shader_destroy(struct shader* shader);
bool vulkan_shader_initialize(struct shader* shader);
bool vulkan_shader_use(struct shader* shader);
bool vulkan_shader_bind_globals(struct shader* shader);
bool vulkan_shader_bind_instance(struct shader* shader, u32 instance_id);
bool vulkan_shader_apply_globals(struct shader* shader);
bool vulkan_shader_apply_instance(struct shader* shader, bool needs_update);
bool vulkan_shader_acquire_instance_resources(struct shader* shader, texture_map** maps, u32* out_instance_id);
bool vulkan_shader_release_instance_resources(struct shader* shader, u32 instance_id);
bool vulkan_shader_set_uniform(struct shader* shader, struct shader_uniform* uniform, const void* value);

void vulkan_render_target_create(u8 attachment_count, texture** attachments, renderpass* pass, u32 width, u32 height, render_target* out_target);
void vulkan_render_target_destroy(render_target* target, bool free_internal_memory);
texture* vulkan_window_attachment_get(u8 index);
texture* vulkan_depth_attachment_get();
u8 vulkan_window_attachment_index_get();

bool vulkan_buffer_create(renderbuffer* buffer);
void vulkan_buffer_destroy(renderbuffer* buffer);
bool vulkan_buffer_resize(renderbuffer* buffer, ptr new_total_size);
bool vulkan_buffer_bind(renderbuffer* buffer, ptr offset);
bool vulkan_buffer_unbind(renderbuffer* buffer);
void* vulkan_buffer_map_memory(renderbuffer* buffer, ptr offset, ptr size);
void vulkan_buffer_unmap_memory(renderbuffer* buffer, ptr offset, ptr size);
bool vulkan_buffer_flush(renderbuffer* buffer, ptr offset, ptr size);
bool vulkan_buffer_read(renderbuffer* buffer, ptr offset, ptr size, void* out_memory);
bool vulkan_buffer_load_range(renderbuffer* buffer, ptr offset, ptr size, const void* data);
bool vulkan_buffer_copy_range(renderbuffer* src, ptr src_offset, renderbuffer* dest, ptr dest_offset, ptr size);
bool vulkan_buffer_draw(renderbuffer* buffer, ptr offset, u32 element_count, bool bind_only);
