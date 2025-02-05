// Cобственные подключения.
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "math/kmath.h"

typedef struct renderer_system_state {
    renderer_backend backend;
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
    if(!memory) return true;

    kzero(memory, *memory_requirement);
    state_ptr = memory;

    // TODO: Сделать настраиваемым из приложения!
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);
    state_ptr->backend.window_state = window_state;

    if(!state_ptr->backend.initialize(&state_ptr->backend))
    {
        return false;
    }

    return true;
}

void renderer_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    state_ptr->backend.shutdown(&state_ptr->backend);
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
        // TODO: Временный тестовый код: начало.
        mat4 projection = mat4_perspective(deg_to_rad(45.0f), 1280/720.0f, 0.1f, 1000.0f);
        static f32 z = -1.0f;
        z -= 0.1f;
        mat4 view = mat4_translation((vec3){{0, 0, z}});

        state_ptr->backend.update_global_state(projection, view, vec3_zero(), vec4_one(), 0);
        // TODO: Временный тестовый код: конец.

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
    state_ptr->backend.resized(&state_ptr->backend, width, height);
}
