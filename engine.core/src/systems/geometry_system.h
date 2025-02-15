#pragma once

#include <defines.h>
#include <resources/resource_types.h>

#define DEFAULT_GEOMETRY_NAME "default"

typedef struct geometry_system_config {
    // @brief Максимальное количество геометрий.
    // NOTE: Должно быть значительно больше, чем число статических сеток,
    //       потому что их может быть и будет больше одной на сетку.
    //       Примите во внимание и другие системы.
    u32 max_geometry_count;
} geometry_system_config;

typedef struct geometry_config {
    u32 vetrex_size;
    u32 vertex_count;
    void* vertices;
    u32 index_size;
    u32 index_count;
    void* indices;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char material_name[MATERIAL_NAME_MAX_LENGTH];
} geometry_config;

/*
*/
bool geometry_system_initialize(u64* memory_requirement, void* memory, geometry_system_config* config);

/*
*/
void geometry_system_shutdown();

/*
*/
geometry* geometry_system_acquire_by_id(u32 id);

/*
*/
geometry* geometry_system_acquire_from_config(geometry_config* config, bool auto_release);

/*
*/
void geometry_system_release(geometry* geometry);

/*
*/
geometry* geometry_system_get_default();

/*
*/
geometry* geometry_system_get_default_2d();

/*
    @brief Создает конфигурацию для плоской геометрии с учетом предоставленных параметров.
    NOTE: Массивы вершин и индексов динамически выделяются и должны быть освобождены при
          утилизации объекта. Таким образом, это не следует считать производительным кодом.
    
*/
geometry_config geometry_system_generate_plane_config(
    f32 width, f32 height, u32 x_segment_count, u32 y_segment_count, f32 tile_x, f32 tile_y,
    const char* name, const char* material_name
);
