// Cобственные подключения.
#include "kstring.h"

// Внутренние подключения.
#include "memory/memory.h"
#include "platform/string.h"

char* string_duplicate(const char* str)
{
    if(!str) return null;

    u64 length = platform_string_length(str) + 1;
    char* newstr = kallocate(length, MEMORY_TAG_STRING);
    kcopy(newstr, str, length);
    return newstr;
}

void string_free(const char* str)
{
    if(!str) return;

    u64 length = platform_string_length(str) + 1;
    kfree(str, length, MEMORY_TAG_STRING);
}

char* string_trim(char* str)
{
    if(!str) return null;

    // Пропуск символов вначале строки.
    while(platform_string_isspace(*str)) ++str;

    // Получение длинны строки и пропуск символов в конце строки.
    u64 index = platform_string_length(str);
    while(platform_string_isspace(str[--index]));

    // Завершающий символ строки.
    str[index + 1] = '\0';
    return str;
}

void string_mid(char* dest, const char* src, u64 start, i64 length)
{
    if(!dest || !src || length == 0)
    {
        return;
    }

    u64 src_length = platform_string_length(src);
    if(start >= src_length)
    {
        dest[0] = '\0';
        return;
    }

    u64 index = 0;
    src += start;

    if(length > 0)
    {
        while(index < length && src[index])
        {
            dest[index] = src[index];
            index++;
        }
    }
    else
    {
        while(src[index])
        {
            dest[index] = src[index];
            index++;
        }
    }

    dest[index] = '\0';
}

i64 string_index_of(char* str, char c)
{
    if(str)
    {
        u64 str_length = platform_string_length(str);
        for(u64 i = 0; i < str_length; ++i)
        {
            if(str[i] == c) return i;
        }
    }

    return -1;
}

bool string_to_vec4(char* str, vec4* out_vector)
{
    if(!str) return false;

    kzero_tc(out_vector, vec4, 1);
    i32 result = platform_string_sscanf(
        str, "%f %f %f %f", &out_vector->x, &out_vector->y, out_vector->z, &out_vector->w
    );

    return result > 0;
}

bool string_to_vec3(char* str, vec3* out_vector)
{
    if(!str) return false;

    kzero_tc(out_vector, vec3, 1);
    i32 result = platform_string_sscanf(str, "%f %f %f", &out_vector->x, &out_vector->y, out_vector->z);

    return result > 0;
}

bool string_to_vec2(char* str, vec2* out_vector)
{
    if(!str) return false;

    kzero_tc(out_vector, vec2, 1);
    i32 result = platform_string_sscanf(str, "%f %f", &out_vector->x, &out_vector->y);

    return result > 0;
}

bool string_to_f32(char *str, f32 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%f", value);

    return result > 0;
}

bool string_to_f64(char *str, f64 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%lf", value);

    return result > 0;
}

bool string_to_i8(char *str, i8 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%hhi", value);

    return result > 0;
}

bool string_to_i16(char *str, i16 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%hi", value);

    return result > 0;
}

bool string_to_i32(char *str, i32 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%i", value);

    return result > 0;
}

bool string_to_i64(char *str, i64 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%lli", value);

    return result > 0;
}

bool string_to_u8(char *str, u8 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%hhu", value);

    return result > 0;
}

bool string_to_u16(char *str, u16 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%hu", value);

    return result > 0;
}

bool string_to_u32(char *str, u32 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%u", value);

    return result > 0;
}

bool string_to_u64(char *str, u64 *value)
{
    if(!str) return false;

    *value = 0;
    i32 result = platform_string_sscanf(str, "%llu", value);

    return result > 0;
}

bool string_to_bool(char* str, bool* value)
{
    if(!str) return false;

    return platform_string_equal(str, "1") || platform_string_equali(str, "true");
}
