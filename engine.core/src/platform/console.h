#pragma once

#include <defines.h>
#include <logger.h>

/*
    @brief Печатает сообщение в консоль.
    @param level Уровень сообщения.
    @param message Сообщение.
*/
KAPI void platform_console_write(log_level level, const char* message);
