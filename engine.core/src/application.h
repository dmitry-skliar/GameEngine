#pragma once

#include <defines.h>

// @brief Конфигурация игры.
typedef struct game {
    // @brief Заголовок окна.
    char* window_title;
    // @brief Щирина окна.
    i32   window_width;
    // @brief Высота окна.
    i32   window_height;
    // @brief Указатель на функцию инициализации игры.
    bool (*initialize)(struct game* inst);
    // @brief Указатель на функцию обновления состояния игры.
    bool (*update)(struct game* inst, f32 delta_time);
    // @brief Указатель на функцию отрисовки игры.
    bool (*render)(struct game* inst, f32 delta_time);
    // @brief Указатель на функцию изменения размера окна игры.
    void (*on_resize)(struct game* inst, i32 width, i32 height);
    // @brief Требования к памяти в байтах (sizeof(game_state)).
    u64 state_memory_requirement;
    // @brief Указатель на состояние приложения.
    void* state;
    // @brief Указатель на контекст приложения.
    void* application_state;
} game;

/*
    @brief Инициализирует и создает приложение.
    @param inst Конфигурация приложения.
    @return В случае успеха - true, в случае ошибок или повторного вызова - false.
*/
KAPI bool application_create(game* game_inst);

/*
    @brief Рабочий цикл приложения.
    @return В случае успеха - true, в случае ошибок - false.
*/
KAPI bool application_run();
