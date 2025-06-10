// Собственные подключения.
#include "renderer/views/render_view_skybox.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "math/kmath.h"
#include "systems/shader_system.h"
#include "systems/camera_system.h"
#include "renderer/renderer_frontend.h"

typedef struct render_view_skybox_internal_data {
    u32 shader_id;
    f32 fov;
    f32 near_clip;
    f32 far_clip;
    mat4 projection_matrix;
    camera* world_camera;
    // uniform переменные.
    u16 projection_location;
    u16 view_location;
    u16 cube_map_location;
} render_view_skybox_internal_data;

static bool view_state_valid(const render_view* self, const char* func_name)
{
    if(!self || !self->internal_data)
    {
        if(func_name) kerror("Function '%s' requires a valid pointer to a view and its internal data.", func_name);
        return false;
    }
    return true;
}

bool render_view_skybox_on_create(render_view* self)
{
    if(!self)
    {
        kerror("Function '%s' requires a valid pointer to a view.", __FUNCTION__);
        return false;
    }

    self->internal_data = kallocate_tc(render_view_skybox_internal_data, 1, MEMORY_TAG_RENDERER);
    render_view_skybox_internal_data* data = self->internal_data;

    shader* s = shader_system_get(self->custom_shader_name ? self->custom_shader_name : BUILTIN_SHADER_NAME_SKYBOX);

    data->shader_id = s->id;
    data->projection_location = shader_system_uniform_index(s, "projection");
    data->view_location = shader_system_uniform_index(s, "view");
    data->cube_map_location = shader_system_uniform_index(s, "cube_texture");

    // TODO: Установка из конфигурации.
    data->fov = deg_to_rad(60.0f);
    data->near_clip = 0.1f;
    data->far_clip = 1000.0f;
    data->projection_matrix = mat4_perspective(data->fov, 1280.0f / 720.0f, data->near_clip, data->far_clip);
    data->world_camera = camera_system_get_default();

    return true;
}

void render_view_skybox_on_destroy(render_view* self)
{
    if(!view_state_valid(self, __FUNCTION__)) return;
    kfree(self->internal_data, MEMORY_TAG_RENDERER);
    self->internal_data = null;
}

void render_view_skybox_on_resize(render_view* self, u32 width, u32 height)
{
    if(!view_state_valid(self, __FUNCTION__)) return;

    if(width != self->width || height != self->height)
    {
        render_view_skybox_internal_data* data = self->internal_data;

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

bool render_view_skybox_on_build_packet(const render_view* self, void* data, render_view_packet* out_packet)
{
    if(!view_state_valid(self, __FUNCTION__) || !data || !out_packet)
    {
        kerror("Function '%s' requires a valid pointer to a packet and a data.", __FUNCTION__);
        return false;
    }

    skybox_packet_data* skybox_data = data;
    render_view_skybox_internal_data* internal_data = self->internal_data;

    out_packet->view = (render_view*)self;
    out_packet->projection_matrix = internal_data->projection_matrix;
    out_packet->view_matrix = camera_view_get(internal_data->world_camera);
    out_packet->view_position = camera_position_get(internal_data->world_camera);
    out_packet->extended_data = skybox_data;

    return true;
}

void render_view_skybox_on_destroy_packet(const render_view* self, render_view_packet* packet)
{
    // NOTE: Очищать нечего!
}

bool render_view_skybox_on_render(const render_view* self, const render_view_packet* packet, u64 frame_number, u64 render_target_index)
{
    if(!view_state_valid(self, __FUNCTION__)) return false;

    render_view_skybox_internal_data* data = self->internal_data;
    u32 shader_id = data->shader_id;

    skybox_packet_data* skybox_data = packet->extended_data;

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

        mat4 view_matrix = camera_view_get(data->world_camera);
        view_matrix.data[12] = 0.0f;
        view_matrix.data[13] = 0.0f;
        view_matrix.data[14] = 0.0f;

        // TODO: Изменить!
        renderer_shader_bind_globals(shader_system_get_by_id(shader_id));
        if(!shader_system_uniform_set_by_index(data->projection_location, &packet->projection_matrix))
        {
            kerror("Function '%s': Failed to apply skybox projection uniform.", __FUNCTION__);
            return false;
        }

        if(!shader_system_uniform_set_by_index(data->view_location, &view_matrix))
        {
            kerror("Function '%s': Failed to apply skybox view uniform.", __FUNCTION__);
            return false;
        }
        shader_system_apply_global();

        shader_system_bind_instance(shader_id);
        if(!shader_system_uniform_set_by_index(data->cube_map_location, &skybox_data->sb->cubemap))
        {
            kerror("Function '%s': Failed to apply skybox cube map uniform.", __FUNCTION__);
            return false;
        }

        bool needs_update = skybox_data->sb->render_frame_number != frame_number;
        shader_system_apply_instance(needs_update);

        skybox_data->sb->render_frame_number = frame_number;

        geometry_render_data render_data = {};
        render_data.geometry = skybox_data->sb->g;
        renderer_geometry_draw(&render_data);

        if(!renderer_renderpass_end(pass))
        {
            kerror("Function '%s' pass index %u failed to end.", __FUNCTION__, p);
            return false;
        }
    }

    return true;
}

