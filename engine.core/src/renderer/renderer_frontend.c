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
    mat4 ui_projection;
    mat4 ui_view;
    f32 fov_radians;
    f32 near_clip;
    f32 far_clip;
    f32 ui_near_clip;
    f32 ui_far_clip;
} renderer_system_state;

static renderer_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the renderer system to be initialized. Call 'renderer_system_initialize' first.";

bool renderer_system_initialize(u64* memory_requirement, void* memory, window* window_state)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once.", __FUNCTION__);
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

    // Создание матриц проекции и вида (world).
    // TODO: Сделать настраиваемыми!
    state_ptr->fov_radians = 45.0f;
    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 1000.0f;
    f32 aspect = window_state->width / (f32)window_state->height;

    state_ptr->projection = mat4_perspective(state_ptr->fov_radians, aspect, state_ptr->near_clip, state_ptr->far_clip);
    // TODO: Конфигурация стартовой позиции камеры.
    state_ptr->view = mat4_translation((vec3){{0, 0, 30.0f}});
    state_ptr->view = mat4_inverse(state_ptr->view);

    // Создание матриц проекции и вида (ui).
    state_ptr->ui_near_clip = -100.0f;
    state_ptr->ui_far_clip = 100.0f;
    state_ptr->ui_projection = mat4_orthographic(0, window_state->width, window_state->height, 0, state_ptr->ui_near_clip, state_ptr->ui_far_clip);
    state_ptr->ui_view = mat4_inverse(mat4_identity());

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

bool renderer_draw_frame(render_packet* packet)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }

    if(state_ptr->backend.begin_frame(&state_ptr->backend, packet->delta_time))
    {
        // Проход (world).
        if(!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD))
        {
            kerror("Begin renderpass -> BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }

        state_ptr->backend.update_global_world_state(state_ptr->projection, state_ptr->view, vec3_zero(), vec4_one(), 0);

        // Рисование World.
        u32 count = packet->geometry_count;
        for(u32 i = 0; i < count; ++i)
        {
            state_ptr->backend.draw_geometry(packet->geometries[i]);
        }

        if(!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD))
        {
            kerror("End renderpass -> BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }

        // Проход (world).
        if(!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI))
        {
            kerror("Begin renderpass -> BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        state_ptr->backend.update_global_ui_state(state_ptr->ui_projection, state_ptr->ui_view, 0);

        // Рисование UI.
        count = packet->ui_geometry_count;
        for(u32 i = 0; i < count; ++i)
        {
            state_ptr->backend.draw_geometry(packet->ui_geometries[i]);
        }

        if(!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI))
        {
            kerror("End renderpass -> BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        bool result = state_ptr->backend.end_frame(&state_ptr->backend, packet->delta_time);
        state_ptr->backend.frame_number++;
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

    // Обновление проекции (world).
    f32 aspect = width / (f32)height;
    state_ptr->projection = mat4_perspective(state_ptr->fov_radians, aspect, state_ptr->near_clip, state_ptr->far_clip);

    // Обновление проекции (ui).
    state_ptr->ui_projection = mat4_orthographic(0, (f32)width, (f32)height, 0, state_ptr->ui_near_clip, state_ptr->ui_far_clip);

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
    geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count,
    const void* indices
)
{
    return state_ptr->backend.create_geometry(geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void renderer_destroy_geometry(geometry* geometry)
{
    state_ptr->backend.destroy_geometry(geometry);
}
