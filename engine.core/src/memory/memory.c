// Cобственные подключения.
#include "memory/memory.h"

// Внутренние подключения.
#include "logger.h"
#include "platform/memory.h"

// Внешние подключения.
// TODO: Удалить после создания обертки над функциями.
#include <string.h>
#include <stdio.h>

typedef struct memory_stats {
    u64 total_allocated;
    u64 tagged_allocated[MEMORY_TAGS_MAX];
} memory_stats;

typedef struct memory_system_state {
    // Статистика по используемой памяти.
    memory_stats stats;
    // Счетчик вызова функции выделения памяти.
    u64 alloc_count;
} memory_system_state;

static memory_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the memory system to be initialized. Call 'memory_system_initialize' first.";

void memory_system_initialize(u64* memory_requirement, void* memory)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once!", __FUNCTION__);
        return;
    }

    *memory_requirement = sizeof(struct memory_system_state);
    if(!memory) return;

    platform_memory_zero(memory, *memory_requirement);
    state_ptr = memory;
}

void memory_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    if(state_ptr->stats.total_allocated > 0)
    {
        kwarng("Detecting memory leaks...");
        const char* meminfo = memory_system_usage_str();
        kwarng(meminfo);
        platform_memory_free((void*)meminfo);
    }

    state_ptr = null;
}

void* memory_allocate(u64 size, memory_tag tag)
{
    if(!size)
    {
        kerror("Function '%s' requires a size greater than zero. Return null!", __FUNCTION__);
        return null;
    }

    if(tag >= MEMORY_TAGS_MAX)
    {
        kerror("Function '%s': Tag is out of bounds. Return null!", __FUNCTION__);
        return null;
    }

    if(tag == MEMORY_TAG_UNKNOWN)
    {
        kwarng("Memory allocation with MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    void* block = platform_memory_allocate(size);

    if(!block)
    {
        kwarng("Function '%s' could not allocate memory and returned null.", __FUNCTION__);
    }

    if(state_ptr && block)
    {
        state_ptr->stats.tagged_allocated[tag] += size;
        state_ptr->stats.total_allocated += size;
        state_ptr->alloc_count++;
    }

    return block;
}

void memory_free(void* block, u64 size, memory_tag tag)
{
    if(!block)
    {
        kerror("Function '%s' requires a non-null memory pointer. Just return!", __FUNCTION__);
        return;
    }

    if(!size)
    {
        kerror("Function '%s' requires a size greater than zero. Just return!", __FUNCTION__);
        return;
    }

    if(tag >= MEMORY_TAGS_MAX)
    {
        kerror("Function '%s': Tag is out of bounds. Just return!", __FUNCTION__);
        return;
    }

    if(state_ptr)
    {
        state_ptr->stats.tagged_allocated[tag] -= size;
        state_ptr->stats.total_allocated -= size;
    }

    platform_memory_free(block);
}

const char* memory_system_usage_str()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return "<void>";
    }

    static const char* memory_tag_strings[MEMORY_TAGS_MAX + 1] = {
        "UNKNOWN          ",
        "SYSTEM           ",
        "ARRAY            ",
        "DARRAY           ",
        "HASHTABLE        ",
        "LINEAR ALLOCATOR ",
        "DICTIONARY       ",
        "RING QUEUE       ",
        "STRING           ",
        "APPLICATION      ",
        "JOB              ",
        "TEXTURE          ",
        "MATERIAL         ",
        "RENDERER         ",
        "GAME             ",
        "TRANSFORM        ",
        "ENTITY           ",
        "ENTITY NODE      ",
        "NODE             ",
        "TOTAL            "
    };

    const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    char buffer[8000] = "System memory use (tagged):\n";
    // TODO: Использовать обертку над функцией.
    u64 offset = strlen(buffer);

    for(u32 i = 0; i <= MEMORY_TAGS_MAX; ++i)
    {
        char unit[4] = "XiB";
        f32 amount = 1.0f;
        u64 size = 0;

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

        // TODO: Использовать обертку над функцией.
        i32 length = snprintf(buffer + offset, 8000, "\t%s: %7.2f %s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }

    // TODO: Использовать обертку!
    return strdup(buffer);
}

u64 memory_system_alloc_count()
{
    if(!state_ptr) return 0;
    return state_ptr->alloc_count;
}
