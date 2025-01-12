#pragma once

#include <defines.h>

// Уставки по умолчанию.
#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR    2

/*
    @brief Создает динамический массив.
    @param stride Размер элемента массива.
    @param capacity Резервируемое количество элементов массива.
    @return В случае успеха - указатель на массив, иначе null.
*/
KAPI void* dynamic_array_create(u64 stride, u64 capacity);

/*
    @brief Изменяет размер динамического массива.
    NOTE: Изменяется указатель на массив, указатель на старый становится недействительным.
    @param array Указатель на старый массив.
    @param capacity Новое количество резервируемых элементов в массиве.
    @return Возвращает старый или новый указатель на массив.
*/
KAPI void* dynamic_array_resize(void* array, u64 capacity);

/*
    @brief Удаляет динамический массив.
    NOTE: Указатель необходимо обнулить самостоятельно!
    @param array Указатель на массив.
*/
KAPI void dynamic_array_destroy(void* array);

/*
    @brief Добавляет элемент в массив.
    NOTE: Изменяется указатель на массив.
    @param array Указатель на массив.
    @param element Указатель на элемент который нужно добавить.
    @return Возвращает старый или новый указатель на массив.
*/
KAPI void* dynamic_array_push(void* array, const void* element);

/*
    @brief Вставляет элемент в заданную позицию массива.
    NOTE: Изменяется указатель на массив.
    @param array Указатель на массив.
    @param index Индекс позиции элемента, может быть от 0 до текущей длинны массива.
    @param element Указатель на элемент который нужно вставить.
    @return Возвращает старый или новый указатель на массив.
*/
KAPI void* dynamic_array_insert_at(void* array, u64 index, const void* element);

/*
    @brief Извлекает последний элемент из массива.
    @param array Указатель на массив.
    @param dest Указатель на память куда скопировать извлекаемый элемент, может быть null.
*/
KAPI void dynamic_array_pop(void* array, void* dest);

/*
    @brief Извлекает заданный позицией элемент из массива.
    @param array Указатель на массив.
    @param index Индекс позиции элемента, может быть от 0 до текущей длинны массива.
    @param dest Указатель на память куда скопировать извлекаемый элемент, может быть null.
*/
KAPI void dynamic_array_pop_at(void* array, u64 index, void* dest);

/*
    @brief Удаляет элементы массива.
    NOTE: Не стирает данные.
    @param array Указатель на массив.
*/
KAPI void dynamic_array_clear(void* array);

/*
    @brief Получает текущее количество элементов массива.
    @param array Указатель на массив.
    @return Текущее количество элементов.
*/
KAPI u64 dynamic_array_get_length(void* array);

/*
    @brief Получает зарезервированное количество элементов массив.
    @param array Указатель на массив.
    @return Зарезервированное количество элементов.
*/
KAPI u64 dynamic_array_get_capacity(void* array);

/*
    @brief Размер элемента массива.
    @param array Указатель на массив.
    @return Размер элемента.
*/
KAPI u64 dynamic_array_get_stride(void* array);

/*
    @brief Создает динамический массив.
    @param type Тип элемента массива.
    @return В случае успеха - указатель на массив, иначе null.
*/
#define darray_create(type) (type*)dynamic_array_create(sizeof(type), DARRAY_DEFAULT_CAPACITY)

/*
    @brief Создает динамический массив c резервированием размера.
    @param type Тип элемента массива.
    @param capacity Резервируемое количество элементов массива.
    @return В случае успеха - указатель на массив, иначе null.
*/
#define darray_reserve(type, capacity) (type*)dynamic_array_create(sizeof(type), capacity)

/*
    @brief Изменяет размер динамического массива.
    NOTE: Изменяется указатель на массив.
    @param array Указатель на старый массив.
    @param capacity Новое количество резервируемых элементов в массиве.
    @return Возвращает старый или новый указатель на массив.
*/
#define darray_resize(old_array, capacity) dynamic_array_resize(old_array, capacity)

/*
    @brief Удаляет динамический массив.
    NOTE: Указатель необходимо обнулить самостоятельно!
    @param array Указатель на массив.
*/
#define darray_destroy(array) dynamic_array_destroy(array)

/*
    @brief Добавляет элемент в массив.
    NOTE: Изменяется указатель на массив.
    @param array Указатель на массив.
    @param element Указатель на элемент который нужно добавить.
*/
#define darray_push(array, element)           \
{                                             \
    typeof(element) temp = element;           \
    array = dynamic_array_push(array, &temp); \
}

/*
    @brief Вставляет элемент в заданную позицию массива.
    NOTE: Изменяется указатель на массив.
    @param array Указатель на массив.
    @param index Индекс позиции элемента, может быть от 0 до текущей длинны массива.
    @param element Указатель на элемент который нужно вставить.
*/
#define darray_insert_at(array, index, element)           \
{                                                         \
    typeof(element) temp = element;                       \
    array = dynamic_array_insert_at(array, index, &temp); \
}

/*
    @brief Извлекает последний элемент из массива.
    @param array Указатель на массив.
    @param dest Указатель на память куда скопировать извлекаемый элемент, может быть null.
*/
#define darray_pop(array, dest) dynamic_array_pop(array, dest)

/*
    @brief Извлекает заданный позицией элемент из массива.
    @param array Указатель на массив.
    @param index Индекс позиции элемента, может быть от 0 до текущей длинны массива.
    @param dest Указатель на память куда скопировать извлекаемый элемент, может быть null.
*/
#define darray_pop_at(array, index, dest) dynamic_array_pop_at(array, index, dest)

/*
    @brief Удаляет элементы массива.
    NOTE: Не стирает данные.
    @param array Указатель на массив.
*/
#define darray_clear(array) dynamic_array_clear(array)

/*
    @brief Получает текущее количество элементов массива.
    @param array Указатель на массив.
    @return Текущее количество элементов.
*/
#define darray_get_length(array) dynamic_array_get_length(array)

/*
    @brief Получает зарезервированное количество элементов массив.
    @param array Указатель на массив.
    @return Зарезервированное количество элементов.
*/
#define darray_get_capacity(array) dynamic_array_get_capacity(array)

/*
    @brief Размер элемента массива.
    @param array Указатель на массив.
    @return Размер элемента.
*/
#define darray_get_stride(array) dynamic_array_get_stride(array)
