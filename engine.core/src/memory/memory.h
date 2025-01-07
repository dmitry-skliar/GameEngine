#pragma once

#include <defines.h>

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
    @brief Инициализирует и запускает систему менеджмента и контроля памяти.
*/
KAPI void memory_system_initialize();

/*
    @brief Завершение и останавливает систему менеджмента и контроля памяти.
*/
KAPI void memory_system_shutdown();

/*
    @brief Получить данные об использовании памяти в виде строки.
    INFO: После использования удалить строку с помощью функции 'string_free'.
    @return Данные об использовании памяти в виде строки.
*/
KAPI const char* memory_system_usage_get();

/*
    @brief Запрашивает память у системы заданного размера и помечает маркером для анализа.
    @param size Количество байт памяти которую необходимо получить.
    @param tag Маркер которым нужно пометить запрашиваемую память.
    @return Указатель на запрашиваемый участок памяти.
*/
KAPI void* memory_allocate(u64 size, memory_tag tag);

/*
    @brief Возвращает память системе заданный указателем. 
    INFO: Применять к указателю, который был выделен функцией 'memory_allocate'.
    А так же указатель необходимо обнулить самостоятельно!
    @param block Указатель на память, которую нужно вернуть системе.
*/
KAPI void memory_free(void* block);

/*
    @brief Обнуляет байты указанного участа памяти заданного размера.
    @param block Указатель на участок памяти для обнуления.
    @param size Количество байт памяти для обнуления.
*/
KAPI void memory_zero(void* block, u64 size);

/*
    @brief Заполняет байты указаного участка памяти заданного размером и значением.
    @param block Указатель на участок памяти для заполнение значением.
    @param size Количество байт памяти для заполнения значением.
    @param value Значение, которым нужно наполнить память.
*/
KAPI void memory_set(void* block, u64 size, i32 value);

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    INFO: Адреса участков памяти не должны пересекаться!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param size Количествой байт которое необходимо скопировать.
*/
KAPI void memory_copy(void* dest, const void* src, u64 size);

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    INFO: Адреса участков памяти могут пересекаться, но операция выполняется дольше!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param size Количествой байт которое необходимо скопировать.
*/
KAPI void memory_move(void* dest, const void* src, u64 size);

/*
    @brief Получить размер участка памяти.
    INFO: Применять к указателю, который был выделен функцией 'memory_allocate'.
    @param block Указатель на память.
    @return Размер памяти.
*/
KAPI u64 memory_size_get(void* block);

/*
    @brief Получить метку участка памяти.
    @param block Указатель на память.
    @return Метка памяти.
*/
KAPI memory_tag memory_tag_get(void* block);

/*
    @brief Запрашивает память у системы заданного размера и помечает маркером для анализа.
    @param size Количество байт памяти которую необходимо получить.
    @param tag Маркер которым нужно пометить запрашиваемую память.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kmallocate(size, tag) memory_allocate(size, tag)

/*
    @brief Запрашивает память у системы заданного типом элемента и помечает маркером.
    @param type Тип элемента участка памяти.
    @param tag Маркер которым нужно пометить запрашиваемую память.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kmallocate_t(type, tag) (type*)memory_allocate(sizeof(type), tag)

/*
    @brief Запрашивает память у системы заданного типом элемента, его ыколичеством и помечает маркером.
    @param type Тип элемента участка памяти.
    @param count Количество элементов заданного типа.
    @param tag Маркер которым нужно пометить запрашиваемую память.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kmallocate_tc(type, count, tag) (type*)memory_allocate(sizeof(size) * count, tag)

/*
    @brief Возвращает память системе заданный указателем. 
    INFO: Применять к указателю, который был выделен функцией 'memory_allocate' 
    или его псевдонимами 'kmallocate*'. А так же указатель необходимо обнулить самостоятельно!
    @param block Указатель на память, которую нужно вернуть системе.
*/
#define kmfree(block) memory_free((void*)block)

/*
    @brief Обнуляет байты указанного участа памяти заданного размера.
    @param block Указатель на участок памяти для обнуления.
    @param size Количество байт памяти для обнуления.
*/
#define kmzero(block, size) memory_zero((void*)block, size)

/*
    @brief Обнуляет байты указанного участа памяти заданного типом элемента и его количеством.
    @param block Указатель на участок памяти для обнуления.
    @param type Тип элемента участка памяти.
    @param count Количество элементов заданного типа.
*/
#define kmzero_tc(block, type, count) memory_zero((void*)block, sizeof(type) * count)

/*
    @brief Заполняет байты указаного участка памяти заданного размером и значением.
    @param block Указатель на участок памяти для заполнение значением.
    @param size Количество байт памяти для заполнения значением.
    @param value Значение, которым нужно наполнить память.
*/
#define kmset(block, size, value) memory_set((void*)block, size, value)

/*
    @brief Заполняет байты указаного участка памяти заданного типом элемента, его количеством и значением.
    @param block Указатель на участок памяти для заполнение значением.
    @param type Тип элемента участка памяти.
    @param count Количество элементов заданного типа.
    @param value Значение, которым нужно наполнить память.
*/
#define kmset_tc(block, type, count, value) memory_set((void*)block, sizeof(size) * count, value)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    INFO: Адреса участков памяти не должны пересекаться!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param size Количествой байт которое необходимо скопировать.
*/
#define kmcopy(dest, src, size) memory_copy((void*)dest, (void*)src, size)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    INFO: Адреса участков памяти не должны пересекаться!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param type Тип элемента участка памяти.
    @param count Количество элементов заданного типа.
*/
#define kmcopy_tc(dest, src, type, count) memory_copy((void*)dest, (void*)src, sizeof(type) * count)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    INFO: Адреса участков памяти могут пересекаться, но операция выполняется дольше!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param size Количествой байт которое необходимо скопировать.
*/
#define kmmove(dest, src, size) memory_move((void*)dest, (void*)src, size)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    INFO: Адреса участков памяти могут пересекаться, но операция выполняется дольше!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param type Тип элемента участка памяти.
    @param count Количество элементов заданного типа.
*/
#define kmmove_tc(dest, src, type, count) memory_move((void*)dest, (void*)src, sizeof(type) * count)
