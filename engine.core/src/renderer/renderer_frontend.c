// Cобственные подключения.
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "math/kmath.h"
#include "resources/resource_types.h"

typedef struct renderer_system_state {
    renderer_backend backend;
    mat4 projection;
    mat4 view;
    f32 fov_radians;
    f32 near_clip;
    f32 far_clip;
} renderer_system_state;

static renderer_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the renderer system to be initialized. Call 'renderer_system_initialize' first.!";

bool renderer_system_initialize(u64* memory_requirement, void* memory, window* window_state)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once. Return false!", __FUNCTION__);
        return false;
    }

    *memory_requirement = sizeof(struct renderer_system_state);

    if(!memory)
    {
        return true;
    }

    kzero(memory, *memory_requirement);
    state_ptr = memory;

    // Инициализация.
    // TODO: Сделать настраиваемым из приложения!
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);
    state_ptr->backend.window_state = window_state;

    if(!state_ptr->backend.initialize(&state_ptr->backend))
    {
        return false;
    }

    // TODO: Сделать настраиваемыми!
    state_ptr->fov_radians = 45.0f;
    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 1000.0f;

    // Создание матрицы проекции.
    f32 aspect = window_state->width / (f32)window_state->height;
    state_ptr->projection = mat4_perspective(state_ptr->fov_radians, aspect, state_ptr->near_clip, state_ptr->far_clip);

    // Создание матрицы вида.
    state_ptr->view = mat4_translation((vec3){{0, 0, 30.0f}});
    state_ptr->view = mat4_inverse(state_ptr->view);

    return true;
}

void renderer_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // Завершение работы рендерера.
    state_ptr->backend.shutdown(&state_ptr->backend);
    renderer_backend_destroy(&state_ptr->backend);
    state_ptr = null;
}

bool renderer_begin_frame(f32 delta_time)
{
    return state_ptr->backend.begin_frame(&state_ptr->backend, delta_time);
}

bool renderer_end_frame(f32 delta_time)
{
    bool result = state_ptr->backend.end_frame(&state_ptr->backend, delta_time);
    state_ptr->backend.frame_number++;
    return result;
}

bool renderer_draw_frame(render_packet* packet)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;    
    }

    if(renderer_begin_frame(packet->delta_time))
    {
        state_ptr->backend.update_global_state(state_ptr->projection, state_ptr->view, vec3_zero(), vec4_one(), 0);

        u32 count = packet->geometry_count;
        for(u32 i = 0; i < count; ++i)
        {
            state_ptr->backend.draw_geometry(packet->geometries[i]);
        }

        bool result = renderer_end_frame(packet->delta_time);
        if(!result)
        {
            kerror("Failed to complete function 'renderer_end_frame'. Shutting down.");
            return false;
        }
    }
    return true;
}

void renderer_on_resize(i32 width, i32 height)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    f32 aspect = width / (f32)height;
    state_ptr->projection = mat4_perspective(state_ptr->fov_radians, aspect, state_ptr->near_clip, state_ptr->far_clip);
    state_ptr->backend.resized(&state_ptr->backend, width, height);
}

void renderer_set_view(mat4 view)
{
    state_ptr->view = view;
}

void renderer_create_texture(texture* texture, const void* pixels)
{
    state_ptr->backend.create_texture(texture, pixels);
}

void renderer_destroy_texture(texture* texture)
{
    state_ptr->backend.destroy_texture(texture);
}

bool renderer_create_material(material* material)
{
    return state_ptr->backend.create_material(material);
}

void renderer_destroy_material(material* material)
{
    state_ptr->backend.destroy_material(material);
}

bool renderer_create_geometry(
    geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count, const u32* indices
)
{
    return state_ptr->backend.create_geometry(geometry, vertex_count, vertices, index_count, indices);
}

void renderer_destroy_geometry(geometry* geometry)
{
    state_ptr->backend.destroy_geometry(geometry);
}
