#pragma once

#include <defines.h>

// @brief Конфигурация приложения.
typedef struct application {

    // @brief Заголовок окна.
    char* window_title;

    // @brief Щирина окна.
    i32   window_width;

    // @brief Высота окна.
    i32   window_height;

    // @brief Указатель на функцию инициализации игры.
    bool (*initialize)(struct application* inst);

    // @brief Указатель на функцию обновления состояния игры.
    bool (*update)(struct application* inst, f32 delta_time);

    // @brief Указатель на функцию отрисовки игры.
    bool (*render)(struct application* inst, f32 delta_time);

    // @brief Указатель на функцию изменения размера окна игры.
    void (*on_resize)(struct application* inst, i32 width, i32 height);

    // @brief Указатель на данные игры.
    void* state;

} application;

/*
    @brief Инициализирует и создает приложение.
    @param inst Конфигурация приложения.
    @return В случае успеха - true, в случае ошибок или повторного вызова - false.
*/
KAPI bool application_create(application* application);

/*
    @brief Рабочий цикл приложения.
    @return В случае успеха - true, в случае ошибок - false.
*/
KAPI bool application_run();
