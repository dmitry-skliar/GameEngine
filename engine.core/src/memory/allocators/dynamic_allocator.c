// Собственные подключения.
#include "memory/allocators/dynamic_allocator.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

typedef struct dynamic_allocator_node {
    // Служебная информация о размере памяти.
    u64 size;
    // Слежебная информация о следующем блоке или часть выделенной памяти.
    struct dynamic_allocator_node * next;
} dynamic_allocator_node;

struct dynamic_allocator {
    // Максимальный размера памяти.
    u64 total_size;
    // Доступный размер памяти в данный момент.
    u64 free_size;
    // Указатель блок памяти.
    void* memory;
    // Количество свободных блоков памяти.
    u64 block_count;
    // Указатель на первый свободный блок памяти.
    dynamic_allocator_node* blocks;
};

dynamic_allocator* dynamic_allocator_create(u64 total_size, u64* memory_requirement, void* memory)
{
    if(!total_size || total_size < sizeof(dynamic_allocator_node))
    {
        kerror(
            "Function '%s' require total_size greater than or equal to %llu B.",
            __FUNCTION__, sizeof(dynamic_allocator_node)
        );
        return null;
    }

    if(!memory_requirement)
    {
        kerror("Function '%s' requires a valid pointer to memory_requiremet to obtain requirements.", __FUNCTION__);
        return null;
    }

    *memory_requirement = sizeof(dynamic_allocator) + sizeof(u64) + total_size;

    if(!memory)
    {
        return null;
    }

    kzero_tc(memory, dynamic_allocator, 1);
    dynamic_allocator* allocator = memory;

    // Настройка заголовка.
    allocator->memory = OFFSET_PTR(allocator, sizeof(dynamic_allocator));
    allocator->blocks = allocator->memory;
    allocator->total_size = total_size;
    allocator->free_size = total_size;
    allocator->block_count = 1;

    // Настройка блока свободной памяти.
    allocator->blocks->size = total_size;
    allocator->blocks->next = null;

    return allocator;
}

void dynamic_allocator_destroy(dynamic_allocator* allocator)
{
    if(!allocator || !allocator->memory)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator.", __FUNCTION__);
        return;
    }

    // TODO: Вынесити в главный распределитель!
    if(allocator->free_size != allocator->total_size)
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
    if(!allocator || !allocator->memory)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator.", __FUNCTION__);
        return null;
    }

    // Минимально допустимый размер памяти.
    if(size < sizeof(u64))
    {
        kwarng("Function '%s': Requested size is less than %llu B, it will be equal to it.", __FUNCTION__, sizeof(u64));
        size = sizeof(u64);
    }

    if(size > allocator->free_size)
    {
        goto j_remaining;
    }

    dynamic_allocator_node* node = allocator->blocks;
    dynamic_allocator_node* prev = null;

    while(node)
    {
        bool greater_than = node->size > size;

        // Упреждающая проверка:
        // - Возможность хранить служемную информацию при разделении блока.
        // - Избавляет от вложенного вызова выделения памяти.
        if(greater_than && (node->size - size) < sizeof(dynamic_allocator_node))
        {
            size = node->size;
        }

        // Получение существующего блока.
        if(node->size == size)
        {
            if(prev)
            {
                prev->next = node->next;
            }
            else
            {
                allocator->blocks = node->next;
            }

            allocator->block_count--;
            allocator->free_size -= size;
            return OFFSET_PTR(node, sizeof(u64));
        }

        // Резделение блока (см. упреждаюую проверку).
        if(greater_than)
        {
            // Резервирование места под хранимую служебную информацию (размер блока только).
            u64 offset = size + sizeof(u64);

            // Данный способ исключает дробление большого куска памяти на мелкие в первую очередь.
            dynamic_allocator_node* piece = (void*)((u8*)node + offset);
            piece->size = node->size - offset;
            piece->next = node->next;

            if(prev)
            {
                prev->next = piece;
            }
            else
            {
                allocator->blocks = piece;
            }

            node->size = size;
            allocator->free_size -= offset;
            return OFFSET_PTR(node, sizeof(u64));
        }

        prev = node;
        node = node->next;
    }

j_remaining:
    kwarng(
        "Function '%s': No block with enough free space found. Requested %llu B, remaining %llu B (blocks %llu).",
        __FUNCTION__, size, allocator->free_size, allocator->block_count
    );
    return null;
}

bool dynamic_allocator_free(dynamic_allocator* allocator, void* block)
{
    if(!allocator || !allocator->memory)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator.", __FUNCTION__);
        return false;
    }

    if(!block)
    {
        kerror("Function '%s' requires a valid pointer to block of memory.", __FUNCTION__);
        return false;
    }

    void* block_offset_right = block;
    block = OFFSET_PTR(block, -sizeof(u64));

    // Получение доступа к служебной информации.
    u64 block_size = MEMBER_GET(dynamic_allocator_node, block, size);
    block_offset_right = OFFSET_PTR(block_offset_right, block_size);

    void* min_area = allocator->memory;
    void* max_area = OFFSET_PTR(min_area, sizeof(u64) + allocator->total_size);

    if(block < min_area || block_offset_right > max_area)
    {
        kerror("Function '%s': Attempting to free memory out of range.", __FUNCTION__);
        return false;
    }

    dynamic_allocator_node* node  = allocator->blocks;
    dynamic_allocator_node* prev  = null;

    if(!node)
    {
        allocator->blocks = block;
        allocator->blocks->size = block_size;
        allocator->blocks->next = null;
        allocator->free_size += block_size;
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

    void* block_offset_left = prev ? OFFSET_PTR(prev, sizeof(u64) + prev->size) : null;

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
        prev->size += block_size + sizeof(u64);
        allocator->free_size += sizeof(u64);
        linked = true;
    }

    dynamic_allocator_node* entry = block;

    // Попытка присоединить справа.
    if(node && block_offset_right == node)
    {
        u64 node_size = node->size + sizeof(u64);
        
        if(linked)
        {
            prev->size += node_size;
            prev->next = node->next;
            allocator->block_count--;
        }
        else
        {
            entry->size += node_size;
            entry->next = node->next;

            if(prev)
            {
                prev->next = entry;
            }
            else
            {
                allocator->blocks = entry;
            }

            linked = true;
        }

        allocator->free_size += sizeof(u64);
    }

    if(!linked)
    {
        entry->next = node;

        if(prev)
        {
            prev->next = entry;
        }
        else
        {
            allocator->blocks = entry;
        }

        allocator->block_count++;
    }

    allocator->free_size += block_size;
    return true;
}

u64 dynamic_allocator_free_space(dynamic_allocator* allocator)
{
    if(!allocator || !allocator->memory)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator.", __FUNCTION__);
        return 0;
    }

    return allocator->free_size;
}

u64 dynamic_allocator_free_blocks(dynamic_allocator* allocator)
{
    if(!allocator || !allocator->memory)
    {
        kerror("Function '%s' requires a valid pointer to dynamic allocator.", __FUNCTION__);
        return 0;
    }

    return allocator->block_count;
}
