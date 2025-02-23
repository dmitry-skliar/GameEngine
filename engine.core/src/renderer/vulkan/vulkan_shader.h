#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <renderer/vulkan/vulkan_types.h>

/*
    @brief Создает новый шейдер с использованием предоставленных параметров.
    NOTE: Должно использоваться только после инициализации шейдера (см. vulkan_shader_init).
    @param context Указатель на контекст Vulkan.
    @param name Имя шейдера. Используется для открытия объектных шейдеров SPIR-V файлов.
    @param renderpass Указатель на проход визуализатора который будет использовать шайдер.
    @param stages Комбинация битовых флагов указывающий этап визуализации (vertex, fregment, и др.).
    @param max_descriptor_set_count Максимальное количество наборов дескрипторов.
    @param use_instances Указывает на использование экземпляров uniform.
    @param use_local Указывает на использование локальных uniform (используются push constants).
    @param out_shader Указатель на новый шейдер.
    @return True если шейдер создан успешно, false если не удалось.
*/
bool vulkan_shader_create(
    vulkan_context* context, const char* name, vulkan_renderpass* renderpass, VkShaderStageFlags stages,
    u16 max_descriptor_set_count, bool use_instances, bool use_local, vulkan_shader* out_shader);

/*
    @brief Освобождает предоставленный шейдер.
    @param Указатель на шейдер который нужно освободить.
    @return True при успехе, false при неудаче.
*/
bool vulkan_shader_destroy(vulkan_shader* shader);

/*
    @brief Добавляет новый вершинный атрибут.
    NOTE: Должно использоваться только после инициализации шейдера (см. vulkan_shader_init).
    @param shader Указатель на шейдер куда добавлять.
    @param name Имя атрибута.
    @param type Тип атрибута.
    @return True при успехе, false при неудаче.
*/
bool vulkan_shader_add_attribute(vulkan_shader* shader, const char* name, shader_attribute_type type);

/*
    @brief Добавляет текстурный сэмплер в шейдер.
    NOTE: Должно использоваться только после инициализации шейдера (см. vulkan_shader_init).
    @param shader Указатель на шейдер куда добавлять.
    @param sampler_name Имя сэмплера.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего атрибута.
*/
bool vulkan_shader_add_sampler(vulkan_shader* shader, const char* sampler_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новое знаковое 8-bit целое в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_i8(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новое знаковое 16-bit целое в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_i16(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новое знаковое 32-bit целое в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_i32(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новое беззнаковое 8-bit целое в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_u8(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новое беззнаковое 16-bit целое в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_u16(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новое беззнаковое 32-bit целое в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_u32(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новое вещественное 32-bit с плавающей точкой в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_f32(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новый 2х элементный вектор (2x 32-bit floats) в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_vec2(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новый 3х элементный вектор (3x 32-bit floats) в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_vec3(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новый 4х элементный вектор (4x 32-bit floats) в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_vec4(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет новую матрицу 4x4 (16x 32-bit floats) в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_mat4(vulkan_shader* shader, const char* uniform_name, shader_scope scope, u32* out_location);

/*
    @brief Добавляет произвольный тип в uniform шейдера.
    @param shader Указатель на шейдер куда добавлять.
    @param uniform_name Имя uniform.
    @param size Размер произвольного типа в байтах.
    @param scope Область применения.
    @param out_location Указатель для хранения будущего uniform.
*/
bool vulkan_shader_add_uniform_custom(vulkan_shader* shader, const char* uniform_name, u32 size, shader_scope scope, u32* out_location);

/*
    @brief Инициализарует настроенный шейдер. Автоматически освобождается при ошибке на этом шаге.
    NOTE: Должно использоваться только после создания шейдера (см. vulkan_shader_create).
    @param shader Указатель на шейдер для инициализации.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_initialize(vulkan_shader* shader);

/*
    @brief Использует, активирует шейдер для обновления атрибутов, uniform и т.п., и для операций рисования.
    @param shader Указатель на шейдер для использования.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_use(vulkan_shader* shader);

/*
    @brief Связывает глобальные ресурсы для использования и обновления.
    @param shader Указатель на шейдер, для которого нужно связать переменные.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_bind_globals(vulkan_shader* shader);

/*
    @brief Связывает ресурсы экземпляров для использования и обновления.
    @param shader Указатель на шейдер, для которого нужно связать переменные.
    @param instance_id Идентификатор экземпляра для связывания.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_bind_instance(vulkan_shader* shader, u32 instance_id);

/*
    @brief Применяет глобальные данные к uniform буферам.
    @param shader Указатель на шейдер, для которого нужно применить данные.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_apply_globals(vulkan_shader* shader);

/*
    @brief Применяет данные к привязанному в данный момент экземпляру.
    @param shader Указатель на шейдер, для которого нужно применить данные.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_apply_instance(vulkan_shader* shader);

/*
    @brief Получает внутренние ресурсы уровня экземпляра и предоставляет его идентификатор.
    @param shader Указатель на шейдер, для которого получить ресурсы.
    @param out_instance_id Указатель для сохранения нового идентификатора экземпляра.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_acquire_instance_resources(vulkan_shader* shader, u32* out_instance_id);

/*
    @brief Освобождает внутренние ресурсы уровня экземпляра по предоставленному идентификатору.
    @param shader Указатель на шейдер, для которого освободить ресурсы.
    @param instance_id Идентификатор экземпляра для которого нужно освободить ресурсы.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_release_instance_resources(vulkan_shader* shader, u32 instance_id);

/*
    @brief Пытается получить местоположение uniform для заданного имени. Uniforms и samplers
           имеют местоположение, независимо от области действия.
    @param shader Указатель на шейдер, из которого нужно получить местоположение.
    @param uniform_name Имя uniform-ы.
    @return Местоположение в случае успеха, INVALID_ID в случае неудачи.
*/
u32 vulkan_shader_uniform_location(vulkan_shader* shader, const char* uniform_name);

/*
    @brief Устанавливает сэмплер в указанном местоположении для использования предоставленной текстуры.
    @param shader Указатель на шейдер, для которого нужно установить сэмплер.
    @param location Местоположение для установки сэмплера.
    @param t Указатель на текстуру, которая будет назначена сэмплеру.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_sampler(vulkan_shader* shader, u32 location, texture* t);

/*
    @brief Устанавливает знакового 8-bit целого значения в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_i8(vulkan_shader* shader, u32 location, i8 value);

/*
    @brief Устанавливает знакового 16-bit целого значения в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_i16(vulkan_shader* shader, u32 location, i16 value);

/*
    @brief Устанавливает знакового 32-bit целого значения в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_i32(vulkan_shader* shader, u32 location, i32 value);

/*
    @brief Устанавливает беззнакового 8-bit целого значения в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_u8(vulkan_shader* shader, u32 location, u8 value);

/*
    @brief Устанавливает беззнакового 16-bit целого значения в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_u16(vulkan_shader* shader, u32 location, u16 value);

/*
    @brief Устанавливает беззнакового 32-bit целого значения в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_u32(vulkan_shader* shader, u32 location, u32 value);

/*
    @brief Устанавливает вещественного 32-bit значения с плавающей точкой в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_f32(vulkan_shader* shader, u32 location, f32 value);

/*
    @brief Устанавливает 2х элементное векторное значение (2х 32-bit float) в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_vec2(vulkan_shader* shader, u32 location, vec2 value);

/*
    @brief Устанавливает 2х элементное векторное значение (2х 32-bit float) в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value_0 Первое значение для установки.
    @param value_1 Второе значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_vec2f(vulkan_shader* shader, u32 location, f32 value_0, f32 value_1);

/*
    @brief Устанавливает 3х элементное векторное значение (3х 32-bit float) в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_vec3(vulkan_shader* shader, u32 location, vec3 value);

/*
    @brief Устанавливает 3х элементное векторное значение (3х 32-bit float) в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value_0 Первое значение для установки.
    @param value_1 Второе значение для установки.
    @param value_2 Третье значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_vec3f(vulkan_shader* shader, u32 location, f32 value_0, f32 value_1, f32 value_2);

/*
    @brief Устанавливает 4х элементное векторное значение (4х 32-bit float) в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_vec4(vulkan_shader* shader, u32 location, vec4 value);

/*
    @brief Устанавливает 4х элементное векторное значение (4х 32-bit float) в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value_0 Первое значение для установки.
    @param value_1 Второе значение для установки.
    @param value_2 Третье значение для установки.
    @param value_3 Четвертое значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_vec4f(vulkan_shader* shader, u32 location, f32 value_0, f32 value_1, f32 value_2, f32 value_3);

/*
    @brief Устанавливает значения матрицы 4x4 (16х 32-bit float) в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Значение для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_mat4(vulkan_shader* shader, u32 location, mat4 value);

/*
    @brief Устанавливает значение произвольного типа в указанном месте.
    @param shader Указатель на шейдер, для установки значения.
    @param location Местоположение для установки значения.
    @param value Указатель на значение произвольного типа для установки.
    @return True в случае успеха, false в случае неудачи.
*/
bool vulkan_shader_set_uniform_custom(vulkan_shader* shader, u32 location, void* value);
