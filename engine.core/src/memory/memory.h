#pragma once

#include <defines.h>
#include <platform/memory.h>

// @brief Маркеры участков памяти.
typedef enum memory_tag {
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_SYSTEM,
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_HASHTABLE,
    MEMORY_TAG_LINEAR_ALLOCATOR,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_STRING,
    MEMORY_TAG_APPLICATION,
    MEMORY_TAG_JOB,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_MATERIAL,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_NODE,
    MEMORY_TAGS_MAX
} memory_tag;

/*
    @brief Запускает систему менеджмента и контроля памяти.
    @param memory_requirement Указатель на переменную для получения требований к памяти.
    @param memory Указатель на выделенную память, для получения требований к памяти передать null.
*/
KAPI void memory_system_initialize(u64* memory_requirement, void* memory);

/*
    @brief Останавливает систему менеджмента и контроля памяти.
*/
KAPI void memory_system_shutdown();

/*
    @brief Запрашивает данные об использовании памяти в виде строки.
    NOTE: После использования удалить строку с помощью функции 'string_free'.
    @return Данные об использовании памяти в виде строки.
*/
KAPI const char* memory_system_usage_str();

/*
    @brief Запрашивает количество операций выделения памяти.
    @return Количество операций выделения памяти.
*/
KAPI u64 memory_system_alloc_count();

/*
    @brief Запрашивает память у системы.
    NOTE: Не обнуляет память!
    @param size Количество байт памяти.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
KAPI void* memory_allocate(u64 size, memory_tag tag);

/*
    @brief Возвращает память системе. 
    NOTE: Указатель необходимо обнулить самостоятельно!
    @param block Указатель на память.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
*/
KAPI void memory_free(void* block, u64 size, memory_tag tag);

/*
    @brief Запрашивает память у системы.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kallocate(size, tag) memory_allocate(size, tag)

/*
    @brief Запрашивает память у системы.
    @param type Тип элемента.
    @param count Количество элементов.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kallocate_tc(type, count, tag) (type*)memory_allocate(sizeof(type) * count, tag)

/*
    @brief Возвращает память системе.
    NOTE: Указатель необходимо обнулить самостоятельно!
    @param block Указатель на память.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
*/
#define kfree(block, size, tag) memory_free((void*)block, size, tag)

/*
    @brief Возвращает память системе.
    NOTE: Указатель необходимо обнулить самостоятельно!
    @param block Указатель на память.
    @param type Тип элемента.
    @param count Количество элементов.
    @param tag Маркер памяти.
*/
#define kfree_tc(block, type, count, tag) memory_free((void*)block, sizeof(type) * count, tag)

/*
    @brief Обнуляет байты указанного участа памяти.
    @param block Указатель на участок памяти.
    @param size Количество байт памяти.
*/
#define kzero(block, size) platform_memory_zero((void*)block, size)

/*
    @brief Обнуляет байты указанного участа памяти.
    @param block Указатель на участок памяти.
    @param type Тип элемента.
    @param count Количество элементов.
*/
#define kzero_tc(block, type, count) platform_memory_zero((void*)block, sizeof(type) * count)

/*
    @brief Заполняет байты указаного участка памяти значением.
    @param block Указатель на участок памяти.
    @param size Количество байт памяти.
    @param value Значение, которым нужно наполнить память.
*/
#define kset(block, size, value) platform_memory_set((void*)block, size, value)

/*
    @brief Заполняет байты указаного участка памяти значением.
    @param block Указатель на участок памяти.
    @param type Тип элемента.
    @param count Количество элементов.
    @param value Значение, которым нужно наполнить память.
*/
#define kset_tc(block, type, count, value) platform_memory_set((void*)block, sizeof(size) * count, value)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    NOTE: Адреса участков памяти не должны пересекаться!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param size Количествой байт которое необходимо скопировать.
*/
#define kcopy(dest, src, size) platform_memory_copy((void*)dest, (void*)src, size)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    NOTE: Адреса участков памяти не должны пересекаться!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param type Тип элемента участка памяти.
    @param count Количество элементов заданного типа.
*/
#define kcopy_tc(dest, src, type, count) platform_memory_copy((void*)dest, (void*)src, sizeof(type) * count)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    NOTE: Адреса участков памяти могут пересекаться, но операция выполняется дольше!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param size Количествой байт которое необходимо скопировать.
*/
#define kmove(dest, src, size) platform_memory_move((void*)dest, (void*)src, size)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    NOTE: Адреса участков памяти могут пересекаться, но операция выполняется дольше!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param type Тип элемента участка памяти.
    @param count Количество элементов заданного типа.
*/
#define kmove_tc(dest, src, type, count) platform_memory_move((void*)dest, (void*)src, sizeof(type) * count)
