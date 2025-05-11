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
    @brief Пытается получить кубическую текстуру с указаным имененм и возвразает ее указатель.
    NOTE:  Если текстура не загружена в память, то выполняет ее загрузку, а если не найдена
           вернет кубическую текстуру по умолчанию, а если загружена то увеличит счетчик ссылок.
    NOTE:  Кубическая текстура состоит из 6 граней и изображения должны быть подготовлены
           следующим образом: name_f - front, name_b - back, name_u - up, name_d - down, name_r - right,
           name_l - left, где name - это базисное имя, а _{f,b,u,d,r,l} - грани кубической текстуры.
    @param name Базисное имя кубической текстуры которую необходимо получить.
    @return Указатель на текстуру, или кубическая теустура по умолчанию если не была найдена.
*/
texture* texture_system_acquire(const char* name, bool auto_release);

/*
    @brief Пытается получить кубическую текстуру с указаным имененм и возвразает ее указатель.
    NOTE:  Если текстура не загружена в память, то выполняет ее загрузку, а если не найдена
           вернет кубическую текстуру по умолчанию, а если загружена то увеличит счетчик ссылок.
    NOTE:  Кубическая текстура состоит из 6 граней и изображения должны быть подготовлены
           следующим образом: name_f - front, name_b - back, name_u - up, name_d - down, name_r - right,
           name_l - left, где name - это базисное имя, а _{f,b,u,d,r,l} - грани кубической текстуры.
    @param name Базисное имя кубической текстуры которую необходимо получить.
    @return Указатель на текстуру, или кубическая теустура по умолчанию если не была найдена.
*/
texture* texture_system_acquire_cube(const char* name, bool auto_release);

/*
    @brief Пытается получить записываемую текстуру с указаным имененм и возвразает ее указатель.
    NOTE:  Не загружает текстуру из файла и не может быть автоматически освобожденной.
    @param name Имя текстуры которую необходимо получить.
    @param width Ширина текстуры в пикселях.
    @param height Высота текстуры в пикселях.
    @param channel_count Количество каналов в пикселе (1-4, RGBA).
    @param has_transparency Указывает, что текстура имеет прозрачность.
    @return Указатель на текстуру, или null если не была найдена.
*/
texture* texture_system_acquire_writable(const char* name, u32 width, u32 height, u8 channel_count, bool has_transparency);

/*
    @brief Пытается освобождить текстуру с указанным именем, игнорирует несуществующие текстуры.
    NOTE:  Уменьшает счетчик ссылок, если он равен нулю и авто освобождение было установлено, то
           текстура освобождает память и внутренние ресурсы графического процессора.
    @param name Имя текстуры которую необходимо освободить.
*/
void texture_system_release(const char* name);

/*
    @brief Оборачивает предоставленные внутренние данные и параметры в текстуру.
    NOTE:  Обернутые текстуры не освобождаются автоматически.
    @param name Имя текстуры которую необходимо получить.
    @param width Ширина текстуры в пикселях.
    @param height Высота текстуры в пикселях.
    @param channel_count Количество каналов в пикселе (1-4, RGBA).
    @param has_transparency Указывает, что текстура имеет прозрачность.
    @param is_writable Указывает, что текстура записываемая.
    @param register_texture Указывает, следует ли регистрировать текстуру в системе.
    @param internal_data Указатель на внутренние данные, которые необходимо обернуть.
    @return Указатель на обернутую текстуру, или null если обернуть не получилось.
*/
texture* texture_system_wrap_internal(
    const char* name, u32 width, u32 height, u8 channel_count, bool has_transparency, bool is_writable,
    bool register_texture, void* internal_data
);

/*
    @brief Устанавливает внутренние данные текстуры. Полезно для замены внутренних данных рендера.
    @param t Указатель на текстуру для обновления внутренних данных рендера.
    @param internal_data Указатель на внутренние данные для установки.
    @return True в случае успеха, false если есть ошибки.
*/
bool texture_system_set_internal(texture* t, void* internal_data);

/*
    @brief Изменяет размера предоставленной записываемой текстуры.
    @param t Указатель на текстуру для изменения размера.
    @param width Новая ширина текстуры в пикселях.
    @param height Новая высота текстуры в пикселях.
    @param regenerate_internal_data Указывает, следует ли регенерировать внутренние данные.
    @return True в случае успеха, false если есть ошибки.
*/
bool texture_system_resize(texture* t, u32 width, u32 height, bool regenerate_internal_data);

/*
    @brief Записывает в заданную записываемую текстуру предоставленные данные.
    @param t Указатель на текстуру для записи.
    @param offset Смещение в байтах от начала данных, которые нужно записать.
    @param size Количество байтов данных, которые нужно записать.
    @param data Указатель на данные которые нужно записать.
    @return True в случае успеха, false если есть ошибки.
*/
bool texture_system_write_data(texture* t, u32 offset, u32 size, void* data);

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
