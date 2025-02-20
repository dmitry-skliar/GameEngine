// Внутренние подключения.
#include "test_manager.h"

// Внешние подключения.
#include <logger.h>
#include <memory/memory.h>

// Подключения тестов.
#include "memory/linear_allocator_tests.h"
#include "memory/dynamic_allocator_tests.h"
#include "containers/hashtable_tests.h"
#include "containers/freelist_test.h"
#include "string/kstring_tests.h"

int main()
{
    // Использование менеджера памяти.
    memory_system_config conf;
    conf.total_allocation_size = GIBIBYTES(1);
    memory_system_initialize(&conf);

    // Инициализация менеджера тестирования.
    test_manager_init();

    // INFO: Регистрация тестов здесь.

    linear_allocator_register_tests();
    hashtable_register_tests();
    string_register_tests();
    freelist_register_tests();
    dynamic_allocator_register_tests();

    // INFO: Конец регистрации тестов.

    kdebug("Starting tests...");

    // Запуск тестов.
    test_manager_run_tests();

    // Завершение работы менеджера тестирования.
    test_manager_shutdown();

    // Завершение работы менеджера.
    memory_system_shutdown();

    return 0;
}
