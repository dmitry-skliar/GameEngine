#pragma once

#include <defines.h>
#include <platform/window.h>
#include <math/math_types.h>
#include <resources/resource_types.h>

#define BUILTIN_SHADER_NAME_MATERIAL "Builtin.MaterialShader"
#define BUILTIN_SHADER_NAME_UI "Builtin.UIShader"

// TODO: Подчистить заголовочные файлы данным способом!
struct shader;
struct shader_uniform;

// @brief Режимы отображения визуализации (для отладки).
typedef enum renderer_view_mode {
    RENDERER_VIEW_MODE_DEFAULT  = 0x00,
    RENDERER_VIEW_MODE_LIGHTING = 0x01,
    RENDERER_VIEW_MODE_NORMALS  = 0x02
} renderer_view_mode;

// @brief Тип рендера.
typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX
} renderer_backend_type;

typedef struct geometry_render_data {
    mat4 model;
    geometry* geometry;
} geometry_render_data;

typedef enum builtin_renderpass {
    BUILTIN_RENDERPASS_WORLD = 0x01,
    BUILTIN_RENDERPASS_UI    = 0x02
} builtin_renderpass;

typedef struct renderer_backend {

    u64 frame_number;

    window* window_state;

    bool (*initialize)(struct renderer_backend* backend);

    void (*shutdown)(struct renderer_backend* backend);

    void (*resized)(struct renderer_backend* backend, i32 width, i32 height);

    bool (*begin_frame)(struct renderer_backend* backend, f32 delta_time);

    bool (*end_frame)(struct renderer_backend* backend, f32 delta_time);

    bool (*begin_renderpass)(struct renderer_backend* backend, builtin_renderpass renderpass_id);

    bool (*end_renderpass)(struct renderer_backend* backend, builtin_renderpass renderpass_id);

    void (*texture_create)(texture* texture, const void* pixels);

    void (*texture_destroy)(texture* texture);

    /*
        @brief Создает новую записываемую текстуру без записанных в нее данных.
        @param texture Указатель на текстуру для получения ресурсов.
    */
    void (*texture_create_writable)(texture* texture);

    /*
        @brief Изменяет размер текстуры. На этом уровне нет проверки на возможность
               записи текстуры. Внутренние ресурсы уничтожаются и создаются заново с
               новым разрешением. Данные теряются и должны быть перезагружены.
        @param texture Указатель на текстуру для изменения размера.
        @param new_width Новая ширина в пикселях.
        @param new_height Новая высота в пикселях.
    */
    void (*texture_resize)(texture* texture, u32 new_width, u32 new_height);

    /*
        @brief Записывает указанные данные в предоставленную текстуру.
        NOTE: На этом уровне это может быть как записываемая, так и не записываемая
              текстура, поскольку она также обрабатывает начальную загрузку текстуры.
              Сама система текстур должна отвечать за блокировку запросов на запись
              в не записываемые текстуры.
        @param texture Указатель на текстуру для записи данных.
        @param offset Смещение в байтах откуда начать запись данных.
        @param size Количество байт данных для записи.
        @param pixels Необработаные данные изображения (пиксели) которые будут записаны.
    */
    void (*texture_write_data)(texture* texture, u32 offset, u32 size, const void* pixels);

    bool (*texture_map_acquire_resources)(texture_map* map);

    void (*texture_map_release_resources)(texture_map* map);

    bool (*geometry_create)(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices);

    void (*geometry_destroy)(geometry* geometry);

    void (*geometry_draw)(geometry_render_data data);

    bool (*shader_create)(struct shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_filenames, shader_stage* stages);

    void (*shader_destroy)(struct shader* shader);

    bool (*shader_initialize)(struct shader* shader);

    bool (*shader_use)(struct shader* shader);

    bool (*shader_bind_globals)(struct shader* s);

    bool (*shader_bind_instance)(struct shader* s, u32 instance_id);

    bool (*shader_apply_globals)(struct shader* s);

    bool (*shader_apply_instance)(struct shader* s, bool needs_update);

    bool (*shader_acquire_instance_resources)(struct shader* s, texture_map** maps, u32* out_instance_id);

    bool (*shader_release_instance_resources)(struct shader* s, u32 instance_id);

    bool (*shader_set_uniform)(struct shader* frontend_shader, struct shader_uniform* uniform, const void* value);

} renderer_backend;

// TODO: Провести рефактор render_packet.
typedef struct render_packet {
    f32 delta_time;

    u32 geometry_count;
    geometry_render_data* geometries;

    u32 ui_geometry_count;
    geometry_render_data* ui_geometries;
} render_packet;
