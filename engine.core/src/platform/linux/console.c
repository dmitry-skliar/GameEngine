#include "platform/console.h"

#if KPLATFORM_LINUX_FLAG

    #include <stdio.h>

    void platform_console_write(log_level level, const char* message)
    {
        const char* colors[LOG_LEVELS_MAX] = {"1;41", "1;31", "1;33", "1;32", "1;34", "0;39"};

        bool is_error = (level == LOG_LEVEL_ERROR || level == LOG_LEVEL_FATAL);
        FILE* stream = is_error ? stderr : stdout;

        fprintf(stream ,"\033[%sm%s\033[0m", colors[level], message);
    }
    
#endif
