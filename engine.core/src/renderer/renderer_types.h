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

typedef struct renderer_backend {

    u64 frame_number;

    window* window_state;

    bool (*initialize)(struct renderer_backend* backend);

    void (*shutdown)(struct renderer_backend* backend);

    void (*resized)(struct renderer_backend* backend, i32 width, i32 height);

    bool (*begin_frame)(struct renderer_backend* backend, f32 delta_time);

    void (*update_global_world_state)(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_color, i32 mode);

    void (*update_global_ui_state)(mat4 projection, mat4 view, i32 mode);

    bool (*end_frame)(struct renderer_backend* backend, f32 delta_time);

    bool (*begin_renderpass)(struct renderer_backend* backend, builtin_renderpass renderpass_id);

    bool (*end_renderpass)(struct renderer_backend* backend, builtin_renderpass renderpass_id);

    void (*draw_geometry)(geometry_render_data data);

    void (*create_texture)(texture* texture, const void* pixels);

    void (*destroy_texture)(texture* texture);

    bool (*create_material)(material* material);

    void (*destroy_material)(material* material);

    bool (*create_geometry)(geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count, const u32* indices);

    void (*destroy_geometry)(geometry* geometry);

} renderer_backend;

typedef struct render_packet {
    f32 delta_time;

    u32 geometry_count;
    geometry_render_data* geometries;

    u32 ui_geometry_count;
    geometry_render_data* ui_geometries;
} render_packet;
