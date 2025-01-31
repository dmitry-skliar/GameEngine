// Cобственные подключения.
#include "platform/time.h"

#if KPLATFORM_LINUX_FLAG

    // Внутренние подключения.
    // ...

    // Внешние подключения.
    #include <time.h>

    f64 platform_time_get_absolute()
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec + now.tv_nsec * 0.000000001;
    }

#endif
