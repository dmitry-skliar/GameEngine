#pragma once

#include <renderer/renderer_types.h>
#include <platform/window.h>

/*
    @brief Запускает систему визуализации.
    @param memory_requirement Указатель на переменную для получения требований к памяти.
    @param memory Указатель на выделенную память, для получения требований к памяти передать null.
    @param window_state Указатель на выделенную память экземпляра оконной системы.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_system_initialize(u64* memory_requirement, void* memory, window* window_state);

/*
    @brief Завершает работу системы визуализации.
*/
void renderer_system_shutdown();

void renderer_on_resize(i32 width, i32 height);

bool renderer_draw_frame(render_packet* packet);

// HACK: Временно!
KAPI void renderer_set_view(mat4 view);

void renderer_create_texture(texture* texture, const void* pixels);

void renderer_destroy_texture(texture* texture);

bool renderer_create_material(material* material);

void renderer_destroy_material(material* material);

bool renderer_create_geometry(
    geometry* geometry, u32 vertex_count, const vertex_3d* vertices, u32 index_count, const u32* indices
);

void renderer_destroy_geometry(geometry* geometry);
