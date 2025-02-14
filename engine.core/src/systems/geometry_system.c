// Собственные подключения.
#include "systems/geometry_system.h"
#include "systems/material_system.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "kstring.h"
#include "renderer/renderer_frontend.h"

typedef struct geometry_reference {
    // @brief Количество ссылок на геометрию.
    u64 reference_count;
    // @brief Геометрия.
    geometry geometry;
    // @brief Авто уничтожение геометрии.
    bool auto_release;
} geometry_reference;

typedef struct geometry_system_state {
    // @brief Конфигурация системы.
    geometry_system_config config;
    // @brief Геомертия по умолчанию.
    geometry default_geometry;
    // @brief Массив геометрий.
    geometry_reference* geometries;
} geometry_system_state;

static geometry_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the geometry system to be initialized. Call 'geometry_system_initialize' first.";

bool default_geometries_create();
bool geometry_create(geometry_config* config, geometry* g);
void geometry_destroy(geometry* g);

bool geometry_system_initialize(u64* memory_requirement, void* memory, geometry_system_config* config)
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

    if(!config->max_geometry_count)
    {
        kerror("Function '%s': config.max_geometry_count must be greater then zero. Return false!", __FUNCTION__);
        return false;
    }

    u64 state_requirement = sizeof(geometry_system_state);
    u64 geometries_requirement = sizeof(geometry_reference) * config->max_geometry_count;
    *memory_requirement = state_requirement + geometries_requirement;

    if(!memory)
    {
        return true;
    }

    // Обнуление заголовка системы геометрий.
    kzero_tc(memory, geometry_system_state, 1);
    state_ptr = memory;

    // Запись данных конфигурации системы.
    state_ptr->config.max_geometry_count = config->max_geometry_count;

    // Получение и запись указателя на блок геометрий.
    void* geometries_block = (void*)((u8*)state_ptr + state_requirement);
    state_ptr->geometries = geometries_block;

    // Отмечает все геометрии как недействительные.
    for(u32 i = 0; i < state_ptr->config.max_geometry_count; ++i)
    {
        state_ptr->geometries[i].geometry.id = INVALID_ID;
        state_ptr->geometries[i].geometry.generation = INVALID_ID;
        state_ptr->geometries[i].geometry.internal_id = INVALID_ID;
    }

    // Создание геометрии по-умолчанию.
    if(!default_geometries_create())
    {
        kerror("Function '%s': Failed to create default geometries.", __FUNCTION__);
        return false;
    }

    return true;
}

void geometry_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // NOTE: Нечего уничтожать.

    state_ptr = null;
}

geometry* geometry_system_acquire_by_id(u32 id)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return null;
    }

    if(id != INVALID_ID && state_ptr->geometries[id].geometry.id != INVALID_ID)
    {
        state_ptr->geometries[id].reference_count++;
        return &state_ptr->geometries[id].geometry;
    }

    kerror("Function '%s': Cannon load invalid geometry id. Return null!", __FUNCTION__);
    return null;
}

geometry* geometry_system_acquire_from_config(geometry_config* config, bool auto_release)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return null;
    }

    geometry* g = null;

    for(u32 i = 0; i < state_ptr->config.max_geometry_count; ++i)
    {
        geometry_reference* ref = &state_ptr->geometries[i];
        if(ref->geometry.id == INVALID_ID)
        {
            ref->auto_release = auto_release;
            ref->reference_count = 1;

            g = &ref->geometry;
            g->id = i;
            break;
        }
    }

    if(!g)
    {
        kerror(
            "Function '%s': Unable to obtain free slot for geometry. Adjust configuration to allow more space. Return null!",
            __FUNCTION__
        );
    }

    if(!geometry_create(config, g))
    {
        kerror("Function '%s': Failed to create geometry. Return null!", __FUNCTION__);
        return null;
    }

    return g;
}

void geometry_system_release(geometry* geometry)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    if(geometry->id == INVALID_ID)
    {
        kwarng("Function '%s': Tried to release non-existent geometry.", __FUNCTION__);
        return;
    }

    geometry_reference* ref = &state_ptr->geometries[geometry->id];

    ref->reference_count--;

    if(ref->reference_count == 0 && ref->auto_release)
    {
        // Уничтожение геометрии.
        geometry_destroy(&ref->geometry);

        // Освобождение/восстановление памяти геометрии для новой.
        ref->reference_count = 0;
        ref->auto_release = false;
    }
}

geometry* geometry_system_get_default()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return null;
    }

    return &state_ptr->default_geometry;
}

bool default_geometries_create()
{
    #define VERT_COUNT 4
    vertex_3d verts[VERT_COUNT];
    kzero_tc(verts, vertex_3d, VERT_COUNT);

    const f32 f = 10.0f;

    verts[0].position.x = -0.5f * f;
    verts[0].position.y = -0.5f * f;
    verts[0].texcoord.x =  0.0f;
    verts[0].texcoord.y =  0.0f;

    verts[1].position.x =  0.5f * f;
    verts[1].position.y =  0.5f * f;
    verts[1].texcoord.x =  1.0f;
    verts[1].texcoord.y =  1.0f;

    verts[2].position.x = -0.5f * f;
    verts[2].position.y =  0.5f * f;
    verts[2].texcoord.x =  0.0f;
    verts[2].texcoord.y =  1.0f;

    verts[3].position.x =  0.5f * f;
    verts[3].position.y = -0.5f * f;
    verts[3].texcoord.x =  1.0f;
    verts[3].texcoord.y =  0.0f;

    #define INDEX_COUNT 6
    u32 indices[INDEX_COUNT] = { 0, 1, 2, 0, 3, 1 };

    if(!renderer_create_geometry(&state_ptr->default_geometry, VERT_COUNT, verts, INDEX_COUNT, indices))
    {
        kerror("Function '%s': Failed to create default geometries.", __FUNCTION__);
        return false;
    }

    // Установка материала по умолчанию.
    state_ptr->default_geometry.material = material_system_get_default();

    return true;
}

bool geometry_create(geometry_config* config, geometry* g)
{
    // Загрузка геометрии в память графического процессора.
    if(!renderer_create_geometry(g, config->vertex_count, config->vertices, config->index_count, config->indices))
    {
        state_ptr->geometries[g->id].reference_count = 0;
        state_ptr->geometries[g->id].auto_release = false;
        g->id = INVALID_ID;
        g->internal_id = INVALID_ID;
        g->generation = INVALID_ID;

        return false;
    }

    // TODO: В системах не хватает проверок ввода пользователя!
    //       Проверку строкуи внесит в систему материалов! Вообще работа со строками медленная!!!
    //       Работу со строками заменить на что-то универсальное!
    if(string_length(config->material_name) > 0)
    {
        g->material = material_system_acquire(config->material_name);

        if(!g->material)
        {
            g->material = material_system_get_default();
        }
    }

    return true;
}

void geometry_destroy(geometry* g)
{
    // Уничтожение геометрии в памяти графического процессора.
    renderer_destroy_geometry(g);

    g->id = INVALID_ID;
    g->internal_id = INVALID_ID;
    g->generation = INVALID_ID;

    string_empty(g->name);

    if(g->material && string_length(g->material->name))
    {
        material_system_release(g->material->name);
        g->material = null;
    }
}

geometry_config geometry_system_generate_plane_config(
    f32 width, f32 height, u32 x_segment_count, u32 y_segment_count, f32 tile_x, f32 tile_y,
    const char* name, const char* material_name
)
{
    if(!width)
    {
        kwarng("Function '%s': Width must be nonzero. Default to one.", __FUNCTION__);
        width = 1.0f;
    }

    if(!height)
    {
        kwarng("Function '%s': Heigth must be nonzero. Default to one.", __FUNCTION__);
        height = 1.0f;
    }

    if(x_segment_count < 1)
    {
        kwarng("Function '%s': x_segment_count must be a positive number. Default to one.", __FUNCTION__);
        x_segment_count = 1;
    }

    if(y_segment_count < 1)
    {
        kwarng("Function '%s': y_segment_count must be a positive number. Default to one.", __FUNCTION__);
        y_segment_count = 1;
    }

    if(!tile_x)
    {
        kwarng("Function '%s': tile_x must be nonzero. Default to one.", __FUNCTION__);
        tile_x = 1.0f;
    }

    if(!tile_y)
    {
        kwarng("Function '%s': tile_y must be nonzero. Default to one.", __FUNCTION__);
        tile_y = 1.0f;
    }

    // NOTE: В поисках искажения координат по оси z! Не забывай ОБНУЛЯТЬ выделенную ПАМЯТЬ,
    //       хоть через СТЕК, хоть через ХИП!!!!!!!!!!!!!
    geometry_config config;
    kzero_tc(&config, geometry_config, 1);
    config.vertex_count = x_segment_count * y_segment_count * 4;
    config.vertices = kallocate_tc(vertex_3d, config.vertex_count, MEMORY_TAG_ARRAY);
    kzero_tc(config.vertices, vertex_3d, config.vertex_count);
    config.index_count = x_segment_count * y_segment_count * 6;
    config.indices = kallocate_tc(u32, config.index_count, MEMORY_TAG_ARRAY);
    kzero_tc(config.indices, u32, config.index_count);

    f32 seg_width = width / x_segment_count;
    f32 seg_height = height / y_segment_count;
    f32 half_width = width * 0.5f;
    f32 half_height = height * 0.5f;

    for(u32 y = 0; y < y_segment_count; ++y)
    {
        for(u32 x = 0 ; x < x_segment_count; ++x)
        {
            // Генерация вершин.
            f32 min_x = (x * seg_width) - half_width;
            f32 min_y = (y * seg_height) - half_height;
            f32 max_x = min_x + seg_width;
            f32 max_y = min_y + seg_height;

            f32 min_uvx = (x / (f32)x_segment_count) * tile_x;
            f32 min_uvy = (y / (f32)y_segment_count) * tile_y;
            f32 max_uvx = ((x + 1) / (f32)x_segment_count) * tile_x;
            f32 max_uvy = ((y + 1) / (f32)y_segment_count) * tile_y;

            u32 v_offset = ((y * x_segment_count) + x) * 4;
            vertex_3d* v0 = &config.vertices[v_offset + 0];
            vertex_3d* v1 = &config.vertices[v_offset + 1];
            vertex_3d* v2 = &config.vertices[v_offset + 2];
            vertex_3d* v3 = &config.vertices[v_offset + 3];

            v0->position.x = min_x;
            v0->position.y = min_y;
            v0->texcoord.x = min_uvx;
            v0->texcoord.y = min_uvy;

            v1->position.x = max_x;
            v1->position.y = max_y;
            v1->texcoord.x = max_uvx;
            v1->texcoord.y = max_uvy;

            v2->position.x = min_x;
            v2->position.y = max_y;
            v2->texcoord.x = min_uvx;
            v2->texcoord.y = max_uvy;

            v3->position.x = max_x;
            v3->position.y = min_y;
            v3->texcoord.x = max_uvx;
            v3->texcoord.y = min_uvy;

            // Генерация индексов.
            u32 i_offset = ((y * x_segment_count) + x) * 6;
            config.indices[i_offset + 0] = v_offset + 0;
            config.indices[i_offset + 1] = v_offset + 1;
            config.indices[i_offset + 2] = v_offset + 2;
            config.indices[i_offset + 3] = v_offset + 0;
            config.indices[i_offset + 4] = v_offset + 3;
            config.indices[i_offset + 5] = v_offset + 1;
        }
    }

    if(name && string_length(name) > 0)
    {
        string_ncopy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    }
    else
    {
        string_ncopy(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
    }

    if(material_name && string_length(material_name) > 0)
    {
        string_ncopy(config.material_name, material_name, MATERIAL_NAME_MAX_LENGTH);
    }
    else
    {
        string_ncopy(config.material_name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    }

    return config;
}
