// Собственные подключения.
#include "resources/loaders/shader_loader.h"
#include "resources/loaders/loader_util.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "memory/memory.h"
#include "containers/darray.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "platform/file.h"

// TODO: Где-то утечка строк 12 байт на вызов!
bool shader_loader_load(resource_loader* self, const char* name, resource* out_resource)
{
    if(!resource_loader_load_valid(self, name, out_resource, __FUNCTION__))
    {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char  filepath_str[512];                // TODO: Сделать общей константой. Добавить в проверку длину имени!
    string_format(filepath_str, format_str, resource_system_base_path(), self->type_path, name, ".shadercfg");

    file* f;
    if(!platform_file_open(filepath_str, FILE_MODE_READ, &f))
    {
        kerror("Function '%s': Unable to open shader config file '%s' for reading.", __FUNCTION__, filepath_str);
        return false;
    }

    out_resource->full_path = string_duplicate(filepath_str);
    shader_config* resource_data = kallocate_tc(shader_config, 1, MEMORY_TAG_RESOURCE);

    resource_data->attribute_count = 0;
    resource_data->attributes = darray_create(shader_attribute_config);
    resource_data->uniform_count = 0;
    resource_data->uniforms = darray_create(shader_uniform_config);
    resource_data->stage_count = 0;
    resource_data->stages = darray_create(shader_stage);
    resource_data->stage_names = darray_create(char*);
    resource_data->stage_filenames = darray_create(char*);
    resource_data->use_instances = false;
    resource_data->use_local = false;
    resource_data->renderpass_name = null;
    resource_data->name = null;

    char bufferline[512] = "";
    char* p = bufferline;
    u64 line_length = 0;
    u32 line_number = 1;

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

        // Поиск смещения токена '='.
        i32 equal_index = string_index_of(trimmed, '=');
        if(equal_index == -1)
        {
            kwarng(
                "Function '%s': Potential formatting issue found in file '%s': '=' token not found. Skipping line %u.",
                __FUNCTION__, filepath_str, line_number
            );
            line_number++;
            continue;
        }

        // Максимальная длина имени 64 символа.
        char raw_var_name[64];
        kzero_tc(raw_var_name, char, 64);
        string_mid(raw_var_name, trimmed, 0, equal_index);
        char* trimmed_var_name = string_trim(raw_var_name);

        // Максимальная длина переменой 446 (511-65 с учетом токена '=').
        char raw_value[446];
        kzero_tc(raw_value, char, 446);
        string_mid(raw_value, trimmed, equal_index + 1, -1); // Читать до конца строки.
        char* trimmed_value = string_trim(raw_value);

        // Обработка переменной.
        if(string_equali(trimmed_var_name, "version"))
        {
            // TODO: Сделать версию файла.
        }
        else if(string_equali(trimmed_var_name, "name"))
        {
            resource_data->name = string_duplicate(trimmed_value);
        }
        else if(string_equali(trimmed_var_name, "renderpass"))
        {
            resource_data->renderpass_name = string_duplicate(trimmed_value);
        }
        else if(string_equali(trimmed_var_name, "stages"))
        {
            u32 count = string_split(trimmed_value, ',', true, true, &resource_data->stage_names);

            if(resource_data->stage_count == 0)
            {
                resource_data->stage_count = count;
            }
            else if(resource_data->stage_count != count)
            {
                kerror(
                    "Function '%s': Invalid file layout. Count mismatch between stage names and stage filenames.",
                    __FUNCTION__
                );
            }

            char** stage_names = resource_data->stage_names;

            for(u8 i = 0; i < resource_data->stage_count; ++i)
            {
                if(string_equali(stage_names[i], "frag") || string_equali(stage_names[i], "fragment"))
                {
                    darray_push(resource_data->stages, SHADER_STAGE_FRAGMENT);
                }
                else if(string_equali(stage_names[i], "vert") || string_equali(stage_names[i], "vertex"))
                {
                    darray_push(resource_data->stages, SHADER_STAGE_VERTEX);
                }
                else if(string_equali(stage_names[i], "geom") || string_equali(stage_names[i], "geometry"))
                {
                    darray_push(resource_data->stages, SHADER_STAGE_GEOMETRY);
                }
                else if(string_equali(stage_names[i], "comp") || string_equali(stage_names[i], "compute"))
                {
                    darray_push(resource_data->stages, SHADER_STAGE_COMPUTE);
                }
                else
                {
                    kerror("Function '%s': Invalid file layout. Unrecognized stage '%s'", __FUNCTION__, stage_names[i]);
                }
            }
        }
        else if(string_equali(trimmed_var_name, "stagefiles"))
        {
            u32 count = string_split(trimmed_value, ',', true, true, &resource_data->stage_filenames);

            if(resource_data->stage_count == 0)
            {
                resource_data->stage_count = count;
            }
            else if(resource_data->stage_count != count)
            {
                kerror(
                    "Function '%s': Invalid file layout. Count mismatch between stage names and stage filenames.",
                    __FUNCTION__
                );
            }
        }
        else if(string_equali(trimmed_var_name, "use_instance"))
        {
            string_to_bool(trimmed_value, &resource_data->use_instances);
        }
        else if(string_equali(trimmed_var_name, "use_local"))
        {
            string_to_bool(trimmed_value, &resource_data->use_local);
        }
        else if(string_equali(trimmed_var_name, "attribute"))
        {
            char** fields = darray_create(char*);
            u32 filed_count = string_split(trimmed_value, ',', true, true, &fields);

            if(filed_count != 2)
            {
                kerror(
                    "Function '%s': Invalid file layout. Attribute fields must be 'type,name'. Skipping.",
                    __FUNCTION__
                );
            }
            else
            {
                shader_attribute_config attribute;

                if(string_equali(fields[0], "f32"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32;
                    attribute.size = 4;
                }
                else if(string_equali(fields[0], "vec2"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32_2;
                    attribute.size = 8;
                }
                else if(string_equali(fields[0], "vec3"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32_3;
                    attribute.size = 12;
                }
                else if(string_equali(fields[0], "vec4"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32_4;
                    attribute.size = 16;
                }
                else if(string_equali(fields[0], "u8"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_UINT8;
                    attribute.size = 1;
                }
                else if(string_equali(fields[0], "u16"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_UINT16;
                    attribute.size = 2;
                }
                else if(string_equali(fields[0], "u32"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_UINT32;
                    attribute.size = 4;
                }
                else if(string_equali(fields[0], "i8"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_INT8;
                    attribute.size = 1;
                }
                else if(string_equali(fields[0], "i16"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_INT16;
                    attribute.size = 2;
                }
                else if(string_equali(fields[0], "i32"))
                {
                    attribute.type = SHADER_ATTRIB_TYPE_INT32;
                    attribute.size = 4;
                }
                else
                {
                    kerror(
                        "Function '%s': Invalid file layout. Attribute type must be f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, or u32.",
                        __FUNCTION__
                    );

                    kwarng("Function '%s': Defaulting to f32.", __FUNCTION__);

                    attribute.type = SHADER_ATTRIB_TYPE_FLOAT32;
                    attribute.size = 4;
                }

                attribute.name = string_duplicate(fields[1]);

                darray_push(resource_data->attributes, attribute);
                resource_data->attribute_count++;
            }

            string_cleanup_split_array(fields);
            darray_destroy(fields);
        }
        else if(string_equali(trimmed_var_name, "uniform"))
        {
            char** fields = darray_create(char*);
            u32 field_count = string_split(trimmed_value, ',', true, true, &fields);

            if (field_count != 3)
            {
                kerror(
                    "Function '%s': Invalid file layout. Uniform fields must be 'type,scope,name'. Skipping.",
                    __FUNCTION__
                );
            }
            else
            {
                shader_uniform_config uniform;

                if(string_equali(fields[0], "f32"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32;
                    uniform.size = 4;
                }
                else if(string_equali(fields[0], "vec2"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32_2;
                    uniform.size = 8;
                }
                else if(string_equali(fields[0], "vec3"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32_3;
                    uniform.size = 12;
                }
                else if(string_equali(fields[0], "vec4"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32_4;
                    uniform.size = 16;
                }
                else if(string_equali(fields[0], "u8"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_UINT8;
                    uniform.size = 1;
                }
                else if(string_equali(fields[0], "u16"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_UINT16;
                    uniform.size = 2;
                }
                else if(string_equali(fields[0], "u32"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_UINT32;
                    uniform.size = 4;
                }
                else if(string_equali(fields[0], "i8"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_INT8;
                    uniform.size = 1;
                }
                else if(string_equali(fields[0], "i16"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_INT16;
                    uniform.size = 2;
                }
                else if(string_equali(fields[0], "i32"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_INT32;
                    uniform.size = 4;
                }
                else if(string_equali(fields[0], "mat4"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_MATRIX_4;
                    uniform.size = 64;
                }
                else if(string_equali(fields[0], "samp") || string_equali(fields[0], "sampler"))
                {
                    uniform.type = SHADER_UNIFORM_TYPE_SAMPLER;
                    uniform.size = 0; // У сэмплера нет размера.
                }
                else
                {
                    kerror(
                        "Function '%s': Invalid file layout. Uniform type must be f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, u32 or mat4.",
                        __FUNCTION__
                    );

                    kwarng("Function '%s': Defaulting to f32.", __FUNCTION__);

                    uniform.type = SHADER_UNIFORM_TYPE_FLOAT32;
                    uniform.size = 4;
                }

                if(string_equal(fields[1], "0"))
                {
                    uniform.scope = SHADER_SCOPE_GLOBAL;
                }
                else if(string_equal(fields[1], "1"))
                {
                    uniform.scope = SHADER_SCOPE_INSTANCE;
                }
                else if(string_equal(fields[1], "2"))
                {
                    uniform.scope = SHADER_SCOPE_LOCAL;
                }
                else
                {
                    kerror(
                        "Function '%s': Invalid file layout: Uniform scope must be 0 for global, 1 for instance or 2 for local.",
                        __FUNCTION__
                    );

                    kwarng("Function '%s': Defaulting to global.", __FUNCTION__);
                    uniform.scope = SHADER_SCOPE_GLOBAL;
                }

                uniform.name = string_duplicate(fields[2]);

                darray_push(resource_data->uniforms, uniform);
                resource_data->uniform_count++;
            }

            string_cleanup_split_array(fields);
            darray_destroy(fields);
        }

        // TODO: Сделать больше полей.

        kzero_tc(bufferline, char, 512);
        line_number++;
    }

    platform_file_close(f);

    out_resource->data = resource_data;
    out_resource->data_size = sizeof(shader_config);
    return true;
}

void shader_loader_unload(struct resource_loader* self, resource* resource)
{
    shader_config* data = resource->data;

    string_cleanup_split_array(data->stage_filenames);
    darray_destroy(data->stage_filenames);

    string_cleanup_split_array(data->stage_names);
    darray_destroy(data->stage_names);

    darray_destroy(data->stages);

    for(u32 i = 0; i < data->attribute_count; ++i)
    {
        string_free(data->attributes[i].name);
    }
    darray_destroy(data->attributes);

    for(u32 i = 0; i < data->uniform_count; ++i)
    {
        string_free(data->uniforms[i].name);
    }
    darray_destroy(data->uniforms);

    string_free(data->renderpass_name);
    string_free(data->name);
    kzero_tc(data, shader_config, 1);

    resource_unload(self, resource, MEMORY_TAG_RESOURCE, __FUNCTION__);
}

resource_loader shader_resource_loader_create()
{
    resource_loader loader;
    loader.type = RESOURCE_TYPE_SHADER;
    loader.custom_type = null;
    loader.load = shader_loader_load;
    loader.unload = shader_loader_unload;
    loader.type_path = "shaders";

    return loader;
}
