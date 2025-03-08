// Собственные подключения.
#include "systems/shader_system.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "containers/darray.h"
#include "containers/hashtable.h"
#include "systems/texture_system.h"
#include "renderer/renderer_frontend.h"

typedef struct shader_system_state {
    // @brief Конфигурация системы шейдеров.
    shader_system_config config;
    // @brief Блок памяти хэш-таблицы для идентификаторов шейдоров.
    void* lookup_memory;
    // @brief Хэш-таблица идентификаторов шейдеров.
    hashtable* lookup;
    // @brief Идентификатор текущего связаного шейдера.
    u32 bound_shader_id;
    // @brief Массив шейдеров.
    shader* shaders;
} shader_system_state;

static shader_system_state* state_ptr = null;

u32 new_shader_id();
u32 get_shader_id(const char* shader_name);
bool add_attribute(shader* shader, shader_attribute_config* config);
bool add_sampler(shader* shader, shader_uniform_config* config);
bool add_uniform(shader* shader, shader_uniform_config* config);
bool uniform_add(shader* shader, const char* uniform_name, u32 size, shader_uniform_type type, shader_scope scope, u32 set_location, bool is_sampler);
bool uniform_add_state_valid(shader* shader);
bool uniform_name_valid(shader* shader, const char* uniform_name);
void shader_destroy(shader* shader);

// Используется для проверки статуса системы.
bool shader_system_status_valid(const char* func_name)
{
    if(!state_ptr)
    {
        if(func_name)
        {
            kerror(
                "Function '%s' requires the shader system to be initialized. Call 'shader_system_initialize' first.",
                func_name
            );
        }
        return false;
    }
    return true;
}

bool shader_system_initialize(u64* memory_requirement, void* memory, shader_system_config* config)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once!", __FUNCTION__);
        return false;
    }

    if(!memory_requirement || !config)
    {
        kerror("Function '%s' requires a valid pointer to memory_requirement and config.", __FUNCTION__);
        return false;
    }

    if(config->max_shader_count == 0)
    {
        kerror("Function '%s' requires a config.max_shader_count must be greater than zero.", __FUNCTION__);
        return false;
    }

    if(config->max_shader_count < 512)
    {
        kwarng("Function '%s': Recommended value for config.max_shader_count should be at least 512.", __FUNCTION__);
    }

    u64 state_requirement = sizeof(shader_system_state);
    u64 hashtable_requirement = 0;
    hashtable_config hcfg = { sizeof(u32), config->max_shader_count };
    hashtable_create(&hashtable_requirement, null, &hcfg, null);
    u64 shader_array_requirement = sizeof(shader) * config->max_shader_count;
    *memory_requirement = state_requirement + hashtable_requirement + shader_array_requirement;

    if(!memory)
    {
        return true;
    }

    kzero(memory, state_requirement);
    state_ptr = memory;
    state_ptr->lookup_memory = POINTER_GET_OFFSET(state_ptr, state_requirement);
    state_ptr->shaders = POINTER_GET_OFFSET(state_ptr->lookup_memory, hashtable_requirement);

    if(!hashtable_create(&hashtable_requirement, state_ptr->lookup_memory, &hcfg, &state_ptr->lookup))
    {
        kerror("Function '%s': Failed to create hashtable.", __FUNCTION__);
        return false;
    }

    state_ptr->config = *config;
    state_ptr->bound_shader_id = INVALID_ID;

    // Помечает шейдеры как свободные (неиспользуемые).
    for(u32 i = 0; i < state_ptr->config.max_shader_count; ++i)
    {
        state_ptr->shaders[i].id = INVALID_ID;
    }

    return true;
}

void shader_system_shutdown()
{
    if(!shader_system_status_valid(__FUNCTION__))
    {
        return;
    }

    // Уничтожение шейдеров.
    for(u32 i = 0; i < state_ptr->config.max_shader_count; ++i)
    {
        shader* s = &state_ptr->shaders[i];
        if(s->id != INVALID_ID)
        {
            shader_destroy(s);
        }
    }

    // Уничтожение хэш-таблицы.
    hashtable_destroy(state_ptr->lookup);

    state_ptr = 0;
}

bool shader_system_create(const shader_config* config)
{
    if(!shader_system_status_valid(__FUNCTION__) || !config)
    {
        return false;
    }

    u32 id = 0;
    if(hashtable_get(state_ptr->lookup, config->name, &id) && id != INVALID_ID)
    {
        kerror("Function '%s': Shader '%s' is exists in system.", __FUNCTION__, config->name);
        return false;
    }

    id = new_shader_id();

    if(id == INVALID_ID)
    {
        kerror("Function '%s': Unable to find free slot to create new shader.", __FUNCTION__);
        return false;
    }

    shader* shader = &state_ptr->shaders[id];
    shader->id = id;
    shader->state = SHADER_STATE_NOT_CREATED;
    shader->name = string_duplicate(config->name);
    shader->use_instances = config->use_instances;
    shader->use_locals = config->use_local;
    shader->bound_instance_id = INVALID_ID;

    // Создание динамаических массивов.
    shader->global_textures = darray_create(texture*);
    shader->uniforms = darray_create(shader_uniform);
    shader->attributes = darray_create(shader_attribute);

    // Создание хеш-таблицы для хранения индексов массивов uniform переменных.
    u64 hashtable_requirement = 0;
    hashtable_config hcfg = { sizeof(u16), 1024 };
    hashtable_create(&hashtable_requirement, null, &hcfg, null);
    shader->uniform_lookup_memory_requirement = hashtable_requirement;
    shader->uniform_lookup_memory = kallocate(hashtable_requirement, MEMORY_TAG_HASHTABLE);
    if(!hashtable_create(&hashtable_requirement, shader->uniform_lookup_memory, &hcfg, &shader->uniform_lookup))
    {
        kerror("Function '%s': Failed to create hashtable.", __FUNCTION__);
        kfree(shader->uniform_lookup_memory, hashtable_requirement, MEMORY_TAG_UNKNOWN); // Можно спокойно уничтожать, т.к. записей еще нет!
        return false;
    }

    // NOTE: Требование выравнивания UBO установлено в бэкэнде рендерера.

    // TODO: Это жестко закодировано, поскольку спецификация Vulkan гарантирует, что доступно только минимум 128 байт
    // пространства, и драйвер должен определить, сколько доступно. Поэтому, чтобы избежать проблем, будет использоваться
    // только наименьший общий знаменатель 128 байт.
    shader->push_constant_stride = 128;

    u8 renderpass_id = INVALID_ID_U8;
    if(!renderer_renderpass_id(config->renderpass_name, &renderpass_id))
    {
        kerror("Function '%s': Unable to find renderpass '%s'.", __FUNCTION__, config->renderpass_name);
        return false;
    }

    if(!renderer_shader_create(shader, renderpass_id, config->stage_count, (const char**)config->stage_filenames, config->stages))
    {
        kerror("Function '%s': Failed to create shader '%s'", __FUNCTION__, shader->name);
        return false;
    }

    shader->state = SHADER_STATE_UNINITIALIZED;

    // Получение и добавление атрибутов.
    for(u32 i = 0; i < config->attribute_count; ++i)
    {
        add_attribute(shader, &config->attributes[i]);
    }

    // Получение и добавление uniform переменных.
    for(u32 i = 0; i < config->uniform_count; ++i)
    {
        if(config->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER)
        {
            add_sampler(shader, &config->uniforms[i]);
        }
        else
        {
            add_uniform(shader, &config->uniforms[i]);
        }
    }

    if(!renderer_shader_initialize(shader))
    {
        kerror("Function '%s': Failed to initialize shader '%s'.", __FUNCTION__, shader->name);
        renderer_shader_destroy(shader);
        return false;
    }

    // Шейдер успешно создан. Обновление записи в хэш-таблице для быстрого поиска.
    if(!hashtable_set(state_ptr->lookup, shader->name, &shader->id, true))
    {
        shader_destroy(shader);
        return false;
    }

    return true;
}

void shader_system_destroy(const char* shader_name)
{
    if(!shader_system_status_valid(__FUNCTION__) || !shader_name)
    {
        return;
    }

    u32 shader_id = get_shader_id(shader_name);

    if (shader_id == INVALID_ID)
    {
        return;
    }

    shader* s = &state_ptr->shaders[shader_id];
    shader_destroy(s);
}

u32 shader_system_get_id(const char* shader_name)
{
    if(!shader_system_status_valid(__FUNCTION__) || !shader_name)
    {
        return INVALID_ID;
    }

    return get_shader_id(shader_name);
}

shader* shader_system_get_by_id(u32 shader_id)
{
    if(!shader_system_status_valid(__FUNCTION__))
    {
        return null;
    }

    if(shader_id >= state_ptr->config.max_shader_count || state_ptr->shaders[shader_id].id == INVALID_ID)
    {
        return null;
    }

    return &state_ptr->shaders[shader_id];
}

shader* shader_system_get(const char* shader_name)
{
    if(!shader_system_status_valid(__FUNCTION__) || !shader_name)
    {
        return null;
    }

    u32 shader_id = get_shader_id(shader_name);

    if(shader_id == INVALID_ID)
    {
        return null;
    }

    return shader_system_get_by_id(shader_id);
}

bool shader_system_use(const char* shader_name)
{
    if(!shader_system_status_valid(__FUNCTION__) || !shader_name)
    {
        return false;
    }

    u32 next_shader_id = get_shader_id(shader_name);

    if(next_shader_id == INVALID_ID)
    {
        return false;
    }

    return shader_system_use_by_id(next_shader_id);
}

bool shader_system_use_by_id(u32 shader_id)
{
    if(!shader_system_status_valid(__FUNCTION__))
    {
        return false;
    }

    // Исключает повторное выполнение.
    if(state_ptr->bound_shader_id != shader_id)
    {
        shader* next_shader = shader_system_get_by_id(shader_id);
        state_ptr->bound_shader_id = shader_id;

        if(!renderer_shader_use(next_shader))
        {
            kerror("Function '%s': Failed to use shader '%s'.", __FUNCTION__, next_shader->name);
            return false;
        }

        // По умолчанию используется глобальная привязка.
        if(!renderer_shader_bind_globals(next_shader))
        {
            kerror("Function '%s': Failed to bind globals for shader '%s'.", __FUNCTION__, next_shader->name);
            return false;
        }
    }

    return true;
}

u16 shader_system_uniform_index(shader* s, const char* uniform_name)
{
    if(!shader_system_status_valid(__FUNCTION__) || !s || s->id == INVALID_ID || !uniform_name)
    {
        kerror("Function '%s' requires a valid pointer to shader and uniform name.", __FUNCTION__);
        return INVALID_ID_U16;
    }

    u16 index = INVALID_ID_U16;
    if(!hashtable_get(s->uniform_lookup, uniform_name, &index) || index == INVALID_ID_U16)
    {
        kerror(
            "Function '%s': Shader '%s' does not have a registered uniform named '%s'",
            __FUNCTION__, s->name, uniform_name
        );
        return INVALID_ID_U16;
    }

    return s->uniforms[index].index;
}

bool shader_system_uniform_set(const char* uniform_name, const void* value)
{
    if(!shader_system_status_valid(__FUNCTION__))
    {
        return false;
    }

    if(state_ptr->bound_shader_id == INVALID_ID)
    {
        kerror("Function '%s' called without a shader in use.", __FUNCTION__);
        return false;
    }

    shader* s = &state_ptr->shaders[state_ptr->bound_shader_id];
    u16 index = shader_system_uniform_index(s, uniform_name);
    return shader_system_uniform_set_by_index(index, value);
}

bool shader_system_uniform_set_by_index(u16 index, const void* value)
{
    if(!shader_system_status_valid(__FUNCTION__) || !value)
    {
        return false;
    }

    shader* shader = &state_ptr->shaders[state_ptr->bound_shader_id];
    shader_uniform* uniform = &shader->uniforms[index];

    if(shader->bound_scope != uniform->scope)
    {
        if(uniform->scope == SHADER_SCOPE_GLOBAL)
        {
            renderer_shader_bind_globals(shader);
        }
        else if(uniform->scope == SHADER_SCOPE_INSTANCE)
        {
            renderer_shader_bind_instance(shader, shader->bound_instance_id);
        }

        shader->bound_scope = uniform->scope;
    }

    return renderer_shader_set_uniform(shader, uniform, value);
}

bool shader_system_sampler_set(const char* sampler_name, const texture* t)
{
    return shader_system_uniform_set(sampler_name, t);
}

bool shader_system_sampler_set_by_index(u16 index, const struct texture* t)
{
    return shader_system_uniform_set_by_index(index, t);
}

bool shader_system_apply_global()
{
    if(!shader_system_status_valid(__FUNCTION__))
    {
        return false;
    }

    return renderer_shader_apply_globals(&state_ptr->shaders[state_ptr->bound_shader_id]);
}

bool shader_system_apply_instance(bool needs_update)
{
    if(!shader_system_status_valid(__FUNCTION__))
    {
        return false;
    }

    return renderer_shader_apply_instance(&state_ptr->shaders[state_ptr->bound_shader_id], needs_update);
}

bool shader_system_bind_instance(u32 instance_id)
{
    if(!shader_system_status_valid(__FUNCTION__))
    {
        return false;
    }

    shader* s = &state_ptr->shaders[state_ptr->bound_shader_id];
    s->bound_instance_id = instance_id;
    return renderer_shader_bind_instance(s, instance_id);
}

u32 new_shader_id()
{
    for(u32 i = 0; i < state_ptr->config.max_shader_count; ++i)
    {
        if(state_ptr->shaders[i].id == INVALID_ID)
        {
            return i;
        }
    }

    return INVALID_ID;
}

u32 get_shader_id(const char* shader_name)
{
    u32 shader_id = INVALID_ID;

    if(!hashtable_get(state_ptr->lookup, shader_name, &shader_id))
    {
        kerror("Function '%s': There is no shader registered named '%s'.", __FUNCTION__, shader_name);
        return INVALID_ID;
    }

    return shader_id;
}

bool add_attribute(shader* shader, shader_attribute_config* config)
{
    u32 size = 0;

    switch(config->type)
    {
        case SHADER_ATTRIB_TYPE_INT8:
        case SHADER_ATTRIB_TYPE_UINT8:
            size = 1;
            break;
        case SHADER_ATTRIB_TYPE_INT16:
        case SHADER_ATTRIB_TYPE_UINT16:
            size = 2;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32:
        case SHADER_ATTRIB_TYPE_INT32:
        case SHADER_ATTRIB_TYPE_UINT32:
            size = 4;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32_2:
            size = 8;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32_3:
            size = 12;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32_4:
            size = 16;
            break;
        default:
            kerror(
                "Function '%s': Unrecognized type, defaulting to size of 4. This probably is not what is desired.",
                __FUNCTION__
            );
            size = 4;
            break;
    }

    shader->attribute_stride += size;

    // Создание и отправка атрибута в массив.
    shader_attribute attr = {};
    attr.name = string_duplicate(config->name);
    attr.size = size;
    attr.type = config->type;
    darray_push(shader->attributes, attr);

    return true;
}

bool add_sampler(shader* shader, shader_uniform_config* config)
{
    if(config->scope == SHADER_SCOPE_INSTANCE && !shader->use_instances)
    {
        kerror("Function '%s': Cannot add an instance sampler for a shader that does not use instances.", __FUNCTION__);
        return false;
    }

    // Сэмплеры нельзя использовать для констант push.
    if(config->scope == SHADER_SCOPE_LOCAL)
    {
        kerror("Function '%s': Cannot add a sampler at local scope.", __FUNCTION__);
        return false;
    }

    // Проверка: является ли имя допустимым и уникальным.
    if(!uniform_name_valid(shader, config->name) || !uniform_add_state_valid(shader))
    {
        return false;
    }

    // If global, push into the global list.
    u32 location = 0;
    if(config->scope == SHADER_SCOPE_GLOBAL)
    {
        u32 global_texture_count = darray_length(shader->global_textures);
        if(global_texture_count + 1 > state_ptr->config.max_global_textures)
        {
            kerror(
                "Function '%s': Shader global texture count %i exceeds max of %i.",
                __FUNCTION__, global_texture_count, state_ptr->config.max_global_textures
            );
            return false;
        }

        location = global_texture_count;
        darray_push(shader->global_textures, texture_system_get_default_texture());
    }
    else
    {
        // На уровене экземпляра учитывайте, сколько нужно добавить во время получения ресурса.
        if(shader->instance_texture_count + 1 > state_ptr->config.max_instance_textures)
        {
            kerror(
                "Function '%s': Shader instance texture count %i exceeds max of %i.",
                __FUNCTION__, shader->instance_texture_count, state_ptr->config.max_instance_textures
            );
            return false;
        }

        location = shader->instance_texture_count;
        shader->instance_texture_count++;
    }

    // Относитесь к нему как к uniform. NOTE: В случае сэмплеров location используется для определения
    // значения поля 'location' записи хэш-таблицы напрямую, а затем устанавливается в индекс массива
    // uniforms. Это позволяет выполнять поиск местоположения для сэмплеров, как если бы они были
    // униформами (поскольку технически они таковыми являются).
    // TODO: может потребоваться сохранить это в другом месте.
    if(!uniform_add(shader, config->name, 0, config->type, config->scope, location, true))
    {
        kerror("Function '%s': Unable to add sampler uniform.", __FUNCTION__);
        return false;
    }

    return true;
}

bool add_uniform(shader* shader, shader_uniform_config* config)
{
    if(!uniform_add_state_valid(shader) || !uniform_name_valid(shader, config->name))
    {
        return false;
    }

    return uniform_add(shader, config->name, config->size, config->type, config->scope, 0, false);
}

bool uniform_add(shader* shader, const char* uniform_name, u32 size, shader_uniform_type type, shader_scope scope, u32 set_location, bool is_sampler)
{
    u32 uniform_count = darray_length(shader->uniforms);

    if(uniform_count + 1 > state_ptr->config.max_uniform_count)
    {
        kerror(
            "Function '%s': A shader can only accept a combined maximum of %d uniforms and samplers at global, instance and local scopes.",
            __FUNCTION__, state_ptr->config.max_uniform_count
        );
        return false;
    }

    shader_uniform entry;
    entry.index = uniform_count; // Индекс сохраняется в хеш-таблице для поиска.
    entry.scope = scope;
    entry.type  = type;

    bool is_global = (scope == SHADER_SCOPE_GLOBAL);

    if(is_sampler)
    {
        entry.location = set_location;
    }
    else
    {
        entry.location = entry.index;
    }

    if(scope != SHADER_SCOPE_LOCAL)
    {
        entry.set_index = (u32)scope;
        entry.offset = is_sampler ? 0 : is_global ? shader->global_ubo_size : shader->ubo_size;
        entry.size = is_sampler ? 0 : size;
    }
    else
    {
        if(entry.scope == SHADER_SCOPE_LOCAL && !shader->use_locals)
        {
            kerror(
                "Function '%s': Cannot add a locally-scoped uniform for a shader that does not support locals.",
                __FUNCTION__
            );
            return false;
        }

        entry.set_index = INVALID_ID_U8;
        range r = get_aligned_range(shader->push_constant_size, size, 4);
        entry.offset = r.offset;
        entry.size = r.size;

        shader->push_constant_ranges[shader->push_constant_range_count] = r;
        shader->push_constant_range_count++;
        shader->push_constant_size += r.size;
    }

    if(!hashtable_set(shader->uniform_lookup, uniform_name, &entry.index, false))
    {
        kerror("Function '%s': Unable to add uniform '%s' because it already exists.", __FUNCTION__, uniform_name);
        return false;
    }

    darray_push(shader->uniforms, entry);

    if(!is_sampler)
    {
        if(entry.scope == SHADER_SCOPE_GLOBAL)
        {
            shader->global_ubo_size += entry.size;
        }
        else if(entry.scope == SHADER_SCOPE_INSTANCE)
        {
            shader->ubo_size += entry.size;
        }
    }

    return true;
}

bool uniform_add_state_valid(shader* shader)
{
    if(shader->state != SHADER_STATE_UNINITIALIZED)
    {
        kerror("Function '%s': Uniforms may only be added to shaders before initialization.", __FUNCTION__);
        return false;
    }

    return true;
}

bool uniform_name_valid(shader* shader, const char* uniform_name)
{
    if(!uniform_name || !string_length(uniform_name))
    {
        kerror("Functio '%s': Uniform name must exist.", __FUNCTION__);
        return false;
    }

    u16 location = INVALID_ID_U16;
    if(hashtable_get(shader->uniform_lookup, uniform_name, &location) && location != INVALID_ID_U16)
    {
        kerror(
            "Function '%s': A uniform by the name '%s' already exists on shader '%s'.",
            __FUNCTION__, uniform_name, shader->name
        );
        return false;
    }

    return true;
}

void shader_destroy(shader* s)
{
    // Освободить ресурсы системы.
    renderer_shader_destroy(s);

    // Делает шейдер недоступным.
    s->state = SHADER_STATE_NOT_CREATED;

    // Сделать индекс шейдера в таблице недействительным.
    if(!hashtable_set(state_ptr->lookup, s->name, &s->id, true))
    {
        kerror("Function '%s': Failed to update hashtable, but continues...", __FUNCTION__);
    }

    // Удалние имени шейдера.
    if(s->name)
    {
        string_free(s->name);
    }

    // Освобождение uniform переменных и сэмплеров.
    darray_destroy(s->uniforms);

    // Освобождение атрибутов.
    u32 count = darray_length(s->attributes);
    for(u32 i = 0; i < count; ++i)
    {
        string_free(s->attributes[i].name);
    }
    darray_destroy(s->attributes);

    // Освобождение hashtable.
    hashtable_destroy(s->uniform_lookup);
    kfree(s->uniform_lookup_memory, s->uniform_lookup_memory_requirement, MEMORY_TAG_HASHTABLE);

    // Освобождение текстур.
    darray_destroy(s->global_textures);

    // Освободить слот шейдера, для новых использований!
    kzero_tc(s, shader, 1);
    s->id = INVALID_ID;
}
