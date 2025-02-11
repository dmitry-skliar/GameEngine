// Cобственные подключения.
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "math/kmath.h"
#include "resources/resource_types.h"

// TODO: Временный тестовый код: начало.
#include "event.h"
#include "kstring.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
// TODO: Временный тестовый код: конец.

typedef struct renderer_system_state {
    renderer_backend backend;
    mat4 projection;
    mat4 view;
    f32 fov_radians;
    f32 near_clip;
    f32 far_clip;

// TODO: Временный тестовый код: начало.
    material* test_material;
// TODO: Временный тестовый код: конец.
} renderer_system_state;

static renderer_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the renderer system to be initialized. Call 'renderer_system_initialize' first.!";

// TODO: Временный тестовый код: начало.
bool event_on_debug_event(event_code code, void* sender, void* listener_inst, event_context* context)
{
    const char* names[3] = { "cobblestone", "paving", "paving2" };
    static i8 choice = 1;

    const char* old_name = names[choice];

    choice++;
    choice %= 3;

    state_ptr->test_material->diffuse_map.texture = texture_system_acquire(names[choice], true);
    if(!state_ptr->test_material->diffuse_map.texture)
    {
        kwarng("NO TEXTURE! Using default.");
        state_ptr->test_material->diffuse_map.texture = texture_system_get_default_texture();
    }

    texture_system_release(old_name);

    return true;
}
// TODO: Временный тестовый код: конец.

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

    // TODO: Временный тестовый код: начало.
    event_register(EVENT_CODE_DEBUG_0, state_ptr, event_on_debug_event);
    // TODO: Временный тестовый код: конец.

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
        // TODO: Временный тестовый код: начало.
        state_ptr->backend.update_global_state(state_ptr->projection, state_ptr->view, vec3_zero(), vec4_one(), 0);

        mat4 model = mat4_translation((vec3){{0, 0, 0}});
        // static f32 angle = 0.0f;
        // angle += 0.01f;
        // quat rotation = quat_from_axis_angle(vec3_forward(), angle, false);
        // mat4 model = quat_to_rotation_matrix(rotation, vec3_zero());

        // Создает материал по умолчанию если не существует.
        if(!state_ptr->test_material)
        {
            state_ptr->test_material = material_system_acquire("test_material");

            if(!state_ptr->test_material)
            {
                kwarng("Automatic material load failed, falling back to manual default material.");

                material_config config;
                string_ncopy(config.name, "test_material", MATERIAL_NAME_MAX_LENGTH);
                config.auto_release = false;
                config.diffuse_color = vec4_one();
                string_ncopy(config.diffuse_map_name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
                state_ptr->test_material = material_system_acquire_from_config(&config);
            }
        }

        geometry_render_data data = {0};
        data.material = state_ptr->test_material;
        data.model = model;
        state_ptr->backend.update_object(data);
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
