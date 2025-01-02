#pragma once

#include <defines.h>

// Включено по умолчанию.
#define KASSERTION_ENABLED 1

// Макрос точки останова используется для отладки приложения.
#if defined(__has_builtin) && !defined(__ibmxl__)
    #if __has_builtin(__builtin_debugtrap)
        #define kdebug_break() __builtin_debugtrap()
    #elif __has_builtin(__debugbreak)
        #define kdebug_break() __debugbreak()
    #endif
#endif

// Старый способ определения макроса точки останова для отладки.
#if !defined(kdebug_break)
    #if KCOMPILER_CLANG_FLAG
        #define kdebug_break() __builtin_trap()
    #elif KCOMPILER_MICROSOFT_FLAG
        #include <intrin.h>
        #define kdebug_break() __debugbreak()
    #else
        #define kdebug_break() asm { int 3 }
    #endif
#endif

#if KASSERTION_ENABLED == 1
    /*
        @brief Проверка утверждения и отображение сообщения с остановкой программы при несоответствии,
        а также выводит местоположение: имя файла и строка, где это произошло.
        @param expression Утверждение которое нужно преверить.
        @param message Сообщение которое нужно вывести.
        @param file Имя файла, где найдено несоответствие.
        @param line Строка в которой найдено несоответствие.
    */
    KAPI void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line);

    /*
        @brief Проверка утверждения и отображение сообщения с остановкой программы при несоответствии, 
        а также выводит местоположение: имя файла и строка, где это произошло.
        @param expr Утверждение которое нужно преверить.
        @param message Сообщение которое нужно вывести.
    */
    #define kassert(expr, message)                                        \
    {                                                                     \
        if(expr) {}                                                       \
        else                                                              \
        {                                                                 \
            report_assertion_failure(#expr, message, __FILE__, __LINE__); \
            kdebug_break();                                               \
        }                                                                 \
    }

    #if KDEBUG_FLAG
        /*
            @brief Проверка утверждения и отображение сообщения с остановкой программы при несоответствии,
            а также выводит местоположение: имя файла и строка, где это произошло.
            INFO: Используется только при отладке приложения.
            @param expr Утверждение которое нужно преверить.
        */
        #define kassert_debug(expr, message)                                  \
        {                                                                     \
            if(expr) {}                                                       \
            else                                                              \
            {                                                                 \
                report_assertion_failure(#expr, message, __FILE__, __LINE__); \
                kdebug_break();                                               \
            }                                                                 \
        }

    #else
        #define kassert_debug(expr)
    #endif
#else
    #define kassert(expr, message)
    #define kassert_debug(expr, message)
#endif
