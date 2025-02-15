#pragma once

#include <defines.h>
#include <memory/memory.h>
#include <resources/resource_types.h>
#include <systems/resource_system.h>

/*
*/
void resource_unload(resource_loader* self, resource* resource, memory_tag tag, const char* func_name);
