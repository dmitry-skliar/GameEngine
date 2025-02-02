// Cобственные подключения.
#include "platform/thread.h"

#if KPLATFORM_LINUX_FLAG

    // Внешние подключения.
    #include <time.h>

    void platform_thread_sleep(u64 time_ms)
    {
        struct timespec ts;
        ts.tv_sec  = time_ms / 1000;
        ts.tv_nsec = (time_ms % 1000) * 1000000;
        nanosleep(&ts, null);
    }

#endif
