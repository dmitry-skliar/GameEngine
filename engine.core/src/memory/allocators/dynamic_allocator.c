// Собственные подключения.
#include "memory/allocators/dynamic_allocator.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

typedef struct dynamic_allocator_node {
    u64 size;
    struct dynamic_allocator_node * next;
} dynamic_allocator_node ;

struct dynamic_allocator {
    // Максимальный размера памяти.
    u64 total_size;
    // Доступный размер памяти в данный момент.
    u64 current_size;
    // Указатель блок памяти.
    void* memory_block;
    // Количество свободных блоков памяти.
    u64 block_count;
    // Указатель на первый свободный блок памяти.
    dynamic_allocator_node * head;
};


dynamic_allocator* dynamic_allocator_create(u64 total_size, u64* memory_requirement, void* memory)
{
    u64 min_size = sizeof(dynamic_allocator_node); // Защита от дурака =)
    if(!total_size || total_size < min_size)
    {
        kerror("Function '%s' require total_size greater than or equal to %llu B.", __FUNCTION__, min_size);
        return null;
    }

    if(!memory_requirement)
    {
        kerror("Function '%s' requires a valid pointer to memory_requiremet to obtain requirements.", __FUNCTION__);
        return null;
    }

    // Это минимум необходимый для работы распределителя памяти.
    if(total_size < sizeof(dynamic_allocator_node ))
    {
        total_size = sizeof(dynamic_allocator_node );
    }

    *memory_requirement = sizeof(dynamic_allocator) + total_size;

    if(!memory)
    {
        return null;
    }

    kzero_tc(memory, dynamic_allocator, 1);
    dynamic_allocator* allocator = memory;

    // Настройка заголовка.
    allocator->memory_block = (void*)((u8*)allocator + sizeof(dynamic_allocator));
    allocator->head = allocator->memory_block;
    allocator->total_size = total_size;
    allocator->current_size = total_size;
    allocator->block_count = 1;

    // Настройка блока свободной памяти.
    allocator->head->size = total_size; // Т.к. при выделении памяти, заголовок уничтожается!
    allocator->head->next = null;

    return allocator;
}

void dynamic_allocator_destroy(dynamic_allocator* allocator)
{
    if(!allocator || !allocator->memory_block)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator.", __FUNCTION__);
        return;
    }

    if(allocator->current_size != allocator->total_size)
    {
        kwarng(
            "Function '%s' called when memory has not yet been freed. The operation will not be aborted!",
            __FUNCTION__
        );
    }

    // Обнуляем что бы сделать его недействительным.
    kzero_tc(allocator, dynamic_allocator, 1);
}

void* dynamic_allocator_allocate(dynamic_allocator* allocator, u64 size)
{
    if(!allocator || !allocator->memory_block)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator.", __FUNCTION__);
        return null;
    }

    if(!size)
    {
        kerror("Function '%s' requires a size greater than zero.", __FUNCTION__);
        return null;
    }

    // Минимально допустимый размер выделения участак памяти, решает 2 проблемы:
    // 1. При освобождении участка такого размера, есть место для записи служебной структуры.
    // 2. При выделении участка памяти, в моменте когда идет разделения куска может возникнуть
    //    опасная ситуация, что служебные структуры могут наложиться и запись повредит обе!
    u64 min_size = sizeof(dynamic_allocator_node);

    // Маленькая хитрость, если все же есть необходимость выделить больше нуля, но меньше необходимого,
    // тогда просто отдадим минимально допустимый кусок памяти. 
    if(size < min_size)
    {
        size = min_size;
    }

    if(size > allocator->current_size)
    {
        goto j_remaining;
    }

    dynamic_allocator_node* node = allocator->head;
    dynamic_allocator_node* prev = null;

    while(node)
    {
        if(node->size == size)
        {
            if(prev)
            {
                prev->next = node->next;
            }
            else
            {
                allocator->head = node->next;
            }

            allocator->block_count--;
            allocator->current_size -= size;
            return node;
        }

        if(node->size > size)
        {
            // Данный способ исключает дробление в первую очередь большого куска памяти
            // на мелкие, если у нас уже есть свободные подходящего размера.
            dynamic_allocator_node* piece = (void*)((u8*)node + size);
            piece->size = node->size - size;
            piece->next = node->next;

            if(prev)
            {
                prev->next = piece;
            }
            else
            {
                allocator->head = piece;
            }

            allocator->current_size -= size;
            return node;
        }

        prev = node;
        node = node->next;
    }

j_remaining:
    // На всякий случай, что бы потом не потерять голову! =)
    if(size == min_size)
    {
        kwarng(
            "Function '%s': If the requested size is less than %llu, it will be equal to it.",
            __FUNCTION__, min_size
        );
    }

    kwarng(
        "Function '%s': No block with enough free space found. Requested: %llu B, remaining: %llu B (blocks %llu).",
        __FUNCTION__, size, allocator->current_size, allocator->block_count
    );
    return null;
}

bool dynamic_allocator_free(dynamic_allocator* allocator, void* block, u64 size)
{
    if(!allocator || !allocator->memory_block || !block)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator and block of memory.", __FUNCTION__);
        return false;
    }

    void* min_area = allocator->memory_block;
    void* max_area = (u8*)min_area + allocator->total_size;
    void* block_offset_right = (u8*)block + size;

    if(block < min_area || block >= max_area || block_offset_right > max_area)
    {
        kerror("Function '%s': Attempting to free memory out of range.", __FUNCTION__);
        return false;
    }

    // Максимально возможный размер памяти который на данный момент можно вернуть.
    u64 max_size = allocator->total_size - allocator->current_size;

    if(!size || size > max_size)
    {
        kerror(
            "Function '%s' requires a size greater than zero and less than or equal to %llu.",
            __FUNCTION__, max_size
        );
        return false;
    }

    u64 min_size = sizeof(dynamic_allocator_node);

    if(size < min_size)
    {
        size = min_size;
    }

    dynamic_allocator_node* node  = allocator->head;
    dynamic_allocator_node* prev  = null;

    if(!node)
    {
        allocator->head = block;
        allocator->head->size = size;
        allocator->head->next = null;
        allocator->block_count++;
        return true;
    }

    // Перелистывание элементов.
    // Левый   : prev = null, node.
    // Средний : prev, node.
    // Правый  : prev, node = null.
    while(node && block > (void*)node)
    {
        prev = node;
        node = node->next;
    }

    void* block_offset_left = prev ? (u8*)prev + prev->size : null;

    // Проверка на коллизию границ.
    if((prev && block_offset_left > block) || (node && block_offset_right > (void*)node))
    {
        kfatal("Function '%s': Memory collision detected on pointer %p. Stopped for debugging!", __FUNCTION__, block);
        return false;
    }

    bool linked = false;

    // Попытка присоединить слева.
    if(prev && block_offset_left == block)
    {
        prev->size += size;
        linked = true;
    }

    dynamic_allocator_node* entry = block;

    // Попытка присоединить справа.
    if(node && block_offset_right == node)
    {
        if(linked)
        {
            prev->size += node->size;
            prev->next = node->next;
            allocator->block_count--;
        }
        else
        {
            entry->size = node->size + size;
            entry->next = node->next;

            if(prev)
            {
                prev->next = entry;
            }
            else
            {
                allocator->head = entry;
            }

            linked = true;
        }
    }

    if(!linked)
    {
        entry->size = size;
        entry->next = node;

        if(prev)
        {
            prev->next = entry;
        }
        else
        {
            allocator->head = entry;
        }

        allocator->block_count++;
    }

    allocator->current_size += size;
    return true;
}

u64 dynamic_allocator_free_space(dynamic_allocator* allocator)
{
    if(!allocator || !allocator->memory_block)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator.", __FUNCTION__);
        return 0;
    }

    return allocator->current_size;
}

u64 dynamic_allocator_block_count(dynamic_allocator* allocator)
{
    if(!allocator || !allocator->memory_block)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator.", __FUNCTION__);
        return 0;
    }

    return allocator->block_count;
}
