#pragma once

#include <defines.h>
#include <platform/string.h>

/*
    @brief Получает количество символов предоставленной строки.
    @param str Указатель на строку.
    @return Количество символов строки.
*/
#define string_length(str) platform_string_length(str)

/*
    @brief Посимвольно сравнивает две строки с учетом регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @return True если строки одинаковые, false разные.
*/
#define string_equal(lstr, rstr) platform_string_equal(lstr, rstr)

/*
    @brief Посимвольно сравнивает две строки без учетом регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @return True - если строки одинаковые, false - разные.
*/
#define string_equali(lstr, rstr) platform_string_equali(lstr, rstr)

/*
    @brief Выполняет форматирование строки в соответствии с заданным форматом
           строки и переменными.
    @param dest Указатель на память куда записать отформатированную строку.
    @param format Формат строки, который указывает как должна выглядеть строка.
    @param ... Параметры строки формата.
    @return Количество записаных символов отформатированной строки.
*/
#define string_format(dest, format, ...) platform_string_format(dest, format, ##__VA_ARGS__)

/*
    @brief Выполняет форматирование строки в соответствии с заданным форматом
           строки и указателем на переменные.
    @param dest Указатель на память куда записать отформатированную строку.
    @param format Формат строки, который указывает как должна выглядеть строка.
    @param va_list Указатель на параметры строки формата.
    @return Количество записаных символов отформатированной строки.
*/
#define string_formatv(dest, format, va_list) platform_string_formatv(dest, format, va_list)

/*
    @brief Создает дубликат строки.
    NOTE: После использования удалить с помощью функуии 'string_free'.
    @param str Указатель на строку.
    @return Указатель на дубликат строки.
*/
KAPI char* string_duplicate(const char* str);

/*
    @brief Удаляет указанную строку.
    NOTE: Удалить можно только строку созданную данной библиотекой.
    @param str Указатель на строку.
*/
KAPI void string_free(const char* str);
