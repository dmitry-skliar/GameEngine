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
    @param path Строка или указатель на строку пути к файлу.
    @return True файл существует, false не найден.
*/
KAPI bool platform_file_exists(const char* path);

/*
    @brief Открывает файл по указанному пути.
    @param path Строка или указатель на строку пути к файлу.
    @param mode Режим открытия файла.
    @param out_file Указатель на экземпляр файла.
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
    @brief Читает текстовую строку из файла.
    @param file Указатель на экземпляр файла.
    @param line_buf Указатель на строку, выделяет память под строку.
    @param True успешно считано, false не удалось прочитать.
*/
KAPI bool platform_file_read_line(file* file, char** line_buf);

/*
    @brief Записывает текстовую строку в файл.
    @param file Указатель на экземпляр файла.
    @param text Строка или указатель на строку, с нулевым символом в конце.
    @return True успешно записано, false не удалось записать.
*/
KAPI bool platform_file_write_line(file* file, const char* text);

/*
    @brief Считывает заданное количество байт из файла.
    @param file Указатель на экземпляр файла.
    @param data_size Количество байт для чтения.
    @param data Указатель на память куда следует записать данные.
    @param True успешно считано, false не удалось прочитать.
*/
KAPI bool platfrom_file_read(file* file, u64 data_size, void* data);

/*
    @brief Считывает все байты из файла.
    @param file Указатель на экземпляр файла.
    @param data_size Указатель на 
    @param out_data Указатель на память, выделяет память под строку.
    @param True успешно считано, false не удалось прочитать.
*/
KAPI bool platform_file_reads(file* file, u64* data_size, void** out_data);

/*
    @brief Записывает заданное количество байт в файл.
    @param file Указатель на экземпляр файла.
    @param data_size Количество байт для записи.
    @param Указатель на память откуда следует брать данные.
    @param True успешно считано, false не удалось прочитать.
*/
KAPI bool platform_file_write(file* file, u64 data_size, const void* data);
