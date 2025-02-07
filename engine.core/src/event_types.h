#pragma once

#include <defines.h>

// @brief Код события.
typedef enum event_code {

    /*
        @brief Ошибочный код события.
    */
    EVENT_CODE_NULL,

    /*
        @brief Завершение работы приложения на следующем кадре.
    */
    EVENT_CODE_APPLICATION_QUIT,

    /*
        @brief Изменение размера окна.
        Получение контекста:
            i32 width = context.i32[0];
            i32 height = context.i32[1];
    */
    EVENT_CODE_APPLICATION_RESIZE,

    /*
        @brief Нажатие клашиш клавиатуры.
        Получение контекста:
            u32 key_code = context.u32[0];
    */
    EVENT_CODE_KEYBOARD_KEY_PRESSED,

    /*
        @brief Освобождение клавиши клавиатуры.
        Получение контекста:
            u32 key_code = context.u32[0];
    */
    EVENT_CODE_KEYBOARD_KEY_RELEASED,

    /*
        @brief Нажатие клавиши мыши.
        Получение контекста:
            u32 button_code = context.u32[0];
    */
    EVENT_CODE_MOUSE_BUTTON_PRESSED,

    /*
        @brief Освобождение клавиши мыши.
        Получение контекста:
            u32 button_code = context.u32[0];
    */
    EVENT_CODE_MOUSE_BUTTON_RELEASED,

    /*
        @brief Движение мыши.
        Получение контекста:
            i32 x = context.i32[0];
            i32 y = context.i32[1];
    */
    EVENT_CODE_MOUSE_MOVED,

    /*
        @brief Прокрутка колесика мыши.
        Получение контекста:
            i32 z_delta = context.i32[0];
    */
    EVENT_CODE_MOUSE_WHEEL,

    /*
        @brief Отладка.
    */
    EVENT_CODE_DEBUG_0,
    EVENT_CODE_DEBUG_1,
    EVENT_CODE_DEBUG_2,
    EVENT_CODE_DEBUG_3,
    EVENT_CODE_DEBUG_4,

    /*
        @brief Максимально допустимое количество кодов событий.
    */
    EVENT_CODES_MAX = 0x3fff

} event_code;

// @brief Контекст события (128 bits).
typedef union event_context {
    // @brief Массив из 2 элементов, 64-битное беззнаковое целое.
    u64 u64[2];
    // @brief Массив из 2 элементов, 64-битное знаковое целое.
    i64 i64[2];
    // @brief Массив из 2 элементов, 64-битное число с плавающей точкой.
    f64 f64[2];
    // @brief Массив из 4 элементов, 32-битное беззнаковое целое.
    u32 u32[4];
    // @brief Массив из 4 элементов, 32-битное знаковое целое.
    i32 i32[4];
    // @brief Массив из 4 элементов, 32-битное число с плавающей точкой.
    f32 f32[4];
    // @brief Массив из 8 элементов, 16-битное беззнаковое целое.
    u16 u16[8];
    // @brief Массив из 8 элементов, 16-битное знаковое целое.
    i16 i16[8];
    // @brief Массив из 16 элементов, 8-битное беззнаковое целое.
    u8  u8[16];
    // @brief Массив из 16 элементов, 8-битное знаковое целое.
    i8  i8[16];
    // @brief Строка из 15 символов, 8-битные символы (16 символ должен быть нулевой!).
    char c[16];
} event_context;
