#pragma once

#include <defines.h>

// @brief Контекст линейного распределителя памяти.
typedef struct linear_allocator linear_allocator;

/*
    @brief Создает линейный распределитель памяти.
    @param size Размер внутреннего буфера памяти.
    @return Указатель на экземпляр линейного распределителя памяти.
*/
KAPI linear_allocator* linear_allocator_create(u64 size);

/*
    @brief Уничтожает линейный распределитель памяти.
    NOTE: После уничтожения, обнулять указатель на экземпляр распределителя памяти!
    @param allocator Указатель на экземпляр линейного распределителя памяти.
*/
KAPI void linear_allocator_destroy(linear_allocator* allocator);

/*
    @brief Выделяет память из своего внутреннего буфера.
    NOTE: Обнулить полученную память, если это необходимо. 
    @param allocator Указатель на экземпляр линейного распределителя памяти.
    @param size Необходимое количество памяти в байтах.
    @return Указатель на необходимое количество памяти, null если запрашиваемая память отсутствует.
*/
KAPI void* linear_allocator_allocate(linear_allocator* allocator, u64 size);

/*
    @brief Возвращает всю выделенную ранее память в внутренний буфер.
    NOTE: Перед освобождением проверь, что уничтожены все указатели полученые ранее, т.к. память
          на самом деле не освобождается и остается доступна, что может вызвать потерю данных и
          коллизии. Так же память при этом не обнуляется, однако ее можно использовать повторно.
    @param allocator Указатель на экземпляр линейного распределителя памяти.
*/
KAPI void linear_allocator_free_all(linear_allocator* allocator);

