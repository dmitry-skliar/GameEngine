// Собственные подключения.
#include "systems/texture_system.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "containers/hashtable.h"
#include "renderer/renderer_frontend.h"

// Внешние подключения.
// TODO: загрузчик ресурсов.
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

typedef struct texture_system_state {
    // @breif Конфигурация системы.
    texture_system_config config;
    // @brief Текстура по умолчанию.
    texture default_texture;
    // @brief Массив текстур.
    texture* textures;
    // @brief Таблица ссылок на текстуры.
    hashtable* texture_references_table;
} texture_system_state;

typedef struct texture_reference {
    // @brief Количество ссылок на текстуру.
    u64 reference_count;
    // @brief Индекс текстуры в массиве текстур.
    u32 index;
    // @brief Авто уничтожение текстуры.
    bool auto_release;
} texture_reference;

static texture_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the texture system to be initialized. Call 'texture_system_initialize' first.";

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

    hashtable_config hconf;
    hconf.data_size = sizeof(texture_reference);
    hconf.entry_count = config->max_texture_count;

    u64 state_requirement = sizeof(texture_system_state);
    u64 textures_requirement = sizeof(texture) * config->max_texture_count;
    u64 hashtable_requirement = 0;
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
    void* textures_block = (void*)((u8*)state_ptr + state_requirement);
    state_ptr->textures = textures_block;

    // Получение и запись указателя на хэш-таблицу.
    void* hashtable_block = (void*)((u8*)textures_block + textures_requirement);
    if(!hashtable_create(&hashtable_requirement, hashtable_block, &hconf, &state_ptr->texture_references_table))
    {
        kerror("Function '%s': Failed to create hashtable of references to textures.", __FUNCTION__);
        return false;
    }

    // Отмечает все текстуры как недействительные.
    for(u32 i = 0; i < state_ptr->config.max_texture_count; ++i)
    {
        state_ptr->textures[i].id = INVALID_ID32;
        state_ptr->textures[i].generation = INVALID_ID32;
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
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // Уничтожение хэш-таблицы.
    hashtable_destroy(state_ptr->texture_references_table);

    // Уничтожение всех созданых текстур.
    for(u32 i = 0; i < state_ptr->config.max_texture_count; ++i)
    {
        texture* t = &state_ptr->textures[i];
        if(t->generation != INVALID_ID32)
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
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
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
    //       Но если авто освобождение включено, то текстура будет удаляться из памяти, и ref.index будет равен INVAID_ID32
    //       а следовательно, что по этому же имени нужно заново использовать эту ссылку!
    if(!hashtable_get(state_ptr->texture_references_table, name, &ref) || ref.index == INVALID_ID32)
    {
        ref.reference_count = 0;
        ref.auto_release = auto_release;
        ref.index = INVALID_ID32;
    }

    ref.reference_count++;

    if(ref.index == INVALID_ID32)
    {
        // Поиск свободной памяти для текстуры.
        for(u32 i = 0; i < state_ptr->config.max_texture_count; ++i)
        {
            if(state_ptr->textures[i].id == INVALID_ID32)
            {
                ref.index = i;
                break;
            }
        }

        // Если свободный участок памяти не найден.
        if(ref.index == INVALID_ID32)
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
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
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
        ref.index = INVALID_ID32;
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
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
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
    state_ptr->default_texture.generation = INVALID_ID32;

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
    // TODO: Должна быть возможность размещения в любом месте.
    char* format_str = "../assets/textures/%s.%s";
    const i32 required_channel_count = 4;
    stbi_set_flip_vertically_on_load(true);
    char full_file_path[512];

    // TODO: попробовать разные расширения.
    string_format(full_file_path, format_str, texture_name, "png");

    u8* data = stbi_load(
        full_file_path, (i32*)&t->width, (i32*)&t->height, (i32*)&t->channel_count,
        required_channel_count
    );

    if(!data || stbi_failure_reason())
    {
        kwarng("Function '%s': Failed to load file '%s' with result: %s.", __FUNCTION__, full_file_path, stbi_failure_reason());
        // Очистка ошибок stbi, что бы следующая текстура могла загрузиться.
        stbi__err(0, 0);
        return false;
    }

    // TODO: Реальное количество может не совпадать!
    t->channel_count = required_channel_count;

    // Копирование имени текстуры.
    string_ncopy(t->name, texture_name, TEXTURE_NAME_MAX_LENGTH);
    

    u64 total_size = t->width * t->height * required_channel_count;

    // Проверка прозрачности.
    for(u64 i = 0; i < total_size; i += required_channel_count)
    {
        // RGB[A]
        u8 a = data[i + 3];
        if(a < 255)
        {
            t->has_transparency = true;
            break;
        }
    }

    // Текстура всегда новая.
    t->generation = 0;

    // Загрузка в графический процессор.
    renderer_create_texture(t, data);

    // Очистка данных.
    stbi_image_free(data);
    return true;
}

void texture_destroy(texture* t)
{
    // Удаление из памяти графического процессора.
    renderer_destroy_texture(t);

    // Освобождение памяти для новой текстуры.
    // kzero_tc(t->name, char, TEXTURE_NAME_MAX_LENGTH);
    kzero_tc(t, texture, 1);
    t->id = INVALID_ID32;
    t->generation = INVALID_ID32;
}
