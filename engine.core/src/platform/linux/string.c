// Cобственные подключения.
#include "platform/string.h"
#include "platform/memory.h"

#if KPLATFORM_LINUX_FLAG

    // Внешние подключения.
    #include <stdio.h>
    #include <stdarg.h>
    #include <string.h>

    u64 platform_string_length(const char *str)
    {
        return strlen(str);    
    }

    bool platform_string_equal(const char* lstr, const char* rstr)
    {
        if(lstr == rstr) return true;
        if(!lstr || !rstr) return false;
        return strcmp(lstr, rstr) == 0;
    }

    bool platform_string_equali(const char* lstr, const char* rstr)
    {
        if(lstr == rstr) return true;
        if(!lstr || !rstr) return false;
        return strcasecmp(lstr, rstr) == 0;
    }

    i32 platform_string_format(char* dest, const char* format, ...)
    {
        if(dest)
        {
            __builtin_va_list args;
            va_start(args, format);
            i32 written = platform_string_formatv(dest, format, args);
            va_end(args);
            return written;
        }
        return -1;
        
    }

    i32 platform_string_formatv(char* dest, const char* format, void* va_list)
    {
        if(dest)
        {
            // TODO: Заменить позже!
            char buffer[32000];
            i32 written = vsnprintf(buffer, 32000, format, va_list);
            buffer[written] = 0;
            platform_memory_copy(dest, buffer, written + 1);

            return written;
        }
        return -1;
    }

#endif
