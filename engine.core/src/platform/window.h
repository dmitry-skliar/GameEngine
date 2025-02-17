#pragma once

#include <defines.h>

// @brief Начальная конфигурация окна.
typedef struct window_config {
    // @brief Заголовок окна.
    char* title;
    // @brief Ширина окна в пикселях.
    i32 width;
    // @brief Высота окна в пикселях.
    i32 height;
} window_config;

// @brief Контекст окна.
typedef struct window {
  // @brief Заголовок окна.
  const char* title;
  // @brief Ширина окна в пикселях.
  i32 width;
  // @brief Высота окна в пикселях.
  i32 height;
} window;

// Указатели на функции обработчики событий окна (слушателей).
typedef void (*PFN_window_handler_close)();
typedef void (*PFN_window_handler_resize)(i32 width, i32 height);
typedef void (*PFN_window_handler_keyboard_key)(u32 keycode, bool pressed);
typedef void (*PFN_window_handler_mouse_move)(i32 x, i32 y);
typedef void (*PFN_window_handler_mouse_button)(u32 button, bool pressed);
typedef void (*PFN_window_handler_mouse_wheel)(i32 zdelta);
typedef void (*PFN_window_handler_focus)(bool focused);

/*
    @brief Создает экземпляр оконного приложения.
    @param memory_requirement Указатель на переменную для получения требований к памяти.
    @param instance Указатель на выделенную память экземпляра, для получения требований к памяти передать null.
    @param config Указатель на конфигурацию создаваемого окна, для получения требований может быть null.
    @return True операция завершена успешно, false в случае ошибок.
*/
KAPI bool platform_window_create(u64* memory_requirement, window* instance, window_config* config);

/*
    @brief Завершает работу окна.
    @param instance Указатель на выделенную память экземпляра окна.
*/
KAPI void platform_window_destroy(window* instance);

/*
    @brief Обрабатывает события окна.
    NOTE: Обязательно добавить в обработку кадра!
    @param instance Указатель на выделенную память экземпляра окна.
    @return True в случае успеха, false при возникновении ошибки.
*/
KAPI bool platform_window_dispatch(window* instance);

/*
    @brief Задает обработчик на закрытие окна.
    @param instance Указатель на выделенную память экземпляра окна.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_set_on_close_handler(window* instance, PFN_window_handler_close handler);

/*
    @brief Задает обработчик на изменение размеров окна.
    @param instance Указатель на выделенную память экземпляра окна.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_set_on_resize_handler(window* instance, PFN_window_handler_resize handler);

/*
    @brief Задает обработчик на нажатия клавиш клавиатуры.
    @param instance Указатель на выделенную память экземпляра окна.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_set_on_keyboard_key_handler(window* instance, PFN_window_handler_keyboard_key handler);

/*
    @brief Задает обработчик на перемещения курсора мышки.
    @param instance Указатель на выделенную память экземпляра окна.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_set_on_mouse_move_handler(window* instance, PFN_window_handler_mouse_move handler);

/*
    @brief Задает обработчик на нажатие клавиш мышки.
    @param instance Указатель на выделенную память экземпляра окна.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_set_on_mouse_button_handler(window* instance, PFN_window_handler_mouse_button handler);

/*
    @brief Задает обработчик на прокрутку колесика мышки.
    @param instance Указатель на выделенную память экземпляра окна.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_set_on_mouse_wheel_handler(window* instance, PFN_window_handler_mouse_wheel handler);

/*
    @brief Задает обработчик на фокусировку окна курсором мышки.
    @param instance Указатель на выделенную память экземпляра окна.
    @param handler Обработчик который будет вызван по событию, может быть null.
*/
KAPI void platform_window_set_on_focus_handler(window* instance, PFN_window_handler_focus handler);

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
