// Собственные подключения.
#include "memory/allocators/dynamic_allocator.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

// Служебная информация выделенного блока памяти, храниться на границе выравнивания cлева (т.е. -sizeof(allocated_block)).
typedef struct allocated_block {
    // Магическое число для провеки.
    ptr magic;
    // Размер памяти относительно невыровненной границы, используется для получения размера блока относительно выровненной границы.
    ptr unaligned_size;
    // Размер выравнивания указателя в байтах, используется для получения резмера блока и адреса невыровненной границы.
    u16 alignment_size;
    // Кратность выравнивания.
    u16 alignment;
} allocated_block;

// Cлужебная информация свободного блока памяти.
typedef struct freed_block {
    // Магическое число для провеки.
    ptr magic;
    // Размер свободного блока памяти.
    ptr size;
    // Указатель на следующий блок свободной памяти.
    struct freed_block* next;
} freed_block;

// Контекст экземпляра динамического распределителя памяти.
struct dynamic_allocator {
    // Максимальный размера памяти.
    ptr total_size;
    // Доступный размер памяти в данный момент.
    ptr free_size;
    // Указатель на пул памяти.
    void* memory_pool;
    // Количество свободных блоков памяти.
    ptr block_count;
    // Пул свободной памяти (указатель на первый блок).
    freed_block* blocks;
};

// Обязательная проверка, что бы при выравнивании блока памяти не произошло наложение заголовка с другим блоком памяти.
// NOTE: Этот случай может возникнуть в том случае, когда размер выравнивания равен 0.
STATIC_ASSERT(sizeof(allocated_block) == sizeof(freed_block), "The allocated_block size must be equal to the freed_block size.");

// Минимально возможный размер блока памяти (заголовок + минимальный размер информации).
#define MIN_BLOCK_SIZE (sizeof(freed_block) + sizeof(u8))

// Магические значения, для выполнения проверок.
#define ALLOCATED_BLOCK_MAGIC 0x4c4c4f43
#define FREED_BLOCK_MAGIC     0x52454544

// Проверяет указатель и контекст распределителя памяти.
bool is_dynamic_allocator_invalid(const dynamic_allocator* allocator, const char* func)
{
    if(!allocator || !allocator->memory_pool)
    {
        if(func)
        {
            kerror("Function '%s' requires a valid pointer to dynamic allocator.", func);
        }
        return true;
    }
    return false;
}

dynamic_allocator* dynamic_allocator_create(ptr total_size, ptr* memory_requirement, void* memory)
{
    // Самый минимум который необходим для работы распределителя памяти.
    ptr min_total_size = sizeof(dynamic_allocator) + MIN_BLOCK_SIZE;

    // Принудительно изменяет размер, если он меньше запрашиваемого. Мера предосторожности!
    if(total_size < min_total_size)
    {
        ktrace(
            "Function '%s': Requested total size is less than the minimum (%llu B), it will be equal to it.",
            __FUNCTION__, min_total_size
        );
        ktrace(
            "Function '%s': The minimum total size includes memory allocator context (%llu B) and block header (%llu B).",
            __FUNCTION__, sizeof(dynamic_allocator), sizeof(freed_block)
        );
        total_size = min_total_size;
    }

    // Получение требований к памяти.
    if(!memory)
    {
        if(!memory_requirement)
        {
            kerror("Function '%s' requires a valid pointer to memory_requiremet to obtain requirements.", __FUNCTION__);
            return null;
        }

        *memory_requirement = total_size;
        return null;
    }

    // Получение реального размера первого блока памяти (учитывается размер контекста распределителя + служебной информации).
    total_size -= sizeof(dynamic_allocator) + sizeof(freed_block);

    // Настройка контекста распределителя памяти.
    // NOTE: Т.к. каждое значение структуры заполняется, то вызывать обнуление ее не требуется.
    dynamic_allocator* allocator = memory;
    allocator->memory_pool = POINTER_GET_OFFSET(allocator, sizeof(dynamic_allocator));
    allocator->blocks = allocator->memory_pool;
    allocator->total_size = total_size;
    allocator->free_size = total_size;
    allocator->block_count = 1;

    // Настройка первого блока свободной памяти.
    allocator->blocks[0].size = total_size;
    allocator->blocks[0].next = null;
    allocator->blocks[0].magic = FREED_BLOCK_MAGIC;

    return allocator;
}

void dynamic_allocator_destroy(dynamic_allocator* allocator)
{
    // Проверка, что распределитель памяти действующий.
    if(is_dynamic_allocator_invalid(allocator, __FUNCTION__))
    {
        return;
    }

    // Проверка на утечку памяти.
    if(allocator->free_size != allocator->total_size)
    {
        kwarng("Function '%s' called when memory has not yet been freed. The operation will not be aborted!", __FUNCTION__);
        ktrace("Free size is %llu B, and total size is %llu B.", allocator->free_size, allocator->total_size);
    }

    // Обнуление контекста распределителя памяти, что бы сделать его недействительным.
    kzero_tc(allocator, dynamic_allocator, 1);
}

void* dynamic_allocator_allocate(dynamic_allocator* allocator, ptr size, u16 alignment)
{
    // Проверка, что распределитель памяти действующий.
    if(is_dynamic_allocator_invalid(allocator, __FUNCTION__))
    {
        return null;
    }

    // Проверка требований к запрашиваемому размеру памяти и кратности выравнивания.
    if(!size || !alignment)
    {
        kerror("Function '%s' requires size and alignment greater than zero.", __FUNCTION__);
        return null;
    }

    // Проверка, соответствует ли кратность выравнивания степени двойки.
    if(alignment & (alignment - 1))
    {
        kerror("Function '%s' requires alignment must be a power of two.", __FUNCTION__);
        return null;
    }

    if(allocator->free_size >= size)
    {
        // Указатели на предыдущий и текуший свободные блоки памяти.
        freed_block* node = allocator->blocks;
        freed_block* prev = null;

        // Процесс поиска подходящего блока памяти.
        while(node)
        {
            // Получение невыровненого адреса памяти (расположен сразу после заголовка блока памяти). 
            ptr unaligned_offset = (ptr)POINTER_GET_OFFSET(node, sizeof(freed_block));
            // Получение выровненого адреса памяти (адрес памяти который необходимо вернуть, если размер памяти подходит).
            ptr aligned_offset = get_aligned(unaligned_offset, alignment);
            // Размер выравнивания в байтах (cохранить в заголовок выделеного блока!).
            u16 alignment_size = aligned_offset - unaligned_offset;
            // Требуемый размер памяти относительно невыровненой границы памяти (cохранить в заголовок выделеного блока!).
            ptr unaligned_size = size + alignment_size;

            // TODO: Заменить проверкой флага переполнения процессора (см. __builtin_add_overflow).
            // Проверка, что unaligned_size не был переполнен.
            if(size > PTR_MAX - alignment_size)
            {
                kerror("Function '%s': Unable to align, requested size is too large.", __FUNCTION__);
                return null;
            }

            // Проверка текущего блока памяти на подходящий требованиям размер.
            if(node->size >= unaligned_size)
            {
                // Вспомогательный указатель на свободный блок памяти.
                freed_block* right_block;
                // Вычисление остатка памяти для разделения на блоки (нужного размера + остаток).
                ptr remaining = node->size - unaligned_size;

                // Проверка на соответствие минимальным требованиям свободного блока памяти для разделения.
                if(remaining >= MIN_BLOCK_SIZE)
                {
                    // NOTE: Для того, что бы большой блок памяти разделялся в последнюю очередь, нужно
                    // при разделении отдавать левую часть памяти (с наименьшим адресом), а свободную
                    // оставлять в правой части (с наибольшим адресом). Это должно снизить дробление
                    // памяти на мелкие блоки про определенных условиях.
                    right_block = POINTER_GET_OFFSET(unaligned_offset, unaligned_size);
                    right_block->size = remaining - sizeof(freed_block);
                    right_block->next = node->next;
                    right_block->magic = FREED_BLOCK_MAGIC;
                    // После разделения новому блоку памяти требуется заголовок со служебной информацией.
                    // NOTE: Требуемый размер памяти отнимается позже (оптимизация).
                    allocator->free_size -= sizeof(freed_block);
                }
                // Разделение невозможно, или размер блока точно соответствует требованиям.
                else
                {
                    // Для удаления текущего блока памяти из пула свободной памяти.
                    right_block = node->next;
                    // Обновление размера т.к. разделение текущего блока невозможно, будет отдан весь блок.
                    unaligned_size = node->size;
                    // Т.к. блок удаляется целеком без разделения.
                    allocator->block_count--;
                }

                // Удаление адреса выделяемого блока из пула свободной памяти.
                if(prev)
                {
                    prev->next = right_block;
                }
                else
                {
                    allocator->blocks = right_block;
                }

                // Уменьшение размера общей памяти на требуемый размер.
                allocator->free_size -= unaligned_size;

                // Указатель на заголовок выделенного блока памяти, для сохранения информации о нем.
                allocated_block* block = POINTER_GET_OFFSET(aligned_offset, -sizeof(allocated_block));
                block->unaligned_size = unaligned_size;
                block->alignment_size = alignment_size;
                block->alignment = alignment;
                block->magic = ALLOCATED_BLOCK_MAGIC;
                return (void*)aligned_offset;
            }

            prev = node;
            node = node->next;
        }
    }

    kwarng(
        "Function '%s': No block with enough free space found. Requested %llu B (alignment %u), remaining %llu B (in blocks %llu).",
        __FUNCTION__, size, alignment, allocator->free_size, allocator->block_count
    );
    return null;
}

bool dynamic_allocator_free(dynamic_allocator* allocator, void* block)
{
    // Проверка, что распределитель памяти действующий.
    if(is_dynamic_allocator_invalid(allocator, __FUNCTION__))
    {
        return false;
    }

    if(!block)
    {
        kerror("Function '%s' requires a valid pointer to block of memory.", __FUNCTION__);
        return false;
    }

    // Получение указателя на служебную информацию блока, который необходимо вернуть в пул свободной памяти.
    block = POINTER_GET_OFFSET(block, -sizeof(allocated_block));

    // Проверка магического значения, для исключения повреждения или двойного освобождения памяти.
    ptr* block_magic_ptr = &MEMBER_GET_VALUE(allocated_block, block, magic);
    if(*block_magic_ptr != ALLOCATED_BLOCK_MAGIC)
    {
        kerror("Function '%s': Invalid block magic (possible double free or corruption).", __FUNCTION__);
        return false;
    }

    // Пометка как освобожденного блока.
    *block_magic_ptr = FREED_BLOCK_MAGIC;

    // Сохранение переменных заголовка, для дальнейшего использования.
    ptr block_unaligned_size = MEMBER_GET_VALUE(allocated_block, block, unaligned_size);
    u16 block_alignment_size = MEMBER_GET_VALUE(allocated_block, block, alignment_size);

    // Получение указателя на начало блока (будущий заголовок свободного блока).
    block = POINTER_GET_OFFSET(block, -block_alignment_size);

    // Получение указателя на начало следующего блока, для проверки пересечений границ.
    void* block_right = POINTER_GET_OFFSET(block, sizeof(freed_block) + block_unaligned_size);

    // Получение границ пула памяти.
    void* min_area = allocator->memory_pool;
    void* max_area = POINTER_GET_OFFSET(min_area, sizeof(freed_block) + allocator->total_size);

    // Проверка выхода за границы пула памяти. 
    if(block < min_area || block_right > max_area)
    { 
        kerror("Function '%s': Attempting to free memory out of range (%p..%p).", __FUNCTION__, min_area, POINTER_GET_OFFSET(max_area, -1));
        return false;
    }

    freed_block* node  = allocator->blocks;
    freed_block* prev  = null;

    if(!node)
    {
        allocator->blocks = block;
        allocator->blocks->size = block_unaligned_size;
        allocator->blocks->next = null;
        allocator->blocks->magic = FREED_BLOCK_MAGIC;
        allocator->free_size += block_unaligned_size;
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

    // Вычисление адреса следующего элемента относительно предыдущего.
    void* block_left = prev ? POINTER_GET_OFFSET(prev, sizeof(freed_block) + prev->size) : null;

    // Проверка на коллизию границ относительно левого и/или правого блоков.
    if((prev && block_left > block) || (node && block_right > (void*)node))
    {
        kfatal("Function '%s': Memory collision detected (pointer to memory block header %p). Stopped for debugging!", __FUNCTION__, block);
        return false;
    }

    // Указывает о присоединение освобождаемого блока к левому или правому.
    bool linked = false;

    // Попытка присоединить к левому блоку.
    if(prev && block_left == block)
    {
        // Просто увеличиваем размер левого блока на величину освобождаемого блока
        // + исчезает необходимость хранить служебную информацию освобождаемого блока.
        prev->size += block_unaligned_size + sizeof(freed_block);
        // Увеличение на размер служебной информации освобождаемого блока, а размер блока будет добавлен в конце (оптимизация).
        allocator->free_size += sizeof(freed_block);
        // Присоединение выполнено.
        linked = true;
    }

    // Вспомогательный указатель для случаем, если с левым блоком объеденить не удалось.
    // Следовательно, либо это будет новый блок, либо необходимо присоединить к нему правый блок.
    freed_block* block_header = block;

    // Попытка присоединить к правому блоку.
    if(node && block_right == node)
    {
        // Получение полного размера правого блока памяти, т.к. исчезает необходимость хранить служебную информацию. 
        ptr node_size = node->size + sizeof(freed_block);

        // Если было присоединение к левому блоку памяти, то присоединяем к нему же и правый.
        if(linked)
        {
            prev->size += node_size;
            prev->next = node->next;
            allocator->block_count--;
        }
        // В противном случае присоединяем правый блок памяти, к освобождаемому.
        else
        {
            block_header->size += node_size;
            block_header->next = node->next;
            block_header->magic = FREED_BLOCK_MAGIC;

            if(prev)
            {
                prev->next = block;
            }
            else
            {
                allocator->blocks = block;
            }

            // Присоединение выполнено.
            linked = true;
        }

        // Увеличение на размер служебной информации правого блока, а размер свободного блока будет добавлен в конце (оптимизация).
        // Размер правого давно добавлен, потому и не добавляется.
        allocator->free_size += sizeof(freed_block);
    }

    // В случае если, присоединение к левому и/или правому не состоялось, то создается новый блок свободной памяти.
    if(!linked)
    {
        block_header->next = node;
        block_header->magic = FREED_BLOCK_MAGIC;

        if(prev)
        {
            prev->next = block;
        }
        else
        {
            allocator->blocks = block;
        }

        allocator->block_count++;
    }

    allocator->free_size += block_unaligned_size;
    return true;
}

ptr dynamic_allocator_get_free_space(dynamic_allocator* allocator)
{
    // Проверка, что распределитель памяти действующий.
    if(is_dynamic_allocator_invalid(allocator, __FUNCTION__))
    {
        return 0;
    }

    return allocator->free_size;
}

ptr dynamic_allocator_get_free_block_count(dynamic_allocator* allocator)
{
    // Проверка, что распределитель памяти действующий.
    if(is_dynamic_allocator_invalid(allocator, __FUNCTION__))
    {
        return 0;
    }

    return allocator->block_count;
}

bool dynamic_allocator_block_get_size(void* block, ptr* out_size)
{
    if(!block || !out_size)
    {
        kerror("Function '%s' requires a valid pointer to memory block and out_size to store the value.", __FUNCTION__);
        return false;
    }

    allocated_block* header = POINTER_GET_OFFSET(block, -sizeof(allocated_block));

    if(header->magic != ALLOCATED_BLOCK_MAGIC)
    {
        kerror("Function '%s': Invalid block magic (possible corruption).", __FUNCTION__);
        return false;
    }

    *out_size = header->unaligned_size - header->alignment_size;
    return true;
}

bool dynamic_allocator_block_get_alignment(void* block, u16* out_alignment)
{
    if(!block || !out_alignment)
    {
        kerror("Function '%s' requires a valid pointer to memory block and out_alignment to store the value.", __FUNCTION__);
        return false;
    }

    allocated_block* header = POINTER_GET_OFFSET(block, -sizeof(allocated_block));

    if(header->magic != ALLOCATED_BLOCK_MAGIC)
    {
        kerror("Function '%s': Invalid block magic (possible corruption).", __FUNCTION__);
        return false;
    }

    *out_alignment = header->alignment;
    return true;
}
