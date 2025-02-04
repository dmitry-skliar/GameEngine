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
        char mode_str[4];

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

        mode_str[index] = 0;

        *out_file = kallocate_tc(struct file, 1, MEMORY_TAG_STRING);
        kzero_tc(*out_file, struct file, 1);

        // Попытка открыть файл.
        FILE* file = fopen(path, mode_str);

        if(!file)
        {
            kerror("Function '%s': Error opening file '%s'.", __FUNCTION__, path);
            return false;
        }

        (*out_file)->handle = file;

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
        kfree_tc(file, struct file, 1, MEMORY_TAG_STRING);
    }

    bool platform_file_read_line(file* file, char** line_buf)
    {
        if(!file || !file->handle)
        {
            kerror("Function '%s' requires a valid pointer to file.", __FUNCTION__);
            return false;
        }

        char buffer[32000];
        if(!fgets(buffer, 32000, file->handle))
        {
            return false;
        }

        u64 length = strlen(buffer) + 1;
        *line_buf = kallocate_tc(char, length, MEMORY_TAG_STRING);
        strcpy(*line_buf, buffer);
        return true;
    }

    bool platform_file_write_line(file* file, const char* text)
    {
        if(!file || !file->handle)
        {
            kerror("Function '%s' requires a valid pointer to file.", __FUNCTION__);
            return false;
        }

        if(!text)
        {
            kerror("Function '%s' requires a valid pointer to string.", __FUNCTION__);
            return false;
        }
        
        i32 result = fputs(text, file->handle);
        if(result != EOF)
        {
            result = fputc('\n', file->handle);
        }

        fflush(file->handle);
        return result != EOF;
    }

    bool platfrom_file_read(file* file, u64 data_size, void* data)
    {
        if(!file || !file->handle)
        {
            kerror("Function '%s' requires a valid pointer to file.", __FUNCTION__);
            return false;
        }

        if(!data_size || !data)
        {
            kerror("Function '%s' requires a data size greater than zero and a pointer to data.", __FUNCTION__);
            return false;
        }

        u64 read = fread(data, 1, data_size, file->handle);
        return read == data_size ? true : false;
    }
    
    bool platform_file_reads(file* file, u64* out_data_size, void** out_data)
    {
        if(!file || !file->handle)
        {
            kerror("Function '%s' requires a valid pointer to file.", __FUNCTION__);
            return false;
        }

        // Получаем размер файла.
        fseek(file->handle, 0, SEEK_END);
        u64 size = ftell(file->handle);
        rewind(file->handle);

        *out_data = kallocate_tc(u8, size, MEMORY_TAG_STRING);
        *out_data_size = fread(*out_data, 1, size, file->handle);
        return size == *out_data_size ? true : false;
    }

    bool platform_file_write(file* file, u64 data_size, const void* data)
    {
        if(!file || !file->handle)
        {
            kerror("Function '%s' requires a valid pointer to file.", __FUNCTION__);
            return false;
        }

        if(!data_size || !data)
        {
            kerror("Function '%s' requires a data size greater than zero and a pointer to data.", __FUNCTION__);
            return false;
        }

        i64 written = fwrite(data, 1, data_size, file->handle);
        fflush(file->handle);
        return written == data_size ? true : false;
    }

#endif
