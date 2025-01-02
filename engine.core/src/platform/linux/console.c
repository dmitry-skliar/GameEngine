// Cобственные подключения.
#include "platform/console.h"

#if KPLATFORM_LINUX_FLAG

    // Внутренние подключения.

    // Внешние подключения.
    #include <stdio.h>

    static const char* colors[CONSOLE_COLORS_MAX] = {"1;95", "1;31", "1;33", "1;32", "1;34", "0;39"};

    void platform_console_write(console_color color, const char* message)
    {
        fprintf(stdout ,"\033[%sm%s\033[0m", colors[color], message);
    }

    void platform_console_write_error(console_color color, const char* message)
    {
        fprintf(stderr ,"\033[%sm%s\033[0m", colors[color], message);
    }

#endif
