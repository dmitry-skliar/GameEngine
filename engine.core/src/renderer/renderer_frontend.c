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

    texture default_texture;
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

    // NOTE: Создание текстуры по умолчанию, сине-белый шахматный узор 256x256.
    // Это делается в коде, чтобы исключить зависимости от ресурсов.
    ktrace("Function '%s' Create default texture...", __FUNCTION__);
    const u32 tex_dimension = 255;
    const u32 bpp = 4; // Байтов на пиксель (RBGA).
    const u32 pixel_count = tex_dimension * tex_dimension;
    u8 pixels[262150];
    // u8* pixels = kallocate(sizeof(u8) * pixel_count * bpp, MEMORY_TAG_TEXTURE);
    kset(pixels, sizeof(u8) * pixel_count * bpp, 255);

    // Каждый пиксель.
    for(u64 row = 0; row < tex_dimension; ++row)
    {
        for(u64 col = 0; col < tex_dimension; ++col)
        {
            u64 index = (row * tex_dimension) + col;
            u64 index_bpp = index * bpp;

            if(row % 2 == col % 2)
            {
                pixels[index_bpp + 0] = 0;
                pixels[index_bpp + 1] = 0;
            }
        }
    }

    renderer_create_texture(
        "default", false, tex_dimension, tex_dimension, 4, pixels, false, &state_ptr->default_texture
    );

    // Установка генерацию текстуры как недействительную, так как это текстура по умолчанию.
    state_ptr->default_texture.generation = INVALID_ID32;

    // TODO: Загрузить другие текстуры.
    // create_texture(&state_ptr->test_diffuse);

    return true;
}

void renderer_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // Уничтожение текстуры по умолчанию.
    renderer_destroy_texture(&state_ptr->default_texture);

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
        static f32 angle = 0.0f;
        // angle += 0.01f;
        // quat rotation = quat_from_axis_angle(vec3_forward(), angle, false);
        // mat4 model = quat_to_rotation_matrix(rotation, vec3_zero());
        geometry_render_data data = {0};
        data.object_id = 0;
        data.model = model;
        data.textures[0] = &state_ptr->default_texture;
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

void renderer_create_texture(
    const char* name, bool auto_release, i32 width, i32 height, i32 channel_count, const u8* pixels,
    bool has_transparency, texture* out_texture
)
{
    state_ptr->backend.create_texture(name, auto_release, width, height, channel_count, pixels, has_transparency, out_texture);
}

void renderer_destroy_texture(texture* texture)
{
    state_ptr->backend.destroy_texture(texture);
}
