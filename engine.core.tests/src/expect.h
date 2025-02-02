#pragma once

#include <logger.h>
#include <math/kmath.h>

/*
    @brief Ожидается, что ожидаемое значение будет равно фактическому.
    @param expected Ожидаемое значение.
    @param actual Фактическое значение.
*/
#define expect_should_be(expected, actual)                                                             \
if(actual != expected)                                                                                 \
{                                                                                                      \
    kerror("--> Expected %lld, but got: %lld. In %s:%d.", expected, actual, __FILE_NAME__, __LINE__);  \
    return false;                                                                                      \
}

/*
    @brief Ожидается, что ожидаемое значение НЕ будет равно фактическому.
    @param expected Ожидаемое значение.
    @param actual Фактическое значение.
*/
#define expect_should_not_be(expected, actual)                                                                  \
if(actual == expected)                                                                                          \
{                                                                                                               \
    kerror("--> Expected %d != %d, but they are equal. In %s:%d.", expected, actual, __FILE_NAME__, __LINE__);  \
    return false;                                                                                               \
}

/*
    @brief Ожидает, что ожидаемый указатель будет равен фактическому указателю.
    @param expected Ожидаемое значение указателя.
    @param actual Фактическое значение указателя.
*/
#define expect_pointer_should_be(expected, actual)                                                     \
if(actual != expected)                                                                                 \
{                                                                                                      \
    if(expected != null)                                                                               \
    {                                                                                                  \
        kerror("--> Expected %p, but got: %p. In %s:%d.", expected, actual, __FILE_NAME__, __LINE__);  \
    }                                                                                                  \
    else                                                                                               \
    {                                                                                                  \
        kerror("--> Expected null, but got: %p. In %s:%d.", actual, __FILE_NAME__, __LINE__);          \
    }                                                                                                  \
    return false;                                                                                      \
}

/*
    @brief Ожидает, что ожидаемый указатель НЕ будет равен фактическому указателю.
    @param expected Ожидаемое значение указателя.
    @param actual Фактическое значение указателя.
*/
#define expect_pointer_should_not_be(expected, actual)                                                            \
if(actual == expected)                                                                                            \
{                                                                                                                 \
    if(expected != null)                                                                                          \
    {                                                                                                             \
        kerror("--> Expected %p != %p, but they are equal. In %s:%d.", actual, expected, __FILE_NAME__, __LINE__);\
    }                                                                                                             \
    else                                                                                                          \
    {                                                                                                             \
        kerror("--> Expected %p != null, but they are equal. In %s:%d.", actual, __FILE_NAME__, __LINE__);        \
    }                                                                                                             \
    return false;                                                                                                 \
}

/*
    @brief Ожидается, что значение будет фактическим с учетом допуска K_FLOAT_EPSILON.
    @param expected Ожидаемое значение.
    @param actual Фактическое значение.
*/
#define expect_float_to_be(expected, actual)                                                       \
if(kabs(expected - actual) > K_FLOAT_EPSILON)                                                      \
{                                                                                                  \
    kerror("--> Expected %f, but got: %f. In %s:%d.", expected, actual, __FILE_NAME__, __LINE__);  \
    return false;                                                                                  \
}

/*
    @brief Ожидает, что фактическое окажется правдой.
    @param actual Фактическое значение.
*/
#define expect_to_be_true(actual)                                                    \
if(actual != true)                                                                   \
{                                                                                    \
    kerror("--> Expected true, but got: false. In %s:%d.", __FILE_NAME__, __LINE__); \
    return false;                                                                    \
}

/*
    @brief Ожидает, что фактическое окажется ложным.
    @param actual Фактическое значение.
*/
#define expect_to_be_false(actual)                                                   \
if(actual != false)                                                                  \
{                                                                                    \
    kerror("--> Expected false, but got: true. In %s:%d.", __FILE_NAME__, __LINE__); \
    return false;                                                                    \
}
