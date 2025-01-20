#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <vulkan/vulkan.h>

typedef struct vulkan_context {
    VkInstance               instance;
    VkAllocationCallbacks*   allocator;
#if KDEBUG_FLAG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} vulkan_context;
