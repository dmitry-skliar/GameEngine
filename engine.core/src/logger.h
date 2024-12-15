#pragma once

#include <defines.h>

// Включено по умолчанию.
#define LOG_WARNG_ENABLED 1
#define LOG_INFOR_ENABLED 1

// Включено при отладке.
#if KDEBUG_FLAG
    #define LOG_DEBUG_ENABLED 1
    #define LOG_TRACE_ENABLED 1
#else
    #define LOG_DEBUG_ENABLED 0
    #define LOG_TRACE_ENABLED 0
#endif

// Уровни логирования.
typedef enum log_level {
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNG,
    LOG_LEVEL_INFOR,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,
    LOG_LEVELS_MAX
} log_level;

// Указатель на функцию обработки сообщения.
typedef void (*PFN_console_write)(log_level level, const char* message);

/*
    @brief Задает указатель на функцию обработки сообщения. По умолчанию выводит в консоль.
    @param hook Указатель на функцию обработки сообщения. Для отбрасывания сообщений установить в 'null'.
*/
KAPI void log_output_hook_set(PFN_console_write hook);

/*
    @brief Восстанавливает значение по умолчанию (вывод сообщений в консоль). 
*/
KAPI void log_output_hook_set_default();

/*
    @brief Функция отправки сообщения с заданным уровнем для дальнейшего логирования в системе.
    @param level Уровень логирования.
    @param message Сообщение или строка форматирования.
    @param ... Аргументы строки форматирования.
*/
KAPI void log_output(log_level level, const char* message, ...);

/*
    @brief Отправляет сообщение с фатальным уровнем в логи и останавливает нормальную работу приложения.
    @param message Сообщение или строка форматирования.
    @param ... Аргументы строки форматирования.
*/
#define KFATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__)

#ifndef KERROR
    /*
        @brief Отправляет сообщение с критическим уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define KERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#endif

#if LOG_WARNG_ENABLED == 1
    /*
        @brief Отправляет сообщение с не критическим уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define KWARNG(message, ...) log_output(LOG_LEVEL_WARNG, message, ##__VA_ARGS__)
#else
    /*
        @brief Отправляет сообщение с не критическим уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define KWARNG(message, ...)
#endif

#if LOG_INFOR_ENABLED == 1
    /*
        @brief Отправляет сообщение с информационным уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define KINFOR(message, ...) log_output(LOG_LEVEL_INFOR, message, ##__VA_ARGS__)
#else
    /*
        @brief Отправляет сообщение с информационным уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define KINFOR(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
    /*
        @brief Отправляет сообщение с отладочным уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define KDEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#else
    /*
        @brief Отправляет сообщение с отладочным уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define KDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
    /*
        @brief Отправляет сообщение с пошаговым уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define KTRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__)
#else
    /*
        @brief Отправляет сообщение с пошаговым уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define KTRACE(message, ...)
#endif
