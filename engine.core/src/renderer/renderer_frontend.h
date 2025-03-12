#pragma once

#include <renderer/renderer_types.h>
#include <systems/shader_system.h>
#include <platform/window.h>

/*
    @brief Инициализирует интерфейс и систему рендеринга.
    NOTE: Вызывать дважды, первый раз для получения требований к памяти, второй для инициализации.
    @param memory_requirement Указатель на переменную для сохранения требований системы к памяти в байтах.
    @param memory Указатель на выделенный блок памяти, или null для получения требований.
    @param window_state Указатель на выделенную память экземпляра оконной системы.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_system_initialize(u64* memory_requirement, void* memory, window* window_state);

/*
    @brief Завершает работу системы рендеринга и освобождает выделеные ей ресурсы.
*/
void renderer_system_shutdown();

/*
    @brief Изменяет размер области рендеринга, обычно вызывается на событие изменение размера окна.
    @param width Новая ширина области рендеринга.
    @param height Новая высота области рендеринга.
*/
void renderer_on_resize(i32 width, i32 height);

/*
    @brief Рисует следующий кард используя предоставленный пакет рендеринга.
    @param packet Указатель на пакет рендеринга.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_draw_frame(render_packet* packet);

// HACK: Временно!
KAPI void renderer_set_view(mat4 view, vec3 view_position);

/*
    @brief Создает новую текстуру по предоставленным данным изображения и загружает в память графического процессора.
    @param texture Указатель на текстуру которую необходимо создать.
    @param pixels Указатель на необработанные данные изображения для загрузки.
*/
void renderer_create_texture(texture* texture, const void* pixels);

/*
    @brief Уничтожает предоставленную текстуру, освобождая память графического процессора.
    @param Указатель на текстуру, которую необходимо уничтожить.
*/
void renderer_destroy_texture(texture* texture);

/*
    @brief Получает ресурсы графического процессора и загружает данные геометрии.
    @param geometry Указатель на геометрию для которой необходимо получить ресурсы.
    @param vertex_size Размер вершин в геометрии.
    @param vertex_count Количество вершин в геометрии.
    @param vertices Массив вершин геометрии.
    @param index_size Размер индекса в геометрии.
    @param index_count Количество индексов в геометрии.
    @param indices Массив индексов геометрии.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_create_geometry(
    geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count,
    const void* indices
);

/*
    @brief Уничтожает предоставленную геометрию, освобождая ресурсы графического процессора.
    @param geometry Указатель на геометрию, которую необходимо уничтожить.
*/
void renderer_destroy_geometry(geometry* geometry);

/*
    @brief Получает идентификатор прохода рендеринга с указанным именем.
    @param name Имя прохода ренедринга, для получения идентификатора прохода рендеринга.
    @param out_renderpass_id Указатель на переменую для сохранения идетификатора прохода рендеринга.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_renderpass_id(const char* name, u8* out_renderpass_id);

/*
    @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.
    @param s Указатель на шейдер для создания внутренних ресурсов.
    @param renderpass_id Указатель прохода рендеринга, который будет связан с шейдером.
    @param stage_count Количество стадий шейдера.
    @param stage_filenames Массив имен файлов стадий шейдера, которые будут загружены. Должен соотвествовать массиву стадий шейдера.
    @param stages Массив стадий шейдера (вершина, фрагмент и т.д), указывающий какие стадии будут использоваться в этом шейдере.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_create(shader* s, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages);

/*
    @brief Уничтожает предоставленный шейдер и освобождает ресурсы им удерживаемые.
    @param s Указатель на шейдер, который необходимо уничтожить.
*/
void renderer_shader_destroy(shader* s);

/*
    @brief Инициализирует настроенный шейдер, должно быть выполено после 'renderer_shader_create',
           а так же последующей настройки шейдер.
    @param s Указатель на шейдер, который необходимо инициализировать.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_initialize(shader* s);

/*
    @brief Использует заданный шейдер, активируя его для обновления атрибутов, uniform перменных и т.д.
    @param s Указатель на шейдер, над которым будут проводиться операции.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_use(shader* s);

/*
    @brief Связывает глобальные ресурсы для использования и обновления.
    @param s Указатель на шейдер, глобальные ресурсы которого должны быть связаны.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_bind_globals(shader* s);

/*
    @brief Связывает ресурсы экземпляра для использования и обновления.
    @param s Указатель на шейдер, ресурсы экземпляра которого должны быть связаны.
    @param instance_id Идентификатор экземпляра, который должен быть связан.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_bind_instance(shader* s, u32 instance_id);

/*
    @brief Применяет данные к текущим привязанному глобальному ресурсу.
    @param s Указатель на шейдер к которому должны быть применены глобальные данные.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_apply_globals(shader* s);

/*
    @brief Применяет данные к текущим привязанному ресурсу экземпляра.
    @param s Указатель на шейдер к которому должны быть применены данные экземпляра.
    @param needs_update Указывает, на необходимость обновить uniform переменные или связать их.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_apply_instance(shader* s, bool needs_update);

/*
    @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.
    @param s Указатель на шейдер из которого нужно получить ресурсы экземпляра.
    @param maps Указатель на массив указателей карт текстур. Должен быть один на текстуру в экземпляре.
    @param out_instance_id Указатель на переменную для сохранения идентификатора экземпляра.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_acquire_instance_resources(shader* s, texture_map** maps, u32* out_instance_id);

/*
    @brief Освобождает внутренние ресурсы уровня экземпляра по предоставленному идентификатору экземпляра.
    @param s Указатель на шейдер для которого нужно освободить ресурсы экземпляра.
    @param instance_id Идентификатор экземпляра, ресурсы которого необходимо освободить.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_release_instance_resources(shader* s, u32 instance_id);

/*
    @brief Задает значение uniform переменой указанного шейдера.
    @param s Указатель на шейдер для которого нужно задать uniform переменую.
    @parma uniform Указатель на uniform переменную значение которой необходимо задать.
    @param value Указатель на значение которое необходимо задать uniform переменной.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_set_uniform(shader* s, shader_uniform* uniform, const void* value);

/*
    @brief Получает внутренние ресурсы для предоставленной карты текстуры.
    @param map Указатель на карту текстуры для получения ресурсов.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_texture_map_acquire_resources(texture_map* map);

/*
    @brief Освобождает внутренние русурсы для предоставленной карты текстуры.
    @param map Указатель на карту текстуры для освобождения ресурсов.
*/
void renderer_texture_map_release_resources(texture_map* map);
