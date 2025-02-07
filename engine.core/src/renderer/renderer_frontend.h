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

void renderer_create_texture(
    const char* name, bool auto_release, i32 width, i32 height, i32 channel_count, const u8* pixels,
    bool has_transparency, texture* out_texture
);

void renderer_destroy_texture(texture* texture);
