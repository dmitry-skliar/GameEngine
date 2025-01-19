#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <vulkan/vulkan.h>

typedef struct vulkan_context {
    VkInstance             instance;
    VkAllocationCallbacks* allocator;
} vulkan_context;
