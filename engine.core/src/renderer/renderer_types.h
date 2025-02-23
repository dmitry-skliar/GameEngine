#pragma once

#include <defines.h>
#include <platform/window.h>
#include <math/math_types.h>
#include <resources/resource_types.h>

// @brief Тип рендера.
typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX
} renderer_backend_type;

typedef struct geometry_render_data {
    mat4 model;
    geometry* geometry;
} geometry_render_data;

typedef enum builtin_renderpass {
    BUILTIN_RENDERPASS_WORLD = 0x01,
    BUILTIN_RENDERPASS_UI    = 0x02
} builtin_renderpass;

// @brief Стадия шейдера на конвейере (можно комбинировать).
typedef enum shader_stage {
    SHADER_STAGE_VERTEX   = 0x00000001,
    SHADER_STAGE_GEOMETRY = 0x00000002,
    SHADER_STAGE_FRAGMENT = 0x00000004,
    SHADER_STAGE_COMPUTE  = 0x00000008,
} shader_stage;

// @brief Тип атрибута шейдера.
typedef enum shader_attribute_type {
    SHADER_ATTRIB_TYPE_FLOAT32,
    SHADER_ATTRIB_TYPE_FLOAT32_2,
    SHADER_ATTRIB_TYPE_FLOAT32_3,
    SHADER_ATTRIB_TYPE_FLOAT32_4,
    SHADER_ATTRIB_TYPE_MATRIX_4,
    SHADER_ATTRIB_TYPE_INT8,
    SHADER_ATTRIB_TYPE_INT8_2,
    SHADER_ATTRIB_TYPE_INT8_3,
    SHADER_ATTRIB_TYPE_INT8_4,
    SHADER_ATTRIB_TYPE_UINT8,
    SHADER_ATTRIB_TYPE_UINT8_2,
    SHADER_ATTRIB_TYPE_UINT8_3,
    SHADER_ATTRIB_TYPE_UINT8_4,
    SHADER_ATTRIB_TYPE_INT16,
    SHADER_ATTRIB_TYPE_INT16_2,
    SHADER_ATTRIB_TYPE_INT16_3,
    SHADER_ATTRIB_TYPE_INT16_4,
    SHADER_ATTRIB_TYPE_UINT16,
    SHADER_ATTRIB_TYPE_UINT16_2,
    SHADER_ATTRIB_TYPE_UINT16_3,
    SHADER_ATTRIB_TYPE_UINT16_4,
    SHADER_ATTRIB_TYPE_INT32,
    SHADER_ATTRIB_TYPE_INT32_2,
    SHADER_ATTRIB_TYPE_INT32_3,
    SHADER_ATTRIB_TYPE_INT32_4,
    SHADER_ATTRIB_TYPE_UINT32,
    SHADER_ATTRIB_TYPE_UINT32_2,
    SHADER_ATTRIB_TYPE_UINT32_3,
    SHADER_ATTRIB_TYPE_UINT32_4,
} shader_attribute_type;

// @brief Область действия шейдера.
typedef enum shader_scope {
    // @brief Глобальная облась действия (обновление каждый кадр).
    SHADER_SCOPE_GLOBAL   = 0,
    // @brief Область действия экземпляра (обновление экземпляра).
    SHADER_SCOPE_INSTANCE = 1,
    // @brief Локальная область действия (обновление объекта).
    SHADER_SCOPE_LOCAL    = 2
} shader_scope;

typedef struct renderer_backend {

    u64 frame_number;

    window* window_state;

    bool (*initialize)(struct renderer_backend* backend);

    void (*shutdown)(struct renderer_backend* backend);

    void (*resized)(struct renderer_backend* backend, i32 width, i32 height);

    bool (*begin_frame)(struct renderer_backend* backend, f32 delta_time);

    bool (*end_frame)(struct renderer_backend* backend, f32 delta_time);

    bool (*begin_renderpass)(struct renderer_backend* backend, builtin_renderpass renderpass_id);

    bool (*end_renderpass)(struct renderer_backend* backend, builtin_renderpass renderpass_id);

    void (*draw_geometry)(geometry_render_data data);

    void (*create_texture)(texture* texture, const void* pixels);

    void (*destroy_texture)(texture* texture);

    bool (*create_material)(material* material);

    void (*destroy_material)(material* material);

    bool (*create_geometry)(
        geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices
    );

    void (*destroy_geometry)(geometry* geometry);

    bool (*shader_create)(const char* name, builtin_renderpass renderpass_id, shader_stage stages, bool use_instances, bool use_local, u32* out_shader_id);

    void (*shader_destroy)(u32 shader_id);

    bool (*shader_add_attribute)(u32 shader_id, const char* name, shader_attribute_type type);

    bool (*shader_add_sampler)(u32 shader_id, const char* sampler_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_i8)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_i16)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_i32)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_u8)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_u16)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_u32)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_f32)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_vec2)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_vec3)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_vec4)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_mat4)(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location);

    bool (*shader_add_uniform_custom)(u32 shader_id, const char* uniform_name, u32 size, shader_scope scope, u32* out_location);

    bool (*shader_initialize)(u32 shader_id);

    bool (*shader_use)(u32 shader_id);

    bool (*shader_bind_globals)(u32 shader_id);

    bool (*shader_bind_instance)(u32 shader_id, u32 instance_id);

    bool (*shader_apply_globals)(u32 shader_id);

    bool (*shader_apply_instance)(u32 shader_id);

    bool (*shader_acquire_instance_resource)(u32 shader_id, u32* out_instance_id);

    bool (*shader_release_instance_resource)(u32 shader_id, u32 instance_id);

    u32 (*shader_uniform_location)(u32 shader_id, const char* uniform_name);

    bool (*shader_set_sampler)(u32 shader_id, u32 location, texture* t);

    bool (*shader_set_uniform_i8)(u32 shader_id, u32 location, i8 value);

    bool (*shader_set_uniform_i16)(u32 shader_id, u32 location, i16 value);

    bool (*shader_set_uniform_i32)(u32 shader_id, u32 location, i32 value);

    bool (*shader_set_uniform_u8)(u32 shader_id, u32 location, u8 value);

    bool (*shader_set_uniform_u16)(u32 shader_id, u32 location, u16 value);

    bool (*shader_set_uniform_u32)(u32 shader_id, u32 location, u32 value);

    bool (*shader_set_uniform_f32)(u32 shader_id, u32 location, f32 value);

    bool (*shader_set_uniform_vec2)(u32 shader_id, u32 location, vec2 value);

    bool (*shader_set_uniform_vec2f)(u32 shader_id, u32 location, f32 value_0, f32 value_1);

    bool (*shader_set_uniform_vec3)(u32 shader_id, u32 location, vec3 value);

    bool (*shader_set_uniform_vec3f)(u32 shader_id, u32 location, f32 value_0, f32 value_1, f32 value_2);

    bool (*shader_set_uniform_vec4)(u32 shader_id, u32 location, vec4 value);

    bool (*shader_set_uniform_vec4f)(u32 shader_id, u32 location, f32 value_0, f32 value_1, f32 value_2, f32 value_3);

    bool (*shader_set_uniform_mat4)(u32 shader_id, u32 location, mat4 value);

    bool (*shader_set_uniform_custom)(u32 shader_id, u32 location, void* value);

} renderer_backend;

typedef struct render_packet {
    f32 delta_time;

    u32 geometry_count;
    geometry_render_data* geometries;

    u32 ui_geometry_count;
    geometry_render_data* ui_geometries;
} render_packet;
