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
    @brief Перенесите предоставленное изображение из old_layout в new_layout.
*/
void vulkan_image_transition_layout(
    vulkan_context* context, vulkan_command_buffer* command_buffer, vulkan_image* image, VkFormat* format, 
    VkImageLayout old_layout, VkImageLayout new_layout
);

/*
    @brief Копирует данные из буфера в предоставленное изображение.
*/
void vulkan_image_copy_from_buffer(
    vulkan_context* context, vulkan_image* image, VkBuffer buffer, vulkan_command_buffer* command_buffer
);

/*
*/
void vulkan_image_destroy(vulkan_context* context, vulkan_image* image);
