// Собственные подключения.
#include "memory/allocators/linear_allocator.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

struct linear_allocator {
    // Размер внутреннего контейнера в байтах.
    u64 size;
    // Размер занятой память в байтах.
    u64 allocated;
};

linear_allocator linear_allocator_create(u64 size)
{
    if(!size)
    {
        kfatal("Function '%s' require a size greater than zero.", __FUNCTION__);
    }

    u64 total_size = sizeof(struct linear_allocator) + size;
    linear_allocator allocator = kallocate(total_size, MEMORY_TAG_LINEAR_ALLOCATOR);

    if(!allocator)
    {
        kfatal("Failed to allocate memory in function '%s'.", __FUNCTION__);
    }

    allocator->size = size;
    allocator->allocated = 0;

    return allocator;
}

void linear_allocator_destroy(linear_allocator allocator)
{
    if(allocator)
    {
        u64 total_size = sizeof(struct linear_allocator) + allocator->size;
        kfree(allocator, total_size, MEMORY_TAG_LINEAR_ALLOCATOR);
        return;
    }

    kwarng("Function '%s' require a pointer to an instance of allocator.", __FUNCTION__);
}

void* linear_allocator_allocate(linear_allocator allocator, u64 size)
{
    if(!allocator)
    {
        kfatal("Function '%s' require a pointer to an instance of allocator.", __FUNCTION__);
    }

    if(!size)
    {
        kfatal("Function '%s' require a size greater than zero.", __FUNCTION__);
    }

    if(allocator->allocated + size <= allocator->size)
    {
        void* block = (u8*)allocator + sizeof(struct linear_allocator) + allocator->allocated;
        allocator->allocated += size;
        return block;
    }

    u64 remaining = allocator->size - allocator->allocated;
    kerror("Function '%s' tried to allocate %llu B, only %llu B remaining.", __FUNCTION__, size, remaining);
    return null;
}

void linear_allocator_free_all(linear_allocator allocator)
{
    if(!allocator)
    {
        kfatal("Function '%s' require a pointer to an instance of allocator.", __FUNCTION__);
    }

    allocator->allocated = 0;
}
