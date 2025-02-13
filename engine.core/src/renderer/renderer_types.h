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

typedef struct global_uniform_object {
    mat4 projection;       // 64 bytes.
    mat4 view;             // 64 bytes.
    mat4 m_reserved[2];    // 128 bytes зарезервировано.
} global_uniform_object;

typedef struct material_uniform_object {
    vec4 diffuse_color;    // 16 bytes.
    vec4 m_reserved[3];    // 48 bytes.
} material_uniform_object;

typedef struct geometry_render_data {
    mat4 model;
    geometry* geometry;
} geometry_render_data;

typedef struct renderer_backend {

    u64 frame_number;

    window* window_state;

    bool (*initialize)(struct renderer_backend* backend);

    void (*shutdown)(struct renderer_backend* backend);

    void (*resized)(struct renderer_backend* backend, i32 width, i32 height);

    bool (*begin_frame)(struct renderer_backend* backend, f32 delta_time);

    void (*update_global_state)(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_color, i32 mode);

    bool (*end_frame)(struct renderer_backend* backend, f32 delta_time);

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
} render_packet;
