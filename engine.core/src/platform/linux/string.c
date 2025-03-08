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

    // TODO: При успешном завершении работы эти функции возвращают количество напечатанных
    // символов (не включая завершающий `\0', использующийся для обозначения конца строки данных).
    // Функции snprintf и vsnprintf передают не больше байтов, чем указано в переменной size
    // (включая завершающий '\0'). Если вывод был урезан из-за этого ограничения, то возвращаемое
    // значение является числом символов (кроме заверщающего '\0'), переданных в конечной строке,
    // если для этого имелось достаточно места. Таким образом, возвращаемое значение size или более
    // обозначает, что вывод был обрезан. Если случилась ошибка, то возвратится отрицательное значение.
    //                                                                             Источник: opennet.ru 
    i32 platform_string_formatv(char* dest, u64 length, const char* format, void* va_list)
    {
        if(!dest || !length || !format || !va_list)
        {
            return -1;
        }

        return vsnprintf(dest, length, format, va_list);
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
