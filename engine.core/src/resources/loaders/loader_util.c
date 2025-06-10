// Собственные подключения.
#include "resources/loaders/loader_util.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"

void resource_unload(resource_loader* self, resource* resource, memory_tag tag, const char* func_name)
{
    if(!self || !resource)
    {
        kwarng("Function '%s' requires a valid pointers to self and resource.", func_name);
        return;
    }
    
    if(resource->full_path)
    {
        string_free(resource->full_path);
    }

    if(resource->data)
    {
        kfree(resource->data, tag);
        resource->data = null;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
}

bool resource_loader_load_valid(resource_loader* self, const char* name, resource* resource, const char* func_name)
{
    if(!self || !name || !resource)
    {
        if(func_name)
        {
            kerror("Function '%s' requires a valid pointer to self, name and resource.", func_name);
        }
        return false;
    }
    return true;
}
