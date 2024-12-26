#pragma once

#include <defines.h>

typedef struct window_config {
    char* title;
    i32 x;
    i32 y;
    i32 width;
    i32 height;
} window_config;

// Указатели на функции обработчики событий окна (слушателей).
typedef void (*PFN_window_handler_close)();
typedef void (*PFN_window_handler_resize)(i32 width, i32 height);
typedef void (*PFN_window_handler_keyboard_key)(u32 keycode, bool pressed);
typedef void (*PFN_window_handler_mouse_move)(i32 x, i32 y);
typedef void (*PFN_window_handler_mouse_button)(u32 button, bool pressed);
typedef void (*PFN_window_handler_mouse_wheel)(i32 zdelta);
typedef void (*PFN_window_handler_focus)(bool focused);

/*
    @brief Создает окно приложения.
    INFO: Если окно было успешно создано, то при повторном вызове возвращает false.
    @param config Конфигурация создаваемого окна.
    @return В случае успеха - true, в случае ошибок или повторного вызова - false.
*/
KAPI bool platform_window_create(window_config config);

/*
    @brief Завершает работу окна.
*/
KAPI void platform_window_destroy();

/*
    @brief Обрабатывает события окна.
    INFO: Обязательно добавить в обработку кадра!
    @return В случае успеха - true, в случае ошибки или завершения работы окна - false.
*/
KAPI bool platform_window_dispatch();

/*
    @brief Задает обработчик на закрытие окна.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_handler_close_set(PFN_window_handler_close handler);

/*
    @brief Задает обработчик на изменение размеров окна.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_handler_resize_set(PFN_window_handler_resize handler);

/*
    @brief Задает обработчик на нажатия клавиш клавиатуры.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_handler_keyboard_key_set(PFN_window_handler_keyboard_key handler);

/*
    @brief Задает обработчик на перемещения курсора мышки.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_handler_mouse_move_set(PFN_window_handler_mouse_move handler);

/*
    @brief Задает обработчик на нажатие клавиш мышки.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_handler_mouse_button_set(PFN_window_handler_mouse_button handler);

/*
    @brief Задает обработчик на прокрутку колесика мышки.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_handler_mouse_wheel_set(PFN_window_handler_mouse_wheel handler);

/*
    @brief Задает обработчик на фокусировку окна курсором мышки.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_handler_focus_set(PFN_window_handler_focus handler);

// TODO: Реализовать следующее.
// KAPI void platform_window_title_set(const char* title);
// KAPI char* platform_window_title_get();

// KAPI void platform_window_mode_fullscreen();
// KAPI bool platform_window_is_mode_fullscreen();

// KAPI void platform_window_mode_window();
// KAPI bool platform_window_is_mode_window();

// KAPI void platform_window_show();
// KAPI void platform_window_hide();

// KAPI void platform_window_mouse_cursor_hide();
// KAPI void platform_window_mouse_cursor_show();
// KAPI void platform_window_is_mouse_cursor_hide();
// KAPI void platform_window_is_mouse_cursor_show();

// KAPI void platform_window_mouse_move_lock_set();
// KAPI void platform_window_mouse_move_unlock_set();
// KAPI bool platform_window_is_mouse_move_lock();
// KAPI bool platform_window_is_mouse_move_unlock();
