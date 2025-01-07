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
            i32 width = data.i32[0];
            i32 height = data.i32[1];
    */
    EVENT_CODE_APPLICATION_RESIZE,

    /*
        @brief Нажатие клашиш клавиатуры.
        Получение контекста:
            u32 key_code = data.u32[0];
    */
    EVENT_CODE_KEYBOARD_KEY_PRESSED,

    /*
        @brief Освобождение клавиши клавиатуры.
        Получение контекста:
            u32 key_code = data.u32[0];
    */
    EVENT_CODE_KEYBOARD_KEY_RELEASED,

    /*
        @brief Нажатие клавиши мыши.
        Получение контекста:
            u32 button_code = data.u32[0];
    */
    EVENT_CODE_MOUSE_BUTTON_PRESSED,

    /*
        @brief Освобождение клавиши мыши.
        Получение контекста:
            u32 button_code = data.u32[0];
    */
    EVENT_CODE_MOUSE_BUTTON_RELEASED,

    /*
        @brief Движение мыши.
        Получение контекста:
            i32 x = data.i32[0];
            i32 y = data.i32[1];
    */
    EVENT_CODE_MOUSE_MOVED,

    /*
        @brief Прокрутка колесика мыши.
        Получение контекста:
            i32 z_delta = data.i32[0];
    */
    EVENT_CODE_MOUSE_WHEEL,

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

// @brief Указатель на функцию обработчик события.
typedef bool (*PFN_event_handler)(event_code code, void* sender, void* listener, event_context data);

/*
    @brief Инициализирует и запускает систему событий.
    @return В случае успеха - true, в случае ошибок - false.
*/
KAPI bool event_system_initialize();

/*
    @brief Завершает и останавливает систему событий.
*/
KAPI void event_system_shutdown();

/*
    @brief Регистрирует функцию-обработчик на заданное событие.
    @param code Код события.
    @param listener Указатель на слушателя события, может быть null.
    @param handler Функция обработчик события.
    @return В случае регистрации события - true, если событие уже существует - false.
*/
KAPI bool event_register(event_code code, void* listener, PFN_event_handler handler);

/*
    @brief Снимает регистрацию функцию-обработчика заданного события.
    @param code Код события.
    @param listener Указатель на слушателя события, может быть null.
    @param handler Функция обработчик события.
    @return В случае снятия регистрации события - true, если событие не существует - false.
*/
KAPI bool event_unregister(event_code code, void* listener, PFN_event_handler handler);

/*
    @brief Создает событие с заданным кодом события и его контекстом.
    @param code Код события.
    @param sender Указатель на отправителя события, может быть null.
    @param data Контекст события, данные передаваемые с событием.
    @return В случае обработки события - true, если событие не было обработано - false.
*/
KAPI bool event_send(event_code code, void* sender, event_context data);

/*
    @brief По коду события выдает символьную строку.
    @param code Код события.
    @return Символьная строка.
*/
KAPI const char* event_code_name_get(event_code code);
