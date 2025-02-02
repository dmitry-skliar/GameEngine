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

// @brief Уровни логирования.
typedef enum log_level {
    // @brief Уровень фатальных ошибок, при котором дальнейшая работа приложения невозможна.
    LOG_LEVEL_FATAL,
    // @brief Уровнь критических ошибок, при котором можно нормально завершить приложение.
    LOG_LEVEL_ERROR,
    // @brief Уровень не критических ошибок, при котором приложение может продолжать работу.
    LOG_LEVEL_WARNG,
    // @brief Уровень информирования, не влияет на работу приложения.
    LOG_LEVEL_INFOR,
    // @brief Уровень отладки, выводит отладочную информацию.
    LOG_LEVEL_DEBUG,
    // @brief Уровень отладки с подробной информацией.
    LOG_LEVEL_TRACE,
    // @brief Количество уровней логирования!
    LOG_LEVELS_MAX
} log_level;

// @brief Указатель на функцию обработки сообщения.
typedef void (*PFN_console_write)(log_level level, const char* message);

/*
    @brief Задает указатель на пользовательскую функцию обработки сообщения. По умолчанию выводит в консоль.
    @param hook Указатель на функцию обработки сообщения. Для отбрасывания сообщений установить в 'null'.
*/
KAPI void log_output_set_custom_hook(PFN_console_write hook);

/*
    @brief Восстанавливает значение по умолчанию (вывод сообщений в консоль). 
*/
KAPI void log_output_set_default_hook();

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
#define kfatal(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__)

#ifndef kerror
    /*
        @brief Отправляет сообщение с критическим уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define kerror(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#endif

#if LOG_WARNG_ENABLED == 1
    /*
        @brief Отправляет сообщение с не критическим уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define kwarng(message, ...) log_output(LOG_LEVEL_WARNG, message, ##__VA_ARGS__)
#else
    /*
        @brief Отправляет сообщение с не критическим уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define kwarng(message, ...)
#endif

#if LOG_INFOR_ENABLED == 1
    /*
        @brief Отправляет сообщение с информационным уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define kinfor(message, ...) log_output(LOG_LEVEL_INFOR, message, ##__VA_ARGS__)
#else
    /*
        @brief Отправляет сообщение с информационным уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define kinfor(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
    /*
        @brief Отправляет сообщение с отладочным уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define kdebug(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#else
    /*
        @brief Отправляет сообщение с отладочным уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define kdebug(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
    /*
        @brief Отправляет сообщение с пошаговым уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define ktrace(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__)
#else
    /*
        @brief Отправляет сообщение с пошаговым уровнем в логи.
        @param message Сообщение или строка форматирования.
        @param ... Аргументы строки форматирования.
    */
    #define ktrace(message, ...)
#endif
