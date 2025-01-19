#pragma once

#include <defines.h>

// @brief Данные таймера.
typedef struct clock {
    f64 start_time;
    f64 elapsed;
} clock;

/*
    @brief Инициализирует и запускает таймер.
    NOTE: Обнуляет 'elapsed'.
    @param ts Структура таймера.
*/
KAPI void clock_start(clock* clock);

/*
    @brief Обнуляет и останавливает таймер.
    NOTE: Не обнуляет 'elapsed'.
    @param ts Структура таймера.
*/
KAPI void clock_stop(clock* clock);

/*
    @brief Обнавляет данные таймера. Обновлять перед получением значения elapsed!
    NOTE: Не работает, если таймер не был запущен функцией 'clock_start'.
    @param ts Структура таймера.
*/
KAPI void clock_update(clock* clock);
