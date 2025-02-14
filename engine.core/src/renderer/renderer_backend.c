// Cобственные подключения.
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "renderer/vulkan/vulkan_backend.h"
#include "memory/memory.h"

bool renderer_backend_create(renderer_backend_type type, renderer_backend* out_renderer_backend)
{
    if(type == RENDERER_BACKEND_TYPE_VULKAN)
    {
        out_renderer_backend->initialize          = vulkan_renderer_backend_initialize;
        out_renderer_backend->shutdown            = vulkan_renderer_backend_shutdown;
        out_renderer_backend->begin_frame         = vulkan_renderer_backend_begin_frame;
        out_renderer_backend->update_global_world_state = vulkan_renderer_update_global_world_state;
        out_renderer_backend->update_global_ui_state    = vulkan_renderer_update_global_ui_state;
        out_renderer_backend->end_frame           = vulkan_renderer_backend_end_frame;
        out_renderer_backend->begin_renderpass    = vulkan_renderer_backend_begin_renderpass;
        out_renderer_backend->end_renderpass      = vulkan_renderer_backend_end_renderpass;
        out_renderer_backend->resized             = vulkan_renderer_backend_on_resized;
        out_renderer_backend->draw_geometry       = vulkan_renderer_backend_draw_geometry;
        out_renderer_backend->create_texture      = vulkan_renderer_backend_create_texture;
        out_renderer_backend->destroy_texture     = vulkan_renderer_backend_destroy_texture;
        out_renderer_backend->create_material     = vulkan_renderer_backend_create_material;
        out_renderer_backend->destroy_material    = vulkan_renderer_backend_destroy_material;
        out_renderer_backend->create_geometry     = vulkan_renderer_backend_create_geometry;
        out_renderer_backend->destroy_geometry    = vulkan_renderer_backend_destroy_geometry;
        return true;
    }
    return false;
}

void renderer_backend_destroy(renderer_backend* backend)
{
    kzero_tc(backend, renderer_backend, 1);
}
