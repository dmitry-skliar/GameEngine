// Собственные подключения.
#include "containers/freelist.h"
#include "containers/darray.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

// Узел с контекстом блока свободной памяти.
typedef struct freelist_node {
    // Смещение адреса (относительно нуля).
    ptr offset;
    // Размер блока.
    ptr size;
    // Указатель на следующий узел.
    struct freelist_node* next;
} freelist_node;

// Контекст экземпляра списка блоков свободной памяти.
struct freelist {
    // Размер памяти с которым работает список.
    ptr total_size;
    // Размер свободной памяти в данный момент.
    ptr free_size;
    // Максимальное количество элементов списка.
    ptr node_capacity;
    // Текущее количество элементов списка.
    ptr node_count;
    // Список элементов freelist_node (использует массив darray).
    freelist_node* nodes;
    // Указатель на первый элемент списка.
    freelist_node* head;
    // Флаг указывающий что были обнавлены указатели nodes и head.
    bool nodes_updated_flag;
};

// Объявления функций.
static freelist_node* node_get(freelist* list);             // При использовании проверять флаг nodes_updated_flag!
static void node_free(freelist* list, freelist_node* node);
static void nodelist_resize(freelist* list);

// Проверяет указатель и контекст распределителя памяти.
bool is_freelist_invalid(const freelist* list, const char* function_name)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list.", function_name);
        return true;
    }
    return false;
}

freelist* freelist_create(ptr total_size, ptr* memory_requirement, void* memory)
{
    if(!total_size)
    {
        kerror("Function '%s' require a total_size greater than zero.", __FUNCTION__);
        return null;
    }

    if(!memory)
    {
        if(!memory_requirement)
        {
            kerror("Function '%s' requires a valid pointer to memory_requiremet to obtain requirements.", __FUNCTION__);
            return null;
        }

        *memory_requirement = sizeof(freelist);
        return null;
    }

    kzero(memory, sizeof(freelist));
    freelist* list = memory;

    // Создание списка элементов.
    freelist_node* node = darray_reserve(freelist_node, NODE_START);

    if(!node)
    {
        kerror("Function '%s': Failed to create dynamic array list.", __FUNCTION__);
        return null;
    }

    // Настройка списка элементов.
    kzero_tc(node, freelist_node, NODE_START);

    // Настройка первого элемента.
    node[0].offset = 0;
    node[0].size = total_size;
    node[0].next = null;

    // Настройка экземпляра контекста списка.
    list->nodes = node;
    list->head = node;
    list->nodes_updated_flag = false;
    list->node_capacity = NODE_START; //darray_capacity(list->nodes);
    list->node_count = 1;
    list->total_size = total_size;
    list->free_size = total_size;
    return list;
}

void freelist_destroy(freelist* list)
{
    // Проверка, что распределитель памяти действующий.
    if(is_freelist_invalid(list, __FUNCTION__))
    {
        return;
    }

    darray_destroy(list->nodes);
    kzero_tc(list, freelist, 1);
}

bool freelist_allocate_block(freelist* list, ptr size, ptr* out_offset)
{
    u16 alignment_size = 0;
    return freelist_allocate_block_aligned(list, size, 1, out_offset, &alignment_size);
}

bool freelist_allocate_block_aligned(freelist* list, ptr size, u16 alignment, ptr* out_aligned_offset, u16* out_alignment_size)
{
    // Проверка, что распределитель памяти действующий.
    if(!list || !list->nodes || !out_aligned_offset || !out_alignment_size)
    {
        kerror("Function '%s' requires valid pointers to list, out_aligned_offset and out_alignment_size.", __FUNCTION__);
        return false;
    }

    // Проверка требований к запрашиваемому размеру памяти и выравниванию.
    if(!size || !alignment)
    {
        kerror("Function '%s' requires size and alignment greater than zero.", __FUNCTION__);
        return false;
    }

    // Проверка, соответствует ли выравнивание степени двойки.
    if(alignment & (alignment - 1))
    {
        kerror("Function '%s' requires alignment must be a power of two.", __FUNCTION__);
        return false;
    }

    if(list->free_size >= size)
    {
        // Указатели на предыдущий и текуший свободные блоки памяти.
        freelist_node* node = list->head;
        freelist_node* prev = null;

        // Процесс поиска подходящего блока памяти.
        while(node)
        {
            // Получение выравненного смещения памяти.
            ptr aligned_offset = get_aligned(node->offset, alignment);
            // Размер выравнивания в байтах. 
            ptr alignment_size = aligned_offset - node->offset;
            // Требуемый размер памяти относительно невыровненой границы памяти.
            ptr unaligned_size = size + alignment_size;

            // TODO: Заменить проверкой флага переполнения процессора (см. __builtin_add_overflow).
            // Проверка, что unaligned_size не был переполнен.
            if(size > PTR_MAX - alignment_size)
            {
                kerror("Function '%s': Unable to align, requested size is too large.", __FUNCTION__);
                return false;
            }

            // Проверка текущего блока памяти на подходящий требованиям размер.
            if(node->size >= unaligned_size)
            {
                // Вычисление остатка памяти для разделения на блоки (нужного размера + остаток).
                ptr remaining = node->size - unaligned_size;

                // Проверка на возможность разделить блок памяти.
                if(remaining)
                {
                    // Просто уменьшить размер и сместить границу свободного участка правее.
                    node->size -= unaligned_size;
                    node->offset += unaligned_size;
                }
                // Разделение блока невозможно, т.к. точно соответствует требованиям.
                else
                {
                    // Удаление блока из списка.
                    if(prev)
                    {
                        prev->next = node->next;
                    }
                    else
                    {
                        list->head = node->next;
                    }

                    node_free(list, node);
                }

                *out_aligned_offset = aligned_offset;
                *out_alignment_size = alignment_size;
                list->free_size -= unaligned_size;
                return true;
            }

            // Т.к. блоки меньшего размера, то поиск продолжается.
            prev = node;
            node = node->next;
        }
    }

    kwarng(
        "Function '%s': No block with enough free space found. Requested: %llu B (alignment %llu), remaining: %llu B (nodes %llu).",
        __FUNCTION__, size, alignment, list->free_size, list->node_count
    );
    return false;
}

bool freelist_free_block(freelist* list, ptr size, ptr offset)
{
    return freelist_free_block_aligned(list, size, offset, 0);
}

bool freelist_free_block_aligned(freelist* list, ptr size, ptr aligned_offset, u16 alignment_size)
{
    if(!list || !list->nodes || !size)
    {
        kerror("Function '%s' requires a valid pointer to list and size greater than zero.", __FUNCTION__);
        return false;
    }

    if(aligned_offset < alignment_size)
    {
        kerror("Function '%s': aligned_offset (%llu) must be >= alignment_size (%u)!", __FUNCTION__, aligned_offset, alignment_size);
        return false;
    }

    ptr unaligned_offset = aligned_offset - alignment_size;
    ptr unaligned_size = size + alignment_size;

    // Смещение для участка памяти правее.
    // NOTE: Если подставить unaligned_offset + unaligned_size, то будет тоже самое!
    ptr offset_right = aligned_offset + size;

    // NOTE: Список имеет адреса от 0 до total_size! Т.к. в данном случае проверяется правая граница памяти,
    //       то имеет смысл проверить aligned_offset вместо unaligned_offset.
    if(aligned_offset >= list->total_size || offset_right > list->total_size)
    {
        kfatal("Function '%s': Attempting to free space out of range. Stopped for debugging!", __FUNCTION__);
        return false;
    }

    freelist_node* node = list->head;
    freelist_node* prev = null;

    // Список пуст (память израсходована)!
    if(!node)
    {
        // NOTE: Вызов нижу не будет вызывать изменение размера списка, т.к. все элементы списка свободны!
        node = node_get(list);
        node->size = unaligned_size;
        node->offset = unaligned_offset;

        list->free_size += unaligned_size;
        list->head = node;
        return true;
    }

    // Перелистывание элементов.
    // Левый   : prev = null, node.
    // Средний : prev, node.
    // Правый  : prev, node = null.
    while(node && unaligned_offset > node->offset)
    {
        prev = node;
        node = node->next;
    }

    // Смещение для участка памяти левее.
    ptr offset_left = prev ? prev->offset + prev->size : 0;

    // NOTE: Условие offset_right > node->offset проверяет не залаит ли наш участок памяти на тот, который правее от него.
    if((prev && offset_left > unaligned_offset) || (node && offset_right > node->offset))
    {
        kerror("Function '%s': Attemping to free aleady-freed block of memory at offset %llu.", __FUNCTION__, unaligned_offset);
        return false;
    }

    // Указывает было ли присоединение блоков памяти.
    bool linked_flag = false;

    // Просоединение к левому участку.
    if(prev && offset_left == unaligned_offset)
    {
        prev->size += unaligned_size;
        linked_flag = true;
    }

    // Присоединение к правому участку.
    if(node && offset_right == node->offset)
    {
        // Соединение левого и правого элементов списка.
        if(linked_flag)
        {
            prev->size += node->size;
            prev->next = node->next;
            node_free(list, node);
        }
        // Присоединение только к правому участку.
        else
        {
            node->offset = unaligned_offset;
            node->size += unaligned_size;
            linked_flag = true;
        }
    }

    // Новый элемент, т.к. невозможно выполнить присоединения к левому и/или правому участку(ам).
    if(!linked_flag)
    {
        freelist_node* new_node = node_get(list);

        // NOTE: Дело в том, что после вызова node_get(...) может произойти создание нового списка и удаление старого,
        //       а потому старые указатели node/prev становятся недействительными. Следовательно их нужно обновить.
        if(list->nodes_updated_flag)
        {
            node = list->head;
            prev = null;

            while(node && unaligned_offset > node->offset)
            {
                prev = node;
                node = node->next;
            }

            list->nodes_updated_flag = false;
        }

        new_node->offset = unaligned_offset;
        new_node->size = unaligned_size;
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

    list->free_size += unaligned_size;

    return true;
}

bool freelist_resize(freelist* list, ptr new_size)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list.", __FUNCTION__);
        return false;
    }

    if(new_size <= list->total_size)
    {
        kerror(
            "Function '%s' requires a new size that is larger than the old size (new: %llu B, old: %llu B).",
            __FUNCTION__, new_size, list->total_size
        );
        return false;
    }

    ptr new_offset = list->total_size;
    list->total_size = new_size;
    new_size -= new_offset;

    return freelist_free_block(list, new_size, new_offset);
}

void freelist_clear(freelist* list)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list.", __FUNCTION__);
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
    list->free_size = list->total_size;
}

ptr freelist_free_space(freelist* list)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list.", __FUNCTION__);
        return 0;
    }

    return list->free_size;
}

ptr freelist_block_count(freelist* list)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list.", __FUNCTION__);
        return 0;
    }

    return list->node_count;
}

ptr freelist_block_capacity(freelist* list)
{
    if(!list || !list->nodes)
    {
        kerror("Function '%s' requires a valid pointer to list.", __FUNCTION__);
        return 0;
    }

    return list->node_capacity;
}

static freelist_node* node_get(freelist* list)
{
    // TODO: Что бы избавиться от флага, обновление можно вынести в отдельную функцию.
    // Расширение списка.
    if(list->node_count == list->node_capacity)
    {
        nodelist_resize(list);
    }

    // Получение нового элемента.
    ptr i = list->nodes_updated_flag ? list->node_count : 0;
    for(; i < list->node_capacity; ++i)
    {
        if(list->nodes[i].size == 0)
        {
            list->node_count++;
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
    list->node_count--;
}

static void nodelist_resize(freelist* list)
{
    freelist_node* old_nodes = list->nodes; // Для удаления старого списка.
    freelist_node* node = list->head;       // Для получения упорядоченого старого списка.
    freelist_node* prev = null;             // Для воссоздания связей новго списка.

    ptr node_index = 0;
    ptr node_capacity_new = list->node_capacity * NODE_RESIZE_FACTOR;

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
