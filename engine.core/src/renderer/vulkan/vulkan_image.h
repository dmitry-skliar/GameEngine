#pragma once

#include <defines.h>
#include <renderer/vulkan/vulkan_types.h>

/*
*/
void vulkan_image_create(
    vulkan_context* context, VkImageType imagetype, u32 width, u32 height, VkFormat imageformat,
    VkImageTiling imagetiling, VkImageUsageFlags imageusage, VkMemoryPropertyFlags memoryflags, bool createview,
    VkImageAspectFlags viewaspectflags, vulkan_image* out_image
);

/*
*/
void vulkan_image_view_create(
    vulkan_context* context, VkFormat format, vulkan_image* image, VkImageAspectFlags aspectflags
);

/*
*/
void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);
