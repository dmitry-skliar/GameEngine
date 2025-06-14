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

linear_allocator* linear_allocator_create(u64 size)
{
    if(!size)
    {
        kerror("Function '%s' require a size greater than zero. Return null!", __FUNCTION__);
        return null;
    }

    u64 total_size = sizeof(struct linear_allocator) + size;
    linear_allocator* allocator = kallocate(total_size, MEMORY_TAG_ALLOCATOR);

    if(!allocator)
    {
        kerror("Function '%s': Failed to allocate memory. Return null!", __FUNCTION__);
        return null;
    }

    allocator->size = size;
    allocator->allocated = 0;
    return allocator;
}

void linear_allocator_destroy(linear_allocator* allocator)
{
    if(!allocator)
    {
        kerror("Function '%s' require a pointer to an instance of allocator.", __FUNCTION__);
        return;
    }

    kfree(allocator, MEMORY_TAG_ALLOCATOR);
}

void* linear_allocator_allocate(linear_allocator* allocator, u64 size)
{
    if(!allocator)
    {
        kerror("Function '%s' require a pointer to an instance of allocator. Return null!", __FUNCTION__);
        return null;
    }

    if(!size)
    {
        kerror("Function '%s' require a size greater than zero. Return null!", __FUNCTION__);
        return null;
    }

    if(allocator->allocated + size <= allocator->size)
    {
        void* block = (u8*)allocator + sizeof(struct linear_allocator) + allocator->allocated;
        allocator->allocated += size;
        return block;
    }

    f32 requested_amount = 0;
    f32 remaining_amount = 0;
    const char* requested_unit = memory_get_unit_for(size, &requested_amount);
    const char* remaining_unit = memory_get_unit_for(allocator->size - allocator->allocated, &remaining_amount);

    kerror(
        "Function '%s' tried to allocate %.2f %s, only %.2f %s remaining.",
        __FUNCTION__, requested_amount, requested_unit, remaining_amount, remaining_unit
    );
    return null;
}

void linear_allocator_free_all(linear_allocator* allocator)
{
    if(!allocator)
    {
        kerror("Function '%s' require a pointer to an instance of allocator.", __FUNCTION__);
        return;
    }

    allocator->allocated = 0;
}
