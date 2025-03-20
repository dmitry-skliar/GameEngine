#pragma once

#include <defines.h>
#include <math/math_types.h>

#define TEXTURE_NAME_MAX_LENGTH 512
#define MATERIAL_NAME_MAX_LENGTH 256
#define GEOMETRY_NAME_MAX_LENGTH 256

typedef enum resource_type {
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_MESH,
    RESOURCE_TYPE_SHADER,
    RESOURCE_TYPE_CUSTOM
} resource_type;

typedef struct resource {
    u32 loader_id;
    const char* name;
    char* full_path;
    u64 data_size;
    void* data;
} resource;

typedef struct image_resouce_data {
    u8 channel_count;
    u32 width;
    u32 height;
    u8* pixels;
} image_resouce_data;

typedef enum texture_flag_bits {
    // @brief Указывает, что текстура имеет прозрачность.
    TEXTURE_FLAG_HAS_TRANSPARENCY = 0x01,
    // @brief Указывает, что текстура может быть записана.
    TEXTURE_FLAG_IS_WRITABLE      = 0x02,
    // @brief Указывает, что текстура создана с помощью обертывания.
    TEXTURE_FLAG_IS_WRAPPED       = 0x04
} texture_flag_bits;

// @brief Данные текстуры.
typedef struct texture {
    // @brief Уникальный идентификатор текстуры.
    u32 id;
    // @brief Ширина текстуры в пикселях.
    u32 width;
    // @brief Высота текстуры в пикселях.
    u32 height;
    // @brief Ширина канала пикселя.
    u8 channel_count;
    // @brief Содержит флаги текстуры.
    texture_flag_bits flags;
    // @brief Генератор изменений, используется для обновления текстуры.
    u32 generation;
    // @brief Имя текстуры.
    char name[TEXTURE_NAME_MAX_LENGTH];
    // @brief Внутренние данные рендера. Обычно текстура в памяти видеокарты.
    void* internal_data;
} texture;

typedef enum texture_use {
    TEXTURE_USE_UNKNOWN,
    TEXTURE_USE_MAP_DIFFUSE,
    TEXTURE_USE_MAP_SPECULAR,
    TEXTURE_USE_MAP_NORMAL,
} texture_use;

typedef enum texture_filter {
    TEXTURE_FILTER_NEAREST,
    TEXTURE_FILTER_LINEAR
} texture_filter;

typedef enum texture_repeat {
    TEXTURE_REPEAT_REPEAT,
    TEXTURE_REPEAT_MIRRORED_REPEAT,
    TEXTURE_REPEAT_CLAMP_TO_EDGE,
    TEXTURE_REPEAT_CLAMP_TO_BORDER
} texture_repeat;

typedef struct texture_map {
    texture* texture;
    texture_use use;
    texture_filter filter_minify;  // При уменьшении.
    texture_filter filter_magnify; // При увеличении.
    texture_repeat repeat_u;       // X
    texture_repeat repeat_v;       // Y
    texture_repeat repeat_w;       // Z
    void* internal_data;           // Внутренние данные рендера. Обычно для сэмплера.
} texture_map;

typedef struct material_config {
    char name[MATERIAL_NAME_MAX_LENGTH];
    char* shader_name;
    bool auto_release;
    f32 shininess;
    vec4 diffuse_color;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
    char specular_map_name[TEXTURE_NAME_MAX_LENGTH];
    char normal_map_name[TEXTURE_NAME_MAX_LENGTH];
} material_config;

typedef struct material {
    u32 id;
    u32 generation;
    u32 internal_id;
    char name[MATERIAL_NAME_MAX_LENGTH];
    vec4 diffuse_color;
    f32 shininess;
    texture_map diffuse_map;
    texture_map specular_map;
    texture_map normal_map;
    u32 shader_id;
    // @brief Указывает кадр с которым произошла синхронизация.
    u32 render_frame_number;
} material;

typedef struct geometry {
    u32 id;
    u32 internal_id;
    u32 generation;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    material* material;
} geometry;

// @brief Объединяет в себе геометрии как единый объект.
typedef struct mesh {
    // @brief Количество внутренних геометрий.
    u16 geometry_count;
    // @brief Массив указателей на геометрии.
    geometry** geometries;
    // @brief Массив преобразований над сеткой геометрий из локальных в мировые.
    transform transform;
} mesh;

// @brief Стадия шейдера на конвейере (можно комбинировать).
typedef enum shader_stage {
    SHADER_STAGE_VERTEX   = 0x00000001,
    SHADER_STAGE_GEOMETRY = 0x00000002,
    SHADER_STAGE_FRAGMENT = 0x00000004,
    SHADER_STAGE_COMPUTE  = 0x00000008,
} shader_stage;

// @brief Тип данных атрибута шейдера.
typedef enum shader_attribute_type {
    SHADER_ATTRIB_TYPE_FLOAT32   = 0U,
    SHADER_ATTRIB_TYPE_FLOAT32_2 = 1U,
    SHADER_ATTRIB_TYPE_FLOAT32_3 = 2U,
    SHADER_ATTRIB_TYPE_FLOAT32_4 = 3U,
    SHADER_ATTRIB_TYPE_MATRIX_4  = 4U,
    SHADER_ATTRIB_TYPE_INT8      = 5U,
    SHADER_ATTRIB_TYPE_UINT8     = 6U,
    SHADER_ATTRIB_TYPE_INT16     = 7U,
    SHADER_ATTRIB_TYPE_UINT16    = 8U,
    SHADER_ATTRIB_TYPE_INT32     = 9U,
    SHADER_ATTRIB_TYPE_UINT32    = 10U
} shader_attribute_type;

// @brief Тип данных uniform переменой шейдера.
typedef enum shader_uniform_type {
    SHADER_UNIFORM_TYPE_FLOAT32   = 0U,
    SHADER_UNIFORM_TYPE_FLOAT32_2 = 1U,
    SHADER_UNIFORM_TYPE_FLOAT32_3 = 2U,
    SHADER_UNIFORM_TYPE_FLOAT32_4 = 3U,
    SHADER_UNIFORM_TYPE_INT8      = 4U,
    SHADER_UNIFORM_TYPE_UINT8     = 5U,
    SHADER_UNIFORM_TYPE_INT16     = 6U,
    SHADER_UNIFORM_TYPE_UINT16    = 7U,
    SHADER_UNIFORM_TYPE_INT32     = 8U,
    SHADER_UNIFORM_TYPE_UINT32    = 9U,
    SHADER_UNIFORM_TYPE_MATRIX_4  = 10U,
    SHADER_UNIFORM_TYPE_SAMPLER   = 11U,
    SHADER_UNIFORM_TYPE_CUSTOM    = 255U
} shader_uniform_type;

// @brief Область действия uniform переменой (влияет на то как часто обновляется).
typedef enum shader_scope {
    // @brief Глобальная облась действия (обновление каждый кадр).
    SHADER_SCOPE_GLOBAL   = 0,
    // @brief Область действия экземпляра (обновление для каждого экземпляра).
    SHADER_SCOPE_INSTANCE = 1,
    // @brief Локальная область действия (обновление для каждого объекта).
    SHADER_SCOPE_LOCAL    = 2
} shader_scope;

// @brief Конфигурация атрибута.
typedef struct shader_attribute_config {
    // @brief Имя атрибута.
    char* name;
    // @brief Размер атрибута в байтах.
    u8 size;
    // @brief Тип данных атрибута.
    shader_attribute_type type;
} shader_attribute_config;

// @brief Конфигурация uniform переменой.
typedef struct shader_uniform_config {
    // @brief Имя uniform переменой.
    char* name;
    // @brief Размер uniform переменой в байтах.
    u8 size;
    // @brief Местоположение uniform переменой.
    u32 location;
    // @brief Тип данных uniform переменой.
    shader_uniform_type type;
    // @brief Область действия uniform переменой.
    shader_scope scope;
} shader_uniform_config;

// @brief Конфигурация шейдера (используется для загрузки шейдеров).
typedef struct shader_config {
    // @brief Имя создаваемого шейдера.
    char* name;
    // @brief Указывает использует ли шейдер uniform-буферы уровня экземпляра.
    bool use_instances;
    // @brief Указывает использует ли шейдер uniform-буферы локльного уровня.
    bool use_local;
    // @brief Количество используемых атрибутов в шейдере.
    u8 attribute_count;
    // @brief Массив атрибутов (используется darray).
    shader_attribute_config* attributes;
    // @brief Количество uniform переменных.
    u8 uniform_count;
    // @brief Массив uniform переменных (используется darray).
    shader_uniform_config* uniforms;
    // @brief Имя прохода рендера используемого шейдером.
    char* renderpass_name;
    // @brief Количество стадий в шейдере.
    u8 stage_count;
    // @brief Массив стадий шейдера (используется darray).
    shader_stage* stages;
    // @brief Массив имен стадий шейдера (используется darray).
    char** stage_names;
    // @brief Массив файлов стадий которые будут загружаться (используется darray).
    char** stage_filenames;
} shader_config;
