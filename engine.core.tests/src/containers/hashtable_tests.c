// Own includies.
#include "test_manager.h"
#include "expect.h"
#include "hashtable_tests.h"

// External incudies.
#include <containers/hashtable.h>
#include <memory/memory.h>
#include <kstring.h>

u8 hashtable_test1()
{
    u64 data_size = sizeof(u64);
    u64 data_count = 1;

    kdebug("Note: The following 2 errors are intentionally caused by this test.");
    hashtable* table = hashtable_create(0, data_count);
    expect_pointer_should_be(null, table);

    table = hashtable_create(data_size, 0);
    expect_pointer_should_be(null, table);

    table = hashtable_create(data_size, data_count);
    expect_pointer_should_not_be(null, table);

    hashtable_destroy(table);
    return true;
}

u8 hashtable_test2()
{
    u64 data_size  = sizeof(u64);
    u64 data_count = 20;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = hashtable_create(data_size, data_count);
    expect_pointer_should_not_be(null, table);

    for(u64 i = 0; i < data_count; ++i)
    {
        u64 tval = i + 1000;
        bool result = hashtable_set(table, str[i], &tval, false);
        expect_to_be_true(result);

    }

    for(u64 i = 0; i < data_count; ++i)
    {
        u64 tval = i + 1000;
        u64 get_val = 0;
        hashtable_get(table, str[i], &get_val);
        expect_should_be(tval, get_val);
    }

    hashtable_destroy(table);
    return true;
}

u8 hashtable_test3()
{
    u64 data_size  = sizeof(u64);
    u64 data_count = 1;

    hashtable* table = hashtable_create(data_size, data_count);
    expect_pointer_should_not_be(null, table);

    u64 get_val = 0;
    kdebug("Note: The following warning is intentionally caused by this test.");
    bool result = hashtable_get(table, "test word", &get_val);
    expect_should_be(0, get_val);
    expect_to_be_false(result);

    hashtable_destroy(table);
    return true;
}

u8 hashtable_test4()
{
    u64 data_size  = sizeof(u64);
    u64 data_count = 20;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = hashtable_create(data_size, data_count);
    expect_pointer_should_not_be(null, table);

    for(u64 i = 0; i < data_count; ++i)
    {
        u64 val = i + 1000;
        bool result = hashtable_set(table, str[i], &val, false);
        expect_to_be_true(result);
    }

    u64 get_val = 0;
    kdebug("Note: The following warning is intentionally caused by this test.");
    bool result = hashtable_get(table, "some string", &get_val);
    expect_should_be(0, get_val);
    expect_to_be_false(result);

    hashtable_destroy(table);
    return true;
}

u8 hashtable_test5()
{
    u64 data_size  = sizeof(u64);
    u64 data_count = 19;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = hashtable_create(data_size, data_count);
    expect_pointer_should_not_be(null, table);

    for(u64 i = 0; i < data_count + 1; ++i)
    {
        u64 val = i + 1000;

        if(i >= data_count)
        {
            kdebug("Note: The following warning is intentionally caused by this test.");
        }

        bool result = hashtable_set(table, str[i], &val, false);

        if(i < data_count)
        {
            expect_to_be_true(result);
        }
        else
        {
            expect_to_be_false(result);
        }
    }

    hashtable_destroy(table);
    return true;
}

u8 hashtable_test6()
{
    u64 data_size  = sizeof(u64);
    u64 data_count = 19;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = hashtable_create(data_size, data_count + 1);
    expect_pointer_should_not_be(null, table);

    for(u64 i = 0; i < data_count + 1; ++i)
    {
        u64 val = i + 1000;
        bool result = false;

        if(i < data_count)
        {
            result = hashtable_set(table, str[i], &val, false);
            expect_to_be_true(result);
        }
        else
        {
            kdebug("Note: The following warning is intentionally caused by this test.");
            result = hashtable_set(table, "A test", &val, false);
            expect_to_be_false(result);
        }
    }

    hashtable_destroy(table);
    return true;
}

u8 hashtable_test7()
{
    u64 data_size  = sizeof(u64);
    u64 data_count = 20;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "A test"
    };

    hashtable* table = hashtable_create(data_size, data_count);
    expect_pointer_should_not_be(null, table);

    u64 rval = 0;

    for(u64 i = 0; i < data_count; ++i)
    {
        u64 val = i + 1000;
        bool result = hashtable_set(table, str[i], &val, true);
        expect_to_be_true(result);

        // На последнем шаге сохранаяется индекс для проверки обновления записи "A test".
        rval = val;
    }

    for(u64 i = 0; i < data_count; ++i)
    {
        u64 val = 0;
        u64 ival = i + 1000;
        bool result = false;

        if(i == 0 || i == data_count - 1)
        {
            result = hashtable_get(table, str[i], &val);
            expect_should_be(rval, val);
        }
        else
        {
            result = hashtable_get(table, str[i], &val);
            expect_should_be(ival, val);
        }
        expect_to_be_true(result);
    }

    hashtable_destroy(table);
    return true;
}

typedef struct test_structure {
    bool  b_value;
    f32 f_value;
    u64 u_value;
} test_structure;

u8 hashtable_test8()
{
    u64 data_size = sizeof(test_structure*);
    u64 data_count = 20;
    test_structure* test_values = kallocate_tc(test_structure, data_count, MEMORY_TAG_ARRAY);
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = hashtable_create(data_size, data_count);
    expect_pointer_should_not_be(null, table);

    bool result = false;

    for(u64 i = 0; i < data_count; ++i)
    {
        // Исходные данные.
        test_values[i].b_value = i % 2 == 0;
        test_values[i].f_value = i / 100.0f;
        test_values[i].u_value = i * 100000;

        // Запись данных.
        test_structure* ptr = &test_values[i];
        result = hashtable_set(table, str[i], &ptr, false);

        // Проверка записи.
        expect_pointer_should_not_be(null, ptr);
        expect_to_be_true(result);
    }

    for(u64 i = 0; i < data_count; ++i)
    {
        // Получение указателя на данные.
        test_structure* tval = null;
        result = hashtable_get(table, str[i], &tval);

        // Проверка получения.
        expect_pointer_should_not_be(null, tval);
        expect_to_be_true(result);
        expect_should_be(i % 2 == 0, tval->b_value);
        expect_should_be(i / 100.0f, tval->f_value);
        expect_should_be(i * 100000, tval->u_value);
    }

    hashtable_destroy(table);
    kfree_tc(test_values, test_structure, data_count, MEMORY_TAG_ARRAY);
    return true;
}

void hashtable_register_tests()
{
    test_managet_register_test(
        hashtable_test1, "Hashtable should create and destroy."
    );

    test_managet_register_test(
        hashtable_test2, "Hashtable should set and get successfully."
    );

    test_managet_register_test(
        hashtable_test3, "Hashtable should get false from an empty table successfully."
    );

    test_managet_register_test(
        hashtable_test4, "Hashtable should set and get false from a full table successfully."
    );

    test_managet_register_test(
        hashtable_test5, "Hashtable should set over size data and then return overflow successfully."
    );

    test_managet_register_test(
        hashtable_test6, "Hashtable should set and set duplicate and then return false successfully."
    );

    test_managet_register_test(
        hashtable_test7, "Hashtable should set and set duplicate with update flag successfully."
    );

    test_managet_register_test(
        hashtable_test8, "Hashtable should set and get pointers successfully."
    );
}
