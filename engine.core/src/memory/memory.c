#include "memory/memory.h"

#include "platform/memory.h"
#include "debug/assert.h"
#include "logger.h"

// TODO: Удалить после создания обертки над функциями.
#include <string.h>
#include <stdio.h>

// Контрольно число для проверки заголовка.
#define CHECK_NUMBER 0x4d454d00

typedef struct memory_system_context {
    // Статистика использования памяти.
    u64 total_allocated;
    u64 tagged_allocated[MEMORY_TAGS_MAX];
} memory_system_context;

typedef struct memory_header {
    // Число для проверки повреждения памяти.
    u32 check;
    // Метка выделенного участка памяти.
    memory_tag tag;
    // Размер выделенного участа памяти.
    u64 size;
} memory_header;

// Указатель на структуру контекста всей памяти.
static memory_system_context* context = null;

// Сообщения.
static const char* message_context_not_initialized = "Memory system was not initialized. Please first call 'memory_system_initialize'.";

void memory_system_initialize()
{
    kassert_debug(context == null, "Trying to call function 'memory_system_initialize' more than once!");

    context = platform_memory_allocate(sizeof(memory_system_context));
    platform_memory_zero(context, sizeof(memory_system_context));

    kinfor("Memory system started.");
}

void memory_system_shutdown()
{
    kassert_debug(context != null, message_context_not_initialized);

    if(context->total_allocated > 0)
    {
        // TODO: Вывести сообщение о утечке памяти и где это произошло.
        kwarng("Detecting memory leaks...");
    }

    platform_memory_free(context);
    context = null;

    kinfor("Memory system stopped.");
}

u64 memory_size_get(void* block)
{
    kassert_debug(context != null, message_context_not_initialized);
    kassert_debug(block != null, "The 'memory_size_get' function requires a non-null memory pointer.");

    memory_header* header = (memory_header*)((u8*)block - sizeof(memory_header));

    if(header->check != CHECK_NUMBER)
    {
        kfatal("Function 'memory_size_get' could not check the header number, or the memory pointer is invalid!");
    }

    return header->size - sizeof(memory_header);
}

const char* memory_system_usage_get()
{
    kassert_debug(context != null, message_context_not_initialized);

    static const char* memory_tag_strings[MEMORY_TAGS_MAX + 1] = {
        "UNKNOWN          ",
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
        char unit[4] = "Xib";
        f32 amount = 1.0f;
        u64 size = 0;

        if(i < MEMORY_TAGS_MAX)
        {
            size = context->tagged_allocated[i];
        }
        else
        {
            size = context->total_allocated;
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
        i32 length = snprintf(buffer + offset, 8000, "\t%s: %.2f %s\n", memory_tag_strings[i], amount, unit);
        offset += length;
    }

    // TODO: Использовать обертку!
    return strdup(buffer);
}

void* memory_allocate(u64 size, memory_tag tag)
{
    kassert_debug(context != null, message_context_not_initialized);
    kassert_debug(size > 0, "Function 'memory_allocate' requires size more than null.");
    kassert_debug(tag >= MEMORY_TAG_UNKNOWN && tag < MEMORY_TAGS_MAX, "Tag value is out of bounds.");

    if(tag == MEMORY_TAG_UNKNOWN)
    {
        kwarng("Memory allocation with MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    u64 total_size = sizeof(memory_header) + size;
    memory_header* header = platform_memory_allocate(total_size);

    if(header)
    {
        platform_memory_zero(header, sizeof(memory_header));

        header->check = CHECK_NUMBER;
        header->size  = total_size;
        header->tag   = tag;

        context->tagged_allocated[tag] += total_size;
        context->total_allocated += total_size;

        return (void*)((u8*)header + sizeof(memory_header));
    }

    return null;
}

void memory_free(void* block)
{
    kassert_debug(context != null, message_context_not_initialized);
    kassert_debug(block != null, "The 'memory_free' function requires a non-null memory pointer.");

    memory_header* header = (memory_header*)((u8*)block - sizeof(memory_header));

    if(header->check != CHECK_NUMBER)
    {
        kfatal("Function 'memory_free' could not check the header number, or the memory pointer is invalid!");
    }

    context->tagged_allocated[header->tag] -= header->size;
    context->total_allocated -= header->size;

    platform_memory_free(header);
}

void memory_zero(void* block, u64 size)
{
    platform_memory_zero(block, size);
}

void memory_set(void* block, u64 size, i32 value)
{
    platform_memory_set(block, size, value);
}

void memory_copy(void* dest, const void* src, u64 size)
{
    platform_memory_copy(dest, src, size);
}

void memory_move(void* dest, const void* src, u64 size)
{
    platform_memory_move(dest, src, size);
}
