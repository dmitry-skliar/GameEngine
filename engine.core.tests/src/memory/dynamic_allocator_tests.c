#include "memory/dynamic_allocator_tests.h"
#include "test_manager.h"
#include "expect.h"

#include <memory/allocators/dynamic_allocator.h>
#include <memory/memory.h>
#include <debug/assert.h>

u8 test1()
{
    u64 total_size = KIBIBYTES(1);
    u64 memory_requirement = 0;
    
    dynamic_allocator* dalloc = dynamic_allocator_create(total_size, &memory_requirement, null);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_be(null, dalloc);

    void* memory = kallocate(memory_requirement, MEMORY_TAG_ALLOCATOR);
    dalloc = dynamic_allocator_create(total_size, &memory_requirement, memory);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_not_be(null, dalloc);
    // Начало зоны тестов!

    u64 free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    u64 free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    // Конец зоны тестов!
    dynamic_allocator_destroy(dalloc);
    kfree(memory, memory_requirement, MEMORY_TAG_ALLOCATOR);
    return true;
}

u8 test2()
{
    u64 total_size = KIBIBYTES(1);
    u64 memory_requirement = 0;
    
    dynamic_allocator* dalloc = dynamic_allocator_create(total_size, &memory_requirement, null);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_be(null, dalloc);

    void* memory = kallocate(memory_requirement, MEMORY_TAG_ALLOCATOR);
    dalloc = dynamic_allocator_create(total_size, &memory_requirement, memory);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_not_be(null, dalloc);
    // Начало зоны тестов!

    u64 free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    u64 free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    void* block = dynamic_allocator_allocate(dalloc, total_size);
    expect_pointer_should_not_be(null, block);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(0, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(0, free_blocks);

    bool result = dynamic_allocator_free(dalloc, block);
    expect_to_be_true(result);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    // Конец зоны тестов!
    dynamic_allocator_destroy(dalloc);
    kfree(memory, memory_requirement, MEMORY_TAG_ALLOCATOR);
    return true;
}

u8 test3()
{
    // Т.к. на каждый выделенный блок дополнительно использует
    // sizeof(u64) = 8 байт.  
    u64 total_size = 1040;
    u64 memory_requirement = 0;
    
    dynamic_allocator* dalloc = dynamic_allocator_create(total_size, &memory_requirement, null);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_be(null, dalloc);

    void* memory = kallocate(memory_requirement, MEMORY_TAG_ALLOCATOR);
    dalloc = dynamic_allocator_create(total_size, &memory_requirement, memory);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_not_be(null, dalloc);
    // Начало зоны тестов!

    u64 free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    u64 free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    void* block = dynamic_allocator_allocate(dalloc, total_size);
    expect_pointer_should_not_be(null, block);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(0, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(0, free_blocks);

    bool result = dynamic_allocator_free(dalloc, block);
    expect_to_be_true(result);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    #define ALLOC_COUNT 3
    u64 alloc_size = 0;
    u64 sizes[ALLOC_COUNT] = {256, 512, 256};
    void* blocks[ALLOC_COUNT] = {0};

    void* expect_block = block;

    for(u64 i = 0; i < ALLOC_COUNT; ++i)
    {
        blocks[i] = dynamic_allocator_allocate(dalloc, sizes[i]);
        expect_pointer_should_not_be(null, blocks[i]);
        expect_pointer_should_be(expect_block, blocks[i]);

        alloc_size += sizes[i] + sizeof(u64);
        expect_block = (void*)blocks[i] + sizes[i] + sizeof(u64);

        if(i < ALLOC_COUNT - 1)
        {
            free_space = dynamic_allocator_free_space(dalloc);
            expect_should_be(total_size - alloc_size, free_space);
            free_blocks = dynamic_allocator_free_blocks(dalloc);
            expect_should_be(1, free_blocks);
        }
    }

    // Так как вся память уже выделена!
    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(0, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(0, free_blocks);

    u64 free_size = 0;

    // Освобождение последнего. На этом этапе появится один блок.
    result = dynamic_allocator_free(dalloc, blocks[2]);
    expect_to_be_true(result);

    free_size += sizes[2];
    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(free_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    // Освобождение первого блока. На этом этапе появится второй блок, но объединения не произойдет.
    result = dynamic_allocator_free(dalloc, blocks[0]);
    expect_to_be_true(result);

    free_size += sizes[0];
    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(free_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(2, free_blocks);

    // kdebug_break();

    // Освобождение среднего блока. На этом этапе должно произойти 2 объединения.
    result = dynamic_allocator_free(dalloc, blocks[1]);
    expect_to_be_true(result);

    free_size += sizes[1] + 2 * sizeof(u64); // Так как останется только 1 блок.
    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(free_size, free_space);
    expect_should_be(total_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    // Что бы убедиться что блок объеденен, выделить и освободить всю память!
    block = dynamic_allocator_allocate(dalloc, total_size);
    expect_pointer_should_not_be(null, block);
    expect_pointer_should_be(blocks[0], block);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(0, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(0, free_blocks);

    result = dynamic_allocator_free(dalloc, block);
    expect_to_be_true(result);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    // Конец зоны тестов!
    dynamic_allocator_destroy(dalloc);
    kfree(memory, memory_requirement, MEMORY_TAG_ALLOCATOR);
    return true;
}

u8 test4()
{
    u64 total_size = 1040;
    u64 memory_requirement = 0;
    
    dynamic_allocator* dalloc = dynamic_allocator_create(total_size, &memory_requirement, null);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_be(null, dalloc);

    void* memory = kallocate(memory_requirement, MEMORY_TAG_ALLOCATOR);
    dalloc = dynamic_allocator_create(total_size, &memory_requirement, memory);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_not_be(null, dalloc);
    // Начало зоны тестов!

    u64 free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    u64 free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    // Так как памяти для разделения блока на два не хватает, то будет выделен весь блок.
    void* block = dynamic_allocator_allocate(dalloc, 1030); // total_size
    expect_pointer_should_not_be(null, block);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(0, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(0, free_blocks);

    kdebug("Note: The following 2 warning messages are intentionally caused by this test.");
    void* block_null = dynamic_allocator_allocate(dalloc, 1);
    expect_pointer_should_be(null, block_null);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(0, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(0, free_blocks);
    
    bool result = dynamic_allocator_free(dalloc, block);
    expect_to_be_true(result);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    // Конец зоны тестов!
    dynamic_allocator_destroy(dalloc);
    kfree(memory, memory_requirement, MEMORY_TAG_ALLOCATOR);
    return true;
}

u8 test5()
{
    u64 total_size = 1040;
    u64 memory_requirement = 0;
    
    dynamic_allocator* dalloc = dynamic_allocator_create(total_size, &memory_requirement, null);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_be(null, dalloc);

    void* memory = kallocate(memory_requirement, MEMORY_TAG_ALLOCATOR);
    dalloc = dynamic_allocator_create(total_size, &memory_requirement, memory);
    expect_should_not_be(0, memory_requirement);
    expect_pointer_should_not_be(null, dalloc);
    // Начало зоны тестов!

    u64 free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    u64 free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    kdebug("Note: The following 2 warning messages are intentionally caused by this test.");
    // Попытка получить памяти больше чем есть вообще.
    void* block = dynamic_allocator_allocate(dalloc, total_size * 2);
    expect_pointer_should_be(null, block);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    block = dynamic_allocator_allocate(dalloc, total_size);
    dynamic_allocator_free(dalloc, block);


    #define ALLOC_COUNT 3
    u64 alloc_size = 0;
    u64 sizes[ALLOC_COUNT] = {256, 512, 128};
    void* blocks[ALLOC_COUNT] = {0};

    void* expect_block = block;

    for(u64 i = 0; i < ALLOC_COUNT; ++i)
    {
        blocks[i] = dynamic_allocator_allocate(dalloc, sizes[i]);
        expect_pointer_should_not_be(null, blocks[i]);
        expect_pointer_should_be(expect_block, blocks[i]);

        alloc_size += sizes[i] + sizeof(u64);
        expect_block = (void*)blocks[i] + sizes[i] + sizeof(u64);

        free_space = dynamic_allocator_free_space(dalloc);
        expect_should_be(total_size - alloc_size, free_space);
        free_blocks = dynamic_allocator_free_blocks(dalloc);
        expect_should_be(1, free_blocks);
    }

    block = dynamic_allocator_allocate(dalloc, 256);
    expect_pointer_should_be(null, block);

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size - alloc_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    bool result = false;

    for(u64 i = 0; i < ALLOC_COUNT; ++i)
    {
        result = dynamic_allocator_free(dalloc, blocks[i]);
        expect_to_be_true(result);
    }

    free_space = dynamic_allocator_free_space(dalloc);
    expect_should_be(total_size, free_space);
    free_blocks = dynamic_allocator_free_blocks(dalloc);
    expect_should_be(1, free_blocks);

    // Конец зоны тестов!
    dynamic_allocator_destroy(dalloc);
    kfree(memory, memory_requirement, MEMORY_TAG_ALLOCATOR);
    return true;
}

void dynamic_allocator_register_tests()
{
    test_managet_register_test(test1, "Dynamic allocator should create and destroy.");
    test_managet_register_test(test2, "Dynamic allocator single alloc for all space.");
    test_managet_register_test(test3, "Dynamic allocator multi alloc for all space.");
    test_managet_register_test(test4, "Dynamic allocator try over allocate.");
    test_managet_register_test(test5, "Dynamic allocator should try to over allocate with not enough space, but not 0 space remaining.");
}


