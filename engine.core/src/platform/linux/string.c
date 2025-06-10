// Cобственные подключения.
#include "platform/string.h"

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

    bool platform_string_nequal(const char* lstr, const char* rstr, u64 length)
    {
        if(lstr == rstr) return true;
        if(!lstr || !rstr) return false;
        return strncmp(lstr, rstr, length) == 0;
    }

    bool platform_string_nequali(const char* lstr, const char* rstr, u64 length)
    {
        if(lstr == rstr) return true;
        if(!lstr || !rstr) return false;
        return strncasecmp(lstr, rstr, length) == 0;
    }

    i32 platform_string_format(char* dest, u16 length, const char* format, ...)
    {
        // Получение указателя на аргументы строки формата.
        // NOTE: Параметры используются только для одного вызова vsnprintf, для повторного использования
        //       необходимо вызвать va_end, после чего повторить va_start и тогда можно вызвать еще раз.
        va_list args;
        va_start(args, format);

        // Форматирование строки с использованием предоставленных максимальной длинны и аргументов.
        i32 written = vsnprintf(dest, length, format, args);

        // Завершение работы с аргументами.
        va_end(args);
        return written;
    }

    i32 platform_string_format_va(char* dest, u64 length, const char* format, void* va_list)
    {
        return vsnprintf(dest, length, format, va_list);
    }

    i32 platform_string_format_unsafe(char* dest, const char* format, ...)
    {
        // Получение указателя на аргументы строки формата.
        // NOTE: Параметры используются только для одного вызова vsprintf, для повторного использования
        //       необходимо вызвать va_end, после чего повторить va_start и тогда можно вызвать еще раз.
        va_list args;
        va_start(args, format);

        // Форматирование строки с использованием предоставленных аргументов.
        i32 written = vsprintf(dest, format, args);

        // Завершение работы с аргументами.
        va_end(args);
        return written;
    }

    i32 platform_string_format_va_unsafe(char* dest, const char* format, void* va_list)
    {
        return vsprintf(dest, format, va_list);
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
        // Получение указателя на аргументы строки формата.
        // NOTE: Параметры используются только для одного вызова vsscanf, для повторного использования
        //       необходимо вызвать va_end, после чего повторить va_start и тогда можно вызвать еще раз.
        va_list args;
        va_start(args, format);

        // Форматирование строки с использованием предоставленных аргументов.
        i32 result = vsscanf(str, format, args);

        // Завершение работы с аргументами.
        va_end(args);
        return result;
    }

#endif
