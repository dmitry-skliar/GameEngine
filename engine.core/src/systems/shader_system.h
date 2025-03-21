#pragma once

#include <defines.h>
#include <containers/hashtable.h>
#include <resources/resource_types.h>

// @brief Конфигурация системы шейдеров.
typedef struct shader_system_config {
    // @brief Максимальное число шейдеров хранящихся в сисетме.
    // NOTE: Должно быть не менее 512.
    u16 max_shader_count;
    // @brife Максимальное количество uniform переменных в одном шейдере.
    u8 max_uniform_count;
    // @brief Максимальное количество текстур глобальной области действия в одном шейдере.
    u8 max_global_textures;
    // @brief Максимальное количество текстур области действия экземпляра в одном шейдере.
    u8 max_instance_textures;
} shader_system_config;

// @brief Состояние шейдера.
typedef enum shader_state {
    // @brief Шейдер еще не создан.
    SHADER_STATE_NOT_CREATED,
    // @brief Шейдер создан, но не инициализирован (голов к конфигурации).
    SHADER_STATE_UNINITIALIZED,
    // @brief Шейдер создан и инициализирован (готов к выполнению).
    SHADER_STATE_INITIALIZED
} shader_state;

// @brief Переменая uniform.
typedef struct shader_uniform {
    // @brief Смещение переменой в байтах относительн начала буфера (глобального/экземпляров/локального).
    u64 offset;
    // @brief Местоположение для поиска переменой.
    // NOTE: Обычно совпадает с индексом, за исключением сэмплеров, которые используются для поиска индекс
    //       текстуры во внутреннем массиве в заданной области (глобальной/экземпляров).
    u16 location;
    // @brief Внутренний индекс uniform переменой.
    u16 index;
    // @brief Размер переменной, 0 для сэмплеров.
    u16 size;
    // @brief Индекс внутренего набора дескрипторов.
    // NOTE: 0 - глобальный, 1 - экземпляр, INVALID_ID - локальный.
    u8 set_index;
    // @brief Область действия переменой uniform.
    shader_scope scope;
    // @brief Тип переменой uniform.
    shader_uniform_type type;
} shader_uniform;

// @brief Атрибут вершиного шейдера.
typedef struct shader_attribute {
    // @brief Имя атрибута.
    char* name;
    // @brief Размер атрибута в байтах.
    u32 size;
    // @brief Тип атрибута.
    shader_attribute_type type;
} shader_attribute;

typedef struct shader {
    // @brief Идетификатор шейдера.
    u32 id;
    // @brief Имя шейдера.
    char* name;
    // @brief Указывает на использование uniform переменых уровня экземпляра.
    bool use_instances;
    // @brief Указывает на использование uniform переменых локального уровня.
    bool use_locals;
    // @brief Запрашиваемое выравнивание в байтах объекта uniform-буфера.
    u64 required_ubo_alignment;
    // @brief Размер глобального объекта uniform-буфера.
    u64 global_ubo_size;
    // @brief Шаг глобального объекта uniform-буфера (расстояние между объектами).
    u64 global_ubo_stride;
    // @brief Смещение в байтах от начала uniform-буфера.
    u64 global_ubo_offset;
    // @brief Размер экземпляра объекта uniform-буфера.
    u64 ubo_size;
    // @brief Шаг экземпляра объекта uniform-буфера.
    u64 ubo_stride;
    // @brief Размер всех диапазонов push-констант, объединенных вместе.
    u64 push_constant_size;
    // @brief Шаг push-констант (выравнивание на 4 байта для Vulkan).
    u64 push_constant_stride;
    // @brief Массив указателей глобальных карт текстур (используется darray).
    texture_map** global_texture_maps;
    // @brief Количество текстур экземпляров.
    u8 instance_texture_count;
    // @brief Текущая область действия для связывания.
    shader_scope bound_scope;
    // @brief Текущий идентификатор экземпляра для связывания.
    u32 bound_instance_id;
    // @brief Текущий размер объекта uniform-буфера для связывания.
    u32 bound_ubo_offset;
    // @brief Требования хэш-таблицы для объектов uniform-буфера.
    u64 uniform_lookup_memory_requirement;
    // @brief Блок памяти хэш-таблицы для объектов uniform-буфера.
    void* uniform_lookup_memory;
    // @brief Хэш-таблица индексов/местоположений по имени uniform переменой.
    hashtable* uniform_lookup;
    // @brief Массив uniform переменых (используется darray).
    shader_uniform* uniforms;
    // @brief Массив атрибутов (используется darray).
    shader_attribute* attributes;
    // @brief Внутреннее состояние шейдера.
    shader_state state;
    // @brief Количество диапазонов push-констант.
    u8 push_constant_range_count;
    // @brief Массив диапазонов push-констант.
    range push_constant_ranges[32];
    // @brief Размер всех атрибутов вместе взятых (размер вершины).
    u16 attribute_stride;
    // @brief Номер кадра для синхронизации (исключает повторный вызов в текущем кадре). 
    u64 render_frame_number;
    // @brief Внутренние данные специфичные для API рендера (Не трогать). // TODO: Спрятать!
    void* internal_data;
} shader;

/*
    @brief Инициализирует систему шейдеров используюя предоставленную конфигурацию.
    NOTE: Вызывать дважды, первый раз для получения требований к памяти, второй для инициализации.
    @param memory_requirement Указатель для хранения требований системы к памяти в байтах.
    @param memory Указатель на выделенный блок памяти, или null для получения требований.
    @param config Конфигурация используемая для инициализации системы и получения требований к памяти.
    @return True в случае успеха, false если есть ошибки.
*/
bool shader_system_initialize(u64* memory_requirement, void* memory, shader_system_config* config);

/*
    @brief Завершает работу системы шейдеров и освобождает выделеные ей ресурсы.
*/
void shader_system_shutdown();

/*
    @brief Создает новый шейдер по заданной конфигурацией.
    @param config Конфигурация используемая для создания шейдера.
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_create(const shader_config* config);

/*
    @brief Уничтожает шейдер по заданному имени.
    @param shader_name Имя шейдера (чувствительно к регистру).
*/
KAPI void shader_system_destroy(const char* shader_name);

/*
    @brief Получает индентификатор шейдера по заданному имени.
    @param shader_name Имя шейдера (чувствительно к регистру).
    @return Идентификатор шейдера, INVALID_ID если не найден.
*/
KAPI u32 shader_system_get_id(const char* shader_name);

/*
    @brief Возвращает указатель на шейдер по заданному идентификатору.
    @param shader_id Идентификатор шейдера.
    @return Указатель на шейдер, в противном случае null.
*/
KAPI shader* shader_system_get_by_id(u32 shader_id);

/*
    @brief Возвращает указатель на шейдер по заданному именем.
    @param shader_name Имя шейдера (чувствительно к регистру).
    @return Указатель на шейдер, в противном случае null.
*/
KAPI shader* shader_system_get(const char* shader_name);

/*
    @brief Использует шейдер по заданному именем.
    @param shader_name Имя шейдера который будет использоваться (чувствительно к регистру).
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_use(const char* shader_name);

/*
    @brief Использует шейдер по заданному идентификатору.
    @param shader_id Идентификатор шейдера который будет использоваться.
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_use_by_id(u32 shader_id);

/*
    @brief Возращает индекс uniform переменой по заданому имени.
    @param s Указатель на шейдер для получения индекса.
    @param uniform_name Имя uniform переменой.
    @return Индекс, INVALID_ID_U16 если индекс не найден.
*/
KAPI u16 shader_system_uniform_index(shader* s, const char* uniform_name);

/*
    @brief Задает значение uniform переменой по заданому имени и значению.
    NOTE: Действует для используемого шейдера в данный момент.
    @param uniform_name Имя uniform переменой которую нужно задать.
    @param value Значение которое необходимо задать.
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_uniform_set(const char* uniform_name, const void* value);

/*
    @brief Задает значение uniform переменой по заданому индексу и значению.
    NOTE: Действует для используемого шейдера в данный момент.
    @param index Индекс uniform переменой которую нужно задать.
    @param value Значение которое необходимо задать.
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_uniform_set_by_index(u16 index, const void* value);

/*
    @brief Задает тукстуру сэмплеру по заданному имени и предоставленной текстуре.
    NOTE: Действует для используемого шейдера в данный момент.
    @param sampler_name Имя uniform переменой для установки текстуры.
    @param t Указатель на текстуру которую нужно установить.
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_sampler_set(const char* sampler_name, const texture* t);

/*
    @brief Задает тукстуру сэмплеру по заданному индексу и предоставленной текстуре.
    NOTE: Действует для используемого шейдера в данный момент.
    @param index Индекс uniform переменой для установки текстуры.
    @param t Указатель на текстуру которую нужно установить.
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_sampler_set_by_index(u16 index, const struct texture* t);

/*
    @brief Применяет изменения uniform переменных глобальной области действия.
    NOTE: Действует для используемого шейдера в данный момент.
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_apply_global();

/*
    @brief Применяет изменения uniform переменных области действия экземпляра.
    NOTE: Действует для используемого шейдера в данный момент.
    @param needs_update Указывает, на необходимость обновить uniform переменные или связать их.
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_apply_instance(bool needs_update);

/*
    @brief Связывает экземпляр с указаным идентификатором для использлвания.
           Необходимо выполнить перед установкой uniform переменых области действия экземпляра.
    NOTE: Действует для используемого шейдера в данный момент.
    @param instance_id Идентификатор экземпляра для привязки.
    @return True в случае успеха, false если есть ошибки.
*/
KAPI bool shader_system_bind_instance(u32 instance_id);
