// Cобственные подключения.
#include "kstring.h"

// Внутренние подключения.
#include "memory/memory.h"
#include "platform/string.h"

u64 string_get_length(const char* str)
{
    return platform_string_get_length(str);
}

char* string_duplicate(const char* str)
{
    u64 length = platform_string_get_length(str) + 1;
    char* newstr = kmallocate(length, MEMORY_TAG_STRING);
    kmcopy(newstr, str, length);
    return newstr;
}

void string_free(const char* str)
{
    kmfree(str);
}

bool string_is_equal(const char* lstr, const char* rstr)
{
    return platform_string_is_equal(lstr, rstr);
}
