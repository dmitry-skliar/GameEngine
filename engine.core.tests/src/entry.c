// Внутренние подключения.
#include "test_manager.h"

// Внешние подключения.
#include <logger.h>

// Подключения тестов.
#include "memory/linear_allocator_tests.h"

int main()
{
    // Инициализация менеджера тестирования.
    test_manager_init();

    // INFO: Регистрация тестов здесь.
    {
        linear_allocator_register_tests();
    }
    // INFO: Конец регистрации тестов.

    kdebug("Starting tests...");

    // Запуск тестов.
    test_manager_run_tests();

    // Завершение работы менеджера тестирования.
    test_manager_shutdown();

    return 0;
}
