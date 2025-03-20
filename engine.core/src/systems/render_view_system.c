// Собственные подключения.
#include "systems/render_view_system.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "containers/hashtable.h"
#include "renderer/renderer_frontend.h"

// TODO: Временно - сделать фабрику и регистрировать вместо этого.
#include "renderer/views/render_view_world.h"
#include "renderer/views/render_view_ui.h"

typedef struct render_view_system_state {
    render_view_system_config config;
    render_view* views;
    hashtable* lookup;
} render_view_system_state;

static render_view_system_state* state_ptr = null;

static bool system_status_valid(const char* func_name)
{
    if(!state_ptr)
    {
        if(func_name)
        {
            kerror(
                "Function '%s' requires the render view system to be initialized. Call 'render_view_system_initialize' first.",
                func_name
            );
        }
        return false;
    }
    return true;
}

bool render_view_system_initialize(u64* memory_requirement, void* memory, render_view_system_config* config)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once!", __FUNCTION__);
        return false;
    }

    if(!memory_requirement || !config)
    {
        kerror("Function '%s' requires a valid pointers to memory_requirement and config.", __FUNCTION__);
        return false;
    }

    if(!config->max_view_count)
    {
        kerror("Function '%s': config.max_view_count must be greater then zero.", __FUNCTION__);
        return false;
    }

    u64 state_requirement = sizeof(render_view_system_state);
    u64 array_requirement = sizeof(render_view) * config->max_view_count;
    u64 hashtable_requirement = 0;
    hashtable_config hcfg = { sizeof(u16), config->max_view_count };
    hashtable_create(&hashtable_requirement, null, &hcfg, null);
    *memory_requirement = state_requirement + array_requirement + hashtable_requirement;

    if(!memory)
    {
        return true;
    }

    // Обнуление заголовка системы камер.
    kzero_tc(memory, render_view_system_state, 1);
    state_ptr = memory;

    // Запись данных конфигурации системы.
    state_ptr->config = *config;

    // Получение и запись указателя на блок камер.
    void* array_block = POINTER_GET_OFFSET(state_ptr, state_requirement);
    state_ptr->views = array_block;

    // Получение и запись указателя на хэш-таблицу.
    void* hashtable_block = POINTER_GET_OFFSET(array_block, array_requirement);
    if(!hashtable_create(&hashtable_requirement, hashtable_block, &hcfg, &state_ptr->lookup))
    {
        kerror("Function '%s': Failed to create hashtable of ids to render view.", __FUNCTION__);
        return false;
    }

    // Отмечает все камеры как недействительные.
    for(u32 i = 0; i < state_ptr->config.max_view_count; ++i)
    {
        state_ptr->views[i].id = INVALID_ID_U16;
    }

    return true;
}

void render_view_system_shutdown()
{
    if(!system_status_valid(__FUNCTION__)) return;
    hashtable_destroy(state_ptr->lookup);

    for(u32 i = 0; i < state_ptr->config.max_view_count; ++i)
    {
        if(state_ptr->views[i].id != INVALID_ID_U16)
        {
            kfree_tc(state_ptr->views[i].passes, renderpass*, state_ptr->views[i].renderpass_count, MEMORY_TAG_ARRAY);
            state_ptr->views[i].on_destroy(&state_ptr->views[i]);
        }
    }
    
    state_ptr = null;
}

bool render_view_system_create(render_view_config* config)
{
    if(!system_status_valid(__FUNCTION__)) return false;

    if(!config)
    {
        kerror("Function '%s' requires a valid pointer to a config.", __FUNCTION__);
        return false;
    }

    if(config->pass_count < 1)
    {
        kerror("Function '%s': Config must have at least one renderpass.", __FUNCTION__);
        return false;
    }

    u16 id = INVALID_ID_U16;
    if(!hashtable_get(state_ptr->lookup, config->name, &id) || id == INVALID_ID_U16)
    {
        // Поиск свободного слота памяти.
        for(u16 i = 0; i < state_ptr->config.max_view_count; ++i)
        {
            if(state_ptr->views[i].id == INVALID_ID_U16)
            {
                id = i;
                break;
            }
        }

        if(id == INVALID_ID_U16)
        {
            kerror("Function '%s' failed to find new slot. Adjust render view system config to allow more.", __FUNCTION__);
            return null;
        }

        render_view* view = &state_ptr->views[id];
        view->id = id;
        view->type = config->type;
        view->custom_shader_name = config->custom_shader_name;
        view->renderpass_count = config->pass_count;
        view->passes = kallocate_tc(renderpass*, view->renderpass_count, MEMORY_TAG_ARRAY);

        for(u32 i = 0; i < view->renderpass_count; ++i)
        {
            view->passes[i] = renderer_renderpass_get(config->passes[i].name);
            if(!view->passes[i])
            {
                kerror("Function '%s': Renderpass not found: '%s'.", __FUNCTION__, config->passes[i].name);
                return false;
            }
        }

        // TODO: Временно!
        if(config->type == RENDERER_VIEW_KNOWN_TYPE_WORLD)
        {
            view->on_build_packet = render_view_world_on_build_packet;
            view->on_render       = render_view_world_on_render;
            view->on_create       = render_view_world_on_create;
            view->on_destroy      = render_view_world_on_destroy;
            view->on_resize       = render_view_world_on_resize;
        }
        else if(config->type == RENDERER_VIEW_KNOWN_TYPE_UI)
        {
            view->on_build_packet = render_view_ui_on_build_packet;
            view->on_render       = render_view_ui_on_render;
            view->on_create       = render_view_ui_on_create;
            view->on_destroy      = render_view_ui_on_destroy;
            view->on_resize       = render_view_ui_on_resize;
        }

        // Создание view и обновление записи в таблице.
        if(!view->on_create(view) || !hashtable_set(state_ptr->lookup, config->name, &id, true))
        {
            kerror("Function '%s': Failed to create view or update lookup table.", __FUNCTION__);
            kfree_tc(view->passes, renderpass*, view->renderpass_count, MEMORY_TAG_ARRAY);
            state_ptr->views[id].id = INVALID_ID_U16;
            return false;
        }

        return true;
    }

    kerror("Function '%s': A view named '%s' already exists. A new one will not be created.", __FUNCTION__, config->name);
    return false;
}

void render_view_system_on_window_resize(u32 width, u32 height)
{
    if(!system_status_valid(__FUNCTION__)) return;

    for(u32 i = 0; i < state_ptr->config.max_view_count; ++i)
    {
        if(state_ptr->views[i].id != INVALID_ID_U16)
        {
            state_ptr->views[i].on_resize(&state_ptr->views[i], width, height);
        }
    }
}

render_view* render_view_system_get(const char* name)
{
    if(!system_status_valid(__FUNCTION__)) return null;

    u16 id = INVALID_ID_U16;
    if(!hashtable_get(state_ptr->lookup, name, &id) || id == INVALID_ID_U16)
    {
        kwarng("Function '%s': Tri to get non-exists view named '%s'.", __FUNCTION__, name);
        return null;
    }

    return &state_ptr->views[id];
}

bool render_view_system_build_packet(render_view* view, void* data, render_view_packet* out_packet)
{
    if(!system_status_valid(__FUNCTION__)) return false;

    if(!out_packet)
    {
        kerror("Function '%s' requires a valid pointer to a packet.", __FUNCTION__);
        return false;
    }

    return view->on_build_packet(view, data, out_packet);
}

bool render_view_system_on_render(render_view* view, render_view_packet* packet, u64 frame_number, u64 render_target_index)
{
    if(!system_status_valid(__FUNCTION__)) return false;

    if(!packet)
    {
        kerror("Function '%s' requires a valid pointer to a packet.", __FUNCTION__);
        return false;
    }

    return view->on_render(view, packet, frame_number, render_target_index);
}
