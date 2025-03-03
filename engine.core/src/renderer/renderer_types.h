#pragma once

#include <defines.h>
#include <platform/window.h>
#include <math/math_types.h>
#include <resources/resource_types.h>

#define BUILTIN_SHADER_NAME_MATERIAL "Builtin.MaterialShader"
#define BUILTIN_SHADER_NAME_UI "Builtin.UIShader"

// TODO: Подчистить заголовочные файлы данным способом!
struct shader;
struct shader_uniform;

// @brief Режимы отображения визуализации (для отладки).
typedef enum renderer_view_mode {
    RENDERER_VIEW_MODE_DEFAULT  = 0x00,
    RENDERER_VIEW_MODE_LIGHTING = 0x01,
    RENDERER_VIEW_MODE_NORMALS  = 0x02
} renderer_view_mode;

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

    bool (*create_geometry)(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);

    void (*destroy_geometry)(geometry* geometry);

    bool (*shader_create)(struct shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages);

    void (*shader_destroy)(struct shader* shader);

    bool (*shader_initialize)(struct shader* shader);

    bool (*shader_use)(struct shader* shader);

    bool (*shader_bind_globals)(struct shader* s);

    bool (*shader_bind_instance)(struct shader* s, u32 instance_id);

    bool (*shader_apply_globals)(struct shader* s);

    bool (*shader_apply_instance)(struct shader* s);

    bool (*shader_acquire_instance_resources)(struct shader* s, u32* out_instance_id);

    bool (*shader_release_instance_resources)(struct shader* s, u32 instance_id);

    bool (*shader_set_uniform)(struct shader* frontend_shader, struct shader_uniform* uniform, const void* value);

} renderer_backend;

typedef struct render_packet {
    f32 delta_time;

    u32 geometry_count;
    geometry_render_data* geometries;

    u32 ui_geometry_count;
    geometry_render_data* ui_geometries;
} render_packet;
