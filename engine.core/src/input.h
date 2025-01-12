#pragma once

#include <defines.h>
#include <input_types.h>

/*
    @brief Запускает систему ввода.
    @return В случае успеха - true, в случае ошибок - false.
*/
bool input_system_initialize();

/*
    @brief Останавливает систему ввода.
*/
void input_system_shutdown();

/*
    @brief Обновляет состояние системы ввода.
    NOTE: Обязательно вставить в обработку кадра!
    @param delta_time Время кадра.
*/
void input_system_update(f64 delta_time);

/*
    @brief Обновляет текущее состояние клавиши клавиатуры.
    NOTE: Используется в функции обработки прерывания ввода.
    @param key Код клавиши.
    @param pressed Состояние клавиши.
*/
void input_update_keyboard_key(key key, bool pressed);

/*
    @brief Обновляет текущее состояние кнопки мышки.
    NOTE: Используется в функции обработки прерывания ввода.
    @param button Код кнопки.
    @param pressed Состояние кнопки.
*/
void input_update_mouse_button(button button, bool pressed);

/*
    @brief Обновляет текущее положения координат мышки.
    NOTE: Используется в функции обработки прерывания ввода.
    @param x Значение координаты X.
    @param y Значение координаты Y.
*/
void input_update_mouse_move(i32 x, i32 y);

/*
    @brief Обновляет текущее значение колесика мышки.
    NOTE: Используется в функции обработки прерывания ввода.
    @param z_delta Значение колесика.
*/
void input_update_mouse_wheel(i32 z_delta);

/*
    @brief Получает текущее состояние клавиши клавиатуры.
    @param key Код нажатой клавиши.
    @return Текущее состояние клавиши.
*/
KAPI bool input_is_keyboard_key_down(key key);

/*
    @brief Получает текущее состояние клавиши клавиатуры.
    @param key Код ненажатой клавиши.
    @return Текущее состояние клавиши.
*/
KAPI bool input_is_keyboard_key_up(key key);

/*
    @brief Получает предыдущее состояние клавиши клавиатуры.
    @param key Код нажатой клавиши.
    @return Предыдущее стостояние клавиши.
*/
KAPI bool input_was_keyboard_key_down(key key);

/*
    @brief Получает предыдущее состояние клавиши клавиатуры.
    @param key Код ненажатой клавиши.
    @return Предыдущее стостояние клавиши.
*/
KAPI bool input_was_keyboard_key_up(key key);

/*
    @brief Получает текущее состояние кнопки мышки.
    @param button Код нажатой кнопки.
    @return Текущее состояние кнопки.
*/
KAPI bool input_is_mouse_button_down(button button);

/*
    @brief Получает текущее состояние кнопки мышки.
    @param button Код ненажатой кнопки.
    @return Текущее состояние кнопки.
*/
KAPI bool input_is_mouse_button_up(button button);

/*
    @brief Получает предыдущее состояние кнопки мышки.
    @param button Код нажатой кнопки.
    @return Предыдущее состояние кнопки.
*/
KAPI bool input_was_mouse_button_down(button button);

/*
    @brief Получает предыдущее состояние кнопки мышки.
    @param button Код ненажатой кнопки.
    @return Предыдущее состояние кнопки.
*/
KAPI bool input_was_mouse_button_up(button button);

/*
    @brief Получает текущее положение координат мышки.
    @param x Значение координаты X.
    @param y Значение координаты Y.
*/
KAPI void input_get_current_mouse_position(i32* x, i32* y);

/*
    @brief Получает предыдущее положение координат мышки.
    @param x Значение координаты X.
    @param y Значение координаты Y.
*/
KAPI void input_get_previous_mouse_position(i32* x, i32* y);

/*
    @brief По коду клавиши клавиатуры возвращает символьную строку.
    @param key Код клавиши.
    @return Символьная строка.
*/
KAPI const char* input_get_keyboard_key_name(key key);

/*
    @brief По коду кнопки мышки возвращает символьную строку.
    @param button Код кнопки.
    @return Символьная строка.
*/
KAPI const char* input_get_mouse_button_name(button button);
