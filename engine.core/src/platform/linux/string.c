// Cобственные подключения.
#include "platform/string.h"

#if KPLATFORM_LINUX_FLAG

    // Внутренние подключения.
    // ...

    // Внешние подключения.
    #include <string.h>

    u64 platform_string_get_length(const char *str)
    {
        return strlen(str);    
    }

    bool platform_string_is_equal(const char* lstr, const char* rstr)
    {
        return strcmp(lstr, rstr) == 0;
    }

#endif
