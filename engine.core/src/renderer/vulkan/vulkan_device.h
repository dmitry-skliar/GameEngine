#pragma once

#include <defines.h>
#include <renderer/vulkan/vulkan_types.h>

/*
*/
VkResult vulkan_device_create(vulkan_context* context);

/*
*/
void vulkan_device_destroy(vulkan_context* context);

/*
*/
void vulkan_device_query_swapchain_support(VkPhysicalDevice physical, VkSurfaceKHR surface, vulkan_device_swapchain_support* out_swapchain_support);

/*
*/
void vulkan_device_destroy_swapchian_support(vulkan_device_swapchain_support* swapchain_support);

/*
*/
bool vulkan_device_detect_depth_format(vulkan_device* device);
