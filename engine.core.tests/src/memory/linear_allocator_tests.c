#include "linear_allocator_tests.h"
#include "test_manager.h"
#include "expect.h"

#include <memory/allocators/linear_allocator.h>

u8 linear_allocator_test1()
{
    linear_allocator* allocator = linear_allocator_create(sizeof(u64));
    expect_pointer_should_not_be(null, allocator);

    linear_allocator_destroy(allocator);
    expect_pointer_should_not_be(null, allocator);

    return true;
}

u8 linear_allocator_test2()
{
    const u64 max_size = 1024;
    linear_allocator* allocator = linear_allocator_create(max_size * sizeof(u64));
    expect_pointer_should_not_be(null, allocator);

    void* block = linear_allocator_allocate(allocator, max_size * sizeof(u64));
    expect_pointer_should_not_be(null, allocator);
    expect_pointer_should_not_be(null, block);
    expect_pointer_should_not_be(allocator, block);
    expect_to_be_true(block > (void*)allocator);

    linear_allocator_free_all(allocator);
    expect_pointer_should_not_be(null, allocator);
    expect_pointer_should_not_be(null, block);
    expect_pointer_should_not_be(allocator, block);

    linear_allocator_destroy(allocator);
    expect_pointer_should_not_be(null, allocator);
    expect_pointer_should_not_be(null, block);
    expect_pointer_should_not_be(allocator, block);

    return true;
}

u8 linear_allocator_test3()
{
    u64 max_size = 1024;
    linear_allocator* allocator = linear_allocator_create(max_size * sizeof(u64));

    void* block = null;
    void* last  = allocator;
    for(u64 i = 0; i < max_size; ++i)
    {
        block = linear_allocator_allocate(allocator, sizeof(u64));
        expect_pointer_should_not_be(null, allocator);
        expect_pointer_should_not_be(null, block);
        expect_pointer_should_not_be(allocator, block);
        expect_pointer_should_not_be(last, block);
        expect_to_be_true(block > last);

        if(last != allocator)
        {
            u64 diff = (u64)block - (u64)last;
            expect_should_be(sizeof(u64), diff);
        }
        last = block;
    }

    linear_allocator_free_all(allocator);
    expect_pointer_should_not_be(null, allocator);
    expect_pointer_should_not_be(null, block);
    expect_pointer_should_not_be(allocator, block);

    linear_allocator_destroy(allocator);
    expect_pointer_should_not_be(null, allocator);
    expect_pointer_should_not_be(null, block);
    expect_pointer_should_not_be(allocator, block);

    return true;
}

u8 linear_allocator_test4()
{
    u64 memory_size      = 20 * sizeof(u64);
    u64 element_size     = 3  * sizeof(u64);
    u64 element_allocate = 6;

    linear_allocator* allocator = linear_allocator_create(memory_size);

    void* block = null;
    void* last  = allocator;
    for(u64 i = 0; i < element_allocate; ++i)
    {
        block = linear_allocator_allocate(allocator, element_size);
        expect_pointer_should_not_be(null, allocator);
        expect_pointer_should_not_be(null, block);
        expect_pointer_should_not_be(allocator, block);
        expect_pointer_should_not_be(last, block);
        expect_to_be_true(block > last);

        if(last != allocator)
        {
            u64 diff = (u64)block - (u64)last;
            expect_should_be(element_size, diff);
        }
        last = block;
    }

    kdebug("Note: The following error is intentionally caused by this test.");

    // Запрос очередного выделения, которое должно вернуть ошибку в консоль и вернуть null.
    block = linear_allocator_allocate(allocator, element_size);
    expect_pointer_should_be(null, block);

    linear_allocator_free_all(allocator);
    expect_pointer_should_not_be(null, allocator);
    expect_pointer_should_not_be(allocator, block);

    linear_allocator_destroy(allocator);

    return true;
}

u8 linear_allocator_test5()
{
    u64 max_size = 1024;
    linear_allocator* allocator = linear_allocator_create(max_size * sizeof(u64));

    void* block = null;
    void* last  = allocator;
    for(u64 i = 0; i < max_size; ++i)
    {
        block = linear_allocator_allocate(allocator, sizeof(u64));
        expect_pointer_should_not_be(null, allocator);
        expect_pointer_should_not_be(null, block);
        expect_pointer_should_not_be(allocator, block);
        expect_pointer_should_not_be(last, block);
        expect_to_be_true(block > last);

        if(last != allocator)
        {
            u64 diff = (u64)block - (u64)last;
            expect_should_be(sizeof(u64), diff);
        }
        last = block;
    }

    linear_allocator_free_all(allocator);
    expect_pointer_should_not_be(null, allocator);
    expect_pointer_should_not_be(allocator, block);

    block = null;
    for(u64 i = 0; i < max_size; ++i)
    {
        block = linear_allocator_allocate(allocator, sizeof(u64));
        expect_pointer_should_not_be(null, last);
        expect_pointer_should_not_be(null, block);

        // На последней итерации last == block!
        if(i < max_size - 1)
        {
            expect_to_be_true(last > block);
            expect_pointer_should_not_be(last, block);
        }
    }

    expect_pointer_should_be(last, block);
    linear_allocator_destroy(allocator);

    return true;
}

void linear_allocator_register_tests()
{
    test_managet_register_test(
        linear_allocator_test1, "Linear allocator should create and destroy successfully."
    );

    test_managet_register_test(
        linear_allocator_test2, "Linear allocator should single allocate for all space successfully."
    );

    test_managet_register_test(
        linear_allocator_test3, "Linear allocator should multi allocate for all space successfully."
    );

    test_managet_register_test(
        linear_allocator_test4, "Linear allocator should try over allocate successfully."
    );

    test_managet_register_test(
        linear_allocator_test5, "Lineat allocator allocated should not be NULL after 'free all' successfully."
    );
}
