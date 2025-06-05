#pragma once

#include <defines.h>
#include <platform/memory.h>

// @brief Маркеры участков памяти.
typedef enum memory_tag {
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_SYSTEM,
    MEMORY_TAG_FILE,
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_HASHTABLE,
    MEMORY_TAG_ALLOCATOR,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_STRING,
    MEMORY_TAG_APPLICATION,
    MEMORY_TAG_JOB,
    MEMORY_TAG_RESOURCE,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_MATERIAL,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_NODE,
    MEMORY_TAG_VULKAN,
    MEMORY_TAG_VULKAN_EXT,
    MEMORY_TAG_GPU_LOCAL,
    MEMORY_TAGS_MAX
} memory_tag;

// @brief Конфигурация системы памяти.
typedef struct memory_system_config {
    // @brief Общий размер памяти в байтах, используемый системой для разпределения.
    ptr total_allocation_size;
} memory_system_config;

/*
    @brief Запускает систему менеджмента и контроля памяти.
    @param config Указатель на конфигурацию системы памяти.
    @return True завершилась успешно, false если не удалось.
*/
KAPI bool memory_system_initialize(memory_system_config* config);

/*
    @brief Останавливает систему менеджмента и контроля памяти.
    @note  Если по окончании работы осталась не освобожденная памяти, то система
           уведомит об этом в логах.
*/
KAPI void memory_system_shutdown();

/*
    @brief Запрашивает данные об использовании памяти в виде строки.
    @note  После использования удалить строку с помощью функции 'string_free'.
    @return Данные об использовании памяти в виде строки.
*/
KAPI const char* memory_system_usage_str();

/*
    @brief Запрашивает количество операций выделения памяти.
    @return Количество операций выделения памяти.
*/
KAPI ptr memory_system_allocation_count();

/*
    @brief Запрашивает у системы память с заданными размером и выравниванием.
    @note  В процессе память не обнуляется!
    @param size Количество байт памяти.
    @param alignment Значение границы выравнивания.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
KAPI void* memory_allocate(ptr size, u16 alignment, memory_tag tag);

/*
    @brief Запрашивает память у системы, без выделения памяти.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
*/
KAPI void memory_allocate_report(ptr size, memory_tag tag);

/*
    @brief Возвращает предоставленную память системе. 
    @note  Указатель необходимо обнулить самостоятельно!
    @param block Указатель на память.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
*/
KAPI void memory_free(void* block, ptr size, memory_tag tag);

/*
    @brief Возвращает память системе, без освобождения реальной памяти. 
    @param size Количество байт памяти.
    @param tag Маркер памяти.
*/
KAPI void memory_free_report(ptr size, memory_tag tag);

/*
    @brief Пытается получить размер указаного блока памяти.
    @param block Указатель на блок памяти, размер которого необходимо получить.
    @param out_size Указатель на переменую для сохранения размера памяти в байтах.
    @retunr True в случае успеха, в противном случае false с выводом сообщения в логи.
*/
KAPI bool memory_get_size(void* block, ptr* out_size);

/*
    @brief Пытается получить кратность выравнивания указаного блока памяти.
    @param block Указатель на блок памяти, кратность выравнивания которого необходимо получить.
    @param out_alignment Указатель на переменую для сохранения кратности выравнивания блока памяти.
    @retunr True в случае успеха, в противном случае false с выводом сообщения в логи.
*/
KAPI bool memory_get_alignment(void* block, u16* out_alignment);

/*
    @brief Запрашивает память у системы без учета выравнивания.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kallocate(size, tag) memory_allocate(size, 1, tag)

/*
    @brief Запрашивает память у системы без учета выравнивания.
    @param type Тип элемента.
    @param count Количество элементов.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kallocate_tc(type, count, tag) (type*)memory_allocate(sizeof(type) * count, 1, tag)

/*
    @brief Запрашивает память у системы c учетом выравнивания.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kallocate_aligned(size, alignment, tag) memory_allocate(size, alignment, tag)

/*
    @brief Запрашивает память у системы без учета выравнивания.
    @param type Тип элемента.
    @param count Количество элементов.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kallocate_aligned_tc(type, count, alignment, tag) (type*)memory_allocate(sizeof(type) * count, alignment, tag)

/*
    @brief Запрашивает память у системы, но не выделяет ее.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kallocate_report(size, tag) memory_allocate_report(size, tag)

/*
    @brief Запрашивает память у системы, но не выделяет ее.
    @param type Тип элемента.
    @param count Количество элементов.
    @param tag Маркер памяти.
    @return Указатель на запрашиваемый участок памяти.
*/
#define kallocate_report_tc(type, count, tag) memory_allocate_report(sizeof(type) * count, tag)

/*
    @brief Возвращает память системе.
    @note  Указатель необходимо обнулить самостоятельно!
    @param block Указатель на память.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
*/
#define kfree(block, size, tag) memory_free((void*)block, size, tag)

/*
    @brief Возвращает память системе.
    @note  Указатель необходимо обнулить самостоятельно!
    @param block Указатель на память.
    @param type Тип элемента.
    @param count Количество элементов.
    @param tag Маркер памяти.
*/
#define kfree_tc(block, type, count, tag) memory_free((void*)block, sizeof(type) * count, tag)

/*
    @brief Возвращает память системе, без реального освобождения.
    @param size Количество байт памяти.
    @param tag Маркер памяти.
*/
#define kfree_report(size, tag) memory_free_report(size, tag)

/*
    @brief Возвращает память системе, без реального освобождения.
    @param type Тип элемента.
    @param count Количество элементов.
    @param tag Маркер памяти.
*/
#define kfree_report_tc(type, count, tag) memory_free_report(sizeof(type) * count, tag)

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
#define kset_tc(block, type, count, value) platform_memory_set((void*)block, sizeof(type) * count, value)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    @note  Адреса участков памяти не должны пересекаться!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param size Количествой байт которое необходимо скопировать.
*/
#define kcopy(dest, src, size) platform_memory_copy((void*)dest, (void*)src, size)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    @note  Адреса участков памяти не должны пересекаться!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param type Тип элемента участка памяти.
    @param count Количество элементов заданного типа.
*/
#define kcopy_tc(dest, src, type, count) platform_memory_copy((void*)dest, (void*)src, sizeof(type) * count)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    @note  Адреса участков памяти могут пересекаться, но операция выполняется дольше!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param size Количествой байт которое необходимо скопировать.
*/
#define kmove(dest, src, size) platform_memory_move((void*)dest, (void*)src, size)

/*
    @brief Копирует заданное количество байт из одного участка памяти в другой.
    @note  Адреса участков памяти могут пересекаться, но операция выполняется дольше!
    @param dest Указатель на участок памяти куда копировать.
    @param src Указатель на участок памяти откуда копировать.
    @param type Тип элемента участка памяти.
    @param count Количество элементов заданного типа.
*/
#define kmove_tc(dest, src, type, count) platform_memory_move((void*)dest, (void*)src, sizeof(type) * count)
