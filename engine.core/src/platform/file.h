#pragma once

#include <defines.h>

// @brief Экземпляр файла.
typedef struct file file;

// @brief Режим открытия файла, комбинируемый.
typedef enum file_mode {
    FILE_MODE_READ   = 0x01,
    FILE_MODE_WRITE  = 0x02,
    FILE_MODE_BINARY = 0x04
} file_mode;

/*
    @brief Проверяет, существует ли файл по указанному пути.
    @param path Указатель на строку пути к файлу.
    @return True файл существует, false не найден.
*/
KAPI bool platform_file_exists(const char* path);

/*
    @brief Открывает файл по указанному пути.
    @param path Указатель на строку пути к файлу.
    @param mode Режим открытия файла.
    @param out_file Указатель на память куда будет сохранен указатель на экземпляр файла.
    @return True файл открыт успешно, false не удалось открыть.
*/
KAPI bool platform_file_open(const char* path, file_mode mode, file** out_file);

/*
    @brief Закрывает файл.
    NOTE: Указатель необходимо обнулить самостоятельно!
    @param file Указатель на экземпляр файла.
*/
KAPI void platform_file_close(file* file);

/*
    @brief Получает размера файла в байтах.
    @param file Указатель на экземпляр файла.
    @return Размер файла в байтах.
*/
KAPI u64 platform_file_size(file* file);

/*
    @brief Читает очередную текстовую строку из файла.
    NOTE: Буфер нужно выделять достаточный для чтения строки.
    @param file Указатель на экземпляр файла.
    @param buffer_size Размер буфера и максимально возможное количество считываемых символов.
    @param buffer Указатель на буфер, куда будет записана строка.
    @param out_length Указатель на память, куда будет записано количество прочитаных символов.
    @param True успешно считано, false не удалось прочитать.
*/
KAPI bool platform_file_read_line(file *file, u64 buffer_size, char* buffer, u64* out_length);

/*
    @brief Записывает текстовую строку в файл.
    @param file Указатель на экземпляр файла.
    @param string Указатель на строку, с нулевым символом в конце.
    @return True успешно записано, false не удалось записать.
*/
KAPI bool platform_file_write_line(file* file, const char* string);

/*
    @brief Читает заданное количество байт из файла.
    @param file Указатель на экземпляр файла.
    @param buffer_size Размер буфера и максимально возможное количество считываемых байт.
    @param buffer Указатель на буфер, куда будут записаны байты.
    @param True успешно считано, false не удалось прочитать.
*/
KAPI bool platfrom_file_read(file* file, u64 buffer_size, void* buffer);

/*
    @brief Записывает заданное количество байт в файл.
    @param file Указатель на экземпляр файла.
    @param data_size Количество байт для записи.
    @param Указатель на память откуда следует брать данные.
    @param True успешно записано, false не удалось записать.
*/
KAPI bool platform_file_write(file* file, u64 data_size, const void* data);

/*
    @brief Читает все байты из файла.
    NOTE: Буфер нужно выделять достаточный для чтения файла.
    @param file Указатель на экземпляр файла.
    @param buffer Указатель на буфер, куда будут записаны байты.
    @param out_size Указатель на память, куда будет записано количество считаных байт в буфер.
    @param True успешно считано, false не удалось прочитать.
*/
KAPI bool platform_file_read_all_bytes(file* file, void* buffer, u64* out_size);

/*
    @brief Читает все символы из файла.
    NOTE: Буфер нужно выделять достаточный для чтения файла.
    @param file Указатель на экземпляр файла.
    @param buffer Указатель на буфер, куда будут записаны символы.
    @param out_size Указатель на память, куда будет записано количество считаных символов в буфер.
    @param True успешно считано, false не удалось прочитать.
*/
KINLINE bool platform_file_read_all_text(file* file, char* buffer, u64* out_size)
{
    return platform_file_read_all_bytes(file, buffer, out_size);
}
