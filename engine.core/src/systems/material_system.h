#pragma once

#include <defines.h>
#include <resources/resource_types.h>

// @brief Имя материала по умолчанию.
#define DEFAULT_MATERIAL_NAME "default"

// @brief Конфигурация системы материалов.
typedef struct material_system_config {
    // @brief Максимальное количество загружаемых материалов.
    u32 max_material_count;
} material_system_config;

/*
    @brief Инициализирует систему материалов используя предоставленную конфигурацию.
    @param memory_requirement Указатель на переменную для сохранения требований системы к памяти в байтах.
    @param memory Указатель на выделенный блок памяти, или null для получения требований.
    @param config Конфигурация используемая для инициализации системы и получения требований к памяти.
    @return True в случае успеха, false если есть ошибки.
*/
bool material_system_initialize(u64* memory_requirement, void* memory, material_system_config* config);

/*
    @brief Завершает работу системы материалов и освобождает выделеные ей ресурсы.
*/
void material_system_shutdown();

/*
    @brief Пытается получить материал с указаным имененм, если такого нет возвращает материал по умолчанию.
    NOTE:  Если материал не загружен в память, то выполняет его загрузку, а если загружен, то увеличивается
           счетчик ссылок на данный материал.
    @param name Имя материала который необходимо получить.
    @return Указатель на материал, или указатель на материал по умолчанию, если не был найден.
*/
material* material_system_acquire(const char* name);

/*
    @brief Пытается получить материал c использованием заданной конфигурации, если это невозможно, возвращает
           материал по умолчанию.
    NOTE:  Если материал не загружен в память, то выполняет его загрузку, а если загружен, то увеличивается
           счетчик ссылок на данный материал.
    @param config Конфигурация материала для загрузки.
    @return Указатель на материал, или указатель на материал по умолчанию, если не был найден.
*/
material* material_system_acquire_from_config(material_config* config);

/*
    @brief Пытается освобождить материал с указанным именем, игнорирует несуществующие материалы.
    NOTE:  Уменьшает счетчик ссылок, если он равен нулю и авто освобождение было установлено, то
           материал освобождает память и внутренние ресурсы графического процессора.
    @param name Имя материала который необходимо освободить.
*/
void material_system_release(const char* name);

/*
    @brief Возвращает указатель на материал по умолчанию.
*/
material* material_system_get_default();

/*
    @brief Применяет глобальные данные для указаного идентификатора шейдера.
    @param shader_id Идентификатор шейдера, для которого необходимо применить глобальные данные.
    @param projection Указатель на матрицу проекции.
    @param view Указатель на матрицу вида.
    @param view_position Указатель на позицию камеры.
    @param ambient_color Указатель на окружающий цвет сцены.
    @return True в случае успеха, false если есть ошибки.
*/
bool material_system_apply_global(u32 shader_id, const mat4* projection, const mat4* view, const vec3* view_position, const vec4* ambient_color);

/*
    @brief Применяет данные материала на уровне экземпляра для предоставленного материала.
    @param m Указатель на материал для которого нужно применить данные.
    @return True в случае успеха, false если есть ошибки.
*/
bool material_system_apply_instance(material* m);

/*
    @brief Применяет данные материала на локальном уровне для предоставленного материала.
    @param m Указатель на материал для которого нужно применить данные.
    @param model Указатель на матрицу модели, которая будет применена.
    @return True в случае успеха, false если есть ошибки.
*/
bool material_system_apply_local(material* m, const mat4* model);
