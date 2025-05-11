// Собственные подключения.
#include "renderer/vulkan/vulkan_swapchain.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_image.h"
#include "renderer/vulkan/vulkan_utils.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "systems/texture_system.h"

// Объявления функицй.
void create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain);
void destroy(vulkan_context* context, vulkan_swapchain* swapchain);

void vulkan_swapchain_create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* out_swapchain)
{
    create(context, width, height, out_swapchain);
}

void vulkan_swapchain_recreate(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain)
{
    destroy(context, swapchain);
    create(context, width, height, swapchain);
}

void vulkan_swapchain_destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{
    destroy(context, swapchain);

    for(u32 i = 0; i < swapchain->image_count; ++i)
    {
        kfree_tc(swapchain->render_textures[i]->internal_data, vulkan_image, 1, MEMORY_TAG_TEXTURE);
    }

    if(swapchain->render_textures)
    {
        u32 texture_count = swapchain->image_count;
        for(u32 i = 0; i < texture_count; ++i)
        {
            texture* t = swapchain->render_textures[i];
            if(t)
            {
                kfree_tc(t, texture, 1, MEMORY_TAG_TEXTURE);
            }
        }

        kfree_tc(swapchain->render_textures, texture*, swapchain->image_count, MEMORY_TAG_RENDERER);
    }
}

bool vulkan_swapchain_acquire_next_image_index(
    vulkan_context* context, vulkan_swapchain* swapchain, u64 timeout_ns, VkSemaphore image_available_semaphore,
    VkFence fence, u32* out_image_index
)
{
    VkResult result = vkAcquireNextImageKHR(
        context->device.logical, swapchain->handle, timeout_ns, image_available_semaphore, fence, out_image_index
    );

    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
        return false;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        kfatal("Funtcion '%s': Failed to acquire swapchain image!", __FUNCTION__);
        return false;
    }

    return true;
}

void vulkan_swapchain_present(
    vulkan_context* context, vulkan_swapchain* swapchain, VkSemaphore render_complete_semaphore, u32 present_image_index
)
{
    // Возвращение изображения в цепочку обмена для показа.
    VkPresentInfoKHR presentinfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentinfo.waitSemaphoreCount = 1;
    presentinfo.pWaitSemaphores = &render_complete_semaphore;
    presentinfo.swapchainCount = 1;
    presentinfo.pSwapchains = &swapchain->handle;
    presentinfo.pImageIndices = &present_image_index;
    presentinfo.pResults = null;

    vkDeviceWaitIdle(context->device.logical); // FIX: Проблема с синхронизацией семафоров (возникла после обновления версии vulkan).
                                               // TODO: Использование vkDeviceWaitIdle плохое решение, следует решить проблему с симафорами.

    VkResult result = vkQueuePresentKHR(context->device.present_queue.handle, &presentinfo);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
        kdebug("Function '%s': Swapchain recreated because swapchain returned out of date or suboptimal.", __FUNCTION__);
    }
    else if(result != VK_SUCCESS)
    {
        kfatal("Function '%s': Failed to present swapchain image!", __FUNCTION__);
    }

    // Циклическое увеличение текущего кадра.
    context->current_frame = (context->current_frame + 1) % swapchain->max_frames_in_flight;
}

void create(vulkan_context* context, u32 width, u32 height, vulkan_swapchain* swapchain)
{
    // Обновление информации о поддержке опций цепочкой обмена.
    vulkan_device_destroy_swapchian_support(&context->device.swapchain_support);
    vulkan_device_query_swapchain_support(context->device.physical, context->surface, &context->device.swapchain_support);

    // Выбор формата прикселей.
    swapchain->image_format = context->device.swapchain_support.formats[0];

    for(u32 i = 0; i < context->device.swapchain_support.format_count; ++i)
    {
        VkSurfaceFormatKHR format = context->device.swapchain_support.formats[i];

        // Предпочитаемый формат пикселей и цветовое пространство.
        if(format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchain->image_format = format;
            ktrace("Vulkan swapchain selected preferred format and color space.");
            break;
        }
    }

    // Выбор режима представления.
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    for(u32 i = 0; i < context->device.swapchain_support.present_mode_count; ++i)
    {
        VkPresentModeKHR mode = context->device.swapchain_support.present_modes[i];

        // Предпочитаемый режим представления.
        if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = mode;
            ktrace("Vulkan swapchain selected preferred present mode.");
            break;
        }
    }

    // Размеры изображения.
    VkExtent2D swapchain_extent = { width, height };
    if(context->device.swapchain_support.capabilities.currentExtent.width != U32_MAX)
    {
        swapchain_extent = context->device.swapchain_support.capabilities.currentExtent;
    }

    // Усечение значения к допусимым пределам для выбранного GPU.
    VkExtent2D min = context->device.swapchain_support.capabilities.minImageExtent;
    VkExtent2D max = context->device.swapchain_support.capabilities.maxImageExtent;
    swapchain_extent.width = KCLAMP(swapchain_extent.width, min.width, max.width);
    swapchain_extent.height = KCLAMP(swapchain_extent.height, min.height, max.height);
    ktrace("Vulkan swapchain image width: %d (%d..%d)", swapchain_extent.width, min.width, max.width);
    ktrace("Vulkan swapchain image height: %d (%d..%d)", swapchain_extent.height, min.height, max.height);

    u32 image_count_min = context->device.swapchain_support.capabilities.minImageCount;
    u32 image_count_max = context->device.swapchain_support.capabilities.maxImageCount;
    u32 image_count = image_count_min + 1;

    // NOTE: Если image_count_max = 0, тогда ограничений по верхнему пределу нет!
    if(image_count_max > 0 && image_count > image_count_max)
    {
        image_count = image_count_max;
        ktrace("Vulkan swapchain image count: %d (range: %d..%d).", image_count, image_count_min, image_count_max);
    }
    else
    {
        ktrace("Vulkan swapchain image count: %d (min: %d).", image_count, image_count_min);
    }

    swapchain->max_frames_in_flight = image_count - 1;

    VkSwapchainCreateInfoKHR swapchaininfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchaininfo.surface = context->surface;
    swapchaininfo.minImageCount = image_count;
    swapchaininfo.imageFormat = swapchain->image_format.format;
    swapchaininfo.imageColorSpace = swapchain->image_format.colorSpace;
    swapchaininfo.imageExtent = swapchain_extent;
    swapchaininfo.imageArrayLayers = 1; // NOTE: Всегда 1, а для стереоскопического изображения больше!
    swapchaininfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // NOTE: Непосредственно в изображения, для постобработки добавить VK_IMAGE_USAGE_TRANSFER_DST_BIT.

    // Указание каким образом обрабатывать цепочки обмена.
    if(context->device.graphics_queue.index != context->device.present_queue.index)
    {
        // Рисовать изображения в цепочке обмена из очереди графики, а затем отправлять их в очередь презентации!
        u32 queue_family_indices[] = {
            (u32)context->device.graphics_queue.index,
            (u32)context->device.present_queue.index
        };

        swapchaininfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchaininfo.queueFamilyIndexCount = 2;
        swapchaininfo.pQueueFamilyIndices = queue_family_indices;

        ktrace("Vulkan swapchain queue sharing mode is VK_SHARING_MODE_CONCURRENT.");
    }
    else
    {
        // Изображение принадлежит одному семейству очередей, и право собственности должно быть явно
        // передано перед его использованием в другом семействе очередей. Этот параметр обеспечивает
        // наилучшую производительность.
        swapchaininfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchaininfo.queueFamilyIndexCount = 0;
        swapchaininfo.pQueueFamilyIndices = null;

        ktrace("Vulkan swapchain queue sharing mode is VK_SHARING_MODE_EXCLUSIVE.");
    }

    swapchaininfo.preTransform = context->device.swapchain_support.capabilities.currentTransform;
    // Указывает использовать ли альфа-канал для смешивания с другими окнами в оконной системе.
    swapchaininfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchaininfo.presentMode = present_mode;
    // Обрезка скрытых пикселей другим окном, увеличивает производительность.
    swapchaininfo.clipped = VK_TRUE;
    // При изменении размера окна, создается новая цепочка обмена, а тут указывается ссылка на старую.
    swapchaininfo.oldSwapchain = null;
    
    VkResult result = vkCreateSwapchainKHR(context->device.logical, &swapchaininfo, context->allocator, &swapchain->handle);
    if(!vulkan_result_is_success(result))
    {
        kfatal(
            "Function '%s': Failed to create vulkan swapchain with result: %s.",
            __FUNCTION__, vulkan_result_get_string(result, true)
        );
    }

    // Установка нулевого индекса кадра.
    context->current_frame = 0;

    // Изображения цепочки.
    swapchain->image_count = 0;

    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical, swapchain->handle, &swapchain->image_count, null));

    if(!swapchain->render_textures)
    {
        swapchain->render_textures = kallocate_tc(texture*, swapchain->image_count, MEMORY_TAG_RENDERER);

        for(u32 i = 0; i < swapchain->image_count; ++i)
        {
            void* internal_data = kallocate_tc(vulkan_image, 1, MEMORY_TAG_TEXTURE);
            char tex_name[38] = "__internal_vulkan_swapchain_image_0__";
            tex_name[34] = '0' + (char)i;

            swapchain->render_textures[i] = texture_system_wrap_internal(
                tex_name, swapchain_extent.width, swapchain_extent.height, 4, false, true, false, internal_data
            );

            if(!swapchain->render_textures[i])
            {
                kfatal("Function '%s': Failed to generate new swapchain image texture!", __FUNCTION__);
                return;
            }
        }
    }
    else
    {
        for(u32 i = 0; i < swapchain->image_count; ++i)
        {
            // Просто обновляется размер текстуры.
            texture_system_resize(swapchain->render_textures[i], swapchain_extent.width, swapchain_extent.height, false);
        }
    }

    VkImage swapchain_images[32];
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical, swapchain->handle, &swapchain->image_count, swapchain_images));

    for(u32 i = 0; i < swapchain->image_count; ++i)
    {
        // Обновление внутренних данных (изображений).
        vulkan_image* image = swapchain->render_textures[i]->internal_data;
        image->handle = swapchain_images[i];
        image->width = swapchain_extent.width;
        image->height = swapchain_extent.height;

        // Представления изображений цепочки.
        VkImageViewCreateInfo viewinfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewinfo.image = image->handle;
        viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewinfo.format = swapchain->image_format.format;
        viewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewinfo.subresourceRange.baseMipLevel = 0;
        viewinfo.subresourceRange.levelCount = 1;
        viewinfo.subresourceRange.baseArrayLayer = 0;
        viewinfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(context->device.logical, &viewinfo, context->allocator, &image->view);
        if(!vulkan_result_is_success(result))
        {
            kfatal(
                "Function '%s': Failed to create swapchain image views with result: %s",
                __FUNCTION__, vulkan_result_get_string(result, true)
            );
        }
    }

    // Получение формата буфера глубины.
    if(!vulkan_device_detect_depth_format(&context->device))
    {
        context->device.depth_format = VK_FORMAT_UNDEFINED;
        kerror("Function '%s': Failed to find a supported depth format.", __FUNCTION__);
    }

    // Создание буфера глубины (изображение и его представление).
    vulkan_image* image = kallocate_tc(vulkan_image, 1, MEMORY_TAG_TEXTURE); // TODO: Можно оптимизировать, что бы не создавать часто!
    vulkan_image_create(
        context, TEXTURE_TYPE_2D, swapchain_extent.width, swapchain_extent.height, context->device.depth_format,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true, VK_IMAGE_ASPECT_DEPTH_BIT, image
    );

    // TODO: Можно оптимизировать, что бы не создавать часто!
    swapchain->depth_texture = texture_system_wrap_internal(
        "__default_depth_texture__", swapchain_extent.width, swapchain_extent.height, context->device.depth_channel_count,
        false, true, false, image
    );

    // TODO: Обновить размер и изменить internal_data в качестве оптимизации.
}

void destroy(vulkan_context* context, vulkan_swapchain* swapchain)
{
    vkDeviceWaitIdle(context->device.logical);

    vulkan_image_destroy(context, swapchain->depth_texture->internal_data);
    kfree_tc(swapchain->depth_texture->internal_data, vulkan_image, 1, MEMORY_TAG_TEXTURE);
    swapchain->depth_texture->internal_data = null;
    kfree_tc(swapchain->depth_texture, texture, 1, MEMORY_TAG_TEXTURE);
    swapchain->depth_texture = null;

    // Уничтожить только представление, но не изображения, поскольку они принадлежат цепочке
    // обмена и, таким образом, уничтожаются при ее изменении.
    for(u32 i = 0; i < swapchain->image_count; ++i)
    {
        vulkan_image* image = swapchain->render_textures[i]->internal_data;
        vkDestroyImageView(context->device.logical, image->view, context->allocator);
    }

    vkDestroySwapchainKHR(context->device.logical, swapchain->handle, context->allocator);
}
