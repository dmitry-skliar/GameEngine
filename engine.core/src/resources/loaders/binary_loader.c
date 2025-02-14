// Собственные подключения.
#include "resources/loaders/binary_loader.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "platform/file.h"

bool binary_loader_load(resource_loader* self, const char* name, resource* out_resource)
{
    char* format_str = "%s/%s";
    char full_file_path[512];
    string_format(full_file_path, format_str, resource_system_base_path(), name);

    // TODO: Должен использоваться распределитель памяти.
    out_resource->full_path = string_duplicate(full_file_path);

    file* f;
    if(!platform_file_open(full_file_path, FILE_MODE_READ | FILE_MODE_BINARY, &f))
    {
        kerror("Function '%s': Unable to open binary file '%s' for reading.", __FUNCTION__, full_file_path);
        return false;
    }

    u64 file_size = platform_file_size(f);

    // TODO: Должен использоваться распределитель памяти.
    u8* resource_data = kallocate_tc(u8, file_size, MEMORY_TAG_FILE);
    u64 read_size = 0;
    if(!platform_file_read_all_bytes(f, resource_data, &read_size))
    {
        kerror("Function '%s': Unable to read binary file '%s'.", __FUNCTION__, full_file_path);
        platform_file_close(f);
        return false;
    }

    platform_file_close(f);

    out_resource->data = resource_data;
    out_resource->data_size = read_size;
    out_resource->name = name;

    return true;
}

void binary_loader_unload(resource_loader* self, resource* resource)
{
    if(string_length(resource->full_path) > 0)
    {
        string_free(resource->full_path);
    }

    if(resource->data)
    {
        kfree(resource->data, resource->data_size, MEMORY_TAG_FILE);
        resource->data = null;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
}

resource_loader binary_resource_loader_create()
{
    resource_loader loader;
    loader.type = RESOURCE_TYPE_BINARY;
    loader.custom_type = null;
    loader.load = binary_loader_load;
    loader.unload = binary_loader_unload;
    loader.type_path = "";

    return loader;
}
