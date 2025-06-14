// Cобственные подключения.
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "renderer/vulkan/vulkan_backend.h"
#include "memory/memory.h"

bool renderer_backend_create(renderer_backend_type type, renderer_backend* out_renderer_backend)
{
    if(type == RENDERER_BACKEND_TYPE_VULKAN)
    {
        out_renderer_backend->initialize                         = vulkan_backend_initialize;
        out_renderer_backend->shutdown                           = vulkan_backend_shutdown;
        out_renderer_backend->resized                            = vulkan_backend_on_resized;
        out_renderer_backend->frame_begin                        = vulkan_backend_frame_begin;
        out_renderer_backend->frame_end                          = vulkan_backend_frame_end;
        out_renderer_backend->is_multithreaded                   = vulkan_backend_is_multithreaded;

        out_renderer_backend->renderpass_create                  = vulkan_renderpass_create;
        out_renderer_backend->renderpass_destroy                 = vulkan_renderpass_destroy;
        out_renderer_backend->renderpass_begin                   = vulkan_renderpass_begin;
        out_renderer_backend->renderpass_end                     = vulkan_renderpass_end;
        out_renderer_backend->renderpass_get                     = vulkan_renderpass_get;

        out_renderer_backend->texture_create                     = vulkan_texture_create;
        out_renderer_backend->texture_create_writable            = vulkan_texture_create_writable;
        out_renderer_backend->texture_destroy                    = vulkan_texture_destroy;
        out_renderer_backend->texture_resize                     = vulkan_texture_resize;
        out_renderer_backend->texture_write_data                 = vulkan_texture_write_data;
        out_renderer_backend->texture_map_acquire_resources      = vulkan_texture_map_acquire_resources;
        out_renderer_backend->texture_map_release_resources      = vulkan_texture_map_release_resources;

        out_renderer_backend->geometry_create                    = vulkan_geometry_create;
        out_renderer_backend->geometry_destroy                   = vulkan_geometry_destroy;
        out_renderer_backend->geometry_draw                      = vulkan_geometry_draw;

        out_renderer_backend->shader_create                      = vulkan_shader_create;
        out_renderer_backend->shader_destroy                     = vulkan_shader_destroy;
        out_renderer_backend->shader_initialize                  = vulkan_shader_initialize;
        out_renderer_backend->shader_use                         = vulkan_shader_use;
        out_renderer_backend->shader_bind_globals                = vulkan_shader_bind_globals;
        out_renderer_backend->shader_bind_instance               = vulkan_shader_bind_instance;
        out_renderer_backend->shader_apply_globals               = vulkan_shader_apply_globals;
        out_renderer_backend->shader_apply_instance              = vulkan_shader_apply_instance;
        out_renderer_backend->shader_acquire_instance_resources  = vulkan_shader_acquire_instance_resources;
        out_renderer_backend->shader_release_instance_resources  = vulkan_shader_release_instance_resources;
        out_renderer_backend->shader_set_uniform                 = vulkan_shader_set_uniform;

        out_renderer_backend->render_target_create               = vulkan_render_target_create;
        out_renderer_backend->render_target_destroy              = vulkan_render_target_destroy;
        out_renderer_backend->window_attachment_get              = vulkan_window_attachment_get;
        out_renderer_backend->depth_attachment_get               = vulkan_depth_attachment_get;
        out_renderer_backend->window_attachment_index_get        = vulkan_window_attachment_index_get;

        out_renderer_backend->renderbuffer_create                = vulkan_buffer_create;
        out_renderer_backend->renderbuffer_destroy               = vulkan_buffer_destroy;
        out_renderer_backend->renderbuffer_resize                = vulkan_buffer_resize;
        out_renderer_backend->renderbuffer_bind                  = vulkan_buffer_bind;
        out_renderer_backend->renderbuffer_unbind                = vulkan_buffer_unbind;
        out_renderer_backend->renderbuffer_map_memory            = vulkan_buffer_map_memory;
        out_renderer_backend->renderbuffer_unmap_memory          = vulkan_buffer_unmap_memory;
        out_renderer_backend->renderbuffer_flush                 = vulkan_buffer_flush;
        out_renderer_backend->renderbuffer_read                  = vulkan_buffer_read;
        out_renderer_backend->renderbuffer_load_range            = vulkan_buffer_load_range;
        out_renderer_backend->renderbuffer_copy_range            = vulkan_buffer_copy_range;
        out_renderer_backend->renderbuffer_draw                  = vulkan_buffer_draw;

        return true;
    }
    return false;
}

void renderer_backend_destroy(renderer_backend* backend)
{
    kzero_tc(backend, renderer_backend, 1);
}
