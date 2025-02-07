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

typedef struct object_uniform_object {
    vec4 diffuse_color;    // 16 bytes.
    vec4 m_reserved[3];    // 48 bytes.
} object_uniform_object;

typedef struct geometry_render_data {
    u32 object_id;
    mat4 model;
    texture* textures[16];
} geometry_render_data;

typedef struct renderer_backend {

    u64 frame_number;

    window* window_state;

    // @brief Указатель на текстуру по умолчанию.
    texture* default_diffuse;

    bool (*initialize)(struct renderer_backend* backend);

    void (*shutdown)(struct renderer_backend* backend);

    void (*resized)(struct renderer_backend* backend, i32 width, i32 height);

    bool (*begin_frame)(struct renderer_backend* backend, f32 delta_time);

    void (*update_global_state)(mat4 projection, mat4 view, vec3 view_position, vec4 ambient_color, i32 mode);

    bool (*end_frame)(struct renderer_backend* backend, f32 delta_time);

    void (*update_object)(geometry_render_data data);

    void (*create_texture)(
        const char* name, bool auto_release, i32 width, i32 height, i32 channel_count, const u8* pixels,
        bool has_transparency, texture* out_texture
    );

    void (*destroy_texture)(texture* texture);

} renderer_backend;

typedef struct render_packet {
    f32 delta_time;    
} render_packet;
