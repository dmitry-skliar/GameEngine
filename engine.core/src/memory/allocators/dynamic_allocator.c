// Собственные подключения.
#include "memory/allocators/dynamic_allocator.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

/*
            базовый адрес блока (адрес свободного блока, а так же выделенного блока, если padding = 0).
           |              невыровненный адрес данных.
           v             v
    block: [freed_header |       unaligned data      ]
    block: [padding | allocated_header | aligned data]
           ^        ^                  ^
           |        |                  выровненный адрес данных, возвращаемый dynamic_allocator_allocate.
           |        адрес заголовка выделенного блока.
           базовый адрес блока.

    * Размер зоны aligned data может быть равен ранее запрошенному размеру (size) + возможный остаток (который оказался < MIN_SPLIT_SIZE).

    * Здесь padding является частью размера свободного блока, т.е. sizeof(unaligned data) =  sizeof(padding) + sizeof(aligned data)!

    * Стоит добавить, что при выделении блока, нужного размера с учетом размера выравнивания может не оказаться, а потому
      берется первый блок с достаточным размером, после чего проверяется, можно ли его разделить. Если после разделения остаточного
      участка памяти хватает для размещения в нем заголовка + 1 байта данных, тогда разделение считается состоявшимся, а если нет,
      тогда обновляется запрашиваемый размер блока, равный всему этому блоку (т.е. появляется так называемый 'возможный остаток').
*/

// Служебная информация выделенного блока памяти.
typedef struct allocated_header {
    // Проверочное число.
    ptr checksum;
    // Изначальный размер свободного блока (выравнивание + запрошенный размер [+ возможный остатк]).
    ptr size;
    // Кратность выравнивания блока (используется при перераспределении памяти).
    u16 alignment;
    // Размер смещения выровненного адреса относительно невыровненного в байтах.
    u16 alignment_size;
} allocated_header;

// Cлужебная информация свободного блока памяти.
typedef struct freed_header {
    // Проверочное число.
    ptr checksum;
    // Изначальный размер свободного блока.
    ptr size;
    // Указатель на следующий блок свободной памяти.
    struct freed_header* next;
} freed_header;

// Контекст экземпляра динамического распределителя памяти.
struct dynamic_allocator {
    // Количество памяти в распоряжении распределителя в байтах.
    ptr total_size;
    // Количество доступной в данный момент памяти в байтах.
    ptr free_size;
    // Указатель на начало пула памяти (только хранится).
    void* memory_pool_start;
    // Указатель на конец пула памяти (только храниться).
    void* memory_pool_end;
    // Количество свободных блоков памяти.
    ptr free_block_count;
    // Указатель на первый свободный блок пула памяти.
    freed_header* free_block_head;
};

// Обязательная проверка, что бы при выравнивании блока памяти не произошло наложение заголовка с другим блоком памяти.
// NOTE: Этот случай может возникнуть в том случае, когда размер выравнивания равен 0.
STATIC_ASSERT(sizeof(allocated_header) == sizeof(freed_header), "The allocated header size must be equal to the freed header size.");

// Минимально возможный размер блока памяти (заголовок + минимальный размер данных блока).
#define MIN_BLOCK_SIZE (sizeof(freed_header) + MIN_SPLIT_SIZE)

// Проверочные значения.
#define BLOCK_FREED     0xDEADBEEF  
#define BLOCK_ALLOCATED 0xCAFEBABE  

// Проверяет указатель и контекст распределителя памяти.
bool is_dynamic_allocator_invalid(const dynamic_allocator* allocator, const char* func)
{
    // NOTE: Проверяй при отладке: memory_pool_begin < memory_pool_end и free_size <= total_size.
    if(!allocator || !allocator->memory_pool_start || !allocator->memory_pool_end)
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
            __FUNCTION__, sizeof(dynamic_allocator), sizeof(freed_header)
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

    // Вычисление размера блока памяти (учитывается хранение контекста распределителя + заголовка блока).
    ptr block_size = total_size - sizeof(dynamic_allocator) - sizeof(freed_header); 

    // Настройка контекста распределителя памяти.
    // NOTE: Т.к. каждое значение структуры заполняется, то вызывать обнуление ее не требуется.
    dynamic_allocator* allocator = memory;
    allocator->memory_pool_start = POINTER_GET_OFFSET(allocator, sizeof(dynamic_allocator));
    allocator->memory_pool_end   = POINTER_GET_OFFSET(allocator->memory_pool_start, sizeof(freed_header) + block_size); // Указывает на первый адрес за границей памяти.
    allocator->free_block_head   = allocator->memory_pool_start;
    allocator->free_block_count  = 1;
    allocator->total_size        = block_size; // Максимальный размер памяти при полном освобождении равен размеру первого блока.
    allocator->free_size         = block_size; // ... аналогично.

    // Настройка первого блока свободной памяти.
    allocator->free_block_head->size     = block_size;
    allocator->free_block_head->next     = null;
    allocator->free_block_head->checksum = BLOCK_FREED;

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
        f32 free_amount = 0;
        f32 total_amount = 0;
        const char* free_unit = memory_get_unit_for(allocator->free_size, &free_amount);
        const char* total_unit = memory_get_unit_for(allocator->total_size, &total_amount);
        kwarng("Function '%s' called when memory has not yet been freed. The operation will not be aborted!", __FUNCTION__);
        ktrace("Free size is %.2f %s and total size is %.2f %s.", free_amount, free_unit, total_amount, total_unit);
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
        freed_header* curr = allocator->free_block_head;
        freed_header* prev = null;

        // Процесс поиска подходящего блока памяти.
        while(curr)
        {
            // Получение невыровненого адреса памяти (расположен сразу после заголовка блока памяти). 
            ptr unaligned_offset = (ptr)POINTER_GET_OFFSET(curr, sizeof(freed_header));
            // Получение выровненого адреса памяти (адрес памяти который необходимо вернуть, если размер памяти подходит).
            ptr aligned_offset = get_aligned(unaligned_offset, alignment);
            // Размер выравнивания в байтах (cохранить в заголовок выделеного блока!).
            u16 alignment_size = aligned_offset - unaligned_offset;
            // Требуемый размер памяти относительно невыровненой границы памяти (cохранить в заголовок выделеного блока!).
            ptr required_size = size + alignment_size;

            // TODO: Заменить проверкой флага переполнения процессора (см. __builtin_add_overflow).
            // Проверка на переполнение required_size.
            if(size > PTR_MAX - alignment_size)
            {
                kerror("Function '%s': Unable to align, requested size is too large.", __FUNCTION__);
                return null;
            }

            // Проверка соответствия размера текущего блока.
            if(curr->size >= required_size)
            {
                // Вспомогательный указатель на следующий свободный блок памяти.
                freed_header* free_block = null;
                // Вычисление остатка памяти после разделения найденого блока.
                ptr remaining = curr->size - required_size;

                // Проверка на соответствие минимальным требованиям свободного блока памяти для разделения.
                if(remaining >= MIN_BLOCK_SIZE)
                {
                    // NOTE: Для того, что бы большой блок памяти разделялся в последнюю очередь, нужно при разделении
                    //       отдавать память с наименьшим адресом, а оставшуюся часть помещать в список. Это должно
                    //       снизить дробление памяти на мелкие блоки про определенных условиях.

                    // Настройка оставшейся свободной части блока, для добавления в список свободной памяти. 
                    free_block = POINTER_GET_OFFSET(unaligned_offset, required_size);
                    free_block->size = remaining - sizeof(freed_header); // Т.к. добавляется заголовок в оставшуюся часть блока.
                    free_block->next = curr->next;
                    free_block->checksum = BLOCK_FREED;

                    // NOTE: Требуемый размер памяти отнимается позже (оптимизация).
                    allocator->free_size -= sizeof(freed_header); // Т.к. добавляется заголовок в оставшуюся часть блока.
                }
                // Разделение невозможно, или размер блока точно соответствует требованиям.
                else
                {
                    // Для удаления текущего блока памяти из пула свободной памяти.
                    free_block = curr->next;
                    // Обновление размера т.к. разделение текущего блока невозможно, будет отдан весь блок.
                    required_size = curr->size;
                    // Т.к. блок удаляется целеком без разделения.
                    allocator->free_block_count--;
                }

                // Удаление адреса выделяемого блока из пула свободной памяти.
                if(prev)
                {
                    prev->next = free_block;
                }
                else
                {
                    allocator->free_block_head = free_block;
                }

                // Уменьшение размера общей памяти на требуемый размер.
                allocator->free_size -= required_size;

                // Указатель на заголовок выделенного блока памяти, для сохранения информации о нем.
                allocated_header* block = POINTER_GET_OFFSET(aligned_offset, -sizeof(allocated_header));
                block->size = required_size;
                block->alignment_size = alignment_size;
                block->alignment = alignment;
                block->checksum = BLOCK_ALLOCATED;
                return (void*)aligned_offset;
            }

            prev = curr;
            curr = curr->next;
        }
    }

    f32 requested_amount = 0;
    f32 remaining_amount = 0;
    const char* requested_unit = memory_get_unit_for(size, &requested_amount);
    const char* remaining_unit = memory_get_unit_for(allocator->free_size, &remaining_amount);

    kwarng(
        "Function '%s': No block with enough free space found. Requested %.2f %s (alignment %u), remaining %.2f %s (in free blocks %llu).",
        __FUNCTION__, requested_amount, requested_unit, alignment, remaining_amount, remaining_unit, allocator->free_block_count
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
    allocated_header* block_header = POINTER_GET_OFFSET(block, -sizeof(allocated_header));

    // Проверка значения, для исключения повреждения или двойного освобождения памяти.
    if(block_header->checksum != BLOCK_ALLOCATED)
    {
        kerror("Function '%s': Invalid block magic (possible double free or corruption).", __FUNCTION__);
        return false;
    }

    // Пометка как освобожденного блока.
    block_header->checksum = BLOCK_FREED; // Исключает случай если попытаться получить доступ к старому заголовку с тем же адресом.

    // Сохранение переменных заголовка, для дальнейшего использования.
    ptr block_size           = block_header->size;
    u16 block_alignment_size = block_header->alignment_size;

    // Получение указателя на начало блока (будущий заголовок свободного блока) и его конец (начало следующего блока).
    void* block_start = POINTER_GET_OFFSET(block_header, -block_alignment_size);
    void* block_end = POINTER_GET_OFFSET(block, block_size - block_alignment_size); // Т.к. block_size учитывает выравнивание!

    // Проверка выхода за границы пула памяти. 
    void* pool_start = allocator->memory_pool_start;
    void* pool_end = allocator->memory_pool_end;

    if(block_start < pool_start || block_end > pool_end)
    { 
        kerror("Function '%s': Attempting to free memory out of range %p...%p.", __FUNCTION__, pool_start, (u8*)pool_end - 1);
        return false;
    }

    // Вспомогательные указатели для поиска левого и правого блоков свободной памяти для присоединения.
    freed_header* curr  = allocator->free_block_head;
    freed_header* prev  = null;

    // Т.к. в пуле нет блоков памяти, выполняется создание его.
    if(!curr)
    {
        allocator->free_block_head = block_start;
        allocator->free_block_head->size = block_size;
        allocator->free_block_head->next = null;
        allocator->free_block_head->checksum = BLOCK_FREED;

        allocator->free_size += block_size;
        allocator->free_block_count++;
        return true;
    }

    // Поиск позиции блока.
    // Левый   : prev = null, node.
    // Средний : prev, node.
    // Правый  : prev, node = null.
    while(curr && block_start > (void*)curr)
    {
        prev = curr;
        curr = curr->next;
    }

    // Вычисление вспомогательных адресов: левого конечного и правого начального блоков памяти.
    void* curr_start = curr;
    void* prev_end = prev ? POINTER_GET_OFFSET(prev, sizeof(freed_header) + prev->size) : null; // При таком расчете, это начало следующего блока.

    // Проверка на коллизию границ относительно левого и/или правого блоков.
    if((prev && (prev_end > block_start)) || (curr && (curr_start < block_end)))
    {
        f32 block_amount = 0;
        const char* block_unit = memory_get_unit_for(block_size, &block_amount);
        kfatal(
            "Function '%s': Block %p (start %p end %p size %.2f %s alignment size %u). Collision detected. Stopped for debugging!",
            __FUNCTION__, block, block_start, block_end, block_amount, block_unit, block_alignment_size
        );
        return false;
    }

    // Указывает о присоединение освобождаемого блока к левому или правому.
    bool block_linked = false;

    // Попытка присоединить к левому блоку.
    if(prev && prev_end == block_start)
    {
        // Просто увеличиваем размер левого блока на величину освобождаемого блока
        // + исчезает необходимость хранить служебную информацию освобождаемого блока.
        prev->size += block_size + sizeof(freed_header);
        // Увеличение на размер служебной информации освобождаемого блока, а размер блока будет добавлен в конце (оптимизация).
        allocator->free_size += sizeof(freed_header);
        // Присоединение выполнено.
        block_linked = true;
    }

    // Вспомогательный указатель указывающий на заголовок освобождаемого блока.
    freed_header* free_block = block_start;

    // Попытка присоединить к правому блоку.
    if(curr && curr_start == block_end)
    {
        // Получение полного размера правого блока памяти, т.к. исчезает необходимость хранить служебную информацию.
        ptr curr_size = curr->size + sizeof(freed_header);

        // Если было присоединение к левому блоку памяти, то присоединяем к нему же и правый.
        if(block_linked)
        {
            prev->size += curr_size;
            prev->next = curr->next;        // Удаление блока из листа свободных блоков.
            allocator->free_block_count--;
        }
        // В противном случае присоединяем правый блок памяти, к освобождаемому.
        else
        {
            free_block->size = block_size + curr_size; // Т.к. в результате выравнивания заголовок может сместиться.
            free_block->next = curr->next;
            free_block->checksum = BLOCK_FREED; // Т.к. в результате выравнивания заголовок может сместиться.

            if(prev)
            {
                prev->next = block_start;
            }
            else
            {
                allocator->free_block_head = block_start;
            }

            // Присоединение выполнено.
            block_linked = true;
        }

        // Увеличение на размер служебной информации правого блока, а размер свободного блока будет добавлен в конце (оптимизация).
        // Размер свободного правого блока давно является частью свободной памяти, а потому не добавляется.
        allocator->free_size += sizeof(freed_header);
    }

    // В случае если, присоединение к левому и/или правому не состоялось, то создается новый блок свободной памяти.
    if(!block_linked)
    {
        free_block->size = block_size; // Т.к. в результате выравнивания заголовок может сместиться.
        free_block->next = curr;
        free_block->checksum = BLOCK_FREED; // Т.к. в результате выравнивания заголовок может сместиться.

        if(prev)
        {
            prev->next = block_start;
        }
        else
        {
            allocator->free_block_head = block_start;
        }

        allocator->free_block_count++;
    }

    allocator->free_size += block_size; // Оптимизация вычислений.
    return true;
}

ptr dynamic_allocator_get_total_space(dynamic_allocator* allocator)
{
    // Проверка, что распределитель памяти действующий.
    if(is_dynamic_allocator_invalid(allocator, __FUNCTION__))
    {
        return 0;
    }

    return allocator->total_size;
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

    return allocator->free_block_count;
}

bool dynamic_allocator_block_get_size(void* block, ptr* out_size)
{
    if(!block || !out_size)
    {
        kerror("Function '%s' requires a valid pointer to memory block and out_size to store the value.", __FUNCTION__);
        return false;
    }

    allocated_header* header = POINTER_GET_OFFSET(block, -sizeof(allocated_header));

    if(header->checksum != BLOCK_ALLOCATED)
    {
        kerror("Function '%s': Invalid block magic (possible corruption).", __FUNCTION__);
        return false;
    }

    *out_size = header->size - header->alignment_size;
    return true;
}

bool dynamic_allocator_block_get_alignment(void* block, u16* out_alignment)
{
    if(!block || !out_alignment)
    {
        kerror("Function '%s' requires a valid pointer to memory block and out_alignment to store the value.", __FUNCTION__);
        return false;
    }

    allocated_header* header = POINTER_GET_OFFSET(block, -sizeof(allocated_header));

    if(header->checksum != BLOCK_ALLOCATED)
    {
        kerror("Function '%s': Invalid block magic (possible corruption).", __FUNCTION__);
        return false;
    }

    *out_alignment = header->alignment;
    return true;
}
