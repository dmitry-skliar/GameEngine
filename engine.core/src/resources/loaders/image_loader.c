// Собственные подключения.
#include "resources/loaders/image_loader.h"
#include "resources/loaders/loader_util.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "platform/file.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"

// Внешние подключения.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "vendor/stb_image.h"

bool image_loader_load(resource_loader* self, const char* name, void* params, resource* out_resource)
{

    image_resouce_params* typed_params = params;

    char* format_str = "%s/%s/%s%s";
    const i32 required_channel_count = 4;
    stbi_set_flip_vertically_on_load_thread(typed_params->flip_y);
    char full_file_path[512];

    #define IMAGE_EXTENSION_COUNT 4
    char* extentions[IMAGE_EXTENSION_COUNT] = { ".tga", ".png", ".jpg", ".bmp" };
    bool found = false;

    // Поиск расширений.
    for(u32 i = 0; i < IMAGE_EXTENSION_COUNT; ++i)
    {
        string_format_unsafe(full_file_path, format_str, resource_system_base_path(), self->type_path, name, extentions[i]);
        if(platform_file_exists(full_file_path))
        {
            found = true;
            break;
        }
    }

    out_resource->data = null;
    out_resource->data_size = 0;
    out_resource->full_path = string_duplicate(full_file_path);
    out_resource->name = name;

    if(!found)
    {
        kerror("Function '%s': Failed to find file '%s' or with any supported extention.", __FUNCTION__, full_file_path);
        return false;
    }

    file* f;
    if(!platform_file_open(full_file_path, FILE_MODE_READ | FILE_MODE_BINARY, &f))
    {
        kerror("Function '%s': Unable to read file: %s.", __FUNCTION__, full_file_path);
        platform_file_close(f);
        return false;
    }

    u64 file_size = 0;
    if(!(file_size = platform_file_size(f)))
    {
        kerror("Function '%s': Unable to get size of file: %s.", __FUNCTION__, full_file_path);
        platform_file_close(f);
        return false;
    }

    // TODO: память выделена, но вот в какой момент ее можно освободить?
    u8* raw_data = kallocate(file_size, MEMORY_TAG_TEXTURE);
    u64 read_bytes = 0;
    bool read_result = platform_file_read_all_bytes(f, raw_data, &read_bytes);
    platform_file_close(f);

    if(!read_result || read_bytes != file_size)
    {
        kerror("Function '%s': Unable to read file: %s.", __FUNCTION__, full_file_path);
        kfree(raw_data, MEMORY_TAG_TEXTURE);
        return false;
    }

    i32 width;
    i32 height;
    i32 channel_count;
    u8* data = stbi_load_from_memory(raw_data, file_size, &width, &height, &channel_count, required_channel_count);
    if(!data)
    {
        kerror("Function '%s': Image resource loader failed to load file: %s.", __FUNCTION__, full_file_path);
        kfree(raw_data, MEMORY_TAG_TEXTURE);
        return false;
    }

    // TODO: Масло масленное, но пока что сойдет и так!
    kfree(raw_data, MEMORY_TAG_TEXTURE);

    image_resouce_data* resource_data = kallocate_tc(image_resouce_data, 1, MEMORY_TAG_TEXTURE);
    resource_data->pixels = data;
    resource_data->width = width;
    resource_data->height = height;
    resource_data->channel_count = required_channel_count;

    out_resource->data = resource_data;
    out_resource->data_size = sizeof(image_resouce_data);
    return true;
}

void image_loader_unload(resource_loader* self, resource* resource)
{
    image_resouce_data* resource_data = resource->data;
    if(resource_data)
    {
        stbi_image_free(resource_data->pixels);
    }

    resource_unload(self, resource, MEMORY_TAG_TEXTURE, __FUNCTION__);
}

resource_loader image_resource_loader_create()
{
    resource_loader loader;
    loader.type = RESOURCE_TYPE_IMAGE;
    loader.custom_type = null;
    loader.load = image_loader_load;
    loader.unload = image_loader_unload;
    loader.type_path = "textures";

    return loader;
}
