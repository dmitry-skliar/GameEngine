#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <resources/resource_types.h>

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

bool vulkan_renderer_create_material(material* material);

void vulkan_renderer_destroy_material(material* material);

bool vulkan_renderer_create_geometry(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);

void vulkan_renderer_destroy_geometry(geometry* geometry);

bool vulkan_renderer_shader_create(const char* name, builtin_renderpass renderpass_id, shader_stage stages, bool use_instances, bool use_local, u32* out_shader_id);

void vulkan_renderer_shader_destroy(u32 shader_id);

bool vulkan_renderer_shader_add_attribute(u32 shader_id, const char* name, shader_attribute_type type);

bool vulkan_renderer_shader_add_sampler(u32 shader_id, const char* sampler_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_i8(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_i16(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_i32(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_u8(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_u16(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_u32(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_f32(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_vec2(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_vec3(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_vec4(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_mat4(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_add_uniform_custom(u32 shader_id, const char* uniform_name, u32 size, shader_scope scope, u32* out_location);

bool vulkan_renderer_shader_initialize(u32 shader_id);

bool vulkan_renderer_shader_use(u32 shader_id);

bool vulkan_renderer_shader_bind_globals(u32 shader_id);

bool vulkan_renderer_shader_bind_instance(u32 shader_id, u32 instance_id);

bool vulkan_renderer_shader_apply_globals(u32 shader_id);

bool vulkan_renderer_shader_apply_instance(u32 shader_id);

bool vulkan_renderer_shader_acquire_instance_resource(u32 shader_id, u32* out_instance_id);

bool vulkan_renderer_shader_release_instance_resource(u32 shader_id, u32 instance_id);

u32 vulkan_renderer_shader_uniform_location(u32 shader_id, const char* uniform_name);

bool vulkan_renderer_shader_set_sampler(u32 shader_id, u32 location, texture* t);

bool vulkan_renderer_shader_set_uniform_i8(u32 shader_id, u32 location, i8 value);

bool vulkan_renderer_shader_set_uniform_i16(u32 shader_id, u32 location, i16 value);

bool vulkan_renderer_shader_set_uniform_i32(u32 shader_id, u32 location, i32 value);

bool vulkan_renderer_shader_set_uniform_u8(u32 shader_id, u32 location, u8 value);

bool vulkan_renderer_shader_set_uniform_u16(u32 shader_id, u32 location, u16 value);

bool vulkan_renderer_shader_set_uniform_u32(u32 shader_id, u32 location, u32 value);

bool vulkan_renderer_shader_set_uniform_f32(u32 shader_id, u32 location, f32 value);

bool vulkan_renderer_shader_set_uniform_vec2(u32 shader_id, u32 location, vec2 value);

bool vulkan_renderer_shader_set_uniform_vec2f(u32 shader_id, u32 location, f32 value_0, f32 value_1);

bool vulkan_renderer_shader_set_uniform_vec3(u32 shader_id, u32 location, vec3 value);

bool vulkan_renderer_shader_set_uniform_vec3f(u32 shader_id, u32 location, f32 value_0, f32 value_1, f32 value_2);

bool vulkan_renderer_shader_set_uniform_vec4(u32 shader_id, u32 location, vec4 value);

bool vulkan_renderer_shader_set_uniform_vec4f(u32 shader_id, u32 location, f32 value_0, f32 value_1, f32 value_2, f32 value_3);

bool vulkan_renderer_shader_set_uniform_mat4(u32 shader_id, u32 location, mat4 value);

bool vulkan_renderer_shader_set_uniform_custom(u32 shader_id, u32 location, void* value);
