// Собственные подключения.
#include "systems/texture_system.h"
#include "systems/resource_system.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "containers/hashtable.h"
#include "renderer/renderer_frontend.h"

typedef struct texture_system_state {
    // Конфигурация системы.
    texture_system_config config;
    // Текстура по умолчанию.
    texture default_texture;
    // Массив текстур.
    texture* textures;
    // Таблица ссылок на текстуры.
    hashtable* texture_references_table;
} texture_system_state;

typedef struct texture_reference {
    // Количество ссылок на текстуру.
    u64 reference_count;
    // Индекс текстуры в массиве текстур.
    u32 index;
    // Авто уничтожение текстуры.
    bool auto_release;
} texture_reference;

static texture_system_state* state_ptr = null;

bool texture_system_status_valid(const char* func_name)
{
    if(!state_ptr)
    {
        if(func_name)
        {
            kerror(
                "Function '%s' requires the texture system to be initialized. Call 'texture_system_initialize' first.",
                func_name
            );
            
        }
        return false;
    }
    return true;
}

bool default_textures_create();
void default_textures_destroy();
bool texture_load(const char* texture_name, texture* t);
void texture_destroy(texture* t);

bool texture_system_initialize(u64* memory_requirement, void* memory, texture_system_config* config)
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

    if(!config->max_texture_count)
    {
        kerror("Function '%s': config.max_texture_count must be greater then zero. Return false!", __FUNCTION__);
        return false;
    }

    u64 state_requirement = sizeof(texture_system_state);
    u64 textures_requirement = sizeof(texture) * config->max_texture_count;
    u64 hashtable_requirement = 0;
    hashtable_config hconf = { sizeof(texture_reference), config->max_texture_count };
    hashtable_create(&hashtable_requirement, null, &hconf, null);
    *memory_requirement = state_requirement + textures_requirement + hashtable_requirement;

    if(!memory)
    {
        return true;
    }

    // Обнуление заголовка системы менеджмента текстур.
    kzero_tc(memory, texture_system_state, 1);
    state_ptr = memory;

    // Запись данных конфигурации системы.
    state_ptr->config.max_texture_count = config->max_texture_count;

    // Получение и запись указателя на блок текстур.
    void* textures_block =  POINTER_GET_OFFSET(state_ptr, state_requirement);
    state_ptr->textures = textures_block;

    // Получение и запись указателя на хэш-таблицу.
    void* hashtable_block = POINTER_GET_OFFSET(textures_block, textures_requirement);
    if(!hashtable_create(&hashtable_requirement, hashtable_block, &hconf, &state_ptr->texture_references_table))
    {
        kerror("Function '%s': Failed to create hashtable of references to textures.", __FUNCTION__);
        return false;
    }

    // Отмечает все текстуры как недействительные.
    for(u32 i = 0; i < state_ptr->config.max_texture_count; ++i)
    {
        state_ptr->textures[i].id = INVALID_ID;
        state_ptr->textures[i].generation = INVALID_ID;
    }

    // Создание текстуры по-умолчанию.
    if(!default_textures_create())
    {
        kerror("Function '%s': Failed to create default textures.", __FUNCTION__);
        return false;
    }

    return true;
}

void texture_system_shutdown()
{
    if(!texture_system_status_valid(__FUNCTION__))
    {
        return;
    }

    // Уничтожение хэш-таблицы.
    hashtable_destroy(state_ptr->texture_references_table);

    // Уничтожение всех созданых текстур.
    for(u32 i = 0; i < state_ptr->config.max_texture_count; ++i)
    {
        texture* t = &state_ptr->textures[i];
        if(t->generation != INVALID_ID)
        {
            texture_destroy(t);
        }
    }

    // Уничтожение текстур по-умолчанию.
    default_textures_destroy();

    state_ptr = null;
}

texture* texture_system_acquire(const char* name, bool auto_release)
{
    if(!texture_system_status_valid(__FUNCTION__))
    {
        return null;
    }

    if(string_equali(name, DEFAULT_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default texture. Call 'texture_system_get_default_texture'!", __FUNCTION__);
        return &state_ptr->default_texture;
    }

    texture_reference ref;
    // NOTE: После появления записи в hashtable заново она не создается, reference_count = 0 всегда, если текстура не используется!
    //       Если авто освобождение отключено, то ref.index будет иметь индекс созданой текстуры!
    //       Но если авто освобождение включено, то текстура будет удаляться из памяти, и ref.index будет равен INVAID_ID
    //       а следовательно, что по этому же имени нужно заново использовать эту ссылку!
    if(!hashtable_get(state_ptr->texture_references_table, name, &ref) || ref.index == INVALID_ID)
    {
        ref.reference_count = 0;
        ref.auto_release = auto_release;
        ref.index = INVALID_ID;
    }

    ref.reference_count++;

    if(ref.index == INVALID_ID)
    {
        // Поиск свободной памяти для текстуры.
        for(u32 i = 0; i < state_ptr->config.max_texture_count; ++i)
        {
            if(state_ptr->textures[i].id == INVALID_ID)
            {
                ref.index = i;
                break;
            }
        }

        // Если свободный участок памяти не найден.
        if(ref.index == INVALID_ID)
        {
            kerror(
                "Function '%s': Texture system cannot hold anymore textures. Adjust configuration to allow more.",
                __FUNCTION__
            );
            return null;
        }

        texture* t = &state_ptr->textures[ref.index];
        t->id = ref.index;

        // Создание текстуры.
        if(!texture_load(name, t))
        {
            kerror("Function '%s': Failed to load texture '%s'.", __FUNCTION__, name);
            return null;
        }

        ktrace(
            "Function '%s': Texture '%s' does not exist. Created, and reference count is now %i.",
            __FUNCTION__, name, ref.reference_count
        );
    }
    else
    {
        ktrace(
            "Function '%s': Texture '%s' already exists, and reference count increased to %i.",
            __FUNCTION__, name, ref.reference_count
        );
    }

    // TODO: Имя создается однажды и остается до конца существования таблицы, может красть память под текстуры!
    // Обновление ссылки на текстуру.
    if(!hashtable_set(state_ptr->texture_references_table, name, &ref, true))
    {
        kerror("Function '%s' Failed to update texture reference.", __FUNCTION__);
        return null;
    }

    return &state_ptr->textures[ref.index];
}

void texture_system_release(const char* name)
{
    if(!texture_system_status_valid(__FUNCTION__))
    {
        return;
    }

    // Игнорирование удаления текстуры по умолчанию.
    if(string_equali(name, DEFAULT_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default texture.", __FUNCTION__);
        return;
    }

    texture_reference ref;
    if(!hashtable_get(state_ptr->texture_references_table, name, &ref) || ref.reference_count == 0)
    {
        kwarng("Function '%s': Tried to release non-existent texture '%s'.", __FUNCTION__, name);
        return;
    }

    // Копия имени.
    char name_copy[TEXTURE_NAME_MAX_LENGTH];
    string_ncopy(name_copy, name, TEXTURE_NAME_MAX_LENGTH);

    ref.reference_count--;

    if(ref.reference_count == 0 && ref.auto_release)
    {
        texture* t = &state_ptr->textures[ref.index];

        // Освобождение/восстановление памяти текстуры для новой.
        texture_destroy(t);

        // Освобождение ссылки.
        ref.index = INVALID_ID;
        ref.auto_release = false;

        ktrace(
            "Function '%s': Released texture '%s', because reference count is 0 and auto release used.",
            __FUNCTION__, name_copy
        );
    }
    else
    {
        ktrace(
            "Function '%s': Released texture '%s', now has a reference count is %i and auto release is %s.",
            __FUNCTION__, name_copy, ref.reference_count, ref.auto_release ? "used" : "unused"
        );
    }

    // Обновление ссылки на текстуру.
    if(!hashtable_set(state_ptr->texture_references_table, name_copy, &ref, true))
    {
        kerror("Function '%s' Failed to update texture reference.", __FUNCTION__);
    }
}

texture* texture_system_get_default_texture()
{
    if(!texture_system_status_valid(__FUNCTION__))
    {
        return null;
    }

    return &state_ptr->default_texture;
}

bool default_textures_create()
{
    // NOTE: Создание текстуры по умолчанию, черно-белый шахматный узор 256x256.
    //       Это делается в коде, чтобы исключить зависимости от ресурсов.
    const u32 tex_dimension = 256;
    const u32 bpp = 4; // Байтов на пиксель (RBGA).
    const u32 pixel_count = tex_dimension * tex_dimension;
    const u32 pixels_size = sizeof(u8) * pixel_count * bpp;

    u8* pixels = kallocate(pixels_size, MEMORY_TAG_TEXTURE);
    if(!pixels)
    {
        kerror("Function '%s': Failed to allocate memory.", __FUNCTION__);
        return false;
    }

    kset(pixels, pixels_size, 0xff);

    // Обработка каждого пикселя.
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

    string_ncopy(state_ptr->default_texture.name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state_ptr->default_texture.width = tex_dimension;
    state_ptr->default_texture.height = tex_dimension;
    state_ptr->default_texture.channel_count = bpp;
    state_ptr->default_texture.has_transparency = false;
    // TODO: Когда текстура отдается на создание сразу, то переключение текстур запаздывает!
    //       Алгоритм шейдера!
    state_ptr->default_texture.generation = INVALID_ID;

    renderer_create_texture(&state_ptr->default_texture, pixels);

    kfree(pixels, pixels_size, MEMORY_TAG_TEXTURE);
    return true;
}

void default_textures_destroy()
{
    texture_destroy(&state_ptr->default_texture);
}

bool texture_load(const char* texture_name, texture* t)
{
    resource img_resource;
    if(!resource_system_load(texture_name, RESOURCE_TYPE_IMAGE, &img_resource))
    {
        kerror("Function '%s': Failed to load image resource for texture '%s'.", __FUNCTION__, texture_name);
        return false;
    }

    image_resouce_data* resource_data = img_resource.data;
    t->width = resource_data->width;
    t->height = resource_data->height;
    t->channel_count = resource_data->channel_count;

    // Проверка прозрачности.
    u64 total_size = t->width * t->height * t->channel_count;
    bool has_transparency = false;
    for(u64 i = 0; i < total_size; i += t->channel_count)
    {
        // RGB[A]
        // NOTE: Не корректно, т.к. реальный размер канала может быть 3!
        u8 a = resource_data->pixels[i + 3];
        if(a < 255)
        {
            has_transparency = true;
            break;
        }
    }

    // Копирование имени текстуры.
    string_ncopy(t->name, texture_name, TEXTURE_NAME_MAX_LENGTH);
    t->generation = 0;
    t->has_transparency = has_transparency;

    // Загрузка в графический процессор.
    renderer_create_texture(t, resource_data->pixels);

    // Очистка данных.
    resource_system_unload(&img_resource);
    return true;
}

void texture_destroy(texture* t)
{
    // Удаление из памяти графического процессора.
    renderer_destroy_texture(t);

    // Освобождение памяти для новой текстуры.
    kzero_tc(t, texture, 1);
    t->id = INVALID_ID;
    t->generation = INVALID_ID;
}
