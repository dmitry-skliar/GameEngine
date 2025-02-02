// Cобственные подключения.
#include "kstring.h"

// Внутренние подключения.
#include "memory/memory.h"
#include "platform/string.h"

char* string_duplicate(const char* str)
{
    u64 length = platform_string_length(str) + 1;
    char* newstr = kallocate(length, MEMORY_TAG_STRING);
    kcopy(newstr, str, length);
    return newstr;
}

void string_free(const char* str)
{
    u64 length = platform_string_length(str) + 1;
    kfree(str, length, MEMORY_TAG_STRING);
}
