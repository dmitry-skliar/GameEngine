#pragma once

#include <defines.h>

/*
    @brief Получает количество символов предоставленной строки.
    @param str Указатель на строку.
    @return Количество символов строки.
*/
KAPI u64 string_get_length(const char* str);

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

/*
    @brief Посимвольно сравнивает две строки с учетом регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @return True - если строки одинаковые, false - разные.
*/
KAPI bool string_is_equal(const char* lstr, const char* rstr);

/*
    TODO: Реализовать!
    @brief Посимвольно сравнивает две строки без учетом регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @return True - если строки одинаковые, false - разные.
*/
KAPI bool string_is_equali(const char* lstr, const char* rstr);
