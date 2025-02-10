#include "hashtable_tests.h"
#include "test_manager.h"
#include "expect.h"

#include <containers/hashtable.h>
#include <memory/memory.h>
#include <kstring.h>

u8 hashtable_test1()
{
    u64 data_size = sizeof(u64);
    u64 entry_count = 1;

    hashtable* table = null;
    bool result = false;
    u64 hashtable_memory_requirement = 0;
    void* hashtable_memory = null;
    hashtable_config hconf;
    hconf.data_size = 0;
    hconf.entry_count = entry_count;

    kdebug("Note: The following 3 errors message are intentionally caused by this test.");
    result = hashtable_create(null, null, null, null);
    expect_to_be_false(result);

    result = hashtable_create(&hashtable_memory_requirement, null, &hconf, null);
    expect_to_be_false(result);

    hconf.data_size = data_size;
    result = hashtable_create(&hashtable_memory_requirement, null, &hconf, null);
    expect_to_be_true(result);

    hashtable_memory = kallocate(hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    result = hashtable_create(&hashtable_memory_requirement, hashtable_memory, &hconf, null);
    expect_to_be_false(result);

    result = hashtable_create(&hashtable_memory_requirement, hashtable_memory, &hconf, &table);
    expect_to_be_true(result);
    expect_pointer_should_not_be(null, table);

    hashtable_destroy(table);
    kfree(hashtable_memory, hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    return true;
}

u8 hashtable_test2()
{
    u64 data_size  = sizeof(u64);
    u64 entry_count = 20;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = null;
    bool result = false;
    u64 hashtable_memory_requirement = 0;
    void* hashtable_memory = null;
    hashtable_config hconf;
    hconf.data_size = data_size;
    hconf.entry_count = entry_count;

    result = hashtable_create(&hashtable_memory_requirement, null, &hconf, null);
    expect_to_be_true(result);

    hashtable_memory = kallocate(hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    result = hashtable_create(&hashtable_memory_requirement, hashtable_memory, &hconf, &table);
    expect_to_be_true(result);
    expect_pointer_should_not_be(null, table);

    for(u64 i = 0; i < entry_count; ++i)
    {
        u64 tval = i + 1000;
        bool result = hashtable_set(table, str[i], &tval, false);
        expect_to_be_true(result);

    }

    for(u64 i = 0; i < entry_count; ++i)
    {
        u64 tval = i + 1000;
        u64 get_val = 0;
        hashtable_get(table, str[i], &get_val);
        expect_should_be(tval, get_val);
    }

    hashtable_destroy(table);
    kfree(hashtable_memory, hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    return true;
}

u8 hashtable_test3()
{
    u64 data_size  = sizeof(u64);
    u64 entry_count = 1;

    hashtable* table = null;
    bool result = false;
    u64 hashtable_memory_requirement = 0;
    void* hashtable_memory = null;
    hashtable_config hconf;
    hconf.data_size = data_size;
    hconf.entry_count = entry_count;

    result = hashtable_create(&hashtable_memory_requirement, null, &hconf, null);
    expect_to_be_true(result);

    hashtable_memory = kallocate(hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    result = hashtable_create(&hashtable_memory_requirement, hashtable_memory, &hconf, &table);
    expect_to_be_true(result);
    expect_pointer_should_not_be(null, table);

    u64 get_val = 0;
    kdebug("Note: The following warning is intentionally caused by this test.");
    result = hashtable_get(table, "test word", &get_val);
    expect_should_be(0, get_val);
    expect_to_be_false(result);

    hashtable_destroy(table);
    kfree(hashtable_memory, hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    return true;
}

u8 hashtable_test4()
{
    u64 data_size  = sizeof(u64);
    u64 entry_count = 20;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = null;
    bool result = false;
    u64 hashtable_memory_requirement = 0;
    void* hashtable_memory = null;
    hashtable_config hconf;
    hconf.data_size = data_size;
    hconf.entry_count = entry_count;

    result = hashtable_create(&hashtable_memory_requirement, null, &hconf, null);
    expect_to_be_true(result);

    hashtable_memory = kallocate(hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    result = hashtable_create(&hashtable_memory_requirement, hashtable_memory, &hconf, &table);
    expect_to_be_true(result);
    expect_pointer_should_not_be(null, table);

    for(u64 i = 0; i < entry_count; ++i)
    {
        u64 val = i + 1000;
        result = hashtable_set(table, str[i], &val, false);
        expect_to_be_true(result);
    }

    u64 get_val = 0;
    kdebug("Note: The following warning is intentionally caused by this test.");
    result = hashtable_get(table, "some string", &get_val);
    expect_should_be(0, get_val);
    expect_to_be_false(result);

    hashtable_destroy(table);
    kfree(hashtable_memory, hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    return true;
}

u8 hashtable_test5()
{
    u64 data_size  = sizeof(u64);
    u64 entry_count = 19;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = null;
    bool result = false;
    u64 hashtable_memory_requirement = 0;
    void* hashtable_memory = null;
    hashtable_config hconf;
    hconf.data_size = data_size;
    hconf.entry_count = entry_count;

    result = hashtable_create(&hashtable_memory_requirement, null, &hconf, null);
    expect_to_be_true(result);

    hashtable_memory = kallocate(hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    result = hashtable_create(&hashtable_memory_requirement, hashtable_memory, &hconf, &table);
    expect_to_be_true(result);
    expect_pointer_should_not_be(null, table);

    for(u64 i = 0; i < entry_count + 1; ++i)
    {
        u64 val = i + 1000;

        if(i >= entry_count)
        {
            kdebug("Note: The following warning is intentionally caused by this test.");
        }

        bool result = hashtable_set(table, str[i], &val, false);

        if(i < entry_count)
        {
            expect_to_be_true(result);
        }
        else
        {
            expect_to_be_false(result);
        }
    }

    hashtable_destroy(table);
    kfree(hashtable_memory, hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    return true;
}

u8 hashtable_test6()
{
    u64 data_size  = sizeof(u64);
    u64 entry_count = 19;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = null;
    bool result = false;
    u64 hashtable_memory_requirement = 0;
    void* hashtable_memory = null;
    hashtable_config hconf;
    hconf.data_size = data_size;
    hconf.entry_count = entry_count;

    result = hashtable_create(&hashtable_memory_requirement, null, &hconf, null);
    expect_to_be_true(result);

    hashtable_memory = kallocate(hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    result = hashtable_create(&hashtable_memory_requirement, hashtable_memory, &hconf, &table);
    expect_to_be_true(result);
    expect_pointer_should_not_be(null, table);

    for(u64 i = 0; i < entry_count + 1; ++i)
    {
        u64 val = i + 1000;
        bool result = false;

        if(i < entry_count)
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
    kfree(hashtable_memory, hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    return true;
}

u8 hashtable_test7()
{
    u64 data_size  = sizeof(u64);
    u64 entry_count = 20;
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "A test"
    };

    hashtable* table = null;
    bool result = false;
    u64 hashtable_memory_requirement = 0;
    void* hashtable_memory = null;
    hashtable_config hconf;
    hconf.data_size = data_size;
    hconf.entry_count = entry_count;

    result = hashtable_create(&hashtable_memory_requirement, null, &hconf, null);
    expect_to_be_true(result);

    hashtable_memory = kallocate(hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    result = hashtable_create(&hashtable_memory_requirement, hashtable_memory, &hconf, &table);
    expect_to_be_true(result);
    expect_pointer_should_not_be(null, table);

    u64 rval = 0;

    for(u64 i = 0; i < entry_count; ++i)
    {
        u64 val = i + 1000;
        bool result = hashtable_set(table, str[i], &val, true);
        expect_to_be_true(result);

        // На последнем шаге сохранаяется индекс для проверки обновления записи "A test".
        rval = val;
    }

    for(u64 i = 0; i < entry_count; ++i)
    {
        u64 val = 0;
        u64 ival = i + 1000;
        bool result = false;

        if(i == 0 || i == entry_count - 1)
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
    kfree(hashtable_memory, hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
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
    u64 entry_count = 20;
    test_structure* test_values = kallocate_tc(test_structure, entry_count, MEMORY_TAG_ARRAY);
    const char* str[20] = {
        "A test", "Ba1 tasaf", "noiasef", "kajwbf", "bjaf", "a", "zzzzz", "jwibaw", "paiejht", "ajenbf",
        "wjf", "wait", "include", "foreach", "continue", "AAAAAAAAAAAA", "GOAL", "Papa", "Mara", "Zeor"
    };

    hashtable* table = null;
    bool result = false;
    u64 hashtable_memory_requirement = 0;
    void* hashtable_memory = null;
    hashtable_config hconf;
    hconf.data_size = data_size;
    hconf.entry_count = entry_count;

    result = hashtable_create(&hashtable_memory_requirement, null, &hconf, null);
    expect_to_be_true(result);

    hashtable_memory = kallocate(hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
    result = hashtable_create(&hashtable_memory_requirement, hashtable_memory, &hconf, &table);
    expect_to_be_true(result);
    expect_pointer_should_not_be(null, table);

    for(u64 i = 0; i < entry_count; ++i)
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

    for(u64 i = 0; i < entry_count; ++i)
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
    kfree_tc(test_values, test_structure, entry_count, MEMORY_TAG_ARRAY);
    kfree(hashtable_memory, hashtable_memory_requirement, MEMORY_TAG_HASHTABLE);
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
        hashtable_test4, "Hashtable should set and get non-exists entry then return false successfully."
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
