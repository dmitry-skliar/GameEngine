// Cобственные подключения.
#include "containers/darray.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

typedef struct dynamic_array_header {
    u64 capacity;
    u64 stride;
    u64 length;
} dynamic_array_header;

// Сообщения.
static const char* message_requires_a_pointer = "Function '%s' requires a pointer to array.";
static const char* message_requires_a_pointer_and_an_element = "Function '%s' requires a pointer to array and a pointer to element.";
static const char* message_empty_array = "Function '%s' called on an empty array";
static const char* message_index_out_of_bounds = "In function '%s' index out of bounds (length %d, index %d).";

void* dynamic_array_create(u64 stride, u64 capacity)
{
    if(stride == 0 || capacity == 0)
    {
        kerror("Function '%s' requires stride and capacity more than 0.", __FUNCTION__);
        return null;
    }

    u64 array_size = stride * capacity;
    u64 total_size = sizeof(dynamic_array_header) + array_size;
    dynamic_array_header* header = kmallocate(total_size, MEMORY_TAG_DARRAY);

    if(header)
    {
        kmzero_tc(header, dynamic_array_header, 1);
        header->capacity = capacity;
        header->length = 0;
        header->stride = stride;
        return (void*)((u8*)header + sizeof(dynamic_array_header));
    }

    kerror("In function '%s' memory was not allocated! Return null!", __FUNCTION__);
    return null;
}

void* dynamic_array_resize(void* array, u64 capacity)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return null;
    }

    dynamic_array_header* old_array = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));

    if(capacity == 0 || capacity <= old_array->capacity)
    {
        kwarng("Function '%s' requires a capacity more than %d. Return old array!", __FUNCTION__, old_array->capacity);
        return array;
    }

    old_array->capacity = capacity;
    u64 new_total_size = capacity * old_array->stride + sizeof(dynamic_array_header);
    void* new_array = kmallocate(new_total_size, MEMORY_TAG_DARRAY);

    if(new_array)
    {
        u64 old_data_size = old_array->stride * old_array->length + sizeof(dynamic_array_header);
        kmcopy(new_array, old_array, old_data_size);
        kmfree(old_array);
        return (void*)((u8*)new_array + sizeof(dynamic_array_header));
    }

    kerror("In function '%s' memory was not allocated! Returne old array!", __FUNCTION__);
    return array;
}

void dynamic_array_destroy(void* array)
{
    if(array)
    {
        array = (void*)((u8*)array - sizeof(dynamic_array_header));
        kmfree(array);
    }
    else
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
    }
}

void* dynamic_array_push(void* array, const void* element)
{
    if(!array || !element)
    {
        kerror(message_requires_a_pointer_and_an_element, __FUNCTION__);
        return null;
    }

    dynamic_array_header* header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));

    if(header->length >= header->capacity)
    {
        array = dynamic_array_resize(array, header->capacity * DARRAY_RESIZE_FACTOR);
        header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));
    }

    u8* addr = (u8*)array + (header->stride * header->length);
    kmcopy(addr, element, header->stride);
    header->length += 1;

    return array;
}

void* dynamic_array_insert_at(void* array, u64 index, const void* element)
{
    if(!array || !element)
    {
        kerror(message_requires_a_pointer_and_an_element, __FUNCTION__);
        return null;
    }

    dynamic_array_header* header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));

    if(index >= header->length)
    {
        kerror(message_index_out_of_bounds, __FUNCTION__, header->length, index);
        return array;
    }

    if(header->length >= header->capacity)
    {
        array = dynamic_array_resize(array, header->capacity * DARRAY_RESIZE_FACTOR);
        header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));
    }

    u8* addr = (u8*)array + (header->stride * index);
    kmmove(addr + header->stride, addr, header->stride * (header->length - index));
    kmcopy(addr, element, header->stride);
    header->length += 1;

    return array;
}

void dynamic_array_pop(void* array, void* dest)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return;
    }

    dynamic_array_header* header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));

    if(header->length < 1)
    {
        kerror(message_empty_array, __FUNCTION__);
        return;
    }

    if(dest)
    {
        u8* addr = (u8*)array + (header->stride * (header->length - 1));
        kmcopy(dest, addr, header->stride);
    }

    header->length -= 1;
}

void dynamic_array_pop_at(void* array, u64 index, void* dest)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return;
    }

    dynamic_array_header* header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));

    if(header->length < 1)
    {
        kerror(message_empty_array, __FUNCTION__);
        return;
    }

    if(index >= header->length)
    {
        kerror(message_index_out_of_bounds, __FUNCTION__, header->length, index);
        return;
    }

    u8* addr = (u8*)array + (header->stride * index);

    if(dest)
    {
        kmcopy(dest, addr, header->stride);
    }

    if(index != (header->length - 1))
    {
        kmmove(addr, addr + header->stride, header->stride * (header->length - (index + 1)));
    }

    header->length -= 1;
}

void dynamic_array_clear(void* array)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return;
    }

    dynamic_array_header* header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));
    header->length = 0;
}

u64 dynamic_array_get_length(void* array)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return 0;
    }

    dynamic_array_header* header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));
    return header->length;    
}

u64 dynamic_array_get_capacity(void* array)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return 0;
    }

    dynamic_array_header* header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));
    return header->capacity;
}

u64 dynamic_array_get_stride(void* array)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return 0;
    }

    dynamic_array_header* header = (dynamic_array_header*)((u8*)array - sizeof(dynamic_array_header));
    return header->stride;
}

