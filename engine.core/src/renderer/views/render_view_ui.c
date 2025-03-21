// Собственные подключения.
#include "renderer/views/render_view_ui.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "math/kmath.h"
#include "math/transform.h"
#include "containers/darray.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"
#include "renderer/renderer_frontend.h"

typedef struct render_view_ui_internal_data {
    u32 shader_id;
    f32 near_clip;
    f32 far_clip;
    mat4 projection_matrix;
    mat4 view_matrix;
    // u32 render_mode;
} render_view_ui_internal_data;

static bool view_state_valid(render_view* self, const char* func_name)
{
    if(!self || !self->internal_data)
    {
        if(func_name) kerror("Function '%s' requires a valid pointer to a view and its internal data.", func_name);
        return false;
    }
    return true;
}

bool render_view_ui_on_create(render_view* self)
{
    if(!self)
    {
        kerror("Function '%s' requires a valid pointer to a view.", __FUNCTION__);
        return false;
    }

    self->internal_data = kallocate_tc(render_view_ui_internal_data, 1, MEMORY_TAG_RENDERER);
    render_view_ui_internal_data* data = self->internal_data;

    data->shader_id = shader_system_get_id(self->custom_shader_name ? self->custom_shader_name : BUILTIN_SHADER_NAME_UI);
    // TODO: Установка из конфигурации.
    data->near_clip = -100.0f;
    data->far_clip = 100.0f;

    data->projection_matrix = mat4_orthographic(0.0f, 1280.0f, 720.0f, 0.0f, data->near_clip, data->far_clip);
    data->view_matrix = mat4_inverse(mat4_identity());

    return true;
}

void render_view_ui_on_destroy(render_view* self)
{
    if(!view_state_valid(self, __FUNCTION__)) return;

    kfree_tc(self->internal_data, render_view_ui_internal_data, 1, MEMORY_TAG_RENDERER);
    self->internal_data = null;
}

void render_view_ui_on_resize(render_view* self, u32 width, u32 height)
{
    if(!view_state_valid(self, __FUNCTION__)) return;

    if(width != self->width || height != self->height)
    {
        render_view_ui_internal_data* data = self->internal_data;

        self->width = width;
        self->height = height;
        data->projection_matrix = mat4_orthographic(0.0f, (f32)width, (f32)height, 0.0f, data->near_clip, data->far_clip);

        for(u32 i = 0; i < self->renderpass_count; ++i)
        {
            self->passes[i]->render_area.x = self->passes[i]->render_area.y = 0;
            self->passes[i]->render_area.width = (f32)width;
            self->passes[i]->render_area.height = (f32)height;
        }
    }
}

bool render_view_ui_on_build_packet(render_view* self, void* data, render_view_packet* out_packet)
{
    if(!view_state_valid(self, __FUNCTION__) || !data || !out_packet)
    {
        kerror("Function '%s' requires a valid pointer to a packet and a data.", __FUNCTION__);
        return false;
    }

    mesh_packet_data* mesh_data = data;
    render_view_ui_internal_data* internal_data = self->internal_data;

    out_packet->geometries = darray_create(geometry_render_data);
    out_packet->view = self;

    out_packet->projection_matrix = internal_data->projection_matrix;
    out_packet->view_matrix = internal_data->view_matrix;

    for(u32 i = 0; i < mesh_data->mesh_count; ++i)
    {
        mesh* m = &mesh_data->meshes[i];

        for(u32 j = 0; j < m->geometry_count; ++j)
        {
            geometry_render_data render_data;
            render_data.geometry = m->geometries[j];
            render_data.model = transform_get_world(&m->transform);

            darray_push(out_packet->geometries, render_data);
            out_packet->geometry_count++;
        }
    }

    return true;
}

bool render_view_ui_on_render(render_view* self, const render_view_packet* packet, u64 frame_number, u64 render_target_index)
{
    if(!view_state_valid(self, __FUNCTION__)) return false;

    render_view_ui_internal_data* data = self->internal_data;
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
            kerror("Function '%s': Failed to use UI shader.", __FUNCTION__);
            return false;
        }

        if(!material_system_apply_global(shader_id, frame_number, &packet->projection_matrix, &packet->view_matrix, null, null, 0))
        {
            kerror("Function '%s': Failed to use apply globals UI shader.", __FUNCTION__);
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
                kwarng("Failed to apply UI instance '%s'. Skipping draw.", m->name);
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
