#pragma once

#include <defines.h>

// @brief Тип рендера.
typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX
} renderer_backend_type;

typedef struct renderer_backend {

    u64 frame_number;

    bool (*initialize)(struct renderer_backend* backend, const char* application_name, u32 width, u32 height);

    void (*shutdown)(struct renderer_backend* backend);

    void (*resized)(struct renderer_backend* backend, i32 width, i32 height);

    bool (*begin_frame)(struct renderer_backend* backend, f32 delta_time);

    bool (*end_frame)(struct renderer_backend* backend, f32 delta_time);

} renderer_backend;

typedef struct render_packet {
    f32 delta_time;    
} render_packet;
