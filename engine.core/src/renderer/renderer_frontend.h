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
    @param t Указатель на текстуру которую необходимо создать.
    @param pixels Указатель на необработанные данные изображения для загрузки.
*/
void renderer_texture_create(texture* t, const void* pixels);

/*
    @brief Создает новую записываемую текстуру без записанных в нее данных.
    @param t Указатель на текстуру для получения ресурсов.
*/
void renderer_texture_create_writable(texture* t);

/*
    @brief Изменяет размер текстуры. На этом уровне нет проверки на возможность
           записи текстуры. Внутренние ресурсы уничтожаются и создаются заново с
           новым разрешением. Данные теряются и должны быть перезагружены.
    @param t Указатель на текстуру для изменения размера.
    @param new_width Новая ширина в пикселях.
    @param new_height Новая высота в пикселях.
*/
void renderer_texture_resize(texture* t, u32 new_width, u32 new_height);

/*
    @brief Записывает указанные данные в предоставленную текстуру.
    NOTE: На этом уровне это может быть как записываемая, так и не записываемая
          текстура, поскольку она также обрабатывает начальную загрузку текстуры.
          Сама система текстур должна отвечать за блокировку запросов на запись
          в не записываемые текстуры.
    @param t Указатель на текстуру для записи данных.
    @param offset Смещение в байтах откуда начать запись данных.
    @param size Количество байт данных для записи.
    @param pixels Необработаные данные изображения (пиксели) которые будут записаны.
*/
void renderer_texture_write_data(texture* t, u32 offset, u32 size, const void* pixels);

/*
    @brief Уничтожает предоставленную текстуру, освобождая память графического процессора.
    @param t Указатель на текстуру, которую необходимо уничтожить.
*/
void renderer_texture_destroy(texture* t);

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
bool renderer_geometry_create(
    geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count,
    const void* indices
);

/*
    @brief Уничтожает предоставленную геометрию, освобождая ресурсы графического процессора.
    @param geometry Указатель на геометрию, которую необходимо уничтожить.
*/
void renderer_geometry_destroy(geometry* geometry);

/*
    @brief Получает проходчик визуализатора с указанным именем.
    @param name Имя проходчика визуализатора.
    @return Указатель на проходчик визуализатора, null если не удалось найти.
*/
renderpass* renderer_renderpass_get(const char* name);

/*
    @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.
    @param s Указатель на шейдер для создания внутренних ресурсов.
    @param pass Указатель проходчика визуализатора, который будет связан с шейдером.
    @param stage_count Количество стадий шейдера.
    @param stage_filenames Массив имен файлов стадий шейдера, которые будут загружены. Должен соотвествовать массиву стадий шейдера.
    @param stages Массив стадий шейдера (вершина, фрагмент и т.д), указывающий какие стадии будут использоваться в этом шейдере.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_create(shader* s, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages);

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
    @brief Создает новую цель визуализации используя предоставленные данные.
    @apram attachment_count Количество вложений (указателей на текстуры).
    @param attachments Массив вложений (указатели текстур).
    @param pass Указатель на проходчик визуализатора, с которым связана цель визуализации.
    @param width Ширина цели визуализации в пикселях.
    @param height Высота цели визуализации в пикселях.
    @param out_target Указатель на структуру render_target для получения новой цели визуализации.
*/
void renderer_render_target_create(u8 attachment_count, texture** attachments, renderpass* pass, u32 width, u32 height, render_target* out_target);

/*
    @brief Уничтожает предоставленную цель визуализации.
    @param out_target Указатель на структуру render_target для уничтожения цели визуализации.
    @param free_internal_memory Указывает, следует ли освободить внутреннюю память.
*/
void renderer_render_target_destroy(render_target* target, bool free_internal_memory);

/*
    @brief Создает новый проходчик визуализации.
    @param out_renderpass Указатель на проходчик визуализации.
    @param depth Значение очистки глубины.
    @param stencil Значение очистки трафарета.
    @param has_prev_pass Указывает, есть ли предыдущий проход визуализации.
    @param has_next_pass Указывает, есть ли следующий проход визуализации.
*/
void renderer_renderpass_create(renderpass* out_renderpass, f32 depth, u32 stencil, bool has_prev_pass, bool has_next_pass);

/*
    @brief Уничтожает предоставленный проходчик визуализации.
    @param pass Указатель на проходчик визуализации для уничтожения.
*/
void renderer_renderpass_destroy(renderpass* pass);
