// Собственные включения.
#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние включения.
#include "logger.h"

void vulkan_image_create(
    vulkan_context* context, VkImageType imagetype, u32 width, u32 height, VkFormat imageformat,
    VkImageTiling imagetiling, VkImageUsageFlags imageusage, VkMemoryPropertyFlags memoryflags, bool createview,
    VkImageAspectFlags viewaspectflags, vulkan_image* out_image
)
{
    // Копирование параметров.
    out_image->width = width;
    out_image->height = height;

    VkImageCreateInfo imageinfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageinfo.imageType = VK_IMAGE_TYPE_2D;
    imageinfo.extent.width = width;
    imageinfo.extent.height = height;
    imageinfo.extent.depth = 1; // TODO: Сделать поддержку конфигурации глубины.
    imageinfo.mipLevels = 1;    // TODO: Сделать поддержку конфигурации уровня дитализации изображения (индекс текстуры!).
    imageinfo.arrayLayers = 1;  // TODO: Сделать поддержку конфигурации количества слоев изображения.
    imageinfo.format = imageformat;
    imageinfo.tiling = imagetiling;
    imageinfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageinfo.usage = imageusage;
    imageinfo.samples = VK_SAMPLE_COUNT_1_BIT;            // TODO: Сделать поддержку конфигурации количества образцов.
    imageinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;    // TODO: Сделать поддержку конфигурации режима совместного использования.

    /*
        NOTE: В спецификации Vulkan указано: mipLevels должен быть меньше или равен количеству уровней в
        полной цепочке mipmap на основе extend.width, extend.height и extend.depth.
    */
    VkResult result = vkCreateImage(context->device.logical, &imageinfo, context->allocator, &out_image->handle);

    // Запрос требований памяти.
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(context->device.logical, out_image->handle, &memory_requirements);

    i32 memory_type = context->find_memory_index(memory_requirements.memoryTypeBits, memoryflags);
    if(memory_type == -1)
    {
        kfatal("Required memory type not found. Image not valid.");
    }

    // Выделение памяти.
    VkMemoryAllocateInfo memoryinfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memoryinfo.allocationSize = memory_requirements.size;
    memoryinfo.memoryTypeIndex = memory_type;

    result = vkAllocateMemory(context->device.logical, &memoryinfo, context->allocator, &out_image->memory);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to allocate memory for vulkan_image with result: %s", vulkan_result_get_string(result, true));
    }

    // Связывание памяти с изображеинем.
    // TODO: Сделать поддержку конфигурации сдвига памяти.
    result = vkBindImageMemory(context->device.logical, out_image->handle, out_image->memory, 0);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to bind memory to image for vulkan_image with result: %s", vulkan_result_get_string(result, true));
    }

    // Создание представления изображения.
    if(createview)
    {
        out_image->view = null;
        vulkan_image_view_create(context, imageformat, out_image, viewaspectflags);
    }
}

void vulkan_image_view_create(
    vulkan_context* context, VkFormat format, vulkan_image* image, VkImageAspectFlags aspectflags
)
{
    VkImageViewCreateInfo viewinfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewinfo.image = image->handle;
    viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;    // TODO: Сделать поддержку конфигурирования.
    viewinfo.format = format;
    viewinfo.subresourceRange.aspectMask = aspectflags;

    // TODO: Сделать поддержку конфигурирования.
    viewinfo.subresourceRange.baseMipLevel = 0;
    viewinfo.subresourceRange.levelCount = 1;
    viewinfo.subresourceRange.baseArrayLayer = 0;
    viewinfo.subresourceRange.layerCount = 1;

    VkResult result = vkCreateImageView(context->device.logical, &viewinfo, context->allocator, &image->view);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to create image view for vulkan_image with result: %s", vulkan_result_get_string(result, true));
    }
}

void vulkan_image_destroy(vulkan_context* context, vulkan_image* image)
{
    if(image->view)
    {
        vkDestroyImageView(context->device.logical, image->view, context->allocator);
        image->view = null;
    }

    if(image->memory)
    {
        vkFreeMemory(context->device.logical, image->memory, context->allocator);
        image->memory = null;
    }

    if(image->handle)
    {
        vkDestroyImage(context->device.logical, image->handle, context->allocator);
        image->handle = null;
    }
}
