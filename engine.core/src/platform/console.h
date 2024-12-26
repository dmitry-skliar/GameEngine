#pragma once

#include <defines.h>

// Цвета символов, фона.
typedef enum console_color {
    CONSOLE_COLOR_BG_RED,
    CONSOLE_COLOR_FG_RED,
    CONSOLE_COLOR_FG_YELLOW,
    CONSOLE_COLOR_FG_GREEN,
    CONSOLE_COLOR_FG_BLUE,
    CONSOLE_COLOR_FG_WHITE,
    CONSOLE_COLORS_MAX
} console_color;

/*
    @brief Печатает сообщение в стандартный поток вывода.
    @param level Уровень сообщения.
    @param message Сообщение.
*/
KAPI void platform_console_write(console_color color, const char* message);

/*
    @brief Печатает сообщение в стандартый поток вывода ошибок.
    @param level Уровень сообщения.
    @param message Сообщение.
*/
KAPI void platform_console_write_error(console_color color, const char* message);
