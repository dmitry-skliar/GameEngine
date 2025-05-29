#pragma once

#include <defines.h>

// @brief Представляет контекст потока процесса.
typedef struct thread {
    u64 thread_id;
    void* internal_data;
} thread;

// @brief Указатель на функицю потока, которая будет вызвана при запуске потока.
typedef u32 (*PFN_thread_entry)(void*);

/*
    @brief Приостанавливает работку потока в котором вызывается на заданное время.
    @param time Время в миллисекундах на которое необходимо приостановить поток.
*/
KAPI void platform_thread_sleep(u64 time_ms);

/*
    @brief Возвращает количество логических ядер процессора.
    @return Количество логических ядер процессора.
*/
KAPI i32 platform_thread_get_processor_count();

/*
    @brief Создает новый поток и немедленно запускает его на выполнение.
    @param func Указатель на функцию, которую нужно выполнить в отдельном потоке.
    @param params Указатель на параметры для передечи в функцию потока. Передать 'null' если не используется.
    @param auto_detach Указывает, что поток создается отсоединенным и должен освободить свои ресурсы по завершении работы.
    @param out_thread Указатель на память для сохранения созданного потока (только если auto_detach = false).
    @return True поток успешно создан, false если не удалось.
*/
KAPI bool platform_thread_create(PFN_thread_entry func, void* params, bool auto_detach, thread* out_thread);

/*
    @brief Уничтожает указанный поток (принудительное завершение).
    @param thread Поток который будет уничтожен.
*/
KAPI void platform_thread_destroy(thread* thread);

/*
    @brief Отсоединяет поток, автоматически освобождая ресурсы по завершении работы.
    @param thread Поток который необходимо отсоеденить.
*/
KAPI void platform_thread_detach(thread* thread);

/*
    @brief Пытается завершить работу потока, и освободить ресурсы, если это возможно.
    @param thread Поток который необходимо попытаться завершить.
*/
KAPI void platform_thread_cancel(thread* thread);

/*
    @brief Проверяет активность потока в данный момент.
    @param thread Поток который необходимо проверить.
    @return True если поток выполняется, false в противном случае.
*/
KAPI bool platform_thread_is_active(thread* thread);

/*
    @brief Получает идентификатор потока.
    @return Идентификатор потока.
*/
KAPI u64 platform_thread_get_id();
