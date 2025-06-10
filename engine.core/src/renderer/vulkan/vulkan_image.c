// Собственные подключения.
#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"

void vulkan_image_create(
    vulkan_context* context, texture_type type, u32 width, u32 height, VkFormat imageformat, VkImageTiling imagetiling,
    VkImageUsageFlags imageusage, VkMemoryPropertyFlags memory_flags, bool createview, VkImageAspectFlags viewaspectflags,
    vulkan_image* out_image
)
{
    // Копирование параметров.
    out_image->width = width;
    out_image->height = height;
    out_image->memory_property_flags = memory_flags;

    VkImageCreateInfo imageinfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

    switch(type)
    {
        default:
        case TEXTURE_TYPE_2D:
        case TEXTURE_TYPE_CUBE:
            imageinfo.imageType = VK_IMAGE_TYPE_2D;
    }

    imageinfo.extent.width = width;
    imageinfo.extent.height = height;
    imageinfo.extent.depth = 1;                                 // TODO: Сделать поддержку конфигурации глубины.
    imageinfo.mipLevels = 1;                                    // TODO: Сделать поддержку конфигурации уровня дитализации изображения (индекс текстуры!).
    imageinfo.arrayLayers = type == TEXTURE_TYPE_CUBE ? 6 : 1;  // TODO: Сделать поддержку конфигурации количества слоев изображения.
    imageinfo.format = imageformat;
    imageinfo.tiling = imagetiling;
    imageinfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageinfo.usage = imageusage;
    imageinfo.samples = VK_SAMPLE_COUNT_1_BIT;            // TODO: Сделать поддержку конфигурации количества образцов.
    imageinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;    // TODO: Сделать поддержку конфигурации режима совместного использования.

    if(type == TEXTURE_TYPE_CUBE)
    {
        imageinfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; 
    }

    /*
        NOTE: В спецификации Vulkan указано: mipLevels должен быть меньше или равен количеству уровней в
        полной цепочке mipmap на основе extend.width, extend.height и extend.depth.
    */
    VkResult result = vkCreateImage(context->device.logical, &imageinfo, context->allocator, &out_image->handle);

    // Запрос требований памяти.
    vkGetImageMemoryRequirements(context->device.logical, out_image->handle, &out_image->memory_requirements);

    i32 memory_type = context->find_memory_index(out_image->memory_requirements.memoryTypeBits, out_image->memory_property_flags);
    if(memory_type == INVALID_ID)
    {
        kfatal("Required memory type not found. Image not valid.");
    }

    // Выделение памяти.
    VkMemoryAllocateInfo memoryinfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memoryinfo.allocationSize = out_image->memory_requirements.size;
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

    // Указание типа используемой памяти (True - GPU, False - Host).
    out_image->use_device_local = (out_image->memory_property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    // Указание системе о выделении типа и размера памяти в Vulkan.
    kallocate_report(out_image->memory_requirements.size, out_image->use_device_local ? MEMORY_TAG_GPU_LOCAL : MEMORY_TAG_VULKAN);

    // Создание представления изображения.
    if(createview)
    {
        out_image->view = null;
        vulkan_image_view_create(context, type, imageformat, out_image, viewaspectflags);
    }
}

void vulkan_image_view_create(
    vulkan_context* context, texture_type type, VkFormat format, vulkan_image* image, VkImageAspectFlags aspectflags
)
{
    VkImageViewCreateInfo viewinfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewinfo.image = image->handle;

    switch(type)
    {
        case TEXTURE_TYPE_CUBE:
            viewinfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;

        default:
        case TEXTURE_TYPE_2D:
            viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    }

    viewinfo.format = format;
    viewinfo.subresourceRange.aspectMask = aspectflags;

    // TODO: Сделать поддержку конфигурирования.
    viewinfo.subresourceRange.baseMipLevel = 0;
    viewinfo.subresourceRange.levelCount = 1;
    viewinfo.subresourceRange.baseArrayLayer = 0;
    viewinfo.subresourceRange.layerCount = type == TEXTURE_TYPE_CUBE ? 6 : 1;

    VkResult result = vkCreateImageView(context->device.logical, &viewinfo, context->allocator, &image->view);
    if(!vulkan_result_is_success(result))
    {
        kfatal("Failed to create image view for vulkan_image with result: %s", vulkan_result_get_string(result, true));
    }
}

void vulkan_image_transition_layout(
    vulkan_context* context, texture_type type, vulkan_command_buffer* command_buffer, vulkan_image* image,
    VkFormat* format, VkImageLayout old_layout, VkImageLayout new_layout
)
{
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = context->device.graphics_queue.index;
    barrier.dstQueueFamilyIndex = context->device.graphics_queue.index;
    barrier.image = image->handle;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = type == TEXTURE_TYPE_CUBE ? 6 : 1;

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    // Не обращайте внимания на старую компоновку - переходите к оптимальной компоновке (для базовой реализации).
    if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // Неважно, на какой стадии находится конвейер в начале.
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        // Используется для копирования.
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(
        old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    )
    {
        // Переход из макета назначения передачи в макет только для чтения шейдера.
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // От копирования к этапу ...
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        // Стадия фрагмента.
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        kerror("Function '%s': Unsupported layout transition.", __FUNCTION__);
        return;
    }

    vkCmdPipelineBarrier(command_buffer->handle, src_stage, dst_stage, 0, 0, null, 0, null, 1, &barrier);
}

void vulkan_image_copy_from_buffer(
    vulkan_context* context, texture_type type, vulkan_image* image, VkBuffer buffer, vulkan_command_buffer* command_buffer
)
{
    // Регион копирования.
    VkBufferImageCopy region;
    kzero_tc(&region, VkBufferImageCopy, 1);
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = type == TEXTURE_TYPE_CUBE ? 6 : 1;

    region.imageExtent.width = image->width;
    region.imageExtent.height = image->height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
        command_buffer->handle, buffer, image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region
    );
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

    // Указание системе об освобождении типа и размера памяти в Vulkan.
    kfree_report(image->memory_requirements.size, image->use_device_local ? MEMORY_TAG_GPU_LOCAL : MEMORY_TAG_VULKAN);
    // Делает изображение недействительным.
    kzero_tc(image, struct vulkan_image, 1);
}
