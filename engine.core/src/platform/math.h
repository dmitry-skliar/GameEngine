#pragma once

#include <defines.h>

/*
    @brief Вычисляет синус числа.
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_sin(f32 x);

/*
    @brief Вычисляет косинус числа.
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_cos(f32 x);

/*
    @brief Вычисляет тангенс числа.
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_tan(f32 x);

/*
    @brief Вычисляет арктангенс числа.
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_atan(f32 x);

/*
    @brief Вычисляет арккосинус числа.
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_acos(f32 x);

/*
    @brief Вычисляет квадратный корень числа.
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_sqrt(f32 x);

/*
    @brief Вычисляет абсолютное значение числа.
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_abs(f32 x);

/*
    @brief Возвращает наибольшее целое значение, меньшее или равное числу.
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_floor(f32 x);

/*
    @brief Возвращает наименьшее целое значение, большее или равное числу.
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_ceil(f32 x);

/*
    @brief Вычисляет логарифм числа по основанию 2 (т.е. сколько раз x можно разделить на 2).
    @param x Число.
    @return Результирующее значение.
*/
KAPI f32 platform_math_log2(f32 x);

/*
    @brief Вычисляет значение возведенного в степень.
    @param x Число.
    @param p Степерь.
    @return Результирующее значение.
*/
KAPI f32 platform_math_pow(f32 x, f32 p);

/*
    @brief Генерирует случайное целое число.
    @return Случайное целое число.
*/
KAPI i32 platform_math_random();

/*
    @brief Генерирует случайное целое число, находящееся в указанном диапазоне (включительно).
    @param min Минимум диапазона.
    @param max Максимум диапазона.
    @return Случайное целое число.
*/
KAPI i32 platform_math_random_in_range(i32 min, i32 max);

/*
    @brief Генерирует случайное число с плавающей точкой.
    @return Случайное число c плавающей точкой.
*/
KAPI f32 platform_math_frandom();

/*
    @brief Генерирует случайное число с плавающей точкой, находящееся в указанном диапазоне (включительно).
    @param min Минимум диапазона.
    @param max Максимум диапазона.
    @return Случайное число c плавающей точкой.
*/
KAPI f32 platform_math_frandom_in_range(f32 min, f32 max);
