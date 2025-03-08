#pragma once

#include <defines.h>
#include <resources/resource_types.h>

// @brief Имя текстуры по умолчанию.
#define DEFAULT_TEXTURE_NAME "default"
#define DEFAULT_DIFFUSE_TEXTURE_NAME "default_diffuse"
#define DEFAULT_SPECULAR_TEXTURE_NAME "default_specular"
#define DEFAULT_NORMAL_TEXTURE_NAME "default_normal"

// @brief Конфигурация системы текстур.
typedef struct texture_system_config {
    // @brief Максимальное количество загружаемых текстур.
    u32 max_texture_count;
} texture_system_config;

/*
    @brief Инициализирует систему текстур используя предоставленную конфигурацию.
    @param memory_requirement Указатель на переменную для сохранения требований системы к памяти в байтах.
    @param memory Указатель на выделенный блок памяти, или null для получения требований.
    @param config Конфигурация используемая для инициализации системы и получения требований к памяти.
    @return True в случае успеха, false если есть ошибки.
*/
bool texture_system_initialize(u64* memory_requirement, void* memory, texture_system_config* config);

/*
    @brief Завершает работу системы текстур и освобождает выделеные ей ресурсы.
*/
void texture_system_shutdown();

/*
    @brief Пытается получить текстуру с указаным имененм и возвразает ее указатель.
    NOTE:  Если текстура не загружена в память, то выполняет ее загрузку, а если загружена, то увеличивает
           счетчик ссылок на данную текстуру.
    @param name Имя текстуры которую необходимо получить.
    @return Указатель на текстуру, или null если не была найдена.
*/
texture* texture_system_acquire(const char* name, bool auto_release);

/*
    @brief Пытается освобождить текстуру с указанным именем, игнорирует несуществующие текстуры.
    NOTE:  Уменьшает счетчик ссылок, если он равен нулю и авто освобождение было установлено, то
           текстура освобождает память и внутренние ресурсы графического процессора.
    @param name Имя текстуры которую необходимо освободить.
*/
void texture_system_release(const char* name);

/*
    @brief Возвращает указатель на текстуру по умолчанию.
*/
texture* texture_system_get_default_texture();

/*
    @brief Возвращает указатель на текстуру по умолчанию.
*/
texture* texture_system_get_default_diffuse_texture();

/*
    @brief Возвращает указатель на текстуру по умолчанию.
*/
texture* texture_system_get_default_specular_texture();

/*
    @brief Возвращает указатель на текстуру по умолчанию.
*/
texture* texture_system_get_default_normal_texture();
