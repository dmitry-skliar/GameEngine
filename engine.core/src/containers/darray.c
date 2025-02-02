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
        kerror("Function '%s' requires stride and capacity more than zero. Return null!", __FUNCTION__);
        return null;
    }

    u64 array_size = stride * capacity;
    u64 total_size = sizeof(struct dynamic_array_header) + array_size;
    dynamic_array_header* header = kallocate(total_size, MEMORY_TAG_DARRAY);
    if(header)
    {
        kzero_tc(header, dynamic_array_header, 1);
        header->capacity = capacity;
        header->length = 0;
        header->stride = stride;
        return (void*)((u8*)header + sizeof(struct dynamic_array_header));
    }

    kfatal("In function '%s' failed to allocate memory!", __FUNCTION__);
    return null;
}

// TODO: Сделать внутренний ресайз!
void* dynamic_array_resize(void* array, u64 capacity)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return null;
    }

    dynamic_array_header* old_array = (void*)((u8*)array - sizeof(struct dynamic_array_header));

    if(capacity == 0 || capacity <= old_array->capacity)
    {
        kwarng("Function '%s' requires a capacity more than %d. Return old array!", __FUNCTION__, old_array->capacity);
        return array;
    }

    u64 old_array_total_size = sizeof(struct dynamic_array_header) + old_array->stride * old_array->capacity;
    u64 new_array_total_size = sizeof(struct dynamic_array_header) + old_array->stride * capacity;

    void* new_array = kallocate(new_array_total_size, MEMORY_TAG_DARRAY);
    if(new_array)
    {
        // Подготовка к копированию из старого в новый.
        u64 old_array_data_size = sizeof(struct dynamic_array_header) + old_array->stride * old_array->length;
        old_array->capacity = capacity; // Заранее обновляем емкость.

        kcopy(new_array, old_array, old_array_data_size);
        kfree(old_array, old_array_total_size, MEMORY_TAG_DARRAY);

        return (void*)((u8*)new_array + sizeof(struct dynamic_array_header));
    }

    kerror("In function '%s' memory was not allocated! Returne old array!", __FUNCTION__);
    return array;
}

void dynamic_array_destroy(void* array)
{
    if(array)
    {
        dynamic_array_header* header = (void*)((u8*)array - sizeof(struct dynamic_array_header));
        u64 total_size = sizeof(struct dynamic_array_header) + header->stride * header->capacity;
        kfree(header, total_size, MEMORY_TAG_DARRAY);
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

    dynamic_array_header* header = (void*)((u8*)array - sizeof(struct dynamic_array_header));

    if(header->length >= header->capacity)
    {
        array = dynamic_array_resize(array, header->capacity * DARRAY_RESIZE_FACTOR);
        header = (void*)((u8*)array - sizeof(struct dynamic_array_header));
    }

    u8* addr = (u8*)array + (header->stride * header->length);
    kcopy(addr, element, header->stride);
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

    dynamic_array_header* header = (void*)((u8*)array - sizeof(struct dynamic_array_header));

    if(index >= header->length)
    {
        kerror(message_index_out_of_bounds, __FUNCTION__, header->length, index);
        return array;
    }

    if(header->length >= header->capacity)
    {
        array = dynamic_array_resize(array, header->capacity * DARRAY_RESIZE_FACTOR);
        header = (void*)((u8*)array - sizeof(struct dynamic_array_header));
    }

    u8* addr = (u8*)array + (header->stride * index);
    kmove(addr + header->stride, addr, header->stride * (header->length - index));
    kcopy(addr, element, header->stride);
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

    dynamic_array_header* header = (void*)((u8*)array - sizeof(struct dynamic_array_header));

    if(header->length < 1)
    {
        kerror(message_empty_array, __FUNCTION__);
        return;
    }

    if(dest)
    {
        u8* addr = (u8*)array + (header->stride * (header->length - 1));
        kcopy(dest, addr, header->stride);
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

    dynamic_array_header* header = (void*)((u8*)array - sizeof(struct dynamic_array_header));

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
        kcopy(dest, addr, header->stride);
    }

    if(index != (header->length - 1))
    {
        kmove(addr, addr + header->stride, header->stride * (header->length - (index + 1)));
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

    dynamic_array_header* header = (void*)((u8*)array - sizeof(struct dynamic_array_header));
    header->length = 0;
}

u64 dynamic_array_length(void* array)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return 0;
    }

    dynamic_array_header* header = (void*)((u8*)array - sizeof(struct dynamic_array_header));
    return header->length;    
}

u64 dynamic_array_capacity(void* array)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return 0;
    }

    dynamic_array_header* header = (void*)((u8*)array - sizeof(struct dynamic_array_header));
    return header->capacity;
}

u64 dynamic_array_stride(void* array)
{
    if(!array)
    {
        kerror(message_requires_a_pointer, __FUNCTION__);
        return 0;
    }

    dynamic_array_header* header = (void*)((u8*)array - sizeof(struct dynamic_array_header));
    return header->stride;
}

