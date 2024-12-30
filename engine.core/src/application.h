#pragma once

#include <defines.h>

// @brief Конфигурация приложения.
typedef struct game {

    // Заголовок окна.
    char* window_title;

    // Щирина окна.
    i32   window_width;

    // Высота окна.
    i32   window_height;

    // Указатель на функцию инициализации игры.
    bool (*initialize)(struct game* inst);

    // Указатель на функцию обновления состояния игры.
    bool (*update)(struct game* inst, f32 delta_time);

    // Указатель на функцию отрисовки игры.
    bool (*render)(struct game* inst, f32 delta_time);

    // Указатель на функцию изменения размера окна игры.
    void (*on_resize)(struct game* inst, i32 width, i32 height);

    // Указатель данные игры.
    void* state;

} game;

/*
    @brief Инициализирует и создает приложение.
    @return В случае успеха - true, в случае ошибок повторного вызова - false.
*/
KAPI bool application_create(game* inst);

/*
    @brief Рабочий цикл приложения.
    @return В случае успеха - true, в случае ошибок - false.
*/
KAPI bool application_run();
