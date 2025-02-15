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
    
    if(string_length(resource->full_path) > 0)
    {
        string_free(resource->full_path);
    }

    if(resource->data)
    {
        kfree(resource->data, resource->data_size, tag);
        resource->data = null;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
}
