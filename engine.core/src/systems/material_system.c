// Собственные подключения.
#include "systems/material_system.h"
#include "systems/texture_system.h"
#include "systems/resource_system.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "containers/hashtable.h"
#include "math/kmath.h"
#include "renderer/renderer_frontend.h"

typedef struct material_system_state {
    // @brief Конфигурация системы.
    material_system_config config;
    // @brief Материал по умолчанию.
    material default_material;
    // @brief Массив материалов.
    material* materials;
    // @brief Таблица ссылок на материалы.
    hashtable* material_references_table;
} material_system_state;

typedef struct material_reference {
    // @brief Количество ссылок на материал.
    u64 reference_count;
    // @brief Индекс материала в массиве материалов.
    u32 index;
    // @brief Авто уничтожение материала.
    bool auto_release;
} material_reference;

static material_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the material system to be initialized. Call 'material_system_initialize' first.";

bool default_materials_create();
void default_materials_destroy();
bool material_load(material_config* config, material* m);
void material_destroy(material* m);

bool material_system_initialize(u64* memory_requirement, void* memory, material_system_config* config)
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

    if(!config->max_material_count)
    {
        kerror("Function '%s': config.max_material_count must be greater then zero. Return false!", __FUNCTION__);
        return false;
    }

    hashtable_config hconf;
    hconf.data_size = sizeof(material_reference);
    hconf.entry_count = config->max_material_count;

    u64 state_requirement = sizeof(material_system_state);
    u64 materials_requirement = sizeof(material) * config->max_material_count;
    u64 hashtable_requirement = 0;
    hashtable_create(&hashtable_requirement, null, &hconf, null);
    *memory_requirement = state_requirement + materials_requirement + hashtable_requirement;

    if(!memory)
    {
        return true;
    }

    // Обнуление заголовка системы материалов.
    kzero_tc(memory, material_system_state, 1);
    state_ptr = memory;

    // Запись данных конфигурации системы.
    state_ptr->config.max_material_count = config->max_material_count;

    // Получение и запись указателя на блок материалов.
    void* materials_block = (void*)((u8*)state_ptr + state_requirement);
    state_ptr->materials = materials_block;

    // Получение и запись указателя на хэш-таблицу.
    void* hashtable_block = (void*)((u8*)materials_block + materials_requirement);
    if(!hashtable_create(&hashtable_requirement, hashtable_block, &hconf, &state_ptr->material_references_table))
    {
        kerror("Function '%s': Failed to create hashtable of references to materials.", __FUNCTION__);
        return false;
    }

    // Отмечает все материалы как недействительные.
    for(u32 i = 0; i < state_ptr->config.max_material_count; ++i)
    {
        state_ptr->materials[i].id = INVALID_ID;
        state_ptr->materials[i].generation = INVALID_ID;
        state_ptr->materials[i].internal_id = INVALID_ID;
    }

    // Создание материала по-умолчанию.
    if(!default_materials_create())
    {
        kerror("Function '%s': Failed to create default materials.", __FUNCTION__);
        return false;
    }

    return true;
}

void material_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // Уничтожение хэш-таблицы.
    hashtable_destroy(state_ptr->material_references_table);

    // Уничтожение всех созданых материалов.
    for(u32 i = 0; i < state_ptr->config.max_material_count; ++i)
    {
        material* m = &state_ptr->materials[i];
        if(m->id != INVALID_ID)
        {
            material_destroy(m);
        }
    }

    // Уничтожение материалов по-умолчанию.
    default_materials_destroy();

    state_ptr = null;
}

material* material_system_acquire(const char* name)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return null;
    }

    // Загрузка конфигурации материала.
    resource material_resource;
    if(!resource_system_load(name, RESOURCE_TYPE_MATERIAL, &material_resource))
    {
        kerror("Function '%s': Failed to load material resource '%s'. Return null!", __FUNCTION__, name);
        return null;
    }

    material* m = null;
    if(material_resource.data)
    {
        m = material_system_acquire_from_config(material_resource.data);
    }

    resource_system_unload(&material_resource);

    if(!m)
    {
        kerror("Function '%s': Failed to load material resource '%s'. Return null!", __FUNCTION__, name);
    }

    return m;
}

material* material_system_acquire_from_config(material_config* config)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return null;
    }

    if(string_equali(config->name, DEFAULT_MATERIAL_NAME))
    {
        return &state_ptr->default_material;
    }

    material_reference ref;
    if(!hashtable_get(state_ptr->material_references_table, config->name, &ref) || ref.index == INVALID_ID)
    {
        ref.reference_count = 0;
        ref.auto_release = config->auto_release;
        ref.index = INVALID_ID;
    }

    ref.reference_count++;

    if(ref.index == INVALID_ID)
    {
        // Поиск свободной памяти для материала.
        for(u32 i = 0; i < state_ptr->config.max_material_count; ++i)
        {
            if(state_ptr->materials[i].id == INVALID_ID)
            {
                ref.index = i;
                break;
            }
        }

        // Если свободный участок памяти не найден.
        if(ref.index == INVALID_ID)
        {
            kerror(
                "Function '%s': Material system cannot hold anymore materials. Adjust configuration to allow more.",
                __FUNCTION__
            );
            return null;
        }

        material* m = &state_ptr->materials[ref.index];
        m->id = ref.index;

        // Создание материала.
        if(!material_load(config, m))
        {
            kerror("Function '%s': Failed to load material '%s'.", __FUNCTION__, config->name);
            return null;
        }

        // TODO: Сомнительно!!
        if(m->generation == INVALID_ID)
        {
            kdebug("MARETIAL generation INVALID_ID!");
            m->generation = 0;
        }
        else
        {
            kdebug("MARETIAL generation %u!", m->generation);
            m->generation++;
        }

        ktrace(
            "Function '%s': Material '%s' does not exist. Created, and reference count is now %i.",
            __FUNCTION__, config->name, ref.reference_count
        );
    }
    else
    {
        ktrace(
            "Function '%s': Material '%s' already exists, and reference count increased to %i.",
            __FUNCTION__, config->name, ref.reference_count
        );
    }

    // Обновление ссылки на материал.
    if(!hashtable_set(state_ptr->material_references_table, config->name, &ref, true))
    {
        kerror("Function '%s' Failed to update material reference.", __FUNCTION__);
        return null;
    }

    return &state_ptr->materials[ref.index];
}

void material_system_release(const char* name)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // Игнорирование удаления материала по умолчанию.
    if(string_equali(name, DEFAULT_MATERIAL_NAME))
    {
        return;
    }

    material_reference ref;
    if(!hashtable_get(state_ptr->material_references_table, name, &ref) || ref.reference_count == 0)
    {
        kwarng("Function '%s': Tried to release non-existent material '%s'.", __FUNCTION__, name);
        return;
    }

    ref.reference_count--;

    if(ref.reference_count == 0 && ref.auto_release)
    {
        material* m = &state_ptr->materials[ref.index];

        // Освобождение/восстановление памяти материала для нового.
        material_destroy(m);

        // Освобождение ссылки.
        ref.index = INVALID_ID;
        ref.auto_release = false;

        ktrace(
            "Function '%s': Released material '%s', because reference count is 0 and auto release used.",
            __FUNCTION__, name
        );
    }
    else
    {
        ktrace(
            "Function '%s': Released material '%s', now has a reference count is %u and auto release is %s.",
            __FUNCTION__, name, ref.reference_count, ref.auto_release ? "used" : "unused"
        );
    }

    // Обновление ссылки на материал.
    if(!hashtable_set(state_ptr->material_references_table, name, &ref, true))
    {
        kerror("Function '%s' Failed to update material reference.", __FUNCTION__);
    }
}

material* material_system_get_default()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return null;
    }

    return &state_ptr->default_material;
}

bool default_materials_create()
{
    kzero_tc(&state_ptr->default_material, material, 1);
    state_ptr->default_material.id = INVALID_ID;
    state_ptr->default_material.generation = INVALID_ID;

    string_ncopy(state_ptr->default_material.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state_ptr->default_material.diffuse_color = vec4_one();    // Белый цвет.
    state_ptr->default_material.diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
    state_ptr->default_material.diffuse_map.texture = texture_system_get_default_texture();

    if(!renderer_create_material(&state_ptr->default_material))
    {
        kerror("Function '%s': Failed to acquire renderer resource for default material.", __FUNCTION__);
        return false;
    }

    return true;
}

void default_materials_destroy()
{
    renderer_destroy_material(&state_ptr->default_material);
}

bool material_load(material_config* config, material* m)
{
    kzero_tc(m, material, 1);

    // Копирование имени материала.
    string_ncopy(m->name, config->name, MATERIAL_NAME_MAX_LENGTH);

    // Тип.
    m->type = config->type;

    // Цвет.
    m->diffuse_color = config->diffuse_color;

    if(string_length(config->diffuse_map_name) > 0)
    {
        m->diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
        m->diffuse_map.texture = texture_system_acquire(config->diffuse_map_name, config->auto_release);

        if(!m->diffuse_map.texture)
        {
            kwarng(
                "Function '%s': Unable to load texture '%s' for material '%s', using default.",
                __FUNCTION__, config->diffuse_map_name, m->name
            );
            m->diffuse_map.texture = texture_system_get_default_texture();
        }
    }
    else
    {
        m->diffuse_map.use = TEXTURE_USE_UNKNOWN;
        m->diffuse_map.texture = null;
    }

    // TODO: другие разметки.

    // Загрузка в графический процессор.
    if(!renderer_create_material(m))
    {
        kerror("Function '%s': Failed to acquire renderer resource for material '%s'.", __FUNCTION__, m->name);
        return false;
    }

    return true;
}

void material_destroy(material* m)
{
    // Удаление загруженой текстуры.
    if(m->diffuse_map.texture)
    {
        texture_system_release(m->diffuse_map.texture->name);
    }

    // Удаление из памяти графического процессора.
    renderer_destroy_material(m);

    // Освобождение памяти для нового материала.
    kzero_tc(m, material, 1);
    m->id = INVALID_ID;
    m->generation = INVALID_ID;
    m->internal_id = INVALID_ID;
}
