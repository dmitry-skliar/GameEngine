// Собственные подключения.
#include "resources/loaders/material_loader.h"
#include "resources/loaders/loader_util.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "math/kmath.h"
#include "platform/file.h"

bool material_loader_load(resource_loader* self, const char* name, resource* out_resource)
{
    char* format_str = "%s/%s/%s%s";
    char full_file_path[512];
    string_format(full_file_path, format_str, resource_system_base_path(), self->type_path, name, ".kmt");

    file* f;
    if(!platform_file_open(full_file_path, FILE_MODE_READ, &f))
    {
        kerror("Function '%s': Unable to open material file '%s' for reading.", __FUNCTION__, full_file_path);
        return false;
    }

    // TODO: Должен использоваться распределитель памяти.
    out_resource->full_path = string_duplicate(full_file_path);

    // TODO: Должен использоваться распределитель памяти.
    material_config* resource_data = kallocate_tc(material_config, 1, MEMORY_TAG_MATERIAL);
    resource_data->shader_name = "Builtin.Material";
    resource_data->auto_release = true;
    resource_data->diffuse_color = vec4_one(); // белый.
    string_empty(resource_data->diffuse_map_name);
    string_ncopy(resource_data->name, name, MATERIAL_NAME_MAX_LENGTH);

    char bufferline[512] = "";
    char* p = bufferline;
    u64 line_length = 0;
    u32 line_number = 1;

    // Построчное чтение файла.
    while(platform_file_read_line(f, 511, p, &line_length))
    {
        char* trimmed = string_trim(bufferline);
        line_length = string_length(trimmed);

        // Пропуск пустых строк и коментариев.
        if(line_length < 1 || trimmed[0] == '#')
        {
            line_number++;
            continue;
        }

        i32 equal_index = string_index_of(trimmed, '=');
        if(equal_index == INVALID_ID)
        {

            kwarng(
                "Potential formatting issue found in file '%s': '=' token not found. Skipping line %u.",
                full_file_path, line_number
            );
            line_number++;
            continue;
        }

        // Максимальное количество символов для имени переменной - 64.
        char raw_var_name[64];
        kzero_tc(raw_var_name, char, 64);
        string_mid(raw_var_name, trimmed, 0, equal_index);
        char* trimmed_var_name = string_trim(raw_var_name);

        // Максимальное количество символов для значения - 446 (511–65).
        char raw_value[446];
        kzero_tc(raw_value, char, 446);
        string_mid(raw_value, trimmed, equal_index + 1, -1);
        char* trimmed_value = string_trim(raw_value);

        // Процесс формирования.
        if(string_equali(trimmed_var_name, "version"))
        {
            // TODO: Версия.
        }
        else if(string_equali(trimmed_var_name, "name"))
        {
            string_ncopy(resource_data->name, trimmed_value, MATERIAL_NAME_MAX_LENGTH);
        }
        else if(string_equali(trimmed_var_name, "diffuse_map_name"))
        {
            string_ncopy(resource_data->diffuse_map_name, trimmed_value, TEXTURE_NAME_MAX_LENGTH);
        }
        else if(string_equali(trimmed_var_name, "diffuse_color"))
        {
            if(!string_to_vec4(trimmed_value, &resource_data->diffuse_color))
            {
                kwarng("Error parsing diffuse_color in file '%s'. Using default of white instead.", full_file_path);
                // NOTE: Уже задано выше.
            }
        }
        else if(string_equali(trimmed_var_name, "shader"))
        {
            resource_data->shader_name = string_duplicate(trimmed_value);
        }

        // TODO: Другие поля.

        // Очистка буфера.
        kzero(bufferline, sizeof(char) * 512);
        line_number++;
    }

    platform_file_close(f);

    out_resource->data = resource_data;
    out_resource->data_size = sizeof(material_config);
    out_resource->name = name;

    return true;
}

void material_loader_unload(resource_loader* self, resource* resource)
{
    material_config* data = resource->data;
    string_free(data->shader_name);

    resource_unload(self, resource, MEMORY_TAG_MATERIAL, __FUNCTION__);
}

resource_loader material_resource_loader_create()
{
    resource_loader loader;
    loader.type = RESOURCE_TYPE_MATERIAL;
    loader.custom_type = null;
    loader.load = material_loader_load;
    loader.unload = material_loader_unload;
    loader.type_path = "materials";

    return loader;
}
