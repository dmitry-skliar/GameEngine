#pragma once

#include <defines.h>
#include <renderer/camera.h>

#define DEFAULT_CAMERA_NAME "default_camera"

// @brief Конфигурация системы камер.
typedef struct camera_system_config {
    // @brief Максимальное количество камер.
    u16 max_camera_count;
} camera_system_config;

/*
    @brief Инициализирует систему камер используя предоставленную конфигурацию.
    @param memory_requirement Указатель на переменную для сохранения требований системы к памяти в байтах.
    @param memory Указатель на выделенный блок памяти, или null для получения требований.
    @param config Конфигурация используемая для инициализации системы и получения требований к памяти.
    @return True в случае успеха, false если есть ошибки.
*/
bool camera_system_initialize(u64* memory_requirement, void* memory, camera_system_config* config);

/*
    @brief Завершает работу системы камер и освобождает выделеные ей ресурсы.
*/
void camera_system_shutdown();

/*
    @brief Получает камеру с указаным имененм и возвращает указатель на нее.
    @param name Имя камеры которую необходимо получить.
    @return Указатель на камеру, или null если есть ошибки.
*/
KAPI camera* camera_system_acquire(const char* name);

/*
    @brief Освобождает камеру с указанным именем, игнорирует несуществующие камеры.
    NOTE:  Уменьшает счетчик ссылок, если он равен нулю и авто освобождение было установлено, то камера освобождает память.
    @param name Имя камеры которую необходимо освободить.
*/
KAPI void camera_system_release(const char* name);

/*
    @brief Возвращает указатель на камеру по умолчанию.
    @return Указатель на камеру по умолчанию.
*/
KAPI camera* camera_system_get_default();
