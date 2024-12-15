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
