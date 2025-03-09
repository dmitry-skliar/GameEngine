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

typedef struct material_shader_uniform_locations {
    u16 projection;
    u16 view;
    u16 view_position;
    u16 shininess;
    u16 ambient_color;
    u16 diffuse_color;
    u16 diffuse_texture;
    u16 specular_texture;
    u16 normal_texture;
    u16 model;
    u16 render_mode;
} material_shader_uniform_locations;

typedef struct ui_shader_uniform_locations {
    u16 projection;
    u16 view;
    u16 diffuse_color;
    u16 diffuse_texture;
    u16 model;
} ui_shader_uniform_locations;

typedef struct material_system_state {
    // Конфигурация системы.
    material_system_config config;
    // Материал по умолчанию.
    material default_material;
    // Массив материалов.
    material* materials;
    // Таблица ссылок на материалы.
    hashtable* material_references_table;
    // Местоположение для материала шейдера и идентификатор шейдера.
    material_shader_uniform_locations material_locations;
    u32 material_shader_id;
    // Местоположение для ui шейдера и идентификатор шейдера.
    ui_shader_uniform_locations ui_locations;
    u32 ui_shader_id;
} material_system_state;

typedef struct material_reference {
    // Количество ссылок на материал.
    u64 reference_count;
    // Индекс материала в массиве материалов.
    u32 index;
    // Авто уничтожение материала.
    bool auto_release;
} material_reference;

static material_system_state* state_ptr = null;

#define MATERIAL_APPLY_OR_FAIL(expr)                  \
    if (!expr) {                                      \
        kerror("Failed to apply material: %s", expr); \
        return false;                                 \
    }

bool material_system_status_valid(const char* func_name)
{
    if(!state_ptr)
    {
        if(func_name)
        {
            kerror(
                "Function '%s' requires the material system to be initialized. Call 'material_system_initialize' first.",
                func_name
            );
            
        }
        return false;
    }
    return true;
}

bool default_materials_create();
void default_materials_destroy();
bool material_load(material_config* config, material* m);
void material_destroy(material* m);

bool material_system_initialize(u64* memory_requirement, void* memory, material_system_config* config)
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

    if(!config->max_material_count)
    {
        kerror("Function '%s': config.max_material_count must be greater then zero.", __FUNCTION__);
        return false;
    }

    // TODO: Исключить повторную обработку, поместив в if(!memory)...
    u64 state_requirement = sizeof(material_system_state);
    u64 materials_requirement = sizeof(material) * config->max_material_count;
    u64 hashtable_requirement = 0;
    hashtable_config hconf = { sizeof(material_reference), config->max_material_count };
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
    state_ptr->config = *config;

    state_ptr->material_shader_id = INVALID_ID;
    state_ptr->material_locations.projection = INVALID_ID_U16;
    state_ptr->material_locations.view = INVALID_ID_U16;
    state_ptr->material_locations.view_position = INVALID_ID_U16;
    state_ptr->material_locations.shininess = INVALID_ID_U16;
    state_ptr->material_locations.ambient_color = INVALID_ID_U16;
    state_ptr->material_locations.diffuse_color = INVALID_ID_U16;
    state_ptr->material_locations.diffuse_texture = INVALID_ID_U16;
    state_ptr->material_locations.specular_texture = INVALID_ID_U16;
    state_ptr->material_locations.normal_texture = INVALID_ID_U16;
    state_ptr->material_locations.model = INVALID_ID_U16;
    state_ptr->material_locations.render_mode = INVALID_ID_U16;

    state_ptr->ui_shader_id = INVALID_ID;
    state_ptr->ui_locations.projection = INVALID_ID_U16;
    state_ptr->ui_locations.view = INVALID_ID_U16;
    state_ptr->ui_locations.diffuse_color = INVALID_ID_U16;
    state_ptr->ui_locations.diffuse_texture = INVALID_ID_U16;
    state_ptr->ui_locations.model = INVALID_ID_U16;

    // Получение и запись указателя на блок материалов.
    void* materials_block = POINTER_GET_OFFSET(state_ptr, state_requirement);
    state_ptr->materials = materials_block;

    // Получение и запись указателя на хэш-таблицу.
    void* hashtable_block = POINTER_GET_OFFSET(materials_block, materials_requirement);
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
        state_ptr->materials[i].render_frame_number = INVALID_ID;
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
    if(!material_system_status_valid(__FUNCTION__))
    {
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
    if(!material_system_status_valid(__FUNCTION__))
    {
        return null;
    }

    // Загрузка конфигурации материала.
    resource material_resource;
    if(!resource_system_load(name, RESOURCE_TYPE_MATERIAL, &material_resource))
    {
        kerror("Function '%s': Failed to load material resource '%s'.", __FUNCTION__, name);
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
        kerror("Function '%s': Failed to load material resource '%s'.", __FUNCTION__, name);
    }

    return m;
}

material* material_system_acquire_from_config(material_config* config)
{
    if(!material_system_status_valid(__FUNCTION__))
    {
        return null;
    }

    if(string_equali(config->name, DEFAULT_MATERIAL_NAME))
    {
        return &state_ptr->default_material;
    }

    // TODO: Может возникнуть когда количество записей в таблице закончится!
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

        shader* s = shader_system_get_by_id(m->shader_id);

        // Сохранение местоположения известных типов для быстрого поиска.
        if(state_ptr->material_shader_id == INVALID_ID && string_equal(config->shader_name, BUILTIN_SHADER_NAME_MATERIAL))
        {
            state_ptr->material_shader_id = s->id;
            state_ptr->material_locations.projection = shader_system_uniform_index(s, "projection");
            state_ptr->material_locations.view = shader_system_uniform_index(s, "view");
            state_ptr->material_locations.view_position = shader_system_uniform_index(s, "view_position");
            state_ptr->material_locations.shininess = shader_system_uniform_index(s, "shininess");
            state_ptr->material_locations.ambient_color = shader_system_uniform_index(s, "ambient_color");
            state_ptr->material_locations.diffuse_color = shader_system_uniform_index(s, "diffuse_color");
            state_ptr->material_locations.diffuse_texture = shader_system_uniform_index(s, "diffuse_texture");
            state_ptr->material_locations.specular_texture = shader_system_uniform_index(s, "specular_texture");
            state_ptr->material_locations.normal_texture = shader_system_uniform_index(s, "normal_texture");
            state_ptr->material_locations.model = shader_system_uniform_index(s, "model");
            state_ptr->material_locations.render_mode = shader_system_uniform_index(s, "mode");
        }
        else if(state_ptr->ui_shader_id == INVALID_ID && string_equal(config->shader_name, BUILTIN_SHADER_NAME_UI))
        {
            state_ptr->ui_shader_id = s->id;
            state_ptr->ui_locations.projection = shader_system_uniform_index(s, "projection");
            state_ptr->ui_locations.view = shader_system_uniform_index(s, "view");
            state_ptr->ui_locations.diffuse_color = shader_system_uniform_index(s, "diffuse_color");
            state_ptr->ui_locations.diffuse_texture = shader_system_uniform_index(s, "diffuse_texture");
            state_ptr->ui_locations.model = shader_system_uniform_index(s, "model");
        }

        if(m->generation == INVALID_ID)
        {
            m->generation = 0;
        }
        else
        {
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

    // TODO: Для hastable сделать hashtable_update которая обновляет!
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
    if(!material_system_status_valid(__FUNCTION__))
    {
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
    if(!material_system_status_valid(__FUNCTION__))
    {
        return null;
    }

    return &state_ptr->default_material;
}

bool material_system_apply_global(u32 shader_id, const mat4* projection, const mat4* view, const vec3* view_position, const vec4* ambient_color, u32 render_mode)
{
    if(!material_system_status_valid(__FUNCTION__))
    {
        return null;
    }

    if(shader_id == state_ptr->material_shader_id)
    {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.projection, projection));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.view, view));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.view_position, view_position));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.ambient_color, ambient_color));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.render_mode, &render_mode));
    }
    else if(shader_id == state_ptr->ui_shader_id)
    {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.projection, projection));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.view, view));
    }
    else
    {
        kerror("Function '%s': Unrecognized shader id '%d'.", __FUNCTION__, shader_id);
        return false;
    }

    MATERIAL_APPLY_OR_FAIL(shader_system_apply_global());
    return true;
}

bool material_system_apply_instance(material* m, bool needs_update)
{
    if(!material_system_status_valid(__FUNCTION__))
    {
        return null;
    }

    MATERIAL_APPLY_OR_FAIL(shader_system_bind_instance(m->internal_id));

    if(needs_update)
    {
        if(m->shader_id == state_ptr->material_shader_id)
        {
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.shininess, &m->shininess));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.diffuse_color, &m->diffuse_color));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.diffuse_texture, m->diffuse_map.texture));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.specular_texture, m->specular_map.texture));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->material_locations.normal_texture, m->normal_map.texture));
        }
        else if(m->shader_id == state_ptr->ui_shader_id)
        {
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.diffuse_color, &m->diffuse_color));
            MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_index(state_ptr->ui_locations.diffuse_texture, m->diffuse_map.texture));
        }
        else
        {
            kerror("Function '%s': Unrecognized shader id '%d' on shader '%s'.", __FUNCTION__, m->shader_id, m->name);
            return false;
        }
    }

    MATERIAL_APPLY_OR_FAIL(shader_system_apply_instance(needs_update));
    return true;
}

bool material_system_apply_local(material* m, const mat4* model)
{
    if(!material_system_status_valid(__FUNCTION__))
    {
        return null;
    }

    if(m->shader_id == state_ptr->material_shader_id)
    {
        return shader_system_uniform_set_by_index(state_ptr->material_locations.model, model);
    }
    else if(m->shader_id == state_ptr->ui_shader_id)
    {
        return shader_system_uniform_set_by_index(state_ptr->ui_locations.model, model);
    }

    kerror("Function '%s': Unrecognized shader id '%d'", __FUNCTION__, m->shader_id);
    return false;
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

    state_ptr->default_material.specular_map.use = TEXTURE_USE_MAP_SPECULAR;
    state_ptr->default_material.specular_map.texture = texture_system_get_default_specular_texture();

    state_ptr->default_material.normal_map.use = TEXTURE_USE_MAP_NORMAL;
    state_ptr->default_material.normal_map.texture = texture_system_get_default_normal_texture();

    shader* s = shader_system_get(BUILTIN_SHADER_NAME_MATERIAL);
    if(!renderer_shader_acquire_instance_resources(s, &state_ptr->default_material.internal_id))
    {
        kerror("Function '%s': Failed to acquire renderer resource for default material.", __FUNCTION__);
        return false;
    }

    state_ptr->default_material.shader_id = s->id;

    return true;
    
}

void default_materials_destroy()
{
    shader* s = shader_system_get(BUILTIN_SHADER_NAME_MATERIAL);
    renderer_shader_release_instance_resources(s, state_ptr->default_material.internal_id);
}

bool material_load(material_config* config, material* m)
{
    kzero_tc(m, material, 1);

    // Копирование имени материала.
    string_ncopy(m->name, config->name, MATERIAL_NAME_MAX_LENGTH);

    m->shader_id = shader_system_get_id(config->shader_name);
    m->diffuse_color = config->diffuse_color;
    m->shininess = config->shininess;

    // Diffuse.
    texture_map* diff_map = &m->diffuse_map;
    if(string_length(config->diffuse_map_name) > 0)
    {
        diff_map->use = TEXTURE_USE_MAP_DIFFUSE;
        diff_map->texture = texture_system_acquire(config->diffuse_map_name, config->auto_release);

        if(!diff_map->texture)
        {
            kwarng(
                "Function '%s': Unable to load diffuse texture '%s' for material '%s', using default.",
                __FUNCTION__, config->diffuse_map_name, m->name
            );
            diff_map->texture = texture_system_get_default_texture();
        }
    }
    else
    {
        diff_map->use = TEXTURE_USE_MAP_DIFFUSE;
        diff_map->texture = texture_system_get_default_texture();
    }

    // Specular.
    texture_map* spec_map = &m->specular_map;
    if(string_length(config->specular_map_name) > 0)
    {
        spec_map->use = TEXTURE_USE_MAP_SPECULAR;
        spec_map->texture = texture_system_acquire(config->specular_map_name, config->auto_release);

        if(!spec_map->texture)
        {
            kwarng(
                "Function '%s': Unable to load specular texture '%s' for material '%s', using default.",
                __FUNCTION__, config->specular_map_name, m->name
            );
            spec_map->texture = texture_system_get_default_specular_texture();
        }
    }
    else
    {
        spec_map->use = TEXTURE_USE_UNKNOWN;
        spec_map->texture = null;
    }

    // Normal.
    texture_map* norm_map = &m->normal_map;
    if(string_length(config->normal_map_name) > 0)
    {
        norm_map->use = TEXTURE_USE_MAP_NORMAL;
        norm_map->texture = texture_system_acquire(config->normal_map_name, config->auto_release);

        if(!norm_map->texture)
        {
            kwarng(
                "Function '%s': Unable to load normal texture '%s' for material '%s', using default.",
                __FUNCTION__, config->normal_map_name, m->name
            );
            norm_map->texture = texture_system_get_default_normal_texture();
        }
    }
    else
    {
        norm_map->use = TEXTURE_USE_UNKNOWN;
        norm_map->texture = null;
    }

    // TODO: другие разметки.

    // Загрузка в графический процессор.
    shader* s = shader_system_get(config->shader_name);
    if(!s)
    {
        kerror(
            "Function '%s': Unable to load material because its shader was not found: '%s'. This is likely a problem with the material asset.",
            __FUNCTION__, config->shader_name
        );
        return false;
    }

    if(!renderer_shader_acquire_instance_resources(s, &m->internal_id))
    {
        kerror("Function '%s': Failed to acquire renderer resource for material '%s'.", __FUNCTION__, m->name);
        return false;
    }

    return true;
}

void material_destroy(material* m)
{
    // Удаление загруженый текстур.
    if(m->diffuse_map.texture)
    {
        texture_system_release(m->diffuse_map.texture->name);
    }

    if(m->specular_map.texture)
    {
        texture_system_release(m->specular_map.texture->name);
    }

    if(m->normal_map.texture)
    {
        texture_system_release(m->normal_map.texture->name);
    }

    // Удаление из памяти графического процессора.
    if(m->shader_id != INVALID_ID && m->internal_id != INVALID_ID)
    {
        renderer_shader_release_instance_resources(shader_system_get_by_id(m->shader_id), m->internal_id);
        m->shader_id = INVALID_ID;
    }

    // Освобождение памяти для нового материала.
    kzero_tc(m, material, 1);
    m->id = INVALID_ID;
    m->generation = INVALID_ID;
    m->internal_id = INVALID_ID;
    m->render_frame_number = INVALID_ID;
}
