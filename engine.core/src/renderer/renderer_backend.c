// Cобственные подключения.
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "renderer/vulkan/vulkan_backend.h"

bool renderer_backend_create(renderer_backend_type type, renderer_backend* out_renderer_backend)
{
    if(type == RENDERER_BACKEND_TYPE_VULKAN)
    {
        out_renderer_backend->initialize  = vulkan_renderer_backend_initialize;
        out_renderer_backend->shutdown    = vulkan_renderer_backend_shutdown;
        out_renderer_backend->begin_frame = vulkan_renderer_backend_begin_frame;
        out_renderer_backend->end_frame   = vulkan_renderer_backend_end_frame;
        out_renderer_backend->resized     = vulkan_renderer_backend_on_resized;
        return true;
    }

    return false;
}

void renderer_backend_destroy(renderer_backend* backend)
{
    backend->initialize  = null;
    backend->shutdown    = null;
    backend->begin_frame = null;
    backend->end_frame   = null;
    backend->resized     = null;
}

