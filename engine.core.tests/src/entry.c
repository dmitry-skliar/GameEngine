// Внутренние подключения.
#include "test_manager.h"

// Внешние подключения.
#include <logger.h>
#include <memory/memory.h>

// Подключения тестов.
#include "memory/linear_allocator_tests.h"
#include "containers/hashtable_tests.h"
#include "containers/freelist_test.h"
#include "string/kstring_tests.h"

int main()
{
    // Вуделение памяти и включение менеджера памяти.
    u64 requirement;
    memory_system_initialize(&requirement, null);
    void* memory = kallocate(requirement, MEMORY_TAG_SYSTEM);
    memory_system_initialize(&requirement, memory);

    // Инициализация менеджера тестирования.
    test_manager_init();

    // INFO: Регистрация тестов здесь.
    {
        linear_allocator_register_tests();
        hashtable_register_tests();
        string_register_tests();
        freelist_register_tests();
    }
    // INFO: Конец регистрации тестов.

    kdebug("Starting tests...");

    // Запуск тестов.
    test_manager_run_tests();

    // Завершение работы менеджера тестирования.
    test_manager_shutdown();

    // Освобождение памяти и завершение работы менеджера.
    memory_system_shutdown();
    kfree(memory, requirement, MEMORY_TAG_SYSTEM);

    return 0;
}
