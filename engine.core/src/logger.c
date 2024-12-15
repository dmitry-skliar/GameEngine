#include "logger.h"
#include "debug/assert.h"
#include "platform/console.h"

#include <stdarg.h>
// TODO: Убрать после замены printf, vsnprintf.
#include <stdio.h>

// Функиця обработки сообщения по умолчанию.
void log_output_hook_default(log_level level, const char* message);

// Размер буфера данных.
#define LOG_BUFFER_SIZE   0xffff

// Отступ в буфере данных (байт).
#define LOG_BUFFER_OFFSET 8

/*
    Буфер сообщений.
    INFO: Только для однопоточной работы.
    TODO: Заменить на многопоточный вариант.
*/
static char buffer[LOG_BUFFER_SIZE];

// Указатель на функцию в которую будет передаваться сообщение. 
static PFN_console_write log_output_hook = log_output_hook_default;

void log_output(log_level level, const char* message, ...)
{
    __builtin_va_list args;
    va_start(args, message);
    // TODO: создать обертку и вынести в оделный заголовочный файл.
    i32 length = vsnprintf(&buffer[LOG_BUFFER_OFFSET], LOG_BUFFER_SIZE - LOG_BUFFER_OFFSET, message, args);
    va_end(args);

    KCOPY2BYTES(&buffer[LOG_BUFFER_OFFSET + length], "\n");

    if(log_output_hook)
    {
        log_output_hook(level, &buffer[LOG_BUFFER_OFFSET]);
        buffer[0] = '\0';
    }

    if(level == LOG_LEVEL_FATAL)
    {
        KDEBUG_BREAK();
    }
}

void log_output_hook_default(log_level level, const char* message)
{
    const char* levels[LOG_LEVELS_MAX] = {"[FATAL] ", "[ERROR] ", "[WARNG] ", "[INFOR] ", "[DEBUG] ", "[TRACE] "};
    KCOPY8BYTES(buffer, levels[level]);
    platform_console_write(level, &buffer[0]);
}

void log_output_hook_set(PFN_console_write hook)
{
    log_output_hook = hook;
}

void log_output_hook_set_default()
{
    log_output_hook = log_output_hook_default;
}

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line)
{
    log_output(LOG_LEVEL_FATAL, "Assertion failure: %s, message: '%s' in %s:%d", expression, message, file, line);
}
