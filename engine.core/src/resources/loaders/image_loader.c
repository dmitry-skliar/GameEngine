// Собственные подключения.
#include "resources/loaders/image_loader.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"

// Внешние подключения.
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

bool image_loader_load(resource_loader* self, const char* name, resource* out_resource)
{
    char* format_str = "%s/%s/%s%s";
    const i32 required_channel_count = 4;
    stbi_set_flip_vertically_on_load(true);
    char full_file_path[512];

    // TODO: разные расширения.
    string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, ".png");

    i32 width;
    i32 height;
    i32 channel_count;

    // TODO: конфигурируемым.
    u8* data = stbi_load(full_file_path, &width, &height, &channel_count, required_channel_count);

    const char* fail_reason = stbi_failure_reason();
    if(fail_reason)
    {
        kerror("Function '%s': Failed to load file '%s': %s.", __FUNCTION__, full_file_path, fail_reason);

        // Очистка ошибок stbi, что бы следующая текстура могла загрузиться.
        stbi__err(0, 0);

        if(data)
        {
            stbi_image_free(data);
        }

        return false;
    }

    if(!data)
    {
        kerror("Function '%s': Failed to load file '%s'.", __FUNCTION__, full_file_path);
        return false;
    }

    // TODO: Должен использоваться распределитель памяти.
    out_resource->full_path = string_duplicate(full_file_path);

    // TODO: Должен использоваться распределитель памяти.
    image_resouce_data* resource_data = kallocate_tc(image_resouce_data, 1, MEMORY_TAG_TEXTURE);
    resource_data->pixels = data;
    resource_data->width = width;
    resource_data->height = height;
    resource_data->channel_count = required_channel_count;

    out_resource->data = resource_data;
    out_resource->data_size = sizeof(image_resouce_data);
    out_resource->name = name;

    return true;
}

void image_loader_unload(resource_loader* self, resource* resource)
{
    if(string_length(resource->full_path) > 0)
    {
        string_free(resource->full_path);
    }

    if(resource->data)
    {
        kfree(resource->data, resource->data_size, MEMORY_TAG_TEXTURE);
        resource->data = null;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
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

