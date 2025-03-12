// Собственные подключения.
#include "systems/geometry_system.h"
#include "systems/material_system.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "math/geometry_utils.h"
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
    // @brief Геометрия по умолчанию 2d.
    geometry default_2d_geometry;
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

    // Создание геометрии по умолчанию.
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

void geometry_system_config_dispose(geometry_config* config)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    if(!config)
    {
        kerror("Function '%s' requires a valid pointer to config.", __FUNCTION__);
        return;
    }

    if(config->vertices)
    {
        kfree(config->vertices, config->vertex_size * config->vertex_count, MEMORY_TAG_ARRAY);
    }

    if(config->indices)
    {
        kfree(config->indices, config->index_size * config->index_count, MEMORY_TAG_ARRAY);
    }

    kzero_tc(config, geometry_config, 1);
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
        kwarng("Function '%s' cannot release invalid geometry id.", __FUNCTION__);
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

geometry* geometry_system_get_default_2d()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return null;
    }

    return &state_ptr->default_2d_geometry;
}

bool default_geometries_create()
{
    #define VERT_COUNT 4
    #define INDEX_COUNT 6
    const f32 f = 10.0f;

    vertex_3d verts[VERT_COUNT];
    kzero_tc(verts, vertex_3d, VERT_COUNT);

    verts[0].position.x = -0.5f * f; // 2    1
    verts[0].position.y = -0.5f * f; //
    verts[0].texcoord.x =  0.0f;     //
    verts[0].texcoord.y =  0.0f;     // 0    3

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

    // Индексы должны идти против часовой.
    u32 indices[INDEX_COUNT] = { 0, 1, 2, 0, 3, 1 };

    // Настройка геометрии по умолчанию.
    state_ptr->default_geometry.material = material_system_get_default();
    state_ptr->default_geometry.internal_id = INVALID_ID;

    if(!renderer_create_geometry(
        &state_ptr->default_geometry, sizeof(vertex_3d), VERT_COUNT, verts, sizeof(u32), INDEX_COUNT, indices
    ))
    {
        kerror("Function '%s': Failed to create default geometry.", __FUNCTION__);
        return false;
    }

    // Создание 2d геометрии.
    vertex_2d verts2d[VERT_COUNT];
    kzero_tc(verts2d, vertex_2d, VERT_COUNT);

    verts2d[0].position.x = -0.5f * f;  // 0    3
    verts2d[0].position.y = -0.5f * f;  //
    verts2d[0].texcoord.x =  0.0f;      //
    verts2d[0].texcoord.y =  0.0f;      // 2    1

    verts2d[1].position.x =  0.5f * f;
    verts2d[1].position.y =  0.5f * f;
    verts2d[1].texcoord.x =  1.0f;
    verts2d[1].texcoord.y =  1.0f;

    verts2d[2].position.x = -0.5f * f;
    verts2d[2].position.y =  0.5f * f;
    verts2d[2].texcoord.x =  0.0f;
    verts2d[2].texcoord.y =  1.0f;

    verts2d[3].position.x =  0.5f * f;
    verts2d[3].position.y = -0.5f * f;
    verts2d[3].texcoord.x =  1.0f;
    verts2d[3].texcoord.y =  0.0f;

    // Индексы должны идти против часовой.
    u32 indices2d[INDEX_COUNT] = { 2, 1, 0, 3, 0, 1 };

    // Настройка 2d геометрии по умолчанию.
    state_ptr->default_2d_geometry.material = material_system_get_default();
    state_ptr->default_2d_geometry.internal_id = INVALID_ID;

    if(!renderer_create_geometry(
        &state_ptr->default_2d_geometry, sizeof(vertex_2d), VERT_COUNT, verts2d, sizeof(u32), INDEX_COUNT, indices2d
    ))
    {
        kerror("Function '%s': Failed to create default 2d geometry.", __FUNCTION__);
        return false;
    }

    return true;
}

bool geometry_create(geometry_config* config, geometry* g)
{
    // Загрузка геометрии в память графического процессора.
    if(!renderer_create_geometry(
        g, config->vertex_size, config->vertex_count, config->vertices, config->index_size, config->index_count,
        config->indices
    ))
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

    geometry_config config;

    kzero_tc(&config, geometry_config, 1);
    config.vertex_size = sizeof(vertex_3d);
    config.vertex_count = x_segment_count * y_segment_count * 4;
    config.vertices = kallocate_tc(vertex_3d, config.vertex_count, MEMORY_TAG_ARRAY);
    kzero_tc(config.vertices, vertex_3d, config.vertex_count);

    config.index_size = sizeof(u32);
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
            vertex_3d* v0 = &((vertex_3d*)config.vertices)[v_offset + 0];
            vertex_3d* v1 = &((vertex_3d*)config.vertices)[v_offset + 1];
            vertex_3d* v2 = &((vertex_3d*)config.vertices)[v_offset + 2];
            vertex_3d* v3 = &((vertex_3d*)config.vertices)[v_offset + 3];

            v0->position = (vec3){{ min_x, min_y, 0 }};
            v0->texcoord = (vec2){{ min_uvx, min_uvy }};
            v0->normal   = (vec3){{ 0, 0, 1}};

            v1->position = (vec3){{ max_x, max_y, 0 }};
            v1->texcoord = (vec2){{ max_uvx, max_uvy }};
            v1->normal   = (vec3){{ 0, 0, 1}};

            v2->position = (vec3){{ min_x, max_y, 0 }};
            v2->texcoord = (vec2){{ min_uvx, max_uvy }};
            v2->normal   = (vec3){{ 0, 0, 1}};

            v3->position = (vec3){{ max_x, min_y, 0 }};
            v3->texcoord = (vec2){{ max_uvx, min_uvy }};
            v3->normal   = (vec3){{ 0, 0, 1}};

            // Генерация индексов.
            u32 i_offset = ((y * x_segment_count) + x) * 6;
            ((u32*)config.indices)[i_offset + 0] = v_offset + 0;
            ((u32*)config.indices)[i_offset + 1] = v_offset + 1;
            ((u32*)config.indices)[i_offset + 2] = v_offset + 2;
            ((u32*)config.indices)[i_offset + 3] = v_offset + 0;
            ((u32*)config.indices)[i_offset + 4] = v_offset + 3;
            ((u32*)config.indices)[i_offset + 5] = v_offset + 1;
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

    geometry_generate_tangent(config.vertex_count, config.vertices, config.index_count, config.indices);

    return config;
}

geometry_config geometry_system_generate_cube_config(
    f32 width, f32 height, f32 depth, f32 tile_x, f32 tile_y, const char* name, const char* material_name
)
{
    if(width == 0)
    {
        kwarng("Function '%s': Width must be nonzero. Defaulting to one.", __FUNCTION__);
        width = 1.0f;
    }

    if(height == 0)
    {
        kwarng("Function '%s': Height must be nonzero. Defaulting to one.", __FUNCTION__);
        height = 1.0f;
    }

    if(depth == 0)
    {
        kwarng("Function '%s': Depth must be nonzero. Defaulting to one.", __FUNCTION__);
        depth = 1;
    }

    if(tile_x == 0)
    {
        kwarng("Function '%s': tile_x must be nonzero. Defaulting to one.", __FUNCTION__);
        tile_x = 1.0f;
    }

    if(tile_y == 0)
    {
        kwarng("Function '%s': tile_y must be nonzero. Defaulting to one.", __FUNCTION__);
        tile_y = 1.0f;
    }

    geometry_config config;
    config.vertex_size = sizeof(vertex_3d);
    config.vertex_count = 4 * 6;
    config.vertices = kallocate_tc(vertex_3d, config.vertex_count, MEMORY_TAG_ARRAY);
    config.index_size = sizeof(u32);
    config.index_count = 6 * 6;
    config.indices = kallocate_tc(u32, config.index_count, MEMORY_TAG_ARRAY);

    f32 half_width  =  width  * 0.5f;
    f32 half_height =  height * 0.5f;
    f32 half_depth  =  depth  * 0.5f;
    f32 min_x       = -half_width;
    f32 min_y       = -half_height;
    f32 min_z       = -half_depth;
    f32 max_x       =  half_width;
    f32 max_y       =  half_height;
    f32 max_z       =  half_depth;
    f32 min_uvx     =  0.0f;
    f32 min_uvy     =  0.0f;
    f32 max_uvx     =  tile_x;
    f32 max_uvy     =  tile_y;

    vertex_3d verts[24];
    // Вид спереди.
    verts[(0 * 4) + 0].position = (vec3){{min_x, min_y, max_z}};
    verts[(0 * 4) + 1].position = (vec3){{max_x, max_y, max_z}};
    verts[(0 * 4) + 2].position = (vec3){{min_x, max_y, max_z}};
    verts[(0 * 4) + 3].position = (vec3){{max_x, min_y, max_z}};
    verts[(0 * 4) + 0].texcoord = (vec2){{min_uvx, min_uvy}};
    verts[(0 * 4) + 1].texcoord = (vec2){{max_uvx, max_uvy}};
    verts[(0 * 4) + 2].texcoord = (vec2){{min_uvx, max_uvy}};
    verts[(0 * 4) + 3].texcoord = (vec2){{max_uvx, min_uvy}};
    verts[(0 * 4) + 0].normal   = (vec3){{0.0f, 0.0f, 1.0f}};
    verts[(0 * 4) + 1].normal   = (vec3){{0.0f, 0.0f, 1.0f}};
    verts[(0 * 4) + 2].normal   = (vec3){{0.0f, 0.0f, 1.0f}};
    verts[(0 * 4) + 3].normal   = (vec3){{0.0f, 0.0f, 1.0f}};

    // Вид сзади.
    verts[(1 * 4) + 0].position = (vec3){{max_x, min_y, min_z}};
    verts[(1 * 4) + 1].position = (vec3){{min_x, max_y, min_z}};
    verts[(1 * 4) + 2].position = (vec3){{max_x, max_y, min_z}};
    verts[(1 * 4) + 3].position = (vec3){{min_x, min_y, min_z}};
    verts[(1 * 4) + 0].texcoord = (vec2){{min_uvx, min_uvy}};
    verts[(1 * 4) + 1].texcoord = (vec2){{max_uvx, max_uvy}};
    verts[(1 * 4) + 2].texcoord = (vec2){{min_uvx, max_uvy}};
    verts[(1 * 4) + 3].texcoord = (vec2){{max_uvx, min_uvy}};
    verts[(1 * 4) + 0].normal   = (vec3){{0.0f, 0.0f, -1.0f}};
    verts[(1 * 4) + 1].normal   = (vec3){{0.0f, 0.0f, -1.0f}};
    verts[(1 * 4) + 2].normal   = (vec3){{0.0f, 0.0f, -1.0f}};
    verts[(1 * 4) + 3].normal   = (vec3){{0.0f, 0.0f, -1.0f}};

    // Вид слева.
    verts[(2 * 4) + 0].position = (vec3){{min_x, min_y, min_z}};
    verts[(2 * 4) + 1].position = (vec3){{min_x, max_y, max_z}};
    verts[(2 * 4) + 2].position = (vec3){{min_x, max_y, min_z}};
    verts[(2 * 4) + 3].position = (vec3){{min_x, min_y, max_z}};
    verts[(2 * 4) + 0].texcoord = (vec2){{min_uvx, min_uvy}};
    verts[(2 * 4) + 1].texcoord = (vec2){{max_uvx, max_uvy}};
    verts[(2 * 4) + 2].texcoord = (vec2){{min_uvx, max_uvy}};
    verts[(2 * 4) + 3].texcoord = (vec2){{max_uvx, min_uvy}};
    verts[(2 * 4) + 1].normal   = (vec3){{-1.0f, 0.0f, 0.0f}};
    verts[(2 * 4) + 0].normal   = (vec3){{-1.0f, 0.0f, 0.0f}};
    verts[(2 * 4) + 2].normal   = (vec3){{-1.0f, 0.0f, 0.0f}};
    verts[(2 * 4) + 3].normal   = (vec3){{-1.0f, 0.0f, 0.0f}};

    // Вид справа.
    verts[(3 * 4) + 0].position = (vec3){{max_x, min_y, max_z}};
    verts[(3 * 4) + 1].position = (vec3){{max_x, max_y, min_z}};
    verts[(3 * 4) + 2].position = (vec3){{max_x, max_y, max_z}};
    verts[(3 * 4) + 3].position = (vec3){{max_x, min_y, min_z}};
    verts[(3 * 4) + 0].texcoord = (vec2){{min_uvx, min_uvy}};
    verts[(3 * 4) + 1].texcoord = (vec2){{max_uvx, max_uvy}};
    verts[(3 * 4) + 2].texcoord = (vec2){{min_uvx, max_uvy}};
    verts[(3 * 4) + 3].texcoord = (vec2){{max_uvx, min_uvy}};
    verts[(3 * 4) + 0].normal   = (vec3){{1.0f, 0.0f, 0.0f}};
    verts[(3 * 4) + 1].normal   = (vec3){{1.0f, 0.0f, 0.0f}};
    verts[(3 * 4) + 2].normal   = (vec3){{1.0f, 0.0f, 0.0f}};
    verts[(3 * 4) + 3].normal   = (vec3){{1.0f, 0.0f, 0.0f}};

    // Вид снизу.
    verts[(4 * 4) + 0].position = (vec3){{max_x, min_y, max_z}};
    verts[(4 * 4) + 1].position = (vec3){{min_x, min_y, min_z}};
    verts[(4 * 4) + 2].position = (vec3){{max_x, min_y, min_z}};
    verts[(4 * 4) + 3].position = (vec3){{min_x, min_y, max_z}};
    verts[(4 * 4) + 0].texcoord = (vec2){{min_uvx, min_uvy}};
    verts[(4 * 4) + 1].texcoord = (vec2){{max_uvx, max_uvy}};
    verts[(4 * 4) + 2].texcoord = (vec2){{min_uvx, max_uvy}};
    verts[(4 * 4) + 3].texcoord = (vec2){{max_uvx, min_uvy}};
    verts[(4 * 4) + 0].normal   = (vec3){{0.0f, -1.0f, 0.0f}};
    verts[(4 * 4) + 1].normal   = (vec3){{0.0f, -1.0f, 0.0f}};
    verts[(4 * 4) + 2].normal   = (vec3){{0.0f, -1.0f, 0.0f}};
    verts[(4 * 4) + 3].normal   = (vec3){{0.0f, -1.0f, 0.0f}};

    // Вид сверху.
    verts[(5 * 4) + 0].position = (vec3){{min_x, max_y, max_z}};
    verts[(5 * 4) + 1].position = (vec3){{max_x, max_y, min_z}};
    verts[(5 * 4) + 2].position = (vec3){{min_x, max_y, min_z}};
    verts[(5 * 4) + 3].position = (vec3){{max_x, max_y, max_z}};
    verts[(5 * 4) + 0].texcoord = (vec2){{min_uvx, min_uvy}};
    verts[(5 * 4) + 1].texcoord = (vec2){{max_uvx, max_uvy}};
    verts[(5 * 4) + 2].texcoord = (vec2){{min_uvx, max_uvy}};
    verts[(5 * 4) + 3].texcoord = (vec2){{max_uvx, min_uvy}};
    verts[(5 * 4) + 0].normal   = (vec3){{0.0f, 1.0f, 0.0f}};
    verts[(5 * 4) + 1].normal   = (vec3){{0.0f, 1.0f, 0.0f}};
    verts[(5 * 4) + 2].normal   = (vec3){{0.0f, 1.0f, 0.0f}};
    verts[(5 * 4) + 3].normal   = (vec3){{0.0f, 1.0f, 0.0f}};

    kcopy(config.vertices, verts, config.vertex_size * config.vertex_count);

    for(u32 i = 0; i < 6; ++i)
    {
        u32 v_offset = i * 4;
        u32 i_offset = i * 6;
        ((u32*)config.indices)[i_offset + 0] = v_offset + 0;
        ((u32*)config.indices)[i_offset + 1] = v_offset + 1;
        ((u32*)config.indices)[i_offset + 2] = v_offset + 2;
        ((u32*)config.indices)[i_offset + 3] = v_offset + 0;
        ((u32*)config.indices)[i_offset + 4] = v_offset + 3;
        ((u32*)config.indices)[i_offset + 5] = v_offset + 1;
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

    geometry_generate_tangent(config.vertex_count, config.vertices, config.index_count, config.indices);

    return config;
}
