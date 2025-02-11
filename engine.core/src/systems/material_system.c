// Собственные подключения.
#include "systems/material_system.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "containers/hashtable.h"
#include "math/kmath.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"

// TODO: Временно!
#include "platform/file.h"

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

// TODO: Общая структура!
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
bool material_load_configuration_file(const char* path, material_config* out_config);

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
        state_ptr->materials[i].id = INVALID_ID32;
        state_ptr->materials[i].generation = INVALID_ID32;
        state_ptr->materials[i].internal_id = INVALID_ID32;
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
        if(m->id != INVALID_ID32)
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

    // Загрузка конфигурации материала с диска.
    material_config config;

    // Загрузка файла.
    // TODO: Возможность менять локацию файла.
    char* format_str = "../assets/materials/%s.%s";
    char filepath[512];

    // TODO: попробовать разные расширения.
    string_format(filepath, format_str, name, "kmt");

    if(!material_load_configuration_file(filepath, &config))
    {
        kerror("Function '%s': Failed to load file '%s'. Return null!", __FUNCTION__, filepath);
        return null;
    }

    return material_system_acquire_from_config(&config);
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
    if(!hashtable_get(state_ptr->material_references_table, config->name, &ref) || ref.index == INVALID_ID32)
    {
        ref.reference_count = 0;
        ref.auto_release = config->auto_release;
        ref.index = INVALID_ID32;
    }

    ref.reference_count++;

    if(ref.index == INVALID_ID32)
    {
        // Поиск свободной памяти для материала.
        for(u32 i = 0; i < state_ptr->config.max_material_count; ++i)
        {
            if(state_ptr->materials[i].id == INVALID_ID32)
            {
                ref.index = i;
                break;
            }
        }

        // Если свободный участок памяти не найден.
        if(ref.index == INVALID_ID32)
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
        if(m->generation == INVALID_ID32)
        {
            kdebug("MARETIAL generation INVALID_ID32!");
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
        ref.index = INVALID_ID32;
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

bool default_materials_create()
{
    kzero_tc(&state_ptr->default_material, material, 1);
    state_ptr->default_material.id = INVALID_ID32;
    state_ptr->default_material.generation = INVALID_ID32;

    string_ncopy(state_ptr->default_material.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state_ptr->default_material.diffuse_color = vec4_one();    // Белый цвет.
    state_ptr->default_material.diffuse_map.use = TEXTURE_USE_MAP_DIFFUSE;
    state_ptr->default_material.diffuse_map.texture = texture_system_get_default_texture();

    if(!renderer_create_material(&state_ptr->default_material))
    {
        kerror("Function '%s': Failed to acquire renderer resource for default texture.", __FUNCTION__);
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
    m->id = INVALID_ID32;
    m->generation = INVALID_ID32;
    m->internal_id = INVALID_ID32;
}

bool material_load_configuration_file(const char* path, material_config* out_config)
{
    file* f = null;

    if(!platform_file_open(path, FILE_MODE_READ, &f))
    {
        kerror("Function '%s': Unable to open material file '%s' for reading.", __FUNCTION__, path);
        return false;
    }

    kdebug("Material file size %llu.", platform_file_size(f));

    char bufferline[512] = "";
    char* p = bufferline;
    u64 line_length = 0;
    u32 line_number = 1;

    // Построчное чтение файла.
    while(platform_file_read_line(f, 511, p, &line_length))
    {
        char* trimmed = string_trim(bufferline);
        line_length = string_length(trimmed);

        // Пропуск пустых строк и коментариев.
        if(line_length < 1 || trimmed[0] == '#')
        {
            line_number++;
            continue;
        }

        i32 equal_index = string_index_of(trimmed, '=');
        if(equal_index == -1)
        {

            kwarng(
                "Potential formatting issue found in file '%s': '=' token not found. Skipping line %u.",
                path, line_number
            );
        }

        // Max of 64 characters for variable name.
        char raw_var_name[64];
        kzero(raw_var_name, sizeof(char) * 64);
        string_mid(raw_var_name, trimmed, 0, equal_index);
        char* trimmed_var_name = string_trim(raw_var_name);

        // Max of 511-65 (446) characters for value.
        char raw_value[446];
        kzero(raw_value, sizeof(char) * 446);
        string_mid(raw_value, trimmed, equal_index + 1, -1);
        char* trimmed_value = string_trim(raw_value);

        // Процесс формирования.
        if(string_equali(trimmed_var_name, "version"))
        {
            // TODO: Версия.
        }
        else if(string_equali(trimmed_var_name, "name"))
        {
            string_ncopy(out_config->name, trimmed_value, MATERIAL_NAME_MAX_LENGTH);
        }
        else if(string_equali(trimmed_var_name, "diffuse_map_name"))
        {
            string_ncopy(out_config->diffuse_map_name, trimmed_value, TEXTURE_NAME_MAX_LENGTH);
        }
        else if(string_equali(trimmed_var_name, "diffuse_color"))
        {
            if(!string_to_vec4(trimmed_value, &out_config->diffuse_color))
            {
                kwarng("Error parsing diffuse_color in file '%s'. Using default of white instead.", path);
                out_config->diffuse_color = vec4_one(); // Белый.
            }
        }

        // TODO: Другие поля.

        // Очистка буфера.
        kzero(bufferline, sizeof(char) * 512);
        line_number++;
    }

    platform_file_close(f);

    return true;
}
