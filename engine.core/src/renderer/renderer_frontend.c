// Cобственные подключения.
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"
#include "renderer/renderer_types.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/render_view_system.h"

typedef struct renderer_system_state {
    renderer_backend backend;
    u32 world_shader_id;
    u32 ui_shader_id;
    // Количество целей визуализации (количество кадров цепочки обмена).
    u8 window_render_target_count;
    u32 framebuffer_width;
    u32 framebuffer_height;
    // TODO: Сделаит настраиваемыми через показы.
    renderpass* world_renderpass;
    renderpass* ui_renderpass;
    // Указывает что сейчас происходит изменение размера окна.
    bool resizing;
} renderer_system_state;

static renderer_system_state* state_ptr = null;

#define CRITICAL_INIT(op, msg) \
    if(!op)                    \
    {                          \
        kerror(msg);           \
        return false;          \
    }

void regenerate_render_targets()
{
    // TODO: Должно настраиваться.
    for(u8 i = 0; i < state_ptr->window_render_target_count; ++i)
    {
        // TODO: При инициализации еще нечего уничтожать, а потому возникают ошибки, но они пока допустимые.
        state_ptr->backend.render_target_destroy(&state_ptr->world_renderpass->targets[i], false);
        state_ptr->backend.render_target_destroy(&state_ptr->ui_renderpass->targets[i], false);

        texture* window_target_texture = state_ptr->backend.window_attachment_get(i);
        texture* depth_target_texture = state_ptr->backend.depth_attachment_get();

        // World.
        texture* attachments[2] = { window_target_texture, depth_target_texture };
        state_ptr->backend.render_target_create(
            2, attachments, state_ptr->world_renderpass, state_ptr->framebuffer_width, state_ptr->framebuffer_height,
            &state_ptr->world_renderpass->targets[i]
        );

        // UI.
        texture* ui_attachments[1] = { window_target_texture };
        state_ptr->backend.render_target_create(
            1, ui_attachments, state_ptr->ui_renderpass, state_ptr->framebuffer_width, state_ptr->framebuffer_height,
            &state_ptr->ui_renderpass->targets[i]
        );
    }
}

bool system_status_valid(const char* func_name)
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

    // TODO: Должны быть единая точка задания размеров кадрового буфера при инициализации!
    state_ptr->framebuffer_width = window_state->width;
    state_ptr->framebuffer_height = window_state->height;
    state_ptr->resizing = false;

    // Инициализация.
    // TODO: Сделать настраиваемым из приложения!
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);
    state_ptr->backend.window_state = window_state; // TODO: Убрать!
    state_ptr->backend.frame_number = 0;

    // Проходчики визуализатора.
    // TODO: Чтение из конфигурации.
    const char* world_renderpass_name = "Builtin.RenderpassWorld";
    const char* ui_renderpass_name = "Builtin.RenderpassUI";
    renderpass_config pass_config[2];
    // World.
    pass_config[0].name = world_renderpass_name;
    pass_config[0].prev_name = null;
    pass_config[0].next_name = ui_renderpass_name;
    pass_config[0].render_area = (vec4){{ 0, 0, state_ptr->framebuffer_width, state_ptr->framebuffer_height }};
    pass_config[0].clear_color = (vec4){{ 0.0f, 0.0f, 0.2f, 1.0f }};
    pass_config[0].clear_flags = RENDERPASS_CLEAR_COLOR_BUFFER_FLAG | RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG | RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG;
    // UI.
    pass_config[1].name = ui_renderpass_name;
    pass_config[1].prev_name = world_renderpass_name;
    pass_config[1].next_name = null;
    pass_config[1].render_area = (vec4){{ 0, 0, state_ptr->framebuffer_width, state_ptr->framebuffer_height }};
    pass_config[1].clear_color = (vec4){{ 0.0f, 0.0f, 0.2f, 1.0f }};
    pass_config[1].clear_flags = RENDERPASS_CLEAR_NONE_FLAG;

    renderer_backend_config renderer_config = {};
    renderer_config.application_name = window_state->title;
    renderer_config.on_rendertarget_refresh_required = regenerate_render_targets;
    renderer_config.renderpass_count = 2;
    renderer_config.pass_configs = pass_config;

    CRITICAL_INIT(
        state_ptr->backend.initialize(&state_ptr->backend, &renderer_config, &state_ptr->window_render_target_count),
        "Renderer backend failed to initialize."
    );

    // TODO: Изменить способ получения.
    state_ptr->world_renderpass = state_ptr->backend.renderpass_get(world_renderpass_name);
    state_ptr->world_renderpass->render_target_count = state_ptr->window_render_target_count;
    state_ptr->world_renderpass->targets = kallocate_tc(render_target, state_ptr->window_render_target_count, MEMORY_TAG_ARRAY);
    kzero_tc(state_ptr->world_renderpass->targets, render_target, state_ptr->window_render_target_count);

    state_ptr->ui_renderpass = state_ptr->backend.renderpass_get(ui_renderpass_name);
    state_ptr->ui_renderpass->render_target_count = state_ptr->window_render_target_count;
    state_ptr->ui_renderpass->targets = kallocate_tc(render_target, state_ptr->window_render_target_count, MEMORY_TAG_ARRAY);
    kzero_tc(state_ptr->ui_renderpass->targets, render_target, state_ptr->window_render_target_count);

    regenerate_render_targets();

    // Обновление разрешения проходов визуализаторов.
    state_ptr->world_renderpass->render_area.x = state_ptr->world_renderpass->render_area.y = 0;
    state_ptr->world_renderpass->render_area.width = state_ptr->framebuffer_width;
    state_ptr->world_renderpass->render_area.height = state_ptr->framebuffer_height;

    state_ptr->ui_renderpass->render_area.x = state_ptr->ui_renderpass->render_area.y = 0;
    state_ptr->ui_renderpass->render_area.width = state_ptr->framebuffer_width;
    state_ptr->ui_renderpass->render_area.height = state_ptr->framebuffer_height;

    // Шейдеры.
    resource config_resource;
    shader_config* sconfig = null;

    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_WORLD, RESOURCE_TYPE_SHADER, &config_resource),
        "Failed to load builtin material shader."
    );

    sconfig = config_resource.data;
    CRITICAL_INIT(
        shader_system_create(sconfig),
        "Failed to create builtin material shader."
    );

    resource_system_unload(&config_resource);
    state_ptr->world_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_WORLD);

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

    return true;
}

void renderer_system_shutdown()
{
    if(!system_status_valid(__FUNCTION__)) return;

    // NOTE: Уничтожение шейдеров сделает система шейдеров автоматически.

    // Уничтожение целей визуализации.
    for(u8 i = 0; i < state_ptr->window_render_target_count; ++i)
    {
        state_ptr->backend.render_target_destroy(&state_ptr->world_renderpass->targets[i], true);
        state_ptr->backend.render_target_destroy(&state_ptr->ui_renderpass->targets[i], true);
    }
    kfree_tc(state_ptr->world_renderpass->targets, render_target, state_ptr->window_render_target_count, MEMORY_TAG_ARRAY);
    kfree_tc(state_ptr->ui_renderpass->targets, render_target, state_ptr->window_render_target_count, MEMORY_TAG_ARRAY);

    // Завершение работы рендерера.
    state_ptr->backend.shutdown(&state_ptr->backend);
    renderer_backend_destroy(&state_ptr->backend);

    state_ptr = null;
}

void renderer_on_resize(i32 width, i32 height)
{
    if(!system_status_valid(__FUNCTION__)) return;

    state_ptr->resizing = true;
    state_ptr->framebuffer_width = width;
    state_ptr->framebuffer_height = height;
}

bool renderer_draw_frame(render_packet* packet)
{
    if(!system_status_valid(__FUNCTION__)) return false;

    // Производить генерацию кадров даже, если исход плохой!
    state_ptr->backend.frame_number++;

    if(state_ptr->resizing)
    {
        f32 width = state_ptr->framebuffer_width;
        f32 height = state_ptr->framebuffer_height;
        render_view_system_on_window_resize(width, height);
        state_ptr->backend.resized(width, height);
        state_ptr->resizing = false;
    }

    if(state_ptr->backend.frame_begin(packet->delta_time))
    {
        u8 attachment_index = state_ptr->backend.window_attachment_index_get();

        for(u32 i = 0; i < packet->view_count; ++i)
        {
            if(!render_view_system_on_render(packet->views[i].view, &packet->views[i], state_ptr->backend.frame_number, attachment_index))
            {
                kerror("Error rendering view index %i.", i);
                return false;
            }
        }

        if(!state_ptr->backend.frame_end(packet->delta_time))
        {
            kerror("Failed to complete function 'renderer_end_frame'. Shutting down.");
            return false;
        }
    }
    return true;
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

void renderer_geometry_draw(geometry_render_data* data)
{
    state_ptr->backend.geometry_draw(data);
}

bool renderer_shader_create(shader* s, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages)
{
    return state_ptr->backend.shader_create(s, pass, stage_count, stage_filenames, stages);
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

void renderer_render_target_create(u8 attachment_count, texture** attachments, renderpass* pass, u32 width, u32 height, render_target* out_target)
{
    state_ptr->backend.render_target_create(attachment_count, attachments, pass, width, height, out_target);    
}

void renderer_render_target_destroy(render_target* target, bool free_internal_memory)
{
    state_ptr->backend.render_target_destroy(target, free_internal_memory);
}

void renderer_renderpass_create(renderpass* out_renderpass, f32 depth, u32 stencil, bool has_prev_pass, bool has_next_pass)
{
    state_ptr->backend.renderpass_create(out_renderpass, depth, stencil, has_prev_pass, has_next_pass);    
}

void renderer_renderpass_destroy(renderpass* pass)
{
    state_ptr->backend.renderpass_destroy(pass);
}

bool renderer_renderpass_begin(renderpass* pass, render_target* target)
{
    return state_ptr->backend.renderpass_begin(pass, target);    
}

bool renderer_renderpass_end(renderpass* pass)
{
    return state_ptr->backend.renderpass_end(pass);
}

renderpass* renderer_renderpass_get(const char* name)
{
    return state_ptr->backend.renderpass_get(name);
}
