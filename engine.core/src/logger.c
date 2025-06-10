// Cобственные подключения.
#include "logger.h"

// Внутренние подключения.
#include "platform/console.h"
#include "platform/string.h"
#include "debug/assert.h"

// Внешние подключения.
#include <stdarg.h>

// Функиця обработки сообщения по умолчанию, message игнорируется, но функция использует buffer напрямую.
void log_output_default_hook(log_level level, const char* message);

// Размер буфера данных в байтах.
#define LOG_BUFFER_SIZE   0xffff

// Отступ в буфере данных в байтах.
#define LOG_BUFFER_OFFSET 8

/*
    Буфер сообщений.
    TODO: Заменить на многопоточный вариант.
*/
static char buffer[LOG_BUFFER_SIZE];

// Указатель на функцию в которую будет передаваться сообщение.
static PFN_console_write log_output_hook = log_output_default_hook;

void log_output(log_level level, const char* message, ...)
{
    // Проверяет наличие указателя на функцию вывода.
    if(log_output_hook)
    {
        // Получение указателя на аргументы строки формата.
        __builtin_va_list args;
        va_start(args, message);

        // Запись отформатированной строки в буфер.
        i32 length = platform_string_format_va(&buffer[LOG_BUFFER_OFFSET], LOG_BUFFER_SIZE - LOG_BUFFER_OFFSET, message, args);

        // Завершение работы с аргументами.
        va_end(args);

        // Запись строки \n\0, но пишется как "\n" в которой символ \0 вставляется компилятором автоматически.
        KCOPY2BYTES(&buffer[LOG_BUFFER_OFFSET + length], "\n");

        // Отправка вывода буфера в заданную функцию.
        log_output_hook(level, &buffer[LOG_BUFFER_OFFSET]);

        // Очистка буфера.
        buffer[0] = '\0';
    }

    // Вызывает остановку программы, в случае фатальной ошибки. Используется для отладки программы.
    if(level == LOG_LEVEL_FATAL)
    {
        kdebug_break();
    }
}

void log_output_default_hook(log_level level, const char* message)
{
    // Текстовые метки сообщений в соответствии с уровнем.
    const char* levels[LOG_LEVELS_MAX] = {
        "[FATAL] ", "[ERROR] ", "[WARNG] ", "[INFOR] ", "[DEBUG] ", "[TRACE] "
    };

    // Цвета сообщений в соответствии с уровнем.
    const console_color colors[LOG_LEVELS_MAX] = {
        CONSOLE_COLOR_BG_RED, CONSOLE_COLOR_FG_RED, CONSOLE_COLOR_FG_YELLOW,
        CONSOLE_COLOR_FG_GREEN, CONSOLE_COLOR_FG_BLUE, CONSOLE_COLOR_FG_WHITE
    };

    // Копирует метку в зарезервированное место, для текстовых меток.
    KCOPY8BYTES(buffer, levels[level]);

    if(level == LOG_LEVEL_ERROR || level == LOG_LEVEL_FATAL)
    {
        platform_console_write_error(colors[level], buffer);
    }
    else
    {
        platform_console_write(colors[level], buffer);
    }
}

void log_output_set_custom_hook(PFN_console_write hook)
{
    log_output_hook = hook;
}

void log_output_set_default_hook()
{
    log_output_hook = log_output_default_hook;
}

// NOTE: Используется в debug/assert.h
void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line)
{
    log_output(LOG_LEVEL_FATAL, "Assertion failure: %s, message: '%s' in %s:%d", expression, message, file, line);
}
