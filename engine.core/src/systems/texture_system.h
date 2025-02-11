#pragma once

#include <defines.h>
#include <resources/resource_types.h>

#define DEFAULT_TEXTURE_NAME "default"

typedef struct texture_system_config {
    u32 max_texture_count;
} texture_system_config;

/*
*/
bool texture_system_initialize(u64* memory_requirement, void* memory, texture_system_config* config);

/*
*/
void texture_system_shutdown();

/*
*/
texture* texture_system_acquire(const char* name, bool auto_release);

/*
*/
void texture_system_release(const char* name);

/*
*/
texture* texture_system_get_default_texture();
