// Cобственные подключения.
#include "platform/memory.h"

#if KPLATFORM_LINUX_FLAG

    // Внутренние подключения.
    // ...

    // Внешние подключения.
    #include <stdlib.h>
    #include <string.h>

    void* platform_memory_allocate(u64 size)
    {
        return malloc(size);
    }

    void platform_memory_free(void* block)
    {
        free(block);
    }

    void platform_memory_zero(void* block, u64 size)
    {
        memset(block, 0, size);
    }

    void platform_memory_set(void* block, u64 size, i32 value)
    {
        memset(block, value, size);
    }

    void platform_memory_copy(void* dest, const void* src, u64 size)
    {
        memcpy(dest, src, size);
    }

    void platform_memory_move(void* dest, const void* src, u64 size)
    {
        memmove(dest, src, size);
    }

#endif
