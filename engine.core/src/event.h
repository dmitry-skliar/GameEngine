#pragma once

#include <defines.h>
#include <event_types.h>

/*
    @brief Указатель на функцию обработчик события.
    @param code Код события.
    @param sender Указатель на отправителя события, может быть null.
    @param listener Указатель на слушателя события, может быть null.
    @param data Контекст события, данные передаваемые с событием.
    @return Завершить обработку события другими слушателями - true, в противном случае - false.
*/
typedef bool (*PFN_event_handler)(event_code code, void* sender, void* listener, event_context data);

/*
    @brief Запускает систему событий.
    @return В случае успеха - true, в случае ошибок - false.
*/
bool event_system_initialize();

/*
    @brief Останавливает систему событий.
*/
void event_system_shutdown();

/*
    @brief Регистрирует функцию-обработчик на заданное событие.
    @param code Код события.
    @param listener Указатель на слушателя события, может быть null.
    @param handler Функция обработчик события.
    @return При успешном добавлении слушателя - true, если слушатель уже существует - false.
*/
KAPI bool event_register(event_code code, void* listener, PFN_event_handler handler);

/*
    @brief Снимает регистрацию функцию-обработчика заданного события.
    @param code Код события.
    @param listener Указатель на слушателя события, может быть null.
    @param handler Функция обработчик события.
    @return При успешном удалении слушателя - true, если слушатель не существует - false.
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
    @brief По коду события возвращает символьную строку.
    @param code Код события.
    @return Символьная строка.
*/
KAPI const char* event_get_code_name(event_code code);
