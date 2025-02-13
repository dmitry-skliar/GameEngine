#pragma once

#include <defines.h>
#include <resources/resource_types.h>

#define DEFAULT_MATERIAL_NAME "default"

typedef struct material_system_config {
    u32 max_material_count;
} material_system_config;

typedef struct material_config {
    char name[MATERIAL_NAME_MAX_LENGTH];
    bool auto_release;
    vec4 diffuse_color;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} material_config;

/*
*/
bool material_system_initialize(u64* memory_requirement, void* memory, material_system_config* config);

/*
*/
void material_system_shutdown();

/*
*/
material* material_system_acquire(const char* name);

/*
*/
material* material_system_acquire_from_config(material_config* config);

/*
*/
void material_system_release(const char* name);

/*
*/
material* material_system_get_default();
