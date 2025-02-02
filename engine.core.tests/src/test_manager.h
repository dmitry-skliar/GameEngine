#pragma once

#include <defines.h>

#define BYPASS 2

/*
    @brief Указателя на функцию тестирования.
*/
typedef u8 (*PFN_test)();

/*
    @brief Инициализация менеджера тестирования.
*/
void test_manager_init();

/*
    @brief Регистрация тестов в менеджере.
    @param function Указатель на функцию тестирования.
    @param description Описание проводимого теста.
*/
void test_managet_register_test(PFN_test function, char* description);

/*
    @brief Выполнить тестирование.
*/
void test_manager_run_tests();

/*
    @brief Завершить работу менеджера тестирования.
*/
void test_manager_shutdown();
