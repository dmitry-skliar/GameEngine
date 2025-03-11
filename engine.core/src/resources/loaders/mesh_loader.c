// Собственные подключения.
#include "resources/loaders/mesh_loader.h"
#include "resources/loaders/loader_util.h"

// Внутренние подключения.
#include "logger.h"
#include "kstring.h"
#include "math/kmath.h"
#include "math/geometry_utils.h"
#include "memory/memory.h"
#include "platform/file.h"
#include "platform/string.h"
#include "containers/darray.h"
#include "resources/resource_types.h"
#include "systems/resource_system.h"
#include "systems/geometry_system.h"

#define SUPPORTED_FILETYPE_COUNT 2
#define FILETYPE_KSM 0
#define FILETYPE_OBJ 1

// Известные файлы загрузчику.
static const loader_filetype_entry supported_filetypes[SUPPORTED_FILETYPE_COUNT] = {
    [FILETYPE_KSM] = {".ksm", LOADER_FILETYPE_MESH_KSM, true  },
    [FILETYPE_OBJ] = {".obj", LOADER_FILETYPE_MESH_OBJ, false }
};

bool load_obj_file(file* obj_file, const char* name, geometry_config** out_geometries_darray);
bool load_ksm_file(file* ksm_file, geometry_config** out_geometries_darray);
bool write_ksm_file(const char* name, geometry_config* geometries);

bool mesh_loader_load(struct resource_loader* self, const char* name, resource* out_resource)
{
    if(!resource_loader_load_valid(self, name, out_resource, __FUNCTION__))
    {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char  filepath_str[512];
    loader_filetype type = LOADER_FILETYPE_NOT_FOUND;
    file* f;

    // Попытка найти поддерживаемый файл.
    for(u32 i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i)
    {
        string_format(filepath_str, format_str, resource_system_base_path(), self->type_path, name, supported_filetypes[i].extension);

        if(platform_file_exists(filepath_str))
        {
            file_mode mode = supported_filetypes[i].is_binary ? FILE_MODE_READ | FILE_MODE_BINARY : FILE_MODE_READ;

            if(platform_file_open(filepath_str, mode, &f))
            {
                type = supported_filetypes[i].type;
                break;
            }
        }
    }

    if(type == LOADER_FILETYPE_NOT_FOUND)
    {
        kerror("Function '%s': Unable to find mesh of supported type called '%s'.", __FUNCTION__, name);
        return false;
    }

    out_resource->full_path = string_duplicate(filepath_str);
    geometry_config* resource_data = darray_create(geometry_config);

    bool result = false;
    switch(type)
    {
        case LOADER_FILETYPE_MESH_KSM:
            result = load_ksm_file(f, &resource_data);
            break;
        case LOADER_FILETYPE_MESH_OBJ:
            result = load_obj_file(f, name, &resource_data);
            write_ksm_file(name, resource_data);
            break;
        default:
            result = false;
            kfatal("Function '%s': Unknown mesh type. Stopped for debugging...", __FUNCTION__);
            break; // NOTE: LOADER_FILETYPE_NOT_FOUND тут не должен оказаться (смотри условия выше)!
    }

    platform_file_close(f);

    if(!result)
    {
        kerror("Function '%s': Failed to process mesh file '%s'.", __FUNCTION__, filepath_str);
        darray_destroy(resource_data);
        out_resource->data = null;
        out_resource->data_size = 0;
        return false;
    }

    out_resource->data = resource_data;
    // Использование размера данных как количество конфигураций геометрий.
    out_resource->data_size = darray_length(resource_data);

    return true;
}

void mesh_loader_unload(struct resource_loader* self, resource* resource)
{
    // TODO: Сделать функцию util для проверки уничтожения файлов!

    u32 count = resource->data_size;
    geometry_config* configs = resource->data;

    for(u32 i = 0; i < count; ++i)
    {
        geometry_system_config_dispose(&configs[i]);
    }

    darray_destroy(resource->data);

    if(string_length(resource->full_path) > 0)
    {
        string_free(resource->full_path);
    }

    resource->data = null;
    resource->data_size = 0;
    resource->loader_id = INVALID_ID;
}

resource_loader mesh_resource_loader_create()
{
    resource_loader loader;
    loader.type = RESOURCE_TYPE_MESH;
    loader.custom_type = null;
    loader.load = mesh_loader_load;
    loader.unload = mesh_loader_unload;
    loader.type_path = "models";

    return loader;
}

//-------------------------------------- KSM ----------------------------------------------

bool load_ksm_file(file* ksm_file, geometry_config** out_geometries_darray)
{
    // NOTE: Ниже игнорируется результат чтения!

    u16 version = 0;
    platfrom_file_read(ksm_file, sizeof(u16), &version);

    u32 name_length = 0;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    platfrom_file_read(ksm_file, sizeof(u32), &name_length);
    platfrom_file_read(ksm_file, sizeof(char) * name_length, name);

    u64 geometry_count = 0;
    platfrom_file_read(ksm_file, sizeof(u64), &geometry_count);

    for(u64 i = 0; i < geometry_count; ++i)
    {
        geometry_config gconf = {};

        // Имя сетки геометрии.
        u32 g_name_length = 0;
        platfrom_file_read(ksm_file, sizeof(u32), &g_name_length);
        platfrom_file_read(ksm_file, sizeof(char) * g_name_length, gconf.name);

        // Имя материала геометрии.
        u32 m_name_length = 0;
        platfrom_file_read(ksm_file, sizeof(u32), &m_name_length);
        platfrom_file_read(ksm_file, sizeof(char) * m_name_length, gconf.material_name);

        // Размеры и центр геометрии.
        platfrom_file_read(ksm_file, sizeof(vec3), &gconf.center);
        platfrom_file_read(ksm_file, sizeof(extents_3d), &gconf.extents);

        // Вершины (размер/количество/данные вершин)
        platfrom_file_read(ksm_file, sizeof(u32), &gconf.vertex_size);
        platfrom_file_read(ksm_file, sizeof(u32), &gconf.vertex_count);
        gconf.vertices = kallocate(gconf.vertex_size * gconf.vertex_count, MEMORY_TAG_ARRAY);
        platfrom_file_read(ksm_file, gconf.vertex_size * gconf.vertex_count, gconf.vertices);

        // Индексы (размер/количество/данные индексов)
        platfrom_file_read(ksm_file, sizeof(u32), &gconf.index_size);
        platfrom_file_read(ksm_file, sizeof(u32), &gconf.index_count);
        gconf.indices = kallocate(gconf.index_size * gconf.index_count, MEMORY_TAG_ARRAY);
        platfrom_file_read(ksm_file, gconf.index_size * gconf.index_count, gconf.indices);

        darray_push(*out_geometries_darray, gconf);
    }

    return true;
}

bool write_ksm_file(const char* name, geometry_config* geometries)
{
    char ksm_filepath[GEOMETRY_NAME_MAX_LENGTH];
    string_format(ksm_filepath, "%s/models/%s.ksm", resource_system_base_path(), name);

    if(platform_file_exists(ksm_filepath))
    {
        // kwarng("Function '%s': File '%s' is already exists.", __FUNCTION__, ksm_filepath);
        return false;
    }

    file* ksm_file;
    if(!platform_file_open(ksm_filepath, FILE_MODE_WRITE | FILE_MODE_BINARY, &ksm_file))
    {
        kerror("Function '%s': Cannot open file '%s' for binary writing. Skipping...", __FUNCTION__, ksm_filepath);
        return false;
    }

    u16 version = 0x0001U;
    platform_file_write(ksm_file, sizeof(u16), &version);

    u32 name_length = string_length(name) + 1;
    platform_file_write(ksm_file, sizeof(u32), &name_length);
    platform_file_write(ksm_file, sizeof(char) * name_length, name);

    u64 geometry_count = darray_length(geometries);
    platform_file_write(ksm_file, sizeof(u64), &geometry_count);

    for(u64 i = 0; i < geometry_count; ++i)
    {
        geometry_config* g = &geometries[i];

        // Имя сетки геометрии.
        u32 g_name_length = string_length(g->name) + 1;
        platform_file_write(ksm_file, sizeof(u32), &g_name_length);
        platform_file_write(ksm_file, sizeof(char) * g_name_length, g->name);

        // Имя материала геометрии.
        u32 m_name_length = string_length(g->material_name) + 1;
        platform_file_write(ksm_file, sizeof(u32), &m_name_length);
        platform_file_write(ksm_file, sizeof(char) * m_name_length, g->material_name);

        // Размеры и центр геометрии.
        platform_file_write(ksm_file, sizeof(vec3), &g->center);
        platform_file_write(ksm_file, sizeof(extents_3d), &g->extents);

        // Вершины (размер/количество/данные вершин)
        platform_file_write(ksm_file, sizeof(u32), &g->vertex_size);
        platform_file_write(ksm_file, sizeof(u32), &g->vertex_count);
        platform_file_write(ksm_file, g->vertex_size * g->vertex_count, g->vertices);

        // Индексы (размер/количество/данные индексов)
        platform_file_write(ksm_file, sizeof(u32), &g->index_size);
        platform_file_write(ksm_file, sizeof(u32), &g->index_count);
        platform_file_write(ksm_file, g->index_size * g->index_count, g->indices);
    }

    platform_file_close(ksm_file);
    return true;
}

//-------------------------------------- OBJ ----------------------------------------------

typedef struct mesh_vertex_index_data {
    u32 position_index;
    u32 normal_index;
    u32 texcoord_index;
} mesh_vertex_index_data;

typedef struct mesh_face_data {
    mesh_vertex_index_data vertices[3];
} mesh_face_data;

typedef struct mesh_group_data {
    // Используется darray.
    mesh_face_data* faces;
    char material_name[MATERIAL_NAME_MAX_LENGTH];
    bool smooth;
} mesh_group_data;

typedef struct index_entry {
    u32 id;                    // По сути собственный индекс.
    u32 index;                 // Новый индекс.
    bool has;                  // Указывает что запись существует.
    struct index_entry *next;  // Следующая запись по текущему id.
} index_entry;

/*
    @brief Выполняет разбор obj файла модели и запись в предоставленные динамические массивы.
    @param obj_file Указатель на открытый obj файл модели.
    @param positions Указатель на динамический массив позиций вершин (указатель на darray).
    @param normals Указатель на динамический массив нормалей вершин (указатель на darray).
    @param texcoords Указатель на динамический массив координат текстур вершин (указатель на darray).
    @param groups Указатель на динамический массив групп индексов вершин (указатель на darray).
    @param out_material_filename Указатель на массив символов для записи имени файла материалов (без расширения).
*/
bool parse_obj_file(file* obj_file, vec3** positions, vec3** normals, vec2** texcoords, mesh_group_data** groups, char* out_material_filename)
{
    u64 normal_count = 0;
    u64 texcoord_count = 0;
    u64 group_current_index = 0;
    char bufferline[512] = "";
    u64 line_length = 0;
    // u32 line_number = 1;
    
    while(platform_file_read_line(obj_file, 511, bufferline, &line_length))
    {
        // Пропуск пустых строк и коментариев.
        if(line_length < 1 || bufferline[0] == '#')
        {
            // line_number++;
            continue;
        }

        if(string_nequali(bufferline, "mtllib ", 7))
        {
            // TODO: Может быть несколько файлов.
            platform_string_sscanf(&bufferline[7], "%s", out_material_filename);
        }
        else if(string_nequali(bufferline, "v ", 2))
        {
            vec3 pos;
            string_to_vec3(&bufferline[2], &pos);
            darray_push(*positions, pos);
        }
        else if(string_nequali(bufferline, "vt ", 3))
        {
            vec2 tex;
            string_to_vec2(&bufferline[3], &tex);
            darray_push(*texcoords, tex);
            texcoord_count++;
        }
        else if(string_nequali(bufferline, "vn ", 3))
        {
            vec3 nor;
            string_to_vec3(&bufferline[3], &nor);
            darray_push(*normals, nor);
            normal_count++;
        }
        else if(string_nequali(bufferline, "usemtl ", 7))
        {
            mesh_group_data new_group;
            string_ncopy(new_group.material_name, &bufferline[7], MATERIAL_NAME_MAX_LENGTH);
            string_trim(new_group.material_name);
            new_group.faces = darray_reserve(mesh_face_data, 16384);

            darray_push(*groups, new_group);

            // Обновление индекса.
            group_current_index = darray_length(*groups) - 1;

        }
        else if(string_nequali(bufferline, "s ", 2))
        {
            // TODO: Обработать!
        }
        else if(string_nequali(bufferline, "g ", 2))
        {
            // TODO: Обработать!
            
        }
        else if(string_nequali(bufferline, "f ", 2))
        {
            // face                        vert 1      vert 2      vert 3
            // f 1 2 3             ==        1            2           3
            // f 1/1/1 2/2/2 3/3/3 == pos/tex/norm pos/tex/norm pos/tex/norm
            mesh_face_data face;

            if(normal_count == 0 || texcoord_count == 0)
            {
                platform_string_sscanf(
                    &bufferline[2], "%d %d %d",
                    &face.vertices[0].position_index, &face.vertices[1].position_index, &face.vertices[2].position_index
                );
            }
            else
            {
                platform_string_sscanf(
                    &bufferline[2], "%d/%d/%d %d/%d/%d %d/%d/%d",
                    &face.vertices[0].position_index, &face.vertices[0].texcoord_index, &face.vertices[0].normal_index,
                    &face.vertices[1].position_index, &face.vertices[1].texcoord_index, &face.vertices[1].normal_index,
                    &face.vertices[2].position_index, &face.vertices[2].texcoord_index, &face.vertices[2].normal_index
                );
            }

            darray_push((*groups)[group_current_index].faces, face);
        }

        kzero_tc(bufferline, char, 512);
        // line_number++;
    }

    // NOTE: Раскоментировать для отладки.
    // kdebug("PARSE OBJ FILE STATISTIC:");
    // kdebug("Position count %llu", darray_length(*positions));
    // kdebug("Texcoord count %llu", darray_length(*texcoords));
    // kdebug("Normal   count %llu", darray_length(*normals));

    // u64 gcount = darray_length(*groups);
    // kdebug("Group    count %llu", gcount);
    // for(u64 i = 0; i < gcount; ++i)
    // {
    //     kdebug("->[%2llu]: %llu faces.", i, darray_length((*groups)[i].faces) * 3);
    // }

    return true;
}

/*
    @brief Конвертирует данные obj файла из предоставленных динамических массивов в geometry_config.
    @param positions Указатель на динамический массив позиций вершин (указатель на darray).
    @param normals Указатель на динамический массив нормалей вершин (указатель на darray).
    @param texcoords Указатель на динамический массив координат текстур вершин (указатель на darray).
    @param group Указатель на группу индексов вершин.
    @param config Указатель на geometry_config элемент для выполнения записи данных.
*/
bool convert_obj_group_to(vec3* positions, vec3* normals, vec2* texcoords, mesh_group_data* group, geometry_config* config)
{
    // Копирование имени материала.
    string_ncopy(config->material_name, group->material_name, MATERIAL_NAME_MAX_LENGTH);

    bool extent_set = false;
    kzero_tc(&config->extents, extents_3d, 1);

    u32 position_index = 0;
    u64 position_count = darray_length(positions);
    u64 normal_count = darray_length(normals);
    u64 texcoord_count = darray_length(texcoords);

    // Вспомогательная индексная таблица (помогает исключить дублирование).
    u64 existing_index_next = position_count;
    index_entry* existing_indices = darray_reserve(index_entry, position_count);
    kzero_tc(existing_indices, index_entry, position_count);

    // Преобразование лицевых поверхностей
    u64 faces_count = darray_length(group->faces);
    for(u64 f = 0; f < faces_count; ++f)
    {
        mesh_face_data* face = &group->faces[f];
        vertex_3d current_vert;

        for(u64 i = 0; i < 3; ++i)
        {
            mesh_vertex_index_data* index_data = &face->vertices[i];
            u32 obj_position_index = index_data->position_index - 1;

            // Создание вершины, вероятно новой.
            current_vert.position = positions[obj_position_index];
            current_vert.texcoord = texcoord_count ? texcoords[index_data->texcoord_index - 1] : vec2_zero();
            current_vert.normal = normal_count ? normals[index_data->normal_index - 1] : vec3_create(0, 0, 1);
            current_vert.color = vec4_one();    // TODO: Цвет. А пока по-умолчанию белый цвет.
            current_vert.tangent = vec4_zero(); // TODO: Тангент. А пока по-умолчанию 0 вектор.

            // Поиск существующего индекса. Вероятно это может быть та самая вершина, если это не так, то
            // назначим ей новый индекс и по старому индексу сохраним ссылку на следующий индекс и т.д.
            if(existing_indices[obj_position_index].has)
            {
                // Получение записи во вспомогательной индексной таблице.
                index_entry* entry = &existing_indices[obj_position_index];
                index_entry* prev  = null;
                bool found = false;

                // NOTE: Раскоментировать для отладки.
                // kerror("Face trace: v/vt/vn: %u/%u/%u", index_data->position_index, index_data->texcoord_index, index_data->normal_index);
                // kwarng(
                //     "Current  pos: x=%.6f, y=%.6f, z=%.6f, tex: u=%.6f, v=%.6f, norm: x=%.6f, y=%.6f, z=%.6f",
                //     current_vert.position.x, current_vert.position.y, current_vert.position.z,
                //     current_vert.texcoord.u, current_vert.texcoord.v,
                //     current_vert.normal.x, current_vert.normal.y, current_vert.normal.z
                // );

                // Поиск..
                while(entry)
                {
                    // NOTE: тип config->vertices это void* из-за чего индексация будет побайтной, а потому необходимо
                    //       привести к vertex_3d*.
                    vertex_3d* exist_vert = &((vertex_3d*)config->vertices)[entry->index];

                    // NOTE: Раскоментировать для отладки.
                    // ktrace(
                    //     "Existing pos: x=%.6f, y=%.6f, z=%.6f, tex: u=%.6f, v=%.6f, norm: x=%.6f, y=%.6f, z=%.6f",
                    //     exist_vert->position.x, exist_vert->position.y, exist_vert->position.z,
                    //     exist_vert->texcoord.u, exist_vert->texcoord.v,
                    //     exist_vert->normal.x, exist_vert->normal.y, exist_vert->normal.z
                    // );

                    if(vertex_3d_equal(*exist_vert, current_vert))
                    {
                        obj_position_index = entry->id;
                        found = true;
                        break;
                    }

                    prev  = entry;
                    entry = entry->next;
                }

                // Необходима дополнительная запись для одинаковых индексов.
                if(!found)
                {
                    u64 current_capacity = darray_capacity(existing_indices);
                    if(existing_index_next >= current_capacity)
                    {
                        u64 new_capacity = current_capacity * 2;
                        index_entry* new = darray_reserve(index_entry, new_capacity);
                        kzero_tc(new, index_entry, new_capacity);
                        kcopy_tc(new, existing_indices, index_entry, current_capacity);

                        darray_destroy(existing_indices);
                        existing_indices = new;
                    }

                    prev->next = &existing_indices[existing_index_next];
                    obj_position_index = existing_index_next++;
                }
            }

            // Добавление вершины и индекса.
            if(!existing_indices[obj_position_index].has)
            {
                existing_indices[obj_position_index].id = obj_position_index;
                existing_indices[obj_position_index].index = position_index;
                existing_indices[obj_position_index].has = true;

                darray_push(config->vertices, current_vert);
                position_index++;

                // Получаем минимальную точку занимаемой моделью в пространстве.
                if(current_vert.position.x < config->extents.min.x || !extent_set)
                {
                    config->extents.min.x = current_vert.position.x;
                }

                if(current_vert.position.y < config->extents.min.y || !extent_set)
                {
                    config->extents.min.y = current_vert.position.y;
                }

                if(current_vert.position.z < config->extents.min.z || !extent_set)
                {
                    config->extents.min.z = current_vert.position.z;
                }

                // Получаем максимальную точку занимаемой моделью в пространстве.
                if(current_vert.position.x > config->extents.max.x || !extent_set)
                {
                    config->extents.max.x = current_vert.position.x;
                }

                if(current_vert.position.y > config->extents.max.y || !extent_set)
                {
                    config->extents.max.y = current_vert.position.y;
                }

                if(current_vert.position.z > config->extents.max.z || !extent_set)
                {
                    config->extents.max.z = current_vert.position.z;
                }

                // Начальная инициализация крайних точек завершена.
                extent_set = true;
            }

            darray_push(config->indices, existing_indices[obj_position_index].index);
        }
    }

    // Уничтожение вспомогательной таблицы индексов.
    darray_destroy(existing_indices);

    return true;
}

// TODO: Переработать!
// NOTE: Выглядит ужасно, но это временная мера.
bool import_mtl_file(const char* mtl_filename, const char* kmt_filename)
{
    char kmt_filepath[MATERIAL_NAME_MAX_LENGTH];
    char mtl_filepath[MATERIAL_NAME_MAX_LENGTH];
    string_format(mtl_filepath, "%s/models/%s", resource_system_base_path(), mtl_filename);
    string_format(kmt_filepath, "%s/materials/%s.kmt", resource_system_base_path(), kmt_filename);

    if(platform_file_exists(kmt_filepath))
    {
        // kwarng("Function '%s': File '%s' is already exists.", __FUNCTION__, kmt_filepath);
        return false;
    }

    file* mtl_file;
    if(!platform_file_open(mtl_filepath, FILE_MODE_READ, &mtl_file))
    {
        kerror("Function '%s': Cannot open file '%s' for reading. Skipping...", __FUNCTION__, mtl_filepath);
        return false;
    }

    file* kmt_file;
    if(!platform_file_open(kmt_filepath, FILE_MODE_WRITE, &kmt_file))
    {
        kerror("Function '%s': Cannot open file '%s' for writing. Skipping...", __FUNCTION__, mtl_filepath);
        platform_file_close(mtl_file);
        return false;
    }

    char texture_name[256];
    char write_line_str[256];
    platform_file_write_line(kmt_file, "version=0.1");
    string_format(write_line_str, "name=%s", kmt_filename);
    platform_file_write_line(kmt_file, write_line_str);
    platform_file_write_line(kmt_file, "shader=Builtin.MaterialShader");

    char bufferline[512] = "";
    u64 line_length = 0;
    bool line_skip = true;
    bool bump_written = false;
    // u32 line_number = 1;

    while(platform_file_read_line(mtl_file, 511, bufferline, &line_length))
    {
        char* trimmed = string_trim(bufferline);
        line_length = string_length(trimmed);
        
        // Пропуск пустых строк и коментариев.
        if(line_length < 1 || bufferline[0] == '#')
        {
            // line_number++;
            continue;
        }

        if(string_nequali(trimmed, "newmtl ", 7))
        {
            line_skip = true;

            if(string_equali(&trimmed[7], kmt_filename))
            {
                line_skip = false;
            }
        }
        else if(line_skip)
        {
            continue;
        }
        else if(string_nequali(trimmed, "Ns ", 3))
        {
            // Коэффициент зеркального отражения.
            string_append_string(write_line_str, "shininess=", &trimmed[3]);
            platform_file_write_line(kmt_file, write_line_str);
        }
        else if(string_nequali(trimmed, "Ka ", 3))
        {
            // TODO: Обработать! Ambient-свойства материала (цвет окружающего освещения)!
        }
        else if(string_nequali(trimmed, "Kd ", 3))
        {
            // Диффузный цвет материала.
            string_format(write_line_str, "diffuse_color=%s 1.0", &trimmed[3]);
            platform_file_write_line(kmt_file, write_line_str);
        }
        else if(string_nequali(trimmed, "Ks ", 3))
        {
            // TODO: Обработать! Specular-свойства материала!
        }
        else if(string_nequali(trimmed, "Ke ", 3))
        {
            // TODO: Обработать!
        }
        else if(string_nequali(trimmed, "Ni ", 3))
        {
            // TODO: Обработать!
        }
        else if(string_nequali(trimmed, "d ", 2))
        {
            // TODO: Обработать!
        }
        else if(string_nequali(trimmed, "illum ", 6))
        {
            // TODO: Обработать!
        }
        else if(string_nequali(trimmed, "map_Kd ", 7))
        {
            string_filename_from_path(texture_name, &trimmed[7], false);
            string_append_string(write_line_str, "diffuse_map_name=", texture_name);
            platform_file_write_line(kmt_file, write_line_str);
        }
        else if(string_nequali(trimmed, "map_Ks ", 7))
        {
            string_filename_from_path(texture_name, &trimmed[7], false);
            string_append_string(write_line_str, "specular_map_name=", texture_name);
            platform_file_write_line(kmt_file, write_line_str);
        }
        else if(!bump_written && string_nequali(trimmed, "map_bump ", 9))
        {
            string_filename_from_path(texture_name, &trimmed[9], false);
            string_append_string(write_line_str, "normal_map_name=", texture_name);
            platform_file_write_line(kmt_file, write_line_str);
            bump_written = true;
        }
        else if(!bump_written && string_nequali(trimmed, "bump ", 5))
        {
            string_filename_from_path(texture_name, &trimmed[5], false);
            string_append_string(write_line_str, "normal_map_name=", texture_name);
            platform_file_write_line(kmt_file, write_line_str);
            bump_written = true;
        }

        kzero_tc(bufferline, char, 512);
        // line_number++;
    }

    platform_file_close(mtl_file);
    platform_file_close(kmt_file);

    return true;
}

bool load_obj_file(file* obj_file, const char* name, geometry_config** out_geometries_darray)
{
    char mtl_filename[MATERIAL_NAME_MAX_LENGTH];
    vec3* positions = darray_reserve(vec3, 16384);
    vec3* normals = darray_reserve(vec3, 16384);
    vec2* texcoords = darray_reserve(vec2, 16384);
    mesh_group_data* groups = darray_reserve(mesh_group_data, 4);

    // Разбор obj файла и формирование исходных данных.
    parse_obj_file(obj_file, &positions, &normals, &texcoords, &groups, mtl_filename);

    u32 group_count = darray_length(groups);
    u32 normal_count = darray_length(normals);
    u32 texcoord_count = darray_length(texcoords);

    // Формирование групп геометрий.
    for(u64 i = 0; i < group_count; ++i)
    {
        // TODO: Временная функция импорта материалов из mtl в kmt.
        import_mtl_file(mtl_filename, groups[i].material_name);

        geometry_config new_config;
        string_ncopy(new_config.name, name, GEOMETRY_NAME_MAX_LENGTH);
        string_append_u64(new_config.name, new_config.name, i);

        u64 optimal_capacity = darray_length(groups[i].faces);
        new_config.vertex_size = sizeof(vertex_3d);
        new_config.vertices = darray_reserve(vertex_3d, optimal_capacity);
        new_config.index_size = sizeof(u32);
        new_config.indices = darray_reserve(u32, optimal_capacity);

        if(!normal_count)
        {
            kwarng("Function '%s': No normals are present in model '%s' (group %llu).", __FUNCTION__, name, i);
        }

        if(!texcoord_count)
        {
            kwarng("Function '%s': No texture coordinates are present in model '%s' (group %llu).", __FUNCTION__, name, i);
        }

        // Получение первой группы.
        convert_obj_group_to(positions, normals, texcoords, &groups[i], &new_config);

        // TODO: В отдельную функцию.
        // Расчет центра модели на основе крайних точек модели.
        for(u8 i = 0; i < 3; ++i)
        {
            new_config.center.elements[i] = (new_config.extents.min.elements[i] + new_config.extents.max.elements[i]) / 2.0f;
        }

        // Расчет тангентов.
        new_config.vertex_count = darray_length(new_config.vertices);
        new_config.index_count = darray_length(new_config.indices);
        geometry_generate_tangent(new_config.vertex_count, new_config.vertices, new_config.index_count, new_config.indices);

        // Пересоздание vertices и indices с использованием kallocate.
        void* vertices = kallocate_tc(vertex_3d, new_config.vertex_count, MEMORY_TAG_ARRAY);
        void* indices = kallocate_tc(u32, new_config.index_count, MEMORY_TAG_ARRAY);
        kcopy_tc(vertices, new_config.vertices, vertex_3d, new_config.vertex_count);
        kcopy_tc(indices, new_config.indices, u32, new_config.index_count);
        darray_destroy(new_config.vertices);
        darray_destroy(new_config.indices);
        new_config.vertices = vertices;
        new_config.indices = indices;

        // NOTE: Раскоментировать для отладки.
        // kdebug("LOAD OBJECT FILE -> GROUP %llu:", i);
        // kdebug("Vertices count %llu", new_config.vertex_count);
        // kdebug("Indices count %llu", new_config.index_count);

        // Записть конечного результата.
        darray_push(*out_geometries_darray, new_config);

        // Очистка текущей группы (генерируется в parse_obj_file).
        darray_destroy(groups[i].faces);
    }

    // Освобождение darray массивов.
    darray_destroy(groups);
    darray_destroy(positions);
    darray_destroy(normals);
    darray_destroy(texcoords);

    return true;
}
