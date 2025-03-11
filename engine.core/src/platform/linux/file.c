// Собственные подключения.
#include "platform/file.h"

#if KPLATFORM_LINUX_FLAG

    // Внутренние подключения.
    #include "logger.h"
    #include "memory/memory.h"

    // Внешние подключения.
    #include <stdio.h>
    #include <string.h>
    #include <sys/stat.h>

    struct file {
        FILE* handle;
        u64 size;
    };

    bool platform_file_exists(const char* path)
    {
        struct stat buffer;
        return stat(path, &buffer) == 0;
    }

    bool platform_file_open(const char* path, file_mode mode, file** out_file)
    {
        if(!path || !mode)
        {
            kfatal("Function '%s' requires a valid pointer to path and mode.", __FUNCTION__);
            return false;
        }

        u8 index = 0;
        char mode_str[4] = {};

        if(mode & FILE_MODE_WRITE)
        {
            mode_str[index++] = 'w';
        }

        if(mode & FILE_MODE_READ)
        {
            if(index > 0)
            {
                mode_str[index++] = '+';
            }
            else
            {
                mode_str[index++] = 'r';
            }
        }

        if(mode & FILE_MODE_BINARY)
        {
            mode_str[index++] = 'b';
        }

        // NOTE: Раскоментировать для отладки.
        // kdebug("For file '%s' in mode %s.", path, mode_str);

        mode_str[index] = 0;

        // Попытка открыть файл.
        FILE* file = fopen(path, mode_str);

        if(!file)
        {
            kerror("Function '%s': Error opening file '%s'.", __FUNCTION__, path);
            return false;
        }

        // Чтение размера файла.
        fseek(file, 0, SEEK_END);
        u64 filesize = ftell(file);
        rewind(file);

        if(!filesize && !(mode & FILE_MODE_WRITE))
        {
            kerror("Function '%s': Failed to get file size of file '%s'.", __FUNCTION__, path);
        }

        *out_file = kallocate_tc(struct file, 1, MEMORY_TAG_FILE);
        kzero_tc(*out_file, struct file, 1);

        // Сохранение информации о файле.
        (*out_file)->handle = file;
        (*out_file)->size = filesize;

        return true;
    }

    void platform_file_close(file* file)
    {
        if(!file || !file->handle)
        {
            kerror("Function '%s' requires a valid pointer to file.", __FUNCTION__);
            return;
        }

        fclose(file->handle);
        kfree_tc(file, struct file, 1, MEMORY_TAG_FILE);
    }

    u64 platform_file_size(file* file)
    {
        return file->size;
    }

    bool platform_file_read_line(file *file, u64 buffer_size, char* buffer, u64* out_length)
    {
        if(!file || !file->handle || !buffer || !buffer_size)
        {
            kerror(
                "Function '%s' requires a valid pointer to file, buffer and buffer size greater then zero.",
                __FUNCTION__
            );
            return false;
        }

        if(!fgets(buffer, buffer_size, file->handle))
        {
            return false;
        }

        *out_length = strlen(buffer);
        return true;
    }

    bool platform_file_write_line(file* file, const char* string)
    {
        if(!file || !file->handle || !string)
        {
            kerror("Function '%s' requires a valid pointer to file and string.", __FUNCTION__);
            return false;
        }
        
        i32 result = fputs(string, file->handle);
        if(result != EOF)
        {
            result = fputc('\n', file->handle);
        }

        fflush(file->handle);
        return result != EOF;
    }

    bool platfrom_file_read(file* file, u64 buffer_size, void* buffer)
    {
        if(!file || !file->handle || !buffer || !buffer_size)
        {
            kerror(
                "Function '%s' requires a valid pointer to file, buffer and buffer size greater than zero.",
                __FUNCTION__
            );
            return false;
        }

        u64 size = fread(buffer, 1, buffer_size, file->handle);
        return size == buffer_size;
    }

    bool platform_file_write(file* file, u64 data_size, const void* data)
    {
        if(!file || !file->handle || !data || !data_size)
        {
            kerror(
                "Function '%s' requires a valid pointer to file, data and data size greater than zero.",
                __FUNCTION__
            );
            return false;
        }

        i64 written = fwrite(data, 1, data_size, file->handle);
        fflush(file->handle);
        return written == data_size;
    }

    bool platform_file_read_all_bytes(file* file, void* buffer, u64* out_size)
    {
        if(!file || !file->handle || !buffer || !out_size)
        {
            kerror("Function '%s' requires a valid pointer to file, buffer and out_size.", __FUNCTION__);
            return false;
        }

        *out_size = fread(buffer, 1, file->size, file->handle);
        return *out_size == file->size;
    }

#endif
