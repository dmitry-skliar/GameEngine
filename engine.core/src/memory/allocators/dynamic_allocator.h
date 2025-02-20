#pragma once

#include <defines.h>

// @brief Контекст динамического распределителя памяти.
typedef struct dynamic_allocator dynamic_allocator;

/*
*/
KAPI dynamic_allocator* dynamic_allocator_create(u64 total_size, u64* memory_requirement, void* memory);

/*
*/
KAPI void dynamic_allocator_destroy(dynamic_allocator* allocator);

/*
*/
KAPI void* dynamic_allocator_allocate(dynamic_allocator* allocator, u64 size);

/*
*/
KAPI bool dynamic_allocator_free(dynamic_allocator* allocator, void* block);

/*
*/
KAPI u64 dynamic_allocator_free_space(dynamic_allocator* allocator);

/*
*/
KAPI u64 dynamic_allocator_free_blocks(dynamic_allocator* allocator);

// TODO: Для наблюдения за фрагментацией памяти, реализовать функцию, которая
// выдеат структуру с информацией по памяти:
//       - Весь размер памяти
//       - Смещения участков свободных относительно нуля и размер каждого.
