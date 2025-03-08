#pragma once

#include <defines.h>
#include <math/math_types.h>

/*
    @brief Вычислияет нормали для заданных данных вершины и индекса. Изменяет вершины.
    @param vertex_count Количество вершин.
    @param vertices Массив вершин.
    @param index_count Количество индексов.
    @param indices Массив индексов.
*/
void geometry_generate_normals(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices);

/*
    @brief Вычислияет тангенты для заданных данных вершины и индекса. Изменяет вершины.
    @param vertex_count Количество вершин.
    @param vertices Массив вершин.
    @param index_count Количество индексов.
    @param indices Массив индексов.
*/
void geometry_generate_tangent(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices);

/*
    @brief Убирает дубликаты вершин, оставляя только уникальные и возращает новый массив вершин.
    @param vertex_count Количество вершин.
    @param vertices Массив вершин.
    @param index_count Количество индексов.
    @param indices Массив индексов.
    @param out_vertex_count Указатель на память, куда сохранить количество получившихся вершин.
    @param out_vertices Указатель на новый массив вершин.
*/
void geometry_deduplicate_vertices(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices, u32* out_vertex_count, vertex_3d** out_vertices);
