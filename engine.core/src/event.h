#pragma once

#include <defines.h>
#include <event_types.h>

/*
    @brief Указатель на функцию обработчик события.
    @param code Код события.
    @param sender Указатель на отправителя события, может быть null.
    @param listener Указатель на слушателя события, может быть null.
    @param context Указатель на контекст события, может быть null.
    @return True останавливает дальнейшую обработку события другими слушателями, false продолжает.
*/
typedef bool (*PFN_event_handler)(event_code code, void* sender, void* listener, event_context* context);

/*
    @brief Запускает систему событий.
*/
void event_system_initialize(u64* memory_requirement, void* memory);

/*
    @brief Останавливает систему событий.
*/
void event_system_shutdown();

/*
    @brief Регистрирует функцию-обработчик на заданное событие.
    @param code Код события.
    @param listener Указатель на слушателя события, может быть null.
    @param handler Функция обработчик события.
    @return True при успешном добавлении слушателя, false если слушатель уже существует.
*/
KAPI bool event_register(event_code code, void* listener, PFN_event_handler handler);

/*
    @brief Снимает регистрацию функцию-обработчика заданного события.
    @param code Код события.
    @param listener Указатель на слушателя события, может быть null.
    @param handler Функция обработчик события.
    @return True при успешном удалении слушателя, false если слушатель не существует.
*/
KAPI bool event_unregister(event_code code, void* listener, PFN_event_handler handler);

/*
    @brief Создает событие с заданным кодом события и его контекстом.
    @param code Код события.
    @param sender Указатель на отправителя события, может быть null.
    @param context Указатель на контекст события, может быть null.
    @return True в случае обработки события, false не определено.
*/
KAPI bool event_send(event_code code, void* sender, event_context* context);

/*
    @brief По коду события возвращает символьную строку.
    @param code Код события.
    @return Символьная строка.
*/
KAPI const char* event_code_str(event_code code);
