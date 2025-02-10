// Cобственные подключения.
#include "platform/string.h"
#include "platform/memory.h"

#if KPLATFORM_LINUX_FLAG

    // Внешние подключения.
    #include <stdio.h>
    #include <stdarg.h>
    #include <string.h>
    #include <ctype.h>

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
        va_list args;
        va_start(args, format);
        // TODO: Заменить без использования стека!
        char buffer[32000];
        i32 written = vsnprintf(buffer, 32000, format, args);
        buffer[written] = 0;
        platform_memory_copy(dest, buffer, written + 1);
        va_end(args);
        return written;
    }

    char* platform_string_copy(char* dest, const char* src)
    {
        return strcpy(dest, src);
    }

    char* platform_string_ncopy(char* dest, const char* src, u64 length)
    {
        return strncpy(dest, src, length);
    }

    bool platform_string_isspace(char c)
    {
        return isspace((unsigned char)c);
    }

    i32 platform_string_sscanf(const char* str, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        i32 result = vsscanf(str, format, args);
        va_end(args);
        return result;
    }

#endif
