// Cобственные подключения.
#include "clock.h"

// Внутренние подключения.
#include "platform/time.h"

// TODO: Сделать защиту от переполнения таймера при переходе в ноль, если такое возможно!

void clock_start(clock* clock)
{
    clock->start_time = platform_absolute_time_get();
    clock->elapsed = 0;
}

void clock_stop(clock* clock)
{
    clock->start_time = 0;
}

void clock_update(clock* clock)
{
    if(clock->start_time)
    {
        clock->elapsed = platform_absolute_time_get() - clock->start_time;
    }
}
