// Собственные подключения.
#include "resources/loaders/text_loader.h"
#include "resources/loaders/loader_util.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "platform/file.h"

bool text_loader_load(resource_loader* self, const char* name, void* params, resource* out_resource)
{
    char* format_str = "%s/%s";
    char full_file_path[512];
    string_format_unsafe(full_file_path, format_str, resource_system_base_path(), name);

    file* f;
    if(!platform_file_open(full_file_path, FILE_MODE_READ, &f))
    {
        kerror("Function '%s': Unable to open text file '%s' for reading.", __FUNCTION__, full_file_path);
        return false;
    }

    // TODO: Должен использоваться распределитель памяти.
    out_resource->full_path = string_duplicate(full_file_path);

    u64 file_size = platform_file_size(f);

    // TODO: Должен использоваться распределитель памяти.
    char* resource_data = kallocate_tc(char, file_size, MEMORY_TAG_FILE);

    u64 read_size = 0;
    if(!platform_file_read_all_text(f, resource_data, &read_size))
    {
        kerror("Function '%s': Unable to read text file '%s'.", __FUNCTION__, full_file_path);
        platform_file_close(f);
        return false;
    }

    platform_file_close(f);

    out_resource->data = resource_data;
    out_resource->data_size = read_size;
    out_resource->name = name;

    return true;
}

void text_loader_unload(resource_loader* self, resource* resource)
{
    resource_unload(self, resource, MEMORY_TAG_FILE, __FUNCTION__);
}

resource_loader text_resource_loader_create()
{
    resource_loader loader;
    loader.type = RESOURCE_TYPE_TEXT;
    loader.custom_type = null;
    loader.load = text_loader_load;
    loader.unload = text_loader_unload;
    loader.type_path = "";

    return loader;
}
