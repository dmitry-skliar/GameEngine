// Собственные подключения.
#include "test_manager.h"

// Внешние подключения.
#include <logger.h>
#include <memory/memory.h>
#include <containers/darray.h>
#include <kstring.h>
#include <clock.h>

typedef struct {
    PFN_test function;
    char* description;
} test_entry;

static test_entry* tests;

void test_manager_init()
{
    tests = darray_create(test_entry);
}

void test_managet_register_test(PFN_test function, char* description)
{
    test_entry e;
    e.function = function;
    e.description = description;
    darray_push(tests, e);
}

void test_manager_run_tests()
{
    u32 passed  = 0;
    u32 failed  = 0;
    u32 skipped = 0;

    u32 count = darray_length(tests);

    clock total_time;
    clock_start(&total_time);

    for(u32 i = 0; i < count; ++i)
    {
        clock test_time;
        clock_start(&test_time);
        
        u8 result = tests[i].function();
        
        clock_update(&test_time);

        if(result == true)
        {
            ++passed;
        }
        else if(result == BYPASS)
        {
            kwarng("[SKIPPED]: %s", tests[i].description);
            ++skipped;
        }
        else
        {
            kwarng("[FAILED]: %s", tests[i].description);
            ++failed;
        }

        char status[20];
        string_format(status, failed ? "*** %d FAILED ***" : "SUCCESS", failed);
        clock_update(&total_time);
        kinfor(
            "Executed %d of %d (skipped %d) %s (%.6f sec / %.6f sec total)", i+1, count, skipped, status, 
            test_time.elapsed, total_time.elapsed
        );
    }

    clock_stop(&total_time);

    kinfor("Result: %d passed, %d failed, %d skipped.", passed, failed, skipped);
}

void test_manager_shutdown()
{
    darray_destroy(tests);
}
