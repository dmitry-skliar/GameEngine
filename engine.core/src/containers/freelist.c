// Собственные подключения.
#include "containers/freelist.h"
#include "containers/darray.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

typedef struct freelist_node {
    u64 offset;
    u64 size;
    struct freelist_node* next;
} freelist_node;

struct freelist {
    // Размер памяти с которым работает список.
    u64 total_size;
    // Размер свободной памяти в данный момент.
    u64 current_size;
    // Максимальное количество элементов списка.
    u64 node_capacity;
    // Текущее количество элементов списка.
    u64 node_count;
    // Список элементов freelist_node (использует массив darray).
    freelist_node* nodes;
    // Указатель на первый элемент списка.
    freelist_node* head;
    // Флаг указывающий что были обнавлены указатели nodes и head.
    bool nodes_updated_flag;
};

static freelist_node* node_get(freelist* list); // При использовании проверять флаг nodes_updated_flag!
static void node_free(freelist* list, freelist_node* node);
static void nodelist_resize(freelist* list);

freelist* freelist_create(u64 total_size, u64* memory_requirement, void* memory)
{
    if(!memory_requirement)
    {
        kerror("Function '%s' requires a valid pointer to memory_requiremet to obtain requirements.", __FUNCTION__);
        return null;
    }

    *memory_requirement = sizeof(freelist);

    if(!memory)
    {
        return null;
    }

    kzero(memory, *memory_requirement);
    freelist* list = memory;

    // Создание списка элементов.
    freelist_node* node = darray_reserve(freelist_node, NODE_START);

    if(!node)
    {
        kerror("Function '%s': Failed to create dynamic array list. Return null!", __FUNCTION__);
        return null;
    }

    // Настройка списка элементов.
    kzero_tc(node, freelist_node, NODE_START);
    node->offset = 0;
    node->size = total_size;
    node->next = null;

    // Настройка заголовка.
    list->nodes = node;
    list->head = node;
    list->nodes_updated_flag = false;
    list->node_capacity = NODE_START; //darray_capacity(list->nodes);
    list->node_count = 1;
    list->total_size = total_size;
    list->current_size = total_size;

    return list;
}

void freelist_destroy(freelist* list)
{
    if(!list)
    {
        kerror("Function '%s' requires a valid pointer to list.", __FUNCTION__);
    }

    darray_destroy(list->nodes);
    kzero_tc(list, freelist, 1);
}

bool freelist_allocate_block(freelist* list, u64 size, u64* out_offset)
{
    if(!list || !list->nodes || !out_offset)
    {
        kerror("Function '%s' requires valid pointers to list and out_offset.", __FUNCTION__);
        return false;
    }

    if(!size)
    {
        kerror("Function '%s' requires a size greater than zero.", __FUNCTION__);
        return false;
    }

    if(list->current_size < size)
    {
        goto j_remaining;
    }

    freelist_node* node = list->head;
    freelist_node* prev = null;

    while(node)
    {
        // Ести блок нужного размера (предпочитаемый).
        if(node->size == size)
        {
            *out_offset = node->offset;
            list->current_size -= size;

            if(prev)
            {
                prev->next = node->next;
            }
            else
            {
                list->head = node->next;
            }

            node_free(list, node);
            return true;
        }
        // Если блок большего размера, то разделить его.
        else if(node->size > size)
        {
            *out_offset = node->offset;
            list->current_size -= size;

            node->size -= size;
            node->offset += size;
            return true;
        }

        prev = node;
        node = node->next;
    }

j_remaining:
    kwarng(
        "Function '%s': No block with enough free space found. Requested: %llu B, remaining: %llu B (nodes %llu).",
        __FUNCTION__, size, list->current_size, list->node_count
    );
    return false;
}

bool freelist_free_block(freelist* list, u64 size, u64 offset)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list.", __FUNCTION__);
        return false;
    }

    if(!size || size > list->total_size)
    {
        kerror("Function '%s' requires a size greater than zero and less than or equal to total_size.", __FUNCTION__);
        return false;
    }

    // Смещение для участка памяти правее.
    u64 offset_right = offset + size;

    if(offset >= list->total_size || offset_right > list->total_size)
    {
        kfatal("Function '%s': Attempting to free space out of range. Stopped for debugging!", __FUNCTION__);
        return false;
    }

    freelist_node* node = list->head;
    freelist_node* prev = null;

    // Список пуст (память израсходована)!
    if(!node)
    {
        node = node_get(list);
        node->size = size;
        node->offset = offset;
        // node->next = null; - нет необходимости список и так содержит нули!

        list->current_size += size;
        list->head = node;
        return true;
    }

    // Перелистывание элементов.
    // NOTE: Левый   : prev = null, node.
    //       Средний : prev, node.
    //       Правый  : prev, node = null.
    while(node && offset > node->offset)
    {
        prev = node;
        node = node->next;
    }

    // Смещение для участка памяти левее.
    u64 offset_left = prev ? prev->offset + prev->size : 0;

    if((prev && offset_left > offset) || (node && offset_right > node->offset))
    {
        kerror("Function '%s': Attemping to free aleady-freed block of memory at offset %llu.", __FUNCTION__, offset);
        return false;
    }

    bool linked_flag = false;

    // Левое присоединение.
    if(prev && offset_left == offset)
    {
        prev->size += size;
        linked_flag = true;
    }

    // Правое присоединение.
    if(node && offset_right == node->offset)
    {
        // Соединение левого и правого элементов списка.
        if(linked_flag)
        {
            prev->size += node->size;
            prev->next = node->next;
            node_free(list, node);
        }
        // Правое присоединение только.
        else
        {
            node->offset = offset;
            node->size += size;
            linked_flag = true;
        }
    }

    // Новый элемент.
    if(!linked_flag)
    {
        freelist_node* new_node = node_get(list);

        // FIX: Обновление node и prev.
        if(list->nodes_updated_flag)
        {
            node = list->head;
            prev = null;

            while(node && offset > node->offset)
            {
                prev = node;
                node = node->next;
            }

            list->nodes_updated_flag = false;
        }

        new_node->offset = offset;
        new_node->size = size;
        new_node->next = node;

        if(prev)
        {
            prev->next = new_node;
        }
        else
        {
            list->head = new_node;
        }
    }

    list->current_size += size;

    return true;
}

bool freelist_resize(freelist* list, u64 new_size)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list. Return false!", __FUNCTION__);
        return false;
    }

    if(new_size <= list->total_size)
    {
        kerror(
            "Function '%s' requires a new size that is larger than the old size (new: %llu, old: %llu).",
            __FUNCTION__, new_size, list->total_size
        );
        return false;
    }

    u64 new_offset = list->total_size;
    list->total_size = new_size;
    new_size -= new_offset;

    return freelist_free_block(list, new_size, new_offset);
}

void freelist_clear(freelist* list)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list. Just return!", __FUNCTION__);
        return;
    }

    // Настройка списка элементов.
    kzero_tc(list->nodes, freelist_node, list->node_capacity);
    list->nodes->offset = 0;
    list->nodes->size = list->total_size;
    // list->nodes->next = null; - kzeor_tc....

    // Настройка заголовка.
    list->head = list->nodes;
    list->node_count = 1;
    list->current_size = list->total_size;
}

u64 freelist_free_space(freelist* list)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list. Return 0!", __FUNCTION__);
        return 0;
    }

    return list->current_size;
}

u64 freelist_block_count(freelist* list)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list. Return 0!", __FUNCTION__);
        return 0;
    }

    return list->node_count;
}

u64 freelist_block_capacity(freelist* list)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list. Return 0!", __FUNCTION__);
        return 0;
    }

    return list->node_capacity;
}

static freelist_node* node_get(freelist* list)
{
    // Расширение списка.
    if(list->node_count == list->node_capacity)
    {
        nodelist_resize(list);
    }

    // Получение нового элемента.
    u64 i = list->nodes_updated_flag ? list->node_count : 0;
    for(; i < list->node_capacity; ++i)
    {
        if(list->nodes[i].size == 0)
        {
            list->node_count += 1;
            return &list->nodes[i];
        }
    }

    kfatal("Function '%s': Failed to get free block. Corruption? Stopped for debugging!", __FUNCTION__);
    return null;
}

static void node_free(freelist* list, freelist_node* node)
{
    node->offset = 0;
    node->size = 0;
    node->next = null;
    list->node_count -= 1;
}

static void nodelist_resize(freelist* list)
{
    freelist_node* old_nodes = list->nodes; // Для удаления старого списка.
    freelist_node* node = list->head;       // Для получения упорядоченого старого списка.
    freelist_node* prev = null;             // Для воссоздания связей новго списка.

    u64 node_index = 0;
    u64 node_capacity_new = list->node_capacity * NODE_RESIZE_FACTOR;

    list->nodes = darray_reserve(freelist_node, node_capacity_new);
    kzero_tc(list->nodes, freelist_node, node_capacity_new);

    while(node)
    {
        list->nodes[node_index] = *node;    // Копирование данных.

        if(prev)
        {
            prev->next = &list->nodes[node_index];
        }

        prev = &list->nodes[node_index];
        node = node->next;
        ++node_index;
    }

    prev->next = null;            // Нужно обнулять указатель только у последнего элемента.
    list->head = list->nodes;     // Восстановление первого элемента списка.
    list->node_capacity = node_capacity_new;
    list->nodes_updated_flag = true;
    darray_destroy(old_nodes);    // Удаление старого массива.
}
