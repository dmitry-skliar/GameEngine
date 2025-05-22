#pragma once

#include <defines.h>
#include <platform/thread.h>

/*
    @brief Создает новый поток и немедленно запускает его на выполнение.
    @param func Указатель на функцию, которую нужно выполнить в отдельном потоке.
    @param params Указатель на параметры для передечи в функцию потока. Передать 'null' если не используется.
    @param auto_detach Указывает, что поток создается отсоединенным и должен освободить свои ресурсы по завершении работы.
    @param out_thread Указатель на память для сохранения созданного потока (только если auto_detach = false).
    @return True поток успешно создан, false если не удалось.
*/
#define kthread_create(func, params, auto_detach, out_thread) platform_thread_create(func, params, auto_detach, out_thread)

/*
    @brief Уничтожает указанный поток (принудительное завершение).
    @param thread Поток который будет уничтожен.
*/
#define kthread_destroy(thread) platform_thread_destroy(thread)

/*
    @brief Отсоединяет поток, автоматически освобождая ресурсы по завершении работы.
    @param thread Поток который необходимо отсоеденить.
*/
#define kthread_detach(thread) platform_thread_detach(thread)

/*
    @brief Пытается завершить работу потока, и освободить ресурсы, если это возможно.
    @param thread Поток который необходимо попытаться завершить.
*/
#define kthread_cancel(thread) platform_thread_cancel(thread)

/*
    @brief Проверяет активность потока в данный момент.
    @param thread Поток который необходимо проверить.
*/
#define kthread_is_active(thread) platform_thread_is_active(thread)

/*
    @brief Приостанавливает предоставленный поток на заданное время, должен вызываться из потока, требующего приостановки.
    @param thread Поток который необходимо приостановить.
    @param time_ms Время в миллисекундах на которое необходимо приостановить поток.
*/
#define kthread_sleep(thread, time_ms) platform_thread_sleep(time_ms)

/*
    @brief Получает идентификатор потока.
*/
#define kthread_get_id() platform_thread_get_id()
