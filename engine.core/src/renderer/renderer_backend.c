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
        out_renderer_backend->begin_frame                        = vulkan_renderer_backend_begin_frame;
        out_renderer_backend->end_frame                          = vulkan_renderer_backend_end_frame;
        out_renderer_backend->resized                            = vulkan_renderer_backend_on_resized;
        out_renderer_backend->begin_renderpass                   = vulkan_renderer_begin_renderpass;
        out_renderer_backend->end_renderpass                     = vulkan_renderer_end_renderpass;
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
        return true;
    }
    return false;
}

void renderer_backend_destroy(renderer_backend* backend)
{
    kzero_tc(backend, renderer_backend, 1);
}
