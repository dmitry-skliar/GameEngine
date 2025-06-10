// Собственные подключения.
#include "renderer/views/render_view_world.h"

// Внутренние подключения.
#include "logger.h"
#include "event.h"
#include "memory/memory.h"
#include "math/kmath.h"
#include "math/transform.h"
#include "containers/darray.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"
#include "systems/camera_system.h"
#include "renderer/renderer_frontend.h"

typedef struct render_view_world_internal_data {
    u32 shader_id;
    f32 fov;
    f32 near_clip;
    f32 far_clip;
    mat4 projection_matrix;
    camera* world_camera;
    vec4 ambient_color;
    u32 render_mode;
} render_view_world_internal_data;

typedef struct geometry_distance {
    geometry_render_data g;
    f32 distance;            // Дистанция относительно камеры.
} geometry_distance;

static bool view_state_valid(const render_view* self, const char* func_name)
{
    if(!self || !self->internal_data)
    {
        if(func_name) kerror("Function '%s' requires a valid pointer to a view and its internal data.", func_name);
        return false;
    }
    return true;
}

static void swap(geometry_distance* a, geometry_distance* b)
{
    geometry_distance t  = *a;
    *a = *b;
    *b = t;
}

static i32 partition(geometry_distance* arr, i32 low_index, i32 high_index, bool ascending)
{
    geometry_distance pivot = arr[high_index];
    i32 i = (low_index - 1);

    for(i32 j = low_index; j <= high_index - 1; ++j)
    {
        if(ascending)
        {
            if(arr[j].distance < pivot.distance)
            {
                ++i;
                swap(&arr[i], &arr[j]);
            }
        }
        else
        {
            if(arr[j].distance > pivot.distance)
            {
                ++i;
                swap(&arr[i], &arr[j]);
            }
        }
    }

    swap(&arr[i + 1], &arr[high_index]);
    return i + 1;
}

/*
    @brief Функция сортировки геометрий.
    @param arr Массив геометрий с дистанцией до камеры для сортировки.
    @param low_index Начальный индекс с которого нужно выполнять сортировку (обычно с 0).
    @param high_index Конечноый индекс до которого нужно выполнять сортировку (для всего массива -1).
    @param ascending True сортировка в порядке возрастания, false в порядке убывания.
*/
static void quick_sort(geometry_distance* arr, i32 low_index, i32 high_index, bool ascending)
{
    if(low_index < high_index)
    {
        i32 partition_index = partition(arr, low_index, high_index, ascending);

        quick_sort(arr, low_index, partition_index - 1, ascending);
        quick_sort(arr, partition_index + 1, high_index, ascending);
    }
}

bool render_view_world_on_event(event_code code, void* sender, void* listener, event_context* context)
{
    if(code == EVENT_CODE_SET_RENDER_MODE && listener)
    {
        render_view_world_internal_data* data = listener;

        switch(context->i32[0])
        {
            case RENDERER_VIEW_MODE_DEFAULT:
                kdebug("Renderer mode set to default.");
                data->render_mode = RENDERER_VIEW_MODE_DEFAULT;
                break;
            case RENDERER_VIEW_MODE_LIGHTING:
                kdebug("Renderer mode set to lighting.");
                data->render_mode = RENDERER_VIEW_MODE_LIGHTING;
                break;
            case RENDERER_VIEW_MODE_NORMALS:
                kdebug("Renderer mode set to normals.");
                data->render_mode = RENDERER_VIEW_MODE_NORMALS;
                break;
        }
        return true;
    }
    return false;
}

bool render_view_world_on_create(render_view* self)
{
    if(!self)
    {
        kerror("Function '%s' requires a valid pointer to a view.", __FUNCTION__);
        return false;
    }

    self->internal_data = kallocate_tc(render_view_world_internal_data, 1, MEMORY_TAG_RENDERER);
    render_view_world_internal_data* data = self->internal_data;

    data->shader_id = shader_system_get_id(self->custom_shader_name ? self->custom_shader_name : BUILTIN_SHADER_NAME_WORLD);
    data->render_mode = RENDERER_VIEW_MODE_DEFAULT;
    // TODO: Установка из конфигурации.
    data->fov = deg_to_rad(60.0f);
    data->near_clip = 0.1f;
    data->far_clip = 1000.0f;
    data->projection_matrix = mat4_perspective(data->fov, 1280.0f / 720.0f, data->near_clip, data->far_clip);
    data->world_camera = camera_system_get_default();
    // TODO: Получение из сцены.
    data->ambient_color = (vec4){{0.25f, 0.25f, 0.25f, 1.0f}};

    event_register(EVENT_CODE_SET_RENDER_MODE, self->internal_data, render_view_world_on_event);

    return true;
}

void render_view_world_on_destroy(render_view* self)
{
    if(!view_state_valid(self, __FUNCTION__)) return;
    event_unregister(EVENT_CODE_SET_RENDER_MODE, self->internal_data, render_view_world_on_event);
    kfree(self->internal_data, MEMORY_TAG_RENDERER);
    self->internal_data = null;
}

void render_view_world_on_resize(render_view* self, u32 width, u32 height)
{
    if(!view_state_valid(self, __FUNCTION__)) return;

    if(width != self->width || height != self->height)
    {
        render_view_world_internal_data* data = self->internal_data;

        self->width = width;
        self->height = height;
        f32 aspect = (f32)width / height;
        data->projection_matrix = mat4_perspective(data->fov, aspect, data->near_clip, data->far_clip);

        for(u32 i = 0; i < self->renderpass_count; ++i)
        {
            self->passes[i]->render_area.x = self->passes[i]->render_area.y = 0;
            self->passes[i]->render_area.width = (f32)width;
            self->passes[i]->render_area.height = (f32)height;
        }
    }
}

bool render_view_world_on_build_packet(const render_view* self, void* data, render_view_packet* out_packet)
{
    if(!view_state_valid(self, __FUNCTION__) || !data || !out_packet)
    {
        kerror("Function '%s' requires a valid pointer to a packet and a data.", __FUNCTION__);
        return false;
    }

    mesh_packet_data* mesh_data = data;
    render_view_world_internal_data* internal_data = self->internal_data;

    out_packet->geometries = darray_create(geometry_render_data);
    out_packet->view = (render_view*)self;

    out_packet->projection_matrix = internal_data->projection_matrix;
    out_packet->view_matrix = camera_view_get(internal_data->world_camera);
    out_packet->view_position = camera_position_get(internal_data->world_camera);
    out_packet->ambient_color = internal_data->ambient_color;

    geometry_distance* geometry_distances = darray_create(geometry_distance);

    for(u32 i = 0; i < mesh_data->mesh_count; ++i)
    {
        mesh* m = mesh_data->meshes[i];
        mat4 model = transform_get_world(&m->transform);

        for(u32 j = 0; j < m->geometry_count; ++j)
        {
            geometry_render_data render_data;
            render_data.geometry = m->geometries[j];
            render_data.model = model;

            // Добавление сеток без прозрачности.
            if((m->geometries[j]->material->diffuse_map.texture->flags & TEXTURE_FLAG_HAS_TRANSPARENCY) == 0)
            {
                darray_push(out_packet->geometries, render_data);
                out_packet->geometry_count++;
            }
            // Добавление сеток с прозрачностью.
            else
            {
                vec3 center = vec3_transform(render_data.geometry->center, 1.0f, model);
                f32 distance = vec3_distance(center, internal_data->world_camera->position);

                geometry_distance gdist;
                gdist.distance = kabs(distance);
                gdist.g = render_data;

                darray_push(geometry_distances, gdist);
            }
        }
    }

    // Сортировка дистанций.
    u32 geometry_count = darray_length(geometry_distances);
    quick_sort(geometry_distances, 0, geometry_count - 1, false);

    for(u32 i = 0; i < geometry_count; ++i)
    {
        darray_push(out_packet->geometries, geometry_distances[i].g);
        out_packet->geometry_count++;
    }

    darray_destroy(geometry_distances);

    return true;
}

void render_view_world_on_destroy_packet(const render_view* self, render_view_packet* packet)
{
    darray_destroy(packet->geometries);
    packet->geometries = null;
}

bool render_view_world_on_render(const render_view* self, const render_view_packet* packet, u64 frame_number, u64 render_target_index)
{
    if(!view_state_valid(self, __FUNCTION__)) return false;

    render_view_world_internal_data* data = self->internal_data;
    u32 shader_id = data->shader_id;

    for(u32 p = 0; p < self->renderpass_count; ++p)
    {
        renderpass* pass = self->passes[p];

        if(!renderer_renderpass_begin(pass, &pass->targets[render_target_index]))
        {
            kerror("Function '%s' pass index %u failed to start.", __FUNCTION__, p);
            return false;
        }

        if(!shader_system_use_by_id(shader_id))
        {
            kerror("Function '%s': Failed to use WORLD shader.", __FUNCTION__);
            return false;
        }

        if(!material_system_apply_global(
            shader_id, frame_number, &packet->projection_matrix, &packet->view_matrix, &packet->view_position,
            &packet->ambient_color, data->render_mode
        ))
        {
            kerror("Function '%s': Failed to use apply globals WORLD shader.", __FUNCTION__);
            return false;
        }

        u32 count = packet->geometry_count;
        for(u32 i = 0; i < count; ++i)
        {
            material* m = null;
            if(packet->geometries[i].geometry->material)
            {
                m = packet->geometries[i].geometry->material;
            }
            else
            {
                m = material_system_get_default();
            }

            // Применение материала.
            bool needs_update = m->render_frame_number != frame_number;
            if(!material_system_apply_instance(m, needs_update))
            {
                kwarng("Failed to apply WORLD instance '%s'. Skipping draw.", m->name);
                continue;
            }
            else
            {
                m->render_frame_number = frame_number;
            }

            // Применение локальной позиции объекта.
            material_system_apply_local(m, &packet->geometries[i].model);

            // Нарисовать!
            renderer_geometry_draw(&packet->geometries[i]);
        }

        if(!renderer_renderpass_end(pass))
        {
            kerror("Function '%s' pass index %u failed to end.", __FUNCTION__, p);
            return false;
        }
    }

    return true;
}
