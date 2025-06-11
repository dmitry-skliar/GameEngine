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
    @brief Рисует предоставленные геометрические данные.
    NOTE: Должно вызываться между началом и концом прохода визуализатора.
    @param data Указатель на геометрические данные для визуализации.
*/
void renderer_geometry_draw(geometry_render_data* data);

/*
    @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.
    @param s Указатель на шейдер для создания внутренних ресурсов.
    @param config Указатель на конфигурацию шейдера.
    @param pass Указатель проходчика визуализатора, который будет связан с шейдером.
    @param stage_count Количество стадий шейдера.
    @param stage_filenames Массив имен файлов стадий шейдера, которые будут загружены. Должен соотвествовать массиву стадий шейдера.
    @param stages Массив стадий шейдера (вершина, фрагмент и т.д), указывающий какие стадии будут использоваться в этом шейдере.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_shader_create(shader* s, const shader_config* config, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages);

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

/*
    @brief Начинает проход визуализатора с указанными параметрами.
    NOTE: Следует вызывать после функции начала кадра begin_frame, после чего вызываются функции отрисовки.
    @param pass Указатель на проходчик визуализатора для выполнения начала прохода.
    @param target Указатель на цель прохода визуализации.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_renderpass_begin(renderpass* pass, render_target* target);

/*
    @brief Завершает проход визуализатора с указанными параметрами.
    NOTE: Следует вызывать только после успешного выполнения begin_renderpass и функций отрисовки. 
    @param pass Указатель на проходчик визуализатора для выполнения завершения прохода.
    @return True операция завершена успешно, false в случае ошибок.
*/
bool renderer_renderpass_end(renderpass* pass);


/*
    @brief Получает проходчик визуализатора с указанным именем.
    @param name Имя проходчика визуализатора.
    @return Указатель на проходчик визуализатора, null если не удалось найти.
*/
renderpass* renderer_renderpass_get(const char* name);

/*
    @brief Указывает поддерживает ли визуализатор многопоточную работу.
    @return True многопоточность поддерживается, false не поддерживается.
*/
bool renderer_is_multithreaded();

/*
    @brief Создает новый буфера для заданной целей визуализатора.
    @param type Тип буфера, определяет цель использования (вершинные/индексные данные, uniforms переменные и т.д.).
    @param total_size Размер буфера в байтах.
    @param use_freelist Указывает, что буфер должен использовать freelist для распределения памяти.
    @param out_buffer Указатель для сохранения созданного буфера данных.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_create(renderbuffer_type type, ptr total_size, bool use_freelist, renderbuffer* out_buffer);

/*
    @brief Уничтожает предоставленный буфер визуализатора.
    @param buffer Указатель на буфер данных для уничтожения.
*/
void renderer_renderbuffer_destroy(renderbuffer* buffer);

/*
    @brief Устанавливает указанное смещение в предоставленном буфере.
    @param buffer Указатель на буфер данных для установки смещения.
    @param offset Смещение в байтах от начала буфера.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_bind(renderbuffer* buffer, ptr offset);

/*
    @brief Сбрасывает смещение в предоставленном буфере.
    @param buffer Указатель на буфер данных для сброса смещения.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_unbind(renderbuffer* buffer);

/*
    @brief Отображает память указанного буфера в блок памяти.
    @param buffer Указатель на буфер данных для отображения в память.
    @param offset Смещение в байтах от начала буфера данных для отображения.
    @param size Количество байт памяти от начала смещения для отображения.
    @return Указатель на отображенный блок памяти, после отмены отображения недействителен.
*/
void* renderer_renderbuffer_map_memory(renderbuffer* buffer, ptr offset, ptr size);

/*
    @brief Отменяет отображение указанного буфера в блок памяти.
    @note  После отмены отображения, указатель на блок памяти становится недействительным.
    @param buffer Указатель на буфер данных для отмены отображения в память.
    @param offset Смещение в байтах от начала буфера данных для отмены отображения.
    @param size Количество байт памяти от начала смещения для отмены отображения.
*/
void renderer_renderbuffer_unmap_memory(renderbuffer* buffer, ptr offset, ptr size);

/*
    @brief Синхронизирует память буфера в указанном диапазоне.
    @note  Должна выполняться после записи данных в буфер.
    @param buffer Указатель на буфер данных для синхронизации памяти.
    @param offset Смещение в байтах от начала буфера данных для синхронизации.
    @param size Количество байт памяти от начала смещения для синхронизации.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_flush(renderbuffer* buffer, ptr offset, ptr size);

/*
    @brief Считывает данные из предоставленного буфера в указанную память.
    @param buffer Указатель на буфер данных для выполнения считывания.
    @param offset Смещение в байтах от начала буфера данных для считывания.
    @param size Количество байт памяти от начала смещения для считывания.
    @param out_memory Указатель на память куда выполнить считывание, должна быть соответствующего размера.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_read(renderbuffer* buffer, ptr offset, ptr size, void* out_memory);

/*
    @brief Создает новый буфер с указанным размером, копирует из старого в новый, после чего уничтожает старый.
    @param buffer Указатель на буфер данных для изменения размера.
    @param new_total_size Новый размер буфера данных, должен быть больше текущего.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_resize(renderbuffer* buffer, ptr new_total_size);

/*
    @brief Пытается выделить память из предоставленного буфера c заданым смещением и размером блока.
    @note  Использовать бля буферов использующих freelist (use_freelist = true).
    @param buffer Указатель на буфер данных для выделения памяти.
    @param size Количество байт памяти которое необходимо выделить (от начала смещения out_offset).
    @param out_offset Указатель на переменную для сохранения смещения в байтах (от начала буфера данных).
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_allocate(renderbuffer* buffer, ptr size, ptr* out_offset);

/*
    @brief Пытается освободить память в предоставленном буфере с заданным смещением и размером блока.
    @note  Использовать бля буферов использующих freelist (use_freelist = true).
    @param buffer Указатель на буфер данных для освобождения памяти.
    @param size Количество байт памяти которое необходимо освободить (от начала смещения offset).
    @param offset Смещения в байтах от начала буфера данных по которому нужно освободить память.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_free(renderbuffer* buffer, ptr size, ptr offset);

/*
    @brief Загружает предоставленные данные в указанный диапазон предоставленного буфера.
    @param buffer Указатель на буфер данных для загрузки данных.
    @param offset Смещение в байтах от начала буфера данных для загрузки.
    @param size Количество байт памяти от начала смещения для загрузки.
    @param data Указатель на данные для загрузки.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_load_range(renderbuffer* buffer, ptr offset, ptr size, const void* data);

/*
    @brief Копирует данные в указанном диапазоне из исходного буфера в целевой.
    @param src Указатель на исходный буфер данных откуда копировать данные.
    @param src_offset Смещение в байтах от начала исходного буфера данных для копирования.
    @param dest Указатель на целевой буфер данных куда копировать данные.
    @param dest_offset Смещение в байтах от начала целевого буфера данных для копирования.
    @param size Количество байт памяти от начала смещений для копирования.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_copy_range(renderbuffer* src, ptr src_offset, renderbuffer* dest, ptr dest_offset, ptr size);

/*
    @brief Пытается нарисовать содержимое указанного буфера по указанному смещению и количеству элементов.
    @note  Предназначено только для использования с буферами вершин и индексов.
    @param buffer Указатель на буфер данных для выполнения отрисовки.
    @param offset Смещение в байтах от начала буфера данных для отрисовки.
    @param element_count Количество элементов, которое необходимо отрисовать.
    @param bind_only Указывает выполнить установку смещения, но не вызывать ортисовку.
    @return True в случае успеха, false в случае ошибки.
*/
bool renderer_renderbuffer_draw(renderbuffer* buffer, ptr offset, u32 element_count, bool bind_only);
