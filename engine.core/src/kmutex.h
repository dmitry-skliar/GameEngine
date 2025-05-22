#pragma once

#include <defines.h>
#include <platform/mutex.h>

/*
    @brief Создает мьютекс.
    @param out_mutex Указатель на память для сохранения созданого мьютекса.
    @return True мьютекс успешно создан, false если не удалось.
*/
#define kmutex_create(out_mutex) platform_mutex_create(out_mutex)

/*
    @brief Уничтожает предоставленный мьютекс.
    @param mutex Указатель на мьютекс который будет уничтожен.
*/
#define kmutex_destroy(mutex) platform_mutex_destroy(mutex)

/*
    @brief Устанавливает блокировку на указанный мтютекс.
    @param mutex Указатель на мьютекс который необходимо заблокировать.
    @return True мьютекс успешно заблокирован, false если не удалть.
*/
#define kmutex_lock(mutex) platform_mutex_lock(mutex)

/*
    @brief Снимает блокировку с указанного мьютекса.
    @param mutex Указатель на мьютекс который необходимо разблокировать.
    @return True мьютекс успешно разблокирован, false если не удалть.
*/
#define kmutex_unlock(mutex) platform_mutex_unlock(mutex)
