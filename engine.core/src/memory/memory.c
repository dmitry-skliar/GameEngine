// Cобственные подключения.
#include "memory/memory.h"
#include "memory/allocators/dynamic_allocator.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "kmutex.h"
#include "platform/memory.h"

typedef struct memory_stats {
    // Пиковое значение использования памяти.
    ptr peak_allocated;
    // Обшее испольщование памяти в данный момент.
    ptr total_allocated;
    // Использование памяти по тегам в данный момент.
    ptr tagged_allocated[MEMORY_TAGS_MAX];
    // Количество выделенных блоков памяти в данный момент.
    ptr allocation_count;
} memory_stats;

typedef struct memory_system_state {
    // Конфигурация системы.
    memory_system_config config;
    // Требования системы памяти.
    ptr system_requirement;
    // Статистика по используемой памяти.
    memory_stats stats;
    // Мьютекс распределителя памяти.
    mutex allocation_mutex;
    // Указатель на динамический распределитель памяти.
    dynamic_allocator* allocator;
} memory_system_state;

// Контекст системы памяти.
static memory_system_state* state_ptr = null;

// Проверяет и указывает на статус системы.
static bool is_memory_system_invalid(const char* func)
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

    // Получение количества и единица измерения выделенной памяти.
    f32 amount = 0;
    const char* unit = memory_get_unit_for(memory_requirement, &amount);

    if(!memory)
    {
        kfatal("Failed to allocate %.2f %s of memory to the system. Unable to continue.", amount, unit);
        return false;
    }

    // Обнуление контекста системы памяти.
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

    ktrace("The memory system has %.2f %s of memory available for use.", amount, unit);
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
    // NOTE: используется после memory_system_usage_str, т.к. она использует string_duplicate,
    //       которая в свою очередь использует этот распределитель памяти.
    kmutex_destroy(&state_ptr->allocation_mutex);

    // Уничтожение динамического распределителя памяти.
    dynamic_allocator_destroy(state_ptr->allocator);

    // Уничтожение памяти выделенной платформой.
    platform_memory_free(state_ptr);

    // Делает недействительным систему памяти.
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

        // Обновление статистики использования памяти.
        if(block)
        {
            // Запрашиваем реальный размер блока, т.к. не всегда может совпадать с запрашиваемым.
            dynamic_allocator_block_get_size(block, &size);

            state_ptr->stats.tagged_allocated[tag] += size;
            state_ptr->stats.total_allocated += size;
            state_ptr->stats.allocation_count++;

            if(state_ptr->stats.total_allocated > state_ptr->stats.peak_allocated)
            {
                state_ptr->stats.peak_allocated = state_ptr->stats.total_allocated;
            }
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
    if(is_memory_system_invalid(__FUNCTION__))
    {
        return;
    }

    if(!kmutex_lock(&state_ptr->allocation_mutex))
    {
        kfatal("Function '%s': Unable obtaining mutex lock during allocation.", __FUNCTION__);
        return;
    }

    // Здесь обновлять размер не требуется, реального блока нет.

    state_ptr->stats.tagged_allocated[tag] += size;

    // NOTE: Пропуск учета памяти GPU, для корректного отображения статистики!
    if(tag != MEMORY_TAG_GPU_LOCAL)
    {
        state_ptr->stats.total_allocated += size;
        state_ptr->stats.allocation_count++;

        if(state_ptr->stats.total_allocated > state_ptr->stats.peak_allocated)
        {
            state_ptr->stats.peak_allocated = state_ptr->stats.total_allocated;
        }
    }

    kmutex_unlock(&state_ptr->allocation_mutex);
}

void memory_free(void* block, memory_tag tag)
{
    if(!block)
    {
        kerror("Function '%s' requires a valid memory pointer.", __FUNCTION__);
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

        ptr size = 0;

        // Получение размера блока.
        dynamic_allocator_block_get_size(block, &size);

        if(dynamic_allocator_free(state_ptr->allocator, block))
        {
            state_ptr->stats.tagged_allocated[tag] -= size;
            state_ptr->stats.total_allocated -= size;
            state_ptr->stats.allocation_count--;
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
    if(is_memory_system_invalid(__FUNCTION__))
    {
        return;
    }

    if(!kmutex_lock(&state_ptr->allocation_mutex))
    {
        kfatal("Function '%s': Unable obtaining mutex lock for free operation.", __FUNCTION__);
        return;
    }

    state_ptr->stats.tagged_allocated[tag] -= size;

    // NOTE: Пропуск учета памяти GPU, для корректного отображения статистики!
    if(tag != MEMORY_TAG_GPU_LOCAL)
    {
        state_ptr->stats.total_allocated -= size;
        state_ptr->stats.allocation_count--;
    }

    kmutex_unlock(&state_ptr->allocation_mutex);
}

bool memory_block_get_size(void* block, ptr* out_size)
{
    // NOTE: Проверка статуса не требуется, т.к. если система небыла инициализирована, то и блок проверку не пройдет.
    return dynamic_allocator_block_get_size(block, out_size);    
}

bool memory_block_get_alignment(void* block, u16* out_alignment)
{
    // NOTE: Проверка статуса не требуется, т.к. если система небыла инициализирована, то и блок проверку не пройдет.
    return dynamic_allocator_block_get_alignment(block, out_alignment);
}

const char* memory_system_usage_str()
{
    if(is_memory_system_invalid(__FUNCTION__))
    {
        return "";
    }

    static const char* memory_tag_strings[MEMORY_TAGS_MAX] = {
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
        "VULKAN INTERNAL",
        "GPU LOCAL      "
    };

    //-----------------------------------------------------------------------------------------------------------------------

    // Буфер для вывода информации о памяти.
    char buffer[8000] = "Memory information:\n\n";
    u16 buffer_length = 8000;
    ptr offset = string_length(buffer);

    //-----------------------------------------------------------------------------------------------------------------------

    ptr total_space = dynamic_allocator_get_total_space(state_ptr->allocator);
    ptr free_space = dynamic_allocator_get_free_space(state_ptr->allocator);
    ptr used_space = total_space - free_space;

    f32 total_amount = 0;
    f32 used_amount = 0;
    const char* total_unit = memory_get_unit_for(total_space, &total_amount);
    const char* used_unit = memory_get_unit_for(used_space, &used_amount);

    f64 used_present = ((f64)used_space / total_space) * 100.0f;

    // Запись статистики использования памяти в буфер.
    i32 length = string_format(
        buffer + offset, buffer_length, "Total memory usage: %.2f %s of %.2f %s (%.2f%%)\n", used_amount, used_unit,
        total_amount, total_unit, used_present
    );

    // Обновление смещения для записи следующей строки.
    offset += length;

    //-----------------------------------------------------------------------------------------------------------------------

    ptr peak_space = state_ptr->stats.peak_allocated;

    f32 peak_amount = 0;
    const char* peak_unit = memory_get_unit_for(peak_space, &peak_amount);

    // Процент использования памяти в пике.
    f64 peak_present = ((f64)peak_space / total_space) * 100.0f;

    // Запись пикового использования памяти.
    length = string_format(buffer + offset, 8000, "Peak memory usgae: %.2f %s (%.2f%%)\n", peak_amount, peak_unit, peak_present);

    // Обновление смещения для записи следующей строки.
    offset += length;

    //-----------------------------------------------------------------------------------------------------------------------

    // Запись заглавной строки тегов.
    length = string_format(buffer + offset, 8000, "Memory usege by tags:\n");

    // Обновление смещения для записи следующей строки.
    offset += length;

    //-----------------------------------------------------------------------------------------------------------------------

    for(u32 i = 0; i < MEMORY_TAGS_MAX; ++i)
    {
        // Получение количества и единицы измерения.
        f32 tag_amount = 0;
        const char* tag_unit = memory_get_unit_for(state_ptr->stats.tagged_allocated[i], &tag_amount);

        // Запись строки тега и его значение в буфер.
        length = string_format(buffer + offset, 8000, "  %s: %7.2f %s\n", memory_tag_strings[i], tag_amount, tag_unit);

        // Обновление смещения для записи следующей строки.
        offset += length;
    }

    // Вернуть копию строки. Не забыть удалить после использование с использованием 'string_free'.
    return string_duplicate(buffer);
}

ptr memory_system_allocation_count()
{
    if(is_memory_system_invalid(__FUNCTION__))
    {
        return 0;
    }

    return state_ptr->stats.allocation_count;
}

const char* memory_get_unit_for(ptr bytes, f32* out_amount)
{
    if(bytes >= 1 GiB)
    {
        *out_amount = (f64)bytes / (1 GiB);
        return "GiB";
    }
    else if(bytes >= 1 MiB)
    {
        *out_amount = (f64)bytes / (1 MiB);
        return "MiB";
    }
    else if(bytes >= 1 KiB)
    {
        *out_amount = (f64)bytes / (1 KiB);
        return "KiB";
    }
    else
    {
        *out_amount = (f32)bytes;
        return "B";
    }
}
