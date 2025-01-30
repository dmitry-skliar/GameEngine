#pragma once

#include <defines.h>
#include <renderer/vulkan/vulkan_types.h>

/*
*/
void vulkan_fence_create(vulkan_context* context, bool create_signaled, vulkan_fence* out_fence);

/*
*/
void vulkan_fence_destroy(vulkan_context* context, vulkan_fence* fence);

/*
*/
bool vulkan_fence_wait(vulkan_context* context, vulkan_fence* fence, u64 timeout_ns);

/*
*/
void vulkan_fence_reset(vulkan_context* context, vulkan_fence* fence);
