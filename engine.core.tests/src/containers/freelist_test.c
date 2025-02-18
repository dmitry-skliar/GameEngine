#include "containers/freelist_test.h"
#include "test_manager.h"
#include "expect.h"

#include <containers/freelist.h>
#include <memory/memory.h>
#include <kstring.h>

u8 freelist_test1()
{
    u64 total_size = KIBIBYTES(1);

    freelist* list = null;  
    u64 freelist_requirement = 0;

    list = freelist_create(total_size, &freelist_requirement, null);
    expect_pointer_should_be(null, list);

    void* memory = kallocate(freelist_requirement, MEMORY_TAG_ARRAY);
    list = freelist_create(total_size, &freelist_requirement, memory);
    expect_pointer_should_not_be(null, list);
    expect_should_be(memory, list);

    u64 current_size = freelist_free_space(list);
    expect_should_be(total_size, current_size);

    u64 current_blocks = freelist_block_count(list);
    expect_should_be(1, current_blocks);

    freelist_destroy(list);
    expect_pointer_should_not_be(null, list);

    kfree(memory, freelist_requirement, MEMORY_TAG_ARRAY);
    return true;
}

u8 freelist_test2()
{
    u64 total_size = KIBIBYTES(1);

    freelist* list = null;  
    u64 freelist_requirement = 0;

    list = freelist_create(total_size, &freelist_requirement, null);
    void* memory = kallocate(freelist_requirement, MEMORY_TAG_ARRAY);
    list = freelist_create(total_size, &freelist_requirement, memory);

    u64 current_size = freelist_free_space(list);
    expect_should_be(total_size, current_size);

    u64 current_blocks = freelist_block_count(list);
    expect_should_be(1, current_blocks);
    // Start!

    bool result = false;
    u64 offset = INVALID_ID; // Для теста!
    u64 size = 128;

    result = freelist_allocate_block(list, size, &offset);
    expect_to_be_true(result);
    expect_should_be(0, offset);

    current_size = freelist_free_space(list);
    expect_should_be(total_size - size, current_size);

    current_blocks = freelist_block_count(list);
    expect_should_be(1, current_blocks);

    result = freelist_free_block(list, size, offset);
    expect_to_be_true(result);

    current_size = freelist_free_space(list);
    expect_should_be(total_size, current_size);

    current_blocks = freelist_block_count(list);
    expect_should_be(1, current_blocks);

    // Finish!
    freelist_destroy(list);
    kfree(memory, freelist_requirement, MEMORY_TAG_ARRAY);
    return true;
}

u8 freelist_test3()
{
    u64 total_size = KIBIBYTES(1);

    freelist* list = null;  
    u64 freelist_requirement = 0;

    list = freelist_create(total_size, &freelist_requirement, null);
    void* memory = kallocate(freelist_requirement, MEMORY_TAG_ARRAY);
    list = freelist_create(total_size, &freelist_requirement, memory);

    u64 current_size = freelist_free_space(list);
    expect_should_be(total_size, current_size);

    u64 current_blocks = freelist_block_count(list);
    expect_should_be(1, current_blocks);
    // Start!

    bool result = false;
    u64 offset = INVALID_ID;       // Для теста!
    u64 size = 128;                // Выделение блоками по 128!
    u64 count = total_size / size; // 8 блоков!
    u64 offset_expected = 0;

    // Выделение блоками по 128 B.
    for(u64 i = 0; i < count; ++i)
    {
        result = freelist_allocate_block(list, size, &offset);
        expect_to_be_true(result);
        expect_should_be(offset_expected, offset);
        offset_expected += size;

        current_size = freelist_free_space(list);
        expect_should_be(total_size - offset_expected, current_size);

        // На последнем шаге блоков в листе не должно быть!
        if(i < count - 1)
        {
            current_blocks = freelist_block_count(list);
            expect_should_be(1, current_blocks);
        }
        else
        {
            current_blocks = freelist_block_count(list);
            expect_should_be(0, current_blocks);
        }
    }

    // index  0  1   2   3   4   5   6   7
    // offset 0 128 256 384 512 640 768 896

    // Освобождаем последний!
    result = freelist_free_block(list, size, offset); // 896.
    expect_to_be_true(result);

    current_size = freelist_free_space(list);
    expect_should_be(size, current_size);

    current_blocks = freelist_block_count(list);
    expect_should_be(1, current_blocks);

    // Освобождаем первый!
    result = freelist_free_block(list, size, 0); // 0.
    expect_to_be_true(result);

    current_size = freelist_free_space(list);
    expect_should_be(256, current_size);

    current_blocks = freelist_block_count(list);
    expect_should_be(2, current_blocks);

    // Освобождаем третий!
    result = freelist_free_block(list, size, 256);
    expect_to_be_true(result);

    current_size = freelist_free_space(list);
    expect_should_be(384, current_size); // 3*128

    current_blocks = freelist_block_count(list);
    expect_should_be(3, current_blocks);

    // Освобождаем второй!
    result = freelist_free_block(list, size, 128);
    expect_to_be_true(result);

    current_size = freelist_free_space(list);
    expect_should_be(512, current_size); // 4*128

    current_blocks = freelist_block_count(list);
    expect_should_be(2, current_blocks); // Освобождение между двумя блоками!

    // Освобождаем предпоследний!
    result = freelist_free_block(list, size, 768);
    expect_to_be_true(result);

    current_size = freelist_free_space(list);
    expect_should_be(640, current_size); // 5*128

    current_blocks = freelist_block_count(list);
    expect_should_be(2, current_blocks);

    // Finish!
    freelist_destroy(list);
    kfree(memory, freelist_requirement, MEMORY_TAG_ARRAY);
    return true;
}

u8 freelist_test4()
{
    u64 total_size = 512;

    freelist* list = null;  
    u64 freelist_requirement = 0;

    list = freelist_create(total_size, &freelist_requirement, null);
    void* memory = kallocate(freelist_requirement, MEMORY_TAG_ARRAY);
    list = freelist_create(total_size, &freelist_requirement, memory);
    // Start!

    bool result = false;
    u64 current_size = 0;
    u64 current_blocks = 0;

    u64 offset1 = INVALID_ID;
    result = freelist_allocate_block(list, 64, &offset1);
    expect_to_be_true(result);
    expect_should_be(0, offset1);

    u64 offset2 = INVALID_ID;
    result = freelist_allocate_block(list, 32, &offset2);
    expect_to_be_true(result);
    expect_should_be(64, offset2);

    u64 offset3 = INVALID_ID;
    result = freelist_allocate_block(list, 64, &offset3);
    expect_to_be_true(result);
    expect_should_be(96, offset3);

    // Проверка памяти и количество свободных блоков.
    current_size = freelist_free_space(list);
    expect_should_be(total_size - 160, current_size);

    current_blocks = freelist_block_count(list);
    expect_should_be(1, current_blocks);

    result = freelist_free_block(list, 32, offset2);
    expect_to_be_true(result);

    // Проверка памяти и количество свободных блоков.
    current_size = freelist_free_space(list);
    expect_should_be(total_size - 128, current_size);

    current_blocks = freelist_block_count(list);
    expect_should_be(2, current_blocks);

    // Так как сейчас два блока в следующем порядке: 32 -> 352, то при выделении должно остаться 2 блока.
    u64 offset4 = INVALID_ID;
    result = freelist_allocate_block(list, 64, &offset4);
    expect_to_be_true(result);
    expect_should_be(160, offset4);

    // Проверка памяти и количество свободных блоков.
    current_size = freelist_free_space(list);
    expect_should_be(total_size - 192, current_size); // 128 + 64!

    current_blocks = freelist_block_count(list);
    expect_should_be(2, current_blocks);

    result = freelist_free_block(list, 64, offset1);
    expect_to_be_true(result);

    current_size = freelist_free_space(list);
    expect_should_be(total_size - 128, current_size);

    current_blocks = freelist_block_count(list);
    expect_should_be(2, current_blocks);              // Т.к. первый объединиться со вторым.

    result = freelist_free_block(list, 64, offset3);
    expect_to_be_true(result);

    current_size = freelist_free_space(list);
    expect_should_be(total_size - 64, current_size); // Т.к. остается только один блок занятый.

    current_blocks = freelist_block_count(list);
    expect_should_be(2, current_blocks);             // Так же будет объединение блоков.

    result = freelist_free_block(list, 64, offset4);
    expect_to_be_true(result);

    current_size = freelist_free_space(list);
    expect_should_be(total_size, current_size);

    current_blocks = freelist_block_count(list);
    expect_should_be(1, current_blocks);

    // Finish!
    freelist_destroy(list);
    kfree(memory, freelist_requirement, MEMORY_TAG_ARRAY);
    return true;
}

u8 freelist_test5()
{
    u64 total_size = 512;

    freelist* list = null;  
    u64 freelist_requirement = 0;

    list = freelist_create(total_size, &freelist_requirement, null);
    void* memory = kallocate(freelist_requirement, MEMORY_TAG_ARRAY);
    list = freelist_create(total_size, &freelist_requirement, memory);
    // Start!

    bool result = false;

    u64 offset = INVALID_ID;
    result = freelist_allocate_block(list, 512, &offset);
    expect_to_be_true(result);
    expect_should_be(0, offset);

    u64 current_size = freelist_free_space(list);
    expect_should_be(0, current_size);

    u64 block_count = freelist_block_count(list);
    expect_should_be(0, block_count);

    u64 offset2 = INVALID_ID;
    kdebug("The following warning message is intentional by this test.");
    result = freelist_allocate_block(list, 64, &offset2);
    expect_to_be_false(result);

    current_size = freelist_free_space(list);
    expect_should_be(0, current_size);

    block_count = freelist_block_count(list);
    expect_should_be(0, block_count);

    // Finish!
    freelist_destroy(list);
    kfree(memory, freelist_requirement, MEMORY_TAG_ARRAY);
    return true;
}

typedef struct alloc_data {
    u64 size;
    u64 offset;
} alloc_data;

u8 util_freelist_allocate(freelist* list, alloc_data* data, u64* currently_allocated, u64 total_list_size)
{
    // Выделите немного места.
    data->offset = INVALID_ID;

    bool result = freelist_allocate_block(list, data->size, &data->offset);
    expect_to_be_true(result);
    expect_should_not_be(data->offset, INVALID_ID);

    // Отслеживание.
    *currently_allocated += data->size;

    u64 free_space = freelist_free_space(list);
    expect_should_be(free_space, total_list_size - *currently_allocated);

    return true;
}

u8 util_freelist_free(freelist* list, alloc_data* data, u64* currently_allocated, u64 total_list_size)
{
    // Освобождение блока и проверка места.
    bool result = freelist_free_block(list, data->size, data->offset);
    expect_to_be_true(result);

    // Отслеживание.
    *currently_allocated -= data->size;

    u64 free_space = freelist_free_space(list);
    expect_should_be(free_space, total_list_size - *currently_allocated);

    data->offset = INVALID_ID;
    return true;
}

u8 freelist_test6()
{
    freelist* list = null;
    const u32 alloc_data_count = 65556;
    alloc_data alloc_datas[65556] = {0};

    for (u32 i = 0; i < alloc_data_count; ++i)
    {
        alloc_datas[i].size = (u64)krandom_in_range(1, 65536);
        alloc_datas[i].offset = INVALID_ID; // Данные не выделены!
    }

    // Общий размер, необходимый для списка.
    u64 total_size = 0;
    for (u32 i = 0; i < alloc_data_count; ++i)
    {
        total_size += alloc_datas[i].size;
    }

    ktrace("Total memory is %lu B.", total_size);

    u64 freelist_requirement = 0;

    list = freelist_create(total_size, &freelist_requirement, null);
    void* memory = kallocate(freelist_requirement, MEMORY_TAG_ARRAY);
    list = freelist_create(total_size, &freelist_requirement, memory);

    u64 free_space = freelist_free_space(list);
    expect_should_be(total_size, free_space);

    // Выполнение ряда случайных операций со списком с проверкой каждой из них.
    u64 currently_allocated = 0;
    u32 op_count = 0;
    const u32 max_op_count = 100000;
    u32 alloc_count = 0;

    while(op_count < max_op_count)
    {
        // Если выделений нет или мы "выбрасываем" много, выделяем. В противном случае освобождаем.
        if(alloc_count == 0 || krandom_in_range(0, 99) > 50)
        {
            while(true)
            {
                u32 index = (u32)krandom_in_range(0, alloc_data_count - 1);

                // Ищем нераспределенный блок.
                if (alloc_datas[index].offset == INVALID_ID)
                {
                    if (!util_freelist_allocate(list, &alloc_datas[index], &currently_allocated, total_size))
                    {
                        kerror("util_allocate failed on index: %u.", index);
                        return false;
                    }
                    alloc_count++;
                    break;
                }
            }
        }
        else
        {
            while(true)
            {
                u32 index = (u32)krandom_in_range(0, alloc_data_count - 1);

                // Поиск выделенного блока.
                if (alloc_datas[index].offset != INVALID_ID)
                {
                    if (!util_freelist_free(list, &alloc_datas[index], &currently_allocated, total_size))
                    {
                        kerror("util_free failed on index: %u.", index);
                        return false;
                    }
                    alloc_count--;
                    break;
                }
            }
        }

        op_count++;
    }

    ktrace("Max op count of %u reached. Freeing remaining allocations.", max_op_count);
    for (u32 i = 0; i < alloc_data_count; ++i)
    {
        if (alloc_datas[i].offset != INVALID_ID)
        {
            if (!util_freelist_free(list, &alloc_datas[i], &currently_allocated, total_size))
            {
                kerror("util_free failed on index: %u.");
                return false;
            }
        }
    }

    ktrace("Freelist has block capacity: %lu.", freelist_block_capacity(list));

    free_space = freelist_free_space(list);
    expect_should_be(total_size, free_space);

    u64 block_count = freelist_block_count(list);
    expect_should_be(1, block_count);

    freelist_destroy(list);
    kfree(memory, freelist_requirement, MEMORY_TAG_ARRAY);
    return true;
}

void freelist_register_tests()
{
    // NOTE: Для проведения этих тестов рекомендуется в freelist.h NODE_START установть в 1.

    test_managet_register_test(freelist_test1, "Freelist should create and destroy successfully.");
    test_managet_register_test(freelist_test2, "Freelist allocate and free one entry.");
    test_managet_register_test(freelist_test3, "Freelist allocate and free multiple entries.");
    test_managet_register_test(freelist_test4, "Freelist allocate and free multiple entries of varying sizes.");
    test_managet_register_test(freelist_test5, "Freelist allocate to full and fail when trying to allocate more.");
    test_managet_register_test(freelist_test6, "Freelist should randomly allocate and free.");
}
