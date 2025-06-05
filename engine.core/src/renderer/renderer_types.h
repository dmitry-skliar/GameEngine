#pragma once

#include <defines.h>
#include <platform/window.h>
#include <math/math_types.h>
#include <resources/resource_types.h>

#define BUILTIN_SHADER_NAME_WORLD  "Builtin.MaterialShader"
#define BUILTIN_SHADER_NAME_UI     "Builtin.UIShader"
#define BUILTIN_SHADER_NAME_SKYBOX "Builtin.SkyboxShader"

// TODO: Подчистить заголовочные файлы данным способом!
struct shader;
struct shader_uniform;

// @brief Режимы отображения визуализации (для отладки).
typedef enum renderer_view_mode {
    RENDERER_VIEW_MODE_DEFAULT  = 0x00,
    RENDERER_VIEW_MODE_LIGHTING = 0x01,
    RENDERER_VIEW_MODE_NORMALS  = 0x02
} renderer_view_mode;

// @brief Представляет тип рендера.
typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX
} renderer_backend_type;

typedef struct geometry_render_data {
    mat4 model;
    geometry* geometry;
} geometry_render_data;

// @brief Представляет флаги очистки прохода визуализатора (комбинируемые).
typedef enum renderpass_clear_flag_bits {
    // @brief Очистка не проводить.
    RENDERPASS_CLEAR_NONE_FLAG           = 0x0,
    // @brief Очистка буфера цвета.
    RENDERPASS_CLEAR_COLOR_BUFFER_FLAG   = 0x1,
    // @brief Очистка буфера глубины.
    RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG   = 0x2,
    // @brief Очистка буфера трафарета.
    RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG = 0x4,
} renderpass_clear_flag_bits;

// @brief Представляет цель визуализатора, которая используется для визуализации в текстуру или набор текстур.
typedef struct render_target {
    // @brief Указывает должна ли цель прохода обновлятся при изменении размера окна.
    bool sync_to_window_size;
    // @brief Количество вложений (текстур).
    u8 attachment_count;
    // @brief Массив вложений (указателей текстур).
    texture** attachments;
    // @brief Внутренние данные визуализатора (объект буфера кадров).
    void* internal_framebuffer;
} render_target;

// @brief Представляет конфигурацию прохода визуализатора.
typedef struct renderpass_config {
    // @brief Имя прохода визуализатора.
    const char* name;
    // @brief Имя предыдущего прохода визуализатора.
    const char* prev_name;
    // @brief Имя следующего прохода визуализатора.
    const char* next_name;
    // @brief Текущая область прохода визуализатора.
    vec4 render_area;
    // @brief Цвет очистки прохода визуализатора.
    vec4 clear_color;
    // @brief Флаги очистки прохода визуализатора.
    renderpass_clear_flag_bits clear_flags;
} renderpass_config;

// @brief Представляет проход визуализатора.
typedef struct renderpass {
    // @brief Идентификатор прохода визуализатора.
    u16 id;
    // @brief Текущая область прохода визуализатора.
    vec4 render_area;
    // @brief Цвет очистки прохода визуализатора.
    vec4 clear_color;
    // @brief Флаги очистки прохода визуализатора необходимые для выполнения.
    renderpass_clear_flag_bits clear_flags;
    // @brief Количество целий визуализатора.
    u8 render_target_count;
    // @brief Массив целей визуализатора.
    render_target* targets;
    // @brief Внутренние данные визуализатора.
    void* internal_data;
} renderpass;

// @brief Представляет конфигурацию визуализатора.
typedef struct renderer_backend_config {
    // @brief Имя приложения.
    const char* application_name;
    // @brief Количество проходов визуализатора.
    u16 renderpass_count;
    // @brief Массив конфигураций проходов визуализатора. Инициализируется автоматически.
    renderpass_config* pass_configs;
    // @brief Функция обновления/воссоздания целей визуализатора по требованию.
    void (*on_rendertarget_refresh_required)();
} renderer_backend_config;

typedef struct renderer_backend {

    u64 frame_number;

    window* window_state;

    /*
        @brief Инициализирует работу визуализатора.
        @param backend Указатель на интерфейс визуализатора.
        @param config Указатель на конфигурацию визуализатора для использования при инициализации.
        @param out_window_render_target_count Указатель для сохранения количества целей (кадров) визуализатора.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*initialize)(struct renderer_backend* backend, const renderer_backend_config* config, u8* out_window_render_target_count);

    /*
        @brief Завершает работу визуализатора.
        @param backend Указатель на интерфейс визуализатора.
    */
    void (*shutdown)(struct renderer_backend* backend);

    /*
        @brief Изменяет размер области рендеринга, обычно вызывается на событие изменение размера окна.
        @param width Новая ширина области рендеринга.
        @param height Новая высота области рендеринга.
    */
    void (*resized)(i32 width, i32 height);

    /*
        @brief Выполняет необходимые настройки в начале кадра.
        NOTE: Возвращение функицей false означает, что кадр не может быть отрисован в данный момент,
              и тогда вызов end_frame делать не нужно. А отрисовку можно повторить в следующем кадре.
        @param backend Указатель на интерфейс визуализатора.
        @param delta_time Время в секундах с момента последнего кадра.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*frame_begin)(f32 delta_time);

    /*
        @brief Выполняет необходимые настройки в конце кадра и его отрисовку.
        NOTE: Следует вызывать только после успешного выполнения begin_frame.
        @param backend Указатель на интерфейс визуализатора.
        @param delta_time Время в секундах с момента последнего кадра.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*frame_end)(f32 delta_time);

    /*
        @brief Создает новый проходчик визуализации.
        @param out_renderpass Указатель на проходчик визуализации.
        @param depth Значение очистки глубины.
        @param stencil Значение очистки трафарета.
        @param has_prev_pass Указывает, есть ли предыдущий проход визуализации.
        @param has_next_pass Указывает, есть ли следующий проход визуализации.
    */
    void (*renderpass_create)(renderpass* out_renderpass, f32 depth, u32 stencil, bool has_prev_pass, bool has_next_pass);

    /*
        @brief Уничтожает предоставленный проходчик визуализации.
        @param pass Указатель на проходчик визуализации для уничтожения.
    */
    void (*renderpass_destroy)(renderpass* pass);

    /*
        @brief Начинает проход визуализатора с указанными параметрами.
        NOTE: Следует вызывать после функции начала кадра begin_frame, после чего вызываются функции отрисовки.
        @param pass Указатель на проходчик визуализатора для выполнения начала прохода.
        @param target Указатель на цель прохода визуализации.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*renderpass_begin)(renderpass* pass, render_target* target);

    /*
        @brief Завершает проход визуализатора с указанными параметрами.
        NOTE: Следует вызывать только после успешного выполнения begin_renderpass и функций отрисовки. 
        @param pass Указатель на проходчик визуализатора для выполнения завершения прохода.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*renderpass_end)(renderpass* pass);

    /*
        @brief Получение указателя на проходчик визуадизатора используя предоставленное имя.
        @param name Имя проходчика визуализатора для получения.
        @return Указатель на проходчик визуализатора, null если не был найден.
    */
    renderpass* (*renderpass_get)(const char* name);

    /*
        @brief Получает ресурсы для новой текстуры и загружает предоставленные данным изображения.
        @param t Указатель на текстуру которую необходимо создать и получить внутренние ресурсы.
        @param pixels Указатель на необработанные данные изображения для загрузки (пиксели).
    */
    void (*texture_create)(texture* t, const void* pixels);

    /*
        @brief Получает ресурсы для новой записываемой текстуры без записанных в нее данных.
        @param t Указатель на записываемую текстуру которую необходимо создать и получить внутренние ресурсы.
    */
    void (*texture_create_writable)(texture* t);

    /*
        @brief Освобождает ресурсы предоставленной текстуры и уничтожает внутренние данные (изображение).
        @param t Указатель на текстуру, которую необходимо уничтожить и освободить внутренние ресурсы.
    */
    void (*texture_destroy)(texture* t);

    /*
        @brief Изменяет размер текстуры. На этом уровне нет проверки на возможность записи текстуры.
               Внутренние ресурсы уничтожаются и создаются заново с новым разрешением, а данные
               теряются и должны быть загруженны вновь.
        @param t Указатель на текстуру для изменения размера.
        @param new_width Новая ширина в пикселях.
        @param new_height Новая высота в пикселях.
    */
    void (*texture_resize)(texture* t, u32 new_width, u32 new_height);

    /*
        @brief Записывает указанные данные в предоставленную текстуру.
        NOTE: На этом уровне это может быть как записываемая, так и не записываемая текстура, поскольку
              она также обрабатывает начальную загрузку текстуры. Сама система текстур должна отвечать
              за блокировку запросов на запись в не записываемые текстуры.
        @param t Указатель на текстуру для записи данных.
        @param offset Смещение в байтах откуда начать запись данных.
        @param size Количество байт данных для записи.
        @param pixels Указатель на необработанные данные изображения для загрузки (пиксели).
    */
    void (*texture_write_data)(texture* t, u32 offset, u32 size, const void* pixels);

    /*
        @brief Получает внутренние ресурсы для предоставленной карты текстуры.
        @param map Указатель на карту текстуры для получения ресурсов.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*texture_map_acquire_resources)(texture_map* map);

    /*
        @brief Освобождает внутренние русурсы для предоставленной карты текстуры.
        @param map Указатель на карту текстуры для освобождения ресурсов.
    */
    void (*texture_map_release_resources)(texture_map* map);

    /*
        @brief Получает ресурсы графического процессора и загружает данные геометрии.
        @param g Указатель на геометрию для которой необходимо получить ресурсы.
        @param vertex_size Размер вершин в геометрии.
        @param vertex_count Количество вершин в геометрии.
        @param vertices Массив вершин геометрии.
        @param index_size Размер индекса в геометрии.
        @param index_count Количество индексов в геометрии.
        @param indices Массив индексов геометрии.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*geometry_create)(geometry* g, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);

    /*
        @brief Уничтожает предоставленную геометрию, освобождая ресурсы графического процессора.
        @param g Указатель на геометрию, которую необходимо уничтожить.
    */
    void (*geometry_destroy)(geometry* g);

    /*
        @brief Рисует предоставленные геометрические данные.
        @param data Указатель на геометрические данные для визуализации.
    */
    void (*geometry_draw)(geometry_render_data* data);

    /*
        @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.
        @param s Указатель на шейдер для создания внутренних ресурсов.
        @param config Указатель на конфигурацию шейдера.
        @param pass Указатель на проходчик визуализатора, который будет связан с шейдером.
        @param stage_count Количество стадий шейдера.
        @param stage_filenames Массив имен файлов стадий шейдера, которые будут загружены. Должен соотвествовать массиву стадий шейдера.
        @param stages Массив стадий шейдера (вершина, фрагмент и т.д), указывающий какие стадии будут использоваться в этом шейдере.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_create)(struct shader* s, const shader_config* config, renderpass* pass, u8 stage_count, const char** stage_filenames, shader_stage* stages);

    /*
        @brief Уничтожает предоставленный шейдер и освобождает ресурсы им удерживаемые.
        @param s Указатель на шейдер, который необходимо уничтожить.
    */
    void (*shader_destroy)(struct shader* s);

    /*
        @brief Инициализирует настроенный шейдер, должно быть выполено после 'shader_create', а так же последующей настройки шейдер.
        @param s Указатель на шейдер, который необходимо инициализировать.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_initialize)(struct shader* s);

    /*
        @brief Использует заданный шейдер, активируя его для обновления атрибутов, uniform перменных и т.д.
        @param s Указатель на шейдер, над которым будут проводиться операции.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_use)(struct shader* s);

    /*
        @brief Связывает глобальные ресурсы для использования и обновления.
        @param s Указатель на шейдер, глобальные ресурсы которого должны быть связаны.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_bind_globals)(struct shader* s);

    /*
        @brief Связывает ресурсы экземпляра для использования и обновления.
        @param s Указатель на шейдер, ресурсы экземпляра которого должны быть связаны.
        @param instance_id Идентификатор экземпляра, который должен быть связан.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_bind_instance)(struct shader* s, u32 instance_id);

    /*
        @brief Применяет данные к текущим привязанному глобальному ресурсу.
        @param s Указатель на шейдер к которому должны быть применены глобальные данные.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_apply_globals)(struct shader* s);

    /*
        @brief Применяет данные к текущим привязанному ресурсу экземпляра.
        @param s Указатель на шейдер к которому должны быть применены данные экземпляра.
        @param needs_update Указывает, на необходимость обновить uniform переменные или связать их.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_apply_instance)(struct shader* s, bool needs_update);

    /*
        @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.
        @param s Указатель на шейдер из которого нужно получить ресурсы экземпляра.
        @param maps Указатель на массив указателей карт текстур. Должен быть один на текстуру в экземпляре.
        @param out_instance_id Указатель на переменную для сохранения идентификатора экземпляра.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_acquire_instance_resources)(struct shader* s, texture_map** maps, u32* out_instance_id);

    /*
        @brief Освобождает внутренние ресурсы уровня экземпляра по предоставленному идентификатору экземпляра.
        @param s Указатель на шейдер для которого нужно освободить ресурсы экземпляра.
        @param instance_id Идентификатор экземпляра, ресурсы которого необходимо освободить.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_release_instance_resources)(struct shader* s, u32 instance_id);

    /*
        @brief Задает значение uniform переменой указанного шейдера.
        @param s Указатель на шейдер для которого нужно задать uniform переменую.
        @parma uniform Указатель на uniform переменную значение которой необходимо задать.
        @param value Указатель на значение которое необходимо задать uniform переменной.
        @return True операция завершена успешно, false в случае ошибок.
    */
    bool (*shader_set_uniform)(struct shader* frontend_shader, struct shader_uniform* uniform, const void* value);

    /*
        @brief Создает новую цель визуализации используя предоставленные данные.
        @apram attachment_count Количество вложений (указателей на текстуры).
        @param attachments Массив вложений (указатели текстур).
        @param pass Указатель на проходчик визуализатора, с которым связана цель визуализации.
        @param width Ширина цели визуализации в пикселях.
        @param height Высота цели визуализации в пикселях.
        @param out_target Указатель на структуру render_target для получения новой цели визуализации.
    */
    void (*render_target_create)(u8 attachment_count, texture** attachments, renderpass* pass, u32 width, u32 height, render_target* out_target);

    /*
        @brief Уничтожает предоставленную цель визуализации.
        @param out_target Указатель на структуру render_target для уничтожения цели визуализации.
        @param free_internal_memory Указывает, следует ли освободить внутреннюю память.
    */
    void (*render_target_destroy)(render_target* target, bool free_internal_memory);

    /*
        @brief Пытается получить цель визуализации (кадр) окна по указанному индексу.
        @param index Индекс для получения вложения (кадр). Должен быть в пределах количества целей визуализации (кадров) окна.
        @return Указатель на вложение (кадр) в случае успеха, null если не удалось.
    */
    texture* (*window_attachment_get)(u8 index);

    /*
        @brief Получает указатель на основную цель изображение буфера глубины.
    */
    texture* (*depth_attachment_get)();

    /*
        @brief Получает текущий индекс вложения (кадра) окна.
    */
    u8 (*window_attachment_index_get)();

    /*
        @brief Указывает поддерживает ли визуализатор многопоточную работу.
    */
    bool (*is_multithreaded)();

} renderer_backend;

// @brief Известные типы визуализации.
typedef enum render_view_known_type {
    RENDERER_VIEW_KNOWN_TYPE_WORLD  = 0x01,
    RENDERER_VIEW_KNOWN_TYPE_UI     = 0x02,
    RENDERER_VIEW_KNOWN_TYPE_SKYBOX = 0x03
} render_view_known_type;

typedef enum render_view_matrix_source {
    RENDER_VIEW_MATRIX_SOURCE_SCENE_CAMERA = 0x01,
    RENDER_VIEW_MATRIX_SOURCE_UI_CAMERA    = 0x02,
    RENDER_VIEW_MATRIX_SOURCE_LIGHT_CAMERA = 0x03,
} render_view_matrix_source;

typedef enum render_view_projection_matrix_source {
    RENDER_VIEW_PROJECTION_MATRIX_SOURCE_DEFAULT_PERSPECTIVE  = 0x01,
    RENDER_VIEW_PROJECTION_MATRIX_SOURCE_DEFAULT_ORTHOGRAPHIC = 0x02,
} render_view_projection_matrix_source;

typedef struct render_view_pass_config {
    const char* name;
} render_view_pass_config;

typedef struct render_view_config {
    const char* name;
    const char* custom_shader_name; // Если не используется то установить null.
    u16 width;                      // Установить в 0 для 100% ширины.
    u16 height;                     // Установить в 0 для 100% высоты.
    render_view_known_type type;
    render_view_matrix_source view_matrix_source;
    render_view_projection_matrix_source projection_matrix_source;
    u8 pass_count;
    render_view_pass_config* passes;
} render_view_config;

struct render_view_packet;

typedef struct render_view {
    u16 id;
    const char* name;
    u16 width;
    u16 height;
    render_view_known_type type;
    u8 renderpass_count;
    renderpass** passes;
    const char* custom_shader_name;
    void* internal_data; // Данные визуализатора.

    bool (*on_create)(struct render_view* self);
    void (*on_destroy)(struct render_view* self);
    void (*on_resize)(struct render_view* self, u32 width, u32 height);
    bool (*on_build_packet)(const struct render_view* self, void* data, struct render_view_packet* out_packet);
    void (*on_destroy_packet)(const struct render_view* self, struct render_view_packet* packet);
    bool (*on_render)(const struct render_view* self, const struct render_view_packet* packet, u64 frame_number, u64 render_target_index);
} render_view;

typedef struct render_view_packet {
    render_view* view;
    mat4 view_matrix;
    mat4 projection_matrix;
    vec3 view_position;
    vec4 ambient_color;
    u32 geometry_count;
    geometry_render_data* geometries;
    const char* custom_shader_name;
    void* extended_data;
} render_view_packet;

typedef struct mesh_packet_data {
    u32 mesh_count;
    mesh** meshes;
} mesh_packet_data;

typedef struct skybox_packet_data {
    skybox* sb;
} skybox_packet_data;

typedef struct render_packet {
    f32 delta_time;
    u16 view_count;
    render_view_packet* views;
} render_packet;
