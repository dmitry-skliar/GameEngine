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
    @return True если строки одинаковые, false разные.
*/
KAPI bool platform_string_equal(const char* lstr, const char* rstr);

/*
    @brief Посимвольно сравнивает две строки без учетом регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @return True если строки одинаковые, false разные.
*/
KAPI bool platform_string_equali(const char* lstr, const char* rstr);

/*
    @brief Посимвольно сравнивает заданое количество символов двух строк с учетом регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @param length Количество символов которое нужно сравнить.
    @return True если строки совпадают, false не совпадают.
*/
KAPI bool platform_string_nequal(const char* lstr, const char* rstr, u64 length);

/*
    @brief Посимвольно сравнивает заданое количество символов двух строки без учета регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @param length Количество символов которое нужно сравнить.
    @return True если строки совпадают, false не совпадают.
*/
KAPI bool platform_string_nequali(const char* lstr, const char* rstr, u64 length);

/*
    @brief Выполняет форматирование строки в соответствии с заданным форматом
           строки и переменными.
    @param dest Указатель на память куда записать отформатированную строку.
    @param format Формат строки, который указывает как должна выглядеть строка.
    @param ... Параметры в соответствии со строкой формата.
    @return Количество записаных символов отформатированной строки.
*/
KAPI i32 platform_string_format(char* dest, const char* format, ...);

/*
    @brief Выполняет форматирование строки в соответствии с заданным форматом
           строки и указателем на переменные.
    @param dest Указатель на память куда записать отформатированную строку.
    @param length Количество символов которое будет записано, включая завершающий (\0).
    @param format Формат строки, который указывает как должна выглядеть строка.
    @param va_list Указатель на параметры строки формата.
    @return Количество записаных символов отформатированной строки.
*/
KAPI i32 platform_string_formatv(char* dest, u64 length, const char* format, void* va_list);

/*
    @brief Копирует строку полностью.
    @param dest Указатель на память куда скопировать.
    @param src Указатель на строку откуда копировать.
    @return Указатель на скопированую строку.
*/
KAPI char* platform_string_copy(char* dest, const char* src);

/*
    @brief Копирует заданное количество байт строки.
    @param dest Указатель на память куда скопировать.
    @param src Указатель на строку откуда копировать.
    @param length Максимальное количество символов для копирования.
    @return Указатель на скопированую строку.
*/
KAPI char* platform_string_ncopy(char* dest, const char* src, u64 length);

/*
    @brief Указывает является ли указанный символ пробелом.
    @param c Указатель на символ для проверки.
    @return True указанный символ является пробелом, false другой символ.
*/
KAPI bool platform_string_isspace(char c);

/*
    @brief Читает строку и записывает в указанные переменные в соответствии с заданной
           форматной строкой.
    @param str Указатель на строку для выполнения чтения данных.
    @param format Форматная строка.
    @param ... Указатели на память для сохранения параметров в соответствии со строкой формата.
    @return Количество полученных элементов, -1 в случае ошибки в строке для чтения.
*/
KAPI i32 platform_string_sscanf(const char* str, const char* format, ...);
