// Cобственные подключения.
#include "memory/memory.h"
#include "memory/allocators/dynamic_allocator.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "kmutex.h"
#include "platform/memory.h"

// TODO: Сделать отдельную подсистему для профилировки памяти, таймкода, стека вызовов и др. И вынести это туда!
typedef struct memory_stats {
    ptr total_allocated;
    ptr tagged_allocated[MEMORY_TAGS_MAX];
} memory_stats;

typedef struct memory_system_state {
    // Конфигурация системы.
    memory_system_config config;
    // Требования системы памяти.
    ptr system_requirement;
    // Статистика по используемой памяти.
    memory_stats stats;
    // Счетчик вызова функции выделения памяти.
    ptr allocation_count;
    // Мьютекс распределителя памяти.
    mutex allocation_mutex;
    // Указатель на динамический распределитель памяти.
    dynamic_allocator* allocator;
} memory_system_state;

static memory_system_state* state_ptr = null;

// Проверяет и указывает на статус истемы.
bool is_memory_system_invalid(const char* func)
{
    if(!state_ptr)
    {
        if(func)
        {
            kerror("Function '%s' requires the memory system to be initialized. Call 'memory_system_initialize' first.", func);
        }
        return true;
    }
    return false;
}

bool memory_system_initialize(memory_system_config* config)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once!", __FUNCTION__);
        return false;
    }

    if(!config || !config->total_allocation_size)
    {
        kerror(
            "Function '%s' requires a valid pointer to config and total_allocation_size greater than zero.",
            __FUNCTION__
        );
        return false;
    }

    // Требования состояния системы.
    ptr state_memory_requirement = sizeof(struct memory_system_state);

    // Требования динамического распределителя памяти.
    ptr allocator_memory_requirement = 0;
    dynamic_allocator_create(config->total_allocation_size, &allocator_memory_requirement, null);

    // Выделение требуемой памяти платформой.
    ptr memory_requirement = state_memory_requirement + allocator_memory_requirement;
    void* memory = platform_memory_allocate(memory_requirement);

    if(!memory)
    {
        // TODO: Сделать обертку для вывода количества байт в зависимости
        // от длинны c соответствующей единице измерения.
        kfatal(
            "Function '%s': Failed to allocate %lu B of memory to the system. Cannot continue.",
            __FUNCTION__, memory_requirement
        );
        return false;
    }

    platform_memory_zero(memory, sizeof(struct memory_system_state));
    state_ptr = memory;

    // Копирование конфигурации системы.
    state_ptr->config.total_allocation_size = config->total_allocation_size;
    state_ptr->system_requirement = memory_requirement;

    // Создание динамического распределителя памяти.
    void* allocator_memory = POINTER_GET_OFFSET(state_ptr, state_memory_requirement);
    state_ptr->allocator = dynamic_allocator_create(config->total_allocation_size, &allocator_memory_requirement, allocator_memory);

    if(!state_ptr->allocator)
    {
        kfatal("Function '%s': Unable to setup internal allocator.", __FUNCTION__);
        return false;
    }

    // Создание мьютекса для распределителя памяти.
    if(!kmutex_create(&state_ptr->allocation_mutex))
    {
        kfatal("Function '%s' unable to create allocation mutex!", __FUNCTION__);
        return false;
    }

    // TODO: Написать функцию которая выводит сокращенную запись байт.
    ktrace("Function '%s': Memory system has %lu B of memory to use.", __FUNCTION__, memory_requirement);
    return true;
}

void memory_system_shutdown()
{
    if(is_memory_system_invalid(__FUNCTION__))
    {
        return;
    }

    // Выводит информацию об утечках памяти.
    if(state_ptr->stats.total_allocated > 0)
    {
        kwarng("Detecting memory leaks...");
        const char* meminfo = memory_system_usage_str();
        kwarng(meminfo);
        string_free(meminfo);
    }

    // Уничтожение мьютекса.
    // NOTE: только после memory_system_usage_str, т.к. она использует string_duplicate,
    //       которая в свою очередь использует этот распределитель памяти.
    kmutex_destroy(&state_ptr->allocation_mutex);

    // Уничтожение динамического распределителя памяти.
    dynamic_allocator_destroy(state_ptr->allocator);

    // Уничтожение памяти выделенной платформой.
    platform_memory_free(state_ptr);

    state_ptr = null;
}

void* memory_allocate(ptr size, u16 alignment, memory_tag tag)
{
    if(!size || !alignment)
    {
        kerror("Function '%s' requires size and alignment greater than zero.", __FUNCTION__);
        return null;
    }

    if(tag >= MEMORY_TAGS_MAX)
    {
        kerror("Function '%s': Tag is out of bounds.", __FUNCTION__);
        return null;
    }

    if(tag == MEMORY_TAG_UNKNOWN)
    {
        kwarng("Function '%s': Allocation with MEMORY_TAG_UNKNOWN. Re-class this allocation.", __FUNCTION__);
    }

    void* block = null;

    // Выбор способа выделения памяти исходя от состояния системы.
    if(state_ptr)
    {
        if(!kmutex_lock(&state_ptr->allocation_mutex))
        {
            kfatal("Function '%s': Unable obtaining mutex lock during allocation.", __FUNCTION__);
            return null;
        }
        
        block = dynamic_allocator_allocate(state_ptr->allocator, size, alignment);

        if(block)
        {
            state_ptr->stats.tagged_allocated[tag] += size;
            state_ptr->stats.total_allocated += size;
            state_ptr->allocation_count++;
        }

        kmutex_unlock(&state_ptr->allocation_mutex);
    }
    else
    {
        kwarng("Function '%s' called before the memory system is initialized.", __FUNCTION__);
        block = platform_memory_allocate(size);
    }

    if(!block)
    {
        kfatal("Function '%s' could not allocate memory. Stop for debugging.", __FUNCTION__);
    }

    return block;
}

void memory_allocate_report(ptr size, memory_tag tag)
{
    if(!kmutex_lock(&state_ptr->allocation_mutex))
    {
        kfatal("Function '%s': Unable obtaining mutex lock during allocation.", __FUNCTION__);
        return;
    }

    state_ptr->stats.tagged_allocated[tag] += size;
    state_ptr->stats.total_allocated += size;
    state_ptr->allocation_count++;

    kmutex_unlock(&state_ptr->allocation_mutex);
}

void memory_free(void* block, ptr size, memory_tag tag)
{
    if(!block)
    {
        kerror("Function '%s' requires a valid memory pointer.", __FUNCTION__);
        return;
    }

    if(!size)
    {
        kerror("Function '%s' requires a size greater than zero.", __FUNCTION__);
        return;
    }

    if(tag >= MEMORY_TAGS_MAX)
    {
        kerror("Function '%s': Tag is out of bounds.", __FUNCTION__);
        return;
    }

    // Выбор способа освобождения памяти исходя от состояния системы.
    if(state_ptr)
    {
        if(!kmutex_lock(&state_ptr->allocation_mutex))
        {
            kfatal("Function '%s': Unable obtaining mutex lock for free operation.", __FUNCTION__);
            return;
        }

        if(dynamic_allocator_free(state_ptr->allocator, block))
        {
            state_ptr->stats.tagged_allocated[tag] -= size;
            state_ptr->stats.total_allocated -= size;
            state_ptr->allocation_count--;
        }

        kmutex_unlock(&state_ptr->allocation_mutex);
    }
    else
    {
        platform_memory_free(block);
    }
}

void memory_free_report(ptr size, memory_tag tag)
{
    if(!kmutex_lock(&state_ptr->allocation_mutex))
    {
        kfatal("Function '%s': Unable obtaining mutex lock for free operation.", __FUNCTION__);
        return;
    }

    state_ptr->stats.tagged_allocated[tag] -= size;
    state_ptr->stats.total_allocated -= size;
    state_ptr->allocation_count--;

    kmutex_unlock(&state_ptr->allocation_mutex);
}

bool memory_get_size(void* block, ptr* out_size)
{
    return dynamic_allocator_block_get_size(block, out_size);    
}

bool memory_get_alignment(void* block, u16* out_alignment)
{
    return dynamic_allocator_block_get_alignment(block, out_alignment);
}

const char* memory_system_usage_str()
{
    if(is_memory_system_invalid(__FUNCTION__))
    {
        return "";
    }

    static const char* memory_tag_strings[MEMORY_TAGS_MAX + 1] = {
        "UNKNOWN        ",
        "SYSTEM         ",
        "FILE           ",
        "ARRAY          ",
        "DARRAY         ",
        "HASHTABLE      ",
        "ALLOCATOR      ",
        "DICTIONARY     ",
        "RING QUEUE     ",
        "STRING         ",
        "APPLICATION    ",
        "JOB            ",
        "RESOURCE       ",
        "TEXTURE        ",
        "MATERIAL       ",
        "RENDERER       ",
        "GAME           ",
        "TRANSFORM      ",
        "ENTITY         ",
        "ENTITY NODE    ",
        "NODE           ",
        "VULKAN         ",
        "VULKAN_EXT     ",
        "GPU_LOCAL      ",
        "TOTAL          "
    };

    const ptr gib = 1024 * 1024 * 1024;
    const ptr mib = 1024 * 1024;
    const ptr kib = 1024;

    char buffer[8000] = "System memory use (tagged):\n";
    // TODO: Использовать обертку над функцией.
    ptr offset = string_length(buffer);

    for(u32 i = 0; i <= MEMORY_TAGS_MAX; ++i)
    {
        char unit[4] = "XiB";
        f32 amount = 1.0f;
        ptr size = 0;

        if(i < MEMORY_TAGS_MAX)
        {
            size = state_ptr->stats.tagged_allocated[i];
        }
        else
        {
            size = state_ptr->stats.total_allocated;
        }

        if(size >= gib)
        {
            unit[0] = 'G';
            amount = size / (f32)gib;
        }
        else if(size >= mib)
        {
            unit[0] = 'M';
            amount = size / (f32)mib;
        }
        else if(size >= kib)
        {
            unit[0] = 'K';
            amount = size / (f32)kib;
        }
        else
        {
            unit[0] = 'B';
            unit[1] = '\0';
            amount = (f32)size;
        }

        i32 length = string_format(buffer + offset, "\t%s: %7.2f %s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }

    return string_duplicate(buffer);
}

ptr memory_system_allocation_count()
{
    if(is_memory_system_invalid(__FUNCTION__))
    {
        return 0;
    }

    return state_ptr->allocation_count;
}
