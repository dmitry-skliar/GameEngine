// Собственные подключения.
#include "systems/resource_system.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"

// Известые загрузчики ресурсов.
#include "resources/loaders/image_loader.h"
#include "resources/loaders/material_loader.h"
#include "resources/loaders/binary_loader.h"
#include "resources/loaders/text_loader.h"
#include "resources/loaders/shader_loader.h"

typedef struct resource_system_state {
    resource_system_config config;
    resource_loader* loaders;
} resource_system_state;

static resource_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the resource system to be initialized. Call 'resource_system_initialize' first.";

bool load(const char* name, resource_loader* loader, resource* out_resource);

bool resource_system_initialize(u64* memory_requirement, void* memory, resource_system_config* config)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once! Return false!", __FUNCTION__);
        return false;
    }

    if(!memory_requirement || !config)
    {
        kerror("Function '%s' requires a valid pointers to memory_requirement and config. Return false!", __FUNCTION__);
        return false;
    }

    if(!config->max_loader_count)
    {
        kerror("Function '%s': config.max_loader_count must be greater then zero. Return false!", __FUNCTION__);
        return false;
    }

    u64 state_requirement = sizeof(resource_system_state);
    u64 loaders_requirement = sizeof(resource_loader) * config->max_loader_count;
    *memory_requirement = state_requirement + loaders_requirement;

    if(!memory)
    {
        return true;
    }

    // Обнуление заголовка системы материалов.
    kzero_tc(memory, resource_system_state, 1);
    state_ptr = memory;

    // Запись данных конфигурации системы.
    state_ptr->config.max_loader_count = config->max_loader_count;
    state_ptr->config.asset_base_path = config->asset_base_path;

    // Получение и запись указателя на блок загрузчиков.
    void* loaders_block = (void*)((u8*)state_ptr + state_requirement);
    state_ptr->loaders = loaders_block;

    // Отмечает все геометрии как недействительные.
    for(u32 i = 0; i < state_ptr->config.max_loader_count; ++i)
    {
        state_ptr->loaders[i].id = INVALID_ID;
    }

    // NOTE: Автоматическая регистрация известных типов загрузчиков здесь.
    resource_system_register_loader(image_resource_loader_create());
    resource_system_register_loader(material_resource_loader_create());
    resource_system_register_loader(binary_resource_loader_create());
    resource_system_register_loader(text_resource_loader_create());
    resource_system_register_loader(shader_resource_loader_create());

    return true;
}

void resource_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // NOTE: Нечего уничтожать.

    state_ptr = null;
}

KAPI bool resource_system_register_loader(resource_loader loader)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }

    u32 empty_slot_id = INVALID_ID;
    resource_loader* empty_slot = null;

    for(u32 i = 0; i < state_ptr->config.max_loader_count; ++i)
    {
        resource_loader* l = &state_ptr->loaders[i];

        if(l->id != INVALID_ID)
        {
            if(l->type == loader.type)
            {
                // TODO: Для типового загрузчика выводить имя вместо цифры!
                kerror(
                    "Function '%s': Loader of type %d already exists and will not be registered.",
                    __FUNCTION__, loader.type
                );
                return false;
            }
            else if(loader.custom_type && string_length(loader.custom_type) > 0
                 && string_equali(l->custom_type, loader.custom_type))
            {
                kerror(
                    "Function '%s': Loader of custom type %s already exists and will not be registered.",
                    __FUNCTION__, loader.custom_type
                );
                return false;
            }
        }

        if(!empty_slot && l->id == INVALID_ID)
        {
            empty_slot = l;
            empty_slot_id = i;
        }
    }

    *empty_slot = loader; // Копирование структуры!
    empty_slot->id = empty_slot_id;
    return true;
}

KAPI bool resource_system_load(const char* name, resource_type type, resource* out_resource)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }

    if(type != RESOURCE_TYPE_CUSTOM)
    {
        for(u32 i = 0; i < state_ptr->config.max_loader_count; ++i)
        {
            resource_loader* l = &state_ptr->loaders[i];
            if(l->id != INVALID_ID && l->type == type)
            {
                return load(name, l, out_resource);
            }
        }
    }

    kerror("Function '%s': No loader for type %d was found.", __FUNCTION__, type);
    out_resource->loader_id = INVALID_ID;
    return false;
}

KAPI bool resource_system_load_custom(const char* name, const char* custom_type, resource* out_resource)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }

    if(custom_type && string_length(custom_type) > 0)
    {
        for(u32 i = 0; i < state_ptr->config.max_loader_count; ++i)
        {
            resource_loader* l = &state_ptr->loaders[i];
            if(l->id != INVALID_ID && l->type == RESOURCE_TYPE_CUSTOM && string_equali(l->custom_type, custom_type))
            {
                return load(name, l, out_resource);
            }
        }
    }

    kerror("Function '%s': No loader for custom type %s was found.", __FUNCTION__, custom_type);
    out_resource->loader_id = INVALID_ID;
    return false;
}

KAPI void resource_system_unload(resource* resource)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    if(resource && resource->loader_id != INVALID_ID)
    {
        resource_loader* l = &state_ptr->loaders[resource->loader_id];

        if(l->id != INVALID_ID && l->unload)
        {
            l->unload(l, resource);
        }
    }
}

KAPI const char* resource_system_base_path()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return "";
    }

    return state_ptr->config.asset_base_path;
}

bool load(const char* name, resource_loader* loader, resource* out_resource)
{
    if(!name || !loader || !out_resource)
    {
        out_resource->loader_id = INVALID_ID;
        return false;
    }

    // ATTENTION: В функции load() не затереть loader_id!
    out_resource->loader_id = loader->id;
    return loader->load(loader, name, out_resource);
}
