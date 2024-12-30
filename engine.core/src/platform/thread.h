#pragma once

#include <defines.h>

/*
    @brief Останавливает/блокирует работку главного потока приложения на заданное время.
    INFO: Возвращает управление операционной системе.
    @param time Время в миллисекундах.
*/
KAPI void platform_thread_sleep(u64 time_ms);
