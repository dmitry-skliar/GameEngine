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
    RESOURCE_TYPE_STATIC_MESH,
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

typedef enum material_type {
    MATERIAL_TYPE_WORLD,
    MATERIAL_TYPE_UI
} material_type;

typedef struct material_config {
    char name[MATERIAL_NAME_MAX_LENGTH];
    material_type type;
    bool auto_release;
    vec4 diffuse_color;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} material_config;

typedef struct material {
    u32 id;
    u32 generation;
    u32 internal_id;
    material_type type;
    char name[MATERIAL_NAME_MAX_LENGTH];
    vec4 diffuse_color;
    texture_map diffuse_map;
} material;

typedef struct geometry {
    u32 id;
    u32 internal_id;
    u32 generation;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    material* material;
} geometry;
