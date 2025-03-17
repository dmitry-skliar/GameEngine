// Cобственные подключения.
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "renderer/vulkan/vulkan_backend.h"
#include "memory/memory.h"

bool renderer_backend_create(renderer_backend_type type, renderer_backend* out_renderer_backend)
{
    if(type == RENDERER_BACKEND_TYPE_VULKAN)
    {
        out_renderer_backend->initialize                         = vulkan_renderer_backend_initialize;
        out_renderer_backend->shutdown                           = vulkan_renderer_backend_shutdown;
        out_renderer_backend->resized                            = vulkan_renderer_backend_on_resized;
        out_renderer_backend->frame_begin                        = vulkan_renderer_backend_frame_begin;
        out_renderer_backend->frame_end                          = vulkan_renderer_backend_frame_end;
        out_renderer_backend->renderpass_create                  = vulkan_renderer_renderpass_create;
        out_renderer_backend->renderpass_destroy                 = vulkan_renderer_renderpass_destroy;
        out_renderer_backend->renderpass_begin                   = vulkan_renderer_renderpass_begin;
        out_renderer_backend->renderpass_end                     = vulkan_renderer_renderpass_end;
        out_renderer_backend->renderpass_get                     = vulkan_renderer_renderpass_get;
        out_renderer_backend->texture_create                     = vulkan_renderer_texture_create;
        out_renderer_backend->texture_create_writable            = vulkan_renderer_texture_create_writable;
        out_renderer_backend->texture_destroy                    = vulkan_renderer_texture_destroy;
        out_renderer_backend->texture_resize                     = vulkan_renderer_texture_resize;
        out_renderer_backend->texture_write_data                 = vulkan_renderer_texture_write_data;
        out_renderer_backend->texture_map_acquire_resources      = vulkan_renderer_texture_map_acquire_resources;
        out_renderer_backend->texture_map_release_resources      = vulkan_renderer_texture_map_release_resources;
        out_renderer_backend->geometry_create                    = vulkan_renderer_geometry_create;
        out_renderer_backend->geometry_destroy                   = vulkan_renderer_geometry_destroy;
        out_renderer_backend->geometry_draw                      = vulkan_renderer_geometry_draw;
        out_renderer_backend->shader_create                      = vulkan_renderer_shader_create;
        out_renderer_backend->shader_destroy                     = vulkan_renderer_shader_destroy;
        out_renderer_backend->shader_initialize                  = vulkan_renderer_shader_initialize;
        out_renderer_backend->shader_use                         = vulkan_renderer_shader_use;
        out_renderer_backend->shader_bind_globals                = vulkan_renderer_shader_bind_globals;
        out_renderer_backend->shader_bind_instance               = vulkan_renderer_shader_bind_instance;
        out_renderer_backend->shader_apply_globals               = vulkan_renderer_shader_apply_globals;
        out_renderer_backend->shader_apply_instance              = vulkan_renderer_shader_apply_instance;
        out_renderer_backend->shader_acquire_instance_resources  = vulkan_renderer_shader_acquire_instance_resources;
        out_renderer_backend->shader_release_instance_resources  = vulkan_renderer_shader_release_instance_resources;
        out_renderer_backend->shader_set_uniform                 = vulkan_renderer_shader_set_uniform;
        out_renderer_backend->render_target_create               = vulkan_renderer_render_target_create;
        out_renderer_backend->render_target_destroy              = vulkan_renderer_render_target_destroy;
        out_renderer_backend->window_attachment_get              = vulkan_renderer_window_attachment_get;
        out_renderer_backend->depth_attachment_get               = vulkan_renderer_depth_attachment_get;
        out_renderer_backend->window_attachment_index_get        = vulkan_renderer_window_attachment_index_get;
        return true;
    }
    return false;
}

void renderer_backend_destroy(renderer_backend* backend)
{
    kzero_tc(backend, renderer_backend, 1);
}
