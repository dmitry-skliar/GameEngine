#pragma once

#include <defines.h>
#include <resources/resource_types.h>

typedef struct resource_system_config {
    u32 max_loader_count;
    char* asset_base_path;
} resource_system_config;

typedef struct resource_loader {
    u32 id;
    resource_type type;
    const char* custom_type;
    const char* type_path;
    bool (*load)(struct resource_loader* self, const char* name, void* params, resource* out_resource);
    void (*unload)(struct resource_loader* self, resource* resource);
} resource_loader;

/*
*/
bool resource_system_initialize(u64* memory_requirement, void* memory, resource_system_config* config);

/*
*/
void resource_system_shutdown();

/*
*/
KAPI bool resource_system_register_loader(resource_loader loader);

/*
    NOTE: Не создает ресурс! Подается готовая структура!
*/
KAPI bool resource_system_load(const char* name, resource_type type, void* params, resource* out_resource);

/*
*/
KAPI bool resource_system_load_custom(const char* name, const char* custom_type, void* params, resource* out_resource);

/*
*/
KAPI void resource_system_unload(resource* resource);

/*
*/
KAPI const char* resource_system_base_path();
