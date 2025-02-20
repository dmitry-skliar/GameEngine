// Cобственные подключения.
#include "logger.h"

// Внутренние подключения.
#include "platform/console.h"
#include "kstring.h"
#include "debug/assert.h"

// Внешние подключения.
#include <stdarg.h>

// Функиця обработки сообщения по умолчанию.
void log_output_default_hook(log_level level, const char* message);

// Размер буфера данных.
#define LOG_BUFFER_SIZE   0xffff

// Отступ в буфере данных (в байтах).
#define LOG_BUFFER_OFFSET 8

/*
    Буфер сообщений.
    NOTE: Только для однопоточной работы.
    TODO: Заменить на многопоточный вариант.
*/
static char buffer[LOG_BUFFER_SIZE];

// Указатель на функцию в которую будет передаваться сообщение.
static PFN_console_write log_output_hook = log_output_default_hook;

void log_output(log_level level, const char* message, ...)
{
    if(log_output_hook)
    {
        __builtin_va_list args;
        va_start(args, message);
        i32 length = string_formatv(&buffer[LOG_BUFFER_OFFSET], LOG_BUFFER_SIZE - LOG_BUFFER_OFFSET, message, args);
        va_end(args);

        KCOPY2BYTES(&buffer[LOG_BUFFER_OFFSET + length], "\n");

        log_output_hook(level, &buffer[LOG_BUFFER_OFFSET]);

        buffer[0] = '\0';
    }

    if(level == LOG_LEVEL_FATAL)
    {
        kdebug_break();
    }
}

void log_output_default_hook(log_level level, const char* message)
{
    const char* levels[LOG_LEVELS_MAX] = {
        "[FATAL] ", "[ERROR] ", "[WARNG] ", "[INFOR] ", "[DEBUG] ", "[TRACE] "
    };

    const console_color colors[LOG_LEVELS_MAX] = {
        CONSOLE_COLOR_BG_RED, CONSOLE_COLOR_FG_RED, CONSOLE_COLOR_FG_YELLOW,
        CONSOLE_COLOR_FG_GREEN, CONSOLE_COLOR_FG_BLUE, CONSOLE_COLOR_FG_WHITE
    };

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

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line)
{
    log_output(LOG_LEVEL_FATAL, "Assertion failure: %s, message: '%s' in %s:%d", expression, message, file, line);
}
