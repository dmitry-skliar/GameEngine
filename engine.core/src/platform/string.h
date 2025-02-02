#pragma once

#include <defines.h>

/*
    @brief Получает количество символов предоставленной строки.
    @param str Указатель на строку.
    @return Количество символов строки.
*/
KAPI u64 platform_string_length(const char* str);

/*
    @brief Посимвольно сравнивает две строки с учетом регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @return True - если строки одинаковые, false - разные.
*/
KAPI bool platform_string_equal(const char* lstr, const char* rstr);

/*
    TODO: Реализовать!
    @brief Посимвольно сравнивает две строки без учетом регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @return True - если строки одинаковые, false - разные.
*/
KAPI bool platform_string_equali(const char* lstr, const char* rstr);

/*
    @brief Выполняет форматирование строки в соответствии с заданным форматом
           строки и переменными.
    @param dest Указатель на память куда записать отформатированную строку.
    @param format Формат строки, который указывает как должна выглядеть строка.
    @param ... Параметры строки формата.
    @return Количество записаных символов отформатированной строки.
*/
KAPI i32 platform_string_format(char* dest, const char* format, ...);

/*
    @brief Выполняет форматирование строки в соответствии с заданным форматом
           строки и указателем на переменные.
    @param dest Указатель на память куда записать отформатированную строку.
    @param format Формат строки, который указывает как должна выглядеть строка.
    @param va_list Указатель на параметры строки формата.
    @return Количество записаных символов отформатированной строки.
*/
KAPI i32 platform_string_formatv(char* dest, const char* format, void* va_list);
