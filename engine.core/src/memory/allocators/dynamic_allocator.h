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
KAPI bool dynamic_allocator_free(dynamic_allocator* allocator, void* block, u64 size);

/*
*/
KAPI u64 dynamic_allocator_free_space(dynamic_allocator* allocator);

/*
*/
KAPI u64 dynamic_allocator_block_count(dynamic_allocator* allocator);
