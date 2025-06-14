// Собственные подключения.
#include "systems/texture_system.h"
#include "systems/resource_system.h"
#include "systems/job_system.h"

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
    texture default_diffuse_texture;
    texture default_specular_texture;
    texture default_normal_texture;
    // Массив текстур.
    texture* textures;
    // Таблица ссылок на текстуры.
    hashtable* texture_references_table;
} texture_system_state;

// TODO: Умную выгрузку текстур. Например вугружать те материалы которые можно выгружать
//       и только при достижении определенной границы памяти для загрузки новых.
//       Или сделать несколько режимов работы выгрузки текстур.
typedef struct texture_reference {
    // Индекс текстуры в массиве текстур.
    u32 id;
    // Количество ссылок на текстуру.
    u64 reference_count;
    // Авто уничтожение текстуры.
    bool auto_release;
} texture_reference;

typedef struct texture_load_params {
    char* resource_name;
    texture* out_texture;
    // texture temp_texture;
    u32 current_generation;
    resource image_resource;
} texture_load_params;

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
bool texture_load(const char* texture_name, texture* t); // TODO: Объединить ...load и ...load_cube в одну!
bool texture_load_cube(const char* base_name, const char texture_names[6][TEXTURE_NAME_MAX_LENGTH], texture* t);
void texture_destroy(texture* t);

// TODO: Отправтить старую текстуру сюда, и выдать новую. В момент пока текстура не загружена должно указывать на страрую,
//       но после загрузки указывать на новую.
bool texture_process_acquire(const char* name, texture_type type, bool auto_release, bool skip_load, u32* out_texture_id);
bool texture_process_release(const char* name);

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

    // Создание текстуры по умолчанию.
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

    // Уничтожение текстур по умолчанию.
    default_textures_destroy();

    state_ptr = null;
}

texture* texture_system_acquire(const char* name, bool auto_release)
{
    if(!texture_system_status_valid(__FUNCTION__)) return null;

    if(string_equali(name, DEFAULT_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default texture. Call 'texture_system_get_default_texture'!", __FUNCTION__);
        return &state_ptr->default_texture;
    }
    
    if(string_equali(name, DEFAULT_DIFFUSE_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default diffuse texture. Call 'texture_system_get_default_diffuse_texture'!", __FUNCTION__);
        return &state_ptr->default_diffuse_texture;
    }

    if(string_equali(name, DEFAULT_SPECULAR_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default specular texture. Call 'texture_system_get_default_speculat_texture'!", __FUNCTION__);
        return &state_ptr->default_specular_texture;
    }

    if(string_equali(name, DEFAULT_NORMAL_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default normal texture. Call 'texture_system_get_default_normal_texture'!", __FUNCTION__);
        return &state_ptr->default_normal_texture;
    }

    u32 id = INVALID_ID;
    if(!texture_process_acquire(name, TEXTURE_TYPE_2D, auto_release, false, &id))
    {
        kerror("Function '%s': Failed to obtain a new texture id.", __FUNCTION__);
        return null;
    }

    return &state_ptr->textures[id];
}

texture* texture_system_acquire_cube(const char* name, bool auto_release)
{
    if(!texture_system_status_valid(__FUNCTION__)) return null;

    if(string_equali(name, DEFAULT_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default texture. Call 'texture_system_get_default_texture'!", __FUNCTION__);
        return &state_ptr->default_texture;
    }
    
    if(string_equali(name, DEFAULT_DIFFUSE_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default diffuse texture. Call 'texture_system_get_default_diffuse_texture'!", __FUNCTION__);
        return &state_ptr->default_diffuse_texture;
    }

    if(string_equali(name, DEFAULT_SPECULAR_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default specular texture. Call 'texture_system_get_default_speculat_texture'!", __FUNCTION__);
        return &state_ptr->default_specular_texture;
    }

    if(string_equali(name, DEFAULT_NORMAL_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default normal texture. Call 'texture_system_get_default_normal_texture'!", __FUNCTION__);
        return &state_ptr->default_normal_texture;
    }

    u32 id = INVALID_ID;
    if(!texture_process_acquire(name, TEXTURE_TYPE_CUBE, auto_release, false, &id))
    {
        kerror("Function '%s': Failed to obtain a new texture id.", __FUNCTION__);
        return null;
    }

    return &state_ptr->textures[id];
}

texture* texture_system_acquire_writable(const char* name, u32 width, u32 height, u8 channel_count, bool has_transparency)
{
    if(!texture_system_status_valid(__FUNCTION__)) return null;

    u32 id = INVALID_ID;
    if(!texture_process_acquire(name, TEXTURE_TYPE_2D, false, true, &id))
    {
        kerror("Function '%s': Failed to obtain a new texture id.", __FUNCTION__);
        return null;
    }

    texture* t = &state_ptr->textures[id];
    string_ncopy(t->name, name, TEXTURE_NAME_MAX_LENGTH);
    t->width = width;
    t->height = height;
    t->channel_count = channel_count;
    t->type = TEXTURE_TYPE_2D;
    t->generation = INVALID_ID;
    t->flags |= has_transparency ? TEXTURE_FLAG_HAS_TRANSPARENCY : 0;
    t->flags |= TEXTURE_FLAG_IS_WRITABLE;
    t->internal_data = null;
    renderer_texture_create_writable(t);
    return t;
}

void texture_system_release(const char* name)
{
    if(!texture_system_status_valid(__FUNCTION__)) return;

    if(!name || !string_length(name))
    {
        kerror("Function '%s' called without texture name.", __FUNCTION__);
        return;
    }

    // Игнорирование удаления текстур по умолчанию.
    if(string_equali(name, DEFAULT_TEXTURE_NAME) || string_equali(name, DEFAULT_DIFFUSE_TEXTURE_NAME)
    || string_equali(name, DEFAULT_SPECULAR_TEXTURE_NAME) || string_equali(name, DEFAULT_NORMAL_TEXTURE_NAME))
    {
        kwarng("Function '%s' called for default texture.", __FUNCTION__);
        return;
    }

    if(!texture_process_release(name))
    {
        kerror("Function '%s': Failed to release texture '%s' properly.", __FUNCTION__, name);
    }
}

texture* texture_system_wrap_internal(
    const char* name, u32 width, u32 height, u8 channel_count, bool has_transparency, bool is_writable,
    bool register_texture, void* internal_data
)
{
    // NOTE: Невозможно создать текстуру для цепочки обмена, так как система не инициализирована.
    // if(!texture_system_status_valid(__FUNCTION__) || !name) return false;

    u32 id = INVALID_ID;
    texture* t = null;

    if(register_texture)
    {
        if(!texture_process_acquire(name, TEXTURE_TYPE_2D, false, true, &id))
        {
            kwarng("Function '%s': Failed to obtain a new texture id.", __FUNCTION__);
            return null;
        }

        t = &state_ptr->textures[id];
    }
    else
    {
        t = kallocate_tc(texture, 1, MEMORY_TAG_TEXTURE);
        ktrace(
            "Function '%s': Created texture '%s', but not registering, resulting in an allocation. It is up to the called to free this memory.",
            __FUNCTION__, name
        );
    }

    string_ncopy(t->name, name, TEXTURE_NAME_MAX_LENGTH);
    t->width = width;
    t->height = height;
    t->channel_count = channel_count;
    t->type = TEXTURE_TYPE_2D;
    t->generation = INVALID_ID;
    t->flags |= has_transparency ? TEXTURE_FLAG_HAS_TRANSPARENCY : 0;
    t->flags |= is_writable ? TEXTURE_FLAG_IS_WRITABLE : 0;
    t->flags |= TEXTURE_FLAG_IS_WRAPPED;
    t->internal_data = internal_data;

    return t;
}

bool texture_system_set_internal(texture* t, void* internal_data)
{
    if(!texture_system_status_valid(__FUNCTION__) || !t) return false;

    t->internal_data = internal_data;
    t->generation++;
    return true;
}

bool texture_system_resize(texture* t, u32 width, u32 height, bool regenerate_internal_data)
{
    if(!texture_system_status_valid(__FUNCTION__) || !t) return false;

    if(!(t->flags & TEXTURE_FLAG_IS_WRITABLE))
    {
        kwarng("Function '%s' should not be called on textures that are not weitable.", __FUNCTION__);
        return false;
    }

    t->width = width;
    t->height = height;

    // Только для необернутых текстур.
    if(!(t->flags & TEXTURE_FLAG_IS_WRAPPED) && regenerate_internal_data)
    {
        renderer_texture_resize(t, width, height);
        return false;
    }

    t->generation++;
    return true;
}

bool texture_system_write_data(texture* t, u32 offset, u32 size, void* data)
{
    return false;
}

texture* texture_system_get_default_texture()
{
    if(!texture_system_status_valid(__FUNCTION__)) return null;
    return &state_ptr->default_texture;
}

texture* texture_system_get_default_diffuse_texture()
{
    if(!texture_system_status_valid(__FUNCTION__)) return null;
    return &state_ptr->default_diffuse_texture;
}

texture* texture_system_get_default_specular_texture()
{
    if(!texture_system_status_valid(__FUNCTION__)) return null;
    return &state_ptr->default_specular_texture;
}

texture* texture_system_get_default_normal_texture()
{
    if(!texture_system_status_valid(__FUNCTION__)) return null;
    return &state_ptr->default_normal_texture;
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
    state_ptr->default_texture.generation = INVALID_ID;
    state_ptr->default_texture.type = TEXTURE_TYPE_2D;
    state_ptr->default_texture.flags = 0;
    renderer_texture_create(&state_ptr->default_texture, pixels);
    state_ptr->default_texture.generation = INVALID_ID;

    // Diffuse.
    string_ncopy(state_ptr->default_diffuse_texture.name, DEFAULT_DIFFUSE_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    u8 diff_pixels[1024];
    kset_tc(diff_pixels, u8, 1024, 255);
    state_ptr->default_diffuse_texture.width = 16;
    state_ptr->default_diffuse_texture.height = 16;
    state_ptr->default_diffuse_texture.channel_count = 4;
    state_ptr->default_diffuse_texture.generation = INVALID_ID;
    state_ptr->default_diffuse_texture.type = TEXTURE_TYPE_2D;
    state_ptr->default_diffuse_texture.flags = 0;
    renderer_texture_create(&state_ptr->default_diffuse_texture, pixels);
    state_ptr->default_diffuse_texture.generation = INVALID_ID;

    // Specular.
    u8 spec_pixels[1024]; // w * h * channels
    kzero_tc(spec_pixels, u8, 1024);
    string_ncopy(state_ptr->default_specular_texture.name, DEFAULT_SPECULAR_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state_ptr->default_specular_texture.width = 16;
    state_ptr->default_specular_texture.height = 16;
    state_ptr->default_specular_texture.channel_count = 4;
    state_ptr->default_specular_texture.generation = INVALID_ID;
    state_ptr->default_specular_texture.type = TEXTURE_TYPE_2D;
    state_ptr->default_specular_texture.flags = 0;
    renderer_texture_create(&state_ptr->default_specular_texture, spec_pixels);
    state_ptr->default_specular_texture.generation = INVALID_ID;

    // Normal.
    u8 norm_pixels[1024];
    for(u64 i = 0; i < 1024; i += 4)
    {
        norm_pixels[i + 0] = 180;
        norm_pixels[i + 1] = 180;
        norm_pixels[i + 2] = 255;
        norm_pixels[i + 3] = 255;
    }
    string_ncopy(state_ptr->default_normal_texture.name, DEFAULT_NORMAL_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state_ptr->default_normal_texture.width = 16;
    state_ptr->default_normal_texture.height = 16;
    state_ptr->default_normal_texture.channel_count = 4;
    state_ptr->default_normal_texture.generation = INVALID_ID;
    state_ptr->default_normal_texture.type = TEXTURE_TYPE_2D;
    state_ptr->default_normal_texture.flags = 0;
    renderer_texture_create(&state_ptr->default_normal_texture, norm_pixels);
    state_ptr->default_normal_texture.generation = INVALID_ID;

    kfree(pixels, MEMORY_TAG_TEXTURE);
    return true;
}

void default_textures_destroy()
{
    texture_destroy(&state_ptr->default_texture);
    texture_destroy(&state_ptr->default_diffuse_texture);
    texture_destroy(&state_ptr->default_specular_texture);
    texture_destroy(&state_ptr->default_normal_texture);
}

bool texture_load_cube(const char* name, const char texture_names[6][TEXTURE_NAME_MAX_LENGTH], texture* t)
{
    u8* pixels = null;
    u64 image_size = 0;

    for(u8 i = 0; i < 6; ++i)
    {
        image_resouce_params params;
        params.flip_y = false;

        resource img_resource;
        if(!resource_system_load(texture_names[i], RESOURCE_TYPE_IMAGE, &params, &img_resource))
        {
            kerror("Function '%s': Failed to load image resource for texture '%s'.", __FUNCTION__, texture_names[i]);
            return false;
        }

        image_resouce_data* resource_data = img_resource.data;
        if(!pixels)
        {
            t->width = resource_data->width;
            t->height = resource_data->height;
            t->channel_count = resource_data->channel_count;
            t->flags = 0;
            t->generation = 0;

            image_size = t->width * t->height * t->channel_count;
            pixels = kallocate_tc(u8, image_size * 6, MEMORY_TAG_ARRAY);
            string_ncopy(t->name, name, TEXTURE_NAME_MAX_LENGTH);
        }
        else if(t->width != resource_data->width || t->height != resource_data->height || t->channel_count != resource_data->channel_count)
        {
            kerror("Function '%s': All textures must be the same resolution and bit depth.", __FUNCTION__);
            kfree(pixels, MEMORY_TAG_ARRAY);
            pixels = null;
            return false;
        }

        kcopy_tc(pixels + image_size * i, resource_data->pixels, u8, image_size);

        resource_system_unload(&img_resource);
    }

    // Загрузка текстуры в видеопамять (текстура с несколькими слоями).
    renderer_texture_create(t, pixels);

    kfree(pixels, MEMORY_TAG_ARRAY);
    pixels = null;

    return true;
}

void texture_load_job_success(void* params)
{
    // TODO:
    // NOTE: Контекст задачи для текстуры.
    texture_load_params* texture_params = params;
    image_resouce_data* resource_data = texture_params->image_resource.data;

    // Копирование данных текстуры.
    texture_params->out_texture->width = resource_data->width;
    texture_params->out_texture->height = resource_data->height;
    texture_params->out_texture->channel_count = resource_data->channel_count;

    // Проверка прозрачности.
    u64 total_size = resource_data->width * resource_data->height * resource_data->channel_count;
    bool has_transparency = false;
    for(u64 i = 0; i < total_size; i += resource_data->channel_count)
    {
        u8 a = resource_data->pixels[i + 3];
        if(a < 255)
        {
            has_transparency = true;
            break;
        }
    }

    string_ncopy(texture_params->out_texture->name, texture_params->resource_name, TEXTURE_NAME_MAX_LENGTH);
    texture_params->out_texture->generation = 0;
    texture_params->out_texture->flags |= has_transparency ? TEXTURE_FLAG_HAS_TRANSPARENCY : 0;

    // Загрузка текстуры на GPU.
    renderer_texture_create(texture_params->out_texture, resource_data->pixels);

    // TODO: Только в этот момент разрешить смену текстуры!

    // Очистка загруженных ресурсов.
    resource_system_unload(&texture_params->image_resource);
    if(texture_params->resource_name)
    {
        string_free(texture_params->resource_name);
    }
}

void texture_load_job_fail(void* params)
{
    texture_load_params* texture_params = params;
    kerror("Function '%s': Failed to load texture '%s'.", __FUNCTION__, texture_params->resource_name);

    resource_system_unload(&texture_params->image_resource);
    if(texture_params->resource_name)
    {
        string_free(texture_params->resource_name);
    }
}

// TODO: Нет обновления имени в хэш таблице.
bool texture_load_job(void* params, void* result_data)
{
    texture_load_params* load_params = params;
    image_resouce_params resource_params;
    resource_params.flip_y = true;

    bool result = resource_system_load(load_params->resource_name, RESOURCE_TYPE_IMAGE, &resource_params, &load_params->image_resource);

    // NOTE: Теже параметры используются и для результата.
    kcopy_tc(result_data, load_params, struct texture_load_params, 1);

    return result;
}

bool texture_load(const char* texture_name, texture* t)
{
    texture_load_params params;
    params.resource_name = string_duplicate(texture_name); // TODO: Выглядит крайне плохо!
    params.out_texture = t; // TODO: Проверить не теряется ли адрес текстуры?
    params.image_resource = (resource){};
    params.current_generation = t->generation;

    job job = job_create_default(texture_load_job, texture_load_job_success, texture_load_job_fail, &params, sizeof(texture_load_params), sizeof(texture_load_params));
    job_system_submit(&job);
    return true;
}

void texture_destroy(texture* t)
{
    // Удаление из памяти графического процессора.
    renderer_texture_destroy(t);

    // Освобождение памяти для новой текстуры.
    kzero_tc(t, texture, 1);
    t->id = INVALID_ID;
    t->generation = INVALID_ID;
}

bool texture_process_acquire(const char* name, texture_type type, bool auto_release, bool skip_load, u32* out_texture_id)
{
    texture_reference ref;

    // Когда текстуры нет или запись помечена как не действительная, то это момент создания новой текстуры.
    if(!hashtable_get(state_ptr->texture_references_table, name, &ref) || ref.id == INVALID_ID)
    {
        ref.reference_count = 0;
        ref.auto_release = auto_release;

        // Поиск свободной памяти для текстуры.
        for(u32 i = 0; i < state_ptr->config.max_texture_count; ++i)
        {
            if(state_ptr->textures[i].id == INVALID_ID)
            {
                ref.id = i;
                break;
            }
        }

        // Если свободный участок памяти не найден.
        if(ref.id == INVALID_ID)
        {
            kerror(
                "Function '%s': Texture system cannot hold anymore textures. Adjust configuration to allow more.",
                __FUNCTION__
            );
            *out_texture_id = INVALID_ID;
            return false;
        }

        texture* t = &state_ptr->textures[ref.id];
        t->id = ref.id;
        t->type = type;

        if(type == TEXTURE_TYPE_CUBE)
        {
            // +X, -X, +Y, -Y, +Z, -Z.
            char texture_names[6][TEXTURE_NAME_MAX_LENGTH];
            string_format_unsafe(texture_names[0], "%s_r", name); // Правая текстура.
            string_format_unsafe(texture_names[1], "%s_l", name); // Левая текстура.
            string_format_unsafe(texture_names[2], "%s_u", name); // Верхняя текстура.
            string_format_unsafe(texture_names[3], "%s_d", name); // Нижняя текстура.
            string_format_unsafe(texture_names[4], "%s_f", name); // Передняя текстура (Фронтовая).
            string_format_unsafe(texture_names[5], "%s_b", name); // Задняя текстура (Тыловая).

            if(!skip_load && !texture_load_cube(name, texture_names, t))
            {
                kerror("Function '%s': Failed to load texture cube '%s'.", __FUNCTION__, name);
                *out_texture_id = INVALID_ID;
                return false;
            }
        }
        else
        {
            // Создание текстуры.
            if(!skip_load && !texture_load(name, t))
            {
                kerror("Function '%s': Failed to load texture '%s'.", __FUNCTION__, name);
                *out_texture_id = INVALID_ID;
                return false;
            }
        }

        // ktrace(
        //     "Function '%s': Texture '%s' does not exist. Created, and reference count is now %i.",
        //     __FUNCTION__, name, ref.reference_count
        // );
    }
    else
    {
        // ktrace(
        //     "Function '%s': Texture '%s' already exists, and reference count increased to %i.",
        //     __FUNCTION__, name, ref.reference_count
        // );
    }

    ref.reference_count++;

    // TODO: hash таблица update function!
    // Обновление ссылки на текстуру.
    if(!hashtable_set(state_ptr->texture_references_table, name, &ref, true))
    {
        kerror("Function '%s' Failed to update texture reference.", __FUNCTION__);
        *out_texture_id = INVALID_ID;
        return false;
    }

    *out_texture_id = ref.id;
    return true;
}

bool texture_process_release(const char* name)
{
    texture_reference ref;
    if(!hashtable_get(state_ptr->texture_references_table, name, &ref) || ref.reference_count == 0)
    {
        kwarng("Function '%s': Tried to release non-existent texture '%s'.", __FUNCTION__, name);
        return false;
    }

    ref.reference_count--;

    if(ref.reference_count == 0 && ref.auto_release)
    {
        texture* t = &state_ptr->textures[ref.id];

        // Освобождение/восстановление памяти текстуры для новой.
        texture_destroy(t);

        // Освобождение ссылки.
        ref.id = INVALID_ID;
        ref.auto_release = false;

        // ktrace(
        //     "Function '%s': Released texture '%s', because reference count is 0 and auto release used.",
        //     __FUNCTION__, name
        // );
    }
    else
    {
        // ktrace(
        //     "Function '%s': Released texture '%s', now has a reference count is %i and auto release is %s.",
        //     __FUNCTION__, name, ref.reference_count, ref.auto_release ? "used" : "unused"
        // );
    }

    // TODO: hash таблица update function!
    // Обновление ссылки на текстуру.
    if(!hashtable_set(state_ptr->texture_references_table, name, &ref, true))
    {
        kerror("Function '%s' Failed to update texture reference.", __FUNCTION__);
        return false;
    }

    return true;
}
