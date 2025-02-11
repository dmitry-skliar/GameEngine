#pragma once

#include <defines.h>
#include <math/math_types.h>

#define TEXTURE_NAME_MAX_LENGTH 512
#define MATERIAL_NAME_MAX_LENGTH 256

// @brief Данные текстуры.
typedef struct texture {
    u32 id;
    u32 width;
    u32 height;
    u8 channel_count;
    bool has_transparency;
    u32 generation;
    char name[TEXTURE_NAME_MAX_LENGTH];
    void* data;
} texture;

typedef enum texture_use {
    TEXTURE_USE_UNKNOWN,
    TEXTURE_USE_MAP_DIFFUSE
} texture_use;

typedef struct texture_map {
    texture* texture;
    texture_use use;
} texture_map;

typedef struct material {
    u32 id;
    u32 generation;
    u32 internal_id;
    char name[MATERIAL_NAME_MAX_LENGTH];
    vec4 diffuse_color;
    texture_map diffuse_map;
} material;
