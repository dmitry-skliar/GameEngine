// Cобственные подключения.
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "math/kmath.h"
#include "resources/resource_types.h"

// TODO: Временный тестовый код: начало.
#include "kstring.h"
#include "event.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
// TODO: Временный тестовый код: конец.

typedef struct renderer_system_state {
    renderer_backend backend;
    mat4 projection;
    mat4 view;
    f32 fov_radians;
    f32 near_clip;
    f32 far_clip;

    texture default_texture;

// TODO: Временный тестовый код: начало.
    texture test_diffuse;
// TODO: Временный тестовый код: конец.
} renderer_system_state;

static renderer_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the renderer system to be initialized. Call 'renderer_system_initialize' first.!";

// TODO: Временный тестовый код: начало.
void texture_create(texture* t)
{
    if(!t)
    {
        kerror("Function '%s' in file '%s' requires a pointer to texture. ", __FUNCTION__, __FILE_NAME__);
        return;
    }

    kzero_tc(t, struct texture, 1);
    t->generation = INVALID_ID32;
}

bool texture_load(const char* texture_name, texture* t)
{
    // TODO: Должна быть возможность размещения в любом месте.
    char* format_str = "../assets/textures/%s.%s";
    const i32 required_channel_count = 4;
    stbi_set_flip_vertically_on_load(true);
    char full_file_path[512];

    // TODO: попробовать разные расширения.
    string_format(full_file_path, format_str, texture_name, "png");

    // Временная текстура для загрузки.
    texture temp_texture;

    u8* data = stbi_load(
        full_file_path, (i32*)&temp_texture.width, (i32*)&temp_texture.height, (i32*)&temp_texture.channel_count,
        required_channel_count
    );

    temp_texture.channel_count = required_channel_count;

    if(!data)
    {
        kwarng("Function '%s': Failed to load file '%s' with result: %s.", __FUNCTION__, full_file_path, stbi_failure_reason());
        return false;
    }

    u32 current_generation = t->generation;
    t->generation = INVALID_ID32;

    u64 total_size = temp_texture.width * temp_texture.height * required_channel_count;

    // Проверка прозрачности.
    bool has_transparency = false;

    for(u64 i = 0; i < total_size; i += required_channel_count)
    {
        // RGB[A]
        u8 a = data[i + 3];
        if(a < 255)
        {
            has_transparency = true;
            break;
        }
    }

    if(stbi_failure_reason())
    {
        kwarng("Function '%s': Failed to load file '%s' with result: %s.", __FUNCTION__, full_file_path, stbi_failure_reason());
    }

    // Получение внутренних ресурсов текстуры и загрузка их в графический процессор.
    renderer_create_texture(
        texture_name, true, temp_texture.width, temp_texture.height, temp_texture.channel_count, data, 
        has_transparency, &temp_texture
    );

    // Копирование старой текстуры.
    texture old = *t;

    // Назначение временной текстуруы указателю.
    *t = temp_texture;

    // Уничтожение старуой текстуры.
    renderer_destroy_texture(&old);

    if(current_generation == INVALID_ID32)
    {
        t->generation = 0;
    }
    else
    {
        t->generation = current_generation + 1;
    }

    // Очистка данных.
    stbi_image_free(data);
    return true;
}

bool event_on_debug_event(event_code code, void* sender, void* listener_inst, event_context* context)
{
    const char* names[3] = { "cobblestone", "paving", "paving2" };
    static i8 choice = 2;

    choice++;
    choice %= 3;

    // TODO: Будет ошибка загрузки, потому что test_diffuse не был ранее создан!
    texture_load(names[choice], &state_ptr->test_diffuse);
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
    if(!memory) return true;

    kzero(memory, *memory_requirement);
    state_ptr = memory;

    // Инициализация.
    // TODO: Сделать настраиваемым из приложения!
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);
    state_ptr->backend.window_state = window_state;
    state_ptr->backend.default_diffuse = &state_ptr->default_texture;

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

            u64 colv = col % 128;
            u64 rowv = row % 128;

            if((colv > 63 && rowv < 64) || (colv < 64 && rowv > 63))
            {
                pixels[index_bpp + 0] = 50;
                pixels[index_bpp + 1] = 50;
                pixels[index_bpp + 2] = 50;
            }
        }
    }

    renderer_create_texture(
        "default", false, tex_dimension, tex_dimension, 4, pixels, false, &state_ptr->default_texture
    );

    // Установка генерацию текстуры как недействительную, так как это текстура по умолчанию.
    state_ptr->default_texture.generation = INVALID_ID32;

    // TODO: Загрузить другие текстуры.
    texture_create(&state_ptr->test_diffuse);

    return true;
}

void renderer_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // TODO: Убрать после тестов. Уничтожение тестовой текстуры.
    renderer_destroy_texture(&state_ptr->test_diffuse);

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
        // static f32 angle = 0.0f;
        // angle += 0.01f;
        // quat rotation = quat_from_axis_angle(vec3_forward(), angle, false);
        // mat4 model = quat_to_rotation_matrix(rotation, vec3_zero());
        geometry_render_data data = {0};
        data.object_id = 0;
        data.model = model;
        // data.textures[0] = &state_ptr->default_texture;
        data.textures[0] = &state_ptr->test_diffuse;
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
