#pragma once

#include <defines.h>

// @brief Контекст мьютекса, предоставляет эксклюзивный доступ к ресурсау для потоков.
typedef struct mutex {
    void* internal_data;
} mutex;

/*
    @brief Создает мьютекс.
    @param out_mutex Указатель на память для сохранения созданого мьютекса.
    @return True мьютекс успешно создан, false если не удалось.
*/
KAPI bool platform_mutex_create(mutex* out_mutex);

/*
    @brief Уничтожает предоставленный мьютекс.
    @param mutex Указатель на мьютекс который будет уничтожен.
*/
KAPI void platform_mutex_destroy(mutex* mutex);

/*
    @brief Устанавливает блокировку на указанный мтютекс.
    @param mutex Указатель на мьютекс который необходимо заблокировать.
    @return True мьютекс успешно заблокирован, false если не удалть.
*/
KAPI bool platform_mutex_lock(mutex* mutex);

/*
    @brief Снимает блокировку с указанного мьютекса.
    @param mutex Указатель на мьютекс который необходимо разблокировать.
    @return True мьютекс успешно разблокирован, false если не удалть.
*/
KAPI bool platform_mutex_unlock(mutex* mutex);

