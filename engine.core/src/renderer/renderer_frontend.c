// Cобственные подключения.
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"
#include "renderer/renderer_types.h"

// Внутренние подключения.
#include "logger.h"
#include "debug/assert.h"
#include "memory/memory.h"
#include "containers/freelist.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/render_view_system.h"

typedef struct renderer_system_state {
    renderer_backend backend;
    u32 skybox_shader_id;
    u32 world_shader_id;
    u32 ui_shader_id;
    // Количество целей визуализации (количество кадров цепочки обмена).
    u8 window_render_target_count;
    u32 framebuffer_width;
    u32 framebuffer_height;
    // TODO: Сделаит настраиваемыми через показы.
    renderpass* skybox_renderpass;
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
    // Первичная инициализация.
    static bool initialized = false;

    // TODO: Должно настраиваться.
    for(u8 i = 0; i < state_ptr->window_render_target_count; ++i)
    {
        // При инициализации еще нечего уничтожать, а потому возникают ошибки, но они пока допустимые.
        if(initialized)
        {
            state_ptr->backend.render_target_destroy(&state_ptr->skybox_renderpass->targets[i], false);
            state_ptr->backend.render_target_destroy(&state_ptr->world_renderpass->targets[i], false);
            state_ptr->backend.render_target_destroy(&state_ptr->ui_renderpass->targets[i], false);
        }

        texture* window_target_texture = state_ptr->backend.window_attachment_get(i);
        texture* depth_target_texture = state_ptr->backend.depth_attachment_get();

        // Skybox.
        texture* skybox_attachments[1] = { window_target_texture };
        state_ptr->backend.render_target_create(
            1, skybox_attachments, state_ptr->skybox_renderpass, state_ptr->framebuffer_width, state_ptr->framebuffer_height,
            &state_ptr->skybox_renderpass->targets[i]
        );

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

    initialized = true;
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
    const char* skybox_renderpass_name = "Builtin.RenderpassSkybox";
    const char* world_renderpass_name = "Builtin.RenderpassWorld";
    const char* ui_renderpass_name = "Builtin.RenderpassUI";
    renderpass_config pass_config[3];

    // Skybox: стандартный рендер (хорошо работает с прозрачными объектами).
    // TODO: Заменить на Deferred shading способ.
    pass_config[0].name = skybox_renderpass_name;
    pass_config[0].prev_name = null;
    pass_config[0].next_name = world_renderpass_name;
    pass_config[0].render_area = (vec4){{ 0, 0, state_ptr->framebuffer_width, state_ptr->framebuffer_height }};
    pass_config[0].clear_color = (vec4){{ 0.0f, 0.0f, 0.2f, 1.0f }};
    pass_config[0].clear_flags = RENDERPASS_CLEAR_COLOR_BUFFER_FLAG;
    // World.
    pass_config[1].name = world_renderpass_name;
    pass_config[1].prev_name = skybox_renderpass_name;
    pass_config[1].next_name = ui_renderpass_name;
    pass_config[1].render_area = (vec4){{ 0, 0, state_ptr->framebuffer_width, state_ptr->framebuffer_height }};
    pass_config[1].clear_color = (vec4){{ 0.0f, 0.0f, 0.2f, 1.0f }};
    pass_config[1].clear_flags = RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG | RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG;
    // UI.
    pass_config[2].name = ui_renderpass_name;
    pass_config[2].prev_name = world_renderpass_name;
    pass_config[2].next_name = null;
    pass_config[2].render_area = (vec4){{ 0, 0, state_ptr->framebuffer_width, state_ptr->framebuffer_height }};
    pass_config[2].clear_color = (vec4){{ 0.0f, 0.0f, 0.2f, 1.0f }};
    pass_config[2].clear_flags = RENDERPASS_CLEAR_NONE_FLAG;

    renderer_backend_config renderer_config = {};
    renderer_config.application_name = window_state->title;
    renderer_config.on_rendertarget_refresh_required = regenerate_render_targets;
    renderer_config.renderpass_count = 3;
    renderer_config.pass_configs = pass_config;

    CRITICAL_INIT(
        state_ptr->backend.initialize(&state_ptr->backend, &renderer_config, &state_ptr->window_render_target_count),
        "Renderer backend failed to initialize."
    );

    // TODO: Изменить способ получения.
    state_ptr->skybox_renderpass = state_ptr->backend.renderpass_get(skybox_renderpass_name);
    state_ptr->skybox_renderpass->render_target_count = state_ptr->window_render_target_count;
    state_ptr->skybox_renderpass->targets = kallocate_tc(render_target, state_ptr->window_render_target_count, MEMORY_TAG_ARRAY);
    kzero_tc(state_ptr->skybox_renderpass->targets, render_target, state_ptr->window_render_target_count);

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
    state_ptr->skybox_renderpass->render_area.x = state_ptr->skybox_renderpass->render_area.y = 0;
    state_ptr->skybox_renderpass->render_area.width = state_ptr->framebuffer_width;
    state_ptr->skybox_renderpass->render_area.height = state_ptr->framebuffer_height;

    state_ptr->world_renderpass->render_area.x = state_ptr->world_renderpass->render_area.y = 0;
    state_ptr->world_renderpass->render_area.width = state_ptr->framebuffer_width;
    state_ptr->world_renderpass->render_area.height = state_ptr->framebuffer_height;

    state_ptr->ui_renderpass->render_area.x = state_ptr->ui_renderpass->render_area.y = 0;
    state_ptr->ui_renderpass->render_area.width = state_ptr->framebuffer_width;
    state_ptr->ui_renderpass->render_area.height = state_ptr->framebuffer_height;

    // Шейдеры.
    resource config_resource;
    shader_config* sconfig = null;

    // Шейдер: Skybox.
    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_SKYBOX, RESOURCE_TYPE_SHADER, null, &config_resource),
        "Failed to load builtin skybox shader."
    );

    sconfig = config_resource.data;
    CRITICAL_INIT(shader_system_create(sconfig), "Failed to create builtin skybox shader.");

    resource_system_unload(&config_resource);
    state_ptr->skybox_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_SKYBOX);

    // Шейдер: World.
    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_WORLD, RESOURCE_TYPE_SHADER, null, &config_resource),
        "Failed to load builtin world shader."
    );

    sconfig = config_resource.data;
    CRITICAL_INIT(
        shader_system_create(sconfig),
        "Failed to create builtin world shader."
    );

    resource_system_unload(&config_resource);
    state_ptr->world_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_WORLD);

    // Шейдер: UI.
    CRITICAL_INIT(
        resource_system_load(BUILTIN_SHADER_NAME_UI, RESOURCE_TYPE_SHADER, null, &config_resource),
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
        state_ptr->backend.render_target_destroy(&state_ptr->skybox_renderpass->targets[i], true);
        state_ptr->backend.render_target_destroy(&state_ptr->world_renderpass->targets[i], true);
        state_ptr->backend.render_target_destroy(&state_ptr->ui_renderpass->targets[i], true);
    }
    kfree(state_ptr->skybox_renderpass->targets, MEMORY_TAG_ARRAY);
    kfree(state_ptr->world_renderpass->targets, MEMORY_TAG_ARRAY);
    kfree(state_ptr->ui_renderpass->targets, MEMORY_TAG_ARRAY);

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

bool renderer_shader_create(shader* s, const shader_config* config, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages)
{
    return state_ptr->backend.shader_create(s, config, pass, stage_count, stage_filenames, stages);
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

bool renderer_is_multithreaded()
{
    if(!system_status_valid(__FUNCTION__)) kdebug_break();
    return state_ptr->backend.is_multithreaded();
}

bool renderer_renderbuffer_create(renderbuffer_type type, ptr total_size, bool use_freelist, renderbuffer* out_buffer)
{
    if(!out_buffer)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    // Очистка структуры.
    kzero_tc(out_buffer, struct renderbuffer, 1);

    out_buffer->type = type;
    out_buffer->total_size = total_size;

    // Создание списка свободной памяти буфера.
    if(use_freelist)
    {
        // Получение требований к памяти.
        ptr freelist_memory_requirement = 0;
        freelist_create(total_size, &freelist_memory_requirement, null);

        // Выделение требуемой памяти.
        void* memory = kallocate(freelist_memory_requirement, MEMORY_TAG_RENDERER);

        // Создание списка памяти.
        out_buffer->buffer_freelist = freelist_create(total_size, &freelist_memory_requirement, memory);
        if(!out_buffer->buffer_freelist)
        {
            kerror("Function '%s': Failed to create freelist of buffer.", __FUNCTION__);
            return false;
        }
    }

    // Создание внутренних данных буфера.
    if(!state_ptr->backend.renderbuffer_create(out_buffer))
    {
        kfatal("Function '%s': Unable to create internal data for renderbuffer. Application cannot continue.", __FUNCTION__);
        return false;
    }

    return true;
}

void renderer_renderbuffer_destroy(renderbuffer* buffer)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return;
    }

    // Уничтожение списка свободной памяти буфера.
    if(buffer->buffer_freelist)
    {
        freelist_destroy(buffer->buffer_freelist);
        kfree(buffer->buffer_freelist, MEMORY_TAG_RENDERER); // Т.к. при создании memory == buffer->buffer_freelist.
        buffer->buffer_freelist = null;
    }

    // Уничтожение внутренних данных буфера.
    state_ptr->backend.renderbuffer_destroy(buffer);
    buffer->internal_data = null;
}

bool renderer_renderbuffer_bind(renderbuffer* buffer, ptr offset)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    return state_ptr->backend.renderbuffer_bind(buffer, offset);
}

bool renderer_renderbuffer_unbind(renderbuffer* buffer)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    return state_ptr->backend.renderbuffer_unbind(buffer);
}

void* renderer_renderbuffer_map_memory(renderbuffer* buffer, ptr offset, ptr size)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    return state_ptr->backend.renderbuffer_map_memory(buffer, offset, size);
}

void renderer_renderbuffer_unmap_memory(renderbuffer* buffer, ptr offset, ptr size)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return;
    }

    state_ptr->backend.renderbuffer_unmap_memory(buffer, offset, size);
}

bool renderer_renderbuffer_flush(renderbuffer* buffer, ptr offset, ptr size)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    return state_ptr->backend.renderbuffer_flush(buffer, offset, size);
}

bool renderer_renderbuffer_read(renderbuffer* buffer, ptr offset, ptr size, void* out_memory)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    return state_ptr->backend.renderbuffer_read(buffer, offset, size, out_memory);
}

bool renderer_renderbuffer_resize(renderbuffer* buffer, ptr new_total_size)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    if(new_total_size <= buffer->total_size)
    {
        kerror("Function '%s' requires that new size be larger than the old, because this can lead to data loss.", __FUNCTION__);
        return false;
    }

    if(buffer->buffer_freelist)
    {
        if(!freelist_resize(buffer->buffer_freelist, new_total_size))
        {
            kerror("Function '%s': Failed to reszie internal freelist.", __FUNCTION__);
            return false;
        }
    }

    bool result = state_ptr->backend.renderbuffer_resize(buffer, new_total_size);
    if(result)
    {
        buffer->total_size = new_total_size;
    }
    else
    {
        kerror("Function '%s': Failed to resize internal buffer.", __FUNCTION__);
    }

    return result;
}

bool renderer_renderbuffer_allocate(renderbuffer* buffer, ptr size, ptr* out_offset)
{
    if(!buffer || !buffer->internal_data || !out_offset)
    {
        kerror("Function '%s' requires a valid buffer pointer and out_offset pointer.", __FUNCTION__);
        return false;
    }

    if(!size)
    {
        kerror("Function '%s' requires a size greater than zero.", __FUNCTION__);
        return false;
    }

    if(!buffer->buffer_freelist)
    {
        kwarng("Function '%s' called on a buffer not using freelist. Offset will not be valid. Call 'renderer_renderbuffer_load_range' instead.", __FUNCTION__);
        *out_offset = 0;
        return true;
    }

    return freelist_allocate_block(buffer->buffer_freelist, size, out_offset);
}

bool renderer_renderbuffer_free(renderbuffer* buffer, ptr size, ptr offset)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    if(!size)
    {
        kerror("Function '%s' requires a size greater than zero.", __FUNCTION__);
        return false;
    }

    if(!buffer->buffer_freelist)
    {
        kwarng("Function '%s' called on a buffer not using freelist. Nothing was done.", __FUNCTION__);
        return true;
    }

    return freelist_free_block(buffer->buffer_freelist, size, offset);
}

bool renderer_renderbuffer_load_range(renderbuffer* buffer, ptr offset, ptr size, const void* data)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    return state_ptr->backend.renderbuffer_load_range(buffer, offset, size, data);
}

bool renderer_renderbuffer_copy_range(renderbuffer* src, ptr src_offset, renderbuffer* dest, ptr dest_offset, ptr size)
{
    if(!src || !src->internal_data || !dest || !dest->internal_data)
    {
        kerror("Function '%s' requires a valid source buffer pointer and destination buffer.", __FUNCTION__);
        return false;
    }

    if(!size)
    {
        kerror("Function '%s' requires a size greater than zero.", __FUNCTION__);
        return false;
    }

    return state_ptr->backend.renderbuffer_copy_range(src, dest_offset, dest, dest_offset, size);
}

bool renderer_renderbuffer_draw(renderbuffer* buffer, ptr offset, u32 element_count, bool bind_only)
{
    if(!buffer || !buffer->internal_data)
    {
        kerror("Function '%s' requires a valid buffer pointer.", __FUNCTION__);
        return false;
    }

    if(!element_count)
    {
        kerror("Function '%s' requires a number of elements greater than zero.", __FUNCTION__);
        return false;
    }

    return state_ptr->backend.renderbuffer_draw(buffer, offset, element_count, bind_only);
}
