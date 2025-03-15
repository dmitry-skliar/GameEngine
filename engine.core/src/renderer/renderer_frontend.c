// Cобственные подключения.
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"
#include "renderer/renderer_types.h"

// Внутренние подключения.
#include "logger.h"
#include "event.h"
#include "memory/memory.h"
#include "math/kmath.h"
#include "kstring.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"

typedef struct renderer_system_state {
    renderer_backend backend;
    mat4 projection;
    mat4 view;
    vec3 view_position;
    vec4 ambient_color;
    mat4 ui_projection;
    mat4 ui_view;
    f32 fov_radians;
    f32 near_clip;
    f32 far_clip;
    f32 ui_near_clip;
    f32 ui_far_clip;
    u32 material_shader_id;
    u32 ui_shader_id;
    u32 render_mode;
} renderer_system_state;

static renderer_system_state* state_ptr = null;

#define CRITICAL_INIT(op, msg) \
    if(!op)                    \
    {                          \
        kerror(msg);           \
        return false;          \
    }

bool renderer_on_event(event_code code, void* sender, void* listener, event_context* context)
{
    if(code == EVENT_CODE_SET_RENDER_MODE)
    {
        switch(context->i32[0])
        {
            case RENDERER_VIEW_MODE_DEFAULT:
                kdebug("Renderer mode set to default.");
                state_ptr->render_mode = RENDERER_VIEW_MODE_DEFAULT;
                break;
            case RENDERER_VIEW_MODE_LIGHTING:
                kdebug("Renderer mode set to lighting.");
                state_ptr->render_mode = RENDERER_VIEW_MODE_LIGHTING;
                break;
            case RENDERER_VIEW_MODE_NORMALS:
                kdebug("Renderer mode set to normals.");
                state_ptr->render_mode = RENDERER_VIEW_MODE_NORMALS;
                break;
        }
        return true;
    }

    return false;
}

// TODO: Создать отдельный макрос, который будет использовать функцию в отладночной редакции только!
// TODO: Вынести во вспомогательную функцию! для всех систем!
// Используется для проверки статуса системы.
bool renderer_system_status_valid(const char* func_name)
{
    if(!state_ptr)
    {
        if(func_name)
        {
            kerror(
                "Function '%s' requires the renderer system to be initialized. Call 'renderer_system_initialize' first.",
                func_name
            );
            
        }
        return false;
    }
    return true;
}

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
    state_ptr->backend.frame_number = 0;
    state_ptr->render_mode = RENDERER_VIEW_MODE_DEFAULT;

    CRITICAL_INIT(
        state_ptr->backend.initialize(&state_ptr->backend),
        "Renderer backend failed to initialize."
    );

    // Шейдеры.
    resource config_resource;
    shader_config* sconfig = null;

    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_MATERIAL, RESOURCE_TYPE_SHADER, &config_resource),
        "Failed to load builtin material shader."
    );

    sconfig = config_resource.data;
    CRITICAL_INIT(
        shader_system_create(sconfig),
        "Failed to create builtin material shader."
    );

    resource_system_unload(&config_resource);
    state_ptr->material_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_MATERIAL);

    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_UI, RESOURCE_TYPE_SHADER, &config_resource),
        "Failed to load builtin UI shader."
    );

    sconfig = config_resource.data;
    CRITICAL_INIT(
        shader_system_create(sconfig),
        "Failed to create builtin UI shader."
    );

    resource_system_unload(&config_resource);
    state_ptr->ui_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_UI);

    // Создание матриц проекции и вида (world).
    // TODO: Сделать настраиваемыми!
    state_ptr->fov_radians = deg_to_rad(60.0f);
    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 500.0f;
    f32 aspect = window_state->width / (f32)window_state->height;

    state_ptr->projection = mat4_perspective(state_ptr->fov_radians, aspect, state_ptr->near_clip, state_ptr->far_clip);
    // TODO: Конфигурация стартовой позиции камеры.
    state_ptr->view = mat4_translation((vec3){{0, 0, 30.0f}}); // game.c установка камеры!
    state_ptr->view = mat4_inverse(state_ptr->view);
    // TODO: Получение из сцены.
    state_ptr->ambient_color = (vec4){{0.15f, 0.15f, 0.15f, 1.0f}};

    // Создание матриц проекции и вида (ui).
    state_ptr->ui_near_clip = -100.0f;
    state_ptr->ui_far_clip = 100.0f;
    state_ptr->ui_projection = mat4_orthographic(0, window_state->width, window_state->height, 0, state_ptr->ui_near_clip, state_ptr->ui_far_clip);
    state_ptr->ui_view = mat4_inverse(mat4_identity());

    event_register(EVENT_CODE_SET_RENDER_MODE, state_ptr, renderer_on_event);

    return true;
}

void renderer_system_shutdown()
{
    if(!renderer_system_status_valid(__FUNCTION__))
    {
        return;
    }

    // NOTE: Уничтожение шейдеров сделает система шейдеров автоматически.

    // Завершение работы рендерера.
    state_ptr->backend.shutdown(&state_ptr->backend);
    renderer_backend_destroy(&state_ptr->backend);

    state_ptr = null;
}

void renderer_on_resize(i32 width, i32 height)
{
    if(!renderer_system_status_valid(__FUNCTION__))
    {
        return;
    }

    f32 aspect = width / (f32)height;
    state_ptr->projection = mat4_perspective(state_ptr->fov_radians, aspect, state_ptr->near_clip, state_ptr->far_clip);
    state_ptr->ui_projection = mat4_orthographic(0, (f32)width, (f32)height, 0, state_ptr->ui_near_clip, state_ptr->ui_far_clip);
    state_ptr->backend.resized(&state_ptr->backend, width, height);    
}

bool renderer_draw_frame(render_packet* packet)
{
    if(!renderer_system_status_valid(__FUNCTION__))
    {
        return false;
    }

    // Производить генерацию кадров даже, если исход плохой!
    state_ptr->backend.frame_number++;

    if(state_ptr->backend.begin_frame(&state_ptr->backend, packet->delta_time))
    {
        // Проход (world).
        if(!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD))
        {
            kerror("Begin renderpass -> BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }

        if(!shader_system_use_by_id(state_ptr->material_shader_id))
        {
            kerror("Failed to use material shader. Render frame failed.");
            return false;
        }

        if(!material_system_apply_global(
                state_ptr->material_shader_id, &state_ptr->projection, &state_ptr->view, &state_ptr->view_position,
                &state_ptr->ambient_color, state_ptr->render_mode
        ))
        {
            kerror("Failed to use apply globals for material shader. Render frame failed.");
            return false;
        }

        // Рисование World.
        u32 count = packet->geometry_count;
        for(u32 i = 0; i < count; ++i)
        {
            material* m = null;
            if(packet->geometries[i].geometry->material)
            {
                m = packet->geometries[i].geometry->material;
            }
            else
            {
                m = material_system_get_default();
            }

            // Применение материала.
            bool needs_update = m->render_frame_number != state_ptr->backend.frame_number;
            if(!material_system_apply_instance(m, needs_update))
            {
                kwarng("Failed to apply material '%s'. Skipping draw.", m->name);
                continue;
            }
            else
            {
                m->render_frame_number = state_ptr->backend.frame_number;
            }

            // Применение локальной позиции объекта.
            material_system_apply_local(m, &packet->geometries[i].model);

            // Нарисовать!
            state_ptr->backend.geometry_draw(packet->geometries[i]);
        }

        if(!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD))
        {
            kerror("End renderpass -> BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }

        // Проход (UI).
        if(!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI))
        {
            kerror("Begin renderpass -> BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        if(!shader_system_use_by_id(state_ptr->ui_shader_id))
        {
            kerror("Failed to use UI shader. Render frame failed.");
            return false;
        }

        if(!material_system_apply_global(state_ptr->ui_shader_id, &state_ptr->ui_projection, &state_ptr->ui_view, null, null, 0))
        {
            kerror("Failed to use apply globals for material shader. Render frame failed.");
            return false;
        }

        // Рисование UI.
        count = packet->ui_geometry_count;
        for(u32 i = 0; i < count; ++i)
        {
            material* m = null;
            if(packet->ui_geometries[i].geometry->material)
            {
                m = packet->ui_geometries[i].geometry->material;
            }
            else
            {
                m = material_system_get_default();
            }

            // Применение материала.
            bool needs_update = m->render_frame_number != state_ptr->backend.frame_number;
            if(!material_system_apply_instance(m, needs_update))
            {
                kwarng("Failed to apply UI material '%s'. Skipping draw.", m->name);
                continue;
            }
            else
            {
                m->render_frame_number = state_ptr->backend.frame_number;
            }

            // Применение локальной позиции объекта.
            material_system_apply_local(m, &packet->ui_geometries[i].model);

            // Нарисовать!
            state_ptr->backend.geometry_draw(packet->ui_geometries[i]);
        }

        if(!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI))
        {
            kerror("End renderpass -> BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        if(!state_ptr->backend.end_frame(&state_ptr->backend, packet->delta_time))
        {
            kerror("Failed to complete function 'renderer_end_frame'. Shutting down.");
            return false;
        }
    }
    return true;
}

void renderer_set_view(mat4 view, vec3 view_position)
{
    state_ptr->view = view;
    state_ptr->view_position = view_position;
}

void renderer_texture_create(texture* texture, const void* pixels)
{
    state_ptr->backend.texture_create(texture, pixels);
}

void renderer_texture_create_writable(texture* texture)
{
    state_ptr->backend.texture_create_writable(texture);
}

void renderer_texture_resize(texture* texture, u32 new_width, u32 new_height)
{
    state_ptr->backend.texture_resize(texture, new_width, new_height);
}

void renderer_texture_write_data(texture* texture, u32 offset, u32 size, const void* pixels)
{
    state_ptr->backend.texture_write_data(texture, offset, size, pixels);
}

void renderer_texture_destroy(texture* texture)
{
    state_ptr->backend.texture_destroy(texture);
}

bool renderer_texture_map_acquire_resources(texture_map* map)
{
    return state_ptr->backend.texture_map_acquire_resources(map);
}

void renderer_texture_map_release_resources(texture_map* map)
{
    state_ptr->backend.texture_map_release_resources(map);
}

bool renderer_geometry_create(
    geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count,
    const void* indices
)
{
    return state_ptr->backend.geometry_create(geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void renderer_geometry_destroy(geometry* geometry)
{
    state_ptr->backend.geometry_destroy(geometry);
}

bool renderer_renderpass_id(const char* name, u8* out_renderpass_id)
{
    // TODO: Нужны динамические проходы рендеринга!

    if(string_equali("Builtin.RenderpassWorld", name))
    {
        *out_renderpass_id = BUILTIN_RENDERPASS_WORLD;
        return true;
    }
    else if(string_equali("Builtin.RenderpassUI", name))
    {
        *out_renderpass_id = BUILTIN_RENDERPASS_UI;
        return true;
    }

    kerror("Function '%s': No renderpass named '%s'.", __FUNCTION__, name);
    *out_renderpass_id = INVALID_ID_U8;
    return false;
}

bool renderer_shader_create(shader* s, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages)
{
    return state_ptr->backend.shader_create(s, renderpass_id, stage_count, stage_filenames, stages);
}

void renderer_shader_destroy(shader* s)
{
    state_ptr->backend.shader_destroy(s);
}

bool renderer_shader_initialize(shader* s)
{
    return state_ptr->backend.shader_initialize(s);
}

bool renderer_shader_use(shader* s)
{
    return state_ptr->backend.shader_use(s);
}

bool renderer_shader_bind_globals(shader* s)
{
    return state_ptr->backend.shader_bind_globals(s);
}

bool renderer_shader_bind_instance(shader* s, u32 instance_id)
{
    return state_ptr->backend.shader_bind_instance(s, instance_id);
}

bool renderer_shader_apply_globals(shader* s)
{
    return state_ptr->backend.shader_apply_globals(s);
}

bool renderer_shader_apply_instance(shader* s, bool needs_update)
{
    return state_ptr->backend.shader_apply_instance(s, needs_update);
}

bool renderer_shader_acquire_instance_resources(shader* s, texture_map** maps, u32* out_instance_id)
{
    return state_ptr->backend.shader_acquire_instance_resources(s, maps, out_instance_id);
}

bool renderer_shader_release_instance_resources(shader* s, u32 instance_id)
{
    return state_ptr->backend.shader_release_instance_resources(s, instance_id);
}

bool renderer_shader_set_uniform(shader* s, shader_uniform* uniform, const void* value)
{
    return state_ptr->backend.shader_set_uniform(s, uniform, value);
}
