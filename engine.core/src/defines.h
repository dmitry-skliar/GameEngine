#pragma once

// @brief 8-битное беззнаковое целое.
typedef unsigned char u8;

// @brief 16-битное беззнаковое целое.
typedef unsigned short u16;

// @brief 32-битное беззнаковое целое.
typedef unsigned int u32;

// @brief 64-битное беззнаковое целое.
typedef unsigned long long u64;

// @brief 8-битное знаковое целое.
typedef signed char i8;

// @brief 16-битное знаковое целое.
typedef signed short i16;

// @brief 32-битное знаковое целое.
typedef signed int i32;

// @brief 64-битное знаковое целое.
typedef signed long long i64;

// @brief 32-битное число c плавающей точкой.
typedef float f32;

// @brief 64-битное число с плавающей точкой.
typedef double f64;

// @brief Логическое значение.
typedef _Bool bool;

#if __cplusplus
    /*
        @brief Выполняет проверку утверждения во время компиляции, и выводит сообщение если оно ложно.
        @param assertion Проверяемое во время компиляции утверждение.
        @param message Сообщение которое будет выведено если утверждение ложно.
    */
    #define STATIC_ASSERT(assertion, message) static_assert(assertion, message)
#else
    /*
        @brief Выполняет проверку утверждения во время компиляции, и выводит сообщение если оно ложно.
        @param assertion Проверяемое во время компиляции утверждение.
        @param message Сообщение которое будет выведено если утверждение ложно.
    */
    #define STATIC_ASSERT(assertion, message) _Static_assert(assertion, message)
#endif

// Проверка типов.
STATIC_ASSERT(sizeof(u8)  == 1, "Assertion 'sizeof(u8) == 1' failed.");
STATIC_ASSERT(sizeof(u16) == 2, "Assertion 'sizeof(u16) == 2' failed.");
STATIC_ASSERT(sizeof(u32) == 4, "Assertion 'sizeof(u32) == 4' failed.");
STATIC_ASSERT(sizeof(u64) == 8, "Assertion 'sizeof(u64) == 8' failed.");
STATIC_ASSERT(sizeof(i8)  == 1, "Assertion 'sizeof(i8) == 1' failed.");
STATIC_ASSERT(sizeof(i16) == 2, "Assertion 'sizeof(i16) == 2' failed.");
STATIC_ASSERT(sizeof(i32) == 4, "Assertion 'sizeof(i32) == 4' failed.");
STATIC_ASSERT(sizeof(i64) == 8, "Assertion 'sizeof(i64) == 8' failed.");
STATIC_ASSERT(sizeof(f32) == 4, "Assertion 'sizeof(f32) == 4' failed.");
STATIC_ASSERT(sizeof(f64) == 8, "Assertion 'sizeof(f64) == 8' failed.");

// Определение логических констант.
#define false 0
#define true  1

#define U8_MIN  0x00
#define I8_MIN  0x80
#define U16_MIN 0x0000
#define I16_MIN 0x8000
#define U32_MIN 0x00000000
#define I32_MIN 0x80000000
#define U64_MIN 0x0000000000000000
#define I64_MIN 0x8000000000000000

#define U8_MAX  0xff
#define I8_MAX  0x7f
#define U16_MAX 0xffff
#define I16_MAX 0x7fff
#define U32_MAX 0xffffffff
#define I32_MAX 0x7fffffff
#define U64_MAX 0xffffffffffffffff
#define I64_MAX 0x7fffffffffffffff

#define INVALID_ID -1

// Определение нулевого указателя.
#define null  ((void*)0)

// Проверка поддерживаемых платформ и их требований (На уровне makefile и библиотеки).
#if KPLATFORM_LINUX_FLAG
#elif KPLATFORM_WINDOWS_FLAG
    // Дополнительная проверка на разрядность операционной системы.
    #ifndef _WIN64
        #error "64-bit is required on Windows."
    #endif
#else
    #error "Unknown platform."
#endif

// Определение используемого компилятора (Только на уровне библиотеки).
#if __clang__
    #define KCOMPILER_CLANG_FLAG 1
#elif _MSC_VER
    #define KCOMPILER_MICROSOFT_FLAG 1
#else
    #error "Unknown compiler"
#endif

// Определение квалификатора KAPI.
#if KEXPORT_FLAG
    #if KCOMPILER_MICROSOFT_FLAG
        #define KAPI __declspec(dllexport)
    #elif KCOMPILER_CLANG_FLAG
        #define KAPI __attribute__((visibility("default")))
    #endif
#else
    #if KCOMPILER_MICROSOFT_FLAG
        #define KAPI __declspec(dllimport)
    #elif KCOMPILER_CLANG_FLAG
        #define KAPI
    #endif
#endif

// Определение квалификаторов KINLINE/NOINLINE.
#if KCOMPILER_MICROSOFT_FLAG
    #define KINLINE  __forceinline
    #define NOINLINE __declspec(noinline)
#elif KCOMPILER_CLANG_FLAG
    #define KINLINE  __attribute__((always_inline)) inline
    #define NOINLINE __attribute__((noinline))
#endif

/*
    @brief Макрос для копирования 8 байт(64 бита) из источника в память назначения.
    @param dest Источник байт которые нужно скопировать.
    @param src Место назначения куда нужно скопировать байты.
*/
#define KCOPY8BYTES(dest, src) *((u64*)dest) = *((u64*)src)

/*
    @brief Макрос для копирования 4 байта(32 бита) из источника в память назначения.
    @param dest Источник байт которые нужно скопировать.
    @param src Место назначения куда нужно скопировать байты.
*/
#define KCOPY4BYTES(dest, src) *((u32*)dest) = *((u32*)src)

/*
    @brief Макрос для копирования 2 байта(16 бит) из источника в память назначения.
    @param dest Источник байт которые нужно скопировать.
    @param src Место назначения куда нужно скопировать байты.
*/
#define KCOPY2BYTES(dest, src) *((u16*)dest) = *((u16*)src)

/*
    @brief Макрос для копирования 1 байт (8 бит) из источника в память назначения.
    @param dest Источник байт которые нужно скопировать.
    @param src Место назначения куда нужно скопировать байты.
*/
#define KCOPY1BYTE(dest, src) *((u8*)dest) = *((u8*)src)

// TODO: Перенести в kmath начало.
/*
    @brief Макрос усечения значения в указанных пределах.
    @param value Значение, которое нужно усечь.
    @param min Значение минимального предела включительно.
    @param max Значение максимального предела включительно.
    @return Значение в заданных пределах.
*/
#define KCLAMP(value, min, max) ((value <= min) ? min : (value >= max) ? max : value)

/*
    @brief Макрос получения минимального значения из двух заданных.
    @param v0 Первое значение.
    @param v1 Второе значение.
    @return Минимальное значение.
*/
#define KMIN(v0,v1) (v0 < v1 ? v0 : v1)

/*
    @brief Макрос получения максимального значения из двух заданных.
    @param v0 Первое значение.
    @param v1 Второе значение.
    @return Максимальное значение.
*/
#define KMAX(v0,v1) (v0 > v1 ? v0 : v1)
// TODO: Перенести в kmath конец.

