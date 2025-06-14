#pragma once

#include <defines.h>
#include <platform/string.h>
#include <math/math_types.h>

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
    @brief Посимвольно сравнивает две строки без учета регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @return True если строки одинаковые, false разные.
*/
#define string_equali(lstr, rstr) platform_string_equali(lstr, rstr)

/*
    @brief Посимвольно сравнивает заданое количество символов двух строк с учетом регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @param length Количество символов которое нужно сравнить.
    @return True если строки совпадают, false не совпадают.
*/
#define string_nequal(lstr, rstr, length) platform_string_nequal(lstr, rstr, length)

/*
    @brief Посимвольно сравнивает заданое количество символов двух строки без учета регистра символов.
    @param lstr Указатель на первую cтроку.
    @param rstr Указатель на вторую cтрока.
    @param length Количество символов которое нужно сравнить.
    @return True если строки совпадают, false не совпадают.
*/
#define string_nequali(lstr, rstr, length) platform_string_nequali(lstr, rstr, length)

/*
    @brief Записывает отформатированную строку в предоставленный буфер в соответствии с заданным параметрами.
    @note  Для получения требуемой длины без записи в буфер запиши в length = 0.
    @param dest Указатель на буфер куда будет записана отформатированная строка.
    @param length Максимальное количество символов, которое можно записать включая завершающий символ (\0).
    @param format Строка формата, которая указывает как должна выглядеть результирующая строка.
    @param ... Аргументы строки формата (должны соответствовать символам строки формата).
    @return В случае успеха количество записаных символов в буфер, в случае ошибки -1.
*/
#define string_format(dest, length, format, ...) platform_string_format(dest, length, format, ##__VA_ARGS__)

/*
    @brief Записывает отформатированную строку в предоставленный буфер в соответствии с заданным параметрами.
    @note  Для получения требуемой длины без записи в буфер запиши в length = 0.
    @param dest Указатель на буфер куда будет записана отформатированная строка.
    @param length Максимальное количество символов, которое можно записать включая завершающий символ (\0).
    @param format Строка формата, которая указывает как должна выглядеть результирующая строка.
    @param va_list Указатель на аргументы строки формата (должны соответствовать символам строки формата).
    @return В случае успеха количество записаных символов в буфер, в случае ошибки -1.
*/
#define string_format_va(dest, length, format, va_list) platform_string_format_va(dest, length, format, va_list)

/*
    @brief Записывает отформатированную строку в предоставленный буфер в соответствии с заданным параметрами.
    @note  Буфер должен иметь достаточный размер, т.к. функиця не проверяет переполнение буфера.
    @param dest Указатель на буфер куда будет записана отформатированная строка.
    @param format Строка формата, которая указывает как должна выглядеть результирующая строка.
    @param ... Аргументы строки формата (должны соответствовать символам строки формата).
    @return В случае успеха количество записаных символов в буфер, в случае ошибки -1.
*/
#define string_format_unsafe(dest, format, ...) platform_string_format_unsafe(dest, format, ##__VA_ARGS__)

/*
    @brief Записывает отформатированную строку в предоставленный буфер в соответствии с заданным параметрами.
    @note  Буфер должен иметь достаточный размер, т.к. функиця не проверяет переполнение буфера.
    @param dest Указатель на буфер куда будет записана отформатированная строка.
    @param format Строка формата, которая указывает как должна выглядеть результирующая строка.
    @param va_list Указатель на аргументы строки формата (должны соответствовать символам строки формата).
    @return В случае успеха количество записаных символов в буфер, в случае ошибки -1.
*/
#define string_format_va_unsafe(dest, format, va_list) platform_string_format_va_unsafe(dest, format, va_list)

/*
    @brief Создает копию предоставленной строки.
    @note  После использования удалить с помощью функуии 'string_free'.
    @param str Указатель на строку которую необходимо скопировать.
    @return Указатель на копию строки.
*/
KAPI char* string_duplicate(const char* str);

/*
    @brief Удаляет указанную строку, созданную функциями данной библиотеки.
    @param str Указатель на строку которую необходимо удалить.
*/
KAPI void string_free(const char* str);

/*
    @brief Очищает строку установкой первого символа в '\0'.
    @param str Указатель на строку которую необходимо очистить.
    @return Возвращает указатель на строку.
*/
KAPI char* string_empty(char* str);

/*
    @brief Копирует строку полностью.
    NOTE: Память куда копируется строка должны быть больше на 1 символ.
    @param dest Указатель на память куда скопировать.
    @param src Указатель на строку откуда копировать.
    @return Указатель на скопированую строку.
*/
#define string_copy(dest, src) platform_string_copy(dest, src)

/*
    @brief Копирует заданное количество байт строки.
    @param dest Указатель на память куда скопировать.
    @param src Указатель на строку откуда копировать.
    @param length Максимальное количество символов для копирования.
    @return Указатель на скопированую строку.
*/
#define string_ncopy(dest, src, length) platform_string_ncopy(dest, src, length)

/*
    @brief Удаляет из текущей строки все начальные и конечные символы пробелов.
    NOTE: Модифицирует строку.
    @param str Указатель на строку для удаления символов.
    @return Указатель на смещение в строке.
*/
KAPI char* string_trim(char* str);

/*
    @brief Получает подстроку из строки заданного размера.
    @param dest Указатель на память куда скопировать.
    @param src Указатель на строку откуда копировать.
*/
KAPI void string_mid(char* dest, const char* src, u64 start, i64 length);

/*
    @brief Указывает позицию первого совпадения искомого символа.
    @param str Указатель на строку для поиска.
    @param c Искомый символ.
    @return Позиция символа в строке, -1 если символ не найден.
*/
KAPI i64 string_index_of(char* str, char c);

/*
    @brief Попытка преобразовать строку (т.е. "1.0 2.0 3.0 4.0") в вектор.
    @param str Указатель на строку откуда читать. Резделитель - пробел.
    @param out_vector Указатель на вектор, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_vec4(char* str, vec4* out_vector);

/*
    @brief Попытка преобразовать строку (т.е. "1.0 2.0 3.0") в вектор.
    @param str Указатель на строку откуда читать. Резделитель - пробел.
    @param out_vector Указатель на вектор, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_vec3(char* str, vec3* out_vector);

/*
    @brief Попытка преобразовать строку (т.е. "1.0 2.0") в вектор.
    @param str Указатель на строку откуда читать. Резделитель - пробел.
    @param out_vector Указатель на вектор, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_vec2(char* str, vec2* out_vector);

/*
    @brief Попытка преобразовать строку в 32-bit число с плавающей точкой.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 32-bit число с плавающей точкой, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_f32(char* str, f32* value);

/*
    @brief Попытка преобразовать строку в 64-bit число с плавающей точкой.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 64-bit число с плавающей точкой, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_f64(char* str, f64* value);

/*
    @brief Попытка преобразовать строку в 8-bit целочисленное число со знаком.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 8-bit целочисленное число со знаком, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_i8(char* str, i8* value);

/*
    @brief Попытка преобразовать строку в 16-bit целочисленное число со знаком.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 16-bit целочисленное число со знаком, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_i16(char* str, i16* value);

/*
    @brief Попытка преобразовать строку в 32-bit целочисленное число со знаком.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 32-bit целочисленное число со знаком, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_i32(char* str, i32* value);

/*
    @brief Попытка преобразовать строку в 64-bit целочисленное число со знаком.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 64-bit целочисленное число со знаком, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_i64(char* str, i64* value);

/*
    @brief Попытка преобразовать строку в 8-bit целочисленное число без знака.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 8-bit целочисленное число без знака, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_u8(char* str, u8* value);

/*
    @brief Попытка преобразовать строку в 16-bit целочисленное число без знака.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 16-bit целочисленное число без знака, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_u16(char* str, u16* value);

/*
    @brief Попытка преобразовать строку в 32-bit целочисленное число без знака.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 32-bit целочисленное число без знака, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_u32(char* str, u32* value);

/*
    @brief Попытка преобразовать строку в 64-bit целочисленное число без знака.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на 64-bit целочисленное число без знака, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_u64(char* str, u64* value);

/*
    @brief Попытка преобразовать строку в логическое значение.
    NOTE: "True" или "true" или "1" - true, в остальных случаях false.
    @param str Указатель на строку откуда читать. Без модификаторов!
    @param value Указатель на логическое значение, куда записывать.
    @return True если преобразование прошло успешно, false если не удалось.
*/
KAPI bool string_to_bool(char* str, bool* value);

/*
    @brief Разбивает строку по предоставленому разделителю и сохраняет в динамический массив (darray).
    NOTE: Перед уничтожение массива необходимо освободить память вызовом функции 'string_cleanup_split_array'.
    @param str Указатель на строку над которой нужно провести разделение.
    @param delim Символ разделителя.
    @param trime_entries Обрезать каждую получившуюся строку перед добавлением в массив.
    @param include_empty Включать пустые строки в массив.
    @param str_darray Указатель на массив строк для получения результата (используется darray).
    @return Число получившихся строк в результате разделения, 0 если не удалось выполнить разделение.
*/
KAPI u32 string_split(const char* str, char delim, bool trime_entries, bool include_empty, char*** str_darray);

/*
    @brief Освобождает строки динамического массива в результате выполнения функции 'string_split'.
    @param str_darray Указатель на массив строк для освобождения выделенной памяти (используется darray).
*/
KAPI void string_cleanup_split_array(char** str_darray);

/*
    @brief Соединяет строку с предоставленым символом и возвращает новую строку.
    @param dest Указатель на память для записи новой строки.
    @param src Указатель на строку к которой нужно добавить.
    @param symbol Символ которуый нужно добавить.
*/
KAPI void string_append_char(char* dest, const char* src, const char symbol);

/*
    @brief Соединяет две строки и возвращает новую строку.
    @param dest Указатель на память для записи новой строки.
    @param src Указатель на строку к которой нужно добавить.
    @param str Указатель на строку которую нужно добавить.
*/
KAPI void string_append_string(char* dest, const char* src, const char* str);

// TODO: Написать для других чисел и типов данных!

/*
    @brief Соединяет строку с 32-bit целочисленным числом со знаком и возвращает новую строку.
    NOTE: Другие типы данных будут преобразованы в строку.
    @param dest Указатель на память для записи новой строки.
    @param src Указатель на строку к которой нужно добавить.
    @param value Число которое нужно добавить.
*/
KAPI void string_append_i32(char* dest, const char* src, i32 value);

/*
    @brief Соединяет строку с 64-bit целочисленным числом со знаком и возвращает новую строку.
    NOTE: Другие типы данных будут преобразованы в строку.
    @param dest Указатель на память для записи новой строки.
    @param src Указатель на строку к которой нужно добавить.
    @param value Число которое нужно добавить.
*/
KAPI void string_append_i64(char* dest, const char* src, i64 value);

/*
    @brief Соединяет строку с 32-bit целочисленным беззнаковым числом и возвращает новую строку.
    NOTE: Другие типы данных будут преобразованы в строку.
    @param dest Указатель на память для записи новой строки.
    @param src Указатель на строку к которой нужно добавить.
    @param value Число которое нужно добавить.
*/
KAPI void string_append_u32(char* dest, const char* src, u32 value);

/*
    @brief Соединяет строку с 64-bit целочисленным беззнаковым числом и возвращает новую строку.
    NOTE: Другие типы данных будут преобразованы в строку.
    @param dest Указатель на память для записи новой строки.
    @param src Указатель на строку к которой нужно добавить.
    @param value Число которое нужно добавить.
*/
KAPI void string_append_u64(char* dest, const char* src, u64 value);

/*
    @brief Соединяет строку с 32-bit числом с плавающей точкой и возвращает новую строку.
    NOTE: Другие типы данных будут преобразованы в строку.
    @param dest Указатель на память для записи новой строки.
    @param src Указатель на строку к которой нужно добавить.
    @param value Число которое нужно добавить.
*/
KAPI void string_append_f32(char* dest, const char* src, f32 value);

/*
    @brief Соединяет строку с логическим значением и возвращает новую строку.
    NOTE: Другие типы данных будут преобразованы в строку.
    @param dest Указатель на память для записи новой строки.
    @param src Указатель на строку к которой нужно добавить.
    @param value Логическое значение которое нужно добавить.
*/
KAPI void string_append_bool(char* dest, const char* src, bool value);

/*
    @brief Извлекает путь к директории файла из полного пути.
    @param directory Указатель на память куда записать имя директории.
    @param path Указатель на строку полного пути откуда извлечь.
*/
KAPI void string_directory_from_path(char* directory, const char* path);

/*
    @brief Извлекает имя файла из полного пути.
    @param directory Указатель на память куда записать имя файла.
    @param path Указатель на строку полного пути откуда извлечь.
    @param with_extention Указывает на получение имени файла с расширением.
*/
KAPI void string_filename_from_path(char* filename, const char* path, bool with_extension);
